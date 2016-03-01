#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "parse.h"
#include "util.h"

#define abortonerror(x) do { if( x->err ) return; } while(0)
#define emit(s, o, v) vm_ins(&(s->vm), o, (unsigned long)v);

static const char* tokstr[] = { "",
  "<string>", "<char>", "<bool>", "==", "<int>", "|", "-", "\\", "+",
  "*", "^", "/", "%", ">>", "<<", ">", "<", "or", "and", "..", "{", "}", "[", "]",
  ":", "=", "(", ")", ";", ",", "<comment>", "&", "~", "!", ".", "<float>",
  "let", "if", "else", "return", "while", "lambda", "true", "false", "nil",
  "<name>", "->", "<eof>", "!=", "::", "break", "Float", "String", "Int", "Char",
  "Map", "Array"
};

void block( struct __state* );
void ifstmt( struct __state* );
void elsestmt( struct __state* );
void stmt( struct __state* );
void whilestmt( struct __state* );
void map( struct __state* );
void pair( struct __state* );
void pairlist( struct __state* );
void array( struct __state* );
void value( struct __state* );
void varsuffix( struct __state* );
void functioncall( struct __state* );
void expr( struct __state* );
int unop( struct __state* );
int istype( struct __state* );
void explist( struct __state* );
void functioncallsuffix( struct __state* );
void typeexp( struct __state* );
void namelist( struct __state* );
void functionbody( struct __state* );
void stmtexpr( struct __state* );
void variable( struct __state* );

void error( struct __state* s, const char* err, const char* str ){
  fprintf(stderr, "error: %s: %s\n", err, str);
  s->err = 1;
  return;
}

int accept( struct __state* s, enum tok t ){
  if( s->err ){ return 0; }
  if( s->tok.type == t ){
    next(s);
    return 1;
  }
  return 0;
}

void expect( struct __state* s, enum tok t ){
  if( s->err ){ return; }
  if( accept(s, t) ){ return; }
  if( s->tok.type != eof_tok ){
    error(s, "unexpected symbol", tokstr[s->tok.type]);
  } else {
    error(s,"expected symbol", tokstr[t]);
  }
}

int peek( struct __state* s, enum tok t ){
  if( s->err ){ return 0; }
  if( s->tok.type == t ){
    return 1;
  }
  return 0;
}

void varsuffix( struct __state* s ){
  abortonerror(s);
  if( peek(s, lparen_tok) || peek(s, doublecolon_tok) ){
    functioncallsuffix(s);
    varsuffix(s);
    return;
  }
  if( accept(s, period_tok) ){
    if( peek(s, name_tok) ){
      unsigned char* n = strclone(s->tok.as_name);
      emit(s, OP_OACCESS, n);
    }
    expect(s, name_tok);
    varsuffix(s);
    return;
  }
  if( accept(s, lbrack_tok) ){
    emit(s, OP_BGACCESS, 0);
    expr(s);
    expect(s, rbrack_tok);
    emit(s, OP_EGACCESS, 0);
    varsuffix(s);
  }  
}

int unop( struct __state* s ){
  if(
    peek(s, not_tok)   ||
    peek(s, tilda_tok) ||
    peek(s, minus_tok)
  ) return 1;
  return 0;
}

int istype( struct __state* s ){
  if(
    peek(s, stringtype_tok) ||
    peek(s, inttype_tok)    ||
    peek(s, floattype_tok)  ||
    peek(s, maptype_tok)    ||
    peek(s, arraytype_tok)  ||
    peek(s, bool_tok)       ||
    peek(s, chartype_tok)
  ) return 1;
  return 0;
}

void functiondef( struct __state* s ){
  abortonerror(s);
  expect(s, lambda_tok);
  functionbody(s);
}

void functionbody( struct __state* s ){
  abortonerror(s);
  namelist(s);
  if( accept(s, lbrace_tok) ){
    block(s);
    expect(s, rbrace_tok);
    return;
  }
  expect(s, arrow_tok);
  expr(s);
  emit(s, OP_RET, 0); /* implicit return */
}

void namelist( struct __state* s ){
  abortonerror(s);
  if( peek(s, name_tok) ){
    unsigned char* n = strclone(s->tok.as_name);
    emit(s, OP_STORE, n);
    next(s);
    typeexp(s);
    if( accept(s, comma_tok) ){
      namelist(s);
    }
  }
}

void typeexp( struct __state* s ){
  abortonerror(s);
  if( accept(s, colon_tok) ){
    if( istype(s) ){
      next(s);
      return;
    }
    error(s, "Expected", "type declaration");
  }
}

