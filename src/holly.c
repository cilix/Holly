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

static void hl_error( hlState_t* s, const char* e, const char* a ){
  /* display line number also */
  if( a ) fprintf(stderr, "error: %s %s\n", e, a);
  else fprintf(stderr, "error: %s\n", e);
  s->error = 1;
}

void* hl_malloc( hlState_t* h, int s ){
  void* buf;
  hl_eabortr(h, NULL);
  if( !(buf = malloc(s)) ){
    hl_error(h, "malloc failure\n", NULL);
  } else {
    memset(buf, 0, s);
  }
  return buf;
}

enum {
  OP_PUSHVAL,
  OP_ADD,
  OP_SUB,
  OP_MULT,
  OP_DIV,
  OP_JMP,
  OP_JMPF,
  OP_JMPT,
  OP_CALL,
  OP_EXIT,
  OP_LOG, /* temporary */
  OP_POP,
  OP_SLOCAL,
  OP_GLOCAL
};

enum {
  numtype,
  strtype,
  booltype,
  objtype,
  arraytype,
  functype,
  niltype
};

static void ipush( hlState_t* h, int op, int arg ){
  h->fs->ins[h->fs->ip++] = (op << 16) | arg;
  /* realloc here */
}

static void adjustarg( hlState_t* h, int off, int arg ){
  h->fs->ins[off] &= 0xffff0000;
  h->fs->ins[off] |= arg;
}

/* value stack */
static int vpushbool( hlState_t* h, hlBool_t b ){
  int i = h->vp++;
  hlValue_t v;
  v.v.b = b;
  v.t = booltype;
  h->vstack[i] = v;
  return i;
}

static int vpushstr( hlState_t* h, unsigned char* s, int l ){
  int i = h->vp++;
  hlValue_t v;
  hlString_t* c = hl_malloc(h, sizeof(hlString_t));
  if( l <= HL_PTR_SIZE ){
    memcpy(c->str.s, s, l);
    free(s);
  } else {
    c->str.l = s;
  }
  c->l = l;
  v.v.s = c;
  v.t = strtype;
  h->vstack[i] = v;
  return i;
}

static int vpushnum( hlState_t* h, hlNum_t n ){
  int i = h->vp++;
  hlValue_t v;
  v.v.n = n;
  v.t = numtype;
  h->vstack[i] = v;
  return i;
}

static int vpushfunc( hlState_t* h, hlFunc_t* f ){
  int i = h->vp++;
  hlValue_t v;
  v.v.f = f;
  v.t = functype;
  h->vstack[i] = v;
  return i;
}

static int vpushnil( hlState_t* h ){
  int i = h->vp++;
  hlValue_t v;
  v.v.n = 0;
  v.t = niltype;
  h->vstack[i] = v;
  return i;
}

static hlFunc_t* funcstate( hlState_t* h ){
  hlFunc_t* f = hl_malloc(h, sizeof(hlFunc_t));
  if( !f ) return NULL;
  f->estack = hl_malloc(h, 100 * sizeof(hlValue_t));
  f->ins = hl_malloc(h, 100 * sizeof(unsigned));
  f->locals = hl_hinit(h);
  f->ep = 0;
  f->ip = 0;
  f->state = h;
  f->scan = 0;
  f->env = NULL;
  return f;
}

