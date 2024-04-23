

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Create a file connection which uses Zstandard compression.
#' 
#' @details
#' This \code{zstdfile()} connection works like R's built-in connections (e.g.
#' \code{gzfile()}, \code{xzfile()}) but using the Zstandard algorithm
#' to compress/decompress the data.
#' 
#' This connection works with both ASCII and binary data, e.g. using 
#' \code{readLines()} and \code{readBin()}.
#' 
#' @param description zstandard filename
#' @param open character string. A description of how to open the connection if 
#'        it is to be opened upon creation e.g. "rb". Default "" (empty string) means
#'        to not open the connection on creation - user must still call \code{open()}.
#'        Note: If an "open" string is provided, the user must still call \code{close()}
#'        otherwise the contents of the file aren't completely flushed until the
#'        connection is garbage collected.
#' @param ... Other named arguments which override the contexts e.g. \code{level = 20}
#' @param cctx,dctx compression/decompression contexts created by 
#'        \code{zstd_cctx()} and \code{zstd_dctx()}. Optional.
#' 
#' @export
#' 
#' @examples
#' # Binary 
#' tmp <- tempfile()
#' dat <- as.raw(1:255)
#' writeBin(dat, zstdfile(tmp, level = 20))
#' readBin(zstdfile(tmp),  raw(), 1000)
#' 
#' # Text
#' tmp <- tempfile()
#' txt <- as.character(mtcars)
#' writeLines(txt, zstdfile(tmp))
#' readLines(zstdfile(tmp))
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
zstdfile <- function(description, open = "", ..., cctx = NULL, dctx = NULL) {
  
  if (is.character(description)) {
    description <- normalizePath(description, mustWork = FALSE)
  }
  
  .Call(
    zstdfile_, 
    description = description,
    open        = open,
    opts        = list(...), 
    cctx        = cctx, dctx = dctx
  )
}


if (FALSE) {
  
  dat <- serialize(mtcars, NULL)
  writeBin(dat, zstdfile("working/test.zst", level = 20))
  file.size("working/test.zst")  
  
  dat <- mtcars[sample(nrow(mtcars), 10000, T),]
  
  bench::mark(  
    zstd_serialize(dat, dst = "working/test1.zst", level = 10),
    {z <- zstdfile("working/test2.zst", "wb", level = 10); serialize(dat, z); close(z)}
  )
  
}