#ifndef _VM_H
#define _VM_H

/* 32 bit or 64 bit 144115188075855871 2147483647 */
#define Int_MAX 2147483647
#define Int_t int
#define word_t unsigned long

#include "hash.h"

struct __vm {
  word_t*    ins;
  word_t*    stack;
  HashTable* sym;
  int        ct;
  int        sct;
  int        s;
};

enum op {
  NOOP = 0,
  OP_BBLOCK,
  OP_EBLOCK,
  OP_PUSHINT,
  OP_PUSHVAL,
  OP_PUSHLVAL, /* for :: function calls */
  OP_PUSHNIL,
  OP_REPVAL,
  OP_STORE,
  OP_BIF,
  OP_EIF,
  OP_BELSE,
  OP_EELSE,
  OP_BWHILE,
  OP_EWHILE,
  OP_PUSHSTR,
  OP_PUSHTRUE,
  OP_PUSHFALSE,
  OP_PUSHCHAR,
  OP_OBJINIT,
  OP_OBJEND,
  OP_ARRAYINIT,
  OP_ARRAYEND,
  OP_OBJSET,
  OP_ARRAYSET,
  OP_RET,
  OP_EEXP,
  OP_BFCALL,
  OP_EFCALL,
  OP_OACCESS,
  OP_BGACCESS, /* generic access (string, array, object) */
  OP_EGACCESS,
  OP_BFDEF, /* named function */
  OP_EFDEF,
  OP_BFNDEF, /* lambda function */
  OP_EFNDEF,
  OP_IFTRUE, /* for 'while' stmts */
  OP_LOR,
  OP_SUB,
  OP_ADD,
  OP_MULT,
  OP_LXOR,
  OP_DIV,
  OP_MOD,
  OP_RSHIFT,
  OP_LSHIFT,
  OP_GT,
  OP_LT,
  OP_ISEQ,
  OP_ISNEQ,
  OP_OR,
  OP_AND,
  OP_LAND,
  OP_NOT,
  OP_LNOT,
  OP_NEG,
  OP_CNCT,
  OP_LHAND
};

void vm_ins( struct __vm*, enum op, unsigned long );
void vm_init( struct __vm* );
void vm_run( struct __vm* );

#endif
