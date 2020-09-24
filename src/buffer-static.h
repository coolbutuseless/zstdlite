
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// The data buffer.
// Needs total length and pos to keep track of how much data it contains
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef struct {
  R_xlen_t length;
  R_xlen_t pos;
  unsigned char *data;
} static_buffer_t;

static_buffer_t *init_buffer(int nbytes);
void write_byte(R_outpstream_t stream, int c);
void write_bytes(R_outpstream_t stream, void *src, int length);
int read_byte(R_inpstream_t stream);
void read_bytes(R_inpstream_t stream, void *dst, int length);
