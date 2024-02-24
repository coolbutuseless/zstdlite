



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

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Serialize an R object to a buffer of fixed size and then compress
// the buffer using zstd
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP zstd_compress_(SEXP vec_, SEXP level_, SEXP num_threads_, SEXP cctx_) {

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Sanity check
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (TYPEOF(vec_) != RAWSXP) {
    error("zstd_compress_() only accepts raw vectors");
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // calculate maximum possible size of compressed buffer in the worst case
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int dstCapacity  = ZSTD_compressBound(length(vec_));

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Allocate a raw R vector to hold anything up to this size
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP rdst = PROTECT(allocVector(RAWSXP, dstCapacity));
  char *dst = (char *)RAW(rdst);
  
  ZSTD_CCtx* cctx;
  if (isNull(cctx_)) {
    cctx = init_cctx(asInteger(level_), asInteger(num_threads_));
  } else {
    cctx = external_ptr_to_zstd_cctx(cctx_);
    ZSTD_CCtx_reset(cctx, ZSTD_reset_session_only);
  }

  int num_compressed_bytes = ZSTD_compress2(cctx, dst, dstCapacity, RAW(vec_), length(vec_));
  
  if (isNull(cctx_)) {
    ZSTD_freeCCtx(cctx);
  }


  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Watch for compression errors
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (ZSTD_isError(num_compressed_bytes)) {
    error("zstd_compress(): Compression error. %s", ZSTD_getErrorName(num_compressed_bytes));
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Truncate the user-viewable size of the RAW vector
  // Requires: R_VERSION >= R_Version(3, 4, 0)
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (num_compressed_bytes < dstCapacity) {
    SETLENGTH(rdst, num_compressed_bytes);
    SET_TRUELENGTH(rdst, dstCapacity);
    SET_GROWABLE_BIT(rdst);
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Tidy and return
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  UNPROTECT(1);
  return rdst;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Unpack a raw vector to an R object
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP zstd_decompress_(SEXP src_, SEXP dctx_) {

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Sanity check
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (TYPEOF(src_) != RAWSXP) {
    error("zstd_decompress(): Only raw vectors can be unserialized");
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // A C pointer into the raw data
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  unsigned char *src = (unsigned char *)RAW(src_);

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Find the number of bytes of compressed data in the frame
  // ZSTDLIB_API size_t ZSTD_findFrameCompressedSize(const void* src, size_t srcSize);
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int compressedSize = ZSTD_findFrameCompressedSize(src, length(src_));

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Determine the final decompressed size in number of bytes
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int dstCapacity = (int)ZSTD_getFrameContentSize(src, compressedSize);

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Create a decompression buffer of the exact required size
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP dst_ = PROTECT(allocVector(RAWSXP, dstCapacity));
  void *dst = (void *)RAW(dst_);

  size_t status;
  
  if (isNull(dctx_)) {
    status = ZSTD_decompress(dst, dstCapacity, src, compressedSize);
  } else {
    ZSTD_DCtx * dctx = external_ptr_to_zstd_dctx(dctx_);
    status = ZSTD_decompressDCtx(dctx, dst, dstCapacity, src, compressedSize);
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Watch for decompression errors
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (ZSTD_isError(status)) {
    error("zstd_unserialize(): De-compression error. %s", ZSTD_getErrorName(status));
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Tidy and return
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  UNPROTECT(1);
  return dst_;
}

