#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Serialize arbitrary objects to a compressed stream of bytes using Zstandard
#'
#' @inheritParams zstd_serialize
#' @param robj Any R object understood by \code{base::serialize()}
#'
#' @return serialized representation compressed into a raw byte vector
#'
#' @noRd
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
zstd_serialize_stream <- function(robj, ..., cctx = NULL) {
  .Call(zstd_serialize_stream_, robj, cctx, list(...))
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Unserialize stream
#' 
#' @param src raw vector 
#' @param dctx context
#'
#' @noRd
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
zstd_unserialize_stream <- function(src, ..., dctx = NULL) {
  .Call(zstd_unserialize_stream_, src, dctx, list(...))
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Serialize arbitrary objects to a compressed stream of bytes using Zstandard
#'
#' @inheritParams zstd_serialize_stream
#' @param file filename
#'
#' @return serialized representation compressed into a raw byte vector
#'
#' @noRd
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
zstd_serialize_stream_file <- function(robj, file, ..., cctx = NULL) {
  .Call(zstd_serialize_stream_file_, robj, file, cctx, list(...))
}

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Unserialize stream from file
#' 
#' @param src raw vector 
#' @param dctx context
#'
#' @noRd
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
zstd_unserialize_stream_file <- function(src, ..., dctx = NULL) {
  .Call(zstd_unserialize_stream_file_, src, dctx, list(...))
}