void hl_init( hlState_t* h ){
  h->error = 0;
  h->vstack = hl_malloc(h, 100 * sizeof(hlValue_t));
  h->global = funcstate(h);
  h->fs = h->global;
  h->ctok.type = -1;
  h->ptr = 0;
  h->vp = 0;
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

typedef enum {
  tk_oeq,    tk_meq,    tk_peq,    tk_teq,     tk_xeq,    tk_deq,    
  tk_modeq,  tk_rseq,   tk_lseq,   tk_aeq,     tk_leq,    tk_geq,        
  tk_ls,     tk_rs,     tk_esc,    tk_lbrc,   
  tk_rbrc,   tk_lbrk,   tk_rbrk,   tk_col,     tk_lp,     tk_rp,     
  tk_sem,    tk_com,    tk_dcol,   tk_spr,     tk_per,    tk_arrow,  
  tk_not,    tk_lnot,   tk_ast,    tk_bor,     tk_sub,    tk_add,    
  tk_xor,    tk_div,    tk_mod,    tk_gt,      tk_lt,     tk_band,   
  tk_iseq,   tk_eq,     tk_let,    tk_if,      tk_else,   tk_return, 
  tk_while,  tk_fn,     tk_true,   tk_false,   tk_nil,    tk_for,    
  tk_in,     tk_break,  tk_land,   tk_lor,     tk_str,    tk_num,     
  tk_object, tk_array,  tk_bool,   tk_function, tk_Nil,   tk_log, /* temporary */
  tk_string,  tk_number, tk_boolean,
  tk_name,   tk_eof
} token;

static const int tkSymCnt = 42;
static const int tkCnt = 64;

static const char* hlTkns[] = {
  "|=", "-=", "+=", "*=", "^=", "/=", "%=", ">>=",
  "<<=", "&=", "<=", ">=", "<<", ">>",
  "\\", "{", "}", "[", "]", ":", "(", ")", ";", ",",
  "::", "..", ".", "->", "!", "~", "*", "|", "-",
  "+", "^", "/", "%", ">", "<", "&", "==", "=", "let",
  "if", "else", "return", "while", "fn", "true",
  "false", "nil", "for", "in", "break", "and", "or",
  "String", "Number", "Object", "Array", 
  "Boolean", "Function", "Nil", "log",
  "<string>", "<number>", "<boolean>", "<name>", "<eof>"
};

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

static unsigned char pesc( unsigned char* p, int* i ){
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
      c = (l << 4) | r;
      (*i) += 2; 
    } break;
    default: c = 0; break;
  }
  (*i)++;
  return c;
}

static unsigned char* pstring( hlState_t* s, unsigned char* v, int l ){
  int i = 0, j = 0;
  unsigned char c, e;
  unsigned char* b = hl_malloc(s, l + 1);
  if( !b ) return NULL;
  while( i < l ){
    c = v[i++];
    if( c == '\\' ){
      e = pesc(v + i, &i);
      if( e ) b[j++] = e;
    } else {
      b[j++] = c;
    }
  }
  return b;
}

static unsigned char* pname( hlState_t* s, unsigned char* v ){
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

static int pow( int b, int e ){
  int r = 1;
  while( e ){
    if( e & 1 ) r *= b;
    e >>= 1;
    b *= b;
  }
  return r;
}

static void pnumber( hlState_t* s, unsigned char* p, int x ){
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
    dec /= pow(10, dc);
    r += dec;
  }
  s->ctok.data.number = r;
  s->ptr += i;
  s->ctok.type = tk_number;
}

static int isreserved( hlState_t* s, unsigned char* str, unsigned l ){
  int i;
  for( i = tkSymCnt; i < tkCnt; i++ ){
    if( 
      l != strlen(hlTkns[i]) || 
      !hl_ismatch(str, hlTkns[i], l) 
    ) continue;
    if( i == tk_true || i == tk_false ){
      s->ctok.type = tk_boolean; 
      s->ctok.data.number = (i == tk_true); 
    } else {
      s->ctok.type = i;
    }
    s->ptr += l;
    return 1;
  }
  return 0;
}

static void next( hlState_t* s ){
  int i, x;
  unsigned char* p = s->prog;
  hl_eabort(s);
  while( hl_isspace(p[s->ptr]) ) s->ptr++; 
  /* inline comments start with -- */
  if( hl_ismatch(p + s->ptr, "--", 2) ){
    s->ptr += 2;
    while( p[s->ptr] != '\n' && p[s->ptr] != 0 ) s->ptr++; 
    while( hl_isspace(p[s->ptr]) ) s->ptr++; 
  }
   /* block comments start with /- and end with -/ */
  if( hl_ismatch(p + s->ptr, "/-", 2) ){
    s->ptr += 2;
    while( !hl_ismatch(p + s->ptr, "-/", 2) && p[s->ptr] != 0 ) s->ptr++; 
    s->ptr += 2;
    while( hl_isspace(p[s->ptr]) ) s->ptr++;
  }
  x = s->ptr;
  if( !p[x] ){
    s->ctok.type = tk_eof;
    return;
  }
  /* check for symbols */
  for( i = 0; i < tkSymCnt; i++ ){
    int l = strlen(hlTkns[i]);
    if( hl_ismatch(p + x, hlTkns[i], l) ){
      s->ctok.type = i;
      s->ptr += l;
      return;
    }
  }
  switch( p[x] ){
    case 0:
      s->ctok.type = 0;
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
        s->error = 1;
        return;
      }
      str = pstring(s, p + x + 1, i - 1);
      if( str ){
        s->ctok.type = tk_string; 
        s->ctok.data.data = str;
        s->ctok.l = i - 1;
        s->ptr += (i + 1);
      }
      return;
    } break;
    default: {
      if( hl_isalpha(p[x]) ){
        unsigned char* str;
        unsigned l = 0;
        s->ctok.type = tk_name; /* name tok */
        str = pname(s, p + x);
        if( !str ){
          /* some error happened */
          return;
        }
        l = strlen((const char *)str);
        if( !isreserved(s, str, l) ){
          s->ctok.data.data = str;
          s->ptr += l;
          s->ctok.l = l;
        }
      } else if( hl_isdigit(p[x]) ){
        pnumber(s, p, x);
      }
    } return;
  }
  s->error = 1;
  fprintf(stderr, "unexpected %c\n", p[x]);
}

