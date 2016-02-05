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

#define hlwsize 8 /* bytes in a word */
#define hlbsize 8 /* bits in a byte */

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
 * Hash Table
 * Quadratic Probing Hash Table
 */
 
typedef struct {
  hlWord_t k;
  hlWord_t v;
} hlHashEl_t; 

typedef struct {
  hlHashEl_t* t;
} hlHashTable_t;
 
void hl_hw2b( hlByte_t* b, hlWord_t w, int l ){
  int idx, i, s = (hlbsize * l);
  for( i = hlbsize; i <= s; i += hlbsize ){
    idx = (i/hlwsize) - 1;
    b[idx] = (w >> (s - i)) & 0xff;
  }
}

hlWord_t hl_hb2w( hlByte_t* b, int l ){
  int idx, i, s = (hlbsize * l);
  hlWord_t w = 0, n;
  for( i = hlbsize; i <= s; i += hlbsize ){
    idx = (i/hlbsize) - 1;
    n = (hlWord_t)b[idx];
    w |= (n << (s - i));
  }
  return w;
} 

/*
 * end hash table
 */ 

int main(void) {
  return 0;
}
