
<!-- README.md is generated from README.Rmd. Please edit that file -->

# zstdlite

<!-- badges: start -->

![](https://img.shields.io/badge/cool-useless-green.svg)
[![R-CMD-check](https://github.com/coolbutuseless/zstdlite/actions/workflows/R-CMD-check.yaml/badge.svg)](https://github.com/coolbutuseless/zstdlite/actions/workflows/R-CMD-check.yaml)
<!-- badges: end -->

`zstdlite` provides access to the very fast (and highly configurable)
[zstd](https://github.com/facebook/zstd) library for serialization of R
objects and compression/decompression of raw byte buffers and strings.

[zstd](https://github.com/facebook/zstd) code provided with this package
is v1.5.6, and is included under its BSD license (compatible with the
MIT license for this package).

## What’s in the box

- `zstdfile()`
  - A connection object (like `gzfile()` or `url()`) which supports
    Zstandard compressed data.
  - Supports read/write of both text and binary data. e.g. `readLines()`
    and `readBin()`
  - Can be used by any R code which supports connections.
- `zstd_serialize()` and `zstd_unserialize()`
  - convert arbitrary R objects to/from a compressed representation
  - this is equivalent to base R’s `serialize()`/`unserialize()` with
    the addition of `zstd` compression on the serialized data
- `zstd_compress()` and `zstd_decompress()` are for
  compressing/decompressing strings and raw vectors - usually for
  interfacing with other systems e.g. data was already compressed on the
  command line.
- `zstd_info()` returns a named list of information about a compressed
  data source
- `zstd_cctx()` and `zstd_dctx()` initialize compression and
  decompression contexts, respectively. Options:
  - `level` compression level in range \[-5, 22\]. Default: 3
  - `num_threads` number of threads to use. Default: 1
  - `include_checksum`/`validate_checksum` Default: FALSE
  - `dict` specify dictionary for aiding compression
- `zstd_train_dict_compress()` and `zstd_train_dict_serialize()` for
  creating dictionaries which can speed up compression/decompression

## Comparison to `saveRDS()`/`readRDS()`

The image below compares `{zstdlite}` with `saveRDS()` for saving
compressed representations of R objects. (See `man/benchmarks.R` for
code)

Things to note in this comparison for the particular data used:

- `zstd` compression can be much faster than the compression options
  offered by `saveRDS()`
- `zstd` decompression speed is very fast and (mostly) independent of
  compression settings
- Compressing with `xz` and `bzip2` can both produce more compressed
  representations but at the expense of slow compression/decompression.

<img src="man/figures/comparison.png" width="100%" />

## Installation

To install from r-universe:

``` r
install.packages('zstdlite', repos = c('https://coolbutuseless.r-universe.dev', 'https://cloud.r-project.org'))
```

To install latest version from
[GitHub](https://github.com/coolbutuseless/zstdlite):

``` r
# install.package('remotes')
remotes::install_github('coolbutuseless/zstdlite')
```

## Basic Usage of `zstd_serialize()` and `zstd_unserialize()`

`zstd_serialize()` and `zstd_unserialize()` are direct analogues of base
R’s `serialize()` and `unserialize()`.

Because `zstd_serialize()` and `zstd_unserialize()` use R’s
serialization mechanism, they will save/load (almost) any R object
e.g. data.frames, lists, environments, etc

``` r
compressed_bytes <- zstd_serialize(head(mtcars))
length(compressed_bytes) 
```

    #> [1] 482

``` r
head(compressed_bytes, 100)
```

    #>   [1] 28 b5 2f fd 60 e8 02 c5 0e 00 86 13 4c 41 30 67 6c 3a 00 f1 70 71 26 7f 4c
    #>  [26] 13 ec 49 07 75 e9 64 d9 57 69 c7 18 a8 12 f8 c6 20 5d 6c 22 00 d5 a2 4b c8
    #>  [51] ac df c3 99 67 c3 c5 5c 31 c3 43 4a 22 40 1c 68 85 23 ac c7 d1 61 7e 4d 83
    #>  [76] c0 82 7e 0a 38 00 42 00 35 00 24 2e c1 48 db 82 a1 b6 24 62 2c e6 c3 b0 a7

``` r
zstd_unserialize(compressed_bytes) 
```

    #>                    mpg cyl disp  hp drat    wt  qsec vs am gear carb
    #> Mazda RX4         21.0   6  160 110 3.90 2.620 16.46  0  1    4    4
    #> Mazda RX4 Wag     21.0   6  160 110 3.90 2.875 17.02  0  1    4    4
    #> Datsun 710        22.8   4  108  93 3.85 2.320 18.61  1  1    4    1
    #> Hornet 4 Drive    21.4   6  258 110 3.08 3.215 19.44  1  0    3    1
    #> Hornet Sportabout 18.7   8  360 175 3.15 3.440 17.02  0  0    3    2
    #> Valiant           18.1   6  225 105 2.76 3.460 20.22  1  0    3    1

## Using a `zstdfile()` connection

Use `zstdfile()` to allow read/write access of compressed data from any
R code or package which supports connections.

``` r
tmp <- tempfile()
dat <- as.raw(1:255)
writeBin(dat, zstdfile(tmp))
readBin(zstdfile(tmp), raw(), 255)
```

    #>   [1] 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f 10 11 12 13 14 15 16 17 18 19
    #>  [26] 1a 1b 1c 1d 1e 1f 20 21 22 23 24 25 26 27 28 29 2a 2b 2c 2d 2e 2f 30 31 32
    #>  [51] 33 34 35 36 37 38 39 3a 3b 3c 3d 3e 3f 40 41 42 43 44 45 46 47 48 49 4a 4b
    #>  [76] 4c 4d 4e 4f 50 51 52 53 54 55 56 57 58 59 5a 5b 5c 5d 5e 5f 60 61 62 63 64
    #> [101] 65 66 67 68 69 6a 6b 6c 6d 6e 6f 70 71 72 73 74 75 76 77 78 79 7a 7b 7c 7d
    #> [126] 7e 7f 80 81 82 83 84 85 86 87 88 89 8a 8b 8c 8d 8e 8f 90 91 92 93 94 95 96
    #> [151] 97 98 99 9a 9b 9c 9d 9e 9f a0 a1 a2 a3 a4 a5 a6 a7 a8 a9 aa ab ac ad ae af
    #> [176] b0 b1 b2 b3 b4 b5 b6 b7 b8 b9 ba bb bc bd be bf c0 c1 c2 c3 c4 c5 c6 c7 c8
    #> [201] c9 ca cb cc cd ce cf d0 d1 d2 d3 d4 d5 d6 d7 d8 d9 da db dc dd de df e0 e1
    #> [226] e2 e3 e4 e5 e6 e7 e8 e9 ea eb ec ed ee ef f0 f1 f2 f3 f4 f5 f6 f7 f8 f9 fa
    #> [251] fb fc fd fe ff

## Using contexts to set compression arguments

The `zstd` algorithm uses *contexts* to control the compression and
decompression. Every time data is compressed/decompressed a context is
created.

*Contexts* can be created ahead-of-time, or created on-the-fly. There
can be a some speed advantages to creating a *context* ahead of time and
reusing it for multiple compression operations.

The following ways of calling `zstd_serialize()` are equivalent:

``` r
zstd_serialize(data1, num_threads = 3, level = 20)
zstd_serialize(data2, num_threads = 3, level = 20)
```

``` r
cctx <- zstd_cctx(num_threads = 3, level = 20)
zstd_serialize(data1, cctx = cctx)
zstd_serialize(data2, cctx = cctx)
```

## Using `zstd_compression()`/`zstd_decompress()` for raw data and strings

`zstd_serialize()`/`zstd_unserialize()` compress R objects that are
really only useful when working in R, or sharing data with other R
users.

In contrast, `zstd_compress()`/`zstd_decompress()` operate on raw
vectors and strings, and these functions are suitable for handling
compressed data which is compatible with other systems and languages

Examples:

- reading compressed JSON files
- writing compressed data for storage in a database that will be
  accessed by different computer languages and operating systems.

#### Reading a compressed JSON file

JSON files are often compressed. In this case, the `data.json` file was
compressed using the `zstd` command-line tool, i.e.

    zstd data.json -o data.json.zst

This compressed file can be read directly into R as uncompressed bytes
(in a raw vector), or as a string

``` r
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Read a compressed JSON file as raw bytes or as a string
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
zstd_decompress("man/figures/data.json.zst")
```

    #>   [1] 7b 0a 20 20 22 6d 70 67 22 3a 20 5b 32 31 5d 2c 0a 20 20 22 63 79 6c 22 3a
    #>  [26] 20 5b 36 5d 2c 0a 20 20 22 64 69 73 70 22 3a 20 5b 31 36 30 5d 2c 0a 20 20
    #>  [51] 22 68 70 22 3a 20 5b 31 31 30 5d 2c 0a 20 20 22 64 72 61 74 22 3a 20 5b 33
    #>  [76] 2e 39 5d 2c 0a 20 20 22 77 74 22 3a 20 5b 32 2e 36 32 5d 2c 0a 20 20 22 71
    #> [101] 73 65 63 22 3a 20 5b 31 36 2e 34 36 5d 2c 0a 20 20 22 76 73 22 3a 20 5b 30
    #> [126] 5d 2c 0a 20 20 22 61 6d 22 3a 20 5b 31 5d 2c 0a 20 20 22 67 65 61 72 22 3a
    #> [151] 20 5b 34 5d 2c 0a 20 20 22 63 61 72 62 22 3a 20 5b 34 5d 0a 7d 0a

``` r
zstd_decompress("man/figures/data.json.zst", type = 'string') |> cat()
```

    #> {
    #>   "mpg": [21],
    #>   "cyl": [6],
    #>   "disp": [160],
    #>   "hp": [110],
    #>   "drat": [3.9],
    #>   "wt": [2.62],
    #>   "qsec": [16.46],
    #>   "vs": [0],
    #>   "am": [1],
    #>   "gear": [4],
    #>   "carb": [4]
    #> }

#### Compressing a string

When transmitting large amounts of text to another system, we may wish
to first compress it.

The following string (`manifesto`) is compressed by `zstd_compress()`
and can be uncompressed by any system which supports the `zstd` library,
or even just using the `zstd` command-line tool.

``` r
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Compress a string directly 
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
manifesto <- paste(lorem::ipsum(paragraphs = 100), collapse = "\n")
lobstr::obj_size(manifesto)
```

    #> 32.83 kB

``` r
compressed <- zstd_compress(manifesto, level = 22)
lobstr::obj_size(compressed)
```

    #> 9.27 kB

``` r
identical(
  zstd_decompress(compressed, type = 'string'),
  manifesto
)
```

    #> [1] TRUE

## Limitations

- Reference objects which need to be serialized with a `refhook`
  approach are not handled.

## Dictionary-based compression

From the `zstd` documentation:

    Zstd can use dictionaries to improve compression ratio of small data.
    Traditionally small files don't compress well because there is very little
    repetition in a single sample, since it is small. But, if you are compressing
    many similar files, like a bunch of JSON records that share the same
    structure, you can train a dictionary on ahead of time on some samples of
    these files. Then, zstd can use the dictionary to find repetitions that are
    present across samples. This can vastly improve compression ratio.

### Dictionary Example

The following shows that using a dictionary for this specific example
doubles the compression ratio!

``` r
set.seed(2024)
countries <- rownames(LifeCycleSavings)

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# In this example consider the case of having a named vector of rankings of 
# countries.  Each ranking will be compressed separately and stored (say in a database)
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
rankings <- lapply(
  1:1000, 
  \(x) setNames(sample(length(countries)), countries)
)

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Create a dictionary
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
dict <- zstd_train_dict_serialize(rankings, size = 1500, optim = TRUE)

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Setup Compression contexts to use this dictionary
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
cctx_nodict <- zstd_cctx(level = 13) # No dictionary. For comparison
cctx_dict   <- zstd_cctx(level = 13, dict = dict)

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# When using the dictionary, what is the size of the compressed data compared
# to not using a dicionary here?
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
s1 <- lapply(rankings, \(x) zstd_serialize(x, cctx = cctx_nodict)) |> lengths() |> sum()
s2 <- lapply(rankings, \(x) zstd_serialize(x, cctx = cctx_dict  )) |> lengths() |> sum()
```

    #> Compression ratio                : 1.9

    #> Compression ratio with dictionary: 4

## Acknowledgements

- Yann Collett for creating [lz4](https://github.com/lz4/lz4) and
  [zstd](https://github.com/facebook/zstd)
- R Core for developing and maintaining such a wonderful language.
- CRAN maintainers, for patiently shepherding packages onto CRAN and
  maintaining the repository
