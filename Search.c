

#include <stdio.h>
#include <stdlib.h>
#include "Search.h"
#include "Bazar.h"
#include "SysEqLin.h"
#include "Algebre.h"

/* **************************************************************************** */
/* 																				*/
/*                   Search of low memory solvers                               */
/* 																				*/
/* **************************************************************************** */

Solver SearchAlgoLowMem(SysEqLin E, const int T)
{
	const int nb_variables = E->a/E->gen;
	int Alloue=T*nb_variables;
	Solver * __restrict R = malloc(Alloue*sizeof(Solver));
	SysEqLin * __restrict Eq = malloc(Alloue*sizeof(SysEqLin));
	int * __restrict nb_guess = malloc(Alloue*sizeof(int));
	int * __restrict nb_total = malloc(Alloue*sizeof(int));
	Solver ** __restrict variables = malloc(Alloue*sizeof(Solver *));
	int i;
	for (i = 0; i < Alloue; i++) variables[i]=malloc(nb_variables*sizeof(Solver));
	Solver * tab[3];
	for (i = 0; i < 3; i++) tab[i]=malloc(nb_variables*sizeof(Solver));
	
	for (i = 0; i < nb_variables; i++) variables[0][i]=BaseSolver(E->varligne[i],E);
	R[0]=NULL;
	Eq[0]=EsachantX(NULL,0,E);
	nb_guess[0]=nb_variables;
	nb_total[0]=nb_variables;
	int nb_state = 1;
	
	Solver result = NULL;
	
	unsigned long int nb_round = 0;
	while (nb_state)
	{		
		nb_state--;
		
		SysEqLin F = Eq[nb_state];
		Solver r = R[nb_state];
		int n[3];
		n[0]=nb_guess[nb_state];
		n[1]=nb_total[nb_state]-n[0];
		n[2]=0;
			
			
		for (i = 0; i < n[0]; i++) tab[0][i]=variables[nb_state][i];
		for (i = 0; i < n[1]; i++) tab[1][i]=variables[nb_state][i+n[0]];
		
		int k;
		for (k = 0; k < 2; k++)
		{
			i=0;
			while (i<n[k])
			{
				int j=i-1;
				while (j>=0)
				{
					Solver tmp = UnionSolverBorne(tab[k][i],tab[k][j],F,min(tab[k][i]->sortie,tab[k][j]->sortie));
					if (tmp)
					{
						freeSolver(tab[k][i]);
						freeSolver(tab[k][j]);
						tab[k][i]=tab[k][--n[k]];
						i--;
						tab[k][j]=tab[k][i];
						tab[k][i]=tmp;							
						i--;
						break;
					}
					j--;
				}
				i++;
			}	
		}
		
		int i=0;
		while (i<n[0])
		{
			int j=0;
			while (j<n[1])
			{
				Solver tmp = UnionSolverBorne(tab[0][i],tab[1][j],F,min(tab[0][i]->sortie,tab[1][j]->sortie));
				if (tmp)
				{
					freeSolver(tab[0][i]);
					tab[0][i]=tab[0][--n[0]];
					freeSolver(tab[1][j]);
					tab[1][j]=tab[1][--n[1]];
					tab[2][n[2]++]=tmp;
					i--;
					break;
				}
				j++;
			}
			i++;
		}
		
		i=0;
		while (i<n[2])
		{
			int j;
			debut:
			j=i-1;
			while (j>=0)
			{
				Solver tmp = UnionSolverBorne(tab[2][i],tab[2][j],F,min(tab[2][i]->sortie,tab[2][j]->sortie));
				if (tmp)
				{
					freeSolver(tab[2][i]);
					freeSolver(tab[2][j]);
					tab[2][j]=tab[2][i-1];
					tab[2][i-1]=tmp;
					tab[2][i]=tab[2][--n[2]];
					i--;
					goto debut;
				}
				j--;
			}
			for (k = 0; k < 2; k++)
			{
				j=0;
				while (j < n[k])
				{
					Solver tmp = UnionSolverBorne(tab[2][i],tab[k][j],F,min(tab[2][i]->sortie,tab[k][j]->sortie));
					if (tmp)
					{
						freeSolver(tab[2][i]);
						freeSolver(tab[k][j]);
						tab[k][j]=tab[k][--n[k]];
						tab[2][i]=tmp;
						goto debut;
					}
					j++;
				}
			}
			i++;
		}		
		
		if (n[0]+n[1]+n[2]==1)
		{
			i=0;
			while (!n[i]) i++;
			if (r!=NULL)
			{
				result=NewSolver();
				result->sortie=max(r->sortie+tab[i][0]->sortie,0);
				result->var=UnionListesTriees(r->var,r->Nvar,tab[i][0]->var,tab[i][0]->Nvar,&result->Nvar);
				result->temps=max(r->temps,max(r->sortie,0)+tab[i][0]->temps);
				result->memoire=max(r->memoire,tab[i][0]->memoire);
				result->fils1=tab[i][0];
				result->fils2=r;
				result->type=2;
			}
			else result=tab[i][0];
						
			for (i = 0; i < nb_state; i++)
			{
				freeSolver(R[i]);
				int j;
				for (j = 0; j < nb_total[i]; j++) freeSolver(variables[i][j]);
				freeSysEq(Eq[i]);
			}
			nb_state=0;
		}
		else
		{
			int one_solution=0;
			int one_solution_i=0;
			int one_solution_j=0;
		
			for (i = 0; i < 3; i++)
			{
				int j;
				for (j = 0; j < n[i]; j++)
				{
					if (tab[i][j]->sortie<=0)
					{
						one_solution=1;
						one_solution_i=i;
						one_solution_j=j;
						goto suite;
					}
				}
			}
			
			if ((r==NULL && T>1) || r->sortie<T-1)
			{
				const int m = (r==NULL ? T : T-r->sortie);
				int g = n[0];
				const int new_nb_total = n[0]+n[1]+n[2]-1;
				while (g>=m)
				{
					if (nb_state==Alloue)
					{
						Alloue=2*Alloue;
						R=realloc(R,Alloue*sizeof(Solver));
						nb_guess=realloc(nb_guess,Alloue*sizeof(int));
						nb_total=realloc(nb_total,Alloue*sizeof(int));
						Eq=realloc(Eq,Alloue*sizeof(SysEqLin));
						variables=realloc(variables,Alloue*sizeof(Solver *));
						for (i = Alloue/2; i < Alloue; i++) variables[i]=malloc(nb_variables*sizeof(Solver));
					}
					g--;
					if (r!=NULL)
					{
						R[nb_state]=NewSolver();
						R[nb_state]->sortie=max(r->sortie+tab[0][g]->sortie,0);
						R[nb_state]->var=UnionListesTriees(r->var,r->Nvar,tab[0][g]->var,tab[0][g]->Nvar,&R[nb_state]->Nvar);
						R[nb_state]->temps=max(r->temps,max(r->sortie,0)+tab[0][g]->temps);
						R[nb_state]->memoire=max(r->memoire,tab[0][g]->memoire);
						R[nb_state]->fils1=copySolver(tab[0][g]);
						R[nb_state]->fils2=copySolver(r);
						R[nb_state]->type=2;
					}
					else R[nb_state]=copySolver(tab[0][g]);
					
					int pos=0;
					for (i = 0; i < g; i++) variables[nb_state][pos++]=copySolver(tab[0][i]);
					for (i = g+1; i < n[0]; i++) variables[nb_state][pos++]=copySolver(tab[0][i]);
					for (i = 0; i < n[1]; i++) variables[nb_state][pos++]=copySolver(tab[1][i]);
					for (i = 0; i < n[2]; i++) variables[nb_state][pos++]=copySolver(tab[2][i]);
					
					nb_total[nb_state]=new_nb_total;
					nb_guess[nb_state]=g;
					Eq[nb_state]=EsachantX(tab[0][g]->var,tab[0][g]->Nvar,F);
					nb_state++;
				}
			}
			freeSolver(r);
			for (i = 0; i < 3; i++)
			{
				int j;
				for (j = 0; j < n[i]; j++) freeSolver(tab[i][j]);					
			}
			
			goto fin;
			suite :
			
			if (one_solution_i==0 || (one_solution_i==2 && r!=NULL && r->sortie==T-1))
			{
				if (nb_state==Alloue)
				{
					Alloue=2*Alloue;
					R=realloc(R,Alloue*sizeof(Solver));
					nb_guess=realloc(nb_guess,Alloue*sizeof(int));
					nb_total=realloc(nb_total,Alloue*sizeof(int));
					Eq=realloc(Eq,Alloue*sizeof(SysEqLin));
					variables=realloc(variables,Alloue*sizeof(Solver *));
					for (i = Alloue/2; i < Alloue; i++) variables[i]=malloc(nb_variables*sizeof(Solver));
				}
				if (r!=NULL)
				{
					R[nb_state]=NewSolver();
					R[nb_state]->sortie=max(r->sortie+tab[one_solution_i][one_solution_j]->sortie,0);
					R[nb_state]->var=UnionListesTriees(r->var,r->Nvar,tab[one_solution_i][one_solution_j]->var,tab[one_solution_i][one_solution_j]->Nvar,&R[nb_state]->Nvar);
					R[nb_state]->temps=max(r->temps,max(r->sortie,0)+tab[one_solution_i][one_solution_j]->temps);
					R[nb_state]->memoire=max(r->memoire,tab[one_solution_i][one_solution_j]->memoire);
					R[nb_state]->fils1=tab[one_solution_i][one_solution_j];
					R[nb_state]->fils2=r;
					R[nb_state]->type=2;
				}
				else R[nb_state]=tab[one_solution_i][one_solution_j];
				tab[one_solution_i][one_solution_j]=tab[one_solution_i][--n[one_solution_i]];
				
				nb_total[nb_state]=n[0]+n[1]+n[2];
				if (r==NULL || r->sortie!=T-1) nb_guess[nb_state]=n[0];
				else nb_guess[nb_state]=nb_total[nb_state];
				
				if (nb_guess[nb_state]==0)
				{
					freeSolver(R[nb_state]);
					for (i = 1; i < 3; i++)
					{
						int j;
						for (j = 0; j < n[i]; j++) freeSolver(tab[i][j]);
					}
				}
				else
				{
					int pos=0;
					for (i = 0; i < 3; i++)
					{
						int j;
						for (j = 0; j < n[i]; j++) variables[nb_state][pos++]=tab[i][j];
					}
					Eq[nb_state]=EsachantX(R[nb_state]->var,R[nb_state]->Nvar,F);
					nb_state++;
				}
			}
			else
			{
				for (i = 0; i < 3; i++)
				{
					int j;
					for (j = 0; j < n[i]; j++) freeSolver(tab[i][j]);
				}
				freeSolver(r);
			}	
		}
		fin :
		freeSysEq(F);
		nb_round++;
		if (nb_round%0x100 == 0)
		{
			i=0;
			while (i<nb_state && R[i]->temps==1) i++;
			printf("\r %lu - %d - ",nb_round,i);
			fflush(stdout);
		}	
	}
	printf("\n");
	free(R);
	free(Eq);
	free(nb_guess);
	free(nb_total);
	for (i = 0; i < Alloue; i++) free(variables[i]);
	free(variables);
	for (i = 0; i < 3; i++) free(tab[i]);
	
	return result;
}

