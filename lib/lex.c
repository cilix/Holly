#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "lex.h"

#define ismatch(x, y) (!strncmp((const char *)x, (const char *)y, strlen(y)))
#define isspace(x)    (x == ' ' || x == '\t' || x == '\n')
#define isdigit(x)    (x >= '0' && x <= '9')
#define islower(x)    (x >= 'a' && x <= 'z')
#define isalpha(x)    (islower(x) || (x >= 'A' && x <= 'Z'))
#define ishex(x)      (isdigit(x) || islower(x))

#define token(s, t, c) do { s->tok.type = t; s->prog += c; } while(0)

void clean( struct __tok* t ){
  if( t->as_string ) free(t->as_string);
  if( t->as_name ) free(t->as_name);
  t->last_tok_type = t->type;
  t->type = no_tok;
  t->as_int = t->as_char = 0;
  t->as_string = t->as_name = NULL;
}

unsigned char esc( unsigned char c ){
  switch( c ){
    case 'a': c = '\a'; break;
    case 'b': c = '\b'; break;
    case 'f': c = '\f'; break;
    case 'n': c = '\n'; break;
    case 'r': c = '\r'; break;
    case 't': c = '\t'; break;
    case 'v': c = '\v'; break;
    default:            break;
  }
  return c;
}

int hex( unsigned char c ){
  int n;
  switch( c ){
    case 'a': n = 10; break;
    case 'b': n = 11; break;
    case 'c': n = 12; break;
    case 'd': n = 13; break;
    case 'e': n = 14; break;
    case 'f': n = 15; break;
    default:  n = c - '0';
    break;
  }
  return n;
}

Int_t Int( unsigned char* v, int c, int b ){
  Int_t i = 0, iv = 0, a = 0;
  Int_t m = Int_MAX, l = Int_MAX / b;
  while( i < c ){
    if( iv >= l && i < c - 1) return m;
    a = hex(v[i++]);
    iv *= b;
    iv += (m - a < iv ? m - iv : a);
  }
  return iv;
}

int numlen( unsigned char* v, int b ){
  int i = 0;
  if( b == 16 ){
    while( ishex(*(v + i)) ) i++;
  } else if( b == 10 ){
    while( isdigit(*(v + i)) ) i++;
  }      
  return i;
}

unsigned char* str( unsigned char* v, int l ){
  int i = 0, j = 0;
  unsigned char c;
  unsigned char* b = malloc(l + 1);
  memset(b, 0, l + 1);
  while( i < l ){
    c = v[i++];
    b[j++] = c == '\\' ? esc(v[i++]) : c;
  }
  return b;
}

unsigned char* var( unsigned char* v ){
  int i = 0;
  unsigned char* b;
  while(
    isalpha(*(v + i)) ||
    isdigit(*(v + i)) ||
    (*(v + i) == '_') 
  ) i++;
  b  = malloc(i + 1);
  memcpy(b, v, i);
  b[i] = 0;
  return b;
}

