
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Get the Dictionary ID of a dictionary or compressed data
#' 
#' @param dict raw vector or filename
#' @return Signed integer value representing the Dictionary ID. If data does not 
#'         represent a dictionary, or data which was compressed with a dictionary,
#'         then a value of 0 is returned.
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
zstd_dict_id <- function(dict) {
  .Call(zstd_dict_id_, dict)
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Train a dictionary
#' 
#' @param samples list of raw vectors.  Each raw vector should be a complete
#'        example of something to be compressed with \code{zstd_compress()}
#' @param size Maximum size of dictionary in bytes. Default: 112640 (110 kB) 
#'        matches the default size set by the command line version of \code{zstd}.
#'        Actual dictionary size may be smaller than this if (1) there was not
#'        enough training data to make use of this size (2) \code{optim_shrink_allow}
#'        was set and a smaller dictionary was found to be almost as 
#'        useful.
#' @param optim optimize the dictionary. Default FALSE.  If TRUE, then ZSTD
#'        will spend time optimizing the dictionary.
#' @param optim_shrink_allow integer value representing a percentage.
#'        If non-zero, then a search will be carried out for dictionaries of a 
#'        smaller size which are up to optim_shrink_allow percent worse than
#'        the maximum sized dictionary.  Default: 0 means that no 
#'        shrinking will be done.
#' @return raw vector containing a ZSTD dictionary
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
zstd_train_dict_compress <- function(samples, size = 100000, optim = FALSE, optim_shrink_allow = 0) {
  .Call(zstd_train_dictionary_, samples, size, optim, optim_shrink_allow = 0)
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Train a dictionary
#' 
#' @inheritParams zstd_train_dict_compress
#' @param samples list of example R objects to train a dictionary to be 
#'        used with \code{zstd_serialize}
#' 
#' @return raw vector containing a ZSTD dictionary
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
zstd_train_dict_serialize <- function(samples, size = 100000, optim = FALSE, optim_shrink_allow = 0) {
  
  stopifnot(is.list(samples))
  serialized_samples <- lapply(samples, \(x) serialize(x, NULL)) 
  
  .Call(zstd_train_dictionary_, serialized_samples, size, optim, optim_shrink_allow)
}



if (FALSE) {
  
  countries <- rownames(LifeCycleSavings)
  
  train_samples <- lapply(
    1:1000, 
    \(x) setNames(sample(length(countries)), countries)
  )
  
  test_samples <- lapply(
    1:1000, 
    \(x) setNames(sample(length(countries)), countries)
  )
  
  length(serialize(train_samples[[1]], NULL))
  
  system.time(
    dict <- zstd_train_dict_serialize(train_samples, size = 100000, optim = FALSE)
  )
  
  system.time(
    dict <- zstd_train_dict_serialize(train_samples, size = 100000, optim = TRUE)
  )
  
  system.time(
    dict <- zstd_train_dict_serialize(train_samples, size = 100000, optim = TRUE, optim_shrink_allow = 75)
  )
  
  zstd_dict_id(dict)
   
  
  cctx      <- zstd_cctx(             level = 3)
  cctx_dict <- zstd_cctx(dict = dict, level = 3)
  dctx_dict <- zstd_dctx(dict = dict)

  dat <- test_samples[[1]]
  zstd_serialize(dat, cctx = cctx)      |> length()
  zstd_serialize(dat, cctx = cctx_dict) |> length()
  
  identical(
    zstd_serialize(dat, cctx = cctx_dict) |> zstd_unserialize(dctx = dctx_dict),
    dat
  )
  
  
  s1 <- lapply(test_samples, \(x) zstd_serialize(x, cctx = cctx     )) |> lengths() |> sum()
  s2 <- lapply(test_samples, \(x) zstd_serialize(x, cctx = cctx_dict)) |> lengths() |> sum()
  round(s2/s1 * 100, 1)
  
  
  bench::mark(
    lapply(test_samples, \(x) zstd_serialize(x, cctx = cctx     )),
    lapply(test_samples, \(x) zstd_serialize(x, cctx = cctx_dict)),
    check = FALSE
  )[, 1:5]
  
  
}
