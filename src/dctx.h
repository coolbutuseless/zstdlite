
ZSTD_DCtx *external_ptr_to_zstd_dctx(SEXP dctx_);
void dctx_set_stable_buffers(ZSTD_DCtx *dctx);
ZSTD_DCtx *init_dctx_with_opts(SEXP opts_, int stable_buffers);
