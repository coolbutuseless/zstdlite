

test_that("Raw compress roundtrip works", {
  dat <- serialize(mtcars, NULL)
  expect_identical(dat, zstd_decompress(zstd_compress(dat)))
  
  dat <- iris
  dat <- serialize(dat, NULL)
  expect_identical(dat, zstd_decompress(zstd_compress(dat)))
  
  dat <- sample(1e6)
  dat <- serialize(dat, NULL)
  expect_identical(dat, zstd_decompress(zstd_compress(dat)))
  
  dat <- function(x) {3 + 7 + x}
  dat <- serialize(dat, NULL)
  expect_identical(dat, zstd_decompress(zstd_compress(dat)))
  
  
  # Multithreading
  dat <- sample(1e6)
  dat <- serialize(dat, NULL)
  expect_identical(dat, zstd_decompress(zstd_compress(dat, num_threads = 2)))
  
  
  
  # separate context
  dat <- mtcars
  dat <- serialize(dat, NULL)
  cctx <- init_zstd_cctx()
  dctx <- init_zstd_dctx()
  expect_identical(dat, zstd_compress(dat, cctx = cctx) |> zstd_decompress(dctx = dctx))
})




test_that("Raw compress roundtrip to file works", {
  file <- tempfile()
  
  dat <- serialize(mtcars, NULL)
  zstd_compress(dat, file = file)
  expect_identical(dat, zstd_decompress(file))
  
  dat <- iris
  dat <- serialize(dat, NULL)
  zstd_compress(dat, file = file)
  expect_identical(dat, zstd_decompress(file))
  
  dat <- sample(1e6)
  dat <- serialize(dat, NULL)
  zstd_compress(dat, file = file)
  expect_identical(dat, zstd_decompress(file))
  
  dat <- function(x) {3 + 7 + x}
  dat <- serialize(dat, NULL)
  zstd_compress(dat, file = file)
  expect_identical(dat, zstd_decompress(file))
  
  
  # Multithreading
  dat <- sample(1e6)
  dat <- serialize(dat, NULL)
  zstd_compress(dat, file = file, num_threads = 2)
  expect_identical(dat, zstd_decompress(file))
  
  
  
  # separate context
  dat <- mtcars
  dat <- serialize(dat, NULL)
  cctx <- init_zstd_cctx()
  dctx <- init_zstd_dctx()
  zstd_compress(dat, file = file, cctx = cctx)
  expect_identical(dat, zstd_decompress(file, dctx = dctx))
})