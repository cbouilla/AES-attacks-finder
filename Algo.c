//      Algo.c
//      
//      Copyright 2011 Patrick Derbez <patrick.derbez@ens.fr>
//      
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; either version 2 of the License, or
//      (at your option) any later version.
//      
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//      
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
//      MA 02110-1301, USA.


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Bazar.h"
#include "CorpsK.h"
#include "Algo.h"
#include "Search.h"
#include "SysEqLin.h"

extern int VARIABLES;

void EcrireEq(FILE *out, Equation a) // Ecrit une équation en C dans un fichier
{
	/* On pourrait peut-être factoriser ? */
	
	if (a==NULL || EstNulEq(a))
	{
		fprintf(out,"0");
	}
	else
	{
		int i;
		for (i = 0; i < a->nV; i++)
		{
			if (i!=0) fprintf(out," ^ ");
			if (a->coefV[i] != 0x01) fprintf(out,"Multiply(0x%02x,",a->coefV[i]);
			fprintf(out,"x%u",a->var[i]);
			if (a->coefV[i]!=0x01) fprintf(out,")");
		}
		if (a->nV!=0 && a->nS!=0) fprintf(out," ^ ");
		for (i = 0; i < a->nS; i++)
		{
			if (i!=0) fprintf(out," ^ ");
			if (a->coefS[i]!=0x01) fprintf(out,"Multiply(0x%02x,",a->coefS[i]);
			if (a->sbox[i]) fprintf(out,"R(");
			else fprintf(out,"S(");
			EcrireEq(out,a->S[i]);
			fprintf(out,")");
			if (a->coefS[i]!=0x01) fprintf(out,")");
		}
	}
}

void copyAlgo(FILE *dst, char *name_src, int tab) // Copie un fichier avec indentation
{
	FILE *src = fopen(name_src,"r");
	int j=1;
	char c;
	while ((c = fgetc(src)) != EOF)
	{
		if (j)
		{
			int i;
			for (i = 0; i < tab; i++) fprintf(dst,"	");
			j=0;
		}
		fputc(c, dst);
		if (c=='\n') j=1;
	}
	fclose(src);
}


