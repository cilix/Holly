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

#define HL_PTR_SIZE 8 /* bytes in a word */

/* error codes */
#define HL_MALLOC_FAIL 0x1

typedef struct {
  int error;
} hlState_t;

void* hl_malloc(hlState_t* h, int s ){
  void* buf;
  if( !(buf = malloc(s)) ){
     h->error = HL_MALLOC_FAIL;
  } else {
     memset(buf, 0, s);
  }
  return buf;
}

/*
 * Hash Table
 * Quadratic Probing Hash Table
 */
 
typedef struct {
  int      l; /* key length */
  unsigned h; /* hash value */
  union {
    unsigned char  skey[HL_PTR_SIZE];
    unsigned char* lkey;
  } k;
  void* v; /* value */
} hlHashEl_t; 

typedef struct {
  int         s; /* table size */
  unsigned    c; /* table count */
  hlHashEl_t* t;
  char        f; /* table full */
  hlState_t*  state; /* compiler state */
} hlHashTable_t;


int hlhprimes[] = {
  5, 11, 23, 47, 97, 197, 397, 797, 1597, 3203, 6421, 
  12853, 25717, 51437, 102877, 205759, 411527, 823117, 
  1646237, 3292489, 6584983, 13169977, 26339969, 52679969, 
  105359939, 210719881, 421439783, 842879579, 1685759167
};


/* forward declarations */
void          hl_hw2b( unsigned char*, void*, int );
void*         hl_hb2w( unsigned char*, int );
hlHashTable_t hl_hinit( hlState_t*  );
hlHashEl_t    hl_hinitnode( unsigned char*, int, void* );
unsigned      hl_hsax( unsigned char *, int );
void          hl_hset( hlHashTable_t*, unsigned char*, int, void* );
int           hl_hget( hlHashTable_t*, unsigned char*, int );
int           hl_hmatch( hlHashEl_t*, unsigned char*, int );
void          hl_hdel( hlHashTable_t*, unsigned char*, int );

/* the maximum index in the primes array
 * this actually needs to be lowered substantially
 * on 32-bit machines
 */
#define HL_HMAX 28

/* string hash function */
unsigned hl_hsax( unsigned char* k, int l ){
  unsigned h = 0;
  int i;
  for( i = 0; i < l; i++ )
    h ^= (h << 5) + (h >> 2) + k[i];
  return h;
}

/* create a new hash table */
hlHashTable_t hl_hinit( hlState_t* s ){
  hlHashTable_t h;
  h.c = 0;
  h.f = 0;
  h.s = 0; /* index in the prime table */
  h.t = hl_malloc(s, hlhprimes[0] * sizeof(hlHashEl_t));
  h.state = s;
  return h;
} 

/* create a hash table node */
hlHashEl_t hl_hinitnode( unsigned char* k, int l, void* v ){
  hlHashEl_t n;
  if( l < HL_PTR_SIZE ){
    memcpy(n.k.skey, k, l);
  } else {
    n.k.lkey = k;
  }
  n.l = l;
  n.v = v;
  n.h = hl_hsax(k, l);
  return n;
}

/* check if a node matches a key */
int hl_hmatch( hlHashEl_t* n, unsigned char* k, int l ){
  unsigned char* c;
  void* w;
  if( n->l != l ) return 0;
  if( n->l < HL_PTR_SIZE ){
    c = n->k.skey;
  } else {
    c = n->k.lkey;
  }
  return !(memcmp(c, k, l));
}

/* resize a hash table either up or down */
void hl_hresize( hlHashTable_t* h, int dir ){
  hlHashEl_t* t = h->t;
  int slot, idx;
  unsigned long ns, i = 0, j, s = hlhprimes[h->s];
  h->s += dir;
  ns = hlhprimes[h->s];
  h->t = hl_malloc(h->state, hlhprimes[h->s] * sizeof(hlHashEl_t));
  for( ; i < s; i++ ){ /* for each node in the old array */
    if( !t[i].l )
      continue;
    slot = t[i].h % ns;
    for( j = 0; j < ns; j++ ){ /* insert into the new array */
      idx = (slot + j * j) % ns; /* probe for new slot */
      if( h->t[idx].l )
        continue;
      h->t[idx] = t[i];
      break;
    }
  }
  free(t);
}

/* add entry to the hash table */
void hl_hset( hlHashTable_t* h, unsigned char* k, int l, void* v ){
  int idx;
  hlHashEl_t n = hl_hinitnode(k, l, v);
  unsigned long i, s = hlhprimes[h->s];
  unsigned slot = n.h % s;
  if( h->f ) return; /* table is full */
  if( !l ) return;
  if( !h->t[slot].l ){
    h->t[slot] = n;
    h->c++;
  } else {
    for( i = 0; i < s; i++ ){
      idx = (slot + i * i) % s;
      if( h->t[idx].l )
        continue;
      h->t[idx] = n;
      h->c++;
      break;
    }
  }
  if( h->c >= (s/2) ){
    if( h->s == HL_HMAX ){
      h->f = 1;
      return;
    }
    hl_hresize(h, 1);
  }
}

/* find the slot of the node */
int hl_hget( hlHashTable_t* h, unsigned char* k, int l ){
  int idx;
  unsigned long i, s = hlhprimes[h->s];
  unsigned slot = hl_hsax(k, l) % s;
  if( hl_hmatch(&(h->t[slot]), k, l) )
    return slot;
  for( i = 0; i < s; i++ ){
    idx = (slot + i * i) % s;
    if( hl_hmatch(&(h->t[idx]), k, l) )
      return idx;
  }
  return -1;
}

/* remove an item from the table */
void hl_hdel( hlHashTable_t* h, unsigned char* k, int l ){
  int i = hl_hget(h, k, l);
  unsigned long s = hlhprimes[h->s];
  if( i == -1 ) return;  
  /* possibly free the key */
  memset(&(h->t[i]), 0, sizeof(hlHashEl_t));
  h->c--;
  h->f = 0; /* table is no longer full */
  if( h->c < s/4 ){
    if( h->s == 0 ) return;
    hl_hresize(h, -1);
  }
}

/*
 * end hash table
 */ 

/*
 * Values and Types
 */
 
#define hlObjType  1
#define hlArrType  2
#define hlStrType  3
#define hlBoolType 4
#define hlNumType  5

typedef unsigned char*    hlString_t;
typedef double            hlNum_t;
typedef unsigned char     hlBool_t;
typedef struct _hlValue_t hlValue_t;

typedef struct {
  /* maybe meta data */
  hlHashTable_t* h;
} hlObject_t;

typedef struct {
  /* maybe meta data, probably not */
  hlValue_t* v; /* index 0 will contain length */
} hlArray_t;

struct _hvValue_t {
  int t;
  union {
    hlString_t s;
    hlNum_t    n;
    hlBool_t   b;
    hlObject_t o;
    hlArray_t  a;
  } v;
};
 
/*
 * a bunch of tests for data structures 
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
  int i = 0, total = 0;
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
  int i;
  double end, start;
  hlHashTable_t table = hl_hinit(&state);

  start = (float)clock()/CLOCKS_PER_SEC; 
  loadWords(&table);
  end = (float)clock()/CLOCKS_PER_SEC;
    
  printf("Inserted, retrieved, and deleted %d objects in %fs - Table size: %d\n", table.c, end - start, hlhprimes[table.s]);
}

int main(void) {
  hashtest();
  return 0;
}
