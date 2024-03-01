

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Initialise a ZSTD compression context
#' 
#' @param level compression level. Default: 3
#' @param num_threads number of compression threads. Default 1
#' @param include_checksum include a checksum with the compressed data. Default: FALSE
#' @param dict dictionary. Can either be a raw vector or a filename.  The
#'        dict can be either a raw vector or a filename.  
#' 
#' @return ZSTD Compression Context
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
init_zstd_cctx <- function(level = 3L, num_threads = 1L, include_checksum = FALSE, dict = NULL) {
  .Call(init_cctx_, level, num_threads, include_checksum, dict)
}

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Initialise a ZSTD compression context
#' 
#' @param validate_checksum Should the checksum of the data be validated if it is present? 
#'        Default: TRUE.  Set to \code{FALSE} to ignore the checksum, which 
#'        can lead to a minor speed improvement.
#'        If no checksum is present in the compressed data, then this option has no 
#'        effect.  
#' @param dict Dictionary. Default: NULL (no dictionary)
#' 
#' @return ZSTD Decompression Context
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
init_zstd_dctx <- function(validate_checksum = TRUE, dict = NULL) {
  .Call(init_dctx_, validate_checksum, dict)
}