Solver * SearchAlgoLowMem_All(SysEqLin E, const int T, int *taille)
{
	const int nb_variables = E->a/E->gen;
	int Alloue=T*nb_variables;
	int Alloue_all_algo = Alloue;
	Solver * __restrict all_algo = malloc(Alloue_all_algo*sizeof(Solver));
	int nb_all_algo = 0;
	Solver * __restrict R = malloc(Alloue*sizeof(Solver));
	SysEqLin * __restrict Eq = malloc(Alloue*sizeof(SysEqLin));
	int * __restrict nb_guess = malloc(Alloue*sizeof(int));
	int * __restrict nb_total = malloc(Alloue*sizeof(int));
	Solver ** __restrict variables = malloc(Alloue*sizeof(Solver *));
	int i;
	for (i = 0; i < Alloue; i++) variables[i]=malloc(nb_variables*sizeof(Solver));
	Solver * tab[3];
	for (i = 0; i < 3; i++) tab[i]=malloc(nb_variables*sizeof(Solver));
	
	for (i = 0; i < nb_variables; i++) variables[0][i]=BaseSolver(E->varligne[i],E);
	R[0]=NULL;
	Eq[0]=EsachantX(NULL,0,E);
	nb_guess[0]=nb_variables;
	nb_total[0]=nb_variables;
	int nb_state = 1;
	
	Solver result = NULL;
	
	unsigned long int nb_round = 0;
	while (nb_state)
	{		
		nb_state--;
		
		SysEqLin F = Eq[nb_state];
		Solver r = R[nb_state];
		int n[3];
		n[0]=nb_guess[nb_state];
		n[1]=nb_total[nb_state]-n[0];
		n[2]=0;
			
			
		for (i = 0; i < n[0]; i++) tab[0][i]=variables[nb_state][i];
		for (i = 0; i < n[1]; i++) tab[1][i]=variables[nb_state][i+n[0]];
		
		int k;
		for (k = 0; k < 2; k++)
		{
			i=0;
			while (i<n[k])
			{
				int j=i-1;
				while (j>=0)
				{
					Solver tmp = UnionSolverBorne(tab[k][i],tab[k][j],F,min(tab[k][i]->sortie,tab[k][j]->sortie));
					if (tmp)
					{
						freeSolver(tab[k][i]);
						freeSolver(tab[k][j]);
						tab[k][i]=tab[k][--n[k]];
						i--;
						tab[k][j]=tab[k][i];
						tab[k][i]=tmp;							
						i--;
						break;
					}
					j--;
				}
				i++;
			}	
		}
		
		int i=0;
		while (i<n[0])
		{
			int j=0;
			while (j<n[1])
			{
				Solver tmp = UnionSolverBorne(tab[0][i],tab[1][j],F,min(tab[0][i]->sortie,tab[1][j]->sortie));
				if (tmp)
				{
					freeSolver(tab[0][i]);
					tab[0][i]=tab[0][--n[0]];
					freeSolver(tab[1][j]);
					tab[1][j]=tab[1][--n[1]];
					tab[2][n[2]++]=tmp;
					i--;
					break;
				}
				j++;
			}
			i++;
		}
		
		i=0;
		while (i<n[2])
		{
			int j;
			debut:
			j=i-1;
			while (j>=0)
			{
				Solver tmp = UnionSolverBorne(tab[2][i],tab[2][j],F,min(tab[2][i]->sortie,tab[2][j]->sortie));
				if (tmp)
				{
					freeSolver(tab[2][i]);
					freeSolver(tab[2][j]);
					tab[2][j]=tab[2][i-1];
					tab[2][i-1]=tmp;
					tab[2][i]=tab[2][--n[2]];
					i--;
					goto debut;
				}
				j--;
			}
			for (k = 0; k < 2; k++)
			{
				j=0;
				while (j < n[k])
				{
					Solver tmp = UnionSolverBorne(tab[2][i],tab[k][j],F,min(tab[2][i]->sortie,tab[k][j]->sortie));
					if (tmp)
					{
						freeSolver(tab[2][i]);
						freeSolver(tab[k][j]);
						tab[k][j]=tab[k][--n[k]];
						tab[2][i]=tmp;
						goto debut;
					}
					j++;
				}
			}
			i++;
		}		
		
		if (n[0]+n[1]+n[2]==1)
		{
			i=0;
			while (!n[i]) i++;
			if (r!=NULL)
			{
				result=NewSolver();
				result->sortie=max(r->sortie+tab[i][0]->sortie,0);
				result->var=UnionListesTriees(r->var,r->Nvar,tab[i][0]->var,tab[i][0]->Nvar,&result->Nvar);
				result->temps=max(r->temps,max(r->sortie,0)+tab[i][0]->temps);
				result->memoire=max(r->memoire,tab[i][0]->memoire);
				result->fils1=tab[i][0];
				result->fils2=r;
				result->type=2;
			}
			else result=tab[i][0];
						
			for (i = 0; i < nb_state; i++)
			{
				freeSolver(R[i]);
				int j;
				for (j = 0; j < nb_total[i]; j++) freeSolver(variables[i][j]);
				freeSysEq(Eq[i]);
			}
			nb_state=0;
		}
		else
		{
			int one_solution=0;
			int one_solution_i=0;
			int one_solution_j=0;
		
			for (i = 0; i < 3; i++)
			{
				int j;
				for (j = 0; j < n[i]; j++)
				{
					if (tab[i][j]->sortie<=0)
					{
						one_solution=1;
						one_solution_i=i;
						one_solution_j=j;
						goto suite;
					}
				}
			}
			
			if ((r==NULL && T>1) || r->sortie<T-1)
			{
				const int m = (r==NULL ? T : T-r->sortie);
				int g = n[0];
				const int new_nb_total = n[0]+n[1]+n[2]-1;
				while (g>=m)
				{
					if (nb_state==Alloue)
					{
						Alloue=2*Alloue;
						R=realloc(R,Alloue*sizeof(Solver));
						nb_guess=realloc(nb_guess,Alloue*sizeof(int));
						nb_total=realloc(nb_total,Alloue*sizeof(int));
						Eq=realloc(Eq,Alloue*sizeof(SysEqLin));
						variables=realloc(variables,Alloue*sizeof(Solver *));
						for (i = Alloue/2; i < Alloue; i++) variables[i]=malloc(nb_variables*sizeof(Solver));
					}
					g--;
					if (r!=NULL)
					{
						R[nb_state]=NewSolver();
						R[nb_state]->sortie=max(r->sortie+tab[0][g]->sortie,0);
						R[nb_state]->var=UnionListesTriees(r->var,r->Nvar,tab[0][g]->var,tab[0][g]->Nvar,&R[nb_state]->Nvar);
						R[nb_state]->temps=max(r->temps,max(r->sortie,0)+tab[0][g]->temps);
						R[nb_state]->memoire=max(r->memoire,tab[0][g]->memoire);
						R[nb_state]->fils1=copySolver(tab[0][g]);
						R[nb_state]->fils2=copySolver(r);
						R[nb_state]->type=2;
					}
					else R[nb_state]=copySolver(tab[0][g]);
					
					int pos=0;
					for (i = 0; i < g; i++) variables[nb_state][pos++]=copySolver(tab[0][i]);
					for (i = g+1; i < n[0]; i++) variables[nb_state][pos++]=copySolver(tab[0][i]);
					for (i = 0; i < n[1]; i++) variables[nb_state][pos++]=copySolver(tab[1][i]);
					for (i = 0; i < n[2]; i++) variables[nb_state][pos++]=copySolver(tab[2][i]);
					
					nb_total[nb_state]=new_nb_total;
					nb_guess[nb_state]=g;
					Eq[nb_state]=EsachantX(tab[0][g]->var,tab[0][g]->Nvar,F);
					nb_state++;
				}
			}
			else
			{
				for (i = 0; i < n[0]; i++)
				{
					{
						if (nb_all_algo==Alloue_all_algo)
						{
							Alloue_all_algo=2*Alloue_all_algo;
							all_algo=realloc(all_algo,Alloue_all_algo*sizeof(Solver));
						}
						if (r!=NULL)
						{
							all_algo[nb_all_algo]=NewSolver();
							all_algo[nb_all_algo]->sortie=max(r->sortie+tab[0][i]->sortie,0);
							all_algo[nb_all_algo]->var=UnionListesTriees(r->var,r->Nvar,tab[0][i]->var,tab[0][i]->Nvar,&all_algo[nb_all_algo]->Nvar);
							all_algo[nb_all_algo]->temps=max(r->temps,max(r->sortie,0)+tab[0][i]->temps);
							all_algo[nb_all_algo]->memoire=max(r->memoire,tab[0][i]->memoire);
							all_algo[nb_all_algo]->fils1=copySolver(tab[0][i]);
							all_algo[nb_all_algo]->fils2=copySolver(r);
							all_algo[nb_all_algo]->type=2;
						}
						else all_algo[nb_all_algo]=copySolver(tab[0][i]);
						nb_all_algo++;
					}
				}
			}
			freeSolver(r);
			for (i = 0; i < 3; i++)
			{
				int j;
				for (j = 0; j < n[i]; j++) freeSolver(tab[i][j]);					
			}
			
			goto fin;
			suite :
			
			if (one_solution_i==0 || (one_solution_i==2 && r!=NULL && r->sortie==T-1))
			{
				if (nb_state==Alloue)
				{
					Alloue=2*Alloue;
					R=realloc(R,Alloue*sizeof(Solver));
					nb_guess=realloc(nb_guess,Alloue*sizeof(int));
					nb_total=realloc(nb_total,Alloue*sizeof(int));
					Eq=realloc(Eq,Alloue*sizeof(SysEqLin));
					variables=realloc(variables,Alloue*sizeof(Solver *));
					for (i = Alloue/2; i < Alloue; i++) variables[i]=malloc(nb_variables*sizeof(Solver));
				}
				if (r!=NULL)
				{
					R[nb_state]=NewSolver();
					R[nb_state]->sortie=max(r->sortie+tab[one_solution_i][one_solution_j]->sortie,0);
					R[nb_state]->var=UnionListesTriees(r->var,r->Nvar,tab[one_solution_i][one_solution_j]->var,tab[one_solution_i][one_solution_j]->Nvar,&R[nb_state]->Nvar);
					R[nb_state]->temps=max(r->temps,max(r->sortie,0)+tab[one_solution_i][one_solution_j]->temps);
					R[nb_state]->memoire=max(r->memoire,tab[one_solution_i][one_solution_j]->memoire);
					R[nb_state]->fils1=tab[one_solution_i][one_solution_j];
					R[nb_state]->fils2=r;
					R[nb_state]->type=2;
				}
				else R[nb_state]=tab[one_solution_i][one_solution_j];
				tab[one_solution_i][one_solution_j]=tab[one_solution_i][--n[one_solution_i]];
				
				nb_total[nb_state]=n[0]+n[1]+n[2];
				if (r==NULL || r->sortie!=T-1) nb_guess[nb_state]=n[0];
				else nb_guess[nb_state]=nb_total[nb_state];
				
				if (nb_guess[nb_state]==0)
				{
					freeSolver(R[nb_state]);
					for (i = 1; i < 3; i++)
					{
						int j;
						for (j = 0; j < n[i]; j++) freeSolver(tab[i][j]);
					}
				}
				else
				{
					int pos=0;
					for (i = 0; i < 3; i++)
					{
						int j;
						for (j = 0; j < n[i]; j++) variables[nb_state][pos++]=tab[i][j];
					}
					Eq[nb_state]=EsachantX(R[nb_state]->var,R[nb_state]->Nvar,F);
					nb_state++;
				}
			}
			else
			{
				for (i = 0; i < 3; i++)
				{
					int j;
					for (j = 0; j < n[i]; j++) freeSolver(tab[i][j]);
				}
				freeSolver(r);
			}	
		}
		fin :
		freeSysEq(F);
		nb_round++;
		if (nb_round%0x100 == 0)
		{
			i=0;
			while (i<nb_state && R[i]->temps==1) i++;
			printf("\r %lu - %d - ",nb_round,i);
			fflush(stdout);
		}	
	}
	printf("\n");
	free(R);
	free(Eq);
	free(nb_guess);
	free(nb_total);
	for (i = 0; i < Alloue; i++) free(variables[i]);
	free(variables);
	for (i = 0; i < 3; i++) free(tab[i]);
	
	if (result==NULL)
	{
		*taille = nb_all_algo;
		return all_algo;
	}
	else
	{
		for (i = 0; i < nb_all_algo; i++) freeSolver(all_algo[i]);
		free(all_algo);
		all_algo=malloc(1*sizeof(Solver));
		all_algo[0]=result;
		*taille=1;
		return all_algo;
	}
	
}
	


