

test_that("roundtrip works", {
  dat <- mtcars
  expect_identical(dat, zstd_unserialize(zstd_serialize(dat)))

  dat <- iris
  expect_identical(dat, zstd_unserialize(zstd_serialize(dat)))

  dat <- sample(1e6)
  expect_identical(dat, zstd_unserialize(zstd_serialize(dat)))

  dat <- function(x) {3 + 7 + x}
  expect_equal(dat, zstd_unserialize(zstd_serialize(dat)))
  
  # Multithreading
  dat <- sample(1e6)
  expect_identical(dat, zstd_unserialize(zstd_serialize(dat, num_threads = 2)))
})


test_that("roundtrip to file works", {
  file <- tempfile()
  dat <- mtcars
  zstd_serialize(dat, file = file)
  expect_identical(dat, zstd_unserialize(file))
  
  dat <- iris
  zstd_serialize(dat, file = file)
  expect_identical(dat, zstd_unserialize(file))
  
  dat <- sample(1e6)
  zstd_serialize(dat, file = file)
  expect_identical(dat, zstd_unserialize(file))
  
  dat <- function(x) {3 + 7 + x}
  zstd_serialize(dat, file = file)
  expect_equal(dat, zstd_unserialize(file))
  
  # Multithreading
  dat <- sample(1e6)
  zstd_serialize(dat, file = file, num_threads = 2)
  expect_identical(dat, zstd_unserialize(file))
})
