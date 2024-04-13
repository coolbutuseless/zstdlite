



#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "zstd.h"
#include "calc-size-robust.h"
#include "cctx.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// A data buffer of constant size
// Needs total length and pos to keep track of how much data it currently contains
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#define INSIZE 131702  // Calculated via ZSTD_CStream_InSize()

typedef struct {
  ZSTD_CCtx *cctx;
  ZSTD_outBuffer zstd_buffer;
  unsigned char uncompressed_data[INSIZE]; 
  size_t uncompressed_pos;
  size_t uncompressed_size;
} serialize_stream_buffer_t;



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Write a byte into the buffer at the current location.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void write_byte_to_stream(R_outpstream_t stream, int c) {
  error("write_byte_to_stream(): Not implemented");
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Write multiple bytes to file
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void write_bytes_to_stream(R_outpstream_t stream, void *src, int length) {
  serialize_stream_buffer_t *buf = (serialize_stream_buffer_t *)stream->data;
  
  
  if (buf->uncompressed_pos + (size_t)length >= buf->uncompressed_size) {
    // There's not enough space to add these new bytes to the
    // uncompressed_data buffer.  So first let's flush 
    // the uncompressed data buffer by compressing and writing to file
    
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // COmperssion input = uncompressed data buffer
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    ZSTD_inBuffer input = { 
      .src  = buf->uncompressed_data, 
      .size = buf->uncompressed_pos, 
      .pos  = 0
    };
    
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Keep compressing until entire input has been consumed
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    do {
      size_t rem = ZSTD_compressStream2(buf->cctx, &(buf->zstd_buffer), &input, ZSTD_e_continue);
      if (ZSTD_isError(rem)) {
        Rprintf("write_bytes_to_stream() [A]: error %s\n", ZSTD_getErrorName(rem));
      }
    } while (input.pos != input.size);
    
    
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Reset the uncompressed data buffer
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    buf->uncompressed_pos = 0;
    
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Check length of new seriazlization data. If larger than the buffer
    // we allocated for uncompressed data, then compress data directly and return
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    if (length >= buf->uncompressed_size) {
      ZSTD_inBuffer input = {
        .src  = src, 
        .size = (size_t)length, 
        .pos  = 0
      };
      
      do {
        size_t rem = ZSTD_compressStream2(buf->cctx, &(buf->zstd_buffer), &input, ZSTD_e_continue);
        if (ZSTD_isError(rem)) {
          Rprintf("write_bytes_to_stream() [A]: error %s\n", ZSTD_getErrorName(rem));
        }
      } while (input.pos != input.size);
      
      return;
    }
  }
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Copy the supplied bytes to the uncompressed_data buffer
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  memcpy(buf->uncompressed_data + buf->uncompressed_pos, src, (size_t)length);
  buf->uncompressed_pos += (size_t)length;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Serialize an R object to a buffer of fixed size and then compress
// the buffer using zstd
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP zstd_serialize_stream_(SEXP robj, SEXP cctx_, SEXP opts_) {
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Create the buffer for the serialized representation
  // Calculate the exact size of the serialized object in bytes
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  size_t num_serialized_bytes = (size_t)calc_serialized_size(robj);
  serialize_stream_buffer_t buf = {
    .uncompressed_pos  = 0,
    .uncompressed_size = INSIZE
  };
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Initialize the ZSTD context
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (isNull(cctx_)) {
    buf.cctx = init_cctx_with_opts(opts_, 0);  // streaming does NOT have stable buffers.
  } else {
    buf.cctx = external_ptr_to_zstd_cctx(cctx_);
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // For streaming, need to explicitly set the number of serialized bytes
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  size_t res = ZSTD_CCtx_setPledgedSrcSize(buf.cctx, num_serialized_bytes);
  if (ZSTD_isError(res)) {
    error("zstd_serialize_stream(): Error on pledge size\n");
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // calculate maximum possible size of compressed buffer in the worst case
  // Allocate compression buffer
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  size_t max_compressed_bytes  = ZSTD_compressBound(num_serialized_bytes);
  SEXP dst_ = PROTECT(allocVector(RAWSXP, (R_xlen_t)max_compressed_bytes));
  char *dst = (char *)RAW(dst_);

  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Setup destination for zstd compressed data
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  buf.zstd_buffer.dst  = dst;
  buf.zstd_buffer.size = max_compressed_bytes;
  buf.zstd_buffer.pos  = 0;
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Setup the R serialization struct
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  struct R_outpstream_st output_stream;
  R_InitOutPStream(
    &output_stream,          // The stream object which wraps everything
    (R_pstream_data_t) &buf, // The actual serialized data. R_pstream_data_t = void *
    R_pstream_binary_format, // Store as binary
    3,                       // Version = 3 for R >3.5.0 See `?base::serialize`
    write_byte_to_stream,    // Function to write single byte to buffer
    write_bytes_to_stream,   // Function for writing multiple bytes to buffer
    NULL,                    // Func for special handling of reference data.
    R_NilValue               // Data related to reference data handling
  );
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Serialize the object into the output_stream
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  R_Serialize(robj, &output_stream);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Compress the remaining serialized data
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  ZSTD_inBuffer input = { 
    .src  = buf.uncompressed_data, 
    .size = buf.uncompressed_pos, 
    .pos  = 0 
  };
  
  size_t remaining_bytes;
  do {
    remaining_bytes = ZSTD_compressStream2(buf.cctx, &(buf.zstd_buffer), &input, ZSTD_e_end);
    if (ZSTD_isError(remaining_bytes)) {
      error("zstd_compress() [end]: Compression error. %s", ZSTD_getErrorName(remaining_bytes));
    }
  } while (remaining_bytes > 0);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // How many compressed bytes did we generate?
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  size_t num_compressed_bytes = buf.zstd_buffer.pos;

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Truncate the user-viewable size of the RAW vector
  // Requires: R_VERSION >= R_Version(3, 4, 0)
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (num_compressed_bytes < max_compressed_bytes) {
    SETLENGTH(dst_, (R_xlen_t)num_compressed_bytes);
    SET_TRUELENGTH(dst_, (R_xlen_t)max_compressed_bytes);
    SET_GROWABLE_BIT(dst_);
  }
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Tidy and return
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (isNull(cctx_)) ZSTD_freeCCtx(buf.cctx);
  UNPROTECT(1);
  return dst_;
}













































