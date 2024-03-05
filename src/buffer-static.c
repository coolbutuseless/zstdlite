


#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "buffer-static.h"


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// This file contains code for reading/writing to a statically sized buffer.
//
// After initialisation, the size of the buffer is never changed
//
// The struct for the buffer is defined in 'buffer-static.h'
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Initialise an empty buffer to hold 'nbytes'
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static_buffer_t *init_buffer(size_t nbytes) {
  static_buffer_t *buf = (static_buffer_t *)malloc(sizeof(static_buffer_t));
  if (buf == NULL) {
    error("init_buffer(): cannot malloc buffer");
  }

  buf->data = (unsigned char *)malloc(nbytes * sizeof(unsigned char));
  if (buf->data == NULL) {
    error("init_buffer(): cannot malloc buffer data");
  }

  buf->length = nbytes;
  buf->pos = 0;

  return buf;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Write a byte into the buffer at the current location.
// The actual buffer is encapsulated as part of the stream structure, so you
// have to extract it first
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void write_byte(R_outpstream_t stream, int c) {
  error("write_byte(): This function unused in binary serialization.  Panic at the disco!");
  // static_buffer_t *buf = (static_buffer_t *)stream->data;
  // buf->data[buf->pos++] = (unsigned char)c;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Write multiple bytes into the buffer at the current location.
// The actual buffer is encapsulated as part of the stream structure, so you
// have to extract it first
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void write_bytes(R_outpstream_t stream, void *src, int length) {
  static_buffer_t *buf = (static_buffer_t *)stream->data;
  memcpy(buf->data + buf->pos, src, (size_t)length);
  buf->pos += (size_t)length;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Read a byte from the serialized stream
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int read_byte(R_inpstream_t stream) {
  static_buffer_t *buf = (static_buffer_t *)stream->data;
  return buf->data[buf->pos++];
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Read multiple bytes from the serialized stream
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void read_bytes(R_inpstream_t stream, void *dst, int length) {
  static_buffer_t *buf = (static_buffer_t *)stream->data;
  memcpy(dst, buf->data + buf->pos, (size_t)length);
  buf->pos += (size_t)length;
}
