
# zstdlite 0.2.6  2024-03-10

* Fixes for CRAN
    * Added explicit notice of copyright by Facebook

# zstdlite 0.2.5  2024-03-09

* Release to CRAN

# zstdlite 0.2.4.9010 2024-03-09

* Add `zstd_version()`
* `zstd_train_dict_compress()` now accepts a list of strings (in addition to
  a list of raw vectors)

# zstdlite 0.2.4.9009 2024-03-04

* Streaming now optional for file I/O for `zstd_serialize()`/`zstd_unserialize()`
    * Use option `use_file_streaming = TRUE` to enable

# zstdlite 0.2.4.9008 2024-03-02

* Added optional streaming for `zstd_compress()`/`zstd_decompress()`
    * Use option `use_file_streaming = TRUE` to enable

# zstdlite 0.2.4.9007 2024-03-01

* Tidy handling of options and initialisation of compression/decompression
  contexts
* Option to include checksum and validate

# zstdlite 0.2.4.9006 2024-02-27

* `zstd_decompress()` now optionally returns the decompressed buffer as a string
* `zstd_compress()` will compress both raw vectors and strings

# zstdlite 0.2.4.9005 2024-02-26

* Add support for training dictionaries
    * `zstd_train_dict_compress()` if training for `zstd_compress()`
    * `zstd_train_dict_serialize()` if training for `zstd_serialize()`
    * `zstd_dict_id()` to get the dict ID

# zstdlite 0.2.4.9004 2024-02-25

* Refactor R interface to just:
    * `zstd_cctx()`, `zstd_dctx()`
    * `zstd_compress()`, `zstd_decompress()`
    * `zstd_serialize()`, `zstd_unserialize()`
        * Will use streaming if working from files


# zstdlite 0.2.4.9003 2024-02-25

* Streaming serialization/unserialisation with compressed files:
    * `zstd_serialize_stream_file()`
    * `zstd_unserialize_stream_file()`

# zstdlite 0.2.4.9002 2024-02-24

* `ZSTD_dctx` can be created separately and passed as compression argument
    * Enables re-use of same context.
    * `zstd_dctx()`
* Framework in place to support dictionaries for compression/decompression.
    * Seems to use dictionary and not crash
    * Results don't seem correct though

# zstdlite 0.2.4.9001 2024-02-23

* Streaming compression
* Multithreaded compression
* `ZSTD_cctx` can be created separately and passed as compression argument
    * Enables re-use of same context 
    * See `zstd_cctx()`


# zstdlite 0.2.4.9000 2024-02-21

* Add compress/decompress of raw vectors

# zstdlite 0.2.4 2022-01-23

* Update to zstd 1.5.2

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
