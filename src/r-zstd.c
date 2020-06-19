
#include <R.h>
#include <Rinternals.h>

#include "zstd.h"

/*  from: https://cran.r-project.org/doc/manuals/r-release/R-ints.html
 SEXPTYPE	Description
  0	  0  NILSXP	      NULL
  1	  1  SYMSXP	      symbols
  2	  2  LISTSXP	    pairlists
  3	  3  CLOSXP	      closures
  4	  4  ENVSXP	      environments
  5	  5  PROMSXP	    promises
  6	  6  LANGSXP	    language objects
  7	  7  SPECIALSXP	  special functions
  8	  8  BUILTINSXP	  builtin functions
  9	  9  CHARSXP	    internal character strings
 10	  a  LGLSXP	      logical vectors
 11   b
 12   c
 13	  d  INTSXP	      integer vectors
 14	  e  REALSXP	    numeric vectors
 15	  f  CPLXSXP	    complex vectors
 16	 10  STRSXP	      character vectors
 17	 11  DOTSXP	      dot-dot-dot object
 18	 12  ANYSXP	      make “any” args work
 19	 13  VECSXP	      list (generic vector)
 20	 14  EXPRSXP	    expression vector
 21	 15  BCODESXP	    byte code
 22	 16  EXTPTRSXP	  external pointer
 23	 17  WEAKREFSXP	  weak reference
 24	 18  RAWSXP       raw vector
 25	 19  S4SXP	      S4 classes not of simple type
 */



/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  Compresses `src` content as a single zstd compressed frame into already allocated `dst`.
 *  Hint : compression runs faster if `dstCapacity` >=  `ZSTD_compressBound(srcSize)`.
 *  @return : compressed size written into `dst` (<= `dstCapacity),
 *            or an error code if it fails (which can be tested using ZSTD_isError()).
 *
 * ZSTDLIB_API size_t ZSTD_compress( void* dst, size_t dstCapacity,
 *                                  const void* src, size_t srcSize,
 *                                  int compressionLevel);
 *
 * @param src_ buffer to be compressed. Raw bytes.
 * @param acc_ acceleration. integer
 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
SEXP zstd_compress_(SEXP src_, SEXP compressionLevel_) {

  /* SEXP type will determine multiplier for data size */
  int sexp_type = TYPEOF(src_);
  int srcSize   = length(src_);
  void *src;

  /* Get a pointer to the data */
  src = (void *)DATAPTR(src_);

  /* Adjust 'srcSize' for the given datatype */
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
    error("compress() cannot handles SEXP type: %i\n", sexp_type);
  }

  /* calculate maximum possible size of compressed buffer in the worst case */
  int dstCapacity  = ZSTD_compressBound(srcSize);

  /* Create a temporary buffer to hold anything up to this size */
  char *dst;
  dst = (char *)malloc(dstCapacity);
  if (!dst) {
    error("Couldn't allocate compression buffer of size: %i\n", dstCapacity);
  }

  /* Compress the data into the temporary buffer */
  int status = ZSTD_compress(dst, dstCapacity, src, srcSize, asInteger(compressionLevel_));

  /* Watch for badness */
  if (status <= 0) {
    error("Compression error. Status: %i", status);
  }

  /* Allocate a raw R vector of the exact length hold the compressed data
   * and memcpy just the compressed bytes into it.
   * Allocate 4 more bytes than necessary so that there is a minimal header
   * at the front of the compressed data with
   *  - 3 bytes: magic bytes: LZ4
   *  - 1 byte: SEXP type
   */
  SEXP rdst = PROTECT(allocVector(RAWSXP, status + 4));
  char *rdstp = (char *)RAW(rdst);
  memcpy(rdstp + 4, dst, status);

  /* Do some header twiddling */
  rdstp[0] = 'Z'; /* 'ZST' */
  rdstp[1] = 'S'; /* 'ZST' */
  rdstp[2] = 'T'; /* 'ZST' */
  rdstp[3] = (char)sexp_type;    /* Store SEXP type here */

  free(dst);
  UNPROTECT(1);
  return rdst;
}



/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * ZSTD_decompress() :
 *  `compressedSize` : must be the _exact_ size of some number of compressed and/or skippable frames.
 *  `dstCapacity` is an upper bound of originalSize to regenerate.
 *  If user cannot imply a maximum upper bound, it's better to use streaming mode to decompress data.
 *  @return : the number of bytes decompressed into `dst` (<= `dstCapacity`),
 *            or an errorCode if it fails (which can be tested using ZSTD_isError()).
 *
 * ZSTDLIB_API size_t ZSTD_decompress( void* dst, size_t dstCapacity,
 *                                     const void* src, size_t compressedSize);
 *
 * @param src_ buffer to be decompressed. Raw bytes. The first 4 bytes of this
 *        must be a header with bytes[0:2] = 'ZST'.  Byte[3] is the SEXPTYPE.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
SEXP zstd_decompress_(SEXP src_) {

  /* A pointers into the buffer */
  char *src = (char *)RAW(src_);

  /* Check the magic bytes are correct i.e. there is a header with length info */
  if (src[0] != 'Z' | src[1] != 'S' | src[2] != 'T') {
    error("Buffer must be ZSTD data compressed with 'zstdlite'. 'ZST' expected as header, but got - '%c%c%c'",
          src[0], src[1], src[2]);
  }

  /* Find the number of bytes of compressed data in src (subtract header) */
  int compressedSize = length(src_) - 4;

  /* Determine the total number of decompressed bytes final decompressed size */
  int dstCapacity = (int)ZSTD_getFrameContentSize(src + 4, compressedSize);

  /* Create a decompression buffer of the exact required size and do decompression */
  SEXP dst_;
  void *dst;
  int sexp_type = src[3];

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
    error("decompress() cannot handles SEXP type: %i\n", sexp_type);
  }

  int status = ZSTD_decompress(dst, dstCapacity, src + 4, compressedSize);

  /* Watch for badness */
  if (status <= 0) {
    error("De-compression error. Status: %i", status);
  }

  UNPROTECT(1);
  return dst_;
}
