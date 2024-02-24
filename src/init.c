
#include <R.h>
#include <Rinternals.h>

extern SEXP init_cctx_(SEXP level_, SEXP num_threads_, SEXP dict_);
extern SEXP init_dctx_(SEXP dict_);

extern SEXP zstd_serialize_(SEXP robj_, SEXP level_, SEXP num_threads_, SEXP cctx_);
extern SEXP zstd_unserialize_(SEXP raw_vec_, SEXP dctx_);

extern SEXP zstd_serialize_stream_(SEXP robj_, SEXP level_, SEXP num_threads_, SEXP cctx_);

extern SEXP zstd_compress_(SEXP raw_vec_, SEXP level_, SEXP num_threads_, SEXP cctx_);
extern SEXP zstd_decompress_(SEXP raw_vec_, SEXP dctx_);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// .C      R_CMethodDef
// .Call   R_CallMethodDef
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static const R_CallMethodDef CEntries[] = {
  {"init_cctx_"            , (DL_FUNC) &init_cctx_            , 3},
  {"init_dctx_"            , (DL_FUNC) &init_dctx_            , 1},
  {"zstd_serialize_"       , (DL_FUNC) &zstd_serialize_       , 4},
  {"zstd_serialize_stream_", (DL_FUNC) &zstd_serialize_stream_, 4},
  {"zstd_unserialize_"     , (DL_FUNC) &zstd_unserialize_     , 2},
  {"zstd_compress_"        , (DL_FUNC) &zstd_compress_        , 4},
  {"zstd_decompress_"      , (DL_FUNC) &zstd_decompress_      , 2},
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
