#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "util.h"

_Noreturn void err( const char* msg, ... ) {
<<<<<<< HEAD
  (void) msg;
  //va_list args;
  //va_start (args, msg);
  //vfprintf(stderr, msg, args);
  //va_end (args);
=======
  va_list args;
  va_start (args, msg);
  vfprintf(stderr, msg, args); // NOLINT 
  va_end (args);
>>>>>>> 5358facd8db9f6b1f499912442843d799add16b8
  abort();
}


extern inline size_t size_max( size_t x, size_t y );
