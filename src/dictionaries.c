



#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#include "zstd.h"
#include "zdict.h"
#include "utils.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Retrieve the ID of the dictionary.  Returns zero if not a dictionary
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP zstd_dict_id_(SEXP src_) {
  
  // ZDICTLIB_API unsigned ZDICT_getDictID(const void* dictBuffer, size_t dictSize);
  // ZSTDLIB_API unsigned ZSTD_getDictID_fromFrame(const void* src, size_t srcSize);
  void *src;
  size_t src_size;
  char buf[ZSTD_FRAMEHEADERSIZE_MAX];
  
  if (TYPEOF(src_) == RAWSXP) {
    src = (void *)RAW(src_);
    src_size = (size_t)length(src_);
  } else if (TYPEOF(src_) == STRSXP) {
    const char *filename = CHAR(STRING_ELT(src_, 0));
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
      error("zstd_dict_id_for_buffer_() couldn't open file '%s'", filename);
    }
    size_t bytes_read = fread(buf, 1, sizeof(buf), fp);
    fclose(fp);
    src = buf;
    src_size = bytes_read;
  } else {
    error("zstd_dict_id_for_buffer_(): Currently only supports raw vector input");
  } 
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Try and get DictID as if this was a compressed buffer,
  // If this fails (id == 0), then try and get Dict ID as if this was a dictionary
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  uint32_t id = ZSTD_getDictID_fromFrame(src, src_size);
  if (id == 0) {
    id = ZDICT_getDictID(src, src_size);
  }
  
  return ScalarInteger((int32_t)id);
}




