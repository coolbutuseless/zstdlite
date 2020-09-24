



#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "zstd.h"
#include "buffer-static.h"
#include "calc-size-robust.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Serialize an R object
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP zstd_serialize_(SEXP robj, SEXP compressionLevel_) {

  // Create the buffer for the serialized representation
  // See also: `expand_buffer()` which re-allocates the memory buffer if
  // it runs out of space
  int start_size = calc_size_robust(robj);

  static_buffer_t *buf = init_buffer(start_size);


  // Create the output stream structure
  struct R_outpstream_st output_stream;

  // Initialise the output stream structure
  R_InitOutPStream(
    &output_stream,          // The stream object which wraps everything
    (R_pstream_data_t) buf,  // The actual data
    R_pstream_binary_format, // Store as binary
    3,                       // Version = 3 for R >3.5.0 See `?base::serialize`
    write_byte,              // Function to write single byte to buffer
    write_bytes,             // Function for writing multiple bytes to buffer
    NULL,                    // Func for special handling of reference data.
    R_NilValue               // Data related to reference data handling
  );

  // Serialize the object into the output_stream
  R_Serialize(robj, &output_stream);

  int srcSize = buf->pos;

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // calculate maximum possible size of compressed buffer in the worst case
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int dstCapacity  = ZSTD_compressBound(srcSize);

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Create a temporary buffer to hold anything up to this size
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  char *dst;
  dst = (char *)malloc(dstCapacity);
  if (!dst) {
    error("Couldn't allocate compression buffer of size: %i\n", dstCapacity);
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Compress the data into the temporary buffer
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int compressionLevel = asInteger(compressionLevel_);
  compressionLevel = compressionLevel < -5 ? -5 : compressionLevel;
  compressionLevel = compressionLevel > 22 ? 22 : compressionLevel;

  ZSTD_CCtx* cctx = ZSTD_createCCtx();
  ZSTD_CCtx_setParameter(cctx, ZSTD_c_compressionLevel, compressionLevel);
  int num_compressed_bytes = ZSTD_compress2(cctx, dst, dstCapacity, buf->data, srcSize);
  ZSTD_freeCCtx(cctx);


  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Watch for compression errors
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (num_compressed_bytes <= 0) {
    error("zstd_compress(): Compression error. Status: %i", num_compressed_bytes);
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Allocate a raw R vector of the exact length hold the compressed data
  // and memcpy just the compressed bytes into it.
  // Allocate more bytes than necessary so that there is a minimal header
  // at the front of the compressed data with
  //  - 3 bytes: magic bytes: LZ4
  //  - 1 byte: SEXP type
  //
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int magic_length = 4; // magic bytes + SEXP type
  SEXP rdst = PROTECT(allocVector(RAWSXP, num_compressed_bytes + magic_length));
  char *rdstp = (char *)RAW(rdst);



  memcpy(rdstp + magic_length, dst, num_compressed_bytes);

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Set 3 magic bytes for the header, and 1 byte for ndims + sexp type
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  rdstp[0] = 'Z'; /* 'ZST' */
  rdstp[1] = 'S'; /* 'ZST' */
  rdstp[2] = 'T'; /* 'ZST' */
  rdstp[3] =  0;    /* Store SEXP type here */


  // Free all the memory
  free(buf->data);
  free(buf);
  UNPROTECT(1);
  return rdst;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Unpack a raw vector to an R object
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP zstd_unserialize_(SEXP src_) {

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Sanity check
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (TYPEOF(src_) != RAWSXP) {
    error("unpack(): Only raw vectors can be unserialized");
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // A C pointer into the raw data
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  unsigned char *src = (unsigned char *)RAW(src_);

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Check the magic bytes are correct i.e. there is a header with length info
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (src[0] != 'Z' | src[1] != 'S' | src[2] != 'T') {
    error("zstd_decompress(): Buffer must be ZSTD data compressed with 'zstdlite'. 'ZST' expected as header, but got - '%c%c%c'",
          src[0], src[1], src[2]);
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Get the SEXP type and the number of dimensions
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int magic_length = 4;       // 3 magic bytes + SEXP type


  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Find the number of bytes of compressed data in the frame
  // ZSTDLIB_API size_t ZSTD_findFrameCompressedSize(const void* src, size_t srcSize);
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int compressedSize = ZSTD_findFrameCompressedSize(src + magic_length, length(src_)); // TODO: '- magic_length'

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Determine the final decompressed size in number of bytes
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int dstCapacity = (int)ZSTD_getFrameContentSize(src + magic_length, compressedSize);


  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Create a decompression buffer of the exact required size
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  void *dst = malloc(dstCapacity);

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Decompress
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int status = ZSTD_decompress(dst, dstCapacity, src + magic_length, compressedSize);

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Watch for decompression errors
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (status <= 0) {
    error("zstd_unserialize(): De-compression error. Status: %i", status);
  }


  // Create a buffer object which points to the raw data
  static_buffer_t *buf = malloc(sizeof(static_buffer_t));
  if (buf == NULL) {
    error("'buf' malloc failed!");
  }
  buf->length = dstCapacity;
  buf->pos    = 0;
  buf->data   = dst;

  // Treat the data buffer as an input stream
  struct R_inpstream_st input_stream;

  R_InitInPStream(
    &input_stream,           // Stream object wrapping data buffer
    (R_pstream_data_t) buf,  // Actual data buffer
    R_pstream_any_format,    // Unpack all serialized types
    read_byte,               // Function to read single byte from buffer
    read_bytes,              // Function for reading multiple bytes from buffer
    NULL,                    // Func for special handling of reference data.
    NULL                     // Data related to reference data handling
  );

  // Unserialize the input_stream into an R object
  SEXP res_  = PROTECT(R_Unserialize(&input_stream));

  free(buf);
  UNPROTECT(1);
  return res_;
}