static int peek( hlState_t* s, token t ){
  hl_eabortr(s, 0);
  return s->ctok.type == t;
}

static int accept( hlState_t* s, token t ){
  hl_eabortr(s, 0);
  if( s->ctok.type == t ){
    next(s);
    return 1;
  }
  return 0;
}

static int expect( hlState_t* s, token t ){
  hl_eabortr(s, 0);
  if( s->ctok.type == t ){
    next(s);
    return 1;
  }
  fprintf(stderr, "expected %s near %s\n", hlTkns[t], hlTkns[s->ctok.type]);
  s->error = 1;
  return 0;
}

static void namelist( hlState_t* );
static void expressionlist( hlState_t* );
static void pairlist( hlState_t* );
static void expression( hlState_t* );
static void ifstatement( hlState_t* );
static void elsestatement( hlState_t* );
static void statementlist( hlState_t* );
static void valuesuffix( hlState_t* );
static void statement( hlState_t* );

static int unop( hlState_t* s ){
  token tok = s->ctok.type;
  return tok == tk_not ||
    tok == tk_lnot     ||
    tok == tk_band     ||
    tok == tk_ast;
}

static int assignment( hlState_t* s ){
  token tok = s->ctok.type;
  return tok == tk_oeq ||
    tok == tk_meq      ||
    tok == tk_peq      ||
    tok == tk_teq      ||
    tok == tk_xeq      ||
    tok == tk_deq      ||
    tok == tk_modeq    ||
    tok == tk_rseq     ||
    tok == tk_lseq     ||
    tok == tk_aeq      ||
    tok == tk_leq      ||
    tok == tk_eq       ||
    tok == tk_geq;
}

static int binop( hlState_t* s ){
  token tok = s->ctok.type;
  return tok == tk_land ||
    tok == tk_lor       ||
    tok == tk_ls        ||
    tok == tk_rs        ||
    tok == tk_bor       ||
    tok == tk_sub       ||
    tok == tk_add       ||
    tok == tk_xor       ||
    tok == tk_div       ||
    tok == tk_mod       ||
    tok == tk_gt        ||
    tok == tk_lt        ||
    tok == tk_band      ||
    tok == tk_ast       ||
    tok == tk_iseq;
}

/*
block ::=
  `{` statementlist `}`
*/

static void block( hlState_t* s ){
  int i;
  hlFunc_t* state = s->fs;
  hlFunc_t* b = funcstate(s);
  b->env = state;
  hl_eabort(s);
  s->fs = b;
  expect(s, tk_lbrc);
  statementlist(s);
  expect(s, tk_rbrc);
  s->fs = state;
  i = vpushfunc(s, b);
  ipush(s, OP_PUSHVAL, i);
}

/*
expressionlist ::=
  expression |
  expression `,` expressionlist 
*/

static void expressionlist( hlState_t* s ){
expression_list:
  hl_eabort(s);
  expression(s);
  if( accept(s, tk_com) ){
    goto expression_list;
  }
}

/*
spread ::=
  Number `..` Number  
*/
/*
static void spread( hlState_t* s ){
  hl_eabort(s);
  expect(s, tk_number);
  expect(s, tk_spr);
  expect(s, tk_number);
}
*/
/*
object ::=
  `{` pairlist `}`
*/

static void object( hlState_t* s ){
  hl_eabort(s);
  expect(s, tk_lbrc);
  pairlist(s);
  expect(s, tk_rbrc);
}

/*
array ::=
  `[` expressionlist `]` | `[` nil `]`
*/

static void array( hlState_t* s ){
  hl_eabort(s);
  expect(s, tk_lbrk);
  if( accept(s, tk_rbrk) ){
    return;
  }
  expressionlist(s);
  expect(s, tk_rbrk);
}

/*
pairlist ::=
  Name `:` expression |
  Name `:` expression `,` pairlist |
  nil
*/

