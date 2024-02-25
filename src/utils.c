#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

unsigned char *read_file(const char *filename, size_t *src_size) {
  FILE *fp = fopen(filename, "rb");
  if (fp == NULL) error("read_file(): Couldn't open file '%s'", filename);
  fseek(fp, 0, SEEK_END);
  size_t fsize = ftell(fp);
  fseek(fp, 0, SEEK_SET);  /* same as rewind(f); */
  
  unsigned char *buf = malloc(fsize);
  if (buf == NULL) error("read_file(): Could not allocate memory to read '%s'", filename);
  size_t n = fread(buf, 1, fsize, fp);
  fclose(fp);
  
  if (n != fsize) {
    error("read_file(): fread() only read %zu/%zu bytes", n, fsize);
  }
  
  *src_size = fsize;
  return buf;
} 
