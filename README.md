
<!-- README.md is generated from README.Rmd. Please edit that file -->

# zstdlite

<!-- badges: start -->

![](https://img.shields.io/badge/cool-useless-green.svg)
[![R-CMD-check](https://github.com/coolbutuseless/zstdlite/actions/workflows/R-CMD-check.yaml/badge.svg)](https://github.com/coolbutuseless/zstdlite/actions/workflows/R-CMD-check.yaml)
<!-- badges: end -->

`zstdlite` provides access to the very fast (and highly configurable)
[zstd](https://github.com/facebook/zstd) library for serialization of R
objects and compression/decompression of raw byte buffers.

[zstd](https://github.com/facebook/zstd) code provided with this package
is v1.5.5, and is included under its BSD license (compatible with the
MIT license for this package).

## What’s in the box

- `zstd_serialize()` and `zstd_unserialize()`
  - converting arbitrary R objects to/from a compressed representation
  - Options:
    - `level` compression level in range \[-5, 22\]. Default: 3
    - `num_threads` number of threads to use. Default: 1
    - `cctx`/`dctx` explicitly create compression and decompression
      contexts to control zstd
- `zstd_compress()` and `zstd_decompress()` are for
  compressing/decompressing raw vectors - usually for interfacing with
  other systems
- `init_zstd_cctx()` and `init_zstd_dctx()` initialize compression and
  decompression contexts, respectively. This allows for explicit setting
  of dictionaries to increase compression efficiency.
- `zstd_train_dict_compress()` and `zstd_train_dict_serialize()` for
  creating dictionaries for compression and serialization respectively

# ToDo before release

- `zstd_compress()`/`zstd_decompress()` to file should use a streaming
  interface
  - Might not be worth it for first release
- Create compelling example for dictionary use.

## Installation

You can install from
[GitHub](https://github.com/coolbutuseless/zstdlite) with:

``` r
# install.package('remotes')
remotes::install_github('coolbutuseless/zstdlite')
```

## Basic Usage of `zstd_serialize()` and `zstd_unserialize()`

`zstd_serialize()` and `zstd_unserialize()` are direct analogues of base
R’s `serialize()` and `unserialize()` if those base R functions
supported `zstd` compression.

Because `zstd_serialize()` and `zstd_unserialize()` use R’s
serialization mechanism, they will save/load (almost) any R object
(Note: currently reference objects, which should be handled with a
`refhook` argument are not handled).

``` r
lobstr::obj_size(mtcars)
```

    #> 7.21 kB

``` r
buf <- zstd_serialize(mtcars, level = 10, num_threads = 4)
length(buf) # Number of compressed bytes
```

    #> [1] 1255

``` r
# compression ratio
length(buf)/as.numeric(lobstr::obj_size(mtcars))
```

    #> [1] 0.1741121

``` r
zstd_unserialize(buf) |> head()
```

    #>                    mpg cyl disp  hp drat    wt  qsec vs am gear carb
    #> Mazda RX4         21.0   6  160 110 3.90 2.620 16.46  0  1    4    4
    #> Mazda RX4 Wag     21.0   6  160 110 3.90 2.875 17.02  0  1    4    4
    #> Datsun 710        22.8   4  108  93 3.85 2.320 18.61  1  1    4    1
    #> Hornet 4 Drive    21.4   6  258 110 3.08 3.215 19.44  1  0    3    1
    #> Hornet Sportabout 18.7   8  360 175 3.15 3.440 17.02  0  0    3    2
    #> Valiant           18.1   6  225 105 2.76 3.460 20.22  1  0    3    1

## Simple Benchmark

``` r
dat <- iris[sample(nrow(iris), 100000, TRUE),]
lobstr::obj_size(dat)
```

    #> 10.00 MB

``` r
file <- tempfile()
res <- bench::mark(
  zstd_serialize(dat, file = file),
  zstd_serialize(dat, file = file, level = -5, num_threads = 3),
  saveRDS(dat, file = file)
)

res[,1:5]
```

    #> # A tibble: 3 × 5
    #>   expression                                    min   median `itr/sec` mem_alloc
    #>   <bch:expr>                               <bch:tm> <bch:tm>     <dbl> <bch:byt>
    #> 1 zstd_serialize(dat, file = file)          14.69ms  14.89ms     66.9    17.27KB
    #> 2 zstd_serialize(dat, file = file, level …   7.09ms   7.23ms    138.     17.27KB
    #> 3 saveRDS(dat, file = file)                163.49ms 163.81ms      6.10    8.63KB

<img src="man/figures/README-unnamed-chunk-4-1.png" width="100%" />

## Basic example `zstd_decompress()`

`zstd_serialize()`/`zstd_unserialize()` compress R objects that are
really only useful when working in R, or sharing data with other R
users.

In contrast, `zstd_compress()`/`zstd_decompress()` operate only on raw
vectors and strings, and are more suitable for handling compressed data
which is compatible with other systems and languages

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

    #> 32.15 kB

``` r
compressed <- zstd_compress(manifesto)
lobstr::obj_size(compressed)
```

    #> 9.96 kB

``` r
identical(
  zstd_decompress(compressed, type = 'string'),
  manifesto
)
```

    #> [1] TRUE

## Dictionary-based compression

The following notes are from `zstd` dictionary documentation:

#### Why should I use a dictionary?

Zstd can use dictionaries to improve compression ratio of small data.
Traditionally small files don’t compress well because there is very
little repetition in a single sample, since it is small. But, if you are
compressing many similar files, like a bunch of JSON records that share
the same structure, you can train a dictionary on ahead of time on some
samples of these files. Then, zstd can use the dictionary to find
repetitions that are present across samples. This can vastly improve
compression ratio.

#### When is a dictionary useful?

Dictionaries are useful when compressing many small files that are
similar. The larger a file is, the less benefit a dictionary will have.
Generally, we don’t expect dictionary compression to be effective past
100KB. And the smaller a file is, the more we would expect the
dictionary to help.

#### How do I train a dictionary?

Gather samples from your use case. These samples should be similar to
each other. If you have several use cases, you could try to train one
dictionary per use case. If the dictionary training function fails, that
is likely because you either passed too few samples, or a dictionary
would not be effective for your data.

#### How large should my dictionary be?

A reasonable dictionary size, the `dictBufferCapacity`, is about 100KB.
The zstd CLI defaults to a 110KB dictionary. You likely don’t need a
dictionary larger than that. But, most use cases can get away with a
smaller dictionary. The advanced dictionary builders can automatically
shrink the dictionary for you, and select the smallest size that doesn’t
hurt compression ratio too much. See the `shrinkDict` parameter. A
smaller dictionary can save memory, and potentially speed up
compression.

#### How many samples should I provide to the dictionary builder?

We generally recommend passing ~100x the size of the dictionary in
samples. A few thousand should suffice. Having too few samples can hurt
the dictionaries effectiveness. Having more samples will only improve
the dictionaries effectiveness. But having too many samples can slow
down the dictionary builder.

#### How do I determine if a dictionary will be effective?

Simply train a dictionary and try it out.

#### When should I retrain a dictionary?

You should retrain a dictionary when its effectiveness drops. Dictionary
effectiveness drops as the data you are compressing changes. Generally,
we do expect dictionaries to “decay” over time, as your data changes,
but the rate at which they decay depends on your use case. Internally,
we regularly retrain dictionaries, and if the new dictionary performs
significantly better than the old dictionary, we will ship the new
dictionary.

## Example

The following shows that using a dictionary for this specific example
gives ~35% smaller files in ~75% of the time.

``` r
set.seed(2024)
countries <- rownames(LifeCycleSavings)

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Create 'test' and 'train' datasets
# In this example consider the case of having a named vector of rankings of 
# countries.  Each ranking will be compressed separately and stored (say in a database)
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
train_samples <- lapply(
  1:1000, 
  \(x) setNames(sample(length(countries)), countries)
)

test_samples <- lapply(
  1:1000, 
  \(x) setNames(sample(length(countries)), countries)
)

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Create a dictionary
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
dict <- zstd_train_dict_serialize(train_samples, size = 5000, optim = FALSE)

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Setup Compression/Decompression contexts to use this dictionary
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
cctx_nodict <- init_zstd_cctx()             # No dictionary. For comparison
cctx_dict   <- init_zstd_cctx(dict = dict)

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# When using the dictionary, what is the size of the compressed data compared
# to not using a dicionary here?
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
s1 <- lapply(test_samples, \(x) zstd_serialize(x, cctx = cctx_nodict)) |> lengths() |> sum()
s2 <- lapply(test_samples, \(x) zstd_serialize(x, cctx = cctx_dict  )) |> lengths() |> sum()
cat(round(s2/s1 * 100, 1), "%")
```

    #> 63.1 %

``` r
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Simple benchmark to test speed when using dicionary.
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bench::mark(
  "No Dict" = lapply(test_samples, \(x) zstd_serialize(x, cctx = cctx_nodict)),
  "Dict"    = lapply(test_samples, \(x) zstd_serialize(x, cctx = cctx_dict  )),
  check = FALSE
)[, 1:5]
```

    #> # A tibble: 2 × 5
    #>   expression      min   median `itr/sec` mem_alloc
    #>   <bch:expr> <bch:tm> <bch:tm>     <dbl> <bch:byt>
    #> 1 No Dict     10.87ms   11.4ms      87.4      18MB
    #> 2 Dict         7.78ms   8.07ms     124.       18MB

### Zstd “Single File” Libary

- To simplify the code within this package, it uses the ‘single file
  library’ version of zstd
- To update this package when zstd is updated, create the single file
  library version
  1.  cd zstd/build/single_file_libs
  2.  sh create_single_file_library.sh
  3.  Wait…..
  4.  copy zstd/built/single_file_libs/zstd.c into zstdlite/src
  5.  copy zstd/lib/zstd.h into zstdlite/src

## Related Software

For a more general solution to fast serialization of R objects, see the
[fst](https://github.com/fstpackage/fst) or
[qs](https://cran.r-project.org/package=qs) packages.

- [fst](https://github.com/fstpackage/fst) for serialisation of
  data.frames only using lz4 and zstd
- [qs](https://cran.r-project.org/package=qs) for fast serialization of
  arbitrary R objects with lz4 and zstd

## Acknowledgements

- Yann Collett for creating [lz4](https://github.com/lz4/lz4) and
  [zstd](https://github.com/facebook/zstd)
- R Core for developing and maintaining such a wonderful language.
- CRAN maintainers, for patiently shepherding packages onto CRAN and
  maintaining the repository
