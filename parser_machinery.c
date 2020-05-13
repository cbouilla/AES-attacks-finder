#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser_machinery.h"
#include "Equation.h"

equation_t    *all_equations = NULL;
string_list_t *all_variables = NULL;
string_list_t *all_boxes = NULL;
string_list_t *known_variables = NULL;

int n_equations = 0;
int n_known_variables = 0;
int n_variables = 0;
int n_messages = 0;

string_list_t **message_variables;



void yyparse(void);

/* ************* Associative lists *****************/
int register_string(string_list_t **list, char *name) {
  string_list_t *head = list[0];

  while (head != NULL) {
    if (strcmp(head->name, name) == 0) return head->number;
    head = head->next;
  }
  int last_number = -1;
  if (list[0] != NULL) last_number = list[0]->number;

  head = malloc(sizeof(string_list_t));
  head->name = name;
  head->number = last_number + 1;
  head->next = *list;
  list[0] = head;
  return head->number;
}


int find_string(string_list_t *list, char *name) {
  string_list_t *head = list;

  while (head != NULL) {
    if (strcmp(head->name, name) == 0) return head->number;
    head = head->next;
  }
  return -1;
}

char * find_var_name(string_list_t *list, int n) {
  string_list_t *head = list;

  while (head != NULL) {
    if (head->number == n) return head->name;
    head = head->next;
  }
  return NULL;
}



