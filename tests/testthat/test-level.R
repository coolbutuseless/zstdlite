

test_that("accelerated compression/decompression of raw vectors works", {
  for (N in c(1e2, 1e3, 1e4, 1e5)) {
    for (acc in 1:10) {
      set.seed(1)
      input_bytes <- as.raw(sample(seq(1:5), N, prob = (1:5)^2, replace = TRUE))

      compressed_bytes <- zstd_compress(input_bytes, acc)
      # cat(N, " ", acc, " ", length(compressed_bytes)/length(input_bytes), "\n")
      decompressed_bytes <- zstd_decompress(compressed_bytes)

      expect_identical(input_bytes, decompressed_bytes)
    }
  }
})

