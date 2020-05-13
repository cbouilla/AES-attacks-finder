#ifndef SYSEQLIN_H

typedef struct col_coeff
{
	unsigned char coeff;
	int colonne;
}COL_COEFF;

typedef struct lignecreuse
{
	int nnz;
	COL_COEFF * __restrict elements;
	int parents;
}LIGNECREUSE,*LigneCreuse;

typedef struct matricecreuse
{
	int nombre_de_lignes;
	LigneCreuse * __restrict ligne;
	int parents;
}MATRICECREUSE,*MatriceCreuse;

typedef struct syseqlin
{
	unsigned char ** __restrict mat; // matrice de système d'équations. nombre de lignes : gen*VARIABLES. nombre de colonnes : b.
	int a; // les variables dont le numero de ligne est >= à "a" sont connues. 
	int b; // nombre de colonne.
	int gen; // nombre de générateurs = 1 + nombre de Sbox.
	int * __restrict lignevar; // lignevar[i] donne la ligne de la variable x_i. ATTENTION : dans la matrice la ligne gen*lignevar[i] correspond à x_i, gen*lignevar[i] + 1 à S(x_i), ...
	int * __restrict varligne; // inverse de lignevar
	char initialise; // vrai si sousmat est initialisée
	MatriceCreuse * __restrict sousmat; // liste de matrice permettant de calculer efficacement la sortie d'une solver
	int col_max; // numero maximal des colonnes de "sousmat"

}SYSEQLIN,*SysEqLin;


void InitialiseSousMat(SysEqLin E); // Initialise E->sousmat

int AjusteColonnes(MatriceCreuse M); // Ajuste la numerotation des colonnes de M, de 0 au nombre de colonnes de M

void freeMatriceCreuse(MatriceCreuse a); // libère la mémoire associée à une MatriceCreuse

int ExtraireEX(unsigned char **dst, int *var, int N, SysEqLin E); // Calcul E(X) (sous forme de matrice)

SysEqLin ExtraireEX2(int *var, int N, SysEqLin E); // Calcul E(X) (sous forme de SysEqLin)

SysEqLin ExtraireEX2Special(int *var, int N, SysEqLin E); // Calcul E(X) mais supprime les equations entre les variables connues.

SysEqLin EsachantX(int *var, int N, SysEqLin E); // Calcul E|_X (les variables X sont maintenant supposées connues

void PermuterVar(int a, int b, SysEqLin E); // Permute les variables a et b dans E

int ApparaitLineairement(int x, SysEqLin E); // Renvoi vrai si il exite f tel que toutes les équations de E sont de la forme alpha*f(x) = ...

void freeSysEq(SysEqLin E); // Libère la mémoire occupée par E

int TailleE(int *X, int N, SysEqLin E); // Renvoi n - dim(E(X)),  n = |X|

void PrintMatCreuse(MatriceCreuse M);

#if 1

int TailleE_Borne(int *X, int N, SysEqLin E, int borne); // Renvoi N - dim(E(X)),  N = |X|
#else

MatriceCreuse InfoSortie_UnionBorneVersion(Solver B1, Solver B2, SysEqLin E, int borne);

MatriceCreuse InfoSortie_UnionVersion(Solver B1, Solver B2, SysEqLin E);

MatriceCreuse InfoSortie_VariablesVersion(int * __restrict X, const int N, SysEqLin E);

#endif

#include "Equation.h"

SysEqLin Transform(Equation *Eq, int taille, int gen); // Transforme une système d'équations en SysEqLin

Equation * ConversionSys(SysEqLin E); // Convertit un SysEqLin en système d'équations, le résulat contient E->b équations.

Equation * ConversionSys2(int *X, int n, SysEqLin E); // Convertit un SysEqLin en système d'équations

void PrintSysEqLinManual(SysEqLin);

#define SYSEQLIN_H

#endif
