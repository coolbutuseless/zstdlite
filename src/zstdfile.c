



#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>
#include <R_ext/Connections.h>

#if ! defined(R_CONNECTIONS_VERSION) || R_CONNECTIONS_VERSION != 1
#error "Unsupported connections API version"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "zstd/zstd.h"
#include "cctx.h"
#include "dctx.h"


// SEXP   R_new_custom_connection(
//            const char *description,  // the filename related to this particular instance
//            const char *mode,         // read/write/binarymode/textmode
//            const char *class_name,   // 'zstdfile'
//            Rconnection *ptr          // Rconnection pointer
//        );
//
//  --- C-level entry to create a custom connection object -- */
// The returned value is the R-side instance. To avoid additional call to getConnection()
//  the internal Rconnection pointer will be placed in ptr[0] if ptr is not NULL.
//  It is the responsibility of the caller to customize callbacks in the structure,
//  they are initialized to dummy_ (where available) and null_ (all others) callbacks.
//  Also note that the resulting object has a finalizer, so any clean up (including after
//  errors) is done by garbage collection - the caller may not free anything in the
//  structure explicitly (that includes the con->private pointer!).
 

// struct Rconn {
//     char* class;
//     char* description;
//     int enc; /* the encoding of 'description' */
//     char mode[5];
//     Rboolean text, isopen, incomplete, canread, canwrite, canseek, blocking, 
// 	isGzcon;
//     Rboolean (*open)(struct Rconn *);
//     void (*close)(struct Rconn *); /* routine closing after auto open */
//     void (*destroy)(struct Rconn *); /* when closing connection */
//     int (*vfprintf)(struct Rconn *, const char *, va_list);
//     int (*fgetc)(struct Rconn *);
//     int (*fgetc_internal)(struct Rconn *);
//     double (*seek)(struct Rconn *, double, int, int);
//     void (*truncate)(struct Rconn *);
//     int (*fflush)(struct Rconn *);
//     size_t (*read)(void *, size_t, size_t, struct Rconn *);
//     size_t (*write)(const void *, size_t, size_t, struct Rconn *);
//     int nPushBack, posPushBack; /* number of lines, position on top line */
//     char **PushBack;
//     int save, save2;
//     char encname[101];
//     /* will be iconv_t, which is a pointer. NULL if not in use */
//     void *inconv, *outconv;
//     /* The idea here is that no MBCS char will ever not fit */
//     char iconvbuff[25], oconvbuff[50], *next, init_out[25];
//     short navail, inavail;
//     Rboolean EOF_signalled;
//     Rboolean UTF8out;
//     void *id;
//     void *ex_ptr;
//     void *private;
//     int status; /* for pipes etc */
//     unsigned char *buff;
//     size_t buff_len, buff_stored_len, buff_pos;
// };