int next( struct __state* s ){
  while( isspace(*(s->prog)) ){
    (s->prog)++;
  }

  clean(&(s->tok));

  switch( *(s->prog) ){
    case 0  : token(s, eof_tok, 0);        break;
    case '+': token(s, plus_tok, 1);       break;
    case '*': token(s, star_tok, 1);       break;
    case '^': token(s, xor_tok, 1);        break;
    case '/': token(s, fslash_tok, 1);     break;
    case '%': token(s, mod_tok, 1);        break;
    case '|': token(s, pipe_tok, 1);       break;
    case '{': token(s, lbrace_tok, 1);     break;
    case '}': token(s, rbrace_tok, 1);     break;
    case '[': token(s, lbrack_tok, 1);     break;
    case ']': token(s, rbrack_tok, 1);     break;
    case '(': token(s, lparen_tok, 1);     break;
    case ')': token(s, rparen_tok, 1);     break;
    case ';': token(s, semicolon_tok, 1);  break;
    case ',': token(s, comma_tok, 1);      break;
    case '&': token(s, ampersand_tok, 1);  break;
    case '~': token(s, tilda_tok, 1);      break;
    case '"': {
      int i = 1;
      unsigned char* c;
      while( 
        *(c = (s->prog + i)) &&
        (*c != '"' || *(c - 1) == '\\')
      ) i++;
      c = str(++(s->prog), i - 1);
      if( c ){
        token(s, string_tok, i);
        s->tok.as_string = c;
      } else {
        return 0;
      }
    } break;
    case '\'': {
      unsigned char c;
      c = *(++(s->prog));
      if( c == '\\' ){
        c = esc(*(++(s->prog)));
      }
      (s->prog)++;
      if( *(s->prog) != '\'' ){
        return 0;
      } else {
        token(s, char_tok, 1);
        s->tok.as_char = c;
      }
    } break;
    case '=': {
      if( *(s->prog + 1) == '=' ){
        token(s, equals_tok, 2);
      } else {
        token(s, assign_tok, 1);
      }
    } break;
    case '!': {
      if( *(s->prog + 1) == '=' ){
        token(s, notequal_tok, 2);
      } else {
        token(s, not_tok, 1);    
      }   
    } break;
    case ':': {
      if( *(s->prog + 1) == ':' ){
        token(s, doublecolon_tok, 2);
      } else {
        token(s, colon_tok, 1);
      }
    } break;
    case '-': {
      unsigned char *p = (s->prog += 1);
      Int_t i = 0, v = 0, b = 10;
      if( *p == '-' ){
        int l = 0;
        while( *p && *(p++) != '\n' ) l++;
        token(s, comment_tok, l);
        break;
      }
      if( *p == '>' ){
        token(s, arrow_tok, 1);
        break;
      }
      if( ismatch(p, "0x") ){
        p = (s->prog += 2);
        b = 16;
      }
      i = numlen(s->prog, b);
      if( i ){
        v = Int(s->prog, i, b);
        token(s, int_tok, i);
        s->tok.as_int = -1 * v;
      } else {
        token(s, minus_tok, 0);
      }
    } break;
    case '>': {
      if( *(s->prog + 1) == '>' ){
        token(s, rshift_tok, 2);
      } else {
        token(s, gthan_tok, 1);
      }
    } break;
    case '<': {
      if( *(s->prog + 1) == '<' ){
        token(s, lshift_tok, 2);
      } else {
        token(s, lthan_tok, 1);
      }
    } break;
    case '.': {
      if( *(s->prog + 1) == '.' ){
        token(s, concat_tok, 2);
      } else {
        token(s, period_tok, 1);
      }
    } break;
    default: {
      unsigned char *p = s->prog;
      if(      ismatch(p, "let")    ) token(s, let_tok, 3);
      else if( ismatch(p, "if")     ) token(s, if_tok, 2);
      else if( ismatch(p, "else")   ) token(s, else_tok, 4);
      else if( ismatch(p, "return") ) token(s, return_tok, 6);
      else if( ismatch(p, "while")  ) token(s, while_tok, 5);
      else if( ismatch(p, "fn")     ) token(s, lambda_tok, 2);
      else if( ismatch(p, "true")   ) token(s, true_tok, 4);
      else if( ismatch(p, "false")  ) token(s, false_tok, 5);
      else if( ismatch(p, "nil")    ) token(s, nil_tok, 3);
      else if( ismatch(p, "and")    ) token(s, and_tok, 3);
      else if( ismatch(p, "or")     ) token(s, or_tok, 2);
      else if( ismatch(p, "String") ) token(s, stringtype_tok, 6);
      else if( ismatch(p, "Char")   ) token(s, chartype_tok, 4);
      else if( ismatch(p, "Bool")   ) token(s, bool_tok, 4);
      else if( ismatch(p, "Int")    ) token(s, inttype_tok, 3);
      else if( ismatch(p, "Map")    ) token(s, maptype_tok, 3);
      else if( ismatch(p, "Array")  ) token(s, arraytype_tok, 5);
      else if( ismatch(p, "Float")  ) token(s, floattype_tok, 5);
      else if( isalpha(*p) ){
        unsigned char* v = var(p);
        s->tok.as_name = v;
        token(s, name_tok, strlen((const char *)v));
      } else {
        Int_t i = 0, v = 0, b = 10;
        if( ismatch(p, "0x") ){
          p = (s->prog += 2);
          b = 16;
        }
        i = numlen(s->prog, b);
        if( i ){
          v = Int(s->prog, i, b);
          token(s, int_tok, i);
          s->tok.as_int = v;
          break;
        }
        return 0;
      }
    } break;
  }
  return 1;
}
