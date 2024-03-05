

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// If a cctx is assigned a refCDict, then the cdict must have a lifetime
// equivalent to cctx, as only a *reference* is copied into the cctx
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef struct {
  ZSTD_CCtx *cctx;
  ZSTD_CDict *cdict;
} cctx_meta_t;



ZSTD_CCtx *external_ptr_to_zstd_cctx(SEXP cctx_);
void cctx_set_stable_buffers(ZSTD_CCtx *cctx);
void cctx_unset_stable_buffers(ZSTD_CCtx *cctx);
cctx_meta_t *init_cctx_with_opts(SEXP opts_, int stable_buffers);
