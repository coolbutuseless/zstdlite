



#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "zstd.h"
#include "dctx.h"
#include "utils.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Unpack an R external pointer to a C pointer 'ZSTD_DCtx *'
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ZSTD_DCtx * external_ptr_to_zstd_dctx(SEXP dctx_) {
  if (TYPEOF(dctx_) == EXTPTRSXP) {
    ZSTD_DCtx *dctx = (ZSTD_DCtx *)R_ExternalPtrAddr(dctx_);
    if (dctx != NULL) {
      return dctx;
    }
  }
  
  error("ZSTD_DCtx pointer is invalid/NULL.");
  return NULL;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Finalizer for a 'ZSTD_DCtx' object.
//
// This function will be called when 'dctx' object gets 
// garbage collected.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static void zstd_dctx_finalizer(SEXP dctx_) {
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Unpack the pointer 
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  ZSTD_DCtx *dctx = (ZSTD_DCtx *) R_ExternalPtrAddr(dctx_);
  if (dctx == NULL) {
    Rprintf("NULL ZSTD_DCtx in finalizer");
    return;
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Free ZSTD_Cctx pointer, Clear pointer to guard against re-use
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  ZSTD_freeDCtx(dctx);
  R_ClearExternalPtr(dctx_);
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Initialize a ZSTD_DCtx from C
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ZSTD_DCtx *init_dctx(int stable_buffers) {
  ZSTD_DCtx *dctx = ZSTD_createDCtx();
  if (dctx == NULL) {
    error("init_dctx(): Couldn't initialse memory for 'dctx'");
  }
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // ZSTD v1.5.5
  // ZSTD_d_stableOutBuffer
  // Experimental parameter.
  // Default is 0 == disabled. Set to 1 to enable.
  // 
  // Tells the decompressor that the ZSTD_outBuffer will ALWAYS be the same
  // between calls, except for the modifications that zstd makes to pos (the
  // caller must not modify pos). This is checked by the decompressor, and
  // decompression will fail if it ever changes. Therefore the ZSTD_outBuffer
  // MUST be large enough to fit the entire decompressed frame. This will be
  // checked when the frame content size is known. The data in the ZSTD_outBuffer
  // in the range [dst, dst + pos) MUST not be modified during decompression
  // or you will get data corruption.
  // 
  // When this flag is enabled zstd won't allocate an output buffer, because
  // it can write directly to the ZSTD_outBuffer, but it will still allocate
  // an input buffer large enough to fit any compressed block. This will also
  // avoid the memcpy() from the internal output buffer to the ZSTD_outBuffer.
  // If you need to avoid the input buffer allocation use the buffer-less
  // streaming API.
  // 
  // NOTE: So long as the ZSTD_outBuffer always points to valid memory, using
  // this flag is ALWAYS memory safe, and will never access out-of-bounds
  // memory. However, decompression WILL fail if you violate the preconditions.
  // 
  // WARNING: The data in the ZSTD_outBuffer in the range [dst, dst + pos) MUST
  // not be modified during decompression or you will get data corruption. This
  // is because zstd needs to reference data in the ZSTD_outBuffer to regenerate
  // matches. Normally zstd maintains its own buffer for this purpose, but passing
  // this flag tells zstd to use the user provided buffer.
  // 
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (stable_buffers) {
    size_t res = ZSTD_DCtx_setParameter(dctx, ZSTD_d_stableOutBuffer, 1);
    if (ZSTD_isError(res)) {
      error("init_dctx(): Could not set 'ZSTD_d_stableOutBuffer'");
    }
  }
  
  
  return dctx;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Initialize a ZSTD_DCtx pointer from R
// @param dict could be a raw vector holding a dictionary or a filename
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP init_dctx_(SEXP dict_) {

  ZSTD_DCtx *dctx = init_dctx(0);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Handle dictionary
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (!isNull(dict_)) {
    size_t status;
    if (TYPEOF(dict_) == RAWSXP) {
      status = ZSTD_DCtx_loadDictionary(dctx, RAW(dict_), (size_t)length(dict_));
    } else if (TYPEOF(dict_) == STRSXP) {
      const char *filename = CHAR(STRING_ELT(dict_, 0));
      size_t fsize;
      unsigned char *dict = read_file(filename, &fsize);
      status = ZSTD_DCtx_loadDictionary(dctx, dict, fsize);
      free(dict);
    } else {
      error("init_dctx(): 'dict' must be a raw vector or a filename");
    }
    if (ZSTD_isError(status)) {
      error("init_dctx(): Error initialising dict. %s", ZSTD_getErrorName(status));
    }
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Wrap 'dctx' into an R external pointer
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP dctx_ = PROTECT(R_MakeExternalPtr(dctx, R_NilValue, R_NilValue));
  R_RegisterCFinalizer(dctx_, zstd_dctx_finalizer);
  Rf_setAttrib(dctx_, R_ClassSymbol, Rf_mkString("ZSTD_DCtx"));
  
  
  UNPROTECT(1);
  return dctx_;
}

