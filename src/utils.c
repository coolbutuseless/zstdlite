#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Read a file into a memory buffer.  Caller is responsible for freeing memory
//
// @param filename full filename
// @param *src_size number of bytes read. returned to user
//
// @return pointer to allocated memory buffer with contents of file.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
unsigned char *read_file(const char *filename, size_t *src_size) {
  FILE *fp = fopen(filename, "rb");
  if (fp == NULL) error("read_file(): Couldn't open file '%s'", filename);
  fseek(fp, 0, SEEK_END);
  size_t fsize = (size_t)ftell(fp);
  fseek(fp, 0, SEEK_SET);  /* same as rewind(f); */
  
  unsigned char *buf = malloc(fsize);
  if (buf == NULL) {
    error("read_file(): Could not allocate memory to read %zu bytes from '%s'", fsize, filename);
  }
  
  size_t n = fread(buf, 1, fsize, fp);
  fclose(fp);
  
  if (n != fsize) {
    error("read_file(): fread() only read %zu/%zu bytes", n, fsize);
  }
  
  *src_size = fsize;
  return buf;
} 


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Read a file into a memory buffer.  Caller is responsible for freeing memory
//
// @param filename full filename
// @param max_bytes maximum number of bytes to read
// @param *src_size number of bytes read. returned to user
//
// @return pointer to allocated memory buffer with contents of file.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
unsigned char *read_partial_file(const char *filename, size_t max_bytes, size_t *src_size) {
  FILE *fp = fopen(filename, "rb");
  if (fp == NULL) error("read_partial_file(): Couldn't open file '%s'", filename);
  fseek(fp, 0, SEEK_END);
  *src_size = (size_t)ftell(fp);
  fseek(fp, 0, SEEK_SET);  /* same as rewind(f); */
  
  unsigned char *buf = malloc(max_bytes);
  if (buf == NULL) {
    error("read_partial_file(): Could not allocate memory to read %zu bytes from '%s'", max_bytes, filename);
  }
  
  size_t n = fread(buf, 1, max_bytes, fp);
  fclose(fp);
  
  if (n != max_bytes) {
    error("read_partial_file(): fread() only read %zu/%zu bytes", n, max_bytes);
  }
  
  return buf;
} 




