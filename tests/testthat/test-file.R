

tmp <- tempfile()
zstd_serialize(mtcars, dst = tmp)

test_that("stream to/from file works", {
  
  res <- zstd_unserialize(tmp)
  expect_identical(
    mtcars,
    res
  )
  
})
