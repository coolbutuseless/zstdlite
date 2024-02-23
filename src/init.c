
#include <R.h>
#include <Rinternals.h>

extern SEXP zstd_serialize_(SEXP robj_, SEXP level_);
extern SEXP zstd_unserialize_(SEXP raw_vec_);

extern SEXP zstd_serialize_stream_(SEXP robj_, SEXP level_, SEXP num_threads_);

extern SEXP zstd_compress_(SEXP raw_vec_, SEXP level_);
extern SEXP zstd_decompress_(SEXP raw_vec_);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// .C      R_CMethodDef
// .Call   R_CallMethodDef
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static const R_CallMethodDef CEntries[] = {
  {"zstd_serialize_"       , (DL_FUNC) &zstd_serialize_       , 2},
  {"zstd_serialize_stream_", (DL_FUNC) &zstd_serialize_stream_, 3},
  {"zstd_unserialize_"     , (DL_FUNC) &zstd_unserialize_     , 1},
  {"zstd_compress_"        , (DL_FUNC) &zstd_compress_        , 2},
  {"zstd_decompress_"      , (DL_FUNC) &zstd_decompress_      , 1},
  {NULL, NULL, 0}
};


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Register the methods
//
// Change the '_simplecall' suffix to match your package name
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void R_init_zstdlite(DllInfo *info) {
  R_registerRoutines(
    info,      // DllInfo
    NULL,      // .C
    CEntries,  // .Call
    NULL,      // Fortran
    NULL       // External
  );
  R_useDynamicSymbols(info, FALSE);
}
