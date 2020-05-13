#ifndef RECHERCHE_H
#define RECHERCHE_H

#include "Solver.h"
#include "Symetric.h"

Solver * ExhaustiveSearchBalancedSolver(SysEqLin E, const int T, int * taille);

int * GuessMinimumLight(int *X, int N, SysEqLin E, int *taille);

Solver RandomizedSearch(const int T, Solver *B1, const int N1, SysEqLin E);

Solver SearchAlgo(SysEqLin E, int temps, int memoire, int *Known, int NKnown, int r, string_list_t *variable_names, Symetric_structure S);

Solver SearchAlgoLowMem(SysEqLin E, const int T);

Solver * SearchAlgoLowMem_All(SysEqLin E, const int T, int *taille);

Solver * ExhaustiveSearch(Solver * __restrict const B1, const int N1, const int t, int m, int * __restrict taille, SysEqLin E);
#endif