/* **************************************************************************** */
/* 																				*/
/*                         Searchs exhaustives                               */
/* 																				*/
/* **************************************************************************** */

Solver * ExhaustiveSearch(Solver * __restrict const B1, const int N1, const int t, int m, int * __restrict taille, SysEqLin E) // Construit toutes les boites de temps <= t et de memoire <= m, ne garde que les plus interessante.
{
	if (m>t) m=t;
	int Alloue=2*N1;
	Solver * __restrict B=malloc(Alloue*sizeof(Solver));
	int Max=0,i;
	for (i = 0; i < N1; i++) /*if (B1[i]->temps<=t && B1[i]->memoire<=m)*/ B[Max++]=copySolver(B1[i]);
	int N=0;
	printf("Search exhaustive : \n");

	/* Partie en test pour les cas surdéterminés */
	Solver Sol = NULL;
	int temps_sol = t, memoire_sol = m+1;
	for (i = 0; i < Max; i++)
	{
		if (B[i]->sortie<=0 && (B[i]->temps<temps_sol || (B[i]->temps=temps_sol && B[i]->memoire<memoire_sol)))
		{
			freeSolver(Sol);
			Sol=copySolver(B[i]);
			temps_sol=B[i]->temps;
			memoire_sol=B[i]->memoire;
		}
	}
	i=0;
	/* La recherche */
	while (Max!=N)
	{
		TriFusion1(B+N,Max-N); // On favorise les boites les plus interessantes
		N=Max;
		while (i<N)
		{
			int j=i-1;
			while (j>=0) // On fait l'union de B[i] avec B[j], 0 <= j < i
			{
				// Un première passe pour voir si ça vaut le coup		
				if ((B[i]->sortie<=m || B[j]->sortie<=m) && !ContenuListesTriees(B[i]->var,B[i]->Nvar,B[j]->var,B[j]->Nvar) && !ContenuListesTriees(B[j]->var,B[j]->Nvar,B[i]->var,B[i]->Nvar))
				{
					//Solver tmp=UnionSolver(B[i],B[j],E);
					Solver tmp=UnionSolverBorne(B[i],B[j],E,t);
					// Une seconde passe pour voir si ça vaut le coup
					if (tmp!=NULL && tmp->temps<=t && (tmp->temps<temps_sol || (tmp->temps==temps_sol && tmp->memoire<memoire_sol)))
					{
						int k;
						for (k = 0; k < Max; k++) if (PlusInteressant(B[k],tmp) && ContenuListesTriees(tmp->var,tmp->Nvar,B[k]->var,B[k]->Nvar)) break;
						if (k==Max) // ça vaut le coup
						{
							Solver tmp2=PropagateSolver(tmp,E,B1,N1); // On l'augmente gratuitement avec les boites de "base"
							freeSolver(tmp);
							tmp=tmp2;
							k=0;
							
							/* On supprime celle qui sont moins interessante
							 * Le but est de toujours avoir j < i < N <= Max
							 * Les unions entre les i premières boites ont déjà été faite
							 */
							while (k<N)
							{
								if (PlusInteressant(tmp,B[k]) && ContenuListesTriees(B[k]->var,B[k]->Nvar,tmp->var,tmp->Nvar))
								{
									freeSolver(B[k]);
									if (k<j)
									{
										N--;
										B[k]=B[j-1];
										B[j-1]=B[j];
										B[j]=B[i-1];
										B[i-1]=B[i];
										B[i]=B[N];
										j--;
										i--;
										Max--;
										B[N]=B[Max];
									}
									else
									{
										if (k<i)
										{
											N--;
											B[k]=B[i-1];
											B[i-1]=B[i];
											B[i]=B[N];
											i--;
											Max--;
											B[N]=B[Max];
										}
										else
										{
											if (k==i)
											{
												N--;
												B[i]=B[N];
												i--;
												j=0;
												Max--;
												B[N]=B[Max];
											}
											else
											{
												N--;
												B[k]=B[N];
												Max--;
												B[N]=B[Max];
											}
										}
									}					
								}
								else k++;
							}
							while (k<Max)
							{
								if (PlusInteressant(tmp,B[k]) && ContenuListesTriees(B[k]->var,B[k]->Nvar,tmp->var,tmp->Nvar))
								{
									freeSolver(B[k]);
									Max--;
									B[k]=B[Max];
								}
								else k++;
							}
							if (tmp->sortie<=0) // En test
							{
								freeSolver(Sol);
								Sol=tmp;
								temps_sol=tmp->temps;
								memoire_sol=tmp->memoire;
							}
							else
							{
								/* On ajoute la boite à la liste et on continue */
								if (Max==Alloue)
								{
									Alloue=2*Alloue;
									B=realloc(B,Alloue*sizeof(Solver));					
								}
								/*printf(" Solver union :");
								PrintVarSolver(tmp);
								getchar();*/
								B[Max]=tmp;
								Max++;
							}
						}
						else freeSolver(tmp);
					}
					else freeSolver(tmp);
				}
				j--;
			}
			i++;
			if (i%0x10==0)
			{
				printf("\ri=%d, N=%d, Max=%d",i,N,Max);
				fflush(stdout);
			}
		}
	}
	printf("\ri=%d, N=%d, Max=%d\n",i,N,Max);
	fflush(stdout);
	if (Sol!=NULL) // En test
	{
		Solver * __restrict B1_tmp=malloc(N1*sizeof(Solver));
		int N1_tmp = 0;
		SysEqLin F = EsachantX(Sol->var,Sol->Nvar,E);
		for (i = 0; i < N1; i++)
		{
			B1_tmp[N1_tmp]=AjusteSolver(B1[i],F);
			if (B1_tmp[N1_tmp]!=NULL) N1_tmp++;
		}
		if (N1_tmp!=0)
		{
			int N_tmp;
			Solver * __restrict B_tmp = ExhaustiveSearch(B1_tmp,N1_tmp,t,m,&N_tmp,F);
			B=realloc(B,(N+N_tmp)*sizeof(Solver));
			for (i = 0; i < N_tmp; i++)
			{
				B[N++]=UnionSolver2(B_tmp[i],Sol,E);
				freeSolver(B_tmp[i]);
			}
			free(B_tmp);
			freeSolver(Sol);
		}
		else
		{
			B=realloc(B,(N+1)*sizeof(Solver));
			B[N++]=Sol;
		}
		freeSysEq(F);
		for (i = 0; i < N1_tmp; i++) freeSolver(B1_tmp[i]);
		free(B1_tmp);
	}
	else B=realloc(B,N*sizeof(Solver));
	*taille=N;
	return B;
}