#define DEBUG_ZSTDFILE 0

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// A magic size calculated via ZSTD_CStream_InSize()
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define  INSIZE 131702  
#define OUTSIZE 131591  

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ZSTD state. 
//   - This is user/private data stored with the 'Rconn' struct that gets 
//     passed to each callback function
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef struct {
  
  int user_dctx; // Boolean: did the user supply a dctx?
  int user_cctx; // Boolean: did the user supply a cctx?
  
  ZSTD_DCtx *dctx; // decompression context
  ZSTD_CCtx *cctx; //   compression context
  
  FILE *fp; // The file containing zstd compresseddata
  
  // Used by readLines()/fgetc()
  unsigned char uncompressed_data[OUTSIZE];
  size_t uncompressed_size;
  size_t uncompressed_pos;
  size_t uncompressed_len;
  
  // Used by readBin()
  unsigned char compressed_data[INSIZE];
  size_t compressed_size;
  size_t compressed_pos;
  size_t compressed_len;
} zstd_state;



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// open()
//  - this may be called explicitly by a user call to open(con, mode)
//  - this is also called implicitly by readBin()/writeBin()/readLines()/writeLines();
//
// Possible Modes
//    - "r" or "rt"    Open for reading in text mode.
//    - "w" or "wt"    Open for writing in text mode.
//    - "a" or "at"    Open for appending in text mode.
//    - "rb"           Open for reading in binary mode.
//    - "wb"           Open for writing in binary mode.
//    - "ab"           Open for appending in binary mode.
//    - "r+", "r+b"    Open for reading and writing.
//    - "w+", "w+b"    Open for reading and writing, truncating file initially.
//    - "a+", "a+b"    Open for reading and appending.
//
// Notes:
//   - Supported modes: r, rt, w, wt, rb, wb
//   - unsupported modes: append, simultaneous read/write
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Rboolean zstdfile_open(struct Rconn *rconn) {
  
  if (DEBUG_ZSTDFILE) Rprintf("zstdfile_open(mode = %s)\n", rconn->mode);
  if (rconn->isopen) {
    error("zstdfile(): Connection is already open. Cannot open twice");
  }
  
  if (strchr(rconn->mode, 'a') != NULL) {
    error("zstdfile() does not support append.");
  } else if (strchr(rconn->mode, '+') != NULL) {
    error("zstdfile() does not support simultaneous r/w.");
  }
  
  rconn->text   = strchr(rconn->mode, 'b') ? FALSE : TRUE;
  rconn->isopen = TRUE;
  
  if (strchr(rconn->mode, 'w') == NULL) {
    rconn->canread  =  TRUE;
    rconn->canwrite = FALSE;
  } else {
    rconn->canread  = FALSE;
    rconn->canwrite =  TRUE;
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Setup file pointer
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  FILE *fp;
  if (rconn->canread) {
    fp = fopen(rconn->description, "rb");
  } else {
    fp = fopen(rconn->description, "wb");
  }
  if (fp == NULL) {
    error("zstdfile_(): Couldn't open input file '%s' with mode '%s'", rconn->description, rconn->mode);
  }
  
  // Buffer of compressed data read from the "*.zstd" file
  zstd_state *zstate = (zstd_state *)rconn->private;
  zstate->fp              = fp;
  
  zstate->compressed_pos  = 0;
  zstate->compressed_len  = 0;
  zstate->compressed_size = INSIZE;
  
  zstate->uncompressed_pos  = 0;
  zstate->uncompressed_len  = 0;
  zstate->uncompressed_size = OUTSIZE;
  
  ZSTD_DCtx_reset(zstate->dctx, ZSTD_reset_session_only);
  ZSTD_CCtx_reset(zstate->cctx, ZSTD_reset_session_only);
  
  return TRUE;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Close()
//  - should only change state. No resources should be created/destroyed
//  - all actual destruction should happen in 'destroy()' which is called
//    by the garbage collector.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void zstdfile_close(struct Rconn *rconn) {
  if (DEBUG_ZSTDFILE)Rprintf("zstdfile_close('%s')\n", rconn->description);
  
  rconn->isopen = FALSE;
  zstd_state *zstate = (zstd_state *)rconn->private;
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Need to flush out and compress any remaining bytes
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (rconn->canwrite) {
    static unsigned char zstd_raw[OUTSIZE];
    ZSTD_inBuffer input = { 
      .src  = zstate->uncompressed_data, 
      .size = zstate->uncompressed_pos, 
      .pos  = 0 
    };
    
    size_t remaining_bytes;
    do {
      ZSTD_outBuffer output = { 
        .dst  = zstd_raw, 
        .size = OUTSIZE, 
        .pos  = 0 
      };
      remaining_bytes = ZSTD_compressStream2(zstate->cctx, &output, &input, ZSTD_e_end);
      if (ZSTD_isError(remaining_bytes)) {
        Rprintf("write_bytes_to_stream_file() [end]: error %s\n", ZSTD_getErrorName(remaining_bytes));
      }
      fwrite(output.dst, 1, output.pos, zstate->fp);
    } while (remaining_bytes > 0);
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Close the file
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (zstate->fp) {
    fclose(zstate->fp);
    zstate->fp = NULL;  
  }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Destroy()
//   - R will destroy the Rbonn struct (?)
//   - R will destroy the Rconnection object (?)
//   - Only really have to take care of 'rconn->private' (?)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void zstdfile_destroy(struct Rconn *rconn) {
  if (DEBUG_ZSTDFILE) Rprintf("zstdfile_destroy()\n");
  
  zstd_state *zstate = (zstd_state *)rconn->private;
  if (!zstate->user_dctx && zstate->dctx != NULL) ZSTD_freeDCtx(zstate->dctx);
  if (!zstate->user_cctx && zstate->cctx != NULL) ZSTD_freeCCtx(zstate->cctx);
  
  free(zstate); 
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Not sure what this is for. Just call the standard 'fgetc' callback.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int zstdfile_fgetc_internal(struct Rconn *rconn) {
  if (DEBUG_ZSTDFILE) Rprintf("zstdfile_fgetc_internal()\n");
  return rconn->fgetc(rconn);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// seek()
//   - zstdfile() will not support seeking
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
double zstdfile_seek(struct Rconn *rconn, double x, int y, int z) {
  error("zstdfile_seek() - not supported");
  return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// truncate
//   - zstdfile() will not support truncation
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void zstdfile_truncate(struct Rconn *rconn) {
  error("zstdfile_truncate() - not supported");
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// fflush
//   - zstdfile will not suport flush()
//   - a flush of buffers to file will only occur during 'close()'
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int zstdfile_fflush(struct Rconn *rconn) {
  error("zstdfile_fflush() - not supported\n");
  return 1;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// readBin()
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
size_t zstdfile_read(void *dst, size_t size, size_t nitems, struct Rconn *rconn) {
  if (DEBUG_ZSTDFILE) Rprintf("zstdfile_read(size = %zu, nitems = %zu)\n", size, nitems);
  
  zstd_state *zstate = (zstd_state *)rconn->private;
  
  size_t len = size * nitems;
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Dummy code so that the buffer has something in it!
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // memset(dst, 0, len);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // If file read buffer  is empty, then fill it
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (zstate->compressed_len == 0) {
    zstate->compressed_len = fread(zstate->compressed_data, 1, zstate->compressed_size, zstate->fp);
    zstate->compressed_pos = 0;
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // ZSTD input struct.
  // Note: There may be multiple calls to 'read_bytes_from_stream_file()'
  //       which are all consuming data from the same buffered read from file
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  ZSTD_inBuffer input   = { 
    .src  = zstate->compressed_data + zstate->compressed_pos, 
    .size = zstate->compressed_len  - zstate->compressed_pos, 
    .pos  = 0
  };
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // ZSTD output struct
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  ZSTD_outBuffer output = {
    .dst  = dst, 
    .size = len, 
    .pos  = 0 
  };
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Decompress bytes in the 'compressed_data' buffer until we have the 
  // number of bytes requested (i.e. 'length')
  // If the 'compressed_data' buffer runs out of data, read in some more!
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  while (output.pos < len) {
    size_t const status = ZSTD_decompressStream(zstate->dctx, &output , &input);
    if (ZSTD_isError(status)) {
      error("zstdfile_read() error: %s", ZSTD_getErrorName(status));
    }
    
    // Update the compressed data pointer to where we have decompressed up to
    zstate->compressed_pos += input.pos;
    
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // If the read file buffer (of compressed data) is exhausted: read more!
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    if (zstate->compressed_pos == zstate->compressed_len) {
      
      if (feof(zstate->fp)) {
        rconn->EOF_signalled = TRUE;
        break;
      }
      
      // file read buffer is exhausted. read more bytes
      zstate->compressed_len = fread(zstate->compressed_data, 1, zstate->compressed_size, zstate->fp);
      zstate->compressed_pos = 0;
      
      input.src  = zstate->compressed_data;
      input.size = zstate->compressed_len;
      input.pos  = 0;
    }
  }
  
  return output.pos;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// readLines()
//   - fgetc() called until '\n'. this counts as 1 line.
//   - when EOF reached, return -1
//   - Using zstdfile_read() to populate the uncompressed data buffer 
//     Rather than attempting to read char-by-char.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int zstdfile_fgetc(struct Rconn *rconn) {
  // Rprintf("zstdfile_fgetc()\n");
  zstd_state *zstate = (zstd_state *)rconn->private;
  
  if (zstate->uncompressed_pos == zstate->uncompressed_len) {
    if (feof(zstate->fp)) {
      // This is where EOF is likely connected. Not in the check below.
      rconn->EOF_signalled = TRUE;
      return -1;
    }
    
    // Read some data. and reset the state of the 'uncompressed_data' buffer
    zstate->uncompressed_len = zstdfile_read(zstate->uncompressed_data, 1, zstate->uncompressed_size, rconn);
    zstate->uncompressed_pos = 0;
    
    if (zstate->uncompressed_len == 0) {
      // This is an unlikely path, but just being really paranoid.
      rconn->EOF_signalled = TRUE;
      return -1;
    }
  }
  
  return zstate->uncompressed_data[zstate->uncompressed_pos++];
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// writeBin()
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
size_t zstdfile_write(const void *src, size_t size, size_t nitems, struct Rconn *rconn) {
  if (DEBUG_ZSTDFILE) Rprintf("zstdfile_write(size = %zu, nitems = %zu)\n", size, nitems);
  
  zstd_state *zstate = (zstd_state *)rconn->private;
  
  size_t len = size * nitems;
  
  static unsigned char zstd_raw[OUTSIZE];
  FILE *fp = zstate->fp;
  
  if (zstate->uncompressed_pos + (size_t)len >= zstate->uncompressed_size) {
    
    // Compress current serialize buffer 
    ZSTD_inBuffer input = {
      .src  = zstate->uncompressed_data, 
      .size = zstate->uncompressed_pos, 
      .pos  = 0
    };
    
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Compress the input data
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    do {
      ZSTD_outBuffer output = { 
        .dst  = zstd_raw, 
        .size = OUTSIZE, 
        .pos  = 0 
      };
      
      size_t rem = ZSTD_compressStream2(zstate->cctx, &output, &input, ZSTD_e_continue);
      if (ZSTD_isError(rem)) {
        Rprintf("zstdfile_write(): error %s\n", ZSTD_getErrorName(rem));
      }
      
      fwrite(output.dst, 1, output.pos, fp);
    } while (input.pos != input.size);
    
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Reset the uncompressed buffer
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    zstate->uncompressed_pos = 0;
    
    
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Check length of new uncompressed data. 
    // If larger than zstate->uncompssed_size, then compress directly and return
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    if (len >= zstate->uncompressed_size) {
      ZSTD_inBuffer input = {
        .src  = src, 
        .size = len, 
        .pos  = 0
      };
      
      do {
        ZSTD_outBuffer output = { 
          .dst  = zstd_raw, 
          .size = OUTSIZE, 
          .pos  = 0 
        };
        
        size_t rem = ZSTD_compressStream2(zstate->cctx, &output, &input, ZSTD_e_continue);
        if (ZSTD_isError(rem)) {
          Rprintf("zstdfile_write(): error %s\n", ZSTD_getErrorName(rem));
        }
        
        fwrite(output.dst, 1, output.pos, fp);
      } while (input.pos != input.size);
      
      return len;
    }
  }
  
  memcpy(zstate->uncompressed_data + zstate->uncompressed_pos, src, len);
  zstate->uncompressed_pos += len;
  
  return len;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// writeLines
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int zstdfile_vfprintf(struct Rconn *rconn, const char* fmt, va_list ap) {
  if (DEBUG_ZSTDFILE) Rprintf("zstdfile_vfprintf(fmt = '%s')\n", fmt);
  
  unsigned char str_buf[OUTSIZE + 1];
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // vsnprintf() return value:
  //   The number of characters written if successful or negative value if an 
  //   error occurred. If the resulting string gets truncated due to buf_size 
  //   limit, function returns the total number of characters (not including the 
  //   terminating null-byte) which would have been written, if the limit 
  //   was not imposed. 
  //
  // So when vsnprintf() overflows the given size, it returns the number of 
  // characters it couldn't write.  Tell it the buffer size is '0' and it
  // will just return how long a buffer would be needed to contain the string!
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int slen = vsnprintf((char *)(str_buf), 0, fmt, ap);
  int wlen = slen;
  if (slen > OUTSIZE) {
    warning("zstdfile_vfprintf(): Long string truncated to length = %i\n", OUTSIZE);
    wlen = OUTSIZE;
  }

  
  slen = vsnprintf((char *)(str_buf), OUTSIZE, fmt, ap);
  if (slen < 0) {
    error("zstdfile_vfprintf(): error in 'vsnprintf()");
  }
  
  zstdfile_write(str_buf, 1, wlen, rconn);
  
  return 1;
}



// #include <Defn.h>  // For RCNTXT. Only available internally in R?
// static void checked_open(Rconnection con) {
//   RCNTXT cntxt;
//   
//   // Set up a context which will destroy the connection on error,
//   // including warning turned into error
//   begincontext(&cntxt, CTXT_CCODE, R_NilValue, R_BaseEnv, R_BaseEnv,
//                R_NilValue, R_NilValue);
//   cntxt.cend = &con->destroy;
//   cntxt.cenddata = con;
//   Rboolean success = con->open(con);
//   endcontext(&cntxt);
//   if(!success) {
//     con>-destroy(con);
//     error("zstdfile(): cannot open the connection");
//   }
// }  
  
  
  
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Initialize a zstdfile() R connection object to return to the user
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP zstdfile_(SEXP description_, SEXP mode_, SEXP opts_, SEXP cctx_, SEXP dctx_) {
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // zstd state of compression
  // Most of this struct gets initialized in 'zstdfile_open()'
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  zstd_state *zstate = (zstd_state *)calloc(1, sizeof(zstd_state));
  
  // Compession/Decompression context
  if (isNull(cctx_)) {
    zstate->cctx = init_cctx_with_opts(opts_, 0, 1);
  } else {
    zstate->user_cctx = TRUE;
    zstate->cctx = external_ptr_to_zstd_cctx(cctx_);
  }
  
  if (isNull(dctx_)) {
    zstate->dctx = init_dctx_with_opts(opts_, 0, 1);
  } else {
    zstate->user_dctx = TRUE;
    zstate->dctx = external_ptr_to_zstd_dctx(dctx_);
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // R will alloc for 'con' within R_new_custom_connection() and then
  // I think it takes responsibility for freeing it later.
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  Rconnection con = NULL;
  SEXP rc = PROTECT(R_new_custom_connection(CHAR(STRING_ELT(description_, 0)), "rb", "zstdfile", &con));
  
  con->isopen     = FALSE; // not open initially.
  con->incomplete =  TRUE; // NFI. Data write hasn't been completed?
  con->text       = FALSE; // binary connection by default
  con->canread    =  TRUE; // read-only for now
  con->canwrite   =  TRUE; // read-only for now
  con->canseek    = FALSE; // not possible in this implementation
  con->blocking   =  TRUE; // blacking IO
  con->isGzcon    = FALSE; // Not a gzcon
  
  // Not sure what this really means, but zstdfile() is not going to do 
  // any character conversion, so let's pretend any text returned in readLines()
  // is utf8.
  con->UTF8out    =  TRUE; 
  con->private = zstate;
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Callbacks
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  con->open           = zstdfile_open;
  con->close          = zstdfile_close;
  con->destroy        = zstdfile_destroy;
  con->vfprintf       = zstdfile_vfprintf;
  con->fgetc          = zstdfile_fgetc;
  con->fgetc_internal = zstdfile_fgetc_internal;
  con->seek           = zstdfile_seek;
  con->truncate       = zstdfile_truncate;
  con->fflush         = zstdfile_fflush;
  con->read           = zstdfile_read;
  con->write          = zstdfile_write;
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Auto open if 'mode' is set to something other than the empty string.
  // An issue is that without the context stuff (not exported from R?), 
  // I don't think I can get the context to auto-close!
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  const char *mode = CHAR(STRING_ELT(mode_, 0));
  strncpy(con->mode, mode, 4);
  con->mode[4] = '\0';
  if (strlen(mode) > 0) {
    con->open(con);
  }
  
  UNPROTECT(1);
  return rc;
}













