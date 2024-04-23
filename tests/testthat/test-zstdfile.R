

test_that("zstdfile to file works", {
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


test_that("zstdfile to connection works", {
  tmp <- tempfile()
  dat <- as.raw(1:255)
  writeBin(dat, zstdfile(file(tmp), level = 20))
  tst <- readBin(zstdfile(file(tmp)),  raw(), 1000)
  expect_identical(tst, dat)
  
  tmp <- tempfile()
  txt <- as.character(mtcars)
  writeLines(txt, zstdfile(file(tmp)))
  readLines(zstdfile(file(tmp)))
})