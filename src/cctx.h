
ZSTD_CCtx *init_cctx(int level, int num_threads, int stable_buffers);
ZSTD_CCtx *external_ptr_to_zstd_cctx(SEXP cctx_);
