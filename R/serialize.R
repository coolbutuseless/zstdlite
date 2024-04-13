
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Get version string of zstd C library
#' 
#' @return String containing version number of zstd C library
#' @export
#'
#' @examples
#' zstd_version()
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
zstd_version <- function() {
  .Call(zstd_version_);
}



#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Serialize/Unserialize arbitrary R objects to a compressed stream of bytes using Zstandard
#'
#' @param robj Any R object understood by \code{base::serialize()}
#' @param dst filename in which to serialize data. If NULL (the default), then 
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
#' @return Raw vector of compressed serialized data, or \code{NULL} if file 
#'         created with compressed data
#'
#' @export
#' 
#' @examples
#' # Raw vector
#' vec <- zstd_serialize(mtcars)
#' zstd_unserialize(src = vec)
#' 
#' # file
#' tmp <- tempfile()
#' zstd_serialize(mtcars, dst = tmp)
#' zstd_unserialize(src = tmp)
#' 
#' # connection
#' tmp <- tempfile()
#' zstd_serialize(mtcars, dst = file(tmp))
#' zstd_unserialize(src = file(tmp))
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
zstd_serialize <- function(robj, ..., dst = NULL, cctx = NULL, use_file_streaming = FALSE) {
  if (!inherits(dst, 'connection')) {
    .Call(zstd_serialize_, robj, dst, cctx, list(...), use_file_streaming)
  } else {
    if(!isOpen(dst)){
      on.exit(close(dst)) 
      open(dst, "wb")
    }
    .Call(zstd_serialize_conn_, robj, dst, cctx, list(...))
  }
}



#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' @rdname zstd_serialize
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
zstd_unserialize <- function(src, ..., dctx = NULL, use_file_streaming = FALSE) {
  if (!inherits(src, 'connection')) {
    .Call(zstd_unserialize_, src, dctx, list(...), use_file_streaming)
  } else {
    if(!isOpen(src)){
      on.exit(close(src)) 
      open(src, "rb")
    }
    .Call(zstd_unserialize_conn_, src, dctx, list(...))
  }
}



#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Compress/Decompress raw vectors and character strings.
#' 
#' This function is appropriate when handling data from other systems e.g.
#' data compressed with the \code{zstd} command-line, or other compression
#' programs.
#'
#' @inheritParams zstd_serialize
#' @param dst destination in which to write the compressed data. If \code{NULL}
#'        (the default) data will be returned as a raw vector.  If a string, 
#'        then this will be the filename to which the data is written.  \code{dst}
#'        may also be a connection object e.g. \code{pipe()}, \code{file()} etc.
#' @param src Source from which compressed data is read. If a string, 
#'        then this will be the filename to read data from.  \code{dst}
#'        may also be a connection object e.g. \code{pipe()}, \code{file()} etc.
#' @param x Data to be compressed.  This may be a raw vector, or a
#'        character string
#' @param type Should data be returned as a 'raw' vector or as a 'string'? 
#'        Default: 'raw'
#'
#' @return Raw vector of compressed data, or \code{NULL} if file created with compressed data
#'
#' @export
#' 
#' @examples
#' # With raw vectors
#' dat <- sample(as.raw(1:10), 1000, replace = TRUE)
#' vec <- zstd_compress(x = dat)
#' zstd_decompress(src = vec)
#' 
#' # With files
#' tmp <- tempfile()
#' zstd_compress(x = dat, dst = tmp)
#' zstd_decompress(src = tmp)
#' 
#' # With connections
#' tmp <- tempfile()
#' zstd_compress(x = dat, dst = file(tmp))
#' zstd_decompress(src = file(tmp))
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
zstd_compress <- function(x, ..., dst = NULL, cctx = NULL, use_file_streaming = FALSE) {
  if (!inherits(dst, 'connection')) {
    .Call(zstd_compress_, x, dst, cctx, list(...), use_file_streaming)
  } else {
    if(!isOpen(dst)){
      on.exit(close(dst)) 
      open(dst, "wb")
    }
    .Call(zstd_compress_conn_, x, dst, cctx, list(...))
  }
}



#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' @rdname zstd_compress
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
zstd_decompress <- function(src, type = 'raw', ..., dctx = NULL, use_file_streaming = FALSE) {
  if (!inherits(src, 'connection')) {
    .Call(zstd_decompress_, src, type, dctx, list(...), use_file_streaming)
  } else {
    if(!isOpen(src)){
      on.exit(close(src))
      open(src, "rb")
    }
    .Call(zstd_decompress_conn_, src, type, dctx, list(...))
  }
} 


