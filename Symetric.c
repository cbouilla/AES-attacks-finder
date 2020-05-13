//      Symetric.c
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
#include "Symetric.h"
#include "Bazar.h"
#include "Search.h"

extern const int VARIABLES;


/* **************************************************************************** */
/* 																				*/
/*                   Operations "symetriques" sur les solvers                    */
/* 																				*/
/* **************************************************************************** */


Symetric_structure InitialiseSymetricStrucure(int **var_by_message,	int n_var_by_message, int n_message)
{
	Symetric_structure S = malloc(sizeof(SYMETRIC_STUCTURE));
	S->var_by_message = var_by_message;
	S->n_var_by_message = n_var_by_message;
	S->n_message = n_message;
	S->message_by_var=malloc(VARIABLES*sizeof(int));
	S->position_by_var=malloc(VARIABLES*sizeof(int));
	
	int i;
	for (i = n_message-1; i>=0 ; i--)
	{
		int j;
		for (j = 0; j < n_var_by_message; j++)
		{
			S->message_by_var[var_by_message[i][j]]=i;
			S->position_by_var[var_by_message[i][j]]=j;
		}
	}
	return S;
}

void UpdateSymetricStructure(Symetric_structure S, SysEqLin E)
{
	if (S!=NULL && S->n_message>1)
	{
		const int NombreDeVariables = E->a/E->gen;
		int i;
		for (i = 0; i < NombreDeVariables; i++)
		{
			printf("\rUpdateSymetricStructure : %d %%",(i*100)/NombreDeVariables);
			fflush(stdout);
			const int v = E->varligne[i];
			const int pos = S->position_by_var[v];
			const int mes = S->message_by_var[v];
			if (mes)
			{
				const int w = S->var_by_message[0][pos];
				if (E->lignevar[w]>=NombreDeVariables)
				{
					int j;
					for (j = 0; j < S->n_message; j++) S->var_by_message[j][pos] = v;
					S->message_by_var[v]=0;
				}
			}
			else
			{
				const int w = S->var_by_message[1][pos];
				if (E->lignevar[w]>=NombreDeVariables)
				{
					int j;
					for (j = 1; j < S->n_message; j++) S->var_by_message[j][pos] = v;
				}
			}
		}
		/*for (i = 0; i < S->n_message; i++)
		{
			int j;
			for (j = 0; j < S->n_var_by_message; j++)
			{
				printf("%03d ",S->var_by_message[i][j]);
			}
			printf("\n");
		}
		getchar();
		printf("\n");*/
		
	}
}

Solver SymetriseSolver(Solver B, SysEqLin E, Symetric_structure S)
{
	if (B!=NULL)
	{
		int decalage = 1;
		Solver out = copySolver(B);
		do
		{
			Solver tmp = SolverSymetrique(out,decalage,S);
			Solver tmp2 = UnionSolverBorne(out,tmp,E,B->sortie);
			freeSolver(tmp);
			if (tmp2!=NULL && tmp2->Nvar!=out->Nvar)
			{
				freeSolver(out);
				out=tmp2;
				decalage=2*decalage;
			}
			else
			{
				freeSolver(tmp2);
				return out;
			}
		} while (1);
	}
	return NULL;
}
	
Solver SolverSymetrique(Solver B, const int decalage, Symetric_structure S)
{
	static unsigned char changement = 0;
	
	if (B!=NULL)
	{
		if (B->type!=0)
		{
			Solver B1 = SolverSymetrique(B->fils1,decalage,S);
			if (changement)
			{
				Solver out=NewSolver();
				out->fils1=B1;
				out->fils2=SolverSymetrique(B->fils2,decalage,S);
				out->var=UnionListesTriees(out->fils1->var,out->fils1->Nvar,out->fils2->var,out->fils2->Nvar,&out->Nvar);
				out->type=B->type;
				out->temps=B->temps;
				out->memoire=B->memoire;
				out->sortie=B->sortie;
				changement=1;
				return out;	
			}
			else
			{
				Solver B2 = SolverSymetrique(B->fils2,decalage,S);
				if (changement)
				{
					Solver out=NewSolver();
					out->fils1=B1;
					out->fils2=B2;
					out->var=UnionListesTriees(out->fils1->var,out->fils1->Nvar,out->fils2->var,out->fils2->Nvar,&out->Nvar);
					out->type=B->type;
					out->temps=B->temps;
					out->memoire=B->memoire;
					out->sortie=B->sortie;
					return out;	
				}
				else
				{
					freeSolver(B1);
					freeSolver(B2);
					return copySolver(B);
				}
			}
		}
		else
		{
			const int var = S->var_by_message[(decalage+S->message_by_var[B->var[0]])%S->n_message][S->position_by_var[B->var[0]]];
			if (B->var[0]!=var)
			{
				changement=1;
				Solver out=NewSolver();
				out->fils1=NULL;
				out->fils2=NULL;
				out->Nvar=1;
				out->var=malloc(sizeof(int));
				out->var[0]=var;
				out->type=B->type;
				out->temps=B->temps;
				out->memoire=B->memoire;
				out->sortie=B->sortie;
				return out;	
			}
			else
			{
				changement=0;
				return copySolver(B);
			}
		}
		
	}
	else
	{
		changement=0;
		return NULL;
	}
}

