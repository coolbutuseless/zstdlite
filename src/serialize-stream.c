



#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "zstd.h"
#include "calc-size-robust.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// A data buffer of constant size
// Needs total length and pos to keep track of how much data it currently contains
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#define INSIZE 131702  // Calculated via ZSTD_CStream_InSize()

typedef struct {
  ZSTD_CCtx *cctx;
  R_xlen_t pos;
  unsigned char data[INSIZE]; 
  ZSTD_outBuffer zstd_buffer;
} stream_buffer_t;


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Write a byte into the buffer at the current location.
// The actual buffer is encapsulated as part of the stream structure, so you
// have to extract it first
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void write_byte_to_stream(R_outpstream_t stream, int c) {
  stream_buffer_t *buf = (stream_buffer_t *)stream->data;
  if (buf->pos == INSIZE) {
    error("write_byte_to_stream(): Buffer exceeded\n");
  }
  buf->data[buf->pos++] = (unsigned char)c;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Write multiple bytes into the buffer at the current location.
// The actual buffer is encapsulated as part of the stream structure, so you
// have to extract it first
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void write_bytes_to_stream(R_outpstream_t stream, void *src, int length) {
  stream_buffer_t *buf = (stream_buffer_t *)stream->data;
  
  if (buf->pos + length >= INSIZE) {
    
    // Compress current serialize buffer 
    ZSTD_inBuffer input = {buf->data, buf->pos, 0};
    
    do {
      size_t rem = ZSTD_compressStream2(buf->cctx, &(buf->zstd_buffer), &input, ZSTD_e_continue);
      if (ZSTD_isError(rem)) {
        Rprintf("write_bytes_to_stream() [A]: error %s\n", ZSTD_getErrorName(rem));
      }
    } while (input.pos != input.size);
    
    // Reset the serialization buffer
    buf->pos = 0;
    
    // Check length of new seriazlization data. If larger than INSIZE, 
    // then compress directly and return
    if (length >= INSIZE) {
      ZSTD_inBuffer input = {src, length, 0};
      
      do {
        size_t rem = ZSTD_compressStream2(buf->cctx, &(buf->zstd_buffer), &input, ZSTD_e_continue);
        if (ZSTD_isError(rem)) {
          Rprintf("write_bytes_to_stream() [A]: error %s\n", ZSTD_getErrorName(rem));
        }
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
SEXP zstd_serialize_stream_(SEXP robj, SEXP compressionLevel_, SEXP num_threads_) {
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Sanitize the given compression level for zstandard
  // Note: min level can be -(2^17), but everyone uses -5 as the minimum
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int compressionLevel = asInteger(compressionLevel_);
  compressionLevel = compressionLevel < -5 ? -5 : compressionLevel;
  compressionLevel = compressionLevel > 22 ? 22 : compressionLevel;
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Create the buffer for the serialized representation
  // Calculate the exact size of the serialized object in bytes
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int num_serialized_bytes = calc_size_robust(robj);
  stream_buffer_t buf = {.pos = 0};
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Initialize the ZSTD context
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  buf.cctx = ZSTD_createCCtx();
  ZSTD_CCtx_setParameter(buf.cctx, ZSTD_c_compressionLevel, compressionLevel);
  int res = ZSTD_CCtx_setPledgedSrcSize(buf.cctx, num_serialized_bytes);
  if (ZSTD_isError(res)) {
    error("Error on pledge size\n");
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Initialise multithreads if asked
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int num_threads = asInteger(num_threads_);
  if (num_threads > 1) {
    size_t const r = ZSTD_CCtx_setParameter(buf.cctx, ZSTD_c_nbWorkers, num_threads);
    if (ZSTD_isError(r)) {
      Rprintf ("Note: the linked libzstd library doesn't support multithreading. "
                 "Reverting to single-thread mode. \n");
    }
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // calculate maximum possible size of compressed buffer in the worst case
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int max_compressed_bytes  = ZSTD_compressBound(num_serialized_bytes);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Allocate a raw R vector to hold anything up to this size
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP rdst = PROTECT(allocVector(RAWSXP, max_compressed_bytes));
  char *dst = (char *)RAW(rdst);

  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Setup destination for zstd compressed data
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  buf.zstd_buffer.dst  = dst;
  buf.zstd_buffer.size = max_compressed_bytes;
  buf.zstd_buffer.pos  = 0;
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Create the output stream structure for R serialization
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
  ZSTD_inBuffer input = { buf.data, buf.pos, 0 };
  
  size_t remaining_bytes;
  do {
    remaining_bytes = ZSTD_compressStream2(buf.cctx, &(buf.zstd_buffer), &input, ZSTD_e_end);
  } while (remaining_bytes > 0);
  
  ZSTD_freeCCtx(buf.cctx);

  int num_compressed_bytes = buf.zstd_buffer.pos;
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Watch for compression errors
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (num_compressed_bytes <= 0) {
    error("zstd_compress(): Compression error. Status: %i", num_compressed_bytes);
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Truncate the user-viewable size of the RAW vector
  // Requires: R_VERSION >= R_Version(3, 4, 0)
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (num_compressed_bytes < max_compressed_bytes) {
    SETLENGTH(rdst, num_compressed_bytes);
    SET_TRUELENGTH(rdst, max_compressed_bytes);
    SET_GROWABLE_BIT(rdst);
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Tidy and return
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // free(buf->data);
  // free(buf);
  UNPROTECT(1);
  return rdst;
}
