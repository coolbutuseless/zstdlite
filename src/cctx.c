



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
// Initialise a cctx from C
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ZSTD_CCtx *init_cctx(int level, int num_threads, int stable_buffers) {
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Create context
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  ZSTD_CCtx *cctx = ZSTD_createCCtx();
  if (cctx == NULL) {
    error("init_cctx(): Couldn't initialse memory for 'cctx'");
  }
  
  size_t res;
  
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
  if (stable_buffers) {
    int res = ZSTD_CCtx_setParameter(cctx, ZSTD_c_stableInBuffer, 1);
    if (ZSTD_isError(res)) {
      error("init_cctx() could not set 'ZSTD_c_stableInBuffer'");
    }
    
    res = ZSTD_CCtx_setParameter(cctx, ZSTD_c_stableOutBuffer, 1);
    if (ZSTD_isError(res)) {
      error("init_cctx() could not set 'ZSTD_c_stableOutBuffer'");
    }
  }
  
  
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
SEXP init_cctx_(SEXP level_, SEXP num_threads_, SEXP dict_) {
  
  ZSTD_CCtx* cctx = init_cctx(asInteger(level_), asInteger(num_threads_), 0);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Handle dictionaries
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (!isNull(dict_)) {
    size_t status;
    if (TYPEOF(dict_) == RAWSXP) {
      status = ZSTD_CCtx_loadDictionary(cctx, RAW(dict_), length(dict_));
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