int EcrireAlgo_tmp(char *s1, char *s2, FILE *outH, Solver B, SysEqLin E, int alea, int *sur, unsigned long int *mem, unsigned long int *mem_max, ALGORITHME *A, int Nb)
{
	static int a=0, b=0;

	int tab=0,i;
	
	for (i = 0; i < Nb; i++) // On a déjà un algo pour la solver
	{
		if (SameSolvers(B,A[i].B))
		{
			int al = a++;
			sprintf(s1,"tmp/out1_%d.txt",al);
			FILE *out1 = fopen(s1,"w+");
			copyAlgo(out1,A[i].debut,0);
			fclose(out1);
			sprintf(s2,"tmp/out2_%d.txt",al);
			FILE *out2 = fopen(s2,"w+");
			copyAlgo(out2,A[i].fin,0);
			fclose(out2);
			*sur=A[i].sur;
			*mem=A[i].mem;
			*mem_max=A[i].mem_max;
			return A[i].tab;
		}
	}
	
	if (B->type==0) // la solver a une variable
	{
		if (E->lignevar[B->var[0]]*E->gen>=E->a) // si elle est connue on ne fait rien (ne devrait pas arriver)
		{
			int al = a++;
			sprintf(s1,"tmp/out1_%d.txt",al);
			FILE *out1 = fopen(s1,"w+");
			fclose(out1);
			sprintf(s2,"tmp/out2_%d.txt",al);
			FILE *out2 = fopen(s2,"w+");
			fclose(out2);
			*sur=1;
			*mem=0;
			*mem_max=0;
			return 0;
		}
		/* On calcul les equations */
		SysEqLin F=ExtraireEX2Special(B->var,B->Nvar,E);
		Equation *Eq=ConversionSys2(B->var,B->Nvar,F);
		int cas=0,k,j=0;
		{
			for (i = 0; i < F->b; i++)
			{
				if (!EstNulEq(Eq[i]))
				{
					cas=1;
					if (apparaitEq(Eq[i],B->var[0]) == 1)
					{
						cas=2;
						k=i;
						break;
					}
				}
			}
		}
		int al=a++;
		sprintf(s1,"tmp/out1_%d.txt",al);
		FILE *out1 = fopen(s1,"w+");
		sprintf(s2,"tmp/out2_%d.txt",al);
		FILE *out2 = fopen(s2,"w+");
		switch (cas)
		{
			case 0: // Il n'y a pas d'équations
				if (alea) fprintf(out1,"const unsigned char x%d=1+(rand()%%255);\n",B->var[0]); // un peu naze
				else
				{
					fprintf(out1,"int i_%d;\n",b);
					fprintf(out1,"for (i_%d = 0; i_%d < 0x100; i_%d++)\n",b,b,b);
					fprintf(out1,"{\n");
					fprintf(out1,"	const unsigned char x%d=i_%d;\n",B->var[0],b);
					
					fprintf(out2,"}\n");
					b++;
				}
				if (alea) tab=0;
				else tab=1;
				*sur=1; // on sait exactement le nombre de solutions.
				break;
			case 1: // Il y a des equations mais on doit les vérifier à posteriori
				fprintf(out1,"int i_%d;\n",b);
				fprintf(out1,"for (i_%d = 0; i_%d < 0x100; i_%d++)\n",b,b,b);
				fprintf(out1,"{\n");
				fprintf(out1,"	const unsigned char x%d=i_%d;\n",B->var[0],b);
				for (i = 0; i < F->b; i++)
				{
					if (!EstNulEq(Eq[i]))
					{
						if (j==0)
						{
							fprintf(out1,"	if ((");
							j++;
						}
						else fprintf(out1," && (");
						EcrireEq(out1,Eq[i]);
						fprintf(out1,")==(");
						EcrireEq(out1,Eq[i+F->b]);
						fprintf(out1,")");
					}
				}
				fprintf(out1,")\n");
				fprintf(out1,"	{\n");
				tab=2;
				fprintf(out2,"	}\n");
				fprintf(out2,"}\n");
				*sur=0;
				b++;
				break;
			case 2: // Il y a des equations mais on peut obtenir la solution d'une directement
				j=F->b-1;
				if (Eq[k]->nS>0)
				{
					unsigned char p=Inverse(Eq[k]->coefS[0]);
					Equation tmp=mulEq(p,Eq[k]);
					freeEq(Eq[k]);
					Eq[k]=tmp;
					tmp=mulEq(p,Eq[k+F->b]);
					freeEq(Eq[k+F->b]);
					Eq[k+F->b]=tmp;
					unsigned char m=1-Eq[k]->sbox[0];
					tmp=appliqueS(Eq[k],m);
					freeEq(Eq[k]);
					Eq[k]=Eq[j];
					Eq[j]=tmp;
					tmp=appliqueS(Eq[k+F->b],m);
					freeEq(Eq[k+F->b]);
					Eq[k+F->b]=Eq[F->b+j];
					Eq[F->b+j]=tmp;
				}
				else
				{
					unsigned char p=Inverse(Eq[k]->coefV[0]);
					Equation tmp=mulEq(p,Eq[k]);
					freeEq(Eq[k]);
					Eq[k]=Eq[j];
					Eq[j]=tmp;
					tmp=mulEq(p,Eq[k+F->b]);
					freeEq(Eq[k+F->b]);
					Eq[k+F->b]=Eq[F->b+j];
					Eq[F->b+j]=tmp;
				}
				fprintf(out1,"const unsigned char x%d=",B->var[0]);
				EcrireEq(out1,Eq[j+F->b]);
				fprintf(out1,";\n");
				k=0;
				for (i = 0; i < j; i++)
				{
					if (!EstNulEq(Eq[i]))
					{
						if (k==0)
						{
							fprintf(out1,"if ((");
							k++;
						}
						else fprintf(out1," && (");
						EcrireEq(out1,Eq[i]);
						fprintf(out1,")==(");
						EcrireEq(out1,Eq[i+F->b]);
						fprintf(out1,")");
					}
				}
				if (k==0)
				{
					tab=0;
					*sur=1;
				}
				else
				{
					fprintf(out1,")\n");
					fprintf(out1,"{\n");
					tab=1;
					fprintf(out2,"}\n");
					*sur=0;
				}
				break;
			default:
				break;
		}
		for (i = 0; i < 2*F->b; i++) freeEq(Eq[i]);
		free(Eq);
		freeSysEq(F);
		printf("\r	- write subroutine (%d)",a);
		*mem=0;
		*mem_max=0;
		fclose(out1);
		fclose(out2);
		return tab;
	}
	
	if (B->type==2 || B->fils2->sortie<=0)
	{
		char *s12 = malloc(30*sizeof(char));
		int sur1;
		unsigned long int mem1,mem_max1;
		int tab1=EcrireAlgo_tmp(s1,s12,outH,B->fils2,E,alea,&sur1,&mem1,&mem_max1,A,Nb);
		SysEqLin F=EsachantX(B->fils2->var,B->fils2->Nvar,E);
		char *s21 = malloc(30*sizeof(char));
		char *s22 = malloc(30*sizeof(char));
		int sur2;
		unsigned long int mem2,mem_max2;
		int tab2=EcrireAlgo_tmp(s21,s22,outH,B->fils1,F,alea,&sur2,&mem2,&mem_max2,A,Nb);
		freeSysEq(F);
		/* On fusionne les algos */
		FILE *out1 = fopen(s1,"a+");
		copyAlgo(out1,s21,tab1); // On les copie avec la bonne indentation
		fclose(out1);
		remove(s21);
		free(s21);
		int al=a++;
		sprintf(s2,"tmp/out2_%d.txt",al);
		FILE *out2 = fopen(s2,"w+");
		copyAlgo(out2,s22,tab1);
		remove(s22);
		free(s22);
		copyAlgo(out2,s12,0);
		fclose(out2);
		remove(s12); // On les supprime
		free(s12);
		tab=tab1+tab2; // On calcule la nouvelle indentation
		*sur = (sur1 && sur2); // Le nombre de solution est-il sûr ?
		*mem=mem1+mem2; // On calcul la mémoire ...
		*mem_max=max(mem_max1,mem1+mem_max2); // ... et la memoire maximum
		return tab;
	}
	if (B->fils1->sortie<=0)
	{
		char *s12 = malloc(30*sizeof(char));;
		int sur1;
		unsigned long int mem1,mem_max1;
		int tab1=EcrireAlgo_tmp(s1,s12,outH,B->fils1,E,alea,&sur1,&mem1,&mem_max1,A,Nb);
		SysEqLin F=EsachantX(B->fils1->var,B->fils1->Nvar,E);
		char *s21 = malloc(30*sizeof(char));
		char *s22 = malloc(30*sizeof(char));
		int sur2;
		unsigned long int mem2,mem_max2;
		int tab2=EcrireAlgo_tmp(s21,s22,outH,B->fils2,F,alea,&sur2,&mem2,&mem_max2,A,Nb);
		freeSysEq(F);
		FILE *out1 = fopen(s1,"a+");
		copyAlgo(out1,s21,tab1);
		fclose(out1);
		remove(s21);
		free(s21);
		int al=a++;
		sprintf(s2,"tmp/out2_%d.txt",al);
		FILE *out2 = fopen(s2,"w+");
		copyAlgo(out2,s22,tab1);
		copyAlgo(out2,s12,0);
		fclose(out2);
		remove(s12);
		remove(s22);
		free(s12);
		free(s22);
		tab=tab1+tab2;
		*sur = (sur1 && sur2);
		*mem=mem1+mem2;
		*mem_max=max(mem_max1,mem1+mem_max2);
		return tab;
	}
	if (B->type==1)
	{
		Equation *Eq;
		int n;
		if (B->fils2->Nvar==1) // Il y a une seule variable, on va voir si on peut l'obtenir directement (sans table autre que S et S^-1)
		{
			if (SearchElement(B->fils2->var[0],B->fils1->var,B->fils1->Nvar)!=-1) return tab=EcrireAlgo_tmp(s1,s2,outH,B->fils1,E,alea,sur,mem,mem_max,A,Nb);
			Eq=ExtraireEquationUnion(B,E,&n);
			for (i = 0; i < n; i++) if (apparaitEq(Eq[i],B->fils2->var[0])==1) break;
			if (i<n) // On peut l'obtenir directement
			{
				char *s12 = malloc(30*sizeof(char));;
				int sur1;
				unsigned long int mem1,mem_max1;
				int tab1=EcrireAlgo_tmp(s1,s12,outH,B->fils1,E,alea,&sur1,&mem1,&mem_max1,A,Nb);
				int j=n-1;
				if (Eq[i]->nS>0)
				{
					unsigned char p=Inverse(Eq[i]->coefS[0]);
					Equation tmp=mulEq(p,Eq[i]);
					freeEq(Eq[i]);
					Eq[i]=tmp;
					tmp=mulEq(p,Eq[i+n]);
					freeEq(Eq[i+n]);
					Eq[i+n]=tmp;
					unsigned char m=1-Eq[i]->sbox[0];
					tmp=appliqueS(Eq[i],m);
					freeEq(Eq[i]);
					Eq[i]=Eq[j];
					Eq[j]=tmp;
					tmp=appliqueS(Eq[i+n],m);
					freeEq(Eq[i+n]);
					Eq[i+n]=Eq[n+j];
					Eq[n+j]=tmp;
				}
				else
				{
					unsigned char p=Inverse(Eq[i]->coefV[0]);
					Equation tmp=mulEq(p,Eq[i]);
					freeEq(Eq[i]);
					Eq[i]=Eq[j];
					Eq[j]=tmp;
					tmp=mulEq(p,Eq[i+n]);
					freeEq(Eq[i+n]);
					Eq[i+n]=Eq[j+n];
					Eq[j+n]=tmp;
				}
				char *s21 = malloc(30*sizeof(char));
				char *s22 = malloc(30*sizeof(char));
				int al = a++;
				sprintf(s21,"tmp/out21_%d.txt",al);
				sprintf(s22,"tmp/out22_%d.txt",al);
				FILE *out21=fopen(s21,"w+");
				FILE *out22=fopen(s22,"w+");
				fprintf(out21,"const unsigned char ");
				EcrireEq(out21,Eq[j]);
				fprintf(out21,"=");
				EcrireEq(out21,Eq[n+j]);
				fprintf(out21,";\n");
				int tab2;
				if (n>1)
				{
					fprintf(out21,"if ((");
					EcrireEq(out21,Eq[0]);
					fprintf(out21,")==(");
					EcrireEq(out21,Eq[n]);
					fprintf(out21,")");
					for (i = 1; i < j; i++)
					{
						fprintf(out21," && (");
						EcrireEq(out21,Eq[i]);
						fprintf(out21,")==(");
						EcrireEq(out21,Eq[n+i]);
						fprintf(out21,")");
					}
					fprintf(out21,")\n");
					fprintf(out21,"{\n");
					tab2=1;
					fprintf(out22,"}\n");
					*sur=0;
				}
				else
				{
					tab2=0;
					*sur=sur1;
				}
				fclose(out21);
				fclose(out22);
				FILE *out1 = fopen(s1,"a+");
				copyAlgo(out1,s21,tab1);
				fclose(out1);
				sprintf(s2,"tmp/out2_%d.txt",al);
				FILE *out2 = fopen(s2,"w+");
				copyAlgo(out2,s22,tab1);
				copyAlgo(out2,s12,0);
				fclose(out2);
				remove(s12);
				remove(s21);
				remove(s22);
				free(s12);
				free(s21);
				free(s22);
				for (i = 0; i < 2*n; i++) freeEq(Eq[i]);
				free(Eq);
				printf("\r	- write subroutine (%d)",a);
				*mem=mem1;
				*mem_max=mem_max1;
				return tab1+tab2;
			}
		}
		else Eq=ExtraireEquationUnion(B,E,&n);
		
		/* A priori on va créer une table */
		
		int N1;
		int *X1=IntersectionListesTriees(B->fils1->var,B->fils1->Nvar,B->fils2->var,B->fils2->Nvar,&N1); // Les variables communes aux fils
		int N2;
		//int *X2=GuessMinimum(X1,N1,N1,0,E,&N2); // Le plus petit ensemble de variables permettant d'obtenir X1
		int *X2=GuessMinimumLight(X1,N1,E,&N2);
		int check=n+N2; // Le nombre de check (peut être supérieur à la réalité)
		
		if (check==0) // Si il n'y en a pas -> passage au séquentiel
		{
			int t1=max(B->fils2->sortie,0) + B->fils1->temps;
			int t2=max(B->fils1->sortie,0) + B->fils2->temps;
			if (t1<t2 || (t1==t2 && B->fils2->Nvar>=B->fils1->Nvar))
			{
				for (i = 0; i < 2*n; i++) freeEq(Eq[i]);
				free(Eq);
				free(X1);
				free(X2);
				char *s12 = malloc(30*sizeof(char));
				int sur1;
				unsigned long int mem1,mem_max1;
				int tab1=EcrireAlgo_tmp(s1,s12,outH,B->fils2,E,alea,&sur1,&mem1,&mem_max1,A,Nb);
				SysEqLin F=EsachantX(B->fils2->var,B->fils2->Nvar,E);
				char *s21 = malloc(30*sizeof(char));
				char *s22 = malloc(30*sizeof(char));
				int sur2;
				unsigned long int mem2,mem_max2;
				int tab2=EcrireAlgo_tmp(s21,s22,outH,B->fils1,F,alea,&sur2,&mem2,&mem_max2,A,Nb);
				freeSysEq(F);
				FILE *out1 = fopen(s1,"a+");
				copyAlgo(out1,s21,tab1);
				fclose(out1);
				int al=a++;
				sprintf(s2,"tmp/out2_%d.txt",al);
				FILE *out2 = fopen(s2,"w+");
				copyAlgo(out2,s22,tab1);
				copyAlgo(out2,s12,0);
				fclose(out2);
				remove(s12);
				remove(s21);
				remove(s22);
				free(s12);
				free(s21);
				free(s22);
				tab=tab1+tab2;
				*sur = (sur1 && sur2);
				*mem=mem1+mem2;
				*mem_max=max(mem_max1,mem1+mem_max2);
				printf("\r	- write subroutine (%d)",a);
				return tab;
			}
			else
			{
				for (i = 0; i < 2*n; i++) freeEq(Eq[i]);
				free(Eq);
				free(X1);
				free(X2);
				char *s12 = malloc(30*sizeof(char));
				int sur1;
				unsigned long int mem1,mem_max1;
				int tab1=EcrireAlgo_tmp(s1,s12,outH,B->fils1,E,alea,&sur1,&mem1,&mem_max1,A,Nb);
				SysEqLin F=EsachantX(B->fils1->var,B->fils1->Nvar,E);
				char *s21 = malloc(30*sizeof(char));
				char *s22 = malloc(30*sizeof(char));
				int sur2;
				unsigned long int mem2,mem_max2;
				int tab2=EcrireAlgo_tmp(s21,s22,outH,B->fils2,F,alea,&sur2,&mem2,&mem_max2,A,Nb);
				freeSysEq(F);
				FILE *out1 = fopen(s1,"a+");
				copyAlgo(out1,s21,tab1);
				fclose(out1);
				int al=a++;
				sprintf(s2,"tmp/out2_%d.txt",al);
				FILE *out2 = fopen(s2,"w+");
				copyAlgo(out2,s22,tab1);
				copyAlgo(out2,s12,0);
				fclose(out2);
				remove(s12);
				remove(s21);
				remove(s22);
				free(s12);
				free(s21);
				free(s22);
				tab=tab1+tab2;
				*sur = (sur1 && sur2);
				*mem=mem1+mem2;
				*mem_max=max(mem_max1,mem1+mem_max2);
				printf("\r	- write subroutine (%d)",a);
				return tab;
			}
		}
		
		char *s11 = malloc(30*sizeof(char));
		char *s12 = malloc(30*sizeof(char));
		int sur1;
		unsigned long int mem1,mem_max1;
		int tab1=EcrireAlgo_tmp(s11,s12,outH,B->fils1,E,alea,&sur1,&mem1,&mem_max1,A,Nb);
		char *s21 = malloc(30*sizeof(char));
		char *s22 = malloc(30*sizeof(char));
		int sur2;
		unsigned long int mem2,mem_max2;
		int tab2=EcrireAlgo_tmp(s21,s22,outH,B->fils2,E,alea,&sur2,&mem2,&mem_max2,A,Nb);

		Solver B1=B->fils1;
		Solver B2=B->fils2;
		Solver B3=copySolver(B);

		/* Calcul de la taille de la structure utilisée */
		int bit1=8;
		while (B2->sortie*8>bit1) bit1=2*bit1;
		int bit2=8;
		while (min(check,B2->sortie)*8>bit2) bit2=2*bit2;

		int N3;
		int *X3=DifferenceListesTriees(B2->var,B2->Nvar,X1,N1,&N3); // Les variables de B2 privée de celle de B1
		
		/* On va chercher à réduire le nombre de variables stockées */
		SysEqLin F=EsachantX(B1->var,B1->Nvar,E); // On suppose que l'on connait B1
		int *X4,N4;
		//if (B2->sortie>2) X4=GuessMinimum(X3,N3,N3,0,F,&N4); // Le plus petit ensemble de guess permettant d'obtenir X3
		if (B2->sortie>2) X4=GuessMinimumLight(X3,N3,F,&N4);
		else
		{
			X4=NULL;
			N4=0;
		}
		Solver B_tmp=NULL, B4;
		int byte2 = (N3+check-min(B2->sortie,check) + (bit1/4) + 1)/4;
		int byte3 = (N4 + (bit1/4) + 1)/4;
		if (N4>0 && N4<N3 && B2->sortie>2 && byte3<byte2) // Si le gain en place est intéressant
		{
			int byte1 = (N4 + (bit1/4) + 1)%4;
			if (byte1) byte1=4-byte1;
			int N5;
			do
			{
				B4=copySolver(B1);
				for (i = 0; i < N4; i++)
				{
					B_tmp=BaseSolver(X4[i],E);
					Solver B5=UnionSolver(B4,B_tmp,E);
					freeSolver(B_tmp);
					freeSolver(B4);
					B4=B5;
				}
				int *X5=DifferenceListesTriees(X3,N3,B4->var,B4->Nvar,&N5);
				Solver *L=malloc(N5*sizeof(Solver));
				for (i = 0; i < N5; i++) L[i]=BaseSolver(X5[i],E);
				if (byte1) // Padding
				{
					for (i = 0; i < min(byte1,N5); i++)
					{
						B_tmp=BaseSolver(X5[i],E);
						Solver B5=UnionSolver(B4,B_tmp,E);
						freeSolver(B_tmp);
						freeSolver(B4);
						B4=B5;
					}
					int *X6;
					X6=UnionListesTriees(X4,N4,X5,min(byte1,N5),&N4);
					free(X4);
					X4=X6;
				}
				/* A ce stade, B4 sera la solver que l'on obtiendra en regardant dans la table à partir de B1 */
				B_tmp=PropagateSolver2(B4,E,L,N5); 
				for (i = 0; i < N5; i++) freeSolver(L[i]);
				free(L);
				free(X5);
				X5=DifferenceListesTriees(X3,N3,B_tmp->var,B_tmp->Nvar,&N5);
				if (N5>0) 
				{
					printf("\nSomething is strange... - 636 - Algo.c : %d\n",N5);
					getchar();
				}
				else free(X5);
			} while (N5>0);
		}
		/* On ouvre pleins de fichiers temporaires */
		char *s31 = malloc(30*sizeof(char));
		char *s32 = malloc(30*sizeof(char));
		char *s41 = malloc(30*sizeof(char));
		char *s42 = malloc(30*sizeof(char));
		char *s43 = malloc(30*sizeof(char));
		char *s44 = malloc(30*sizeof(char));
		char *s45 = malloc(30*sizeof(char));
		char *s46 = malloc(30*sizeof(char));
		char *s1_tmp = malloc(30*sizeof(char));
		char *s2_tmp = malloc(30*sizeof(char));
		int al = a++;
		sprintf(s31,"tmp/out31_%d.txt",al);
		sprintf(s32,"tmp/out32_%d.txt",al);
		sprintf(s41,"tmp/out41_%d.txt",al);
		sprintf(s42,"tmp/out42_%d.txt",al);
		sprintf(s43,"tmp/out43_%d.txt",al);
		sprintf(s44,"tmp/out44_%d.txt",al);
		sprintf(s45,"tmp/out45_%d.txt",al);
		sprintf(s46,"tmp/out46_%d.txt",al);
		sprintf(s1_tmp,"tmp/out1_tmp_%d.txt",al);
		sprintf(s2_tmp,"tmp/out2_tmp_%d.txt",al);
		
		FILE *out31=fopen(s31,"w+");
		FILE *out32=fopen(s32,"w+");
		FILE *out41=fopen(s41,"w+");
		FILE *out42=fopen(s42,"w+");
		FILE *out43=fopen(s43,"w+");
		FILE *out44=fopen(s44,"w+");
		FILE *out45=fopen(s45,"w+");
		FILE *out46=fopen(s46,"w+");
		FILE *out1_tmp=fopen(s1_tmp,"w+");
		FILE *out2_tmp=fopen(s2_tmp,"w+");
		freeSysEq(F);
		int stock; // nombre de bytes stockés
		if (B_tmp==NULL) stock=N3+check-min(B2->sortie,check);
		else stock=N4;

		/* On crée les structures */
		fprintf(outH,"typedef struct position%d_%d\n",b,stock);
		fprintf(outH,"{\n");
		fprintf(outH,"	uint8_t a;\n");
		fprintf(outH,"	uint%d_t b;\n",bit1);
		fprintf(outH,"}POSITION%d_%d,*Position%d_%d;\n\n",b,stock,b,stock);
		 
		fprintf(outH,"typedef struct chaine%d_%d\n",b,stock);
		fprintf(outH,"{\n");
		for (i = 0; i < stock; i++) fprintf(outH,"	unsigned char d%d;\n",i);
		fprintf(outH,"	uint8_t a;\n");
		fprintf(outH,"	uint%d_t b;\n",bit1);
		fprintf(outH,"}CHAINE%d_%d,*Chaine%d_%d;\n\n",b,stock,b,stock);
		
		fprintf(out41,"\n");

		/*On définit la taille de la table */
		fprintf(outH,"#define taille1_%d 0x1",b);
		for (i = 0; i < min(check,B2->sortie); i++) fprintf(outH,"00");
		fprintf(outH,"L\n");
		fprintf(outH,"#define taille2_%d 0x1",b);
		for (i = 0; i < B2->sortie; i++) fprintf(outH,"00");
		fprintf(outH,"L\n");
		if (sur2) fprintf(out41,"Chaine%d_%d espace_%d[2];\n",b,stock,b); // Le nombre d'éléments dans la table est connu ...
		else
		{
			// ... ou pas.
			fprintf(out41,"CHAINE%d_%d **espace_%d;\n",b,stock,b);
			fprintf(out41,"espace_%d=malloc(2*sizeof(Chaine%d_%d));\n",b,b,stock);
		}
		if (min(check,B2->sortie)<=2) fprintf(out41,"POSITION%d_%d L_%d[taille1_%d];\n",b,stock,b,b); // La table est suffisament petite pour l'allouer à la compilation ...
		else
		{
			// ... ou pas.
			fprintf(out41,"POSITION%d_%d *L_%d;\n",b,stock,b);
			fprintf(out41,"L_%d=malloc(taille1_%d*sizeof(POSITION%d_%d));\n",b,b,b,stock);
		}
		if (sur2 && B2->sortie<=2)
		{
			// Ici aussi ...
			fprintf(out41,"CHAINE%d_%d t_%d[taille2_%d];\n",b,stock,b,b);
			fprintf(out41,"espace_%d[1]=t_%d;\n",b,b);
		}
		else fprintf(out41,"espace_%d[1]=malloc(taille2_%d*sizeof(CHAINE%d_%d));\n",b,b,b,stock); // ... mais pas là.
		fprintf(out41,"\n");		
		
		if (!sur2 || B2->sortie>2) fprintf(out41,"uint8_t Alloue_%d=1;\n",b);
		fprintf(out41,"unsigned long int pos_%d;\n",b);
		fprintf(out41,"for (pos_%d = 0; pos_%d < taille1_%d ; pos_%d++) L_%d[pos_%d].a=0;\n",b,b,b,b,b,b);
		fprintf(out41,"unsigned long int cpt_%d=0;\n",b);

		/* L'index de la table (check) */
		int j=1;
		for (i = 0; i < min(n,B2->sortie); i++)
		{
			if (j)
			{
				fprintf(out42,"pos_%d=(",b);
				j=0;
			}
			else fprintf(out42,"pos_%d=(pos_%d << 8) ^ (",b,b);
			EcrireEq(out42,Eq[i]);
			fprintf(out42,");\n");
		}
		for (i = 0; i < min(N2,B2->sortie-n); i++)
		{
			if (j)
			{
				fprintf(out42,"pos_%d=x%d;\n",b,X2[i]);
				j=0;
			}
			else fprintf(out42,"pos_%d=(pos_%d << 8) ^ x%d;\n",b,b,X2[i]);
		}
		if (j) fprintf(out42,"pos_%d=0; //nombre de check attendu = %d\n",b,B1->sortie+B2->sortie-B3->sortie);
		if (!sur2)
		{
			/* On prévoit de réallouer au cas où */
			fprintf(out42,"if (cpt_%d==taille2_%d)\n",b,b);
			fprintf(out42,"{\n");
			fprintf(out42,"	cpt_%d=0;\n",b);
			fprintf(out42,"	Alloue_%d++;\n",b);
			fprintf(out42,"	espace_%d=realloc(espace_%d,(Alloue_%d+1)*sizeof(Chaine%d_%d));\n",b,b,b,b,stock);
			fprintf(out42,"	espace_%d[Alloue_%d]=malloc(taille2_%d*sizeof(CHAINE%d_%d));\n",b,b,b,b,stock);
			fprintf(out42,"}\n");
			int pos=0; 
			if (B_tmp==NULL)
			{
				for (i = 0; i < N3; i++) fprintf(out42,"espace_%d[Alloue_%d][cpt_%d].d%d=x%d;\n",b,b,b,pos++,X3[i]);
				for (i = min(n,B2->sortie); i < n; i++)
				{
					fprintf(out42,"espace_%d[Alloue_%d][cpt_%d].d%d=",b,b,b,pos++);
					EcrireEq(out42,Eq[i]);
					fprintf(out42,";\n");
				}
				for (i = min(N2,max(B2->sortie-n,0)); i < N2; i++) fprintf(out42,"espace_%d[Alloue_%d][cpt_%d].d%d=x%d;\n",b,b,b,pos++,X2[i]);
			}
			else for (i = 0; i < N4; i++) fprintf(out42,"espace_%d[Alloue_%d][cpt_%d].d%d=x%d;\n",b,b,b,pos++,X4[i]);
			fprintf(out42,"espace_%d[Alloue_%d][cpt_%d].a=L_%d[pos_%d].a;\n",b,b,b,b,b);
			fprintf(out42,"espace_%d[Alloue_%d][cpt_%d].b=L_%d[pos_%d].b;\n",b,b,b,b,b);
		}
		else
		{
			int pos=0; 
			if (B_tmp==NULL)
			{
				for (i = 0; i < N3; i++) fprintf(out42,"espace_%d[1][cpt_%d].d%d=x%d;\n",b,b,pos++,X3[i]);
				for (i = min(n,B2->sortie); i < n; i++)
				{
					fprintf(out42,"espace_%d[1][cpt_%d].d%d=",b,b,pos++);
					EcrireEq(out42,Eq[i]);
					fprintf(out42,";\n");
				}
				for (i = min(N2,max(B2->sortie-n,0)); i < N2; i++) fprintf(out42,"espace_%d[1][cpt_%d].d%d=x%d;\n",b,b,pos++,X2[i]);
			}
			else for (i = 0; i < N4; i++) fprintf(out42,"espace_%d[1][cpt_%d].d%d=x%d;\n",b,b,pos++,X4[i]);
			fprintf(out42,"espace_%d[1][cpt_%d].a=L_%d[pos_%d].a;\n",b,b,b,b);
			fprintf(out42,"espace_%d[1][cpt_%d].b=L_%d[pos_%d].b;\n",b,b,b,b);
		}
		
		
		if (!sur2) fprintf(out42,"L_%d[pos_%d].a=Alloue_%d;\n",b,b,b);
		else fprintf(out42,"L_%d[pos_%d].a=1;\n",b,b);
		fprintf(out42,"L_%d[pos_%d].b=cpt_%d;\n",b,b,b);
		fprintf(out42,"cpt_%d++;\n",b);

		j=1;
		/* On calcul l'index pour la solver qu'on énumère */
		for (i = 0; i < min(n,B2->sortie); i++)
		{
			if (j)
			{
				fprintf(out44,"pos_%d=(",b);
				j=0;
			}
			else fprintf(out44,"pos_%d=(pos_%d << 8) ^ (",b,b);
			EcrireEq(out44,Eq[i+n]);
			fprintf(out44,");\n");
		}
		for (i = 0; i < min(N2,B2->sortie-n); i++)
		{
			if (j)
			{
				fprintf(out44,"pos_%d=x%d;\n",b,X2[i]);
				j=0;
			}
			else fprintf(out44,"pos_%d=(pos_%d << 8) ^ x%d;\n",b,b,X2[i]);
		}
		if (j) fprintf(out44,"pos_%d=0;\n",b);
		fprintf(out44,"POSITION%d_%d tmp2_%d=L_%d[pos_%d];\n",b,stock,b,b,b);
		fprintf(out44,"while (tmp2_%d.a) /* Entrée : %d*/\n",b,B1->sortie);
		fprintf(out44,"{\n");
		/* On "charge" les valeurs de la table */
		char *s;
		if (!sur2)
		{
			s=malloc(50*sizeof(char));
			sprintf(s,"tmp2_%d.a",b);
		}
		else
		{
			s=malloc(2*sizeof(char));
			sprintf(s,"1");
		}
		int tab4;	
		if (B_tmp==NULL)
		{
			for (i = 0; i < N3; i++) fprintf(out44,"	const unsigned char x%d=espace_%d[%s][tmp2_%d.b].d%d;\n",X3[i],b,s,b,i);
			if (stock>N3)
			{
				fprintf(out44,"	if (");
				j=0;
				int pos=N3;
				for (i = min(n,B2->sortie); i < n; i++)
				{
					if (j>0) fprintf(out44," && ");
					else j++;
					fprintf(out44,"espace_%d[%s][tmp2_%d.b].d%d==(",b,s,b,pos++);
					EcrireEq(out44,Eq[i+n]);
					fprintf(out44,")");
				}
				for (i = min(N2,max(B2->sortie-n,0)); i < N2; i++)
				{
					if (j>0) fprintf(out44," && ");
					else j++;
					fprintf(out44,"espace_%d[%s][tmp2_%d.b].d%d==x%d",b,s,b,pos++,X2[i]);
				}
				fprintf(out44,")\n");
				fprintf(out44,"	{\n");
				if (!sur2)
				{
					fprintf(out44,"		uint8_t a_%d = espace_%d[tmp2_%d.a][tmp2_%d.b].a;\n",b,b,b,b);
					fprintf(out44,"		tmp2_%d.b=espace_%d[tmp2_%d.a][tmp2_%d.b].b;\n",b,b,b,b);
					fprintf(out44,"		tmp2_%d.a=a_%d;\n",b,b);
				}
				else
				{
					fprintf(out44,"		tmp2_%d.a=espace_%d[1][tmp2_%d.b].a;\n",b,b,b);
					fprintf(out44,"		tmp2_%d.b=espace_%d[1][tmp2_%d.b].b;\n",b,b,b);
				}				
				tab4=2;
				fprintf(out45,"	}\n");
				fprintf(out45,"	else\n");
				fprintf(out45,"	{\n");
				if (!sur2)
				{
					fprintf(out45,"		uint8_t a_%d = espace_%d[tmp2_%d.a][tmp2_%d.b].a;\n",b,b,b,b);
					fprintf(out45,"		tmp2_%d.b=espace_%d[tmp2_%d.a][tmp2_%d.b].b;\n",b,b,b,b);
					fprintf(out45,"		tmp2_%d.a=a_%d;\n",b,b);
				}
				else
				{
					fprintf(out45,"		tmp2_%d.a=espace_%d[1][tmp2_%d.b].a;\n",b,b,b);
					fprintf(out45,"		tmp2_%d.b=espace_%d[1][tmp2_%d.b].b;\n",b,b,b);
				}
				fprintf(out45,"	}\n");
			}
			else
			{
				tab4=1;
				if (!sur2)
				{
					fprintf(out44,"	uint8_t a_%d = espace_%d[tmp2_%d.a][tmp2_%d.b].a;\n",b,b,b,b);
					fprintf(out44,"	tmp2_%d.b=espace_%d[tmp2_%d.a][tmp2_%d.b].b;\n",b,b,b,b);
					fprintf(out44,"	tmp2_%d.a=a_%d;\n",b,b);
				}
				else
				{
					fprintf(out44,"	tmp2_%d.a=espace_%d[1][tmp2_%d.b].a;\n",b,b,b);
					fprintf(out44,"	tmp2_%d.b=espace_%d[1][tmp2_%d.b].b;\n",b,b,b);
				}	
			}
		}
		else
		{
			for (i = 0; i < N4; i++) fprintf(out44,"	const unsigned char x%d=espace_%d[%s][tmp2_%d.b].d%d;\n",X4[i],b,s,b,i);
			if (!sur2)
			{
				fprintf(out44,"	uint8_t a_%d = espace_%d[tmp2_%d.a][tmp2_%d.b].a;\n",b,b,b,b);
				fprintf(out44,"	tmp2_%d.b=espace_%d[tmp2_%d.a][tmp2_%d.b].b;\n",b,b,b,b);
				fprintf(out44,"	tmp2_%d.a=a_%d;\n",b,b);
			}
			else
			{
				fprintf(out44,"	tmp2_%d.a=espace_%d[1][tmp2_%d.b].a;\n",b,b,b);
				fprintf(out44,"	tmp2_%d.b=espace_%d[1][tmp2_%d.b].b;\n",b,b,b);
			}
			if (check>B2->sortie) // Il y a plus de check que nécessaire, on vérifie ceux qui restent
			{
				for (i = 0; i < 2*n; i++) freeEq(Eq[i]);
				free(Eq);
				SysEqLin G=EsachantX(B4->var,B4->Nvar,E);
				F=ExtraireEX2Special(X4,N4,G);
				freeSysEq(G);
				Eq=ConversionSys2(X4,N4,F);
				freeSysEq(F);
				n=F->b;
				j=1;
				for (i = 0; i < n; i++)
				{
					if (!EstNulEq(Eq[i]))
					{
						if (j)
						{
							fprintf(out44,"	if((");
							j=0;
						}
						else fprintf(out44," && (");
						EcrireEq(out44,Eq[i]);
						fprintf(out44,")==(");
						EcrireEq(out44,Eq[i+n]);
						fprintf(out44,")");
					}					
				}
				if (!j)
				{
					fprintf(out44,")\n");
					fprintf(out44,"	{\n");
					tab4=2;
					fprintf(out45,"	}\n");
				}
				else tab4=1;				
			}
			else tab4=1;
		}
		free(s);
		fprintf(out45,"}\n");
		/* On libère la table */
		if (!sur2 || B2->sortie>2)
		{
			fprintf(out46,"int j_%d;\n",b);
			fprintf(out46,"for (j_%d = 1; j_%d <= Alloue_%d; j_%d++) free(espace_%d[j_%d]);\n",b,b,b,b,b,b);
		}
		if (!sur2) fprintf(out46,"free(espace_%d);\n",b);
		if (min(check,B2->sortie)>2) fprintf(out46,"free(L_%d);\n",b);
		fclose(out41);
		fclose(out42);
		fclose(out43);
		fclose(out44);
		fclose(out45);
		fclose(out46);
		/* On met en ordre toutes les parties */
		copyAlgo(out1_tmp,s41,0);
		copyAlgo(out1_tmp,s21,0);
		copyAlgo(out1_tmp,s42,tab2);
		copyAlgo(out1_tmp,s22,0);
		copyAlgo(out1_tmp,s43,0);
		copyAlgo(out1_tmp,s11,0);
		copyAlgo(out1_tmp,s44,tab1);
		if (B_tmp!=NULL)
		{
			for (i = 0; i < tab1; i++) fprintf(out1_tmp,"	");
			fprintf(out1_tmp,"/* --- %d */\n",b);
		}		
		copyAlgo(out2_tmp,s45,tab1);
		copyAlgo(out2_tmp,s12,0);
		copyAlgo(out2_tmp,s46,0);
		remove(s11);
		remove(s12);
		remove(s21);
		remove(s22);
		remove(s41);
		remove(s42);
		remove(s43);
		remove(s44);
		remove(s45);
		remove(s46);
		free(s11);
		free(s12);
		free(s21);
		free(s22);
		free(s41);
		free(s42);
		free(s43);
		free(s44);
		free(s45);
		free(s46);
		fclose(out1_tmp);
		fclose(out2_tmp);
		fclose(out31);
		fclose(out32);
		free(X1);
		free(X2);
		free(X3);
		free(X4);
		for (i = 0; i < 2*n; i++) freeEq(Eq[i]);
		free(Eq);
		freeSolver(B3);
		*sur=0;
		b++;
		if (B_tmp!=NULL) // On vient seulement d'écrire l'algo pour B4 alors qu'on doit le faire pour B1 u B2
		{
			/* On sauvegarde l'algo de B4 */
			ALGORITHME *A_tmp=malloc((Nb+1)*sizeof(ALGORITHME));
			for (i = 0; i < Nb; i++)
			{
				A_tmp[i].B=A[i].B;
				A_tmp[i].debut=A[i].debut;
				A_tmp[i].fin=A[i].fin;
				A_tmp[i].sur=A[i].sur;
				A_tmp[i].tab=A[i].tab;
			}
			A_tmp[Nb].B=B4;
			A_tmp[Nb].debut=s1_tmp;
			A_tmp[Nb].fin=s2_tmp;
			A_tmp[Nb].sur=0;
			A_tmp[Nb].tab=tab4+tab1;
			tab=EcrireAlgo_tmp(s1,s2,outH,B_tmp,E,alea,sur,mem,mem_max,A_tmp,Nb+1); // On écrit le bon algo
			FILE *out1 = fopen(s1,"a+");
			for (i = 0; i < tab; i++) fprintf(out1,"	");
			fprintf(out1,"/* --- %d */\n",b-1);
			fclose(out1);
			free(A_tmp);
			freeSolver(B_tmp);
			freeSolver(B4);
		}
		else
		{
			sprintf(s1,"tmp/out1_%d.txt",al);
			FILE *out1 = fopen(s1,"w+");
			copyAlgo(out1,s1_tmp,0);
			fclose(out1);
			sprintf(s2,"tmp/out2_%d.txt",al);
			FILE *out2 = fopen(s2,"w+");
			copyAlgo(out2,s2_tmp,0);
			fclose(out2);
			tab=tab4+tab1;
		}
		remove(s31);
		remove(s32);
		free(s31);
		free(s32);
		remove(s1_tmp);
		remove(s2_tmp);
		free(s1_tmp);
		free(s2_tmp);
	}
	printf("\r	- write subroutine (%d)",a);
	fflush(stdout);
	return tab;
}

