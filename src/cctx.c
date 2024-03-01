



#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "zstd.h"
#include "cctx.h"
#include "utils.h"



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ZSTD_c_stableInBuffer
// Experimental parameter.
// Default is 0 == disabled. Set to 1 to enable.
// 
// Tells the compressor that input data presented with ZSTD_inBuffer
// will ALWAYS be the same between calls.
// Technically, the @src pointer must never be changed,
// and the @pos field can only be updated by zstd.
// However, it's possible to increase the @size field,
// allowing scenarios where more data can be appended after compressions starts.
// These conditions are checked by the compressor,
// and compression will fail if they are not respected.
// Also, data in the ZSTD_inBuffer within the range [src, src + pos)
// MUST not be modified during compression or it will result in data corruption.
// 
// When this flag is enabled zstd won't allocate an input window buffer,
// because the user guarantees it can reference the ZSTD_inBuffer until
// the frame is complete. But, it will still allocate an output buffer
// large enough to fit a block (see ZSTD_c_stableOutBuffer). This will also
// avoid the memcpy() from the input buffer to the input window buffer.
// 
// NOTE: So long as the ZSTD_inBuffer always points to valid memory, using
// this flag is ALWAYS memory safe, and will never access out-of-bounds
// memory. However, compression WILL fail if conditions are not respected.
// 
// WARNING: The data in the ZSTD_inBuffer in the range [src, src + pos) MUST
// not be modified during compression or it will result in data corruption.
// This is because zstd needs to reference data in the ZSTD_inBuffer to find
// matches. Normally zstd maintains its own window buffer for this purpose,
// but passing this flag tells zstd to rely on user provided buffer instead.
// 
// ZSTD_c_stableOutBuffer
// Experimental parameter.
// Default is 0 == disabled. Set to 1 to enable.
// 
// Tells he compressor that the ZSTD_outBuffer will not be resized between
// calls. Specifically: (out.size - out.pos) will never grow. This gives the
// compressor the freedom to say: If the compressed data doesn't fit in the
// output buffer then return ZSTD_error_dstSizeTooSmall. This allows us to
// always decompress directly into the output buffer, instead of decompressing
// into an internal buffer and copying to the output buffer.
// 
// When this flag is enabled zstd won't allocate an output buffer, because
// it can write directly to the ZSTD_outBuffer. It will still allocate the
// input window buffer (see ZSTD_c_stableInBuffer).
// 
// Zstd will check that (out.size - out.pos) never grows and return an error
// if it does. While not strictly necessary, this should prevent surprises.
// 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void cctx_set_stable_buffers(ZSTD_CCtx *cctx) {
  
  size_t res = ZSTD_CCtx_setParameter(cctx, ZSTD_c_stableInBuffer, 1);
  if (ZSTD_isError(res)) {
    error("cctx_set_stable_buffers() could not set 'ZSTD_c_stableInBuffer'");
  }
  
  res = ZSTD_CCtx_setParameter(cctx, ZSTD_c_stableOutBuffer, 1);
  if (ZSTD_isError(res)) {
    error("cctx_set_stable_buffers() could not set 'ZSTD_c_stableOutBuffer'");
  }
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Initialise a cctx from C
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ZSTD_CCtx *init_cctx(int level, int num_threads, int include_checksum, int stable_buffers) {
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Create context
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  ZSTD_CCtx *cctx = ZSTD_createCCtx();
  if (cctx == NULL) {
    error("init_cctx(): Couldn't initialse memory for 'cctx'");
  }
  
  size_t res;
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Add checksum?
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (include_checksum) {
    res = ZSTD_CCtx_setParameter(cctx, ZSTD_c_checksumFlag, 1);
    if (ZSTD_isError(res)) {
      error("init_cctx(): Couldn't set checksum flag");  
    }
  }
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Set compression level
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  level = level < -5 ? -5 : level;
  level = level > 22 ? 22 : level;
  res = ZSTD_CCtx_setParameter(cctx, ZSTD_c_compressionLevel, level);
  if (ZSTD_isError(res)) {
    error("init_cctx(): Bad compression level");  
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Initialise multi-threading if asked
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (num_threads > 1) {
    res = ZSTD_CCtx_setParameter(cctx, ZSTD_c_nbWorkers, num_threads);
    if (ZSTD_isError(res)) {
      Rprintf ("init_cctx(): Included zstd library doesn't support multithreading. "
                 "Reverting to single-thread mode. \n");
    }
  }
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Stable buffers?
  //   - streaming compression uses *UNSTABLE* buffers
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (stable_buffers) {
    cctx_set_stable_buffers(cctx);
  }
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Controls whether the new and experimental "dedicated dictionary search
  // structure" can be used. This feature is still rough around the edges, be
  // prepared for surprising behavior!
  // 
  // How to use it:
  // 
  // When using a CDict, whether to use this feature or not is controlled at
  // CDict creation, and it must be set in a CCtxParams set passed into that
  // construction (via ZSTD_createCDict_advanced2()). A compression will then
  // use the feature or not based on how the CDict was constructed; the value of
  // this param, set in the CCtx, will have no effect.
  // 
  // However, when a dictionary buffer is passed into a CCtx, such as via
  // ZSTD_CCtx_loadDictionary(), this param can be set on the CCtx to control
  // whether the CDict that is created internally can use the feature or not.
  // 
  // What it does:
  // 
  // Normally, the internal data structures of the CDict are analogous to what
  // would be stored in a CCtx after compressing the contents of a dictionary.
  // To an approximation, a compression using a dictionary can then use those
  // data structures to simply continue what is effectively a streaming
  // compression where the simulated compression of the dictionary left off.
  // Which is to say, the search structures in the CDict are normally the same
  // format as in the CCtx.
  // 
  // It is possible to do better, since the CDict is not like a CCtx: the search
  // structures are written once during CDict creation, and then are only read
  // after that, while the search structures in the CCtx are both read and
  // written as the compression goes along. This means we can choose a search
  // structure for the dictionary that is read-optimized.
  // 
  // This feature enables the use of that different structure.
  // 
  // Note that some of the members of the ZSTD_compressionParameters struct have
  // different semantics and constraints in the dedicated search structure. It is
  // highly recommended that you simply set a compression level in the CCtxParams
  // you pass into the CDict creation call, and avoid messing with the cParams
  // directly.
  // 
  // Effects:
  // 
  // This will only have any effect when the selected ZSTD_strategy
  // implementation supports this feature. Currently, that's limited to
  // ZSTD_greedy, ZSTD_lazy, and ZSTD_lazy2.
  // 
  // Note that this means that the CDict tables can no longer be copied into the
  // CCtx, so the dict attachment mode ZSTD_dictForceCopy will no longer be
  // usable. The dictionary can only be attached or reloaded.
  // 
  // In general, you should expect compression to be faster--sometimes very much
  // so--and CDict creation to be slightly slower. Eventually, we will probably
  // make this mode the default.
  // 
  //
  // 2024-02-27 Didn't seem to have any effect for current zstdlite cases. 
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // res = ZSTD_CCtx_setParameter(cctx, ZSTD_c_enableDedicatedDictSearch, 1);
  // if (ZSTD_isError(res)) {
  //   error("init_cctx() could not set 'ZSTD_c_enableDedicatedDictSearch'");
  // }
  
  
  return cctx;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Unpack an R external pointer to a C pointer 'ZSTD_CCtx *'
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ZSTD_CCtx * external_ptr_to_zstd_cctx(SEXP cctx_) {
  if (TYPEOF(cctx_) == EXTPTRSXP) {
    ZSTD_CCtx *cctx = (ZSTD_CCtx *)R_ExternalPtrAddr(cctx_);
    if (cctx != NULL) {
      return cctx;
    }
  }
  
  error("ZSTD_CCtx pointer is invalid/NULL.");
  return NULL;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Finalizer for a 'ZSTD_CCtx' object.
//
// This function will be called when 'cctx' object gets 
// garbage collected.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static void zstd_cctx_finalizer(SEXP cctx_) {
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Unpack the pointer 
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  ZSTD_CCtx *cctx = (ZSTD_CCtx *) R_ExternalPtrAddr(cctx_);
  if (cctx == NULL) {
    Rprintf("NULL ZSTD_CCtx in finalizer");
    return;
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Free ZSTD_Cctx pointer, Clear pointer to guard against re-use
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  ZSTD_freeCCtx(cctx);
  R_ClearExternalPtr(cctx_);
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Initialize a ZSTD_CCtx pointer from R
// @param dict could be a raw vector holding a dictionary or a filename
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP init_cctx_(SEXP level_, SEXP num_threads_, SEXP include_checksum_, SEXP dict_) {
  
  ZSTD_CCtx* cctx = init_cctx(
    asInteger(level_), 
    asInteger(num_threads_), 
    asLogical(include_checksum_),
    0  // StableBuffers
  );
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Handle dictionaries
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (!isNull(dict_)) {
    size_t status;
    if (TYPEOF(dict_) == RAWSXP) {
      status = ZSTD_CCtx_loadDictionary(cctx, RAW(dict_), (size_t)length(dict_));
    } else if (TYPEOF(dict_) == STRSXP) {
      const char *filename = CHAR(STRING_ELT(dict_, 0));
      size_t fsize;
      unsigned char *dict = read_file(filename, &fsize);
      status = ZSTD_CCtx_loadDictionary(cctx, dict, fsize);
      free(dict);
    } else {
      error("init_cctx(): 'dict' must be a raw vector or a filename");
    }
    if (ZSTD_isError(status)) {
      error("init_cctx(): Error initialising dict. %s", ZSTD_getErrorName(status));
    }
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Wrap 'cctx' as an R external pointer
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP cctx_ = PROTECT(R_MakeExternalPtr(cctx, R_NilValue, R_NilValue));
  R_RegisterCFinalizer(cctx_, zstd_cctx_finalizer);
  Rf_setAttrib(cctx_, R_ClassSymbol, Rf_mkString("ZSTD_CCtx"));
  
  
  UNPROTECT(1);
  return cctx_;
}

