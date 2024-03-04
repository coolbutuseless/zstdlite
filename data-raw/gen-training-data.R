

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Prepare a integer dictionary
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

unlink(root, recursive = TRUE)

tmpdir <- tempdir()
root   <- file.path(tmpdir, "train")
dir.create(root, recursive = TRUE, showWarnings = FALSE)


N <- 1000
len <- 1000

for (i in seq(N)) {
  vec <- sample(1:1000, len, replace = TRUE, prob = (1000:1)^2)
  vec <- serialize(vec, NULL)
  filename <- sprintf("i%05i.rds", i)
  writeBin(vec, file.path(root, filename))
}



for (i in seq(N)) {
  vec <- as.double(sample(1:1000, len, replace = TRUE, prob = (1000:1)^2))
  vec <- serialize(vec, NULL)
  filename <- sprintf("d%05i.rds", i)
  writeBin(vec, file.path(root, filename))
}


system2("zstd", c("--train", file.path(root, "*"), "-o", here::here("data-raw", "vec.dict")))       



dat <- mtcars[sample(nrow(mtcars), 30000, TRUE), ]
dat <- mtcars
dat <- sample(50)
pryr::object_size(dat)

cctx1 <- zstd_cctx()
cctx2 <- zstd_cctx(dict = "./data-raw/vec.dict")

cctx1a <- zstd_cctx(num_threads = 4)
cctx2a <- zstd_cctx(num_threads = 4, dict = "./data-raw/vec.dict")

library(qs)

bench::mark(
  zstd_serialize(dat),
  zstd_serialize(dat, cctx = cctx1),
  zstd_serialize(dat, cctx = cctx2),
  zstd_serialize(dat, num_threads = 4),
  zstd_serialize(dat, num_threads = 4, cctx = cctx1a),
  zstd_serialize(dat, num_threads = 4, cctx = cctx2a),
  qserialize(dat),  
  check = FALSE
)
