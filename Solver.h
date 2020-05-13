#ifndef SOLVER_H

typedef struct solver
{
  int * __restrict var;  // liste des variables
  int Nvar; // nombre de variables
  int temps; // complexité en temps
  int memoire; // complexité en memoire
  int sortie; // nombre de solutions
  char type;        /* 0=enumerates a variable   1=parallel union     2=sequential union */
  int parents; // nombre de pointeurs vers la solver
  struct solver * __restrict fils1; 
  struct solver * __restrict fils2;
  // Lors d'une union de type 1, fils2 est la solver stockée.
  // Lors d'une union de type 2, fils2 est la solver énumérée en premier. 
}SOLVER,*Solver;

#include <stdio.h>
#include "parser_machinery.h"
#include "SysEqLin.h"



/* **************************************************************************** */
/* 																				*/
/*                       Gestion mémoire Solver                                  */
/* 																				*/
/* **************************************************************************** */

#define NOMBREDESOLVER 10000

Solver NewSolver(void); // Renvoi l'adresse d'une SOLVER disponible

Solver copySolver(Solver a); // copie une solver "pour de faux" (NE SURTOUT PAS MODIDIFIER UNE SOLVER DIRECTEMENT)

void freeSolver(Solver a); // libère la mémoire associée à une solver



/* **************************************************************************** */
/* 																				*/
/*                       Interface xml <-> Solver                                */
/* 																				*/
/* **************************************************************************** */


Solver charge_solver(FILE *out, string_list_t *variable_names); // charge une solver à partir d'un xml

void print_solver(FILE *out, Solver B, string_list_t *variable_names); // crée le xml associé à une solver



/* **************************************************************************** */
/* 																				*/
/*                   Operations simples sur le type Solver                       */
/* 																				*/
/* **************************************************************************** */


void PrintVarSolver(Solver B); // Print les variables d'une solver (sous la forme d'entiers)

void PrintVarSolver2(Solver B, string_list_t *variable_names); // Print les variables d'une solver

Solver BaseSolver(int i, SysEqLin E); // Solver associée à la variable i

int SameSolvers(Solver B1, Solver B2); // Vrai si les variables des deux solvers sont les mêmes.

#if 0
int PlusInteressant(Solver B1, Solver B2); // Vrai si la solver B1 est plus intéressante que la solver B2. VERIFIER AU PREALABLE QUE LES SOLVERS SONT COMPARABLES!

int PlusInteressant2(Solver B1, Solver B2); // Vrai si la solver B1 est plus intéressante que la solver B2 sauf en mémoire. VERIFIER AU PREALABLE QUE LES SOLVERS SONT COMPARABLES!

#else

#define PlusInteressant(B1,B2) (B1->Nvar>=B2->Nvar && B1->sortie<=B2->sortie && (B1->temps<B2->temps || (B1->temps==B2->temps && B1->memoire<=B2->memoire)))
#define PlusInteressant2(B1,B2) (B1->Nvar>=B2->Nvar && B1->sortie<=B2->sortie && B1->temps<=B2->temps)
#define PlusInteressant_NoVars(B1,B2) (B1->sortie<=B2->sortie && (B1->temps<B2->temps || (B1->temps==B2->temps && B1->memoire<=B2->memoire)))
#define PlusInteressant_NoVarsNoMem(B1,B2) (B1->sortie<=B2->sortie && B1->temps<=B2->temps)

#endif

void ParcoursSolver(Solver B); // Permet de parcourir une solver, utile pour debugguer

void TriFusion1(Solver *B1, int N); // Trie une liste de solvers selon 1) la sortie dans l'ordre croissant 2) le nombre de variables dans l'ordre décroissant.



/* **************************************************************************** */
/* 																				*/
/*                   Operations "complexes" sur les solvers                      */
/* 																				*/
/* **************************************************************************** */


int TimeForOne(Solver B); // Renvoi la complexité pour avoir la première solution (PROBABLEMENT FAUSSE)

Solver SimplifierTypeSolver(Solver B); // Si B est de type 2, renvoi une solver equivalente où B->fils1 n'est pas de type 2.

Solver UnionSolver_tmp(Solver B1, Solver B2, int *X, int n, int sortie); // Réalise l'union de deux solvers. (Appelée par UnionSolver)

Solver UnionSolver(Solver B1, Solver B2, SysEqLin E); // Réalise l'union de deux solvers.

Solver UnionSolverBorne(Solver B1, Solver B2, SysEqLin E, int borne);

Solver UnionSpeciale(Solver B1, Solver B2, SysEqLin E); // fonction bizarre que j'avais crée pour un cas spéciale. NE PAS UTILISER

Solver UnionSolver2(Solver B1, Solver B2, SysEqLin E); // Retourne la solver B2 -> B1. PEUT PRESENTER DES PROBLEMES SI B1 n B2 NON VIDE.

int ReduireSolvers_tmp(Solver *B, int Nsolvers, SysEqLin E, int DejaTester);  // Comme ci-dessous mais la liste restreinte au "DejaTester" premiers éléments est dejà réduite.

extern int ReduireSolvers(Solver *B, int Nsolvers, SysEqLin E); // Reduit une liste de solvers en le sens que si A u B est plus interessant que A ET que B, on met à jour.

Solver * MakeSolver1(SysEqLin E, int *N1); // crée la liste réduite des solvers de complexité 1 associée à E




/* **************************************************************************** */
/* 																				*/
/*                   Améliorations des solvers                                   */
/* 																				*/
/* **************************************************************************** */



Solver PropagateSolver(Solver B, SysEqLin E, Solver *B1, int N1); // Renvoi la solver B augmentée gratuitement avec le maximum de solvers de la liste B1

Solver PropagateSolver2(Solver B, SysEqLin E, Solver *B1, int N1); // Pareil que PropagateSolver mais la mémoire ne compte pas

Solver OptimizeSolver(Solver B, SysEqLin E); // voir .c

Solver ImproveSolver(Solver B, SysEqLin E); // Renvoi une solver telle qu'avant chaque union "complexe" les deux solvers ne peuvent être augmentées gratuitement avec une solver de type 0.

Solver AjusteSolver(Solver B, SysEqLin E); // Refait la solver B par rapport à E (supprime les variables connues, ...)

Solver DiminuerMemoire(Solver B, SysEqLin E); // Essai de modifier légèrement la solver B pour diminuer la mémoire

Solver Refine(Solver B, SysEqLin E);



/* **************************************************************************** */
/* 																				*/
/*                   Fonctions diverses sur les solvers                          */
/* 																				*/
/* **************************************************************************** */
	

Equation * ExtraireEquationUnion(Solver B, SysEqLin E, int *taille); // Renvoi les equations de E(X1 u X2) - (E(X1) + E(X2)).

int EnleveFaussesEq(Solver *B1, int N1, SysEqLin E); // Prend en compte le fait que x = y soit equivalent à S(x) = S(y)

#define SOLVER_H

#endif
