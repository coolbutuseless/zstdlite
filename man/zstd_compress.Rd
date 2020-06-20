% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/zstd.R
\name{zstd_compress}
\alias{zstd_compress}
\title{Compress numeric vector data with zstd}
\usage{
zstd_compress(src, level = 3)
}
\arguments{
\item{src}{Vector, matrix or array of raw, integer, real or logical values.}

\item{level}{Compression level. Default 3 (to match the command line
interface to zstd).  Higher values run slower but with
more compression. Standard compression values are in range [-5, 22].
Other values are possible, but have little to no effect.}
}
\description{
Vector data must be raw, integer, numeric or logical.
}
\details{
The R object meta-data is not compressed, so attributes and dimensions of
the matrix/vector argument will be lost.

To compress general R objects, they can first be converted to a raw byte
representation using \code{base::serialize()} e.g.
\code{zstdlite::zstd_compress(base::serialize(mtcars))}
}