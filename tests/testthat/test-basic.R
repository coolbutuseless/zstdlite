

test_that("basic compression/decompression of raw vectors works", {
  set.seed(1)
  for (N in c(1e2, 1e3, 1e4, 1e5)) {
    input_bytes <- as.raw(sample(seq(1:5), N, prob = (1:5)^2, replace = TRUE))

    compressed_bytes <- zstd_compress(input_bytes)
    expect_true(length(compressed_bytes) < length(input_bytes))  # for non-pathological inputs
    decompressed_bytes <- zstd_decompress(compressed_bytes)

    expect_identical(input_bytes, decompressed_bytes)
  }
})


test_that("basic compression/decompression of integer vectors works", {
  set.seed(2)
  for (N in c(1e2, 1e3, 1e4, 1e5)) {
    input_bytes <- sample(seq(1:5), N, prob = (1:5)^2, replace = TRUE)

    compressed_bytes <- zstd_compress(input_bytes)
    expect_true(length(compressed_bytes) < length(input_bytes) * 4)  # for non-pathological inputs
    decompressed_bytes <- zstd_decompress(compressed_bytes)

    expect_identical(input_bytes, decompressed_bytes)
  }
})


test_that("basic compression/decompression of numeric vectors works", {
  set.seed(3)
  for (N in c(1e2, 1e3, 1e4, 1e5)) {
    input_bytes <- sample(rnorm(5), N, prob = (1:5)^2, replace = TRUE)

    compressed_bytes <- zstd_compress(input_bytes)
    expect_true(length(compressed_bytes) < length(input_bytes) * 8)  # for non-pathological inputs
    decompressed_bytes <- zstd_decompress(compressed_bytes)

    expect_identical(input_bytes, decompressed_bytes)
  }
})


test_that("basic compression/decompression of logical vectors works", {
  set.seed(4)
  for (N in c(1e2, 1e3, 1e4, 1e5)) {
    input_bytes <- as.logical(sample(c(T, F, T, T, F), N, prob = (1:5)^2, replace = TRUE))

    compressed_bytes <- zstd_compress(input_bytes)
    expect_true(length(compressed_bytes) < length(input_bytes) * 8)  # for non-pathological inputs
    decompressed_bytes <- zstd_decompress(compressed_bytes)

    expect_identical(input_bytes, decompressed_bytes)
  }
})


test_that("basic compression/decompression of complex vectors works", {
  set.seed(5)
  for (N in c(1e2, 1e3, 1e4, 1e5)) {
    real <- sample(rnorm(5), N, prob = (1:5)^2, replace = TRUE)
    imag <- sample(rnorm(5), N, prob = (1:5)^2, replace = TRUE)
    input_bytes <- complex(real = real, imaginary = imag)

    compressed_bytes <- zstd_compress(input_bytes)
    expect_true(length(compressed_bytes) < length(input_bytes) * 8)  # for non-pathological inputs
    decompressed_bytes <- zstd_decompress(compressed_bytes)

    expect_identical(input_bytes, decompressed_bytes)
  }
})

