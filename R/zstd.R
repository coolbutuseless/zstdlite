

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Compress numeric vector/array data with zstd
#'
#' Data must be raw, integer, numeric or logical.  Storage can be a plain
#' atomic vector or a multi-dimensional array.
#'
#' The R object meta-data is not compressed, so attributes (other than the
#' dimensions) of the matrix/vector argument will be lost.
#'
#' To compress general R objects, they can first be converted to a raw byte
#' representation using \code{base::serialize()} e.g.
#' \code{zstdlite::zstd_compress(base::serialize(mtcars))}
#'
#' @param src  Vector, matrix or array of raw, integer, real or logical values.
#' @param level Compression level. Default 3 (to match the command line
#'        interface to zstd).  Higher values run slower but with
#'        more compression. Standard compression values are in range [-5, 22].
#'        Other values are possible, but have little to no effect.
#'
#' @useDynLib zstdlite zstd_compress_
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
zstd_compress <- function(src, level = 3) {
  .Call(zstd_compress_, src, as.integer(level))
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Unpack compressed bytes into the original data with matching dimensions.
#'
#' @param raw_vec Vector of raw values representing the compressed data.
#'        This data must have been created with \code{zstdlite::zstd_compress()}
#'
#' @useDynLib zstdlite zstd_decompress_
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
zstd_decompress <- function(raw_vec) {
  .Call(zstd_decompress_, raw_vec)
}

