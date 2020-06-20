
<!-- README.md is generated from README.Rmd. Please edit that file -->

# zstdlite

<!-- badges: start -->

![](https://img.shields.io/badge/cool-useless-green.svg)
<!-- badges: end -->

`zstdlite` provides access to the very fast (and highly configurable)
compression in [zstd](https://github.com/facebook/zstd) for performing
in-memory compression.

The scope of this package is limited - it aims to provide functions for
direct hashing of vectors which contain raw, integer, real, complex or
logical values. It does this by operating on the data payload within the
vectors, and gains significant speed by not serializing the R object
itself. If you wanted to compress arbitrary R objects, you must first
manually convert into a raw vector representation using
`base::serialize()`.

For a more general solution to fast serialization of R objects, see the
[fst](https://github.com/fstpackage/fst) or
[qs](https://cran.r-project.org/package=qs) packages.

Currently zstd code provided with this package is v1.4.5.

### Design Choices

`zstdlite` will compress the *data payload* within a numeric-ish vector,
and not the R object itself.

#### Limitations

  - As it is the *data payload* of the vector that is being compressed,
    this does not include any notion of the container for that data i.e
    dimensions or other attributes are not compressed with the data.
  - Values must be of type: raw, integer, real, complex or logical.
  - Decompressed values are always returned as a vector i.e. all
    dimensional information is lost during compression.

## Installation

You can install from
[GitHub](https://github.com/coolbutuseless/zstdlite) with:

``` r
# install.package('remotes')
remotes::install_github('coolbutuseless/zstdlite)
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
  zstd_compress(input_ints, level =  -5),
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
| zstdlite | zstd\_compress(input\_ints, level = -5)                    | 14.97ms |      66 | 254.8 |              0.150 |
| zstdlite | zstd\_compress(input\_ints, level = 1)                     | 15.18ms |      65 | 251.3 |              0.131 |
| zstdlite | zstd\_compress(input\_ints, level = 3)                     | 14.86ms |      67 | 256.6 |              0.131 |
| zstdlite | zstd\_compress(input\_ints, level = 10)                    | 95.12ms |      10 |  40.1 |              0.106 |
| zstdlite | zstd\_compress(input\_ints, level = 22)                    |   2.47s |       0 |   1.5 |              0.076 |
| lz4lite  | lz4\_compress(input\_ints, acc = 1)                        |  6.55ms |     152 | 582.2 |              0.306 |
| lz4lite  | lz4\_compress(input\_ints, use\_hc = TRUE, hc\_level = 12) |  11.36s |       0 |   0.3 |              0.122 |

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
| zstdlite | zstd\_decompress(compressed\_lo)     | 8.59ms |     113 |  444.2 |
| zstdlite | zstd\_decompress(compressed\_hi)     | 2.57ms |     375 | 1486.8 |
| lz4lite  | lz4\_decompress(compressed\_lo\_lz4) | 1.65ms |     576 | 2306.0 |
| lz4lite  | lz4\_decompress(compressed\_hi\_lz4) | 1.24ms |     768 | 3067.4 |

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
2.  Ignore any attributes or dimension information- just compress the
    data payload within the object.
3.  Prefix the compressed data with an 4 byte header giving the SEXP
    type.
4.  Return a raw vector to the user containing the compressed bytes.

##### Decompression

1.  Strip off the 4-bytes of header information.
2.  Feed the other bytes in to the ZSTD decompression function written
    in C
3.  Use the header to decide what sort of R object this is.
4.  Decompress the data into an R object of the correct type.
5.  Return the R object to the user.

**Note:** matrices and arrays may also be passed to `zstd_compress()`,
but since no attributes are retained (e.g. dims), the uncompressed
object returned by `zstd_decompress()` can only be a simple vector.

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

## Related Software

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
