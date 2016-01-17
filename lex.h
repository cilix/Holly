#ifndef _LEX_H
#define _LEX_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "vm.h"

enum tok {
  no_tok = 0,
  string_tok,
  char_tok,
  bool_tok,
  equals_tok,
  int_tok,
  pipe_tok,
  minus_tok,
  bslash_tok,
  plus_tok,
  star_tok,
  xor_tok,
  fslash_tok,
  mod_tok,
  rshift_tok,
  lshift_tok,
  gthan_tok,
  lthan_tok,
  or_tok,
  and_tok,
  concat_tok,
  lbrace_tok,
  rbrace_tok,
  lbrack_tok,
  rbrack_tok,
  colon_tok,
  assign_tok,
  lparen_tok,
  rparen_tok,
  semicolon_tok,
  comma_tok,
  comment_tok,
  ampersand_tok,
  tilda_tok,
  not_tok,
  period_tok,
  float_tok,
  let_tok,
  if_tok,
  else_tok,
  return_tok,
  while_tok,
  lambda_tok,
  true_tok,
  false_tok,
  nil_tok,
  name_tok,
  arrow_tok,
  eof_tok,
  notequal_tok,
  doublecolon_tok,
  break_tok,
  floattype_tok,
  stringtype_tok,
  inttype_tok,
  chartype_tok,
  maptype_tok,
  arraytype_tok
};

struct __tok { 
  enum tok       type;
  enum tok       last_tok_type;
  unsigned char* as_string;
  unsigned char* as_name;
  unsigned char  as_char;
  Int_t          as_int;
};

struct __state { 
  unsigned char* prog;
  struct __tok   tok;
  struct __vm    vm;
  int            err;
};

int next ( struct __state* );

#endif
