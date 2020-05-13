%{
#include <stdio.h>
#include <string.h>
#include "parser.tab.h"
%}

%option noinput
%option nounput
%option noyywrap
%option yylineno

HEXNUMBER   [0-9a-f]+
IDENTIFIER  ([a-zA-Z][a-zA-Z0-9',\]\[_\^]*)

%%

[ \t]         /* eat up white space */
#[^\n]*\n     /* eat up one-line comments */
^[ \t]*\n     /* eat up white lines */

{HEXNUMBER}   { sscanf(yytext, "%x", &yylval.number); return COEFF;}

"+" { return PLUS; }
"(" { return LPAR; }
")" { return RPAR; }
"*" { return TIMES; }
"\n" { return NEWLINE; }

("=")* { return EQUAL; }
("-")* { return MINUS; }


{IDENTIFIER} { yylval.name = strdup(yytext); return IDENTIFIER; }