void functioncallsuffix( struct __state* s ){
  abortonerror(s);
  if( accept(s, lparen_tok) ){
    emit(s, OP_BFCALL, 0);
    if( !peek(s, rparen_tok) ) explist(s);
    emit(s, OP_EFCALL, 0);
    expect(s, rparen_tok);
    return;
  }
  /* get last computed value here */
  expect(s, doublecolon_tok);
  if( peek(s, name_tok) ){
    unsigned char* n = strclone(s->tok.as_name);
    emit(s, OP_OACCESS, n);
    next(s);
  } else {
    error(s, "unexpected symbol", tokstr[s->tok.type]);
  }
  expect(s, lparen_tok);
  emit(s, OP_BFCALL, 0);
  emit(s, OP_PUSHLVAL, 0);
  if( !peek(s, rparen_tok) ) explist(s);
  emit(s, OP_EFCALL, 0);
  expect(s, rparen_tok);
}

void expr( struct __state* s ){
  abortonerror(s);

  if( peek(s, int_tok) ){
    Int_t n = s->tok.as_int;
    next(s);
    emit(s, OP_PUSHINT, n);
    goto check_binop;
    return;
  }

  if( peek(s, true_tok) ){
    next(s);
    emit(s, OP_PUSHTRUE, 0);
    goto check_binop;
    return;
  }

  if( peek(s, false_tok) ){
    next(s);
    emit(s, OP_PUSHFALSE, 0);
    goto check_binop;
    return;
  }

  if( peek(s, string_tok) ){
    unsigned char* str = strclone(s->tok.as_string);
    next(s);
    emit(s, OP_PUSHSTR, str);
    goto check_binop;
    return;
  }

  if( peek(s, char_tok) ){
    char c = s->tok.as_char;
    next(s);
    emit(s, OP_PUSHCHAR, c);
    goto check_binop;
    return;
  }

  if( accept(s, lparen_tok) ){
    expr(s);
    expect(s, rparen_tok);
    goto check_binop;
    return;
  }

  if( accept(s, nil_tok) ){
    emit(s, OP_PUSHNIL, 0);
    return;
  }

  if( unop(s) ){
    int t = s->tok.type;
    enum op o;
    next(s);
    expr(s);
    switch( t ){
      case not_tok:   o = OP_NOT; break;
      case tilda_tok: o = OP_LNOT; break;
      case minus_tok: o = OP_NEG; break;
    }
    emit(s, o, 0);
    return;
  }

  if( peek(s, lbrace_tok) ){
    emit(s, OP_OBJINIT, 0);
    map(s);
    emit(s, OP_OBJEND, 0);
    return;
  }

  if( peek(s, lbrack_tok) ){
    array(s);
    return;
  }

  if( peek(s, lambda_tok) ){
    emit(s, OP_BFNDEF, 0);
    functiondef(s);
    emit(s, OP_EFNDEF, 0);
    return;
  }

  variable(s);

  check_binop: {
    int t = s->tok.type;
    enum op o = 0;
    switch( t ){
      case pipe_tok:      o = OP_LOR; break;
      case minus_tok:     o = OP_SUB; break;
      case plus_tok:      o = OP_ADD; break;
      case star_tok:      o = OP_MULT; break;
      case xor_tok:       o = OP_LXOR; break;
      case fslash_tok:    o = OP_DIV; break;
      case mod_tok:       o = OP_MOD; break;
      case rshift_tok:    o = OP_RSHIFT; break;
      case lshift_tok:    o = OP_LSHIFT; break;
      case gthan_tok:     o = OP_GT; break;
      case lthan_tok:     o = OP_LT; break;
      case equals_tok:    o = OP_ISEQ; break;
      case notequal_tok:  o = OP_ISNEQ; break;
      case or_tok:        o = OP_OR; break;
      case and_tok:       o = OP_AND; break;
      case ampersand_tok: o = OP_LAND; break;
      case concat_tok:    o = OP_CNCT; break;
      default:            break;
    }
    if( o ){
      next(s);
      expr(s);
      emit(s, o, 0);
    }
  }
}

void explist( struct __state* s ){
  abortonerror(s);
  expr(s);
  emit(s, OP_EEXP, 0);
  if( accept(s, comma_tok) ){
    explist(s);
  }
}

void elsestmt( struct __state* s ){
  abortonerror(s);
  if( accept(s, else_tok) ){
    emit(s, OP_BELSE, 0);
    if( accept(s, lbrace_tok) ){
      block(s);
      expect(s, rbrace_tok);
      emit(s, OP_EELSE, 0);
    } else {
      ifstmt(s);
    }
  }
}

