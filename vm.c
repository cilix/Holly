#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "vm.h"
#include "util.h"

static const char* op_debug[] = {
  "NOOP",
  "OP_BBLOCK",
  "OP_EBLOCK",
  "OP_PUSHINT",
  "OP_PUSHVAL",
  "OP_PUSHLVAL", 
  "OP_PUSHNIL",
  "OP_REPVAL",
  "OP_STORE",
  "OP_BIF",
  "OP_EIF",
  "OP_BELSE",
  "OP_EELSE",
  "OP_BWHILE",
  "OP_EWHILE",
  "OP_PUSHSTR",
  "OP_PUSHTRUE",
  "OP_PUSHFALSE",
  "OP_PUSHCHAR",
  "OP_OBJINIT",
  "OP_OBJEND",
  "OP_ARRAYINIT",
  "OP_ARRAYEND",
  "OP_OBJSET",
  "OP_ARRAYSET",
  "OP_RET",
  "OP_EEXP",
  "OP_BFCALL",
  "OP_EFCALL",
  "OP_OACCESS",
  "OP_BGACCESS",
  "OP_EGACCESS",
  "OP_BFDEF",
  "OP_EFDEF",
  "OP_BFNDEF",
  "OP_EFNDEF",
  "OP_IFTRUE",
  "OP_LOR",
  "OP_SUB",
  "OP_ADD",
  "OP_MULT",
  "OP_LXOR",
  "OP_DIV",
  "OP_MOD",
  "OP_RSHIFT",
  "OP_LSHIFT",
  "OP_GT",
  "OP_LT",
  "OP_ISEQ",
  "OP_ISNEQ",
  "OP_OR",
  "OP_AND",
  "OP_LAND",
  "OP_NOT",
  "OP_LNOT",
  "OP_NEG",
  "OP_CNCT"
};

#define T_INT  0x1
#define T_STR  0x2
#define T_BOOL 0x3
#define T_OBJ  0x4
#define T_ARR  0x5
#define T_CHAR 0x6

#define gettype(o) (o >> 61)
#define getval(o) ((o << 3) >> 3)

#define binop(op) do {     \
  word_t r = pop(v);       \
  word_t l = pop(v);       \
  int tr = gettype(r);     \
  int tl = gettype(l);     \
  if( tr != tl ){          \
    err = 1;               \
    puts("Error: type mismatch"); \
    break; \
  }                        \
  r = getval(r);           \
  l = getval(l);           \
  push(v, (word_t)(op), (word_t)tr); \
} while( 0 )

void vm_init( struct __vm* v ){
  v->s = 100;
  v->ins = malloc(sizeof(word_t) * 100);
  v->stack = malloc(sizeof(word_t) * 100);
  v->ct = v->sct = 0;
  v->sym = hashinit();
  memset(v->ins, 0, 100);
  memset(v->stack, 0, 100);
}

void vm_ins( struct __vm* v, enum op o, unsigned long a ){
  int x = v->ct++;
  word_t i = 0;
  i = o;
  i <<= 58;
  i |= ((a << 6) >> 6);
  v->ins[x] = i;
  if( v->ct == v->s ){
    v->ins = realloc(v->ins, (v->s += 100) * sizeof(word_t));
  }
}

void vm_print( struct __vm* v, int i ){
  word_t a = v->stack[i];
  int t = gettype(a);
  word_t d = getval(a);
  switch( t ){
    case T_INT:
      printf("Value: %d\n", (Int_t)d);
    break;
    case T_STR:
      printf("Value: %s\n", (char *)d);
    break;
    default: break;
  }
}

void push( struct __vm* v, word_t a, word_t t ){
  int i = v->sct++;
  a |= (t << 61);
  v->stack[i] = a;
  if( v->sct == v->s ){
    int s = (v->s += 100);
    v->stack = realloc(v->stack, s * sizeof(word_t));
  }
}

word_t pop( struct __vm* v ){
  int i = --v->sct;
  return v->stack[i];
}

