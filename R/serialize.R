

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Serialize arbitrary objects to a compressed stream of bytes using Zstandard
#'
#' @param robj Any R object understood by \code{base::serialize()}
#' @param file filename in which to serialize data. If NULL (the default), then 
#'        serialize the results to a raw vector
#' @param src Raw vector or filename containing a ZSTD compressed serialized representation of
#'        an R object
#' @param cctx ZSTD Compression Context created by \code{init_zstd_cctx()} or NULL.  
#'        Default: NULL will create a context on the fly
#' @param dctx ZSTD Decompression Context created by \code{init_zstd_dctx()} or NULL.
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
#'
#' @return raw vector of compressed data
#'
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
zstd_compress <- function(src, file = NULL, ..., cctx = NULL) {
  .Call(zstd_compress_, src, file, cctx, list(...))
}



#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Decompress raw bytes
#' 
#' @inheritParams zstd_serialize
#' @param type should data be returned as a 'raw' vector? or as a 'string'? 
#'        Default: 'raw'
#' @param dctx ZSTD Decompression Context
#' 
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
zstd_decompress <- function(src, type = 'raw', ..., dctx = NULL) {
  .Call(zstd_decompress_, src, type, dctx, list(...))
} 

