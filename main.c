#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "parse.h"

unsigned char* read( FILE* f ){
  unsigned char* b = NULL;
  int c = 0, i = 0, s = 10;

  b = malloc(s + 1);
  memset(b, 0, s + 1);

  while( (c = fgetc(f)) != EOF ){
    if( i == s ) b = realloc(b, (s += 10) + 1);
    b[i++] = (unsigned char)c;
  }

  b[i] = 0;
  return b;
}

int main( int argc, char** argv ){
  FILE* f = NULL;
  unsigned char* p;
  struct __state state;
  
  if( argc >= 2 ){
    f = fopen(argv[1], "r");
  } else {
    fprintf(stderr, "You must provide a file name.\n");
    return 1;
  }

  if( !f ){
    fprintf(stderr, "File open error.\n");
    return 1;
  }

  state.prog = p = read(f);
  state.err = 0;
  state.tok.type = no_tok;
  state.tok.as_int = state.tok.as_char = 0;
  state.tok.as_string = state.tok.as_name = NULL;
  
  vm_init(&(state.vm));
  start(&state);
  vm_run(&(state.vm));
  free(p);
  
  return 0;
}
