#ifndef PARSER_MACHINERY_H
#define PARSER_MACHINERY_H

#include <stdio.h>

#include "Equation.h"

struct string_list_t {
  char *name;
  int number;
  struct string_list_t *next;
};

struct term_t {
  char *variable;
  char *sbox;
  unsigned char coeff;
  struct term_t *next;
};

struct equation_t {
  struct term_t *head_term;
  struct equation_t * next;
};

extern struct equation_t *all_equations;
extern struct string_list_t *known_variables;
extern struct string_list_t **message_variables;


extern int n_equations;
extern int n_known_variables;
extern int n_messages;


typedef struct string_list_t string_list_t;
typedef struct equation_t equation_t;
typedef struct term_t term_t;


extern FILE *yyin;

void parse(char *filename, 
	   Equation **equations, 
	   int *n_eqs, 
	   int *n_vars, 
	   int **known_vars, 
	   int *known_array_size, 
	   string_list_t **dictionnary,
	   int *n_msg,
	   int *n_var_by_msg,
	   int ***message_groups);

char * find_var_name(string_list_t *dictionnary, int var);
int find_string(string_list_t *list, char *name);
#endif
