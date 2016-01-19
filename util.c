#include <stdlib.h>
#include <string.h>

#include "util.h"

unsigned char* strclone( unsigned char* s ){
  int l = strlen((const char *)s);
  unsigned char* n = malloc(l + 1);
  if( !n ) return NULL;
  memcpy(n, s, l);
  n[l] = 0;
  return n;
}
