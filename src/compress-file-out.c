



#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "zstd/zstd.h"
#include "calc-size-robust.h"
#include "cctx.h"
#include "compress-file.h"


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Magic values calculated with: 
// Calculated via ZSTD_CStream_InSize()
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define INSIZE  131702  
#define OUTSIZE 131591  


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Serialize an R object to a buffer of fixed size and then compress
// the buffer using zstd
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP zstd_compress_stream_file_(SEXP vec_, SEXP file_, SEXP cctx_, SEXP opts_) {
  
  static unsigned char zstd_raw[OUTSIZE];
  ZSTD_CCtx *cctx;
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Unpack 'raw' or 'string' arguments
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  unsigned char *src;
  size_t src_size;
  
  if (TYPEOF(vec_) == RAWSXP) {
    src = RAW(vec_);
    src_size = (size_t)length(vec_);
  } else if (TYPEOF(vec_) == STRSXP) {
    src = (unsigned char *)CHAR(STRING_ELT(vec_, 0));
    src_size = (size_t)strlen(CHAR(STRING_ELT(vec_, 0)));
  } else {
    error("zstd_compress() only accepts raw vectors or strings");
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Initialize the ZSTD context
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (isNull(cctx_)) {
    cctx = init_cctx_with_opts(opts_, 0);
  } else {
    cctx = external_ptr_to_zstd_cctx(cctx_);
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Open file for output
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  const char *filename = CHAR(STRING_ELT(file_, 0));
  FILE *fp = fopen(filename, "wb");
  if (fp == NULL) {
    error("zstd_compress_stream_file_(): Couldn't open output file '%s'", filename);
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // For streaming compression, need to manually set the 
  // number of uncompressed bytes
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  size_t res = ZSTD_CCtx_setPledgedSrcSize(cctx, (unsigned long long)src_size);
  if (ZSTD_isError(res)) {
    error("zstd_compress_stream_file_(): Error on pledge size\n");
  }
  

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Need to flush out and compress any remaining bytes
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  ZSTD_inBuffer input = {
    input.src  = src,
    input.size = src_size,
    input.pos  = 0
  };

  do {
    ZSTD_outBuffer output = { 
      .dst  = zstd_raw, 
      .size = OUTSIZE, 
      .pos  = 0 
    };
    size_t remaining_bytes = ZSTD_compressStream2(cctx, &output, &input, ZSTD_e_continue);
    if (ZSTD_isError(remaining_bytes)) {
      error("zstd_compress_stream_file_() [mid]: error %s\n", ZSTD_getErrorName(remaining_bytes));
    }
    fwrite(output.dst, 1, output.pos, fp);
  } while(input.pos != input.size);
    

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Flush remaining
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  size_t remaining_bytes;
  do {
    ZSTD_outBuffer output = { 
      .dst  = zstd_raw, 
      .size = OUTSIZE, 
      .pos  = 0 
    };
    remaining_bytes = ZSTD_compressStream2(cctx, &output, &input, ZSTD_e_end);
    if (ZSTD_isError(remaining_bytes)) {
      error("zstd_compress_stream_file_() [end]: error %s\n", ZSTD_getErrorName(remaining_bytes));
    }
    fwrite(output.dst, 1, output.pos, fp);
  } while (remaining_bytes != 0);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Tidy and return
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (isNull(cctx_)) ZSTD_freeCCtx(cctx);
  fclose(fp);
  return R_NilValue;
}