void parse(char *filename, 
	   Equation **equations, 
	   int *n_eqs, 
	   int *n_vars, 
	   int **known_vars, 
	   int *known_array_size, 
	   string_list_t **dictionnary,
	   int *n_msg,
	   int *n_var_by_msg,
	   int ***message_groups) {
  int i,j;

  message_variables = malloc(32*sizeof(string_list_t*));
  for(i=0; i<32; i++)
    message_variables[i] = NULL;

  yyin = fopen(filename, "r");
  // TODO : detect errors

  yyparse();
  fclose(yyin);

  register_string(&all_variables, "1"); // register the constant variable



  // allocate the equations
  Equation *eqs = malloc(n_equations*sizeof(Equation));

  equation_t *equation = all_equations;
  i=0;
  while (equation != NULL) {
    
    // allocate the equation
    eqs[i] = malloc(sizeof(EQUATION));
    
    // scan the terms
    eqs[i]->nV = 0;
    eqs[i]->nS = 0;
    term_t *term = equation->head_term;
    while (term != NULL) {
      if (term->sbox == NULL) eqs[i]->nV++; else eqs[i]->nS++;
      term = term->next;
    }
    
    // allocate the coefficients/stuff
    eqs[i]->var = malloc(eqs[i]->nV*sizeof(int));
    eqs[i]->coefV = malloc(eqs[i]->nV*sizeof(unsigned char));
    eqs[i]->sbox = malloc(eqs[i]->nS*sizeof(unsigned char));
    eqs[i]->coefS = malloc(eqs[i]->nS*sizeof(unsigned char));
    eqs[i]->S = malloc(eqs[i]->nS*sizeof(EQUATION*));
    for(j=0; j<eqs[i]->nS; j++)
      eqs[i]->S[j] = malloc(sizeof(EQUATION));
    
    // now rescan the terms and populate the datastructure
    term = equation->head_term;
    int lin_i = 0;
    int box_i = 0;
    while (term != NULL) {
      if (term->variable == NULL) { /* constant term */
	eqs[i]->coefV[lin_i] = term->coeff;
	//printf("read coeff : %02x\n", term->coeff);
	eqs[i]->var[lin_i] = 0; // variable zero denotes constant terms

	lin_i++;
      } else if (term->sbox == NULL) { /* linear term */
	eqs[i]->coefV[lin_i] = term->coeff;
	eqs[i]->var[lin_i] = register_string(&all_variables, term->variable); 

	lin_i++;	
      } else { /* Sbox */
	eqs[i]->coefS[box_i] = term->coeff;
	eqs[i]->sbox[box_i] = register_string(&all_boxes, term->sbox); 
	eqs[i]->S[box_i]->nV = 1;
	eqs[i]->S[box_i]->nS = 0;
	eqs[i]->S[box_i]->S = NULL;
	eqs[i]->S[box_i]->coefS = NULL;
	eqs[i]->S[box_i]->sbox = NULL;
	eqs[i]->S[box_i]->coefV = malloc(sizeof(unsigned char));
	eqs[i]->S[box_i]->var = malloc(sizeof(int));
	eqs[i]->S[box_i]->coefV[0] = 1;
	eqs[i]->S[box_i]->var[0] = register_string(&all_variables, term->variable);

	box_i++;	
      }
      term = term->next;
    }

    i++;
    equation = equation->next;
  }

  // now dealing with the known vars

  n_known_variables++;   // we know "variable 0", ie the constant terms
  int * known_vars_array = malloc(n_known_variables*sizeof(int));
  string_list_t *curr_var = known_variables;
  known_vars_array[0] = 0;
  j = 1;
  while (curr_var != NULL) {
    known_vars_array[j] = find_string(all_variables, curr_var->name);
    if (known_vars_array[j] < 0) {
      fprintf(stderr, "[Parser] a known variable (%s) does not occur in the equations !\n", curr_var->name);
      exit(1);
    }
    curr_var = curr_var->next;
    j++;
  }

  // dealing with the "message" grouping, if relevant
  int all_msg_nvar = -1;
  int **msg_grouping = NULL;

  if (n_messages == 0 && message_variables[0] == NULL) 
    printf("[+] No (explicit) message information available\n");
  else {
    printf("[+] %d distinct messages appear in the equations\n", n_messages+1);
    n_messages++;

    msg_grouping = malloc(n_messages*sizeof(int*));
    for(i=0; i<n_messages; i++) {
      
      // count the number of variables in current message
      int nvars = 0;
      curr_var = message_variables[i];
      while (curr_var != NULL) { curr_var = curr_var->next; nvars++; }
      printf("  [+] %d variables found in message %d\n", nvars, i+1);
      
      if (all_msg_nvar < 0) all_msg_nvar = nvars;
      if (all_msg_nvar > 0 && nvars != all_msg_nvar) {
	printf("\nHmmm, all your message do not have the same number of variables... This is quite suspect !\n");
	exit(1);
      }
      
      // allocate and fill array 
      msg_grouping[i] = malloc(nvars*sizeof(int));
      curr_var = message_variables[i];
      j = 0;
      while (curr_var != NULL) {
	msg_grouping[i][j] = find_string(all_variables, curr_var->name);
	if (msg_grouping[i][j] < 0) {
	  fprintf(stderr, "[Parser] in the ''message'' section, a variable (%s) does not occur in the equations !\n", curr_var->name);
	  exit(1);
	}
	curr_var = curr_var->next;
	j++;
      }
      
      // moving on to the next message
    }
  }
    
  // storing the results and leaving
  
  *equations = eqs;
  *known_vars = known_vars_array;
  
  *n_eqs = n_equations;
  *n_vars = all_variables->number + 1;  // numbered from 0 to all_variables->number
  *n_msg = n_messages;
  *n_var_by_msg = all_msg_nvar;
  
  *known_array_size = n_known_variables;
  *dictionnary = all_variables;
  *message_groups = msg_grouping;
}

/*
int main() {

  // parse(....);
  
  EQUATION *eqs;
  int n_equations;
  int n_variables;
  int *known_variables;
  int n_known_variables;
  string_list_t *variable_names;

  parse(&eqs,&n_equations, &n_variables, &known_variables, &n_known_variables, &variable_names);

  printf("%d equations found in %d variables and %d known variables\n", n_equations, n_variables, n_known_variables);
}
*/