Solver * ExhaustiveSearchBalancedSolver(SysEqLin E, const int T, int * taille)
{
	int nb_wait;
	Solver * __restrict wait=SearchAlgoLowMem_All(E,T,&nb_wait);		
	
	int alloc_wait = nb_wait;
	
	Solver * __restrict checked=malloc(nb_wait*sizeof(Solver));
	int alloc_checked = nb_wait;
	int nb_checked = 0;
	
	int nb_low_time;
	Solver * __restrict low_time=MakeSolver1(E,&nb_low_time);
	
	int max_checked=0;
	int max_wait=nb_wait;
	start_1 :
	while (nb_wait>0)
	{
		if (nb_checked>max_checked) max_checked=nb_checked;
		if (nb_wait>max_wait) max_wait=nb_wait;
		printf("\r wait : %d - checked : %d - ",nb_wait,nb_checked);
		Solver B = wait[--nb_wait];
		const int nb_wait_backup = nb_wait;
		int nb_staying = nb_checked;
		start_2 :
		while (nb_staying>0)
		{
			Solver result = UnionSolverBorne(B,checked[--nb_staying],E,T);
			if (result!=NULL)
			{
				if (result->Nvar==checked[nb_staying]->Nvar)
				{
					freeSolver(result);
					if (PlusInteressant2(checked[nb_staying],B))
					{
						freeSolver(B);
						int i;
						for (i = nb_wait_backup; i < nb_wait; i++) freeSolver(wait[i]);
						nb_wait=nb_wait_backup;
						goto start_1;
					}
					else goto start_2;
				}
				if (result->Nvar==B->Nvar)
				{
					if (PlusInteressant2(B,checked[nb_staying]))
					{
						freeSolver(checked[nb_staying]);
						checked[nb_staying--]=checked[--nb_checked];
					}
					freeSolver(result);
					goto start_2;
				}
				Solver tmp = PropagateSolver2(result,E,low_time,nb_low_time);
				freeSolver(result);
				if (PlusInteressant_NoVarsNoMem(tmp,B))
				{
					freeSolver(B);
					int i=nb_checked;
					while (i>0)
					{
						i--;
						if (ContenuListesTriees(checked[i]->var,checked[i]->Nvar,tmp->var,tmp->Nvar) && PlusInteressant_NoVarsNoMem(tmp,checked[i]))
						{
							freeSolver(checked[i]);
							checked[i]=checked[--nb_checked];
						}
					}
					if (nb_wait==alloc_wait)
					{
						alloc_wait=2*alloc_wait;
						wait=realloc(wait,alloc_wait*sizeof(Solver));
					}
					wait[nb_wait++]=tmp;
					goto start_1;
				}
				else
				{
					int i=nb_staying;
					while (i>0)
					{
						i--;
						if (ContenuListesTriees(checked[i]->var,checked[i]->Nvar,tmp->var,tmp->Nvar) && PlusInteressant_NoVarsNoMem(tmp,checked[i]))
						{
							freeSolver(checked[i]);
							checked[i]=checked[--nb_staying];
							checked[nb_staying]=checked[--nb_checked];
						}
					}
					i=nb_checked;
					while (i>nb_staying)
					{
						i--;
						if (ContenuListesTriees(checked[i]->var,checked[i]->Nvar,tmp->var,tmp->Nvar) && PlusInteressant_NoVarsNoMem(tmp,checked[i]))
						{
							freeSolver(checked[i]);
							checked[i]=checked[--nb_checked];
						}
					}
					if (nb_wait==alloc_wait)
					{
						alloc_wait=2*alloc_wait;
						wait=realloc(wait,alloc_wait*sizeof(Solver));
					}
					wait[nb_wait++]=tmp;					
				}		
			}
		}
		if (nb_checked==alloc_checked)
		{
			alloc_checked=2*alloc_checked;
			checked=realloc(checked,alloc_checked*sizeof(Solver));
		}
		checked[nb_checked++]=B;		
	}
	printf("\n");
	printf("\r wait : %d - checked : %d",max_wait,max_checked);
	free(wait);
	int i;
	for (i = 0; i < nb_low_time; i++) freeSolver(low_time[i]);
	free(low_time);
	*taille=nb_checked;
	return checked;
}

