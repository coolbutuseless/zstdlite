



#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "zstd.h"
#include "utils.h"

#include "dictionaries.h"


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Extract zstd info from file, raw vec or connection
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP zstd_info_(SEXP src_) {
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Unpack data
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  unsigned char *src;
  size_t src_size;
  
  if (TYPEOF(src_) == STRSXP) {
    const char *filename = CHAR(STRING_ELT(src_, 0));
    size_t max_bytes = 18; // maximum size of frame header: ZSTD_frameHeaderSize_max
    src = read_partial_file(filename, max_bytes, &src_size);
  } else if (TYPEOF(src_) == RAWSXP) {
    src      = RAW(src_);
    src_size = (size_t)length(src_);
  } else {
    error("zstd_info_() currently only accepts raw vectors or filenames");
  }
  
  if (src_size < 18) {
    // warning("zstd_info_() probably not Zstandard compressed data");
    return R_NilValue;
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Retrieve frameHeader
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // typedef struct {
  //   unsigned long long frameContentSize; /* if == ZSTD_CONTENTSIZE_UNKNOWN, it means this field is not available. 0 means "empty" */
  //   unsigned long long windowSize;       /* can be very large, up to <= frameContentSize */
  //   unsigned blockSizeMax;
  //   ZSTD_frameType_e frameType;          /* if == ZSTD_skippableFrame, frameContentSize is the size of skippable content */
  //   unsigned headerSize;
  //   unsigned dictID;
  //   unsigned checksumFlag;
  //   unsigned _reserved1;
  //   unsigned _reserved2;
  // } ZSTD_frameHeader;
  ZSTD_frameHeader fh;
  size_t res = ZSTD_getFrameHeader(&fh, src, src_size);
  if (ZSTD_isError(res)) {
    // warning("zstd_info_() probably not Zstandard compressed data (Error: %s)", ZSTD_getErrorName(res));
    return R_NilValue;
  }
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Create a list to return
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define NINFO 4
  SEXP res_ = PROTECT(allocVector(VECSXP, NINFO));
  SEXP nms_ = PROTECT(allocVector(STRSXP, NINFO));
  
  SET_VECTOR_ELT(res_, 0, ScalarReal((double)fh.frameContentSize));
  SET_STRING_ELT(nms_, 0, mkChar("uncompressed_size"));
  
  SET_VECTOR_ELT(res_, 1, ScalarInteger(src_size));
  SET_STRING_ELT(nms_, 1, mkChar("compressed_size"));
  
  SET_VECTOR_ELT(res_, 2, ScalarInteger((int)fh.dictID));
  SET_STRING_ELT(nms_, 2, mkChar("dict_id"));
  
  SET_VECTOR_ELT(res_, 3, ScalarLogical((int)fh.checksumFlag));
  SET_STRING_ELT(nms_, 3, mkChar("has_checksum"));

  setAttrib(res_, R_NamesSymbol, nms_);
  

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Tidy and return
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  UNPROTECT(2);
  return res_;
}

