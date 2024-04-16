
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Return information about the zstd stream
#' 
#' @param src raw vector, file or connection
#' 
#' @return named list with \code{compressed_size}, \code{uncompressed_size}, 
#'         \code{dict_id} and \code{has_checksum}.  If an error occurs, or 
#'         the data does not appear to represent Zstandard compressed data,
#'         function returns \code{NULL}
#' @export
#' 
#' @examples
#' data <- as.raw(sample(1:2, 10000, replace = TRUE))
#' cdata <- zstd_compress(data)
#' zstd_info(cdata)
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
zstd_info <- function(src) {
  .Call(zstd_info_, src)
}

