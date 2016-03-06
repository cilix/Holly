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

void* hl_malloc(hlState_t* h, int s ){
  void* buf;
  if( !(buf = malloc(s)) ){
    hl_error(h, "malloc failuer\n", NULL);
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

typedef enum {
  tk_str,   tk_num,    tk_object, tk_array,   tk_bool,   tk_oeq,
  tk_meq,   tk_peq,    tk_teq,    tk_xeq,     tk_deq,    tk_modeq,
  tk_rseq,  tk_lseq,   tk_aeq,    tk_leq,     tk_geq,    tk_land,
  tk_lor,   tk_ls,     tk_rs,     tk_esc,     tk_lbrc,   tk_rbrc,
  tk_lbrk,  tk_rbrk,   tk_col,    tk_lp,      tk_rp,     tk_sem,
  tk_com,   tk_dcol,   tk_spr,    tk_per,     tk_arrow,  tk_not,
  tk_lnot,  tk_ast,    tk_bor,    tk_sub,     tk_add,    tk_xor,
  tk_div,   tk_mod,    tk_gt,     tk_lt,      tk_band,   tk_iseq,
  tk_eq,    tk_let,    tk_if,     tk_else,    tk_return, tk_while,
  tk_fn,    tk_true,   tk_false,  tk_nil,     tk_for,    tk_in,
  tk_break, tk_string, tk_number, tk_boolean, tk_name,   tk_eof
} token;

static const int hlTkCnt = 60;

static const char* hlTkns[] = {
  "String", "Number", "Object", "Array", "Boolean",
  "|=", "-=", "+=", "*=", "^=", "/=", "%=", ">>=", 
  "<<=", "&=", "<=", ">=", "and", "or", "<<", ">>",
  "\\", "{", "}", "[", "]", ":", "(", ")", ";", ",",
  "::", "..", ".", "->", "!", "~", "*", "|", "-", 
  "+", "^", "/", "%", ">", "<", "&", "==", "=", "let", 
  "if", "else", "return", "while", "fn", "true", 
  "false", "nil", "for", "in", "break",
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

static unsigned char pEsc( unsigned char* p, int* i ){
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
      e = pEsc(v + i, &i);
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

static int pow( int b, int e ){
  int r = 1;
  while( e ){
    if( e & 1 ) r *= b;
    e >>= 1;
    b *= b;
  }
  return r;
}

static void next( hlState_t* s ){
  int i, x;
  unsigned char* p = s->prog;
  hl_eabort(s);

  /* possibly free previous token data here 
     probably not, so we can pass the data to the vm */
  if( s->ctok.type == tk_name ){
    printf("token: <name> :%s\n", s->ctok.data.data);
  } else {
    printf("token: %s\n", hlTkns[s->ctok.type]);
  }
  s->ctok.ltype = s->ctok.type;

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
    s->ctok.type = tk_eof;
    return;
  }
  /* XXX : rewrite this part completely 
     check for symbols and reserved words individually */
  for( i = 0; i < hlTkCnt; i++ ){
    int l = strlen(hlTkns[i]);
    /* make sure the string comparison doesn't go off 
       the end of the buffer */
    if( hl_ismatch(p + x, hlTkns[i], l) ){
      if( i == tk_true || i == tk_false ){
        s->ctok.type = tk_boolean; 
        s->ctok.data.number = (i == tk_true); 
      } else {
        s->ctok.type = i;
      }
      s->ptr += l;
      return;
    }
  }
  switch( p[x] ){
    case 0:
      s->ctok.type = 0;
      return;
    /* check for symbols here */
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
      str = pString(s, p + x + 1, i - 1);
      if( str ){
        s->ctok.type = tk_string; 
        s->ctok.data.data = str;
        s->ptr += (i + 1);
      }
      return;
    } break;
    default: {
      if( hl_isalpha(p[x]) ){
        unsigned char* str;
        int l = 0;
        s->ctok.type = tk_name; /* name tok */
        str = pName(s, p + x);
        if( str ){
          /* check for reserved words here */
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
          dec /= pow(10, dc);
          r += dec;
        }
        s->ctok.data.number = r;
        s->ptr += i;
        s->ctok.type = tk_number; 
        return;
      }
    } break;
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
  hl_error(s, "unexpected token", hlTkns[s->ctok.type]);
  fprintf(stderr, "expected %s\n", hlTkns[t]);
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
    tok == tk_iseq;
}

/*
block ::=
  `{` statementlist `}`
*/

static void block( hlState_t* s ){
  hl_eabort(s);
  expect(s, tk_lbrc);
  statementlist(s);
  expect(s, tk_rbrc);
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

static void spread( hlState_t* s ){
  hl_eabort(s);
  expect(s, tk_number);
  expect(s, tk_spr);
  expect(s, tk_number);
}

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
    accept(s, tk_str)    ||   
    accept(s, tk_num)    ||
    accept(s, tk_object) ||
    accept(s, tk_array)  ||
    expect(s, tk_bool);
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
    expect(s, tk_name);
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
    /* emit */
  } else if( accept(s, tk_number) ){
    /* emit */
  } else if( accept(s, tk_boolean) ){
    /* emit */
  } else if( accept(s, tk_nil) ){
    /* emit */
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
    next(s);
    expression(s);
  }
}

/*
valuesuffix ::=
  `.` Name valuesuffix |
  `[` expression `]` valuesuffix |  
  `(` expressionlist `)` valuesuffix |
  nil
*/

static void valuesuffix( hlState_t* s ){
value_suffix:
  hl_eabort(s);
  if( accept(s, tk_per) ){
    expect(s, tk_name);
    goto value_suffix;
  } else if( accept(s, tk_lbrk) ){
    expression(s);
    expect(s, tk_rbrk);
    goto value_suffix;
  } else if( accept(s, tk_lp) ){
    expressionlist(s);
    expect(s, tk_rp);
    goto value_suffix;
  }
}

/*
ifstatement ::=
  `if` expression block elsestatement |
  `if` expression statement elsestatement
*/

static void ifstatement( hlState_t* s ){
  hl_eabort(s);
  expect(s, tk_if);
  expression(s);
  if( peek(s, tk_lbrc) ){
    block(s);
    elsestatement(s);
  } else {
    statement(s);
  }
}

/*
elsestatement ::=
  `else` block |
  `else` statement |
  `else` ifstatement |
  nil
*/

static void elsestatement( hlState_t* s ){
  hl_eabort(s);
  if( accept(s, tk_else) ){
    if( peek(s, tk_lbrc) ){
      block(s);
    } else if( peek(s, tk_if) ){
      ifstatement(s);
    } else {
      statement(s);
    }
  }
}

/* 
whilestatement ::=
  `while` expression block |
  `while` expression statement
*/

static void whilestatement( hlState_t* s ){
  hl_eabort(s);
  expect(s, tk_while);
  expression(s);
  if( peek(s, tk_lbrc) ){
    block(s);
  } else {
    statement(s);
  }
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
    expect(s, tk_name);
    if( assignment(s) ){
      next(s);
      expression(s);
    }
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
}
