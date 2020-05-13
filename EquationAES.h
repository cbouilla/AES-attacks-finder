#ifndef EQUATIONAES_H
#define EQUATIONAES_H

#include "Equation.h"

typedef struct Vars
{
	int v;
	char lettre;
	int message;
	int round;
	int i;
	int j;
}VARS;

int MakeKS(Equation *Eq, int Nk, int Tours, int Final);

int MakeAK(Equation *Eq, int key, int state1, int state2);

int MakeMC(Equation *Eq, int state1, int state2);

int MakeSS(Equation *Eq, int key, int state1, int state2);

Equation * MakeEquations(int Nk, int Tours, int Final, int Messages, int *N);

int * VariablesKnownes(int Tours, int Final, int Messages, int *N);

Equation * MakeEquations2(int Nk, int Tours, int Final, int Messages, int *N);

int * VariablesKnownes2(int Tours, int Final, int Messages, int *N);

#endif
