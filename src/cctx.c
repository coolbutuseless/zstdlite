



#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "zstd.h"
#include "cctx.h"

ZSTD_CCtx *init_cctx(int level, int num_threads) {
  
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
  // Initialise multithreads if asked
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (num_threads > 1) {
    res = ZSTD_CCtx_setParameter(cctx, ZSTD_c_nbWorkers, num_threads);
    if (ZSTD_isError(res)) {
      Rprintf ("init_cctx(): Included zstd library doesn't support multithreading. "
                 "Reverting to single-thread mode. \n");
    }
  }
  
  
  return cctx;
}




//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Unpack an external pointer to a C 'ZSTD_CCtx *'
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ZSTD_CCtx * external_ptr_to_zstd_context(SEXP cctx_) {
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
static void zstd_context_finalizer(SEXP cctx_) {
  
  // Rprintf("zstd_context_finalizer !!!!\n");
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Unpack the pointer 
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  ZSTD_CCtx *cctx = (ZSTD_CCtx *) R_ExternalPtrAddr(cctx_);
  if (cctx == NULL) {
    Rprintf("NULL ZSTD_CCtx in finalizer");
    return;
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Free ZSTD_cctx pointer, Clear pointer to guard against re-use
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  ZSTD_freeCCtx(cctx);
  R_ClearExternalPtr(cctx_);
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Serialize an R object to a buffer of fixed size and then compress
// the buffer using zstd
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP init_cctx_(SEXP compressionLevel_, SEXP num_threads_) {

  ZSTD_CCtx* cctx = init_cctx(asInteger(compressionLevel_), asInteger(num_threads_));

  SEXP cctx_ = PROTECT(R_MakeExternalPtr(cctx, R_NilValue, R_NilValue));
  R_RegisterCFinalizer(cctx_, zstd_context_finalizer);
  Rf_setAttrib(cctx_, R_ClassSymbol, Rf_mkString("ZSTD_CCtx"));
  UNPROTECT(1);
  
  return cctx_;
}

