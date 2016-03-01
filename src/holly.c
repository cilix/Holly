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

#include "holly.h"

void* hl_malloc(hlState_t* h, int s ){
  void* buf;
  if( !(buf = malloc(s)) ){
     h->error = HL_MALLOC_FAIL;
  } else {
     memset(buf, 0, s);
  }
  return buf;
}

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
 * Parser
 */

#define hl_ismatch(x, y, z) (!strncmp((const char *)x, (const char *)y, z))
#define hl_isspace(x)       (x == ' ' || x == '\t' || x == '\n')
#define hl_isdigit(x)       (x >= '0' && x <= '9')
#define hl_islower(x)       (x >= 'a' && x <= 'z')
#define hl_isupper(x)       (x >= 'A' && x <= 'Z')
#define hl_isalpha(x)       (hl_islower(x) || hl_isupper(x))
#define hl_ishex(x)         (x >= 'a' && x <= 'f')

#define hl_hextod(x) do { \
  if( hl_isdigit(x) ) x = x - '0'; \
  else if( hl_ishex(x) ) x = 10 + (x - 'a'); \
} while( 0 )

unsigned char hl_pesc( unsigned char* p, int* i ){
  unsigned char c = *p;
  switch( c ){
    case 'a': c = '\a'; break;
    case 'b': c = '\b'; break;
    case 'f': c = '\f'; break;
    case 'n': c = '\n'; break;
    case 'r': c = '\r'; break;
    case 't': c = '\t'; break;
    case 'v': c = '\v'; break;
    case 'x': {
      unsigned char l = *(p + 1);
      unsigned char r = *(p + 2);
      hl_hextod(l);
      hl_hextod(r);
      c = l << 4;
      c = c | r;
      (*i) += 2; 
    } break;
    default: c = 0; break;
  }
  (*i)++;
  return c;
}

static unsigned char* pString( hlState_t* s, unsigned char* v, int l ){
  int i = 0, j = 0;
  unsigned char c, e;
  unsigned char* b = hl_malloc(s, l + 1);
  if( !b ) return NULL;
  while( i < l ){
    c = v[i++];
    if( c == '\\' ){
      e = hl_pesc(v + i, &i);
      if( e ) b[j++] = e;
    } else {
      b[j++] = c;
    }
  }
  return b;
}

static unsigned char* pName( hlState_t* s, unsigned char* v ){
  int i = 0;
  unsigned char* b;
  while(
    hl_isalpha(*(v + i)) ||
    hl_isdigit(*(v + i)) ||
    (*(v + i) == '_') 
  ) i++;
  b = hl_malloc(s, i + 1);
  if( !b ) return NULL;
  memcpy(b, v, i);
  return b;
}

int hl_ppow( int b, int e ){
  int r = 1;
  while( e ){
    if( e & 1 ) r *= b;
    e >>= 1;
    b *= b;
  }
  return r;
}

void hl_pnext( hlState_t* s ){
  int i, x;
  unsigned char* p = s->prog;
  hl_eabort(s);
  /* possibly free previous token data here 
     probably not, so we can pass the data to the vm */
  s->ctok.type = -1;
  while( hl_isspace(p[s->ptr]) ) s->ptr++; 
  /* comments 
     inline comments start with -- 
     block comments start with --- and end with -- */
  if( hl_ismatch(p + s->ptr, "--", 2) ){
    s->ptr += 2;
    if( p[s->ptr] == '-' ){
      while( 
        !hl_ismatch(p + s->ptr, "--", 2) && 
        p[s->ptr] != 0
      ) s->ptr++;
      if( p[s->ptr] != 0 ) s->ptr += 2;
    } else {
      while( p[s->ptr] != '\n' && p[s->ptr] != 0 ) s->ptr++; 
    }
    while( hl_isspace(p[s->ptr]) ) s->ptr++; 
  }
  x = s->ptr;
  if( !p[x] ){
    s->ctok.type = -1;
    return;
  }
  for( i = 0; i < hlTkCnt; i++ ){
    int l = strlen(hlTkns[i]);
    /* make sure the string comparison doesn't go off 
       the end of the buffer */
    if( hl_ismatch(p + x, hlTkns[i], l) ){
      if( i == 53 || i == 54 ){
        s->ctok.type = 63; /* boolean */
        s->ctok.data.number = (i == 53); /* i == true */
      } else {
        s->ctok.type = i;
      }
      s->ptr += l;
      return;
    }
  }
  switch( p[x] ){
    case 0:
      s->ctok.type = -1;
      return;
    case '\'':
    case '"': {
      int i = 1;
      char end = p[x];
      unsigned char c, *str;
      while( 
        (c = p[x + i]) &&
        (c != end || p[x + i - 1] == '\\')
      ) i++;
      if( !c ){
        s->error = HL_LEXSTR_ERR;
        return;
      }
      str = pString(s, p + x + 1, i - 1);
      if( str ){
        s->ctok.type = 59; /* string tok */
        s->ctok.data.data = str;
        s->ptr += (i + 1);
      }
      return;
    } break;
    default: {
      if( hl_isalpha(p[x]) ){
        unsigned char* str;
        int l = 0;
        s->ctok.type = 64; /* name tok */
        str = pName(s, p + x);
        if( str ){
          l = strlen((const char *)str);
          s->ctok.data.data = str;
          s->ptr += l;
          s->ctok.l = l;
        }
        return;
      } else if( hl_isdigit(p[x]) ){
        int i = 0, df = 0, dc = 0;
        unsigned char c;
        hlNum_t r = 0.0f, dec = 0.0f;
        while( 
          (c = p[x + i]) &&
          (hl_isdigit(c) || (c == '.' && !df))
        ){
          if( c == '.' ) df = 1;
          else {
            if( !df ) r = r * 10 + (c - '0');
            else {
              dc++;
              dec = dec * 10 + (c - '0');
            }
          }
          i++;  
        }
        if( dec ){
          dec /= hl_ppow(10, dc);
          r += dec;
        }
        s->ctok.data.number = r;
        s->ptr += i + 1;
        s->ctok.type = 60; /* number */
        return;
      }
    } break;
  }
  s->ptr++;
}
