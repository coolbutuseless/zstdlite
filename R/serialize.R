

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Serialize arbitrary objects to a compressed stream of bytes using Zstandard
#'
#' @param robj Any R object understood by \code{base::serialize()}
#' @param file filename in which to serialize data. If NULL (the default), then 
#'        serialize the results to a raw vector
#' @param src Raw vector or filename containing a ZSTD compressed serialized representation of
#'        an R object
#' @param cctx ZSTD Compression Context created by \code{zstd_cctx()} or NULL.  
#'        Default: NULL will create a context on the fly
#' @param dctx ZSTD Decompression Context created by \code{zstd_dctx()} or NULL.
#'        Default: NULL will decompression without any context using default seetings.
#' @param ... extra arguments passed to context initializer
#'
#' @return serialized representation compressed into a raw byte vector
#'
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
zstd_serialize <- function(robj, file = NULL, ..., cctx = NULL) {
  .Call(zstd_serialize_, robj, file, cctx, list(...))
}



#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' @rdname zstd_serialize
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
zstd_unserialize <- function(src, ..., dctx = NULL) {
  .Call(zstd_unserialize_, src, dctx, list(...))
}




#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Serialize arbitrary objects to a compressed stream of bytes using Zstandard
#'
#' @inheritParams zstd_serialize
#' @param src Source data to be compressed.  This may be a raw vector, or a
#'        character string
#' @param use_file_streaming use the streaming interface.  Can reduce memory pressure
#'        and make better use of mutlithreading.  default: FALSE
#'
#' @return raw vector of compressed data
#'
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
zstd_compress <- function(src, file = NULL, ..., cctx = NULL, use_file_streaming = FALSE) {
  .Call(zstd_compress_, src, file, cctx, list(...), use_file_streaming)
}



#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Decompress raw bytes
#' 
#' @inheritParams zstd_serialize
#' @inheritParams zstd_compress
#' @param type should data be returned as a 'raw' vector? or as a 'string'? 
#'        Default: 'raw'
#' @param dctx ZSTD Decompression Context
#' 
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
zstd_decompress <- function(src, type = 'raw', ..., dctx = NULL, use_file_streaming = FALSE) {
  .Call(zstd_decompress_, src, type, dctx, list(...), use_file_streaming)
} 

if (FALSE) {
  
  zz <- as.raw(sample(255, 100000000, T))
  lobstr::obj_size(zz)
  cctx <- zstd_cctx(num_threads = 2)
  bench::mark(
    zstd_compress(zz, "working/z1", use_file_streaming = FALSE),
    zstd_compress(zz, "working/z1", use_file_streaming = FALSE, num_threads = 2),
    zstd_compress(zz, "working/z1", use_file_streaming = TRUE),
    zstd_compress(zz, "working/z1", use_file_streaming = TRUE, num_threads = 2),
    zstd_compress(zz, "working/z1", use_file_streaming = FALSE, cctx = cctx),
    zstd_compress(zz, "working/z1", use_file_streaming = TRUE, cctx = cctx)
  )
  
  
  zz <- as.raw(sample(5, 10000000, T))
  lobstr::obj_size(zz)
  zstd_compress(zz, "working/z1")
  yy <- zstd_decompress("working/z1", use_file_streaming = TRUE, num_threads = 2)
  identical(yy, zz)
}