static void pairlist( hlState_t* s ){
pair_list:
  hl_eabort(s);
  if( accept(s, tk_name) ){
    expect(s, tk_col);
    expression(s);
    if( accept(s, tk_com) ){
      goto pair_list;
    }
  }
}

/*
name ::= 
  Name |
  Name typehint
*/

static void name( hlState_t* s ){
  hl_eabort(s);
  expect(s, tk_name);
  if( accept(s, tk_col) ){
    if( accept(s, tk_str)      ||   
        accept(s, tk_num)      ||
        accept(s, tk_object)   ||
        accept(s, tk_array)    ||
        accept(s, tk_function) ||
        accept(s, tk_Nil)      ||
        accept(s, tk_bool) ){
      return;
    } else {
      hl_error(s, "expected", "type declaration");
    }
  };
}


/*
namelist ::=
  name |
  name `,` namelist |
  nil
*/

static void namelist( hlState_t* s ){
name_list:
  hl_eabort(s);
  name(s);
  if( accept(s, tk_com) ){
    goto name_list;
  }
}

/*
lambda ::=
  `fn` namelist block | 
  `fn` namelist `->` expression 
*/

static void lambda( hlState_t* s ){
  hl_eabort(s);
  expect(s, tk_fn);
  namelist(s);
  if( accept(s, tk_arrow) ){
    expression(s);
  } else {
    block(s);
  }
}


/*
value ::=
  object |
  array |
  lambda |
  Name valuesuffix 
*/

static void value( hlState_t* s ){
  hl_eabort(s);
  if( peek(s, tk_lbrc) ){
    object(s);
  } else if( peek(s, tk_lbrk) ){
    array(s);
  } else if( peek(s, tk_fn) ){
    lambda(s);
  } else {
    int i, l = s->ctok.l;
    unsigned char* n = s->ctok.data.data;
    expect(s, tk_name);
    i = vpushstr(s, n, l);
    ipush(s, OP_GLOCAL, i);
    valuesuffix(s);
  }
}

/*
expression ::=
  literal |
  value |
  unop expression |
  expression binop expression |
  `(` expression `)` 
*/

static void expression( hlState_t* s ){
  /* this doesn't handle precedence yet, 
     and everything is right assosiative */
  hl_eabort(s);
  if( accept(s, tk_string) ){
    int i = vpushstr(s, s->ctok.data.data, s->ctok.l);
    ipush(s, OP_PUSHVAL, i);
  } else if( accept(s, tk_number) ){
    int i = vpushnum(s, s->ctok.data.number);
    ipush(s, OP_PUSHVAL, i);
  } else if( accept(s, tk_boolean) ){
    int i = vpushbool(s, s->ctok.data.number);
    ipush(s, OP_PUSHVAL, i);
  } else if( accept(s, tk_nil) ){
    ipush(s, OP_PUSHVAL, vpushnil(s));
  } else if( unop(s) ){
    next(s);
    expression(s);
  } else if( accept(s, tk_lp) ){
    expression(s);
    expect(s, tk_rp);
  } else {
    value(s);
  }
  if( binop(s) ){
    int op = 0;
    switch( s->ctok.type ){
      case tk_sub: op = OP_SUB; break;
      case tk_add: op = OP_ADD; break;
      case tk_div: op = OP_DIV; break;
      case tk_ast: op = OP_MULT; break;
      default: break;
    }
    next(s);
    expression(s);
    ipush(s, op, 0);
  }
}

/*
valuesuffix ::=
  `.` Name `(` expressionlist `)` valuesuffix |
  `:` Name valuesuffix |
  `[` expression `]` valuesuffix |  
  `(` expressionlist `)` valuesuffix |
  nil
*/

static void valuesuffix( hlState_t* s ){
value_suffix:
  hl_eabort(s);
  if( accept(s, tk_per) ){
    expect(s, tk_name);
    expect(s, tk_lp);
    if( !accept(s, tk_rp) ){
      expressionlist(s);
      expect(s, tk_rp);
    }
    goto value_suffix;
  } else if( accept(s, tk_col) ){
    expect(s, tk_name);
    goto value_suffix;
  } else if( accept(s, tk_lbrk) ){
    expression(s);
    expect(s, tk_rbrk);
    goto value_suffix;
  } else if( accept(s, tk_lp) ){
    if( !accept(s, tk_rp) ){
      expressionlist(s);
      expect(s, tk_rp);
    }
    goto value_suffix;
  }
}

