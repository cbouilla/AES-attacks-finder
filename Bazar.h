#ifndef BAZAR_H
#define BAZAR_H

#define min(a,b) ((a<b) ? a : b)
#define max(a,b) ((a>b) ? a : b)

int OrdreListe(int *L1, int t1, int *L2, int t2);

int * UnionListesTriees(int *, int, int *, int, int *);

int * IntersectionListesTriees(int *, int, int *, int, int *);

int * DifferenceListesTriees(int *, int, int *, int, int *);

int SearchElement(int, int *, int);

void Tri(int *, int);

void PrintListe(int *, int);

void TriSpecial(int *L1, int *L2, int taille);

int ContenuListesTriees(int * L1, int t1, int * L2, int t2);

int IntersectionNonVideListeTriees(int *L1, int t1, int *L2, int t2);

int Puissance(int a, int b);

int UnionListesTriees2(int *L1, int t1, int *L2, int t2, int *Final);

int NextSet(int * X, int l, int n);

//char *strdup(char *str);

#endif
