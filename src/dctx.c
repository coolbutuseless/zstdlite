



#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "zstd.h"
#include "dctx.h"


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



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Initialize a ZSTD_Cctx pointer from R
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP init_dctx_(SEXP dict_) {

  ZSTD_DCtx *dctx = ZSTD_createDCtx();
  if (dctx == NULL) {
    error("init_dctx(): Couldn't initialse memory for 'dctx'");
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Handle dict
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (!isNull(dict_)) {
    size_t status;
    if (TYPEOF(dict_) == RAWSXP) {
      status = ZSTD_DCtx_loadDictionary(dctx, RAW(dict_), length(dict_));
    } else if (TYPEOF(dict_) == STRSXP) {
      const char *filename = CHAR(STRING_ELT(dict_, 0));
      FILE *fp = fopen(filename, "rb");
      if (fp == NULL) error("Couldn't open file '%s'", filename);
      fseek(fp, 0, SEEK_END);
      long fsize = ftell(fp);
      fseek(fp, 0, SEEK_SET);  /* same as rewind(f); */
      
      unsigned char *dict = malloc(fsize);
      if (dict == NULL) error("Couldn't allocate for reading dict from file");
      unsigned long n = fread(dict, fsize, 1, fp);
      fclose(fp);
      
      if (n != fsize) {
        error("init_cctx(): fread() could not read entire dict from file");
      }
      
      status = ZSTD_DCtx_loadDictionary(dctx, dict, fsize);
      
      free(dict);
    } else {
      error("init_dctx(): 'dict' must be a raw vector or a filename");
    }
    if (ZSTD_isError(status)) {
      error("init_dctx(): Error initialising dict");
    }
  }
  

  SEXP dctx_ = PROTECT(R_MakeExternalPtr(dctx, R_NilValue, R_NilValue));
  R_RegisterCFinalizer(dctx_, zstd_dctx_finalizer);
  Rf_setAttrib(dctx_, R_ClassSymbol, Rf_mkString("ZSTD_DCtx"));
  UNPROTECT(1);
  
  return dctx_;
}

