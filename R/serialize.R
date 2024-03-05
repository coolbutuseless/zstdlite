

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Serialize arbitrary R objects to a compressed stream of bytes using Zstandard
#'
#' @param robj Any R object understood by \code{base::serialize()}
#' @param file filename in which to serialize data. If NULL (the default), then 
#'        serialize the results to a raw vector
#' @param src Raw vector or filename containing a ZSTD compressed serialized representation of
#'        an R object
#' @param cctx ZSTD Compression Context created by \code{zstd_cctx()} or NULL.  
#'        Default: NULL will create a default compression context on-the-fly
#' @param dctx ZSTD Decompression Context created by \code{zstd_dctx()} or NULL.
#'        Default: NULL will create a default decompression context on-the-fly.
#' @param use_file_streaming Use the streaming interface when reading or writing
#'        to a file?  This may reduce memory allocations
#'        and make better use of mutlithreading.  Default: FALSE
#' @param ... extra arguments passed to \code{zstd_cctx()} or \code{zstd_dctx()}
#'        context initializers. 
#'        Note: These argument are only used when \code{cctx} or \code{dctx} is NULL
#'
#' @return serialized representation compressed into a raw byte vector
#'
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
zstd_serialize <- function(robj, ..., file = NULL, cctx = NULL, use_file_streaming = FALSE) {
  .Call(zstd_serialize_, robj, file, cctx, list(...), use_file_streaming)
}



#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' @rdname zstd_serialize
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
zstd_unserialize <- function(src, ..., dctx = NULL, use_file_streaming = FALSE) {
  .Call(zstd_unserialize_, src, dctx, list(...), use_file_streaming)
}




#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Compress raw vectors and character strings.
#' 
#' This function is appropriate when handling data from other systems e.g.
#' data compressed with the \code{zstd} command-line, or other compression
#' programs.
#'
#' @inheritParams zstd_serialize
#' @param src Source data to be compressed.  This may be a raw vector, or a
#'        character string
#'
#' @return Raw vector of compressed data, or file created with compressed data
#'
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
zstd_compress <- function(src, ..., file = NULL, cctx = NULL, use_file_streaming = FALSE) {
  .Call(zstd_compress_, src, file, cctx, list(...), use_file_streaming)
}



#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Decompress raw bytes
#' 
#' @inheritParams zstd_serialize
#' @inheritParams zstd_compress
#' @param type Should data be returned as a 'raw' vector or as a 'string'? 
#'        Default: 'raw'
#' 
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
zstd_decompress <- function(src, type = 'raw', ..., dctx = NULL, use_file_streaming = FALSE) {
  .Call(zstd_decompress_, src, type, dctx, list(...), use_file_streaming)
} 

