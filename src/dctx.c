



#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "zstd/zstd.h"
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
// NOTE: The user doesn't get to set this option
//       It is set by 'zstdlite' C functions depeneding on whether streaming 
//       is used or not.
//       i.e. streaming buffers are NOT stable
//
//
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
void dctx_set_stable_buffers(ZSTD_DCtx *dctx) {
  size_t res = ZSTD_DCtx_setParameter(dctx, ZSTD_d_stableOutBuffer, 1);
  if (ZSTD_isError(res)) {
    error("zstd_decompress_(): Could not set 'ZSTD_d_stableOutBuffer'");
  }
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ZSTD_DCtx *init_dctx_with_opts(SEXP opts_, int stable_buffers) {
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Defaults
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP dict_ = R_NilValue;
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Init DCtx
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  ZSTD_DCtx *dctx = ZSTD_createDCtx();
  if (dctx == NULL) {
    error("init_dctx(): Couldn't initialse memory for 'dctx'");
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Stable buffers?
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (stable_buffers) {
    dctx_set_stable_buffers(dctx);
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Short circuit if no opts
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (length(opts_) == 0) {
    return dctx;
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Sanity check
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (!isNewList(opts_)) {
    error("'opts_' must be a list");
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Unpack names
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP nms_ = getAttrib(opts_, R_NamesSymbol);
  if (isNull(nms_)) {
    error("'opts_' must be a named list");
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Parse options from user
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  for (int i = 0; i < length(opts_); i++) {
    const char *opt_name = CHAR(STRING_ELT(nms_, i));
    SEXP val_ = VECTOR_ELT(opts_, i);
    
    if (strcmp(opt_name, "validate_checksum") == 0) {
      int validate_checksum = asInteger(val_);
      if (!validate_checksum) {
        size_t res = ZSTD_DCtx_setParameter(dctx, ZSTD_d_forceIgnoreChecksum, 1);
        if (ZSTD_isError(res)) {
          error("init_dctx(): Could not set 'ZSTD_d_forceIgnoreChecksum'");
        } 
      }
    } else if (strcmp(opt_name, "dict") == 0) {
      dict_ = val_;
    } else {
      warning("init_dctx(): Unknown option '%s'", opt_name);
    }
  }
  
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
  
  return dctx;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Initialize a ZSTD_DCtx pointer from R
// @param dict could be a raw vector holding a dictionary or a filename
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP init_dctx_(SEXP opts_) {

  ZSTD_DCtx *dctx = init_dctx_with_opts(opts_, 0);  // Assume NOT stable buffers by default
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Wrap 'dctx' into an R external pointer
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP dctx_ = PROTECT(R_MakeExternalPtr(dctx, R_NilValue, R_NilValue));
  R_RegisterCFinalizer(dctx_, zstd_dctx_finalizer);
  Rf_setAttrib(dctx_, R_ClassSymbol, Rf_mkString("ZSTD_DCtx"));
  
  
  UNPROTECT(1);
  return dctx_;
}





//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Get the context settings
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP get_dctx_settings_(SEXP dctx_) {
  ZSTD_DCtx *dctx = external_ptr_to_zstd_dctx(dctx_);
  
  SEXP res_ = PROTECT(allocVector(VECSXP, 1));
  
  int validate_checksum;
  ZSTD_DCtx_getParameter(dctx, ZSTD_d_forceIgnoreChecksum, &validate_checksum);
  
  SET_VECTOR_ELT(res_, 0, ScalarLogical(validate_checksum));
  
  SEXP nms_ = PROTECT(allocVector(STRSXP, 1));
  SET_STRING_ELT(nms_, 0, mkChar("validate_checksum"));
  
  setAttrib(res_, R_NamesSymbol, nms_);
  
  UNPROTECT(2);
  return res_;
}
