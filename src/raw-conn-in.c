



#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>
#include <R_ext/Connections.h>

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
SEXP zstd_decompress_conn_(SEXP conn_, SEXP type_, SEXP dctx_, SEXP opts_) {
  
  
  Rconnection rconn = R_GetConnection(conn_);
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
    dctx = init_dctx_with_opts(opts_, 0, 0); // Streaming does NOT have stable buffers
  }
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Allocate the full length buffer to decompress to
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP dst_;
  unsigned char *dst;
  ZSTD_outBuffer output;
  
  size_t bytes_read;
  int first = 1;
  while ( (bytes_read = R_ReadConnection(rconn, file_buf, INSIZE)) ) {
    
    // If this is the first read, then 
    if (first) {
      if (bytes_read < 18) {
        error("zstd_decompress_conn_(): Could not determine uncompressed size");
      }
      size_t uncompressed_size = ZSTD_getFrameContentSize(file_buf, 18);
      
      // Setup R destination raw vector or string
      if (return_raw) {
        dst_ = PROTECT(allocVector(RAWSXP, (R_xlen_t)uncompressed_size));
        dst = (void *)RAW(dst_);
      } else {
        dst_ = PROTECT(allocVector(STRSXP, 1));
        dst = (unsigned char *)malloc(uncompressed_size + 1);
        dst[uncompressed_size] = 0; // Add "\0" terminator to string
      }  
      
      // setup ZSTD output buffer
      output.dst  = dst;
      output.size = uncompressed_size;
      output.pos  = 0;
      
      first = 0;
    }
    
    
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
  if (isNull(dctx_)) ZSTD_freeDCtx(dctx);
  UNPROTECT(1);
  return dst_;
}










































