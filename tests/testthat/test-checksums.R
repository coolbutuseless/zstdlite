
test_that("checksums work", {
  
  dat <- 1:10
  v1 <- zstd_serialize(dat)
  
  cctx <- zstd_cctx(include_checksum = TRUE)
  v2 <- zstd_serialize(dat, cctx = cctx)
  
  expect_equal(
    length(v1) + 4,
    length(v2)
  )
  
  dec <- zstd_unserialize(v2)
  expect_identical(
    dec,
    dat
  )    

  # Manually hack the checkshum
  v2[length(v2)] <- as.raw(0)

  expect_error(
    zstd_unserialize(v2),
    "doesn't match checksum"
  )

  dctx <- zstd_dctx(validate_checksum = FALSE)
  expect_no_error(
    zstd_unserialize(v2, dctx = dctx)
  )
  
      
})
