


#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Write a byte into the buffer at the current location.
// The actual buffer is encapsulated as part of the stream structure, so you
// have to extract it first
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void count_byte(R_outpstream_t stream, int c) {
  int *count = (int *)stream->data;
  *count += 1;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Write multiple bytes into the buffer at the current location.
// The actual buffer is encapsulated as part of the stream structure, so you
// have to extract it first
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void count_bytes(R_outpstream_t stream, void *src, int length) {
  int *count = (int *)stream->data;
  *count += length;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Serialize an R object, but ony count the bytes.  C function
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int calc_size_robust(SEXP robj) {

  // Initialise the count
  int count = 0;

  // Create the output stream structure
  struct R_outpstream_st output_stream;

  // Initialise the output stream structure
  R_InitOutPStream(
    &output_stream,            // The stream object which wraps everything
    (R_pstream_data_t) &count, // The actual data
    R_pstream_binary_format,   // Store as binary
    3,                         // Version = 3 for R >3.5.0 See `?base::serialize`
    count_byte,                // Function to write single byte to buffer
    count_bytes,               // Function for writing multiple bytes to buffer
    NULL,                      // Func for special handling of reference data.
    R_NilValue                 // Data related to reference data handling
  );

  // Serialize the object into the output_stream
  R_Serialize(robj, &output_stream);

  return count;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Serialize an R object, but ony count the bytes. R shim function
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP calc_size_robust_(SEXP robj) {
  return ScalarInteger(calc_size_robust(robj));
}
