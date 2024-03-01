
ZSTD_DCtx *external_ptr_to_zstd_dctx(SEXP dctx_);
ZSTD_DCtx *init_dctx(int validate_checksum, int stable_buffers);
void dctx_set_stable_buffers(ZSTD_DCtx *dctx);
