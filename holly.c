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
 * All accepted types
 * Ints, Floats, Strings (long and short), Objects, Arrays
 */

typedef enum {
  hlInt,
  hlFlt,
  hlStr, /* strings 8 bytes or shorter */
  hlStrl, /* string longer than 8 bytes */
  hlObj,
  hlArr
} hlType_t;

/*
 * Scalar types
 * and some scalar type operations
 */

typedef unsigned char hlByte_t;
typedef unsigned long hlWord_t;

typedef double hlFloat_t;
typedef long   hlInt_t;

typedef struct {
  int       l;
  hlByte_t* s;
} hlStrl_t;

/*
 * NaN-Boxed Values
 */
 
 
#define hlnan  0x7ff8000000000000
#define hlnnan 0xfff8000000000000
#define hlinf  0x7ff0000000000000
#define hlninf 0xfff0000000000000


const hlFloat_t ftest = 0.0/0.0; 
 
 
/*
 * end nan boxed values
 */

/*
 * Value type (does not depend on IEEE Floating Point Numbers)
 * Depends on: Types, long string functions
 * Arbitrary value type that stores Ints, Floats, Strings, 
 * and pointers to objects (objects, arrays, long strings, etc)
 * in no more than 9 bytes
 */

#define hlvmask 0xff00000000000000 

#define hl_vref(v)   (hlByte_t *)(&(v[1]))
#define hl_vclear(v) memset((void *)v, 0, _v_ws + 1)

#define hl_vsetnum(v, n) do { \
  int c = hl_vbcount((hlWord_t)n); \
  hl_vwtob(hl_vref(v), (hlWord_t)n, c); \
  v[0] = (c << 4); \
} while( 0 )

typedef unsigned char hlValue_t[8 + 1]; /* bytes in a word + 1 */

const int _v_bs = 8; /* bits in a byte */
const int _v_ws = sizeof(hlWord_t);

/* prototypes */
int hl_vbcount( hlWord_t );

/* api */
void     hl_vwtob( hlByte_t *, hlWord_t, int );
hlWord_t hl_vbtow( hlByte_t *, int );
void     hl_vsetint( hlValue_t, hlInt_t );
hlInt_t  hl_vgetint( hlValue_t );


int hl_vbcount( hlWord_t w ){
  int c = _v_ws, i = 0;
  int s = _v_bs * _v_ws;
  hlWord_t v;
  for( ; i < s; i += _v_bs ){
    v = w & (hlvmask >> i);
    if( v ) break;
    else c--;
  }
  return c;
}

void hl_vwtob( hlByte_t* b, hlWord_t w, int l ){
  int idx, i, s = (_v_bs * l);
  for( i = _v_bs; i <= s; i += _v_bs ){
    idx = (i/_v_ws) - 1;
    b[idx] = (w >> (s - i)) & 0xff;
  }
}

hlWord_t hl_vbtow( hlByte_t* b, int l ){
  int idx, i, s = (_v_bs * l);
  hlWord_t w = 0, n;
  for( i = _v_bs; i <= s; i += _v_bs ){
    idx = (i/_v_bs) - 1;
    n = (hlWord_t)b[idx];
    w |= (n << (s - i));
  }
  return w;
}

void hl_vsetstr( hlValue_t v, hlByte_t* s, int l ){
  if( l > _v_ws ){
    hlWord_t d = (hlWord_t)s;
    hl_vwtob(hl_vref(v), d, _v_ws);
  } else {
    memcpy(hl_vref(v), s, l);
  }
  v[0] = l;
}

hlByte_t* hl_vgetstr( hlValue_t v ){
  int c = v[0];
  hlByte_t* s;
  if( c > _v_ws ){
    s = (hlByte_t *)hl_vbtow(hl_vref(v), _v_ws);
  } else s = hl_vref(v);
  return s;
}

void hl_vsetint( hlValue_t v, hlInt_t i ){
  hl_vsetnum(v, i);
  v[0] |= hlInt;
}

void hl_vsetfloat( hlValue_t v, hlInt_t i ){
  hl_vsetnum(v, i);
  v[0] |= hlFlt;
}

hlInt_t hl_vgetint( hlValue_t v ){
  int c = v[0] >> 4;
  return hl_vbtow(hl_vref(v), c);
}

/*
 * end value type
 */
 
/*
 * Hash Table
 * Quadratic Probing Hash Table
 */
 
typedef struct {
  void*     k;
  hlValue_t v;
} hlHashEl_t; 

typedef struct {
  hlHashEl_t* t;
} hlHashTable_t;
 
/*
 * end hash table
 */ 

void valueTypeTest() {
  hlValue_t v;
  puts("BEGIN VALUE TEST");
  hl_vsetint(v, 123);
  printf("Value: %lu\n", hl_vgetint(v));
  hl_vclear(v);
  hl_vsetstr(v, (hlByte_t *)"hello how are you", 17);
  printf("Value: %s\n", hl_vgetstr(v));
}

int main(void) {
  unsigned long d = (unsigned long)*(double *)&ftest;
  valueTypeTest();
  printf("%lx\n", d);
  return 0;
}
