



#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "zstd.h"
#include "calc-size-robust.h"
#include "dctx.h"
#include "serialize-file.h"


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// A magic size calculated via ZSTD_CStream_InSize()
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define INSIZE 131702  


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// User Struct for Serialization
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef struct {
  ZSTD_DCtx *dctx;
  
  FILE **fp;
  
  unsigned char compressed_data[INSIZE];
  size_t compressed_size;
  size_t compressed_pos;
  size_t compressed_len;
} unserialize_stream_file_buffer_t;


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Read a single byte
// this only seems to be used for ASCII mode?
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int read_byte_from_stream_file(R_inpstream_t stream) {
  error("read_byte_from_stream_file(): Reading single byte is unsupported\n");
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Read multiple bytes from compressed file
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void read_bytes_from_stream_file(R_inpstream_t stream, void *dst, int length) {
  unserialize_stream_file_buffer_t *buf = (unserialize_stream_file_buffer_t *)stream->data;
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // If file read buffer  is empty, then fill it
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (buf->compressed_len == 0) {
    buf->compressed_len = fread(buf->compressed_data, 1, buf->compressed_size, *(buf->fp));
    buf->compressed_pos = 0;
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // ZSTD input struct.
  // Note: There may be multiple calls to 'read_bytes_from_stream_file()'
  //       which are all consuming data from the same buffered read from file
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  ZSTD_inBuffer input   = { 
    .src  = buf->compressed_data + buf->compressed_pos, 
    .size = buf->compressed_len  - buf->compressed_pos, 
    .pos  = 0
  };
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // ZSTD output struct
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  ZSTD_outBuffer output = {
    .dst  = dst, 
    .size = (size_t)length, 
    .pos  = 0 
  };
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Decompress bytes in the 'compressed_data' buffer until we have the 
  // number of bytes requested (i.e. 'length')
  // If the 'compressed_data' buffer runs out of data, read in some more!
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  while (output.pos < length) {
    size_t const status = ZSTD_decompressStream(buf->dctx, &output , &input);
    if (ZSTD_isError(status)) {
      error("read_bytes_from_stream_file() error: %s", ZSTD_getErrorName(status));
    }
    
    // Update the compressed data pointer to where we have decompressed up to
    buf->compressed_pos += input.pos;
    
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // If the read file buffer (of compressed data) is exhausted: read more!
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    if (buf->compressed_pos == buf->compressed_len) {
      // file read buffer is exhausted. read more bytes
      buf->compressed_len = fread(buf->compressed_data, 1, buf->compressed_size, *(buf->fp));
      buf->compressed_pos = 0;
      
      input.src  = buf->compressed_data;
      input.size = buf->compressed_len;
      input.pos  = 0;
    }
  }
  
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Unserialize an object by streaming compressed data from a file
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP zstd_unserialize_stream_file_(SEXP src_, SEXP dctx_, SEXP opts_) {
  
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
  // Setup the user data structure to keep track of decompression
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  unserialize_stream_file_buffer_t user_data = {
    .fp               = &fp,
    .dctx             = dctx,
    .compressed_pos   = 0,
    .compressed_len   = 0,
    .compressed_size  = INSIZE
  };

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Setup the R serialization struct
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  struct R_inpstream_st input_stream;
  R_InitInPStream(
    &input_stream,                  // Stream object wrapping data buffer
    (R_pstream_data_t) &user_data,  // Actual data buffer
    R_pstream_any_format,           // Unpack all serialized types
    read_byte_from_stream_file,     // Function to read single byte from buffer
    read_bytes_from_stream_file,    // Function for reading multiple bytes from buffer
    NULL,                           // Func for special handling of reference data.
    NULL                            // Data related to reference data handling
  );
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Unserialize the input_stream into an R object
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP res_  = PROTECT(R_Unserialize(&input_stream));
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Tidy and return
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  fclose(fp);
  if (isNull(dctx_)) ZSTD_freeDCtx(dctx);
  UNPROTECT(1);
  return res_;
}










































