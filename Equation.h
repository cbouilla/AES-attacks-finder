#ifndef EQUATION_H
#define EQUATION_H

typedef struct equation
{
  int nV;   /* #variables */
  int * __restrict var; /*  tableau contenant les variables (désordre) */
  unsigned char * __restrict coefV; /*  coefficients devant chacune des variables du tableau précedent */
  int nS;               /*  #termes en S */
  struct equation ** __restrict S;  /*  ce qu'il y a à l'intérieur des S */ 
  unsigned char * __restrict coefS; /*  coefficients devant les S-solvers */
  unsigned char * __restrict sbox;  /*  0=il s'agit de S --- 1=il s'agit de S^{-1} */
}EQUATION,*Equation;


void PrintEq(Equation);

Equation NulEq(void);

int EstNulEq(Equation);

void freeEq(Equation);

Equation copyEq(Equation);

int EgaleEq(Equation, Equation);

Equation varEq(int);

Equation mulEq(unsigned char, Equation);

Equation addEq(Equation, Equation);

int apparaitEq(Equation, int);

Equation appliqueS(Equation, unsigned char);

int dependEq(Equation, int);

Equation remplaceEq(Equation, int, Equation);

Equation inverseEq(Equation, int);

Equation * Linearise(Equation *, int, int *);

Equation * RemonteSysEq(Equation *, int, int *, int *, int);

Equation CoupeEq(Equation, int *, int);

#endif
