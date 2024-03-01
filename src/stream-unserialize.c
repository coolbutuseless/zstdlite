



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
  
  unsigned char *compressed_data;
  size_t compressed_size;
  size_t compressed_pos;
} unserialize_stream_buffer_t;



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Write a byte into the buffer at the current location.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int read_byte_from_stream(R_inpstream_t stream) {
  error("read_byte_from_stream(): Reading single byte is unsupported\n");
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Write multiple bytes into the buffer at the current location.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void read_bytes_from_stream(R_inpstream_t stream, void *dst, int length) {
  unserialize_stream_buffer_t *buf = (unserialize_stream_buffer_t *)stream->data;
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Unpack bytes into the 'dst' buffer
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  ZSTD_inBuffer input   = { 
    .src  = buf->compressed_data, 
    .size = buf->compressed_size, 
    .pos  = 0 
  };
  ZSTD_outBuffer output = {
    .dst  = dst, 
    .size = (size_t)length, 
    .pos  = 0 
  };
  
  
  while (output.pos < length) {
    size_t const ret = ZSTD_decompressStream(buf->dctx, &output , &input);
    if (ZSTD_isError(ret)) {
      error("read_bytes_from_stream() error: %s", ZSTD_getErrorName(ret));
    }
  }
  
  buf->compressed_data += input.pos;
  buf->compressed_size -= input.pos;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Unpack a raw vector to an R object
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP zstd_unserialize_stream_(SEXP src_, SEXP dctx_, SEXP opts_) {
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Setup Decompression Context
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  ZSTD_DCtx *dctx;
  if (isNull(dctx_)) {
    dctx = init_dctx_with_opts(opts_, 0); // Streaming does NOT have stable buffers
  } else {
    dctx = external_ptr_to_zstd_dctx(dctx_);
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Setup user data for serialization
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  unserialize_stream_buffer_t user_data = {
    .dctx             = dctx,
    .compressed_pos   = 0
  };

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // This vanilla version of streaming unserialization only handles 
  // the src being a raw vector
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (TYPEOF(src_) == RAWSXP) {
    user_data.compressed_data = (unsigned char *)RAW(src_);
    user_data.compressed_size = (size_t)length(src_);
  } else {
    error("zstd_unserialize_stream_(): source must be a raw vector");
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Setup the R serialization struct
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  struct R_inpstream_st input_stream;
  R_InitInPStream(
    &input_stream,           // Stream object wrapping data buffer
    (R_pstream_data_t) &user_data,  // Actual data buffer
    R_pstream_any_format,    // Unpack all serialized types
    read_byte_from_stream,   // Function to read single byte from buffer
    read_bytes_from_stream,  // Function for reading multiple bytes from buffer
    NULL,                    // Func for special handling of reference data.
    NULL                     // Data related to reference data handling
  );
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Unserialize the input_stream into an R object
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP res_  = PROTECT(R_Unserialize(&input_stream));
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Tidy and return
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  ZSTD_freeDCtx(dctx);
  UNPROTECT(1);
  return res_;
}










































