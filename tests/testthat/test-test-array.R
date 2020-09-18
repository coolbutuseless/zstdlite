

test_that("arrays recreated identically", {

  set.seed(1)

  for (i in 1:200) {
    N <- sample(2:7, 1)
    dims <- sample(1:10, N)
    n <- prod(dims)

    x <- runif(n, 1, 100000)
    arr <- array(x, dim = dims)

    cmp <- zstd_compress(arr)
    dec <- NULL
    dec <- zstd_decompress(cmp)

    expect_identical(dim(arr), dim(dec))
    expect_identical(arr, dec)
  }



  for (i in 1:200) {
    N <- sample(2:7, 1)
    dims <- sample(1:10, N)
    n <- prod(dims)

    x <- as.integer(runif(n, 1, 100000))
    arr <- array(x, dim = dims)

    cmp <- zstd_compress(arr)
    dec <- NULL
    dec <- zstd_decompress(cmp)

    expect_identical(dim(arr), dim(dec))
    expect_identical(arr, dec)
  }


  for (i in 1:200) {
    N <- sample(2:7, 1)
    dims <- sample(1:10, N)
    n <- prod(dims)

    x <- as.raw(runif(n, 1, 255))
    arr <- array(x, dim = dims)

    cmp <- zstd_compress(arr)
    dec <- NULL
    dec <- zstd_decompress(cmp)

    expect_identical(dim(arr), dim(dec))
    expect_identical(arr, dec)
  }






})
