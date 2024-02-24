

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Serialize arbitrary objects to a compressed stream of bytes using Zstandard
#'
#' @param robj Any R object understood by \code{base::serialize()}
#' @param level compression level -5 to 22. Default: 3
#' @param raw_vec Raw vector containing a compressed serialized representation of
#'        an R object
#' @param num_threads number of threads to use for compression
#' @param cctx ZSTD Compression Context created by \code{init_zstd_cctx()} or NULL.  
#'        Default: NULL will create a context on the fly
#' @param dctx ZSTD Decompression Context created by \code{init_zstd_dctx()} or NULL.
#'        Default: NULL will decompression without any context using default seetings.
#'
#' @return serialized representation compressed into a raw byte vector
#'
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
zstd_serialize <- function(robj, level = 3L, num_threads = 1L, cctx = NULL) {
  .Call('zstd_serialize_', robj, level, num_threads, cctx)
}



#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' @rdname zstd_serialize
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
zstd_unserialize <- function(raw_vec, dctx = NULL) {
  .Call('zstd_unserialize_', raw_vec, dctx)
}




#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Serialize arbitrary objects to a compressed stream of bytes using Zstandard
#'
#' @inheritParams zstd_serialize
#' @param raw_vec raw vector
#'
#' @return raw vector
#'
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
zstd_compress <- function(raw_vec, level = 3L, num_threads = 1L, cctx = NULL) {
  .Call('zstd_compress_', raw_vec, level, num_threads, cctx)
}



#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Decompress raw bytes
#' 
#' @param raw_vec raw vector 
#' @param dctx ZSTD Decompression Context
#' 
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
zstd_decompress <- function(raw_vec, dctx = NULL) {
  .Call('zstd_decompress_', raw_vec, dctx)
}

