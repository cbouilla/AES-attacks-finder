#ifndef ALGEBRE_H
#define ALGEBRE_H

void PrintMat(unsigned char **mat, int a, int b); // Print la matrice mat de taille a x b 

void echangeligne(int, int, unsigned char **, int, int);

void echangecolonne(int, int, unsigned char **, int, int);

int Dimension(unsigned char **, int, int, int);

int Dimension_tmp(unsigned char **m, int a, int b);

int DimensionBorne_tmp(unsigned char **m, const int a, const int b, const int borne);

int Base(unsigned char **, unsigned char **, int, int);

int Somme(unsigned char **, unsigned char **, int, unsigned char **, int, int);

int Supplementaire(unsigned char **, unsigned char **, int, unsigned char **, int, int);

int Intersection(unsigned char **, unsigned char **, int, unsigned char **, int, int);

int BaseSpeciale(unsigned char **dst, unsigned char **mat, int a, int b, int Known);

#endif
