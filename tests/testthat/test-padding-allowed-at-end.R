


test_that("works on seritalized data.frames", {

  test_df  <- mtcars[1:5, ]
  test_ser <- base::serialize(test_df, NULL)
  test_zst <- zstd_compress(test_ser)

  test_unc <- zstd_decompress(test_zst)
  test_uns <- base::unserialize(test_unc)
  expect_identical(test_df, test_uns)

})

test_that("works on seritalized data.frames with allowed padding at end", {

  test_df  <- mtcars[1:5, ]
  test_ser <- base::serialize(test_df, NULL)
  test_zst <- zstd_compress(test_ser)

  test_zst <- c(test_zst, raw(10))

  test_unc <- zstd_decompress(test_zst)
  test_uns <- base::unserialize(test_unc)
  expect_identical(test_df, test_uns)

})
