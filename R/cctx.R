

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Initialise a ZSTD compression context
#' 
#' @param level compression level. Default: 3
#' @param num_threads number of compression threads. Default 1
#' 
#' @return ZSTD context
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
init_zstd_cctx <- function(level = 3L, num_threads = 1L) {
  .Call(init_cctx_, level, num_threads)
}