void vm_run( struct __vm* v ){
  int i = 0, 
      op = 0, 
      err = 0,
      iff = 0; /* 'if' check has failed */

  unsigned long d;
  v->s = 100;
  for( ; i < v->ct; i++ ){
    word_t x = v->ins[i];
    if( err ) break;
    op = x >> 58;
    d = (x << 6) >> 6;
    printf("%s 0x%lx\n", op_debug[op], d);
    switch( op ){
      case OP_BBLOCK:
        break;
      case OP_EBLOCK: 
        break;
      case OP_PUSHINT:
        push(v, (word_t)d, T_INT);
        break;
      case OP_PUSHLVAL:
        break;
      case OP_PUSHVAL: {
        int dl = strlen((const char *)d);
        int hv = hashfind(v->sym, (Byte_t *)d, dl, 0);
        word_t o = hashget(v->sym, (Byte_t *)d, dl);
        if( hv == -1 ) {
          err = 1; 
          printf("Unknown variable %s\n", (char *)d); 
          break; 
        }
        push(v, o, T_INT);
      } break;
      case OP_PUSHNIL: 
        break;
      case OP_REPVAL: 
        break;
      case OP_STORE: {
        word_t o = getval(pop(v));
        hashset(v->sym, (Byte_t *)d, strlen((const char *)d), o);
        printf("Stored %lu in %s\n", o, (Byte_t *)d);
        vm_print(v, v->sct);
      } break;
      case OP_BIF: {
        word_t o = getval(pop(v)), x;
        int co = op, ct = -1;
        if( o ) break;
        iff = 1;
        /* skip everything until endif (including nested if endif pairs) */
        while( i < v->ct ) {
          x = v->ins[++i];
          co = x >> 58;
          if( co == OP_BIF ) ct--;
          if( co == OP_EIF ) ct++;
          if( !ct ) break;
        }
        co = v->ins[i + 1] >> 58;
        if( co != OP_BELSE ) iff = 0;
      } break;
      case OP_BELSE: {
        word_t x;
        int co = op, ct = -1;
        if( iff ) {
          iff = 0;
          break;
        }
        while( i < v->ct ) {
          x = v->ins[++i];
          co = x >> 58;
          if( co == OP_BELSE ) ct--;
          if( co == OP_EELSE ) ct++;
          if( !ct ) break;
        }
        iff = 0;
      } break;
      case OP_BWHILE: 
        break;
      case OP_EWHILE: 
        break;
      case OP_PUSHSTR: 
        push(v, (word_t)d, T_STR);
        break;
      case OP_PUSHTRUE: 
        break;
      case OP_PUSHFALSE: 
        break;
      case OP_PUSHCHAR: 
        break;
      case OP_OBJINIT: 
        break;
      case OP_OBJEND: 
        break;
      case OP_ARRAYINIT: 
        break;
      case OP_ARRAYEND: 
        break;
      case OP_OBJSET: 
        break;
      case OP_ARRAYSET: 
        break;
      case OP_RET: 
        break;
      case OP_EEXP: 
        break;
      case OP_BFCALL: 
        break;
      case OP_EFCALL: 
        break;
      case OP_OACCESS: 
        break;
      case OP_BGACCESS: 
        break; 
      case OP_EGACCESS: 
        break;
      case OP_BFDEF: 
        break; 
      case OP_EFDEF: 
        break;
      case OP_BFNDEF: 
        break; 
      case OP_EFNDEF: 
        break;
      case OP_IFTRUE: 
        break; 
      case OP_LOR: 
        binop(l | r);
        break;
      case OP_SUB: 
        binop(l - r);
        break;
      case OP_ADD: {
        binop(l + r);
      } break;
      case OP_MULT: 
        binop(l * r);
        break;
      case OP_LXOR: 
        binop(l ^ r);
        break;
      case OP_DIV: 
        binop(l / r);
        break;
      case OP_MOD: 
        binop(l % r);
        break;
      case OP_RSHIFT: 
        binop(l >> r);
        break;
      case OP_LSHIFT: 
        binop(l << r);
        break;
      case OP_GT: 
        binop(l > r);
        break;
      case OP_LT: 
        binop(l < r);
        break;
      case OP_ISEQ: 
        binop(l == r);
        break;
      case OP_ISNEQ: 
        binop(l != r);
        break;
      case OP_OR: 
        binop(l || r);
        break;
      case OP_AND: 
        binop(l && r);
        break;
      case OP_LAND: 
        binop((l & r));
        break;
      case OP_CNCT: {
        word_t r = pop(v);
        word_t l = pop(v);
        int rt = gettype(r);
        int lt = gettype(l);
        int len = 0;
        unsigned char *s, *ls, *rs;
        if( lt != rt || rt != T_STR ){
          err = 1;      
          puts("Error: can only concatenate strings");
          break;
        }
        ls = strclone((Byte_t *)getval(l));
        rs = strclone((Byte_t *)getval(r));
        len = strlen((const char *)ls) + strlen((const char *)rs);
        s = malloc(len + 1);
        s[len] = 0;
        strcpy((char *)s, (const char *)ls);
        strcat((char *)s, (const char *)rs);
        push(v, (word_t)s, T_STR);
      } break;
      case OP_NOT: 
        break;
      case OP_LNOT: 
        break;
      case OP_NEG: 
        break;
    }
  }
  vm_print(v, v->sct-1);
}
