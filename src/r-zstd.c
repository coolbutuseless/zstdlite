
#include <R.h>
#include <Rinternals.h>

#include "zstd.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// from: https://cran.r-project.org/doc/manuals/r-release/R-ints.html
//  SEXPTYPE	Description
//   0	  0  NILSXP	      NULL
//   1	  1  SYMSXP	      symbols
//   2	  2  LISTSXP	    pairlists
//   3	  3  CLOSXP	      closures
//   4	  4  ENVSXP	      environments
//   5	  5  PROMSXP	    promises
//   6	  6  LANGSXP	    language objects
//   7	  7  SPECIALSXP	  special functions
//   8	  8  BUILTINSXP	  builtin functions
//   9	  9  CHARSXP	    internal character strings
//  10	  a  LGLSXP	      logical vectors
//  11   b
//  12   c
//  13	  d  INTSXP	      integer vectors
//  14	  e  REALSXP	    numeric vectors
//  15	  f  CPLXSXP	    complex vectors
//  16	 10  STRSXP	      character vectors
//  17	 11  DOTSXP	      dot-dot-dot object
//  18	 12  ANYSXP	      make “any” args work
//  19	 13  VECSXP	      list (generic vector)
//  20	 14  EXPRSXP	    expression vector
//  21	 15  BCODESXP	    byte code
//  22	 16  EXTPTRSXP	  external pointer
//  23	 17  WEAKREFSXP	  weak reference
//  24	 18  RAWSXP       raw vector
//  25	 19  S4SXP	      S4 classes not of simple type
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//  Compresses `src` content as a single zstd compressed frame into already allocated `dst`.
//  Hint : compression runs faster if `dstCapacity` >=  `ZSTD_compressBound(srcSize)`.
//  @return : compressed size written into `dst` (<= `dstCapacity),
//            or an error code if it fails (which can be tested using ZSTD_isError()).
//
// ZSTDLIB_API size_t ZSTD_compress( void* dst, size_t dstCapacity,
//                                  const void* src, size_t srcSize,
//                                  int compressionLevel);
//
// @param src_ buffer to be compressed. Raw bytes.
// @param acc_ acceleration. integer
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP zstd_compress_(SEXP src_, SEXP compressionLevel_) {

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // SEXP type will be used to determine multiplier for data size
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int sexp_type = TYPEOF(src_);
  int srcSize   = length(src_);
  void *src;

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Get a pointer to the data part of this vector
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  src = (void *)DATAPTR(src_);

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Adjust 'srcSize' for the given datatype
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  switch(sexp_type) {
  case LGLSXP:
  case INTSXP:
    srcSize *= 4;
    break;
  case REALSXP:
    srcSize *= 8;
    break;
  case CPLXSXP:
    srcSize *= 16;
    break;
  case RAWSXP:
    break;
  default:
    error("zstd_compress() cannot handles SEXP type: %i\n", sexp_type);
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // If this is an array, get the dimensions to store in the zstd header
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  unsigned int ndims = 0;
  unsigned int dims[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  if (isArray(src_)) {
    SEXP x_ = PROTECT(getAttrib(src_, R_DimSymbol));
    ndims = length(x_);
    if (ndims > 7) {
      // Only have 3 bits to store this info at the top of the 'sexp_type' byte.
      error("zstd_compress(): 7-dimensional arrays are the maximum.");
    }
    for (int i = 0; i < ndims; i++) {
      if (INTEGER(x_)[i] > INT32_MAX) {
        // Todo: long vector support
        error("zstd_compress(): Long vectors not supported.");
      }
      dims[i] = INTEGER(x_)[i];
    }
    UNPROTECT(1);
  } else if (!isVectorAtomic(src_)) {
    error("zstd_compress(): Unknown class. Was expecting an array, matrix or atomic vector");
  }



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
  //Rprintf("Comp level: %i\n", asInteger(compressionLevel_));
  int num_compressed_bytes = ZSTD_compress(dst, dstCapacity, src, srcSize, asInteger(compressionLevel_));

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
  int dim_length = ndims * 4; // 4 bytes per int

  SEXP rdst = PROTECT(allocVector(RAWSXP, num_compressed_bytes + magic_length + dim_length));
  char *rdstp = (char *)RAW(rdst);
  unsigned int *idstp = (unsigned int *)RAW(rdst);

  memcpy(rdstp + magic_length + dim_length, dst, num_compressed_bytes);

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Store number of dimensions in the upper 3 bits of the sexp_type
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  sexp_type |= (ndims << 5);

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Set 3 magic bytes for the header, and 1 byte for ndims + sexp type
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  rdstp[0] = 'Z'; /* 'ZST' */
  rdstp[1] = 'S'; /* 'ZST' */
  rdstp[2] = 'T'; /* 'ZST' */
  rdstp[3] = (char)sexp_type;    /* Store SEXP type here */

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Copy the dimensions into the header starting at the 5th byte
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  for (int i = 0; i < ndims; i++) {
    idstp[1 + i] = dims[i];
  }


  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Free memory, unprotect R objects and return
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  free(dst);
  UNPROTECT(1);

  return rdst;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ZSTD_decompress() :
//  `compressedSize` : must be the _exact_ size of some number of compressed and/or skippable frames.
//  `dstCapacity` is an upper bound of originalSize to regenerate.
//  If user cannot imply a maximum upper bound, it's better to use streaming mode to decompress data.
//  @return : the number of bytes decompressed into `dst` (<= `dstCapacity`),
//            or an errorCode if it fails (which can be tested using ZSTD_isError()).
//
// ZSTDLIB_API size_t ZSTD_decompress( void* dst, size_t dstCapacity,
//                                     const void* src, size_t compressedSize);
//
// @param src_ buffer to be decompressed. Raw bytes. The first 4 bytes of this
//        must be a header with bytes[0:2] = 'ZST'.  Byte[3] is the SEXPTYPE.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP zstd_decompress_(SEXP src_) {

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
  // Create a decompression buffer of the exact required size
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP dst_;
  void *dst;

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Get the SEXP type and the number of dimensions
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int sexp_type = src[3] & 31;  // Lower 5 bits is SEXP type
  int ndims = src[3] >> 5;

  int magic_length = 4;       // 3 magic bytes + SEXP type
  int dim_length = ndims * 4; // 4 bytes per int

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Find the number of bytes of compressed data in the frame
  // ZSTDLIB_API size_t ZSTD_findFrameCompressedSize(const void* src, size_t srcSize);
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int compressedSize = ZSTD_findFrameCompressedSize(src + magic_length + dim_length, length(src_));


  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Determine the final decompressed size in number of bytes
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int dstCapacity = (int)ZSTD_getFrameContentSize(src + magic_length + dim_length, compressedSize);


  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Create an R vector of the appropriate type and a (void *) into it
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  switch(sexp_type) {
  case LGLSXP:
    dst_ = PROTECT(allocVector(LGLSXP, dstCapacity/4));
    dst = (void *)LOGICAL(dst_);
    break;
  case INTSXP:
    dst_ = PROTECT(allocVector(INTSXP, dstCapacity/4));
    dst = (void *)INTEGER(dst_);
    break;
  case REALSXP:
    dst_ = PROTECT(allocVector(REALSXP, dstCapacity/8));
    dst = (void *)REAL(dst_);
    break;
  case RAWSXP:
    dst_ = PROTECT(allocVector(RAWSXP, dstCapacity));
    dst = (void *)RAW(dst_);
    break;
  case CPLXSXP:
    dst_ = PROTECT(allocVector(CPLXSXP, dstCapacity/16));
    dst = (void *)COMPLEX(dst_);
    break;
  default:
    error("zstd_decompress(): cannot handles SEXP type: %i\n", sexp_type);
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Decompress
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int status = ZSTD_decompress(dst, dstCapacity, src + magic_length + dim_length, compressedSize);

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Watch for decompression errors
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (status <= 0) {
    error("zstd_decompress(): De-compression error. Status: %i", status);
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // If there is are multiple dimensions, then set the dimension attribute
  // on the returned vector to make it an array or matrix.
  // The elements of the dimension attribute are stored in 4-byte ints
  // starting after the first 4 bytes.
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (ndims > 0) {
    SEXP dims_ = PROTECT(allocVector(INTSXP, ndims));
    unsigned int *isrc = (unsigned int *)src;
    for (int i = 0; i < ndims; i++) {
      INTEGER(dims_)[i] = isrc[i + 1];
    }
    setAttrib(dst_, R_DimSymbol, dims_);
    UNPROTECT(1);
  }


  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Unprotect and return
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  UNPROTECT(1);
  return dst_;
}
