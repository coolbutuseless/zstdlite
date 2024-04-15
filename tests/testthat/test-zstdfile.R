

test_that("zstdfile works", {
  tmp <- tempfile()
  dat <- as.raw(1:255)
  writeBin(dat, zstdfile(tmp, level = 20))
  tst <- readBin(zstdfile(tmp),  raw(), 1000)
  expect_identical(tst, dat)
  
  tmp <- tempfile()
  txt <- as.character(mtcars)
  writeLines(txt, zstdfile(tmp))
  readLines(zstdfile(tmp))
})
