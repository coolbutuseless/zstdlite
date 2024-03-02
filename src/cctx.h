
ZSTD_CCtx *external_ptr_to_zstd_cctx(SEXP cctx_);
void cctx_set_stable_buffers(ZSTD_CCtx *cctx);
void cctx_unset_stable_buffers(ZSTD_CCtx *cctx);
ZSTD_CCtx *init_cctx_with_opts(SEXP opts_, int stable_buffers);
