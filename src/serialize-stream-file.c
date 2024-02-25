



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

#define INSIZE  131702  // Calculated via ZSTD_CStream_InSize()
#define OUTSIZE 131591  

typedef struct {
  ZSTD_CCtx *cctx;
  R_xlen_t pos;
  unsigned char data[INSIZE]; 
  FILE **fp;
} stream_file_buffer_t;


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Write a byte into the buffer at the current location.
// The actual buffer is encapsulated as part of the stream structure, so you
// have to extract it first
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void write_byte_to_stream_file(R_outpstream_t stream, int c) {
  stream_file_buffer_t *buf = (stream_file_buffer_t *)stream->data;
  if (buf->pos == INSIZE) {
    error("write_byte_to_stream(): Buffer exceeded");
  }
  buf->data[buf->pos++] = (unsigned char)c;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Write multiple bytes into the buffer at the current location.
// The actual buffer is encapsulated as part of the stream structure, so you
// have to extract it first
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void write_bytes_to_stream_file(R_outpstream_t stream, void *src, int length) {
  stream_file_buffer_t *buf = (stream_file_buffer_t *)stream->data;
  
  static unsigned char zstd_raw[OUTSIZE];
  FILE *fp = *(buf->fp);
  
  if (buf->pos + length >= INSIZE) {
    
    // Compress current serialize buffer 
    ZSTD_inBuffer input = {buf->data, buf->pos, 0};
    
    do {
      ZSTD_outBuffer output = { zstd_raw, OUTSIZE, 0 };
      size_t rem = ZSTD_compressStream2(buf->cctx, &output, &input, ZSTD_e_continue);
      if (ZSTD_isError(rem)) {
        Rprintf("write_bytes_to_stream() [A]: error %s\n", ZSTD_getErrorName(rem));
      }
      fwrite(output.dst, 1, output.pos, fp);
    } while (input.pos != input.size);
    
    // Reset the serialization buffer
    buf->pos = 0;
    
    // Check length of new seriazlization data. If larger than INSIZE, 
    // then compress directly and return
    if (length >= INSIZE) {
      ZSTD_inBuffer input = {src, length, 0};
      
      do {
        ZSTD_outBuffer output = { zstd_raw, OUTSIZE, 0 };
        size_t rem = ZSTD_compressStream2(buf->cctx, &output, &input, ZSTD_e_continue);
        if (ZSTD_isError(rem)) {
          Rprintf("write_bytes_to_stream() [A]: error %s\n", ZSTD_getErrorName(rem));
        }
        fwrite(output.dst, 1, output.pos, fp);
      } while (input.pos != input.size);
      
      return;
    }
  }
  
  memcpy(buf->data + buf->pos, src, length);
  buf->pos += length;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Serialize an R object to a buffer of fixed size and then compress
// the buffer using zstd
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP zstd_serialize_stream_file_(SEXP robj, SEXP file_, SEXP level_, SEXP num_threads_, SEXP cctx_) {
  
  static unsigned char zstd_raw[OUTSIZE];
  
  const char *filename = CHAR(STRING_ELT(file_, 0));
  
  
  FILE *fp = fopen(filename, "wb");
  if (fp == NULL) {
    error("zstd_serialize_stream_file_(): Couldn't open output file '%s'", filename);
  }
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Create the buffer for the serialized representation
  // Calculate the exact size of the serialized object in bytes
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int num_serialized_bytes = calc_size_robust(robj);
  stream_file_buffer_t buf = {.pos = 0, .fp = &fp};
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Initialize the ZSTD context
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (isNull(cctx_)) {
    buf.cctx = init_cctx(asInteger(level_), asInteger(num_threads_));
  } else {
    buf.cctx = external_ptr_to_zstd_cctx(cctx_);
    ZSTD_CCtx_reset(buf.cctx, ZSTD_reset_session_only);
  }
  
  
  int res = ZSTD_CCtx_setPledgedSrcSize(buf.cctx, num_serialized_bytes);
  if (ZSTD_isError(res)) {
    error("zstd_serialize_stream_file(): Error on pledge size\n");
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
    write_byte_to_stream_file,  // Function to write single byte to buffer
    write_bytes_to_stream_file, // Function for writing multiple bytes to buffer
    NULL,                       // Func for special handling of reference data.
    R_NilValue                  // Data related to reference data handling
  );
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Serialize the object into the output_stream
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  R_Serialize(robj, &output_stream);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Compress the remaining serialized data
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  ZSTD_inBuffer input = { buf.data, buf.pos, 0 };

  size_t remaining_bytes;
  do {
    ZSTD_outBuffer output = { zstd_raw, OUTSIZE, 0 };
    remaining_bytes = ZSTD_compressStream2(buf.cctx, &output, &input, ZSTD_e_end);
    fwrite(output.dst, 1, output.pos, fp);
  } while (remaining_bytes > 0);

  if (isNull(cctx_)) {
    ZSTD_freeCCtx(buf.cctx);
  }


  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Tidy and return
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  fclose(fp);
  return R_NilValue;
}
