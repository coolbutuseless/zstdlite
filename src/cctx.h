
ZSTD_CCtx *init_cctx(int level, int num_threads, int include_checksum, int stable_buffers);
ZSTD_CCtx *external_ptr_to_zstd_cctx(SEXP cctx_);
void cctx_set_stable_buffers(ZSTD_CCtx *cctx);
