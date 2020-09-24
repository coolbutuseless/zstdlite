
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
#> 352 B


buf <- zstd_serialize(arr)
length(buf) # Number of bytes
#> [1] 120

# compression ratio
length(buf)/as.numeric(lobstr::obj_size(arr))
#> [1] 0.3409091

zstd_unserialize(buf)
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
set.seed(1)

N                 <- 1e6
input_ints        <- sample(1:5, N, prob = (1:5)^4, replace = TRUE)
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
  zstd_serialize(input_ints, level =  -5),
  zstd_serialize(input_ints, level =   3),
  zstd_serialize(input_ints, level =  10),
  zstd_serialize(input_ints, level =  22),
  memCompress(serialize(input_ints, NULL, xdr = FALSE)),
  check = FALSE
)
```

</details>

| expression                                             |   median | itr/sec |  MB/s | compression\_ratio |
| :----------------------------------------------------- | -------: | ------: | ----: | -----------------: |
| zstd\_serialize(input\_ints, level = -5)               |   14.5ms |      68 | 263.1 |              0.122 |
| zstd\_serialize(input\_ints, level = 3)                |  14.22ms |      70 | 268.2 |              0.101 |
| zstd\_serialize(input\_ints, level = 10)               |  90.37ms |      11 |  42.2 |              0.083 |
| zstd\_serialize(input\_ints, level = 22)               |    2.45s |       0 |   1.6 |              0.058 |
| memCompress(serialize(input\_ints, NULL, xdr = FALSE)) | 178.95ms |       6 |  21.3 |              0.079 |

### Decompressing 1 million integers

<details>

<summary> Click here to show/hide benchmark code </summary>

``` r
res <- bench::mark(
  zstd_unserialize(compressed_lo),
  zstd_unserialize(compressed_mid2),
  zstd_unserialize(compressed_hi),
  unserialize(memDecompress(compressed_base, type = 'gzip')),
  check = TRUE
)
```

</details>

| expression                                                  |  median | itr/sec |  MB/s |
| :---------------------------------------------------------- | ------: | ------: | ----: |
| zstd\_unserialize(compressed\_lo)                           |  9.41ms |     104 | 405.4 |
| zstd\_unserialize(compressed\_mid2)                         |  7.05ms |     140 | 541.1 |
| zstd\_unserialize(compressed\_hi)                           |   4.2ms |     217 | 909.3 |
| unserialize(memDecompress(compressed\_base, type = “gzip”)) | 14.24ms |      70 | 267.9 |

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
