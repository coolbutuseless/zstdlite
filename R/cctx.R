

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Initialise a ZSTD compression context
#' 
#' Compression contexts can be re-used, meaning that they don't have to be
#' created each time a compression function is called.  This can make 
#' things faster when performing multiple compression operations.
#' 
#' @param level Compression level. Default: 3.  Valid range is [-5, 22] with 
#'        -5 representing the mode with least compression and 22
#'        representing the mode with most compression.  Note \code{level = 0} 
#'        corresponds to the \emph{default} level and is equivalent to
#'        \code{level = 3}
#' @param num_threads Number of compression threads. Default 1.  Using 
#'        more threads can result in faster compression, but the magnitude 
#'        of this speed-up depends on lots of factors e.g. cpu, drive speed,
#'        type of data compression level etc.
#' @param include_checksum Include a checksum with the compressed data? 
#'        Default: FALSE.  If \code{TRUE} then a 32-bit hash of the original
#'        uncompressed data will be appended to the compressed data and 
#'        checked for validity during decompression.  See matching option 
#'        for decompression in  
#'        \code{zstd_dctx()} argument \code{validate_checksum}.
#' @param dict Dictionary. Default: NULL. Can either be a raw vector or a filename. 
#'        This dictionary can be created with \code{zstd_train_dict_compress()}
#'        , \code{zstd_train_dict_seriazlie()} or any other tool supporting
#'        \code{zstd} dictionary creation.  Note: compressed data created 
#'        with a dictionary \emph{must} be decompressed with the same dictionary.
#' 
#' @return External pointer to a ZSTD Compression Context which can be passed to
#'         \code{zstd_serialize()} and \code{zstd_compress()}
#' @export
#' 
#' @examples
#' cctx <- zstd_cctx(level = 4)
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
zstd_cctx <- function(level = 3L, num_threads = 1L, include_checksum = FALSE, dict = NULL) {
  .Call(
    init_cctx_, 
    list(
      level            = level, 
      num_threads      =  num_threads, 
      include_checksum = include_checksum, 
      dict             = dict
    )
  )
}

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Initialise a ZSTD decompression context
#' 
#' Decompression contexts can be re-used, meaning that they don't have to be
#' created each time a decompression function is called.  This can make 
#' things faster when performing multiple decompression operations.
#' 
#' @inheritParams zstd_cctx
#' @param validate_checksum If a checksum is present on the comrpessed data, 
#'        should the checksum be validated? 
#'        Default: TRUE.  Set to \code{FALSE} to ignore the checksum, which 
#'        may lead to a minor speed improvement.
#'        If no checksum is present in the compressed data, then this option has no 
#'        effect.  
#' 
#' @return External pointer to a ZSTD Decompression Context which can be passed to
#'         \code{zstd_unserialize()} and \code{zstd_decompress()}
#' @export
#' 
#' @examples
#' dctx <- zstd_dctx(validate_checksum = FALSE)
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
zstd_dctx <- function(validate_checksum = TRUE, dict = NULL) {
  .Call(
    init_dctx_, 
    list(
      validate_checksum = validate_checksum, 
      dict              = dict
    )
  )
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Get the configuration settings of a compression context 
#' 
#' @param cctx ZSTD compression context, as created by \code{zstd_cctx()}
#' 
#' @return named list of configuration options
#' @export
#' 
#' @examples
#' cctx <- zstd_cctx()
#' zstd_cctx_settings(cctx)
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
zstd_cctx_settings <- function(cctx) {
  .Call(get_cctx_settings_, cctx)
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Get the configuration settings of a decompression context 
#' 
#' @param dctx ZSTD decompression context, as created by \code{zstd_dctx()}
#' 
#' @return named list of configuration options
#' @export
#' 
#' @examples
#' dctx <- zstd_dctx()
#' zstd_dctx_settings(dctx)
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
zstd_dctx_settings <- function(dctx) {
  .Call(get_dctx_settings_, dctx)
}
