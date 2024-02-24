

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Initialise a ZSTD compression context
#' 
#' @param level compression level. Default: 3
#' @param num_threads number of compression threads. Default 1
#' @param dict dictionary. Can either be a raw vector or a filename.  The
#'        dict can be either a raw vector or a filename.  
#' 
#' @return ZSTD Compression Context
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
init_zstd_cctx <- function(level = 3L, num_threads = 1L, dict = NULL) {
  .Call(init_cctx_, level, num_threads, dict)
}

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Initialise a ZSTD compression context
#' 
#' @param dict Dictionary. Default: NULL (no dictioary)
#' 
#' @return ZSTD Decompression Context
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
init_zstd_dctx <- function(dict = NULL) {
  .Call(init_dctx_, dict)
}