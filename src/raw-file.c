



#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "zstd.h"
#include "buffer-static.h"
#include "calc-size-robust.h"
#include "cctx.h"
#include "dctx.h"
#include "utils.h"
#include "raw-file.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Serialize an R object to a buffer of fixed size and then compress
// the buffer using zstd
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP zstd_compress_(SEXP vec_, SEXP file_, SEXP cctx_, SEXP opts_, SEXP use_file_streaming_) {

  if (!Rf_isNull(file_) && Rf_asLogical(use_file_streaming_)) {
    return zstd_compress_stream_file_(vec_, file_, cctx_, opts_);
  }
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Unpack 'raw' or 'string' arguments
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  unsigned char *src;
  size_t src_size;
  
  if (TYPEOF(vec_) == RAWSXP) {
    src = RAW(vec_);
    src_size = (size_t)Rf_length(vec_);
  } else if (TYPEOF(vec_) == STRSXP) {
    src = (unsigned char *)CHAR(STRING_ELT(vec_, 0));
    src_size = (size_t)strlen(CHAR(STRING_ELT(vec_, 0)));
  } else {
    Rf_error("zstd_compress() only accepts raw vectors or strings");
  }
  

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // calculate maximum possible size of compressed buffer in the worst case
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  size_t dstCapacity  = (size_t)ZSTD_compressBound(src_size);

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Allocate a raw R vector to hold anything up to this size
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int nprotect = 0;
  SEXP dst_ = PROTECT(Rf_allocVector(RAWSXP, (R_xlen_t)dstCapacity)); nprotect++;
  char *dst = (char *)RAW(dst_);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Prepare compression context
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  ZSTD_CCtx* cctx;
  if (Rf_isNull(cctx_)) {
    cctx = init_cctx_with_opts(opts_, 1, 0); // stable buffers
  } else {
    cctx = external_ptr_to_zstd_cctx(cctx_);
    cctx_set_stable_buffers(cctx);
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Compress data
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  size_t num_compressed_bytes = ZSTD_compress2(cctx, dst, dstCapacity, src, src_size);
  if (Rf_isNull(cctx_)) {
    ZSTD_freeCCtx(cctx);
  } else {
    cctx_unset_stable_buffers(cctx);
  }
  
  if (ZSTD_isError(num_compressed_bytes)) {
    Rf_error("zstd_compress(): Compression error. %s", ZSTD_getErrorName(num_compressed_bytes));
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Dump to file - non-streaming 
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (!Rf_isNull(file_)) {
    const char *filename = CHAR(STRING_ELT(file_, 0));
    FILE *fp = fopen(filename, "wb");
    if (fp == NULL) {
      Rf_error("zstd_compress(): Couldn't open file for output '%s'", filename);
    }
    size_t num_written = fwrite(dst, 1, num_compressed_bytes, fp);
    fclose(fp);
    if (num_written != num_compressed_bytes) {
      Rf_error("zstd_compress(): File '%s' only wrote %zu/%zu bytes", filename, num_written, num_compressed_bytes);
    }
    UNPROTECT(1);
    return R_NilValue;
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Truncate the user-viewable size of the RAW vector
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (num_compressed_bytes < dstCapacity) {
    dst_ = PROTECT(Rf_lengthgets(dst_, num_compressed_bytes)); nprotect++;
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Tidy and return
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  UNPROTECT(nprotect);
  return dst_;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Unpack a raw vector to an R object
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP zstd_decompress_(SEXP src_, SEXP type_, SEXP dctx_, SEXP opts_, SEXP use_file_streaming_) {
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Unpack data
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  unsigned char *src;
  size_t src_size;
  
  if (TYPEOF(src_) == STRSXP) {
    if (Rf_asLogical(use_file_streaming_)) {
      return zstd_decompress_stream_file_(src_, type_, dctx_, opts_);
    } else {
      src = read_file(CHAR(STRING_ELT(src_, 0)), &src_size);
    }
  } else if (TYPEOF(src_) == RAWSXP) {
    src = RAW(src_);
    src_size = (size_t)Rf_length(src_);
  } else {
    Rf_error("zstd_compress_() only accepts raw vectors or filenames");
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Find the number of bytes of compressed data in the frame
  // ZSTDLIB_API size_t ZSTD_findFrameCompressedSize(const void* src, size_t srcSize);
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  size_t compressedSize = ZSTD_findFrameCompressedSize(src, src_size);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Determine the final decompressed size in number of bytes
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  size_t dstCapacity = ZSTD_getFrameContentSize(src, compressedSize);

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Create a decompression buffer of the exact required size
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int return_raw = strcmp(CHAR(STRING_ELT(type_, 0)), "raw") == 0;
  
  SEXP dst_;
  unsigned char *dst;
  
  if (return_raw) {
    dst_ = PROTECT(Rf_allocVector(RAWSXP, (R_xlen_t)dstCapacity));
    dst = (void *)RAW(dst_);
  } else {
    dst_ = PROTECT(Rf_allocVector(STRSXP, 1));
    dst = (unsigned char *)malloc(dstCapacity + 1);
    dst[dstCapacity] = 0; // Add "\0" terminator to string
  }  

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Initialise decompression context
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  ZSTD_DCtx * dctx;
  
  if (Rf_isNull(dctx_)) {
    dctx = init_dctx_with_opts(opts_, 1, 0); // This method has a stable buffer
  } else {
    dctx = external_ptr_to_zstd_dctx(dctx_);
    dctx_set_stable_buffers(dctx);
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Decompress
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  size_t status = ZSTD_decompressDCtx(dctx, dst, dstCapacity, src, compressedSize);
  if (ZSTD_isError(status)) {
    if (Rf_isNull(dctx_)) ZSTD_freeDCtx(dctx);
    Rf_error("zstd_decompress_(): De-compression error. %s", ZSTD_getErrorName(status));
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Creating string if this was requested
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (!return_raw) {
    SET_STRING_ELT(dst_, 0, Rf_mkChar((char *)dst));
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Tidy and return
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (TYPEOF(src_) == STRSXP) {
    // We decoded from a file buffer. Free the buffer
    free(src);
  }
  if (Rf_isNull(dctx_)) ZSTD_freeDCtx(dctx);
  UNPROTECT(1);
  return dst_;
}

