
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
in-memory compression of R objects.

This is mainly an exploratory package figuring out some of R internals,
but it does compress data, and it is pretty fast.

For rock solid general solutions to fast serialization of R objects, see
the [fst](https://github.com/fstpackage/fst) or
[qs](https://cran.r-project.org/package=qs) packages.

[zstd](https://github.com/facebook/zstd) code provided with this package
is v1.4.6.

## What’s in the box

  - `zstd_serialize()` and `zstd_unserialize()` for converting R objects
    to/from a compressed representation

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
lobstr::obj_size(arr)
```

    #> 352 B

``` r
buf <- zstd_serialize(arr)
length(buf) # Number of bytes
```

    #> [1] 120

``` r
# compression ratio
length(buf)/as.numeric(lobstr::obj_size(arr))
```

    #> [1] 0.3409091

``` r
zstd_unserialize(buf)
```

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

## Compressing 1 million Integers

``` r
library(zstdlite)
library(lz4lite)
set.seed(1)

N                 <- 1e6
input_ints        <- sample(1:5, N, prob = (1:5)^4, replace = TRUE)
uncompressed      <- serialize(input_ints, NULL, xdr = FALSE)
compressed_lo     <- zstd_serialize(input_ints, level = -5)
compressed_mid    <- zstd_serialize(input_ints, level =  3)
compressed_mid2   <- zstd_serialize(input_ints, level = 10)
compressed_hi     <- zstd_serialize(input_ints, level = 22)
compressed_base   <- memCompress(serialize(input_ints, NULL, xdr = FALSE))
```

<details>

<summary> Click here to show/hide benchmark code </summary>

``` r
library(zstdlite)

res <- bench::mark(
  serialize(input_ints, NULL, xdr = FALSE),
  memCompress(serialize(input_ints, NULL, xdr = FALSE)),
  zstd_serialize(input_ints, level =  -5),
  zstd_serialize(input_ints, level =   3),
  zstd_serialize(input_ints, level =  10),
  zstd_serialize(input_ints, level =  22),
  check = FALSE
)
```

</details>

| expression                                             |   median | itr/sec |   MB/s | compression\_ratio |
| :----------------------------------------------------- | -------: | ------: | -----: | -----------------: |
| serialize(input\_ints, NULL, xdr = FALSE)              |   3.02ms |     256 | 1261.1 |              1.000 |
| memCompress(serialize(input\_ints, NULL, xdr = FALSE)) | 184.02ms |       5 |   20.7 |              0.079 |
| zstd\_serialize(input\_ints, level = -5)               |  14.63ms |      68 |  260.8 |              0.122 |
| zstd\_serialize(input\_ints, level = 3)                |  14.53ms |      69 |  262.6 |              0.101 |
| zstd\_serialize(input\_ints, level = 10)               |  90.86ms |      11 |   42.0 |              0.083 |
| zstd\_serialize(input\_ints, level = 22)               |     2.4s |       0 |    1.6 |              0.058 |

### Decompressing 1 million integers

<details>

<summary> Click here to show/hide benchmark code </summary>

``` r
res <- bench::mark(
  unserialize(uncompressed),
  zstd_unserialize(compressed_lo),
  zstd_unserialize(compressed_mid2),
  zstd_unserialize(compressed_hi),
  unserialize(memDecompress(compressed_base, type = 'gzip')),
  check = TRUE
)
```

</details>

| expression                                                  |   median | itr/sec |   MB/s |
| :---------------------------------------------------------- | -------: | ------: | -----: |
| unserialize(uncompressed)                                   | 456.96µs |    1801 | 8347.9 |
| zstd\_unserialize(compressed\_lo)                           |   9.91ms |      96 |  385.0 |
| zstd\_unserialize(compressed\_mid2)                         |   7.28ms |     139 |  523.9 |
| zstd\_unserialize(compressed\_hi)                           |   4.57ms |     189 |  834.7 |
| unserialize(memDecompress(compressed\_base, type = “gzip”)) |  14.96ms |      70 |  255.0 |

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
