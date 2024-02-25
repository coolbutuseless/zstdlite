#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Serialize arbitrary objects to a compressed stream of bytes using Zstandard
#'
#' @inheritParams zstd_serialize
#' @param robj Any R object understood by \code{base::serialize()}
#'
#' @return serialized representation compressed into a raw byte vector
#'
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
zstd_serialize_stream <- function(robj, level = 3L, num_threads = 1L, cctx = NULL) {
  .Call(zstd_serialize_stream_, robj, level, num_threads, cctx)
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Unserialize stream
#' 
#' @param src raw vector 
#' @param dctx context
#'
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
zstd_unserialize_stream <- function(src, dctx = NULL) {
  .Call(zstd_unserialize_stream_, src, dctx)
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Serialize arbitrary objects to a compressed stream of bytes using Zstandard
#'
#' @inheritParams zstd_serialize_stream
#' @param file filename
#'
#' @return serialized representation compressed into a raw byte vector
#'
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
zstd_serialize_stream_file <- function(robj, file, level = 3L, num_threads = 1L, cctx = NULL) {
  .Call(zstd_serialize_stream_file_, robj, file, level, num_threads, cctx)
}

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Unserialize stream from file
#' 
#' @param src raw vector 
#' @param dctx context
#'
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
zstd_unserialize_stream_file <- function(src, dctx = NULL) {
  .Call(zstd_unserialize_stream_file_, src, dctx)
}




if (FALSE) {
  library(testthat)
  dat <- sample(1e6)
  expect_identical(dat, zstd_unserialize(zstd_serialize_stream(dat)))
  
  c1 <- zstd_serialize(dat)
  c2 <- zstd_serialize_stream(dat)
 
  max_length <- max(c(length(c1), length(c2)))
  length(c1) <- max_length
  length(c2) <- max_length
 
  
  dat <- sample(1e7)
  lobstr::obj_size(dat)
  bench::mark(
    zstd_serialize(dat),
    zstd_serialize(dat, level = -5),
    zstd_serialize_stream(dat),
    zstd_serialize_stream(dat, num_threads = 4),
    rlang::hash(dat),
    qs::qserialize(dat),
    check = FALSE
  )
}