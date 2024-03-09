
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Get the Dictionary ID of a dictionary or a vector compressed data.
#' 
#' Dictionary IDs are generated automatically when a dictionary is created.
#' When using a dictionary for compression, the same dictionary must be used
#' during decompression.  ZSTD internally does this check for matching IDs
#' when attempting to decompress.  This function exposes the dictionary ID
#' to aid in handling and tracking dictionaries in R.
#' 
#' @param dict raw vector or filename.  This object could contain either a 
#'        zstd dictionary, or a compressed object.  If it is a compressed object,
#'        then it will return the dictionary id which was used to compress it.
#' @return Signed integer value representing the Dictionary ID. If data does not 
#'         represent a dictionary, or data which was compressed with a dictionary,
#'         then a value of 0 is returned.
#' @export
#' 
#' @examples
#' dict_file <- system.file("sample_dict.raw", package = "zstdlite", mustWork = TRUE)
#' dict <- readBin(dict_file, raw(), file.size(dict_file))
#' zstd_dict_id(dict)
#' compressed_mtcars <- zstd_serialize(mtcars, dict = dict)
#' zstd_dict_id(compressed_mtcars)
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
zstd_dict_id <- function(dict) {
  .Call(zstd_dict_id_, dict)
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Train a dictionary for use with \code{zstd_compress()} and \code{zstd_decompress()}
#' 
#' This function requires multiple samples representative of the expected data to 
#' train a dictionary for use during compression.
#' 
#' @param samples list of raw vectors, or length-1 character vectors.  
#'        Each raw vector or string, should be a complete
#'        example of something to be compressed with \code{zstd_compress()}
#' @param size Maximum size of dictionary in bytes. Default: 112640 (110 kB) 
#'        matches the default size set by the command line version of \code{zstd}.
#'        Actual dictionary created may be smaller than this if (1) there was not
#'        enough training data to make use of this size (2) \code{optim_shrink_allow}
#'        was set and a smaller dictionary was found to be almost as 
#'        useful.
#' @param optim optimize the dictionary. Default FALSE.  If TRUE, then ZSTD
#'        will spend time optimizing the dictionary.  This can be a very 
#'        length operation.
#' @param optim_shrink_allow integer value representing a percentage.
#'        If non-zero, then a search will be carried out for dictionaries of a 
#'        smaller size which are up to \code{optim_shrink_allow} percent worse than
#'        the maximum sized dictionary.  Default: 0 means that no 
#'        shrinking will be done.
#'        
#' @return raw vector containing a ZSTD dictionary
#' @export
#' 
#' @examples
#' # This example shows the mechanics of creating and training a dictionary but
#' # may not be a great example of when a dictionary might be useful
#' cars <- rownames(mtcars)
#' samples <- lapply(seq_len(1000), \(x) serialize(sample(cars), NULL))
#' zstd_train_dict_compress(samples, size = 5000)
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
zstd_train_dict_compress <- function(samples, size = 100000, optim = FALSE, optim_shrink_allow = 0) {
  .Call(zstd_train_dictionary_, samples, size, optim, optim_shrink_allow = 0)
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Train a dictionary for use with \code{zstd_serialize()} and \code{zstd_unserialize()}
#' 
#' @inheritParams zstd_train_dict_compress
#' @param samples list of example R objects to train a dictionary to be 
#'        used with \code{zstd_serialize()}
#' 
#' @return raw vector containing a ZSTD dictionary
#' @export
#' 
#' @examples
#' # This example shows the mechanics of creating and training a dictionary but
#' # may not be a great example of when a dictionary might be useful
#' cars <- rownames(mtcars)
#' samples <- lapply(seq_len(1000), \(x) sample(cars))
#' zstd_train_dict_serialize(samples, size = 5000)
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
zstd_train_dict_serialize <- function(samples, size = 100000, optim = FALSE, optim_shrink_allow = 0) {
  
  stopifnot(is.list(samples))
  serialized_samples <- lapply(samples, \(x) serialize(x, NULL)) 
  
  .Call(zstd_train_dictionary_, serialized_samples, size, optim, optim_shrink_allow)
}

