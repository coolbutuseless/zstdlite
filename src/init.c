
#include <R.h>
#include <Rinternals.h>

extern SEXP init_cctx_(SEXP level_, SEXP num_threads_, SEXP dict_);
extern SEXP init_dctx_(SEXP dict_);

extern SEXP zstd_compress_(SEXP raw_vec_, SEXP file_, SEXP level_, SEXP num_threads_, SEXP cctx_);
extern SEXP zstd_decompress_(SEXP raw_vec_, SEXP dctx_);

extern SEXP zstd_serialize_(SEXP robj_, SEXP file_, SEXP level_, SEXP num_threads_, SEXP cctx_);
extern SEXP zstd_unserialize_(SEXP src_, SEXP dctx_);

extern SEXP zstd_serialize_stream_file_(SEXP robj_, SEXP file_, SEXP level_, SEXP num_threads_, SEXP cctx_);
extern SEXP zstd_unserialize_stream_file_(SEXP raw_vec_, SEXP dctx_);

extern SEXP zstd_serialize_stream_(SEXP robj_, SEXP level_, SEXP num_threads_, SEXP cctx_);
extern SEXP zstd_unserialize_stream_(SEXP raw_vec_, SEXP dctx_);

extern SEXP zstd_train_dictionary_(SEXP samples_, SEXP size_, SEXP optim_, SEXP optim_shrink_allow_);
extern SEXP zstd_dict_id_(SEXP dict_);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// .C      R_CMethodDef
// .Call   R_CallMethodDef
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static const R_CallMethodDef CEntries[] = {
  {"init_cctx_"                   , (DL_FUNC) &init_cctx_                   , 3},
  {"init_dctx_"                   , (DL_FUNC) &init_dctx_                   , 1},
  
  {"zstd_serialize_"              , (DL_FUNC) &zstd_serialize_              , 5},
  {"zstd_unserialize_"            , (DL_FUNC) &zstd_unserialize_            , 2},
  
  {"zstd_compress_"               , (DL_FUNC) &zstd_compress_               , 5},
  {"zstd_decompress_"             , (DL_FUNC) &zstd_decompress_             , 2},
  
  {"zstd_serialize_stream_file_"  , (DL_FUNC) &zstd_serialize_stream_file_  , 5},
  {"zstd_unserialize_stream_file_", (DL_FUNC) &zstd_unserialize_stream_file_, 2},
  
  {"zstd_serialize_stream_"       , (DL_FUNC) &zstd_serialize_stream_       , 4},
  {"zstd_unserialize_stream_"     , (DL_FUNC) &zstd_unserialize_stream_     , 2},
  
  {"zstd_train_dictionary_"       , (DL_FUNC) &zstd_train_dictionary_       , 4},
  {"zstd_dict_id_"                , (DL_FUNC) &zstd_dict_id_                , 2},
  
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