/*
ifstatement ::=
  `if` expression block elsestatement |
  `if` expression statement elsestatement
*/

static void ifstatement( hlState_t* s ){
  int ip;
  hlFunc_t* state = s->fs;
  hl_eabort(s);
  expect(s, tk_if);
  expression(s);
  ipush(s, OP_JMPF, 0);
  ip = state->ip - 1;
  if( peek(s, tk_lbrc) ){
    block(s);
    ipush(s, OP_CALL, 0);
  } else {
    statement(s);
  }
  adjustarg(s, ip, state->ip - ip);
  elsestatement(s);
}

/*
elsestatement ::=
  `else` block |
  `else` statement |
  `else` ifstatement |
  nil
*/

static void elsestatement( hlState_t* s ){
  int ip;
  hlFunc_t* state = s->fs;
  hl_eabort(s);
  if( accept(s, tk_else) ){
    ipush(s, OP_JMPT, 0);
    ip = state->ip - 1;
    if( peek(s, tk_if) ){
      ifstatement(s);
    } else if( peek(s, tk_lbrc) ){
      block(s);
      ipush(s, OP_CALL, 0);
    } else {
      statement(s);
    }
    adjustarg(s, ip, state->ip - ip);
  }
}

/* 
whilestatement ::=
  `while` expression block |
  `while` expression statement
*/

static void whilestatement( hlState_t* s ){
  int ip, jmp;
  hlFunc_t* state = s->fs;
  hl_eabort(s);
  expect(s, tk_while);
  jmp = state->ip;
  expression(s);
  ipush(s, OP_JMPF, 0);
  ip = state->ip - 1;
  if( peek(s, tk_lbrc) ){
    block(s);
    ipush(s, OP_CALL, 0);
  } else {
    statement(s);
  }
  ipush(s, OP_JMP, jmp);
  adjustarg(s, ip, state->ip - ip);
}

/*
forstatement ::=
  `for` Name [ `,` Name ] `in` iterable block |         
  `for` Name [ `,` Name ] `in` iterable statement 
*/

static void forstatement( hlState_t* s ){
  hl_eabort(s);
  expect(s, tk_for);
  expect(s, tk_name);
  if( accept(s, tk_com) ){
    expect(s, tk_name);
  }
  expect(s, tk_in);
  value(s);
  if( peek(s, tk_lbrk) ){
    block(s);
  } else {
    statement(s);
  }
}

/*
functionstatement ::=
  `fn` Name namelist block | 
  `fn` Name namelist `->` expression 
*/

static void functionstatement( hlState_t* s ){
  hl_eabort(s);
  expect(s, tk_fn);
  expect(s, tk_name);
  namelist(s);
  if( accept(s, tk_arrow) ){
    expression(s);
  } else {
    block(s);
  }
}

/*
statement ::=
  ifstatement |
  whilestatement |
  forstatement |
  `return` expression |
  `break` |
  `let` Name [ assignment expression ] |
  functionstatement |                         *sugar for assigning a lambda to a variable
  Name valuesuffix  assignment expression | 
  functioncall
*/

static void statement( hlState_t* s ){
  hl_eabort(s);
  if( peek(s, tk_if) ){
    ifstatement(s);
  } else if( peek(s, tk_while) ){
    whilestatement(s);
  } else if( peek(s, tk_for) ){
    forstatement(s);
  } else if( accept(s, tk_return) ){
    expression(s);
  } else if( peek(s, tk_break) ){
    return;
  } else if( accept(s, tk_let) ){
    int i, l = s->ctok.l;
    unsigned char* n = s->ctok.data.data;
    expect(s, tk_name);
    if( s->ctok.type == tk_eq ){
      next(s);
      expression(s);
    } else {
      ipush(s, OP_PUSHVAL, vpushnil(s));
    }
    i = vpushstr(s, n, l);
    ipush(s, OP_SLOCAL, i);
  } else if( peek(s, tk_fn) ){
    functionstatement(s);
  } else if( peek(s, tk_name) ){
    value(s);
    if( assignment(s) ){
      next(s);
      expression(s);
    } else {
      /* must be a functioncall */
    }
  } else if( accept(s, tk_log) ){
    expression(s);
    ipush(s, OP_LOG, 0);
  } else {
    hl_error(s, "unexpected", hlTkns[s->ctok.type]);
  }
}

/*                              
statementlist ::=
  statement statementlist | 
  nil
*/

static void statementlist( hlState_t* s ){
statement_list:
  hl_eabort(s);
  if( accept(s, tk_eof) || peek(s, tk_rbrc)){
    return;
  }
  statement(s);
  goto statement_list;
}

