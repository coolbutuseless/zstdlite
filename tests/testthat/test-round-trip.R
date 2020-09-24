

test_that("roundtrip works", {
  dat <- mtcars
  expect_identical(dat, zstd_unserialize(zstd_serialize(dat)))

  dat <- iris
  expect_identical(dat, zstd_unserialize(zstd_serialize(dat)))

  dat <- sample(1e6)
  expect_identical(dat, zstd_unserialize(zstd_serialize(dat)))

  dat <- function(x) {3 + 7 + x}
  expect_equal(dat, zstd_unserialize(zstd_serialize(dat)))
})
