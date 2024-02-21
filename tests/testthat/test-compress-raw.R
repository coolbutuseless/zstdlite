

test_that("roundtrip works", {
  dat <- serialize(mtcars, NULL)
  expect_identical(dat, zstd_decompress(zstd_compress(dat)))
  
  dat <- iris
  dat <- serialize(dat, NULL)
  expect_identical(dat, zstd_unserialize(zstd_serialize(dat)))
  
  dat <- sample(1e6)
  dat <- serialize(dat, NULL)
  expect_identical(dat, zstd_unserialize(zstd_serialize(dat)))
  
  dat <- function(x) {3 + 7 + x}
  dat <- serialize(dat, NULL)
  expect_identical(dat, zstd_unserialize(zstd_serialize(dat)))
})
