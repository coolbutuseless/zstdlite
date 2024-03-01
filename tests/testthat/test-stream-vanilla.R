

test_that("vanilla stream implementation works", {
  
  res_stream <- .Call("zstd_serialize_stream_", mtcars, cctx = NULL, list(level = 3L, num_threads = 1L)) |>
    zstd_unserialize()
  
  expect_identical(res_stream, mtcars)
  
  
  
  res_stream <- zstd_serialize(mtcars)
  res_stream <- .Call("zstd_unserialize_stream_", res_stream, dctx = NULL, list())
  
  expect_identical(res_stream, mtcars)
  
})
