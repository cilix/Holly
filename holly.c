/*
 * The Holly Programming Language
 *
 * Author  - Matthew Levenstein
 * License - MIT
 */
 
/*
 * Code guidelines 
 *
 * All typedefs take the form hl<Type>_t, where <Type> is capitalized
 *
 * All functions (or function-like macros) take the form 
 *      hl_<namespace><function>, where
 *      where namesespace is (as often as possible) a single letter denoting the 
 *      class of function. Example hl_hset() for the 'set' function of the hash table.
 *      No camelcase for functions.
 *
 * All global variables and #defines take the same form as functions 
 *      without the underscores
 * 
 */
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
 * System wide definitions
 */
 
typedef unsigned char hlByte_t;
typedef unsigned long hlWord_t;

#define HL_WORD_SIZE 8 /* bytes in a word */
#define HL_BYTE_SIZE 8 /* bits in a byte */

/* error codes */
#define HL_MALLOC_FAIL 0x1

typedef struct {
  int error;
} HollyState_t;

void* hl_malloc(HollyState_t* h, int s ){
  void* buf;
  if( !(buf = malloc(s)) ){
     h->error = HL_MALLOC_FAIL;
  }
  return buf;
}

/*
 * Hash Table
 * Quadratic Probing Hash Table
 */
 
typedef struct {
  int      l; /* key length */
  hlWord_t k; /* key */
  hlWord_t v; /* value */
} hlHashEl_t; 

typedef struct {
  int         s; /* table size */
  int         c; /* table count */
  hlHashEl_t* t;
} hlHashTable_t;


unsigned hlhprimes[] = {
  5, 11, 23, 47, 97, 197, 397, 797, 1597, 3203, 6421, 
  12853, 25717, 51437, 102877, 205759, 411527, 823117, 
  1646237, 3292489, 6584983, 13169977, 26339969, 52679969, 
  105359939, 210719881, 421439783, 842879579, 1685759167
};

void hl_hw2b( hlByte_t* b, hlWord_t w, int l ){
  int idx, i, s = (HL_BYTE_SIZE * l);
  for( i = HL_BYTE_SIZE; i <= s; i += HL_BYTE_SIZE ){
    idx = (i/HL_WORD_SIZE) - 1;
    b[idx] = (w >> (s - i)) & 0xff;
  }
}

hlWord_t hl_hb2w( hlByte_t* b, int l ){
  int idx, i, s = (HL_BYTE_SIZE * l);
  hlWord_t w = 0, n;
  for( i = HL_BYTE_SIZE; i <= s; i += HL_BYTE_SIZE ){
    idx = (i/HL_BYTE_SIZE) - 1;
    n = (hlWord_t)b[idx];
    w |= (n << (s - i));
  }
  return w;
} 

hlHashTable_t hl_init( HollyState_t s ){
  hlHashTable_t h;
  h.c = 0;
  h.s = 0; /* index in the prime table */
  /* malloc h.t to the size */
  return h;
} 

hlHashEl_t hl_hinitnode( hlByte_t* k, int l, hlWord_t v ){
  hlHashEl_t n;
  if( l < HL_WORD_SIZE ){
    n.k = hl_hb2w(k, l);
  } else {
    n.k = (hlWord_t)k;
  }
  n.l = l;
  n.v = v;
  return n;
}

/*
 * end hash table
 */ 

/*
 * Types
 */

typedef hlByte_t* hlString_t;
typedef double    hlNum_t;
typedef hlByte_t  hlBool_t;

/*
 * NaN-Boxed Values
 */
 
#define hlnan  0x7ff8000000000000
#define hlnnan 0xfff8000000000000
#define hlinf  0x7ff0000000000000
#define hlninf 0xfff0000000000000
 
/*
 * end values
 */
 
#include <math.h>
 
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
    ptest = s;
    prev = p;
    for( ;; ){
      p = ptest++;
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


int main(void) {
  primelist();  
  return 0;
}