/*
start ::=
  statementlist
*/

void hl_pstart( hlState_t* s ){
  hl_eabort(s);
  next(s);
  statementlist(s);
  ipush(s, OP_EXIT, 0);
}

#define pop(x) x->estack[--(x->ep)]
#define top(x) x->estack[(x->ep)++]

#define getop(x)  x->ins[x->scan] >> 16
#define getarg(x) x->ins[x->scan] & 0xffff

static hlNum_t popn( hlFunc_t* s ){
  hlValue_t* v = &(pop(s));
  hlNum_t n = 0;
  if( v->t != numtype ){
    s->state->error = 1;
    fprintf(stderr, "invalid operand\n");
  } else {
    n = v->v.n;
  }
  return n;
}

static void printstr( hlString_t* str ){
  int i = 0;
  int l = str->l;
  unsigned char* s = l > HL_PTR_SIZE ? str->str.l : str->str.s;
  for( ; i < l; i++) putchar(s[i]);
  printf("\n");
}

void hl_vrun( hlState_t* s ){
  hlFunc_t* frames[256], *f;
  int fp = 0;
  f = frames[fp] = s->global;
  hl_eabort(s);
  for( ; ; f->scan++ ){
    int op, arg;
    if( f->scan >= f->ip ){
      if( fp > 0 ){ /* unnest */
        f = frames[--fp];
        f->scan++;
      } else goto exit_vm;
    }
    op = getop(f);
    arg = getarg(f);
    switch( op ){
      case OP_LOG: {
        /* temporary */
        hlValue_t d = pop(f);
        hl_eabort(s);
        switch( d.t ){
          case 0: 
            printf("%f\n", d.v.n); 
            break;
          case 1: 
            printstr(d.v.s);
            break;
          case 2: 
            printf("%s\n", d.v.b ? "true" : "false"); 
            break;
          case 3: 
            printf("Object\n"); 
            break;
          case 4: 
            printf("Array\n"); 
            break;
          case 5: 
            printf("Function\n"); 
            break;
        }
      } break;
      case OP_POP: {
        (f->ep)--;
        /* free resources */
        break;
      }
      case OP_CALL: {
        hlFunc_t* b = pop(f).v.f;
        f = frames[++fp] = b;
        f->scan = -1; /* this will get incremented */
      } break;
      case OP_JMP: {
        f->scan += arg - 1;
      } break;
      case OP_JMPF: {
        hlNum_t b = popn(f);
        hl_eabort(s);
        if( !b ) f->scan += arg - 1;
        f->ep++;
      } break;
      case OP_JMPT: {
        hlNum_t b = popn(f);
        hl_eabort(s);
        if( b ) f->scan += arg - 1;
        f->ep++;
      } break;
      case OP_PUSHVAL: {
        top(f) = s->vstack[arg];
      } break;
      case OP_SLOCAL: {
        hlValue_t c = s->vstack[arg];
        hlValue_t v = pop(f);
        hl_hset(&f->locals, c.v.s->str.s, c.v.s->l, &v);
      } break;
      case OP_GLOCAL: {
        int i;
        hlFunc_t* state = f;
        hlValue_t c = s->vstack[arg];
        while( state ){
          i = hl_hget(&state->locals, c.v.s->str.s, c.v.s->l);
          if( i == -1 ){
            state = state->env;
          } else {
            top(f) = *(hlValue_t *)(state->locals.t[i].v);
            break;
          }
        }
        if( i == -1 ){
          s->error = 1;
          fprintf(stderr, "undeclared variable %s\n", c.v.s->str.s);
        }
      } break;
      case OP_ADD: {
        hlNum_t r = popn(f);
        hlNum_t l = popn(f);
        hl_eabort(s);
        top(f).v.n = l + r;
      } break;
      case OP_DIV: {
        hlNum_t r = popn(f);
        hlNum_t l = popn(f);
        hl_eabort(s);
        top(f).v.n = l / r;
      } break;
      case OP_SUB: {
        hlNum_t r = popn(f);
        hlNum_t l = popn(f);
        hl_eabort(s);
        top(f).v.n = l - r;
      } break;
      case OP_MULT: {
        hlNum_t r = popn(f);
        hlNum_t l = popn(f);
        hl_eabort(s);
        top(f).v.n = l * r;
      } break;
      case OP_EXIT: {
exit_vm:
        /* free resources */
        return;
      } break;
      default: break;
    }
  }
}