void EcrireAlgo(char *s, Solver b, int *Known, int NKnown, SysEqLin E, string_list_t *variable_names)
{
	Solver *B1,B;
	int N1,i,sur,n,tab;
	char *s1,*s2;
	SysEqLin F;
	unsigned long int mem,mem_max;

	printf("Sart C code generation (may take a long time ...) : \n");
	B1=MakeSolver1(E,&N1);
	while (EnleveFaussesEq(B1,N1,E)==1)
	{
		for (i = 0; i < N1; i++) freeSolver(B1[i]);
		free(B1);
		B1=MakeSolver1(E,&N1);
	}
	for (i = 0; i < N1; i++) freeSolver(B1[i]);
	free(B1);
	F=EsachantX(Known,NKnown,E);
	printf("- Refine solver\n");
	fflush(stdout);
	Solver b1=OptimizeSolver(b,F);
	B=Refine(b1,F);

	FILE *out1,*out2,*outH,*out;

	n=strlen(s)+1+strlen("Programs/.c");
	s1=malloc(n*sizeof(char));
	s2=malloc(n*sizeof(char));
	sprintf(s1,"Programs/%s.c",s);
	sprintf(s2,"Programs/%s.h",s);

	out=fopen(s1,"w+");
	out1=fopen("Programs/debut.c","w+");
	out2=fopen("Programs/fin.c","w+");
	outH=fopen(s2,"w+");
	
	fprintf(outH,"#include <stdint.h>\n\n");

	printf("- Write attacks\n");
	fflush(stdout);
	{
		char *s1_tmp = malloc(30*sizeof(char));
		char *s2_tmp = malloc(30*sizeof(char));
		tab=EcrireAlgo_tmp(s1_tmp,s2_tmp,outH,B,F,0,&sur,&mem,&mem_max,NULL,0);
		copyAlgo(out1,s1_tmp,0);
		copyAlgo(out2,s2_tmp,0);
		remove(s1_tmp);
		remove(s2_tmp);
		free(s1_tmp);
		free(s2_tmp);
	}
	printf("\n");

	fprintf(out,"#include <stdio.h>\n");
	fprintf(out,"#include <stdlib.h>\n");
	fprintf(out,"#include <unistd.h>\n");
	fprintf(out,"#include \"%s.h\"\n\n",s);
	
	fprintf(out,"const unsigned char sbox[256] = {\n");   
    fprintf(out,"	//0     1    2      3     4    5     6     7      8    9     A      B    C     D     E     F   \n"); 
    fprintf(out,"	0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76, //0  \n");  
    fprintf(out,"	0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0, //1   \n"); 
	fprintf(out,"	0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15, //2   \n"); 
    fprintf(out,"	0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75, //3   \n"); 
    fprintf(out,"	0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84, //4   \n"); 
    fprintf(out,"	0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf, //5   \n"); 
    fprintf(out,"	0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8, //6   \n"); 
    fprintf(out,"	0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2, //7   \n"); 
    fprintf(out,"	0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73, //8   \n"); 
    fprintf(out,"	0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb, //9   \n"); 
    fprintf(out,"	0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79, //A   \n"); 
    fprintf(out,"	0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08, //B   \n"); 
    fprintf(out,"	0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a, //C   \n"); 
    fprintf(out,"	0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e, //D   \n"); 
    fprintf(out,"	0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf, //E   \n"); 
    fprintf(out,"	0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16 \n"); 
	fprintf(out,"	};\n\n"); 

	fprintf(out,"const unsigned char rsbox[256] = { \n");  
    fprintf(out,"	0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,  \n"); 
    fprintf(out,"	0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb,  \n"); 
    fprintf(out,"	0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,  \n"); 
    fprintf(out,"	0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,  \n"); 
    fprintf(out,"	0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92,  \n"); 
    fprintf(out,"	0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,  \n"); 
    fprintf(out,"	0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,  \n"); 
    fprintf(out,"	0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b,  \n"); 
    fprintf(out,"	0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,  \n"); 
    fprintf(out,"	0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e,  \n"); 
    fprintf(out,"	0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b,  \n"); 
    fprintf(out,"	0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,  \n"); 
    fprintf(out,"	0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,  \n"); 
    fprintf(out,"	0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef,  \n"); 
    fprintf(out,"	0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,  \n"); 
    fprintf(out,"	0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d \n"); 
	fprintf(out,"	};\n\n"); 
    
	fprintf(out,"/*const unsigned char Rcon[255] = { \n");   
    fprintf(out,"	0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36, 0x6c, 0xd8, 0xab, 0x4d, 0x9a,    \n"); 
    fprintf(out,"	0x2f, 0x5e, 0xbc, 0x63, 0xc6, 0x97, 0x35, 0x6a, 0xd4, 0xb3, 0x7d, 0xfa, 0xef, 0xc5, 0x91, 0x39,    \n"); 
    fprintf(out,"	0x72, 0xe4, 0xd3, 0xbd, 0x61, 0xc2, 0x9f, 0x25, 0x4a, 0x94, 0x33, 0x66, 0xcc, 0x83, 0x1d, 0x3a,    \n"); 
    fprintf(out,"	0x74, 0xe8, 0xcb, 0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36, 0x6c, 0xd8,    \n"); 
    fprintf(out,"	0xab, 0x4d, 0x9a, 0x2f, 0x5e, 0xbc, 0x63, 0xc6, 0x97, 0x35, 0x6a, 0xd4, 0xb3, 0x7d, 0xfa, 0xef,    \n"); 
    fprintf(out,"	0xc5, 0x91, 0x39, 0x72, 0xe4, 0xd3, 0xbd, 0x61, 0xc2, 0x9f, 0x25, 0x4a, 0x94, 0x33, 0x66, 0xcc,    \n"); 
    fprintf(out,"	0x83, 0x1d, 0x3a, 0x74, 0xe8, 0xcb, 0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b,    \n"); 
    fprintf(out,"	0x36, 0x6c, 0xd8, 0xab, 0x4d, 0x9a, 0x2f, 0x5e, 0xbc, 0x63, 0xc6, 0x97, 0x35, 0x6a, 0xd4, 0xb3,    \n"); 
    fprintf(out,"	0x7d, 0xfa, 0xef, 0xc5, 0x91, 0x39, 0x72, 0xe4, 0xd3, 0xbd, 0x61, 0xc2, 0x9f, 0x25, 0x4a, 0x94,    \n"); 
    fprintf(out,"	0x33, 0x66, 0xcc, 0x83, 0x1d, 0x3a, 0x74, 0xe8, 0xcb, 0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20,    \n"); 
    fprintf(out,"	0x40, 0x80, 0x1b, 0x36, 0x6c, 0xd8, 0xab, 0x4d, 0x9a, 0x2f, 0x5e, 0xbc, 0x63, 0xc6, 0x97, 0x35,    \n"); 
    fprintf(out,"	0x6a, 0xd4, 0xb3, 0x7d, 0xfa, 0xef, 0xc5, 0x91, 0x39, 0x72, 0xe4, 0xd3, 0xbd, 0x61, 0xc2, 0x9f,    \n"); 
    fprintf(out,"	0x25, 0x4a, 0x94, 0x33, 0x66, 0xcc, 0x83, 0x1d, 0x3a, 0x74, 0xe8, 0xcb, 0x8d, 0x01, 0x02, 0x04,    \n"); 
    fprintf(out,"	0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36, 0x6c, 0xd8, 0xab, 0x4d, 0x9a, 0x2f, 0x5e, 0xbc, 0x63,    \n"); 
    fprintf(out,"	0xc6, 0x97, 0x35, 0x6a, 0xd4, 0xb3, 0x7d, 0xfa, 0xef, 0xc5, 0x91, 0x39, 0x72, 0xe4, 0xd3, 0xbd,    \n"); 
    fprintf(out,"	0x61, 0xc2, 0x9f, 0x25, 0x4a, 0x94, 0x33, 0x66, 0xcc, 0x83, 0x1d, 0x3a, 0x74, 0xe8, 0xcb\n"); 
	fprintf(out,"	};*/\n\n");

	fprintf(out,"#define S(x) sbox[x]\n");
	fprintf(out,"#define R(x) rsbox[x]\n\n");

	fprintf(out,"unsigned char TableMul2_8[256][256];\n\n");
	
	fprintf(out,"void MakeTableMul2_8(void)\n"); 
	fprintf(out,"{\n");
	fprintf(out,"	int a,b;\n");
	fprintf(out,"	unsigned char aa,bb,r,t;\n");
	fprintf(out,"	for (a = 0; a <256 ; a++)\n");
	fprintf(out,"	{\n");
	fprintf(out,"		for (b = a; b < 256 ; b++)\n");
	fprintf(out,"		{\n");
	fprintf(out,"			aa=a;\n");
	fprintf(out,"			bb=b;\n");
	fprintf(out,"			r=0;\n");
	fprintf(out,"			while (aa != 0)\n");
	fprintf(out,"			{\n");
	fprintf(out,"				if ((aa & 1) != 0) r=r^bb;\n");
	fprintf(out,"				t=bb & 0x80;\n");
	fprintf(out,"				bb=bb<<1;\n");
	fprintf(out,"				if (t != 0) bb=bb^0x1b;\n");
	fprintf(out,"				aa=aa>>1;\n");
	fprintf(out,"			}\n");
	fprintf(out,"			TableMul2_8[a][b]=TableMul2_8[b][a]=r;\n");	
	fprintf(out,"		}\n");
	fprintf(out,"	}\n");
	fprintf(out,"}\n\n");

	fprintf(out,"#define Multiply(a,b) TableMul2_8[a][b]\n\n");
	/* blabla
	 *
	 *
	 *
	 *
	 * */
	fprintf(out,"/*unsigned char Inverse(unsigned char a)\n");
	fprintf(out,"{\n");
	fprintf(out,"	static unsigned char TableInverse[256]={0x00,0x01,0x8d,0xf6,0xcb,0x52,0x7b,0xd1,0xe8,0x4f,0x29,0xc0,0xb0,0xe1,0xe5,0xc7,0x74,0xb4,0xaa,0x4b,0x99,0x2b,0x60,0x5f,0x58,0x3f,0xfd,0xcc,0xff,0x40,0xee,0xb2,0x3a,0x6e,0x5a,0xf1,0x55,0x4d,0xa8,0xc9,0xc1,0x0a,0x98,0x15,0x30,0x44,0xa2,0xc2,0x2c,0x45,0x92,0x6c,0xf3,0x39,0x66,0x42,0xf2,0x35,0x20,0x6f,0x77,0xbb,0x59,0x19,0x1d,0xfe,0x37,0x67,0x2d,0x31,0xf5,0x69,0xa7,0x64,0xab,0x13,0x54,0x25,0xe9,0x09,0xed,0x5c,0x05,0xca,0x4c,0x24,0x87,0xbf,0x18,0x3e,0x22,0xf0,0x51,0xec,0x61,0x17,0x16,0x5e,0xaf,0xd3,0x49,0xa6,0x36,0x43,0xf4,0x47,0x91,0xdf,0x33,0x93,0x21,0x3b,0x79,0xb7,0x97,0x85,0x10,0xb5,0xba,0x3c,0xb6,0x70,0xd0,0x06,0xa1,0xfa,0x81,0x82,0x83,0x7e,0x7f,0x80,0x96,0x73,0xbe,0x56,0x9b,0x9e,0x95,0xd9,0xf7,0x02,0xb9,0xa4,0xde,0x6a,0x32,0x6d,0xd8,0x8a,0x84,0x72,0x2a,0x14,0x9f,0x88,0xf9,0xdc,0x89,0x9a,0xfb,0x7c,0x2e,0xc3,0x8f,0xb8,0x65,0x48,0x26,0xc8,0x12,0x4a,0xce,0xe7,0xd2,0x62,0x0c,0xe0,0x1f,0xef,0x11,0x75,0x78,0x71,0xa5,0x8e,0x76,0x3d,0xbd,0xbc,0x86,0x57,0x0b,0x28,0x2f,0xa3,0xda,0xd4,0xe4,0x0f,0xa9,0x27,0x53,0x04,0x1b,0xfc,0xac,0xe6,0x7a,0x07,0xae,0x63,0xc5,0xdb,0xe2,0xea,0x94,0x8b,0xc4,0xd5,0x9d,0xf8,0x90,0x6b,0xb1,0x0d,0xd6,0xeb,0xc6,0x0e,0xcf,0xad,0x08,0x4e,0xd7,0xe3,0x5d,0x50,0x1e,0xb3,0x5b,0x23,0x38,0x34,0x68,0x46,0x03,0x8c,0xdd,0x9c,0x7d,0xa0,0xcd,0x1a,0x41,0x1c};\n");
	fprintf(out,"	return TableInverse[a];\n");
	fprintf(out,"}\n*/");

	fprintf(out,"/* Index variables \n");
	for (i = 0; i < VARIABLES; i++) fprintf(out," * x[%d] = %s\n",i,find_var_name(variable_names,i));
	fprintf(out," **/\n");

	fprintf(out,"void Attack(const unsigned char Known[%d])\n",NKnown);
	fprintf(out,"{\n");
	//fprintf(out,"	unsigned char x[%d];\n",VARIABLES);
	//fprintf(out,"	int i;\n");
	//fprintf(out,"	for (i = 0; i < %d; i++) x[i]=0;\n\n",VARIABLES);
	for (i = 0; i < NKnown; i++) fprintf(out,"	const unsigned char x%d=Known[%d]; /* %s */\n",Known[i],i,find_var_name(variable_names,Known[i]));
	fclose(out1);
	copyAlgo(out,"Programs/debut.c",1);
	out1=fopen("Programs/debut.c","w+");
	fprintf(out1,"printf(\"# Solution found :\\n\");\n");
	for (i = 0; i < VARIABLES; i++) {
	  if(strncmp(find_var_name(variable_names,i), "1", 1)) 
	    fprintf(out1,"printf(\"%s = %%02x\\n\",x%d);\n",find_var_name(variable_names,i),i);
	}
	fprintf(out1,"return ;/*getchar();*/\n");
	fclose(out1);
	copyAlgo(out,"Programs/debut.c",1+tab);
	fclose(out2);
	copyAlgo(out,"Programs/fin.c",1);
	remove("Programs/debut.c");
	remove("Programs/fin.c");
	fprintf(out,"}\n\n");	
	freeSysEq(F);
	printf(" Make a generator for known variables ? (y/n) ");
	char c;
	scanf("%c",&c);
	getchar();
	Solver B_tmp = NULL;
	if (c=='y') B_tmp=SearchAlgo(E,1,1,Known,1,2,variable_names,NULL);
	if (c=='n' || B_tmp==NULL)
	{
		F=ExtraireEX2(Known,NKnown,E);
		B_tmp=SearchAlgo(F,1,1,Known,1,2,variable_names,NULL);
		freeSysEq(F);
	}
	if (B_tmp!=NULL)
	{
		out1=fopen("Programs/debut.c","w+");
		out2=fopen("Programs/fin.c","w+");
		printf("- Write generator\n");
		F=EsachantX(Known,1,E);
		//tab=EcrireAlgo_tmp(out1,out2,outH,B_tmp,F,1,&sur,&mem,&mem_max,NULL,0);
		{
			char *s1_tmp = malloc(30*sizeof(char));
			char *s2_tmp = malloc(30*sizeof(char));
			tab=EcrireAlgo_tmp(s1_tmp,s2_tmp,outH,B_tmp,F,1,&sur,&mem,&mem_max,NULL,0);
			copyAlgo(out1,s1_tmp,0);
			copyAlgo(out2,s2_tmp,0);
			remove(s1_tmp);
			remove(s2_tmp);
			free(s1_tmp);
			free(s2_tmp);
		}
		freeSysEq(F);
		printf("\n");
			
		fprintf(out,"int Generator(unsigned char Known[%d]/*, unsigned char y[%d]*/)\n",NKnown,VARIABLES);
		fprintf(out,"{\n");
		//fprintf(out,"	unsigned char *x = malloc(%d*sizeof(unsigned char));\n",VARIABLES);
		//fprintf(out,"	int i;\n");
		//fprintf(out,"	for (i = 1; i < %d; i++) x[i]=0;\n\n",VARIABLES);
		fprintf(out,"	const unsigned char x0=1;\n");
		fclose(out1);
		copyAlgo(out,"Programs/debut.c",1);
		out1=fopen("Programs/debut.c","w+");
		for (i = 0; i < NKnown; i++) fprintf(out1,"Known[%d]=x%d; /* %s */\n",i,Known[i], find_var_name(variable_names,Known[i]));
		//fprintf(out1,"free(x);\n");
		fprintf(out1,"return 1;\n");
		fclose(out1);
		copyAlgo(out,"Programs/debut.c",1+tab);
		fclose(out2);
		copyAlgo(out,"Programs/fin.c",1);
		//fprintf(out,"	free(x);\n");
		fprintf(out,"	return 0;\n");
		fprintf(out,"}\n\n");

		fprintf(out,"int main(int argc, char** argv)\n");
		fprintf(out,"{\n");
		fprintf(out,"	unsigned char Known[%d];\n",NKnown);
		fprintf(out,"   srand(getpid()); /* Init PRNG */\n");
		fprintf(out,"	MakeTableMul2_8();\n");
		fprintf(out,"	/* assign values */\n");
		//for (i = 0; i < NKnown; i++) fprintf(out,"	Known[%d] = %s;\n",i,find_var_name(variable_names, Known[i]));
		//fprintf(out,"	y[0]=1;\n");
		fprintf(out,"	Generator(Known);\n");
		fprintf(out,"	/* Attack */\n");
		fprintf(out,"	Attack(Known);\n");
		fprintf(out,"	return 0;\n");
		fprintf(out,"}\n");
		freeSolver(B_tmp);
		remove("Programs/debut.c");
		remove("Programs/fin.c");
	}
	else
	{
		fprintf(out,"int main(int argc, char** argv)\n");
		fprintf(out,"{\n");
		fprintf(out,"	unsigned char Known[%d];\n",NKnown);
		fprintf(out,"   srand(getpid()); /* Init PRNG */\n");
		fprintf(out,"	MakeTableMul2_8();\n");
		fprintf(out,"	/* assign values */\n");
		fprintf(out,"	Known[0] = 1;\n");
		for (i = 1; i < NKnown; i++) fprintf(out,"	Known[%d] = (1+rand()%%255) /* %s */;\n",i,find_var_name(variable_names, Known[i]));
		fprintf(out,"	/* Attack */\n");
		fprintf(out,"	Attack(Known);\n");
		fprintf(out,"	return 0;\n");
		fprintf(out,"}\n");
	}
	freeSolver(B);
	fclose(outH);
	fclose(out);
	printf("\nDone\n");
	
}
	


	
