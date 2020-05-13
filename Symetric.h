#ifndef SYMETRIC_H
#define SYMETRIC_H

#include "Solver.h"

typedef struct symetric_structure
{
	int ** __restrict var_by_message;
	int n_var_by_message;
	int n_message;
	int * __restrict message_by_var;
	int * __restrict position_by_var;
}SYMETRIC_STUCTURE,*Symetric_structure;


/* **************************************************************************** */
/* 																				*/
/*                   Operations "symetriques" sur les solvers                    */
/* 																				*/
/* **************************************************************************** */


Symetric_structure InitialiseSymetricStrucure(int **var_by_message,	int n_var_by_message, int n_message);

void UpdateSymetricStructure(Symetric_structure S, SysEqLin E);

Solver SolverSymetrique(Solver B, const int message, Symetric_structure S);

Solver SearchUneSolverN3_SymetricVersion(int T, Solver *B1, int N1, SysEqLin E, Symetric_structure S);

#endif
