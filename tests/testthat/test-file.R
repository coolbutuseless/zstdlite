

tmp <- tempfile()
zstd_serialize_stream_file(mtcars, file = tmp)

test_that("stream to/from file works", {
  
  res <- zstd_unserialize(tmp)
  expect_identical(
    mtcars,
    res
  )
  
})
