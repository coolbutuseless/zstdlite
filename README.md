
<!-- README.md is generated from README.Rmd. Please edit that file -->

# zstdlite

<!-- badges: start -->

![](https://img.shields.io/badge/cool-useless-green.svg) [![R build
status](https://github.com/coolbutuseless/zstdlite/workflows/R-CMD-check/badge.svg)](https://github.com/coolbutuseless/zstdlite/actions)
[![Lifecycle:
experimental](https://img.shields.io/badge/lifecycle-experimental-orange.svg)](https://www.tidyverse.org/lifecycle/#experimental)
<!-- badges: end -->

`zstdlite` provides access to the very fast (and highly configurable)
[zstd](https://github.com/facebook/zstd) library for performing
in-memory compression of numeric vectors and arrays.

The scope of this package is limited - it aims to provide functions for
direct compression of vectors and arrays which contain raw, integer,
real, complex or logical values. It does this by operating on the data
payload within the vectors, and gains significant speed by not
serializing the R object itself. If you wanted to compress arbitrary R
objects, you must first manually convert into a raw vector
representation using `base::serialize()`.

As of v0.1.3, the dimensions of the input data is stored, and matrices
and vectors are restored to their original shape upon decompression.

Currently zstd code provided with this package is v1.4.6.

### Design Choices

`zstdlite` will compress the *data payload* within a numeric-ish vector,
and not the R object itself. Along with this payload, a minimum of meta
information is kept to store data type and array dimensions.

#### Limitations

  - Any attribute information (other than array dimensions) is lost.
  - Values must be of type: raw, integer, real, complex or logical.

## Installation

You can install from
[GitHub](https://github.com/coolbutuseless/zstdlite) with:

``` r
# install.package('remotes')
remotes::install_github('coolbutuseless/zstdlite)
```

## Basic usage of zstdlite

``` r
arr <- array(1:27, c(3, 3, 3))
pryr::object_size(arr)
#> 352 B


buf <- zstd_compress(arr)
length(buf) # Number of bytes
#> [1] 69

zstd_decompress(buf)
#> , , 1
#> 
#>      [,1] [,2] [,3]
#> [1,]    1    4    7
#> [2,]    2    5    8
#> [3,]    3    6    9
#> 
#> , , 2
#> 
#>      [,1] [,2] [,3]
#> [1,]   10   13   16
#> [2,]   11   14   17
#> [3,]   12   15   18
#> 
#> , , 3
#> 
#>      [,1] [,2] [,3]
#> [1,]   19   22   25
#> [2,]   20   23   26
#> [3,]   21   24   27
```

## Compressing 1 million Integers

`zstdlite` supports the direct compression of raw, integer, real,
complex and logical vectors.

These vectors do not need to be serialized first to a raw
representation, instead the data-payload within these vectors is
compressed.

``` r
library(zstdlite)
library(lz4lite)

N                 <- 1e6
input_ints        <- sample(1:5, N, prob = (1:5)^2, replace = TRUE)
compressed_lo     <- zstd_compress(input_ints)
compressed_hi     <- zstd_compress(input_ints, level = 100)
compressed_lo_lz4 <- lz4_compress(input_ints, acc = 1)
compressed_hi_lz4 <- lz4_compress(input_ints, use_hc = TRUE, hc_level = 12)
```

<details>

<summary> Click here to show/hide benchmark code </summary>

``` r
library(zstdlite)

res <- bench::mark(
  zstd_compress(input_ints, level =   1),
  zstd_compress(input_ints, level =   3),
  zstd_compress(input_ints, level =  10),
  zstd_compress(input_ints, level =  22),
  lz4_compress (input_ints, acc = 1),
  lz4_compress (input_ints, use_hc = TRUE, hc_level = 12),
  check = FALSE
)
```

</details>

| package  | expression                                                 |  median | itr/sec |  MB/s | compression\_ratio |
| :------- | :--------------------------------------------------------- | ------: | ------: | ----: | -----------------: |
| zstdlite | zstd\_compress(input\_ints, level = 1)                     | 14.78ms |      67 | 258.1 |              0.131 |
| zstdlite | zstd\_compress(input\_ints, level = 3)                     | 14.73ms |      67 | 259.0 |              0.131 |
| zstdlite | zstd\_compress(input\_ints, level = 10)                    | 94.08ms |      11 |  40.5 |              0.106 |
| zstdlite | zstd\_compress(input\_ints, level = 22)                    |   2.36s |       0 |   1.6 |              0.076 |
| lz4lite  | lz4\_compress(input\_ints, acc = 1)                        |  6.38ms |     155 | 597.8 |              0.306 |
| lz4lite  | lz4\_compress(input\_ints, use\_hc = TRUE, hc\_level = 12) |  11.21s |       0 |   0.3 |              0.122 |

### Decompressing 1 million integers

<details>

<summary> Click here to show/hide benchmark code </summary>

``` r
res <- bench::mark(
  zstd_decompress(compressed_lo),
  zstd_decompress(compressed_hi),
  lz4_decompress(compressed_lo_lz4),
  lz4_decompress(compressed_hi_lz4),
  check = FALSE
)
```

</details>

| package  | expression                           | median | itr/sec |   MB/s |
| :------- | :----------------------------------- | -----: | ------: | -----: |
| zstdlite | zstd\_decompress(compressed\_lo)     | 8.33ms |     116 |  458.1 |
| zstdlite | zstd\_decompress(compressed\_hi)     | 2.52ms |     344 | 1511.3 |
| lz4lite  | lz4\_decompress(compressed\_lo\_lz4) | 1.68ms |     527 | 2275.4 |
| lz4lite  | lz4\_decompress(compressed\_hi\_lz4) | 1.22ms |     779 | 3135.4 |

## Technical bits

### Why only vectors of raw, integer, real, complex or logical?

R objects can be considered to consist of:

  - a header - giving information like length and information for the
    garbage collector
  - a body - data of some kind.

The vectors supported by `zstdlite` are those vectors whose body
consists of data that is directly interpretable as a contiguous sequence
of bytes representing numerical values.

Other R objects (like lists or character vectors) are really collections
of pointers to other objects, and do not live in memory as a contiguous
sequence of byte data.

### How it works.

##### Compression

1.  Given a pointer to a standard numeric vector from R (i.e. an *SEXP*
    pointer).
2.  Ignore any attributes (except for dimensions) and compress the data
    payload within the object.
3.  Prefix the compressed data with
      - a 4 byte header giving the SEXP type
      - the dimensions of the data
4.  Return a raw vector to the user containing the compressed bytes.

##### Decompression

1.  Strip off the 4-bytes of header information and the dimension
    information
2.  Feed the other bytes in to the ZSTD decompression function written
    in C
3.  Use the header to decide what sort of R object this is.
4.  Decompress the data into an R object of the correct type, and set
    the dimensions
5.  Return the R object to the user.

### Framing of the compressed data

  - `zstdlite` prefixes the standard Zstandard frame with some extra
    bytes.
  - The compressed representation is the compressed data prefixed with a
    custom 8 byte header consisting of
      - ‘ZST’
      - 1-byte for SEXP type i.e. INTSXP, RAWSXP, REALSXP or LGLSXP
  - This data representation
      - is compatible with the standard Zstandard frame format if the
        leading bytes are removed.
      - is likely to evolve (so currently do not plan on compressing
        something in one version of `zstdlite` and decompressing in
        another version.)

### Zstd “Single File” Libary

  - To simplify the code within this package, it uses the ‘single file
    library’ version of zstd
  - To update this package when zstd is updated, create the single file
    library version
    1.  cd zstd/contrib/single\_file\_libs
    2.  sh create\_single\_file\_library.sh
    3.  Wait…..
    4.  copy zstd/contrib/single\_file\_libs/zstd.c into zstdlite/src
    5.  copy zstd/lib/zstd.h into zstdlite/src

## Related Software

For a more general solution to fast serialization of R objects, see the
[fst](https://github.com/fstpackage/fst) or
[qs](https://cran.r-project.org/package=qs) packages.

  - [lz4](https://github.com/lz4/lz4) and
    [zstd](https://github.com/facebook/zstd) - both by Yann Collet
  - [fst](https://github.com/fstpackage/fst) for serialisation of
    data.frames using lz4 and zstd
  - [qs](https://cran.r-project.org/package=qs) for fast serialization
    of arbitrary R objects with lz4 and zstd

## Acknowledgements

  - Yann Collett for releasing, maintaining and advancing
    [lz4](https://github.com/lz4/lz4) and
    [zstd](https://github.com/facebook/zstd)
  - R Core for developing and maintaining such a wonderful language.
  - CRAN maintainers, for patiently shepherding packages onto CRAN and
    maintaining the repository
