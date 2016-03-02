#ifndef _HOLLY_H
#define _HOLLY_H
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
 * System wide definitions
 */

#define HL_PTR_SIZE 8 /* bytes in a word */

#define hl_eabort(s) do { if( s->error ) return; } while( 0 )
#define hl_eabortr(s, r) do { if( s->error ) return r; } while( 0 )

/* error codes */
#define HL_MALLOC_FAIL 1
#define HL_LEXSTR_ERR  2

static const char* hlErrors[] = {
  NULL,
  "malloc failure",
  "Lex error: incomplete string",
};

/* forward declaration */
typedef struct _hlState_t hlState_t;

void* hl_malloc( hlState_t*, int );

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


static const int hlhprimes[] = {
  5, 11, 23, 47, 97, 197, 397, 797, 1597, 3203, 6421, 
  12853, 25717, 51437, 102877, 205759, 411527, 823117, 
  1646237, 3292489, 6584983, 13169977, 26339969, 52679969, 
  105359939, 210719881, 421439783, 842879579, 1685759167
};


/* hash table api */
hlHashTable_t hl_hinit( hlState_t*  );
void          hl_hset( hlHashTable_t*, unsigned char*, int, void* );
int           hl_hget( hlHashTable_t*, unsigned char*, int );
void          hl_hdel( hlHashTable_t*, unsigned char*, int );

/*
 * Values and Types
 * Arithmetic functions are declared here as well as some primitive functions
 *   like array, string and object access and manipulation
 * Other functions belong in the stdlib via the embedding api
 */

typedef double            hlNum_t;
typedef unsigned char     hlBool_t;
typedef struct _hlValue_t hlValue_t;

typedef struct {
  int l;
  union {
    unsigned char  s[HL_PTR_SIZE]; 
    unsigned char* l;
  } str;
} hlString_t;

typedef struct {
  /* maybe meta data */
  hlHashTable_t h;
} hlObject_t;

typedef struct {
  /* maybe meta data, probably not */
  /* if definitely not, ditch the box */
  hlValue_t* v; /* index 0 will contain length */
} hlArray_t;

struct _hlValue_t {
  int t;
  union {
    hlString_t* s; /* objects larger than 8 bytes are references */
    hlNum_t     n;
    hlBool_t    b;
    hlObject_t* o; 
    hlArray_t   a;
  } v;
};

/*
 * Token Data
 */
 
/*
types: String, Number, Object, Array, Boolean
binary operators: | - + * ^ / % >> << & 
assignment: |= -= += *= ^= /= %= >>= <<= &= =
unary operator: ! ~ * -
comparison: < > == != <= >= and or
reserved symbols: \ { } [ ] : ( ) ; , " ' -- :: .. . ->
other reserved words: let if else return while fn true false nil for in break
*/

typedef enum {
  tk_str,
  tk_num, 
  tk_object,
  tk_array,
  tk_bool,
  tk_oeq,
  tk_meq,
  tk_peq,
  tk_teq,
  tk_xeq,
  tk_deq,
  tk_modeq,
  tk_rseq,
  tk_lseq,
  tk_aeq,
  tk_leq,
  tk_geq,
  tk_and,
  tk_or,
  tk_ls,
  tk_rs,
  tk_esc,
  tk_lbrc,
  tk_rbrc,
  tk_lbrk,
  tk_rbrk,
  tk_col,
  tk_lp,
  tk_rp,
  tk_sem,
  tk_com,
  tk_dcol,
  tk_spr,
  tk_per,
  tk_arrow,
  tk_not,
  tk_lnot,
  tk_ast,
  tk_lor,
  tk_sub,
  tk_add,
  tk_xor,
  tk_div,
  tk_mod,
  tk_gt,
  tk_lt,
  tk_land,
  tk_iseq,
  tk_eq,
  tk_let,
  tk_if,
  tk_else,
  tk_return,
  tk_while,
  tk_fn,
  tk_true,
  tk_false,
  tk_nil,
  tk_for,
  tk_in,
  tk_break,
  tk_string,
  tk_number,
  tk_boolean,
  tk_name,
  tk_eof
} token;

typedef struct {
  token type;
  int  l;
  union {
    hlNum_t number;
    unsigned char* data;
  } data;
} hlToken_t;

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

/*
 * Compiler State
 */

struct _hlState_t {
  int            error;
  int            ptr;
  hlToken_t      ctok;
  unsigned char* prog;
};

/* temporary (eventually make static) */
void hl_pnext( hlState_t* );

#endif