/* **************************************************************************** */
/* 																				*/
/*                   Génération heuristique de solvers                           */
/* 																				*/
/* **************************************************************************** */


/*
 * Toutes les fonctions qui suivent ont le même but : générer aléatoirement une solver de temps T et de memoire 1
 */ 


// La plus rapide mais les solvers générées sont de moins bonne "qualité"
Solver GenerateSolver(const int T, Solver *B1, const int N1, SysEqLin E) 
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
		B=tmp;
	}
	return B;
}

Solver GenerateSolver_manual(int T, Solver *B1, int N1, SysEqLin E, string_list_t *variable_names) // Search manuelle
{
	int i,N;
	Solver B,*B1_tmp,tmp;

	for (i = 0; i < N1; i++)
	{
		printf(" Solver %d (%d || %d || %d): ",i,B1[i]->temps,B1[i]->memoire,B1[i]->sortie);
		PrintVarSolver2(B1[i],variable_names);
	}
	do
	{
		printf("index of guessed solver : ");
		scanf(" %d",&i);
		getchar();
	} while (i<0 || i>=N1);
	//i=rand()%N1;
	N=N1-1;
	B=B1[i];
	B1[i]=B1[N];
	B1[N]=B;
	B=copySolver(B);
	B1_tmp=malloc(N*sizeof(Solver));
	for (i = 0; i < N; i++) B1_tmp[i]=copySolver(B1[i]);
	while (B->sortie<T && N>0)
	{
		for (i = 0; i < N; i++)
		{
			tmp=UnionSpeciale(B1_tmp[i],B,E);
			if (tmp->sortie>T || tmp->temps>T)
			{
				printf("Something wrong - 2204 - Search.c");
				getchar();
			}
			freeSolver(B1_tmp[i]);
			B1_tmp[i]=tmp;
		}
		N=ReduireSolvers(B1_tmp,N,E);
		TriFusion1(B1_tmp,N);
		if (B1_tmp[0]->sortie<=B->sortie)
		{
			freeSolver(B);
			tmp=B1_tmp[0];
			N--;
			B1_tmp[0]=B1_tmp[N];
			B=PropagateSolver2(tmp,E,B1,N1);
			freeSolver(tmp);
			for (i = 0; i < N; i++) freeSolver(B1_tmp[i]);
			N=0;
			int *test,Ntest;
			for (i = 0; i < N1; i++)
			{
				test=UnionListesTriees(B->var,B->Nvar,B1[i]->var,B1[i]->Nvar,&Ntest);
				free(test);
				if (Ntest>B->Nvar) B1_tmp[N++]=copySolver(B1[i]);
			}			
		}
		else
		{
			for (i = 0; i < N; i++)
			{
				printf(" Solver %d : ",i);
				PrintVarSolver2(B1_tmp[i]->fils1,variable_names);
			}
			do
			{
				printf("index of guessed solver : ? ");
				scanf(" %d",&i);
				getchar();
			} while (i<0 || i>=N);
			freeSolver(B);
			B=B1_tmp[i];
			N--;
			B1_tmp[i]=B1_tmp[N];
		}
	}
	if (B->Nvar<T || B->memoire>1 || B->temps>T)
	{
		printf("Something wrong - 2249 - Search.c : %d %d %d\n",B->memoire,B->temps,B->sortie);
		PrintVarSolver(B);
		getchar();
	}
	printf(" Solver built (%d || %d || %d): ",B->temps,B->memoire,B->sortie);
	PrintVarSolver2(B,variable_names);
	getchar();
	for (i = 0; i < N; i++) freeSolver(B1_tmp[i]);
	free(B1_tmp);
	return B;
}

