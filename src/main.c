#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "holly.h"
 
int isprime( unsigned n ){
  int i, r;
  if(  n < 2   ) return 0;
  if(  n < 4   ) return 1;
  if( !(n % 2) ) return 0;
  r = sqrt(n);
  for( i = 3; i <= r; i += 2 ){
    if( !(n % i ) ) return 0;
  }
  return 1;
} 

void primelist( void ){
  unsigned s = 2;
  unsigned ptest;
  unsigned prev;
  unsigned p = 2;
  int i = 0;
  
  printf("\nunsigned hlhprimes[] = {");
  for( ; i < 29; i++ ){
    ptest = s + 1;
    prev = p;
    for( ;; ){
      p = ptest += 2;
      if( p < (prev << 1) ) continue; 
      if( isprime(p) )
        break;
    }
    printf("%d", p);
    if( i < 28 ) printf(", ");
    s <<= 1;
  }
  printf("};\n\n");
}

#include <time.h>

/* get word from file */
unsigned char* getWord (FILE* f) {
  unsigned char* buf = NULL;
  int c = 0, i = 0, bufsize = 10;
  buf = malloc(bufsize + 1);
  memset(buf, 0, bufsize + 1);
  while ((c = fgetc(f)) != '\n') {
    if (c == EOF) return NULL;
    if (i == bufsize)
      buf = realloc(buf, (bufsize += 10) + 1);
    buf[i++] = (unsigned char)c;
  }
  buf[i] = 0;
  return buf;
} 

void loadWords (hlHashTable_t* root) {
  FILE* in = fopen("../cuckoo/words.txt", "r");
  unsigned char* word = NULL;
  /*int i = 0, total = 0;*/
  while ((word = getWord(in))) {
    int l = strlen((char *)word);
    hl_hset(root, word, l, (void*)word);
  }/*
  fclose(in);
  
  printf("inserted %d elements\n", root->c);
  
  in = fopen("../cuckoo/words.txt", "r");
  while ((word = getWord(in))) {
    int l = strlen((char *)word);
    i = hl_hget(root, word, l);
    if( i > -1 ){
      total++;
    }
  }
  printf("retrieved %d elements\n", total);
  
  fclose(in);
  
  in = fopen("../cuckoo/words.txt", "r");
  while ((word = getWord(in))) {
    int l = strlen((char *)word);
    hl_hdel(root, word, l);
  }
  
  fclose(in);
  puts("deleted");
  printf("remaining elements: %d\n", root->c);*/
}

void hashtest( void ){
  hlState_t state;
  double end, start;
  hlHashTable_t table = hl_hinit(&state);

  start = (float)clock()/CLOCKS_PER_SEC; 
  loadWords(&table);
  end = (float)clock()/CLOCKS_PER_SEC;
    
  printf("Inserted, retrieved, and deleted %d objects in %fs - Table size: %d\n", table.c, end - start, hlhprimes[table.s]);
}

unsigned char* readfile( const char* n ){
  unsigned char *buf;
  long fsize;
  FILE *f = fopen(n, "rb");
  fseek(f, 0, SEEK_END);
  fsize = ftell(f);
  fseek(f, 0, SEEK_SET);
  buf = malloc(fsize + 1);
  if( !fread(buf, fsize, 1, f) ){
    fclose(f);
    return NULL;
  }
  fclose(f);
  buf[fsize] = 0;
  return buf;
}

int main( int argc, char** argv ) {
  if( argc > 1 ){
    unsigned char* p = readfile(argv[1]);
    hlState_t s;
    s.error = 0;
    s.prog = p;
    s.ctok.type = -1;
    s.ctok.ltype = -1;
    hl_pstart(&s);
  }
  return 0;
}
