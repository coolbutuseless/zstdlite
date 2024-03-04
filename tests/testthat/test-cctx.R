



test_that("cctx with zstd_serialize works", {
  
  cctx <- zstd_cctx()
  expect_identical(
    zstd_serialize(mtcars),
    zstd_serialize(mtcars, cctx = cctx)
  )
  
  # Re-using a cctx
  expect_identical(
    zstd_serialize(mtcars),
    zstd_serialize(mtcars, cctx = cctx)
  )
  
})



test_that("cctx with zstd_compress works", {
  
  cctx <- zstd_cctx()
  expect_identical(
    zstd_compress(serialize(mtcars, NULL)),
    zstd_compress(serialize(mtcars, NULL) , cctx = cctx)
  )
  
  # Re-using a cctx
  expect_identical(
    zstd_compress(serialize(mtcars, NULL)),
    zstd_compress(serialize(mtcars, NULL) , cctx = cctx)
  )
  
})