/* **************************************************************************** */
/* 																				*/
/*                      Searchs heuristiques                                 */
/* 																				*/
/* **************************************************************************** */


/*
 *  Toutes les fonctions qui suivent marchent de la même façon :
 * 		- On génère une solver de temps T et de memoire 1 à l'aide des fonctions ci-dessus
 * 		- On fait l'union de cette solver avec celles déjà obtenues
 * 		- On recommence jusqu'à obtention d'une solution
 */



Solver RandomizedSearch(const int T, Solver *B1, const int N1, SysEqLin E)
{
	int Alloue=1000; // espace alloué pour stocker les solvers
	Solver * __restrict Candidat=malloc(Alloue*sizeof(Solver));
	int N=0;
	int v_max=0;
	int proba = 0;
	int pos=0;
	Solver * __restrict guess_probable = malloc(N1*sizeof(Solver));
	printf("Randomized Search : \n");
	unsigned int cpt=0;
	int cpt2=0;
	const int coeff_proba=200*E->gen/E->a;
	Solver tmp;
	while (1)
	{
		cpt++;
		// On génère une solver
		if (cpt2 < proba) tmp=GenerateSolver(T,guess_probable,pos,E);
		else tmp=GenerateSolver(T,B1,N1,E);
		int i=0;
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
					printf("\ncollision found!! Output : %d\n",tmp->sortie);
					if (tmp->sortie<T && tmp->sortie>0)
					{
						SysEqLin F = EsachantX(tmp->var,tmp->Nvar,E);
						tmp2 = SearchAlgoLowMem(F,T-tmp->sortie);
						if (tmp2)
						{
							Solver tmp3 = NewSolver();
							tmp3->var = UnionListesTriees(tmp->var,tmp->Nvar,tmp2->var,tmp2->Nvar,&tmp3->Nvar);
							tmp3->temps = max(tmp->temps,tmp->sortie+tmp2->temps);
							tmp3->memoire = max(tmp->memoire,tmp2->memoire);
							tmp3->sortie = tmp->sortie + tmp2->sortie;
							tmp3->type = 2;
							tmp3->fils2 = tmp;
							tmp3->fils1 = tmp2;
							tmp = tmp3;
						}
						freeSysEq(F);
					}
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
			if (tmp->Nvar>=v_max) // Elle a beaucoup de variables -> elle parait interessante
			{
				/* On cherchera une partie des solvers générées dans son complémentaire */
				v_max=tmp->Nvar;
				for (i = 0; i < pos; i++) freeSolver(guess_probable[i]);
				pos=0;
				for (i = 0; i < N1; i++) if (!ContenuListesTriees(B1[i]->var,B1[i]->Nvar,tmp->var,tmp->Nvar)) guess_probable[pos++]=copySolver(B1[i]);
				proba=v_max*coeff_proba;
			}
			if (cpt2 >= 199)
			{
				printf("\r - generated solvers : %u, number of variables max : %d / %d ",cpt,v_max,E->a/E->gen);
				fflush(stdout);
				cpt2=0;
			}
			else cpt2++;
			if (N==Alloue) // On a atteint la limite d'espace alloué
			{
				i=0;
				while (i<N) // On supprime celles qui semblent peu intéressantes
				{
					if (Candidat[i]->Nvar<=(v_max+T)/2 && Candidat[i]->memoire<=1)
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
	int i;
	for (i = 0; i < N; i++) freeSolver(Candidat[i]);
	free(Candidat);
	printf("\n");
	return NULL;
}

Solver ManualSearch(int T, Solver *B1, int N1, SysEqLin E, string_list_t *variable_names)
{
	int Alloue,N,i,N2,v_max;
	Solver *Candidat,tmp,tmp2;
	char c;
	Alloue=1000;
	Candidat=malloc(Alloue*sizeof(Solver));
	N=0;
	N2=N1/2;
	v_max=0;
	c='y';
	printf("Manual Search : \n");
	while (N<Alloue)
	{
		if (c=='y')
		{
			tmp=GenerateSolver_manual(T,B1,N1,E,variable_names);
			printf("Choose the next solver ? (y/n) :");
			scanf(" %c",&c);
			getchar();
		}
		else tmp=GenerateSolver(T,B1,N1,E);
		i=0;
		while (i < N)
		{
			tmp2=UnionSolver(Candidat[i],tmp,E);
			if (c=='o')
			{
				printf("Join (%d || %d || %d): ",tmp2->temps,tmp2->memoire,tmp2->sortie);
				PrintVarSolver2(tmp2,variable_names);
			}
			if (tmp2->temps<=T)
			{
				if (PlusInteressant2(Candidat[i],tmp2))
				{
					freeSolver(tmp2);
					break;
				}
				if (PlusInteressant2(tmp,tmp2))
				{
					N--;
					freeSolver(Candidat[i]);
					Candidat[i]=Candidat[N];
					freeSolver(tmp2);
				}
				else
				{
					printf("Two solvers were merged");
					printf("Join (%d || %d || %d): ",tmp2->temps,tmp2->memoire,tmp2->sortie);
					PrintVarSolver2(tmp2,variable_names);
					getchar();
					N--;
					freeSolver(Candidat[i]);
					Candidat[i]=Candidat[N];
					freeSolver(tmp);
					tmp=PropagateSolver2(tmp2,E,B1,N1);
					freeSolver(tmp2);
					i=0;
				}
			}
			else
			{
				freeSolver(tmp2);
				i++;
			}
		}
		if (i==N)
		{
			Candidat[N++]=tmp;
			v_max=max(v_max,tmp->Nvar);
			printf("Candidat %d (%d || %d || %d): ",i,tmp->temps,tmp->memoire,tmp->sortie);
			PrintVarSolver2(tmp,variable_names);
			if (N==Alloue)
			{
				i=0;
				while (i<N)
				{
					if (Candidat[i]->Nvar<=(T+v_max)/2)
					{
						freeSolver(Candidat[i]);
						N--;
						Candidat[i]=Candidat[N];
					}
					else i++;
				}
				if (N==Alloue)
				{
					Alloue=2*Alloue;
					Candidat=realloc(Candidat,Alloue*sizeof(Solver));
				}
			}		
		}
		else freeSolver(tmp);
		if (N==1 && Candidat[0]->Nvar==E->a/E->gen)
		{
			tmp=Candidat[0];
			free(Candidat);
			printf("Solution %d || %d : ",tmp->temps,tmp->memoire);
			PrintVarSolver(tmp);
			return tmp;
		}
	}
	for (i = 0; i < N; i++) freeSolver(Candidat[i]);
	free(Candidat);
	return NULL;
}


/* **************************************************************************** */
/* 																				*/
/*                        Search Algorithme                                  */
/* 																				*/
/* **************************************************************************** */


Solver SearchAlgo_tmp(SysEqLin E, int temps, int memoire, int r, string_list_t *variable_names, Symetric_structure S) // 2° partie de la recherche
{
	if (E->a==0) return NULL;
	
	char c;
	
	if (r==4)
	{
		do
		{
			printf("You will perform a manual search. Would you like to keep all the variables? (o/n) : ");
			scanf(" %c",&c);
			getchar();
		} while (c!='o' && c!='n');
	}
	else c='n';
	int N=E->a/E->gen;
	if (c=='n')
	{
		/* On enlève les variables apparaissant linéairement */
		int * __restrict X1=malloc(N*sizeof(int));
		int * __restrict X2=malloc(N*sizeof(int));
		int pos1=0;
		int pos2=0;
		int i;
		for (i = 0; i < N; i++)
		{
			if (ApparaitLineairement(E->varligne[i],E)) X1[pos1++]=E->varligne[i];
			else X2[pos2++]=E->varligne[i];
		}
		if (pos1>0)
		{
			SysEqLin F=ExtraireEX2(X2,pos2,E);
			Solver B=SearchAlgo_tmp(F,temps,memoire,r,variable_names,S);
			freeSysEq(F);
			if (B==NULL && pos2>0)
			{
				free(X1);
				free(X2);
				return NULL;
			}
			for (i = 0; i < pos1; i++)
			{
				Solver tmp=BaseSolver(X1[i],E);
				Solver tmp2=UnionSolver(B,tmp,E);
				freeSolver(tmp);
				freeSolver(B);
				B=tmp2;
			}
			free(X1);
			free(X2);
			return B;
		}
	}	
	
	int N1;
	Solver * __restrict B1=MakeSolver1(E,&N1);
	Solver B;


	if (N1==1)
	{
		B=B1[0];
		free(B1);
		return B;
	}
	
	if (B1[0]->sortie<=0) // Si l'une des solvers 1 a moins d'une solution autant la trouver et faire une recherche en la connaissant.
	{
		int i;
		for (i = 1; i < N1; i++) freeSolver(B1[i]);
		SysEqLin F=EsachantX(B1[0]->var,B1[0]->Nvar,E);
		B=SearchAlgo_tmp(F,temps,memoire,r,variable_names,S);
		freeSysEq(F);
		if (B==NULL)
		{
			freeSolver(B1[0]);
			free(B1);
			return NULL;
		}
		Solver tmp=NewSolver();
		tmp->sortie=B->sortie;
		tmp->var=UnionListesTriees(B1[0]->var,B1[0]->Nvar,B->var,B->Nvar,&tmp->Nvar);
		tmp->temps=max(B->temps,B1[0]->temps);
		tmp->memoire=max(B->memoire,B1[0]->memoire);
		tmp->fils1=B;
		tmp->fils2=B1[0];
		tmp->type=2;
		//tmp->parents=1;
		free(B1);
		return tmp;
	}
	
	if (temps<E->a/E->gen-E->b)
	{
		printf("We expect 2^(8 x %d) solutions for the non-linear part of the system ---> increase time\n",E->a/E->gen-E->b);
		return NULL;
	}
	
	UpdateSymetricStructure(S,E);
	
	Solver * __restrict B2;
	int N2,i;
	printf(" %d low time solvers \n",N1);
	switch (r)
	{
		case 0:
			B=RandomizedSearch(temps,B1,N1,E);
			if (B==NULL)
			{
				printf("No solution found in %d || %d\n",temps,temps);
				getchar();
			}
			break;
		case 1:
			B2 = ExhaustiveSearchBalancedSolver(E,temps,&N2);
			B=NULL;
			for (i = 0; i < N2; i++)
			{
				//printf("Solver %d :",i);
				//PrintVarSolver(B2[i]);
				if (B2[i]->Nvar==E->a/E->gen) B=copySolver(B2[i]);
				freeSolver(B2[i]);
			}
			if (B==NULL)
			{
				printf("There is no solution in %d || %d\n",temps,memoire);
				getchar();
			}
			free(B2);
			break;
		case 2:
			B = SearchAlgoLowMem(E,temps);
			if (B==NULL)
			{
				printf("No solution found in %d || %d\n",temps,temps);
				getchar();
			}
			break;
		case 3:
			if (S!=NULL && S->n_message>1)
			{
				B =SearchUneSolverN3_SymetricVersion(temps,B1,N1,E,S);
				if (B==NULL)
				{
					printf("No solution found in %d || %d\n",temps,temps);
					getchar();
				}
			}
			else
			{
				printf("search not available for this system\n");
				B=NULL;
			}
			break;
		case 4:
			B=ManualSearch(temps,B1,N1,E,variable_names);
			if (B==NULL)
			{
				printf("No solution found in %d || %d\n",temps,temps);
				getchar();
			}
			break;
		default:
			break;
	}
	if (B==NULL)
	{
		for (i = 0; i < N1; i++) freeSolver(B1[i]);
		free(B1);
		return NULL;
	}
	
	if (B->sortie<=0 && B->Nvar<E->a/E->gen) // On a trouvé une solver avec moins d'une solution -> on relance en supposant qu'on la connait
	{
		for (i = 0; i < N1; i++) freeSolver(B1[i]);
 		B1[0]=B;
		SysEqLin F=EsachantX(B1[0]->var,B1[0]->Nvar,E);
		B=SearchAlgo_tmp(F,temps,memoire,r,variable_names,S);
		freeSysEq(F);
		if (B==NULL)
		{
			freeSolver(B1[0]);
			free(B1);
			return NULL;
		}
		Solver tmp=NewSolver();
		tmp->sortie=B->sortie;
		tmp->var=UnionListesTriees(B1[0]->var,B1[0]->Nvar,B->var,B->Nvar,&tmp->Nvar);
		tmp->temps=max(B->temps,B1[0]->temps);
		tmp->memoire=max(B->memoire,B1[0]->memoire);
		tmp->fils1=B;
		tmp->fils2=B1[0];
		tmp->type=2;
		//tmp->parents=1;
		free(B1);
		return tmp;
	}
	for (i = 0; i < N1; i++) freeSolver(B1[i]);
	free(B1);
	return B;
}

Solver SearchAlgo(SysEqLin E, int temps, int memoire, int *Known, int NKnown, int r, string_list_t *variable_names, Symetric_structure S) // 1° partie de la recherche d'algo
{
	int N1;
	Solver * __restrict B1=MakeSolver1(E,&N1);

	/* Ajustement du système d'équations pour prendre en compte que x = y est équivalent à S(x) = S(y) */
	while (EnleveFaussesEq(B1,N1,E)==1)
	{
		int i;
		for (i = 0; i < N1; i++) freeSolver(B1[i]);
		free(B1);
		B1=MakeSolver1(E,&N1);
	}
	
	Solver B;
	
	if (N1==1) // Si il y a une seule solver de complexité 1 on a gagné
	{
		B=B1[0];
		free(B1);
	}
	else
	{
		int i;
		for (i = 0; i < N1; i++) freeSolver(B1[i]);
		free(B1);

		/* On rentre dans le vif */
		SysEqLin F=EsachantX(Known,NKnown,E);
		B=SearchAlgo_tmp(F,temps,memoire,r,variable_names,S);
		
		freeSysEq(F);
	}
	
	return B;
}


/* **************************************************************************** */
/* 																				*/
/*                        Fonctions diverses                                    */
/* 																				*/
/* **************************************************************************** */


int * GuessMinimumLight_tmp(int *X, int N, SysEqLin E, int *taille)
{
	if (N>0)
	{
		int i;
		for (i = 0; i < N; i++)
		{
			int l=E->lignevar[X[i]]*E->gen;
			int dim=Dimension(E->mat+l,E->gen,E->b,1);
			if (dim==1)
			{
				N--;
				X[i]=X[N];
				SysEqLin F = ExtraireEX2(X,N,E);
				int * result = GuessMinimumLight_tmp(X,N,F,taille);
				freeSysEq(F);
				return result;
			}
			if (dim==0)
			{
				int tmp = X[i];
				X[i]=X[--N];
				int * result_tmp = GuessMinimumLight_tmp(X,N,E,taille);
				int * result = UnionListesTriees(result_tmp,*taille,&tmp,1,taille);
				free(result_tmp);
				return result;
			}
		}
		SysEqLin F = ExtraireEX2(X,--N,E);
		int * result = GuessMinimumLight_tmp(X,N,F,taille);
		freeSysEq(F);
		return result;
	}
	else
	{
		*taille=0;
		return NULL;
	}
}

int * GuessMinimumLight(int *X, int N, SysEqLin E, int *taille)
{
	int * __restrict X_tmp = malloc(N*sizeof(int));
	int i;
	for (i = 0; i < N; i++) X_tmp[i]=X[i];
	int * result = GuessMinimumLight_tmp(X_tmp,N,E,taille);
	free(X_tmp);
	return result;
}