void ifstmt( struct __state* s ){
  abortonerror(s);
  expect(s, if_tok);
  expr(s);
  emit(s, OP_BIF, 0);
  expect(s, lbrace_tok);
  block(s);
  expect(s, rbrace_tok);
  emit(s, OP_EIF, 0);
  elsestmt(s);
}

void whilestmt( struct __state* s ){
  abortonerror(s);
  expect(s, while_tok);
  emit(s, OP_BWHILE, 0);
  expr(s);
  expect(s, lbrace_tok);
  emit(s, OP_IFTRUE, 0);
  block(s);
  emit(s, OP_EWHILE, 0);
  expect(s, rbrace_tok);
}

void map( struct __state* s ){
  abortonerror(s);
  expect(s, lbrace_tok);
  pairlist(s);
  expect(s, rbrace_tok);
}

void pair( struct __state* s ){
  unsigned char* n;
  abortonerror(s);
  n = strclone(s->tok.as_name);
  expect(s, name_tok);
  expect(s, colon_tok);
  expr(s);
  emit(s, OP_OBJSET, n);
}

void pairlist( struct __state* s ){
  abortonerror(s);
  pair(s);
  if( accept(s, comma_tok) ){
    pairlist(s);
  }
}

void array( struct __state* s ){
  abortonerror(s);
  expect(s, lbrack_tok);
  emit(s, OP_ARRAYINIT, 0);
  explist(s);
  expect(s, rbrack_tok);
  emit(s, OP_ARRAYEND, 0);
}

void variable( struct __state* s ){
  abortonerror(s);
  /* TODO
   * [1, 2, 3][0] and { hello: "world" }.hello is unsupported
   * possible solution:
   * var ::= Name varsuffix | exp varsuffix
   */
  if( accept(s, lparen_tok) ){
    variable(s);
    expect(s, rparen_tok);
    return;
  }
  if( peek(s, name_tok) ){
    unsigned char* n = strclone(s->tok.as_name);
    emit(s, OP_PUSHVAL, n);
    next(s);
    varsuffix(s);
  } else {
    error(s, "unexpected symbol", tokstr[s->tok.type]);
  }
} 

void stmt( struct __state* s ){
  abortonerror(s);
  if( accept(s, semicolon_tok) ){
    return; /* empty statement */
  } else if( peek(s, if_tok) ){
    ifstmt(s);
  } else if( peek(s, while_tok) ){
    whilestmt(s);
  } else if( accept(s, return_tok) ){
    expr(s);
    emit(s, OP_RET, 0);
    accept(s, semicolon_tok);
  } else if( accept(s, break_tok) ){
    return;
  } else if( peek(s, lparen_tok) ){
    variable(s);
    if( accept(s, assign_tok) ){
      emit(s, OP_LHAND, 0);
      expr(s); 
      emit(s, OP_REPVAL, 0);
    }
    accept(s, semicolon_tok);
  } else if( accept(s, let_tok) ){
    unsigned char* n = strclone(s->tok.as_name);
    expect(s, name_tok);
    if( accept(s, assign_tok) ){
      expr(s);
      accept(s, semicolon_tok);
      emit(s, OP_STORE, n);
    }
    accept(s, semicolon_tok);
  } else {
    stmtexpr(s);
  }
}

void stmtexpr( struct __state* s ){
  if( peek(s, name_tok) ){
    unsigned char* n;
    n = strclone(s->tok.as_name);
    next(s);
    if( 
      (peek(s, name_tok) || peek(s, lbrace_tok) || peek(s, arrow_tok)) &&
      (s->tok.last_tok_type != rparen_tok)
    ){
      emit(s, OP_BFDEF, n);
      functionbody(s);
      emit(s, OP_EFDEF, 0);
      return;
    }
    emit(s, OP_PUSHVAL, n);
    varsuffix(s);
    if( accept(s, assign_tok) ){
      emit(s, OP_LHAND, 0);
      expr(s); 
      emit(s, OP_REPVAL, 0);
    }
    accept(s, semicolon_tok);
  } else {
    /* expr(s) ? */
    error(s, "unexpected symbol", tokstr[s->tok.type]);
  }
}

void stmtlist( struct __state* s ){
  abortonerror(s);
  if( peek(s, rbrace_tok) ){
    return;
  }
  if( peek(s, eof_tok) ){
    return;
  }
  stmt(s);
  stmtlist(s);
}

void block( struct __state* s ){
  abortonerror(s);
  emit(s, OP_BBLOCK, 0);
  if( accept(s, lbrace_tok) ){
    block(s);
    expect(s, rbrace_tok);
  }
  stmtlist(s);
  emit(s, OP_EBLOCK, 0);
}

void start( struct __state* s ){
  abortonerror(s);
  next(s);
  block(s);
}
