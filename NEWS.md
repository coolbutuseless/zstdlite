

# zstdlite 0.2.3 2021-05-16

* Update to zstd 1.5.0

# zstdlite 0.2.2 2021-03-06

* Update to zstd v1.4.9
* Drop the `magic` header which was used to store the text `ZSTD` as an 
  identifier for the type of data contained.  It's now
  up to the user to ensure that the raw strings fed to `deserialize()` are
  appropriate.
* refactor approach in C to remove a memory allocation.

# zstdlite 0.2.1 2020-12-17

* Update to zstd v1.4.7

# zstdlite 0.2.0 2020-09-24

* Feature: Now uses R's serialization mechanism to compress any R object
* Removed `zstd_compress()` and `zstd_uncompress()`

# zstdlite 0.1.4 2020-09-18

* Use a slightly more complex API into zstd such that negative compression levels
  are now available.
  
# zstdlite 0.1.3 2020-09-18

* Encode dimension information such that arrays and matrices can be recreated
  when decompressing i.e. no longer just simple atomic vectors 

# zstdlite 0.1.2 2020-09-16

* Update to zstd v1.4.6

# zstdlite 0.1.1

* Ignore any padding after end of compressed bytes.

# zstdlite 0.1.0

* Initial release
