

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Serialize arbitrary objects to a compressed stream of bytes using Zstandard
#'
#' @param robj Any R object understood by \code{base::serialize()}
#' @param level compression level -5 to 22. Default: 3
#' @param raw_vec Raw vector containing a compressed serialized representation of
#'        an R object
#'
#' @return serialized representation compressed into a raw byte vector
#'
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
zstd_serialize <- function(robj, level = 3L) {
  .Call('zstd_serialize_', robj, as.integer(level))
}



#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' @rdname zstd_serialize
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
zstd_unserialize <- function(raw_vec) {
  .Call('zstd_unserialize_', raw_vec)
}

