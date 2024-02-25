

test_that("roundtrip works for stream compression", {
  dat <- mtcars
  expect_identical(dat, zstd_unserialize_stream(zstd_serialize_stream(dat)))

  dat <- iris
  expect_identical(dat, zstd_unserialize_stream(zstd_serialize_stream(dat)))

  dat <- sample(1e6)
  expect_identical(dat, zstd_unserialize_stream(zstd_serialize_stream(dat)))

  dat <- function(x) {3 + 7 + x}
  expect_equal(dat, zstd_unserialize_stream(zstd_serialize_stream(dat)))
  
  
  dat <- sample(1e6)
  expect_identical(dat, zstd_unserialize_stream(zstd_serialize_stream(dat, num_threads = 2)))
})
