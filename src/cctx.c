



#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "zstd.h"
#include "cctx.h"


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Initialise a cctx from C
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
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
  // Initialise multi-threading if asked
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
// Initialize a ZSTD_Cctx pointer from R
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP init_cctx_(SEXP level_, SEXP num_threads_, SEXP dict_) {

  ZSTD_CCtx* cctx = init_cctx(asInteger(level_), asInteger(num_threads_));

  if (!isNull(dict_)) {
    size_t status;
    if (TYPEOF(dict_) == RAWSXP) {
      status = ZSTD_CCtx_loadDictionary(cctx, RAW(dict_), length(dict_));
    } else if (TYPEOF(dict_) == STRSXP) {
      const char *filename = CHAR(STRING_ELT(dict_, 0));
      FILE *fp = fopen(filename, "rb");
      if (fp == NULL) error("Couldn't open file '%s'", filename);
      fseek(fp, 0, SEEK_END);
      long fsize = ftell(fp);
      fseek(fp, 0, SEEK_SET);  /* same as rewind(f); */
      
      unsigned char *dict = malloc(fsize);
      if (dict == NULL) error("Couldn't allocate for reading dict from file");
      fread(dict, fsize, 1, fp);
      fclose(fp);
      
      status = ZSTD_CCtx_loadDictionary(cctx, dict, fsize);
      
      free(dict);
    } else {
      error("init_cctx(): 'dict' must be a raw vector or a filename");
    }
    if (ZSTD_isError(status)) {
      error("init_cctx(): Error initialising dict");
    }
  }
  
  
  SEXP cctx_ = PROTECT(R_MakeExternalPtr(cctx, R_NilValue, R_NilValue));
  R_RegisterCFinalizer(cctx_, zstd_cctx_finalizer);
  Rf_setAttrib(cctx_, R_ClassSymbol, Rf_mkString("ZSTD_CCtx"));
  UNPROTECT(1);
  
  return cctx_;
}

