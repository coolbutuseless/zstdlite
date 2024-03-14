
# Submission Notes for v0.2.7

## R CMD check results

0 errors | 0 warnings | 1 note

* This is a new release/re-submission after removal from CRAN.

## Response to Prof. Ripley's Comment on Copyright and Authorship.

This section provides explanation in response to Prof. Ripley's email comment 
(13 March 2024).

* 'Meta' has been clarified as the sole 'cph' in DESCRIPTION
* 'Yann' has been clarified as an 'aut' in DESCRIPTION (replacing 'ctb' and 
  'cph' roles in prior submission)
* Full name of library ('Zstandard') has been used in Authors@R field and in 
  'Copyright' field in DESCRIPTION

## Response to Prof. Ripleys' Comment on dynamic linking to system library

This section provides explanation in response to Prof Ripley's email comment 
(13 March 2024).

The 'zstdlite' package (now and in the future) makes use of features
only available when compiled with the flag 'ZSTD_STATIC_LINKING_ONLY'
and statically linked.  That is, the dynamic linking with existing
system 'Zstandard' libraries does not provide all features necessary for this 
package and its future development - hence 'zstd.c' and 'zstd.h' are included 
as part of the package and statically linked.  
