
#include <R.h>
#include <Rinternals.h>

extern SEXP zstd_version_(void);

extern SEXP init_cctx_(SEXP opts_);
extern SEXP init_dctx_(SEXP opts_);

extern SEXP get_cctx_settings_(SEXP cctx_);
extern SEXP get_dctx_settings_(SEXP dctx_);

extern SEXP zstd_compress_(SEXP src_, SEXP file_, SEXP cctx_, SEXP opts_, SEXP use_file_streaming_);
extern SEXP zstd_decompress_(SEXP src_, SEXP type_, SEXP dctx_, SEXP opts_, SEXP use_file_streaming_);

extern SEXP zstd_compress_stream_file_(SEXP robj, SEXP file_, SEXP cctx_, SEXP opts_);
extern SEXP zstd_decompress_stream_file_(SEXP raw_vec_, SEXP type_, SEXP dctx_, SEXP opts_);

extern SEXP zstd_serialize_(SEXP robj_, SEXP file_, SEXP cctx_, SEXP opts_, SEXP use_file_streaming_);
extern SEXP zstd_unserialize_(SEXP src_, SEXP dctx_, SEXP opts_, SEXP use_file_streaming_);

extern SEXP zstd_serialize_stream_file_(SEXP robj, SEXP file_, SEXP cctx_, SEXP opts_);
extern SEXP zstd_unserialize_stream_file_(SEXP raw_vec_, SEXP dctx_, SEXP opts_);

extern SEXP zstd_serialize_stream_(SEXP robj, SEXP cctx_, SEXP opts_);
extern SEXP zstd_unserialize_stream_(SEXP raw_vec_, SEXP dctx_, SEXP opts_);

extern SEXP zstd_train_dictionary_(SEXP samples_, SEXP size_, SEXP optim_, SEXP optim_shrink_allow_);
extern SEXP zstd_dict_id_(SEXP dict_);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// .C      R_CMethodDef
// .Call   R_CallMethodDef
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static const R_CallMethodDef CEntries[] = {
  {"zstd_version_"                , (DL_FUNC) &zstd_version_                , 0},
  {"init_cctx_"                   , (DL_FUNC) &init_cctx_                   , 1},
  {"init_dctx_"                   , (DL_FUNC) &init_dctx_                   , 1},
  
  {"get_cctx_settings_"           , (DL_FUNC) &get_cctx_settings_           , 1},
  {"get_dctx_settings_"           , (DL_FUNC) &get_dctx_settings_           , 1},
  
  {"zstd_compress_"               , (DL_FUNC) &zstd_compress_               , 5},
  {"zstd_decompress_"             , (DL_FUNC) &zstd_decompress_             , 5},
  
  {"zstd_compress_stream_file_"   , (DL_FUNC) &zstd_compress_stream_file_   , 4},
  {"zstd_decompress_stream_file_" , (DL_FUNC) &zstd_decompress_stream_file_ , 4},
  
  {"zstd_serialize_"              , (DL_FUNC) &zstd_serialize_              , 5},
  {"zstd_unserialize_"            , (DL_FUNC) &zstd_unserialize_            , 4},
  
  {"zstd_serialize_stream_file_"  , (DL_FUNC) &zstd_serialize_stream_file_  , 4},
  {"zstd_unserialize_stream_file_", (DL_FUNC) &zstd_unserialize_stream_file_, 3},
  
  {"zstd_serialize_stream_"       , (DL_FUNC) &zstd_serialize_stream_       , 3},
  {"zstd_unserialize_stream_"     , (DL_FUNC) &zstd_unserialize_stream_     , 3},
  
  {"zstd_train_dictionary_"       , (DL_FUNC) &zstd_train_dictionary_       , 4},
  {"zstd_dict_id_"                , (DL_FUNC) &zstd_dict_id_                , 1},
  
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