Solver SearchUneSolver3_SymetricVersion(const int T, Solver *B1, const int N1, SysEqLin E, Symetric_structure S) 
{
	int i=rand()%N1;
	int N=N1-1;
	Solver B=B1[i]; // On prend une solver au hasard
	B1[i]=B1[N];
	B1[N]=B;
	B=copySolver(B);
	while (B->sortie<T && N>0) // Tant que l'on peux guesser des solvers
	{
		while (B->sortie<T && N>0) // Tant que l'on peux guesser des solvers
		{
			i=rand()%N;
			Solver tmp=B1[i];
			N--;
			B1[i]=B1[N];
			B1[N]=tmp;
			tmp=UnionSolver(B,B1[N],E);
			freeSolver(B);
			B=tmp;
		}
		Solver tmp=PropagateSolver2(B,E,B1,N); // On l'augmente gratuitement
		freeSolver(B);
		B=SymetriseSolver(tmp,E,S);
		if (B==NULL) B=tmp;
		else freeSolver(tmp);
	}
	return B;
}

// La plus rapide
Solver SearchUneSolverN3_SymetricVersion(int T, Solver *B1, int N1, SysEqLin E, Symetric_structure S)
{

	Solver * __restrict Solver_du_premier_message = malloc(N1*sizeof(Solver));
	int Nombre_de_solver_a_guesser = 0;
	int i;
	for (i = 0; i < N1; i++)
	{
		int j = 0;
		while (j<B1[i]->Nvar && S->message_by_var[B1[i]->var[j]]!=0) j++;
		if (j!=B1[i]->Nvar)	Solver_du_premier_message[Nombre_de_solver_a_guesser++]=B1[i];
	}

	int Alloue=1000; // espace alloué pour stocker les solvers
	Solver * __restrict Candidat=malloc(Alloue*sizeof(Solver));
	int N=0;
	int v_max=0;
	int proba = 0;
	int pos=0;
	Solver * __restrict guess_probable = malloc(N1*sizeof(Solver));
	printf("Randomized Search : (%d/%d) \n",Nombre_de_solver_a_guesser,N1);
	unsigned int cpt=0;
	int cpt2=0;
	Solver tmp;
	const int coeff_proba=200*E->gen/E->a;
	while (1)
	{
		cpt++;
		// On génère une solver
		if (cpt2 < proba) tmp=SearchUneSolver3_SymetricVersion(T,guess_probable,pos,E,S);
		else tmp=SearchUneSolver3_SymetricVersion(T,Solver_du_premier_message,Nombre_de_solver_a_guesser,E,S);
		i=0;
		// On la teste vis à vis des autres
		while (i < N)
		{
			Solver tmp2=UnionSolverBorne(Candidat[i],tmp,E,T);
			if (tmp2!=NULL)
			{
				if (tmp2->Nvar == Candidat[i]->Nvar)
				{
					freeSolver(tmp2);
					break;
				}
				if (tmp2->Nvar == tmp->Nvar)
				{
					N--;
					freeSolver(Candidat[i]);
					Candidat[i]=Candidat[N];
					freeSolver(tmp2);
				}
				else
				{
					N--;
					freeSolver(Candidat[i]);
					Candidat[i]=Candidat[N];
					freeSolver(tmp);
					tmp=PropagateSolver2(tmp2,E,B1,N1);
					freeSolver(tmp2);
					if (tmp->sortie>0) i=0;
					else i=N;
				}
			}
			else
			{
				//freeSolver(tmp2);
				i++;
			}
		}
		if (i==N) // c'est une nouvelle solver
		{
			if (tmp->sortie<=0) // On a probablement gagné
			{
				for (i = 0; i < N; i++) freeSolver(Candidat[i]);
				free(Candidat);
				for (i = 0; i < pos; i++) freeSolver(guess_probable[i]);
				free(guess_probable);
				printf("\n");
				return tmp;
			}
			Candidat[N++]=tmp;
			if (tmp->Nvar>v_max) // Elle a beaucoup de variables -> elle parait interessante
			{
				/* On cherchera une partie des solvers générées dans son complémentaire */
				v_max=tmp->Nvar;
				for (i = 0; i < pos; i++) freeSolver(guess_probable[i]);
				pos=0;
				for (i = 0; i < Nombre_de_solver_a_guesser; i++) if (!ContenuListesTriees(Solver_du_premier_message[i]->var,Solver_du_premier_message[i]->Nvar,tmp->var,tmp->Nvar)) guess_probable[pos++]=copySolver(Solver_du_premier_message[i]);
				proba=v_max*coeff_proba;
			}
			//v_max=max(v_max,tmp->Nvar);
			if (cpt2 >= 199)
			{
				printf("\r - Candidats générés : %u (%d stockés), Nombre var max : %d / %d ",cpt,N,v_max,E->a/E->gen);
				fflush(stdout);
				cpt2=0;
			}
			else cpt2++;
			if (N==Alloue) // On a atteint la limite d'espace alloué
			{
				i=0;
				while (i<N) // On supprime celles qui semblent peu intéressantes
				{
					if (Candidat[i]->Nvar<=(v_max+T)/2)
					{
						freeSolver(Candidat[i]);
						N--;
						Candidat[i]=Candidat[N];
					}
					else i++;
				}
				if (Alloue-N<500) // On realloue
				{
					Alloue+=1000;
					Candidat=realloc(Candidat,Alloue*sizeof(Solver));
					if (Candidat==NULL)
					{
						printf("Erreur allocation");
						getchar();
					}
				}
			}		
		}
		else // elle est moins interessante que celles que l'on a déjà
		{
			freeSolver(tmp);
			cpt2++;
		}
		if (N==1 && Candidat[0]->Nvar==E->a/E->gen) // On a gagné
		{
			tmp=Candidat[0];
			free(Candidat);
			printf("\n");
			return tmp;
		}
	}
	for (i = 0; i < N; i++) freeSolver(Candidat[i]);
	free(Candidat);
	printf("\n");
	return NULL;
}

