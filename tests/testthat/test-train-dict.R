

test_that("multiplication works", {
  
  cars <- rownames(mtcars)
  
  samples <- lapply(
    seq_len(1000),
    \(x) paste(sample(cars), collapse=",")
  )
  
  dict <- zstd_train_dict_serialize(samples, size = 2000)
  expect_true(zstd_dict_id(dict) != 0)  # is a real dictionary!
  
  
  samples_raw <- lapply(samples, charToRaw)
  
  dict1 <- zstd_train_dict_compress(samples_raw, size = 2000)
  expect_true(zstd_dict_id(dict1) != 0)  # is a real dictionary!
  
  dict2 <- zstd_train_dict_compress(samples, size = 2000)
  expect_true(zstd_dict_id(dict2) != 0)  # is a real dictionary!
  
  # two dictionaries trained identically should be identical
  expect_identical(dict1, dict2)
  expect_identical(zstd_dict_id(dict1), zstd_dict_id(dict2))
    
})
