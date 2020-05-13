#ifndef ALGO_H
#define ALGO_H

#include "Solver.h"

typedef struct algorithme
{
	Solver B;
	char *debut;
	char *fin;
	int tab;
	int sur;
	unsigned long int mem;
	unsigned long int mem_max;
}ALGORITHME;
	
void EcrireEq(FILE *out, Equation a);

void EcrireAlgo(char *s, Solver b, int *Known, int NKnown, SysEqLin E, string_list_t *variable_names);

#endif
