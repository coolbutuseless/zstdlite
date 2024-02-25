

test_that("vanilla stream implementation works", {
  
  res_stream <- .Call("zstd_serialize_stream_", mtcars, level = 3L, num_threads = 1L, cctx = NULL) |>
    zstd_unserialize()
  
  expect_identical(res_stream, mtcars)
  
  
  
  res_stream <- zstd_serialize(mtcars)
  res_stream <- .Call("zstd_unserialize_stream_", res_stream, dctx = NULL)
  
  expect_identical(res_stream, mtcars)
  
})
