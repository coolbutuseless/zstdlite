



#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>
#include <R_ext/Connections.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "zstd.h"
#include "calc-size-robust.h"
#include "cctx.h"


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Magic values calculated with: 
// Calculated via ZSTD_CStream_InSize()
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define INSIZE  131702  
#define OUTSIZE 131591  

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// User Struct for Compression
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef struct {
  ZSTD_CCtx *cctx;
  Rconnection rconn;
  
  unsigned char uncompressed_data[INSIZE]; 
  size_t uncompressed_pos;
  size_t uncompressed_size;
  
} stream_conn_buffer_t;


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Write a byte into the buffer at the current location.
// The actual buffer is encapsulated as part of the stream structure, so you
// have to extract it first
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void write_byte_to_conn(R_outpstream_t stream, int c) {
  error("write_byte_to_conn(): Writing single byte is unsupported\n");
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Write multiple serialized bytes to file
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void write_bytes_to_conn(R_outpstream_t stream, void *src, int length) {
  stream_conn_buffer_t *buf = (stream_conn_buffer_t *)stream->data;
  
  static unsigned char zstd_raw[OUTSIZE];
  
  if (buf->uncompressed_pos + (size_t)length >= buf->uncompressed_size) {
    
    // Compress current serialize buffer 
    ZSTD_inBuffer input = {
      .src  = buf->uncompressed_data, 
      .size = buf->uncompressed_pos, 
      .pos  = 0
    };
    
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Compress the input data
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    do {
      ZSTD_outBuffer output = { 
        .dst  = zstd_raw, 
        .size = OUTSIZE, 
        .pos  = 0 
      };
      
      size_t rem = ZSTD_compressStream2(buf->cctx, &output, &input, ZSTD_e_continue);
      if (ZSTD_isError(rem)) {
        Rprintf("write_bytes_to_stream_file(): error %s\n", ZSTD_getErrorName(rem));
      }
      
      R_WriteConnection(buf->rconn, output.dst, output.pos);
    } while (input.pos != input.size);
    
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Reset the serialization buffer
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    buf->uncompressed_pos = 0;
    
    
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Check length of new seriazlization data. If larger than buf->uncompssed_size, 
    // then compress directly and return
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    if (length >= buf->uncompressed_size) {
      ZSTD_inBuffer input = {
        .src  = src, 
        .size = (size_t)length, 
        .pos  = 0
      };
      
      do {
        ZSTD_outBuffer output = { 
          .dst  = zstd_raw, 
          .size = OUTSIZE, 
          .pos  = 0 
        };
        
        size_t rem = ZSTD_compressStream2(buf->cctx, &output, &input, ZSTD_e_continue);
        if (ZSTD_isError(rem)) {
          Rprintf("write_bytes_to_stream_file(): error %s\n", ZSTD_getErrorName(rem));
        }
        
        R_WriteConnection(buf->rconn, output.dst, output.pos);
      } while (input.pos != input.size);
      
      return;
    }
  }
  
  memcpy(buf->uncompressed_data + buf->uncompressed_pos, src, (size_t)length);
  buf->uncompressed_pos += (size_t)length;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Serialize an R object to a buffer of fixed size and then compress
// the buffer using zstd
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP zstd_serialize_conn_(SEXP robj, SEXP conn_, SEXP cctx_, SEXP opts_) {
  
  static unsigned char zstd_raw[OUTSIZE];
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Create the buffer for the serialized representation
  // Calculate the exact size of the serialized object in bytes
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int num_serialized_bytes = calc_serialized_size(robj);
  stream_conn_buffer_t buf = {
    .uncompressed_pos  = 0, 
    .uncompressed_size = INSIZE,
    .rconn             = R_GetConnection(conn_)
  };
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Initialize the ZSTD context
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (isNull(cctx_)) {
    buf.cctx = init_cctx_with_opts(opts_, 0);
  } else {
    buf.cctx = external_ptr_to_zstd_cctx(cctx_);
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // For streaming compression, need to manually set the 
  // number of uncompressed bytes
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  size_t res = ZSTD_CCtx_setPledgedSrcSize(buf.cctx, (unsigned long long)num_serialized_bytes);
  if (ZSTD_isError(res)) {
    error("zstd_serialize_conn_(): Error on pledge size\n");
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Create the output stream structure for R serialization
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  struct R_outpstream_st output_stream;

  R_InitOutPStream(
    &output_stream,             // The stream object which wraps everything
    (R_pstream_data_t) &buf,    // The actual serialized data. R_pstream_data_t = void *
    R_pstream_binary_format,    // Store as binary
    3,                          // Version = 3 for R >3.5.0 See `?base::serialize`
    write_byte_to_conn,         // Function to write single byte to buffer
    write_bytes_to_conn,        // Function for writing multiple bytes to buffer
    NULL,                       // Func for special handling of reference data.
    R_NilValue                  // Data related to reference data handling
  );
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Serialize the object into the output_stream
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  R_Serialize(robj, &output_stream);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Need to flush out and compress any remaining bytes
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  ZSTD_inBuffer input = { 
    .src  = buf.uncompressed_data, 
    .size = buf.uncompressed_pos, 
    .pos  = 0 
  };

  size_t remaining_bytes;
  do {
    ZSTD_outBuffer output = { 
      .dst  = zstd_raw, 
      .size = OUTSIZE, 
      .pos  = 0 
    };
    remaining_bytes = ZSTD_compressStream2(buf.cctx, &output, &input, ZSTD_e_end);
    if (ZSTD_isError(remaining_bytes)) {
      Rprintf("zstd_serialize_conn_() [end]: error %s\n", ZSTD_getErrorName(remaining_bytes));
    }
    R_WriteConnection(R_GetConnection(conn_), output.dst, output.pos);
  } while (remaining_bytes > 0);

  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Tidy and return
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (isNull(cctx_)) ZSTD_freeCCtx(buf.cctx);
  return R_NilValue;
}
