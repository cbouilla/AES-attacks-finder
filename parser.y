%{
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser_machinery.h"

void yyerror(char *s);
int yylex(void);
%}


%union {
  struct string_list_t *string_list;
  struct equation_t *eq;
  struct term_t *term;
  int number;
  char *name;
}


%type <term> equation
%type <term> term
%type <term> simple_term

%token <number> COEFF
%token <name> IDENTIFIER

%token LPAR
%token RPAR
%token PLUS
%token TIMES
%token VBAR
%token NEWLINE
%token EQUAL
%token MINUS

%left PLUS
%left TIMES
%nonassoc MINUS
%nonassoc EQUAL


%start document


%%

document: 
| equations
| equations EQUAL NEWLINE variable_list NEWLINE
| equations EQUAL NEWLINE variable_list NEWLINE EQUAL NEWLINE msg_separation 


equations: /* empty */        
| equations NEWLINE          
| equations equation NEWLINE  { struct equation_t *res = malloc(sizeof(struct equation_t));
                            res->head_term = $2;
			    res->next = all_equations;
			    all_equations = res;
			    n_equations++;};




equation: equation PLUS term { $$ = $3; $3->next = $1; };
| term                       { $$ = $1;};


simple_term:  IDENTIFIER            { struct term_t *res = malloc(sizeof(struct term_t)); 
                                    res->variable = $1; 
				    res->coeff = 1; 
				    res->sbox = NULL;
				    res->next = NULL;
				    $$ = res;};
| IDENTIFIER LPAR IDENTIFIER RPAR   { struct term_t *res = malloc(sizeof(struct term_t)); 
                                    res->variable = $3; 
				    res->coeff = 1; 
				    res->sbox = $1;
				    res->next = NULL;
				    $$ = res;};


term: COEFF                         { struct term_t *res = malloc(sizeof(struct term_t)); 
                                    res->variable = NULL;
				    res->sbox = NULL;
				    res->coeff = $1; 
				    res->next = NULL;
				    $$ = res;};
| COEFF TIMES simple_term           { $$ = $3; $3->coeff=$1;};
| simple_term                       { $$ = $1;};


variable_list:   /* empty */         
| variable_list IDENTIFIER            { struct string_list_t *res = malloc(sizeof(struct string_list_t));
                                        res->name = $2;
					res->next = known_variables;
					known_variables = res;
					n_known_variables++;
                                      };
| variable_list NEWLINE IDENTIFIER    { struct string_list_t *res = malloc(sizeof(struct string_list_t));
                                        res->name = $3;
					res->next = known_variables;
					known_variables = res;
					n_known_variables++;
                                      };

variables_msg:   /* empty */          
| variables_msg IDENTIFIER            { struct string_list_t *res = malloc(sizeof(struct string_list_t));
                                        res->name = $2;
					res->next = message_variables[n_messages];
					message_variables[n_messages] = res;
                                      };


| variables_msg NEWLINE IDENTIFIER    { struct string_list_t *res = malloc(sizeof(struct string_list_t));
                                        res->name = $3;
					res->next = message_variables[n_messages];
					message_variables[n_messages] = res;
                                      };

msg_packet: variables_msg NEWLINE MINUS NEWLINE {n_messages++;}


msg_separation:  variables_msg NEWLINE 
| msg_packet msg_separation 


%%

void yyerror(char *s) {
  extern int yylineno;
  extern char* yytext;
  fprintf(stderr, "[Parser] %s at symbol '%s' at line %d\n", s, yytext, yylineno);
}
