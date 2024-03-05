



#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "zstd/zstd.h"
#include "calc-size-robust.h"
#include "dctx.h"
#include "serialize-file.h"


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// A magic size calculated via ZSTD_CStream_InSize()
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define INSIZE 131702  


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Unserialize an object by streaming compressed data from a file
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP zstd_decompress_stream_file_(SEXP src_, SEXP type_, SEXP dctx_, SEXP opts_) {
  
  static unsigned char file_buf[INSIZE];
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Create a decompression buffer of the exact required size
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int return_raw = strcmp(CHAR(STRING_ELT(type_, 0)), "raw") == 0;
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Setup the Decompression Context
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  ZSTD_DCtx *dctx;
  if (!isNull(dctx_)) {
    dctx = external_ptr_to_zstd_dctx(dctx_);
  } else {
    dctx = init_dctx_with_opts(opts_, 0); // Streaming does NOT have stable buffers
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Setup file pointer
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  const char *filename = CHAR(STRING_ELT(src_, 0));
  FILE *fp = fopen(filename, "rb");
  if (fp == NULL) {
    error("zstd_unserialize_stream_file(): Couldn't open input file '%s'", filename);
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Determine uncompressed file size
  // ZSTD_frameHeaderSize_max = 18
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  size_t bytes_read = fread(file_buf, 1, 18, fp);
  fseek(fp, 0, SEEK_SET);  // same as rewind(f); 
  if (bytes_read != 18) {
    fclose(fp);
    error("zstd_decompress_stream_file_(): Couldn't read file '%s' to determine uncompressed size", filename);
  }
  size_t uncompressed_size = ZSTD_getFrameContentSize(file_buf, 18);
  if (ZSTD_isError(uncompressed_size)) {
    error("zstd_decompress_stream_file_(): Could not determine uncompressed size");
  }
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Allocate the full length buffer to decompress to
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP dst_;
  unsigned char *dst;
  
  if (return_raw) {
    dst_ = PROTECT(allocVector(RAWSXP, (R_xlen_t)uncompressed_size));
    dst = (void *)RAW(dst_);
  } else {
    dst_ = PROTECT(allocVector(STRSXP, 1));
    dst = (unsigned char *)malloc(uncompressed_size + 1);
    dst[uncompressed_size] = 0; // Add "\0" terminator to string
  }  
  
  ZSTD_outBuffer output = {
    .dst  = dst,
    .size = uncompressed_size,
    .pos  = 0
  };
  
  
  while ( (bytes_read = fread(file_buf, 1, INSIZE, fp)) ) {
    ZSTD_inBuffer input = {
      .src  = file_buf,
      .size = bytes_read,
      .pos  = 0
    };
    
    while (input.pos < input.size) {
      ZSTD_decompressStream(dctx, &output, &input);
    }
  };
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Tidy and return
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  fclose(fp);
  if (isNull(dctx_)) ZSTD_freeDCtx(dctx);
  UNPROTECT(1);
  return dst_;
}










































