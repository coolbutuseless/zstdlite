



#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "zstd.h"
#include "calc-size-robust.h"
#include "dctx.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// A data buffer of constant size
// Needs total length and pos to keep track of how much data it currently contains
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#define INSIZE 131702  // Calculated via ZSTD_CStream_InSize()
#define STARTSIZE 131702

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
// Write multiple bytes into the buffer at the current location.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void read_bytes_from_stream_file(R_inpstream_t stream, void *dst, int length) {
  unserialize_stream_file_buffer_t *buf = (unserialize_stream_file_buffer_t *)stream->data;
  
  
  // Fill the buffer
  if (buf->compressed_len == 0) {
    buf->compressed_len = fread(buf->compressed_data, 1, buf->compressed_size, *(buf->fp));
    buf->compressed_pos = 0;
  }
  
  ZSTD_inBuffer input   = { 
    .src  = buf->compressed_data + buf->compressed_pos, 
    .size = buf->compressed_len  - buf->compressed_pos, 
    .pos  = 0
  };
  
  ZSTD_outBuffer output = {
    .dst  = dst, 
    .size = length, 
    .pos  = 0 
  };
  
  
  while (output.pos < length) {
    size_t const ret = ZSTD_decompressStream(buf->dctx, &output , &input);
    if (ZSTD_isError(ret)) {
      error("read_bytes_from_stream_file() error: %s", ZSTD_getErrorName(ret));
    }
    buf->compressed_pos += input.pos;
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


// bb <- readBin("working/out.rds.zst", raw(), n = file.size("working/out.rds.zst"))
// zstd_unserialize_stream_file("working/out.rds.zst")
// bench::mark(zstd_unserialize_stream(bb))

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Unpack a raw vector to an R object
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP zstd_unserialize_stream_file_(SEXP src_, SEXP dctx_) {
  
  ZSTD_DCtx *dctx = init_dctx();
  
  const char *filename = CHAR(STRING_ELT(src_, 0));
  FILE *fp = fopen(filename, "rb");
  if (fp == NULL) {
    error("zstd_unserialize_stream_file(): Couldn't open input file '%s%'", filename);
  }
  
  
  unserialize_stream_file_buffer_t user_data = {
    .fp               = &fp,
    .dctx             = dctx,
    .compressed_pos   = 0,
    .compressed_len   = 0,
    .compressed_size  = INSIZE
  };

  
  // Treat the data buffer as an input stream
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
  
  // Unserialize the input_stream into an R object
  SEXP res_  = PROTECT(R_Unserialize(&input_stream));
  
  fclose(fp);
  ZSTD_freeDCtx(dctx);
  UNPROTECT(1);
  return res_;
}










