//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Train a dictionary
// ZDICTLIB_API size_t ZDICT_trainFromBuffer(void* dictBuffer, 
//                                           size_t dictBufferCapacity,
//                                           const void* samplesBuffer,
//                                           const size_t* samplesSizes, 
//                                           unsigned nbSamples);
//
// 'dictBuffer' is pre-malloced memory of size 'dictBufferCapacity'
// 'samplesBuffer' contains data for 'nbSamples' of the training data, 
//    all concatendated together.
// 'samplesSizes' is an integer vector (of length 'nbSamples') which contains
//    the actual lengths of all the individual samples (so that zstd can
//    unpack them and learn from them one-by-one)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP zstd_train_dictionary_(SEXP samples_, SEXP size_, SEXP optim_, SEXP optim_shrink_allow_) {
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Unpack and sanity check args
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (!isNewList(samples_)) {
    error("zstd_train_dictionary(): samples must be provided as a list of raw vectors or character strings");
  }
  
  size_t dictBufferCapacity = (size_t)asInteger(size_);
  uint32_t nbSamples = (uint32_t)length(samples_);
  
  if (nbSamples == 0) {
    error("zstd_train_dictionary(): No samples provided");
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // 'samples_' must be a list containing raw vectors.
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  size_t total_len = 0;
  for (uint32_t i = 0; i < nbSamples; i++) {
    SEXP elem_ = VECTOR_ELT(samples_, i);
    if (TYPEOF(elem_) == RAWSXP) {
      if (length(elem_) < 8) {
        error("zstd_train_dictionary(): When samples are raw vectors, all vector lengths must be >= 8 bytes");
      }
      total_len += (size_t)length(elem_);
    } else if (TYPEOF(elem_) == STRSXP) {
      if (length(elem_) != 1) {
        warning("zstd_train_dictionary(): When samples are a list of character vectors, each vector must only contain a single string");
      }
      total_len += (size_t)strlen(CHAR(STRING_ELT(elem_, 0)));
    }
  }
  
  if (total_len < 100 * dictBufferCapacity) {
    warning("zstd_train_dictionary() ZSTD documentation recommends training data size 100x dictionary size.\nOnly supplied with %.1fx", (double)total_len / (double)dictBufferCapacity);
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Allocate
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  unsigned char *samplesBuffer = (unsigned char *)malloc(total_len);
  if (samplesBuffer == NULL) {
    error("zstd_train_dictionary(): Could not allocate %zu bytes for 'samplesBuffer'", total_len);
  }
  size_t *samplesSizes = (size_t *)calloc(nbSamples, sizeof(size_t));
  if (samplesSizes == NULL) {
    error("zstd_train_dictionary(): Could not allocate %i * %zu = %zu bytes for 'samplesSizes'", nbSamples, sizeof(size_t), nbSamples * sizeof(size_t));
  }

  SEXP dictBuffer_ = PROTECT(allocVector(RAWSXP, (R_xlen_t)dictBufferCapacity));
  unsigned char *dictBuffer = (unsigned char *)RAW(dictBuffer_);

  size_t pos = 0;
  for (uint32_t i = 0; i < length(samples_); i++) {
    SEXP elem_ = VECTOR_ELT(samples_, i);
    if (TYPEOF(elem_) == RAWSXP) {
      size_t len = (size_t)length(elem_);
      samplesSizes[i] = len;
      memcpy(samplesBuffer + pos, RAW(elem_), len);
      pos += len;
    } else if (TYPEOF(elem_) == STRSXP) {
      const char *tmp = CHAR(STRING_ELT(elem_, 0));
      size_t len = (size_t)strlen(tmp);
      samplesSizes[i] = len;
      memcpy(samplesBuffer + pos, (void *)tmp, len);
      pos += len;
    }
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Train
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  size_t actual_dict_size;
  
  if (!asLogical(optim_)) {
    actual_dict_size  = ZDICT_trainFromBuffer((void *)dictBuffer, dictBufferCapacity, (void *)samplesBuffer, samplesSizes, nbSamples);
  } else {
    
    // typedef struct {
    //   unsigned k;                  /* Segment size : constraint: 0 < k : Reasonable range [16, 2048+] */
    //   unsigned d;                  /* dmer size : constraint: 0 < d <= k : Reasonable range [6, 16] */
    //   unsigned steps;              /* Number of steps : Only used for optimization : 0 means default (40) : Higher means more parameters checked */
    //   unsigned nbThreads;          /* Number of threads : constraint: 0 < nbThreads : 1 means single-threaded : Only used for optimization : Ignored if ZSTD_MULTITHREAD is not defined */
    //   double splitPoint;           /* Percentage of samples used for training: Only used for optimization : the first nbSamples * splitPoint samples will be used to training, the last nbSamples * (1 - splitPoint) samples will be used for testing, 0 means default (1.0), 1.0 when all samples are used for both training and testing */
    //   unsigned shrinkDict;         /* Train dictionaries to shrink in size starting from the minimum size and selects the smallest dictionary that is shrinkDictMaxRegression% worse than the largest dictionary. 0 means no shrinking and 1 means shrinking  */
    //   unsigned shrinkDictMaxRegression; /* Sets shrinkDictMaxRegression so that a smaller dictionary can be at worse shrinkDictMaxRegression% worse than the max dict size dictionary. */
    //   ZDICT_params_t zParams;
    // } ZDICT_cover_params_t;
    //
    // ZDICT_optimizeTrainFromBuffer_cover():
    // The same requirements as above hold for all the parameters except `parameters`.
    // This function tries many parameter combinations and picks the best parameters.
    // `*parameters` is filled with the best parameters found,
    // dictionary constructed with those parameters is stored in `dictBuffer`.
    // 
    // All of the parameters d, k, steps are optional.
    // If d is non-zero then we don't check multiple values of d, otherwise we check d = {6, 8}.
    // if steps is zero it defaults to its default value.
    // If k is non-zero then we don't check multiple values of k, otherwise we check steps values in [50, 2000].
    // 
    // @return: size of dictionary stored into `dictBuffer` (<= `dictBufferCapacity`)
    //          or an error code, which can be tested with ZDICT_isError().
    //          On success `*parameters` contains the parameters selected.
    //          See ZDICT_trainFromBuffer() for details on failure modes.
    // Note: ZDICT_optimizeTrainFromBuffer_cover() requires about 8 bytes of memory for each input byte and additionally another 5 bytes of memory for each byte of memory for each thread.
    // 
    // ZDICTLIB_STATIC_API size_t ZDICT_optimizeTrainFromBuffer_cover(
    //     void* dictBuffer, size_t dictBufferCapacity,
    //     const void* samplesBuffer, const size_t* samplesSizes, unsigned nbSamples,
    //     ZDICT_cover_params_t* parameters);
    //
    ZDICT_cover_params_t params;
    memset(&params, 0, sizeof(params));
    uint32_t optim_shrink_allow  = (uint32_t)asInteger(optim_shrink_allow_);
    if (optim_shrink_allow > 0) {
      params.shrinkDict = 1;
      params.shrinkDictMaxRegression = optim_shrink_allow;
    }    
    actual_dict_size = ZDICT_optimizeTrainFromBuffer_cover(
      dictBuffer, dictBufferCapacity,
      samplesBuffer, samplesSizes, (uint32_t)nbSamples, &params);
  }
  
  if (ZDICT_isError(actual_dict_size)) {
    free(samplesBuffer);
    free(samplesSizes);
    UNPROTECT(1);
    error("zstd_train_dictionary() Training error %s", ZDICT_getErrorName(actual_dict_size));
  }
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // The size of the 'dict' may be less than the full capacity of the raw vector
  // allocated to hold it.
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (actual_dict_size < dictBufferCapacity) {
    // Rprintf("zstd_train_dictionary() Note: dict only used %i / %i bytes\n", actual_dict_size, dictBufferCapacity);
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Truncate the user-viewable size of the RAW vector
    // Requires: R_VERSION >= R_Version(3, 4, 0)
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    SETLENGTH(dictBuffer_, (R_xlen_t)actual_dict_size);
    SET_TRUELENGTH(dictBuffer_, (R_xlen_t)dictBufferCapacity);
    SET_GROWABLE_BIT(dictBuffer_);
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Tidy and return
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  free(samplesBuffer);
  free(samplesSizes);
  UNPROTECT(1);
  return dictBuffer_;
}







