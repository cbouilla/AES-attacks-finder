#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "Search.h"
#include "Bazar.h"
#include "Algebre.h"
#include "parser_machinery.h"
#include "Solver.h"
#include "SysEqLin.h"


extern const int VARIABLES;

/* **************************************************************************** */
/* 																				*/
/*                       Gestion mémoire Solver                                  */
/* 																				*/
/* **************************************************************************** */

#define Taille_bloc_pour_allocation_Espace 10000

int Nombre_de_Solver_disponible_pour_allocation = 0, Taille_Espace_Solver_pour_allocation = 0;
Solver * __restrict Espace_Solver_pour_allocation = NULL;

Solver NewSolver(void) // Renvoi l'adresse d'une SOLVER disponible
{
	static int Initialiser_Espace = 1;
		
	if (Nombre_de_Solver_disponible_pour_allocation) return Espace_Solver_pour_allocation[--Nombre_de_Solver_disponible_pour_allocation];
	else
	{
		if (Initialiser_Espace)
		{
			Initialiser_Espace = 0;
			Espace_Solver_pour_allocation = malloc(Taille_bloc_pour_allocation_Espace*sizeof(Solver));
			Taille_Espace_Solver_pour_allocation = Taille_bloc_pour_allocation_Espace;
		}
		SOLVER * __restrict tmp = malloc(Taille_bloc_pour_allocation_Espace*sizeof(SOLVER));
		int i;
		for (i = 0; i < Taille_bloc_pour_allocation_Espace; i++)
		{
			Espace_Solver_pour_allocation[i]=tmp++;
			Espace_Solver_pour_allocation[i]->parents=1;
		}
		Nombre_de_Solver_disponible_pour_allocation = Taille_bloc_pour_allocation_Espace;
		return Espace_Solver_pour_allocation[--Nombre_de_Solver_disponible_pour_allocation];
	}
}
	
Solver copySolver(Solver a) // copie une solver "pour de faux" (NE SURTOUT PAS MODIDIFIER UNE SOLVER DIRECTEMENT)
{
	if (a!=NULL) (a->parents)++; // mise à jour du nombre de pointeurs vers cette solver
	return a;
}

void freeSolver(Solver a) // libère la mémoire associée à une solver
{
	if (a!=NULL)
	{
		if (a->parents==1) // elle n'est plus utilisée
		{
			if (Taille_Espace_Solver_pour_allocation == Nombre_de_Solver_disponible_pour_allocation)
			{
				Taille_Espace_Solver_pour_allocation+= Taille_bloc_pour_allocation_Espace;
				Espace_Solver_pour_allocation = realloc(Espace_Solver_pour_allocation, Taille_Espace_Solver_pour_allocation*sizeof(Solver));
			}
			Espace_Solver_pour_allocation[Nombre_de_Solver_disponible_pour_allocation++]=a;
			free(a->var);
			freeSolver(a->fils1);
			freeSolver(a->fils2);
		}
		else (a->parents)--;
	}
}	



/* **************************************************************************** */
/* 																				*/
/*                       Interface xml <-> Solver                                */
/* 																				*/
/* **************************************************************************** */

Solver charge_solver(FILE *out, string_list_t *variable_names) // charge une solver à partir d'un xml
{
	Solver B=NewSolver();
	unsigned char c = fgetc(out);
	while (c!='\n') c = fgetc(out);
	while (c!='>') c = fgetc(out);
	fscanf(out,"%d",&B->temps);
	while (c!='\n') c = fgetc(out);
	while (c!='>') c = fgetc(out);
	fscanf(out,"%d",&B->memoire);
	while (c!='\n') c = fgetc(out);
	while (c!='>') c = fgetc(out);
	fscanf(out,"%d",&B->sortie);
	while (c!='\n') c = fgetc(out);
	while (c!='>') c = fgetc(out);

	{
		int type;
		fscanf(out,"%d",&type);
		B->type=type;
	}
	
	while (c!='\n') c = fgetc(out);
	while (c!='>') c = fgetc(out);
	B->Nvar=0;
	B->var=NULL;
	c = fgetc(out);
	while (c!='s')
	{
		char s[80];
		while (c!='>') c = fgetc(out);
		int pos=0;
		c = fgetc(out);
		while (c!='<')
		{
			s[pos++]=c;
			c = fgetc(out);
		}
		s[pos]='\0';
		while (c!='>') c = fgetc(out);
		c = fgetc(out);
		int i = find_string(variable_names,s);
		if (i==-1)
		{
			printf("le système d'équations ne correspond pas!!");
			getchar();
		}
		
		int *X=UnionListesTriees(&i,1,B->var,B->Nvar,&B->Nvar);
		free(B->var);
		B->var=X;
		while (c!='>' && c!='s') c = fgetc(out);
	}
	//B->parents=1;
	while (c!='\n') c = fgetc(out);
	c = fgetc(out);
	if (B->type!=0)
	{
		B->fils1=charge_solver(out,variable_names);
		B->fils2=charge_solver(out,variable_names);
	}
	else
	{
		B->fils1=NULL;
		B->fils2=NULL;
	}
	while (c!='\n') c = fgetc(out);
	return B;
}

void print_solver(FILE *out, Solver B, string_list_t *variable_names) // crée le xml associé à une solver
{
  int i;

  fprintf(out,"<solver>\n");
  fprintf(out,"  <time>%d</time>\n",  B->temps);
  fprintf(out,"  <memory>%d</memory>\n", B->memoire);
  fprintf(out,"  <output>%d</output>\n", B->sortie);
  fprintf(out,"  <type>%d</type>\n", B->type);
  fprintf(out,"  <variables>");
  for(i=0; i<B->Nvar; i++)
    fprintf(out,"  <variable>%s</variable>\n", find_var_name(variable_names, B->var[i]));
  fprintf(out,"</variables>\n");
  if (B->fils1 != NULL) print_solver(out, B->fils1, variable_names);
  if (B->fils2 != NULL) print_solver(out, B->fils2, variable_names);
  fprintf(out,"</solver>\n");
}



/* **************************************************************************** */
/* 																				*/
/*                   Operations simples sur le type Solver                       */
/* 																				*/
/* **************************************************************************** */


void PrintVarSolver(Solver B) // Print les variables d'une solver (sous la forme d'entiers)
{
	int i;
	for (i = 0; i < B->Nvar; i++) printf("%d ",B->var[i]);
	printf("\n");
}

void PrintVarSolver2(Solver B, string_list_t *variable_names) // Print les variables d'une solver
{
	int i;
	for (i = 0; i < B->Nvar; i++) printf("%s ",find_var_name(variable_names, B->var[i]));
	printf("\n");
}

Solver BaseSolver(int i, SysEqLin E) // Solver associée à la variable i
{
	if (E->gen*E->lignevar[i]<E->a)
	{
		Solver B = NewSolver();
		B->var=malloc(sizeof(int));
		B->Nvar=1;
		B->var[0]=i;
		#if 0
		B->initialisation=E;
		B->mat=InfoSortie_VariablesVersion(B->var,B->Nvar,E);
		B->sortie=B->mat->nombre_de_lignes-(E->gen-1);
		#else
		B->sortie=TailleE(B->var,1,E);
		#endif
		B->temps=1;
		B->memoire=1; // mettre 0 cause quelques problèmes, en plus il faut bien avoir la table de la Sbox
		B->type=0;
		B->fils1=NULL;
		B->fils2=NULL;
		return B;
	}
	else return NULL;
}

int SameSolvers(Solver B1, Solver B2) // Vrai si les variables des deux solvers sont les mêmes. 
{
	if (B1->Nvar==B2->Nvar)
	{
		if (B1==B2) return 1;
		else
		{
			int i;
			for (i = 0; i < B1->Nvar; i++) if (B1->var[i]!=B2->var[i]) return 0;
			return 1;
		}
	}
	else return 0;
}

#if 0
int PlusInteressant(Solver B1, Solver B2) // Vrai si la solver B1 est plus intéressante que la solver B2. VERIFIER AU PREALABLE QUE LES SOLVERS SONT COMPARABLES!
{
	return (B1->Nvar>=B2->Nvar && B1->sortie<=B2->sortie && (B1->temps<B2->temps || (B1->temps==B2->temps && B1->memoire<=B2->memoire)));
}

int PlusInteressant2(Solver B1, Solver B2) // Vrai si la solver B1 est plus intéressante que la solver B2 sauf en mémoire. VERIFIER AU PREALABLE QUE LES SOLVERS SONT COMPARABLES!
{
	return (B1->Nvar>=B2->Nvar && B1->sortie<=B2->sortie && B1->temps<=B2->temps);
}
#endif


void ParcoursSolver(Solver B) // Permet de parcourir une solver, utile pour debugguer
{
	printf("Solver (%d || %d || %d) - %d : ",B->temps,B->memoire,B->sortie,B->type);
	PrintVarSolver(B);
	if (B->type!=0)
	{
		printf("Solver 1 (%d || %d || %d) - %d : ",B->fils1->temps,B->fils1->memoire,B->fils1->sortie,B->fils1->type);
		PrintVarSolver(B->fils1);
		printf("Solver 2 (%d || %d || %d) - %d : ",B->fils2->temps,B->fils2->memoire,B->fils2->sortie,B->fils2->type);
		PrintVarSolver(B->fils2);
		int i;
		if (B->fils1->Nvar==1) i = 2; // zap les unions simples
		else
		{
			if (B->fils2->Nvar==1) i = 1;
			else
			{
				printf("Quelle solver : ?");
				scanf("%d",&i);
				getchar();
			}
		}
		if (i==1) ParcoursSolver(B->fils1);
		else ParcoursSolver(B->fils2);
	}	
}

void TriFusion1_tmp(Solver * __restrict B1, const int N, Solver * __restrict B)
{
	if (N>=2)
	{
		const int N1=N/2;
		TriFusion1_tmp(B1,N1,B);
		TriFusion1_tmp(B1+N1,N-N1,B);
		if (B1[N1]->sortie<=B1[N1-1]->sortie && (B1[N1]->sortie!=B1[N1-1]->sortie || B1[N1]->Nvar>=B1[N1-1]->Nvar)) // On vérifie qu'elle n'est pas déjà triée
		{
			int i;
			for (i = 0; i < N1; i++) B[i]=B1[i];
			i=0;
			int j=N1;
			int cpt=0;
			while (i<N1 && j<N)
			{
				if ((B[i]->sortie==B1[j]->sortie && B[i]->Nvar >= B1[j]->Nvar) || B[i]->sortie<B1[j]->sortie ) B1[cpt++]=B[i++];
				else B1[cpt++]=B1[j++];
			}
			for (i = i; i < N1; i++) B1[cpt++]=B[i];
		}	
	}
}

void TriFusion1(Solver * __restrict B1, const int N) // Trie une liste de solvers selon 1) la sortie dans l'ordre croissant 2) le nombre de variables dans l'ordre décroissant.
{
	if (N>=2)
	{
		Solver * __restrict B=malloc((N/2)*sizeof(Solver));
		TriFusion1_tmp(B1,N,B);
		free(B);
	}
}
	

/* **************************************************************************** */
/* 																				*/
/*                   Operations "complexes" sur les solvers                      */
/* 																				*/
/* **************************************************************************** */


int TimeForOne(Solver B) // Renvoi la complexité pour avoir la première solution (PROBABLEMENT FAUSSE)
{
	//if (B->type==0) return min(1-B->sortie,B->temps);
	if (B->type==0) return (B->sortie >= 0 ? 0 : 1);
	if (B->sortie<=0) return B->temps;
	if (B->type==2) return max(TimeForOne(B->fils2),TimeForOne(B->fils1));
	int t1=TimeForOne(B->fils1);
	int t2=B->fils2->temps;
	return min(max(t2,t1+max(B->fils1->sortie-B->sortie,0)),B->temps);
}	

Solver SimplifierTypeSolver(Solver B) // Si B est de type 2, renvoi une solver equivalente où fils1 n'est pas de type 2.
{
	if (B->type!=2 || B->fils1->type!=2) return copySolver(B);
	int N;
	int *X=UnionListesTriees(B->fils1->fils2->var,B->fils1->fils2->Nvar,B->fils2->var,B->fils2->Nvar,&N);
	Solver tmp=NewSolver();
	tmp->var=malloc(B->Nvar*sizeof(int));
	tmp->Nvar=B->Nvar;
	int i;
	for (i = 0; i < tmp->Nvar; i++) tmp->var[i]=B->var[i];
	tmp->sortie=B->sortie;
	tmp->fils2=UnionSolver_tmp(B->fils2,B->fils1->fils2,X,N,B->sortie-B->fils1->fils1->sortie);
	tmp->fils1=copySolver(B->fils1->fils1);
	tmp->temps=max(tmp->fils2->temps,tmp->fils1->temps + max(0,tmp->fils2->sortie));
	tmp->memoire=max(tmp->fils1->memoire,tmp->fils2->memoire);
	tmp->type=2;
	Solver tmp2=SimplifierTypeSolver(tmp);
	freeSolver(tmp);
	return tmp2;
}

Solver UnionSolver_tmp(Solver B1, Solver B2, int *X, const int n, const int sortie) // Réalise l'union de deux solvers. (Appelée par UnionSolver)
{	
	#if 0 // traité dans UnionSolver
	/* Cas où une solver est contenue dans l'autre */
	if (n==B1->Nvar)
	{
		free(X);
		if (PlusInteressant(B2,B1)) return copySolver(B2);
		else return copySolver(B1);
	}
	if (n==B2->Nvar)
	{
		free(X);
		return copySolver(B2);
	}
	#endif
	
	Solver __restrict B = NewSolver();
	B->var=X;
	B->Nvar=n;
	B->sortie=sortie;
	
	/* Cas où les deux solvers sont indépendantes (E(X1 u X2) = E(X1) + E(X2)) */
	if (B->sortie==B1->sortie + B2->sortie)
	{
		const int t1=max(max(B1->sortie,0) + B2->temps, B1->temps);
		const int t2=max(max(B2->sortie,0) + B1->temps, B2->temps);
		if (min(t1,t2)<=max(sortie,max(B1->temps,B2->temps)))
		{
			B->type=2;
			if (t1<t2 || (t1==t2 && OrdreListe(B2->var,B2->Nvar,B1->var,B1->Nvar)))
			{
				B->temps=t1;
				B->fils2=copySolver(B1);
				B->fils1=copySolver(B2);
			}
			else
			{
				B->temps=t2;
				B->fils2=copySolver(B2);
				B->fils1=copySolver(B1);
			}
			B->memoire=max(B1->memoire,B2->memoire);
			Solver tmp=SimplifierTypeSolver(B);
			freeSolver(B);
			return tmp;
		}
	}
	/* Cas où les deux solvers sont de type A -> B et A -> C */
	/*if (B1->type==2 && B2->type==2 && SameSolvers(B1->fils2,B2->fils2))
	{
		if (B1->fils2->temps<B2->fils2->temps || (B1->fils2->temps==B2->fils2->temps && B1->fils2->memoire<B2->fils2->memoire)) B->fils2=copySolver(B1->fils2);
		else B->fils2=copySolver(B2->fils2);
		int n_tmp;
		int *X_tmp=UnionListesTriees(B1->fils1->var,B1->fils1->Nvar,B2->fils1->var,B2->fils1->Nvar,&n_tmp);
		B->fils1=UnionSolver_tmp(B1->fils1,B2->fils1,X_tmp,n,B->sortie-B->fils2->sortie);
		B->temps=max(B->fils1->temps+max(B->fils2->sortie,0),B->fils2->temps);
		B->memoire=max(B->fils1->memoire,B->fils2->memoire);
		B->type=2;		
		Solver tmp=SimplifierTypeSolver(B);
		freeSolver(B);
		return tmp;
	}	
	else*/
	{
		/* Cas où l'on utilise de la mémoire */
		B->temps=max(B1->temps,max(B2->temps,B->sortie));
		if (max(B1->sortie,0)>max(B2->sortie,0) || (max(B1->sortie,0)==max(B2->sortie,0)  && (B1->Nvar>B2->Nvar || (B1->Nvar==B2->Nvar && B1->memoire<=B2->memoire))))
		{
			B->memoire=max(B2->sortie,max(B1->memoire,B2->memoire));
			B->fils1=copySolver(B1);
			B->fils2=copySolver(B2);
		}
		else
		{
			B->memoire=max(B1->sortie,max(B1->memoire,B2->memoire));
			B->fils1=copySolver(B2);
			B->fils2=copySolver(B1);
		}
		B->type=1;
		return B;
	}
}
	

Solver UnionSolver(Solver B1, Solver B2, SysEqLin E) // Réalise l'union de deux solvers. (Les solvers sont supposées être associée à E)
{
	if (B1!=NULL && B2!=NULL)
	{
		int n;
		int *X=UnionListesTriees(B1->var,B1->Nvar,B2->var,B2->Nvar,&n);
		if (n != max(B1->Nvar,B2->Nvar))
		{
			#if 0
			MatriceCreuse tmp = InfoSortie_UnionVersion(B1,B2,E);
			Solver B = UnionSolver_tmp(B1,B2,X,n,tmp->nombre_de_lignes-(E->gen-1)*n);
			/*if (B->sortie!=TailleE(X,n,E))
			{
				printf("ça craint du boudin\n");
				getchar();
			}*/
			
			B->initialisation = E;
			B->mat = tmp;
			return B;
			#else
			return UnionSolver_tmp(B1,B2,X,n,TailleE(X,n,E));
			#endif
		}
		//if (n != max(B1->Nvar,B2->Nvar)) return UnionSolver_tmp(B1,B2,X,n,TailleE_Borne(X,n,E,B1->sortie+B2->sortie-1));
		else // Les variables d'une solvers sont contenues dans les variables de l'autre
		{
			free(X);
			if (n==B1->Nvar)
			{
				if (PlusInteressant(B2,B1)) return copySolver(B2);
				else return copySolver(B1);
			}
			else return copySolver(B2);
		}
	}
	else
	{
		if (B1==NULL) return copySolver(B2);
		else return copySolver(B1);
	}
}


Solver UnionSolverBorne(Solver B1, Solver B2, SysEqLin E, const int borne) // Réalise l'union de deux solvers ssi le résultat à une sortie <= borne
{
	if (B1!=NULL && B2!=NULL)
	{
		int n;
		int *X=UnionListesTriees(B1->var,B1->Nvar,B2->var,B2->Nvar,&n);
		if (n != max(B1->Nvar,B2->Nvar))
		{
			#if 0
			const int cst = (E->gen-1)*n;
			MatriceCreuse tmp = InfoSortie_UnionBorneVersion(B1,B2,E,borne+cst);
			if (tmp->nombre_de_lignes == -1)
			{
				free(X);
				freeMatriceCreuse(tmp);
				return NULL;
			}
			else
			{
				Solver B = UnionSolver_tmp(B1,B2,X,n,tmp->nombre_de_lignes-cst);
				/*const int t = TailleE(B->var,B->Nvar,E);
				MatriceCreuse tmp2 = InfoSortie_VariablesVersion(B->var,B->Nvar,E);
				if (B->sortie!=t)
				{
					printf("sortie = %d au lieu de %d\n",B->sortie,t);
					printf("TailleE (%d) :\n",tmp2->nombre_de_lignes-cst);
					PrintMatCreuse(tmp2);
					printf("Premier bloc :\n");
					PrintMatCreuse(B1->mat);
					printf("Deuxieme bloc :\n");
					PrintMatCreuse(B2->mat);
					printf("Resultat (n = %d) :\n",n);
					PrintMatCreuse(tmp);
					getchar();
				}*/
				B->initialisation = E;
				B->mat = tmp;
				return B;
			}
			#else
			const int sortie = TailleE_Borne(X,n,E,borne);
			if (sortie<=borne) return UnionSolver_tmp(B1,B2,X,n,sortie);
			else
			{
				free(X);
				return NULL;
			}
			#endif
			
			
			
		}
		else // Les variables d'une solvers sont contenues dans les variables de l'autre
		{
			free(X);
			if (n==B1->Nvar)
			{
				if (PlusInteressant(B2,B1)) return copySolver(B2);
				else return copySolver(B1);
			}
			else return copySolver(B2);
		}
	}
	else
	{
		if (B1==NULL) return copySolver(B2);
		else return copySolver(B1);
	}
}

Solver UnionSpeciale(Solver B1, Solver B2, SysEqLin E) // fonction bizarre que j'avais crée pour un cas spéciale. NE PAS UTILISER
{
	Solver B,tmp;
	B=NewSolver();
	if (B1->type==2) tmp=copySolver(B1->fils1);
	else tmp=copySolver(B1);
	B->var=UnionListesTriees(tmp->var,tmp->Nvar,B2->var,B2->Nvar,&(B->Nvar));
	#if 0
	B->initialisation=E;
	B->mat=InfoSortie_UnionVersion(B1,B2,E);
	B->sortie=B->mat->nombre_de_lignes - (E->gen-1)*B->Nvar;
	#else
	B->sortie=TailleE(B->var,B->Nvar,E);
	#endif
	B->temps=max(B->sortie,max(tmp->temps+B2->sortie,B2->temps));
	B->memoire=max(tmp->memoire,B2->memoire);
	B->type=2;
	B->fils1=tmp;
	B->fils2=copySolver(B2);
	if (B->memoire>1)
	{
		printf("C'est très bizarre : %d %d %d",B->memoire,B1->memoire,B2->memoire);
		getchar();
	}
	return B;
}

Solver UnionSolver2(Solver B1, Solver B2, SysEqLin E) // Retourne la solver B2 -> B1. PEUT PRESENTER DES PROBLEMES SI B1 n B2 NON VIDE.
{
	Solver B = NewSolver();
	B->var=UnionListesTriees(B1->var,B1->Nvar,B2->var,B2->Nvar,&B->Nvar);
	#if 0
	B->initialisation=E;
	B->mat=InfoSortie_UnionVersion(B1,B2,E);
	B->sortie=B->mat->nombre_de_lignes - (E->gen-1)*B->Nvar;
	#else
	B->sortie=TailleE(B->var,B->Nvar,E);
	#endif
	B->temps = max(B2->temps,max(0,B2->sortie)+B1->temps);
	B->memoire=max(B1->memoire,B2->memoire);
	B->type=2;
	B->fils1=copySolver(B1);
	B->fils2=copySolver(B2);
	return B;
}

int ReduireSolvers_tmp(Solver *B, int Nsolvers, SysEqLin E, int DejaTester)  // Comme ci-dessous mais la liste restreinte au "DejaTester" premiers éléments est dejà réduite.
{
	int i=DejaTester;
	while (i<Nsolvers)
	{
		int j=i-1;
		while (j>=0)
		{
			//Solver tmp=UnionSolver(B[i],B[j],E);
			Solver tmp=UnionSolverBorne(B[i],B[j],E,min(B[i]->sortie,B[j]->sortie));
			//if (PlusInteressant(tmp,B[i]) && PlusInteressant(tmp,B[j]))
			if (tmp!=NULL && PlusInteressant_NoVars(tmp,B[i]) && PlusInteressant_NoVars(tmp,B[j]))
			{
				freeSolver(B[i]);
				Nsolvers--;
				B[i]=B[Nsolvers];
				freeSolver(B[j]);
				i--;
				B[j]=B[i];
				B[i]=tmp;
				j=i-1;
			}
			else
			{
				freeSolver(tmp);
				j--;
			}
		}
		i++;
	}
	return Nsolvers;
}
	
inline int ReduireSolvers(Solver *B, int Nsolvers, SysEqLin E) // Reduit une liste de solvers en le sens que si A u B est plus interessant que A ET que B, on met à jour. 
{
	return ReduireSolvers_tmp(B,Nsolvers,E,1);
}

Solver * MakeSolver1(SysEqLin E, int *N1) // crée la liste réduite des solvers de complexité 1 associée à E
{
	int N=E->a/E->gen; // Nombre de variables qui ne sont pas connues.
	
	Solver * __restrict B1=malloc(N*sizeof(Solver)); // Il y a au plus N solvers 1

	int i;
	for (i = 0; i < N; i++) B1[i]=BaseSolver(E->varligne[i],E);
	
	N=ReduireSolvers(B1,N,E); // On réduite la liste.
	
	//TriFusion1(B1,N); // On la tri. (Pratique pour repérer les solvers 1 de sortie nulle)
	for (i = 0; i < N; i++)
	{
		if (B1[i]->sortie<=0)
		{
			Solver tmp = B1[i];
			B1[i]=B1[0];
			B1[0]=tmp;
			break;
		}
	}
	
	
	#if 0 // Activer pour afficher les solvers 1
	for (i = 0; i < N; i++)
	{
		printf("Solver %d : (%d %d %d) : ",i,B1[i]->temps,B1[i]->memoire,B1[i]->sortie);
		PrintVarSolver(B1[i]);
		#if 0 // Activer pour voir les équations des solvers 1
		if (B1[i]->type!=0)
		{
			SysEqLin F=ExtraireEX2Special(B1[i]->var,B1[i]->Nvar,E);
			Equation *Eq=ConversionSys2(B1[i]->var,B1[i]->Nvar,F);
			if (F->b>0)
			{
				int j;
				printf("Equations : \n");
				for (j = 0; j < F->b; j++)
				{
					PrintEq(Eq[j]);
					freeEq(Eq[j]);
					printf("=");
					if (EstNulEq(Eq[j+F->b])) printf("0");
					else PrintEq(Eq[j+F->b]);
					freeEq(Eq[j+F->b]);
					printf("\n");
				}
			}
			freeSysEq(F);
			free(Eq);
		}
		#endif 
	}
	getchar();
	#endif
	*N1=N;
	return B1;
}


/* **************************************************************************** */
/* 																				*/
/*                   Améliorations des solvers                                   */
/* 																				*/
/* **************************************************************************** */



Solver PropagateSolver(Solver B, SysEqLin E, Solver *B1, int N1) // Renvoi la solver B augmentée gratuitement avec le maximum de solvers de la liste B1
{
	Solver B2=copySolver(B);
	int i=0;
	while (i < N1)
	{
		Solver tmp=UnionSolverBorne(B2,B1[i],E,B2->sortie);
		if (tmp==NULL || !PlusInteressant_NoVars(tmp,B2)) // cas où la solver n'est pas améliorée "gratuitement" (cas le plus fréquent)
		{
			freeSolver(tmp);
			i++;
		}
		else
		{
			Solver tmp2=B1[i];
			N1--;
			B1[i]=B1[N1];
			B1[N1]=tmp2;
			if (tmp->Nvar==B2->Nvar) freeSolver(tmp); // cas B1[i] contenue dans B2
			else // cas où l'on améliore "gratuitement" la solver B2
			{
				freeSolver(B2);
				B2=tmp;
				i=0;
			}
			
		}
	}
	return B2;
}

Solver PropagateSolver2(Solver B, SysEqLin E, Solver *B1, int N1) // Pareil que PropagateSolver mais la mémoire ne compte pas
{
	Solver B2=copySolver(B);
	int i=0;
	while (i < N1)
	{
		Solver tmp=UnionSolverBorne(B2,B1[i],E,B2->sortie);
		if (tmp==NULL) i++;
		else
		{
			if (!PlusInteressant_NoVarsNoMem(tmp,B2)) // cas où la solver n'est pas améliorée "gratuitement" (cas le plus fréquent)
			{
				freeSolver(tmp);
				i++;
			}
			else
			{
				Solver tmp2=B1[i];
				N1--;
				B1[i]=B1[N1];
				B1[N1]=tmp2;
				if (tmp->Nvar==B2->Nvar) freeSolver(tmp); // cas B1[i] contenue dans B2
				else // cas où l'on améliore "gratuitement" la solver B2
				{
					freeSolver(B2);
					B2=tmp;
					i=0;
				}
			}
		}
	}
	return B2;
}

#if 1

Solver DeleteVariable(int x, Solver B, SysEqLin E)
{
	if (B==NULL) return NULL;
	if (SearchElement(x,B->var,B->Nvar)==-1) return copySolver(B);
	if (B->type == 0) return NULL;
	Solver f2 = DeleteVariable(x,B->fils2,E);
	if (f2==NULL) return DeleteVariable(x,B->fils1,E);
	Solver f1;
	if (B->type==1) f1 = DeleteVariable(x,B->fils1,E);
	else
	{
		SysEqLin F = EsachantX(B->fils2->var,B->fils2->Nvar,E);
		f1 = DeleteVariable(x,B->fils1,F);
		freeSysEq(F);
	}
	if (f1 == NULL) return f2;
	if (B->type==2)
	{
		Solver result = NewSolver();
		result->var=DifferenceListesTriees(B->var,B->Nvar,&x,1,&result->Nvar);
		result->temps=max(f2->temps,max(0,f2->sortie)+f1->temps);
		result->memoire=max(f1->memoire,f2->memoire);
		result->sortie=max(0,f2->sortie)+max(0,f1->sortie);
		result->type=2;
		result->fils1 = f1;
		result->fils2 = f2;
		return result;
	}
	else
	{
		Solver result = UnionSolver(f1,f2,E);
		freeSolver(f1);
		freeSolver(f2);
		return result;
	}
}


Solver Refine(Solver B, SysEqLin E)
{
	if (B==NULL || B->type==0) return copySolver(B); // cas simple : rien à faire
	
	SysEqLin F = ExtraireEX2(B->var,B->Nvar,E);
	int * __restrict X = malloc(B->Nvar*sizeof(int));
	int n=0;
	int i;
	for (i = 0; i < B->Nvar; i++) if (ApparaitLineairement(B->var[i],F)) X[n++]=B->var[i];
	if (n)
	{
		Solver result = copySolver(B);
		for (i = 0; i < n; i++)
		{
			Solver tmp = DeleteVariable(X[i],result,F);
			freeSolver(result);
			result = tmp;
		}
		
		{
			Solver tmp = Refine(result,F);
			freeSolver(result);
			result = tmp;
		}
		
		for (i = 0; i < n; i++)
		{
			Solver bs = BaseSolver(X[i],F);
			Solver tmp = UnionSolver(result,bs,F);
			freeSolver(result);
			freeSolver(bs);
			result = tmp;
		}
		free(X);
		freeSysEq(F);
		return result;
	}
	else
	{
		free(X);
		if (B->type==1)
		{
			if (B->fils1->Nvar + B->fils2->Nvar != B->Nvar)
			{
				X=IntersectionListesTriees(B->fils1->var,B->fils1->Nvar,B->fils2->var,B->fils2->Nvar,&n);
				Solver f1 = copySolver(B->fils1);
				Solver f2 = copySolver(B->fils2);
				SysEqLin F1 = ExtraireEX2(f1->var,f1->Nvar,F);
				SysEqLin F2 = ExtraireEX2(f2->var,f2->Nvar,F);
				for (i = 0; i < n; i++)
				{
					int l1 = ApparaitLineairement(X[i],F1);
					int l2 = ApparaitLineairement(X[i],F2);
					if (l1 || l2)
					{
						if (l1>l2)
						{
							Solver tmp = DeleteVariable(X[i],f1,F1);
							freeSolver(f1);
							f1 = tmp;
						}
						else
						{
							Solver tmp = DeleteVariable(X[i],f2,F2);
							freeSolver(f2);
							f2 = tmp;
						}
						break;
					}
				}
				free(X);
				freeSysEq(F1);
				freeSysEq(F2);
				if (i<n)
				{
					Solver tmp = UnionSolver(f1,f2,F);
					freeSolver(f1);
					freeSolver(f2);
					Solver result = Refine(tmp,F);
					freeSolver(tmp);
					freeSysEq(F);
					return result;
				}
				else
				{
					freeSolver(f1);
					freeSolver(f2);
				}		
			}
			Solver f1 = Refine(B->fils1,F);
			Solver f2 = Refine(B->fils2,F);
			Solver result = UnionSolver(f1,f2,F);
			freeSolver(f1);
			freeSolver(f2);
			freeSysEq(F);
			return result;
		}
		else
		{
			Solver f2 = Refine(B->fils2,F);
			SysEqLin G = EsachantX(f2->var,f2->Nvar,F);
			Solver f1 = Refine(B->fils1,G);
			freeSysEq(G);
			freeSysEq(F);
			Solver result = NewSolver();
			result->var=UnionListesTriees(f1->var,f1->Nvar,f2->var,f2->Nvar,&result->Nvar);
			result->temps=max(f2->temps,max(0,f2->sortie)+f1->temps);
			result->memoire=max(f1->memoire,f2->memoire);
			result->sortie=max(0,f1->sortie)+f2->sortie;
			result->type=2;
			result->fils1 = f1;
			result->fils2 = f2;
			return result;
		}
	}
	
		
}
#endif
	


Solver OptimizeSolver(Solver B, SysEqLin E) 
{
	/*
	 *  Fonction censée mettre en forme la solver avant implémentation.
	 * 	Plusieurs objectifs :
	 * 		- ne doit pas être trop longue.
	 * 		- avoir un maximum d'unions avec des solvers de type 0 (ie 1 variable)
	 * 		- essayer de supprimer les variables en commun
	 */
	  
	if (B==NULL || B->type==0) return copySolver(B); // cas simple : rien à faire
	
	if (B->type==2) // cas union sequentielle, on met en forme les deux parties.
	{
		SysEqLin F=EsachantX(B->fils2->var,B->fils2->Nvar,E);
		Solver B1=OptimizeSolver(B->fils1,F);
		freeSysEq(F);
		Solver B2=OptimizeSolver(B->fils2,E);
		if (B1==NULL) return B2;
		Solver tmp=NewSolver();
		tmp->var=UnionListesTriees(B1->var,B1->Nvar,B2->var,B2->Nvar,&(tmp->Nvar));
		tmp->sortie=B->sortie;
		tmp->temps=max(B2->temps,max(B2->sortie,0)+B1->temps);
		tmp->memoire=max(B1->memoire,B2->memoire);
		tmp->type=2;
		tmp->fils1=B1;
		tmp->fils2=B2;		
		return tmp;
	}
	
	if (B->fils2->sortie<=0) // la solver 2 a moins d'une solution -> passage au sequentiel
	{
		SysEqLin F=EsachantX(B->fils2->var,B->fils2->Nvar,E);
		Solver B2=AjusteSolver(B->fils1,F); // On met à jour la solver car maintenant les variables de la solver 2 sont connues
		Solver B1=OptimizeSolver(B2,F);
		freeSolver(B2);
		freeSysEq(F);
		B2=OptimizeSolver(B->fils2,E);
		if (B1==NULL) return B2;
		Solver tmp=NewSolver();
		tmp->var=UnionListesTriees(B1->var,B1->Nvar,B2->var,B2->Nvar,&(tmp->Nvar));
		tmp->sortie=B->sortie;
		tmp->temps=max(B2->temps,B1->temps);
		tmp->memoire=max(B1->memoire,B2->memoire);
		tmp->type=2;
		tmp->fils1=B1;
		tmp->fils2=B2;
		return tmp;
	}
	
	if (B->fils1->sortie<=0) // pareil que ci-dessus
	{
		SysEqLin F=EsachantX(B->fils1->var,B->fils1->Nvar,E);
		Solver B2=AjusteSolver(B->fils2,F);
		Solver B1=OptimizeSolver(B2,F);
		freeSolver(B2);
		freeSysEq(F);
		B2=OptimizeSolver(B->fils1,E);
		if (B1==NULL) return B2;
		Solver tmp=NewSolver();
		tmp->var=UnionListesTriees(B1->var,B1->Nvar,B2->var,B2->Nvar,&(tmp->Nvar));
		tmp->sortie=B->sortie;
		tmp->temps=max(B2->temps,B1->temps);
		tmp->memoire=max(B1->memoire,B2->memoire);
		tmp->type=2;
		//tmp->parents=1;
		tmp->fils1=B1;
		tmp->fils2=B2;
		if (tmp->Nvar!=B->Nvar)
		{
			printf("Gros bug 3");
			getchar();
		}
		return tmp;
	}
	
	if (B->sortie==B->fils1->sortie + B->fils2->sortie) // les deux solvers sont indépendantes -> passage au séquentiel
	{
		/* Les deux temps possibles */
		int t1=max(max(B->fils1->sortie,0) + B->fils2->temps, B->fils1->temps);
		int t2=max(max(B->fils2->sortie,0) + B->fils1->temps, B->fils2->temps);
		if (min(t1,t2)<=B->temps)
		{
			Solver tmp=NewSolver();
			tmp->var=UnionListesTriees(B->fils1->var,B->fils1->Nvar,B->fils2->var,B->fils2->Nvar,&(tmp->Nvar));
			tmp->sortie=B->sortie;
			tmp->type=2;
			//tmp->parents=1;
			Solver B1,B2;
			/* On choisit la meilleur option */
			if (t1<t2 || (t1==t2 && B->fils1->Nvar>=B->fils2->Nvar))
			{
				tmp->temps=t1;
				SysEqLin F=EsachantX(B->fils1->var,B->fils1->Nvar,E);
				B2=AjusteSolver(B->fils2,F);
				B1=OptimizeSolver(B2,F);
				freeSolver(B2);
				freeSysEq(F);
				B2=OptimizeSolver(B->fils1,E);
			}
			else
			{
				tmp->temps=t2;
				SysEqLin F=EsachantX(B->fils2->var,B->fils2->Nvar,E);
				B2=AjusteSolver(B->fils1,F);
				B1=OptimizeSolver(B2,F);
				freeSolver(B2);
				freeSysEq(F);
				B2=OptimizeSolver(B->fils2,E);
			}
			tmp->memoire=max(B1->memoire,B2->memoire);
			tmp->fils1=B1;
			tmp->fils2=B2;
			if (tmp->Nvar!=B->Nvar)
			{
				printf("Gros bug 4");
				getchar();
			}
			return tmp;
		}
	}

	/* Cas idéal */
	if (B->fils2->Nvar==1) 
	{
		Solver B1=OptimizeSolver(B->fils1,E);
		Solver B2=UnionSolver(B1,B->fils2,E);
		freeSolver(B1);
		return B2;
	}
	if (B->fils1->Nvar==1)
	{
		Solver B1=OptimizeSolver(B->fils2,E);
		Solver B2=UnionSolver(B1,B->fils1,E);
		freeSolver(B1);
		return B2;
	}

	/* Partie d'amélioration */

	// On est donc dans le cas d'une union de type 1, chacune des solvers à au moins 2 variables
	// On garde le fils1 pour l'énumération et on va chercher à diviser fils2

	int N1;	
	int *X1=DifferenceListesTriees(B->fils2->var,B->fils2->Nvar,B->fils1->var,B->fils1->Nvar,&N1);
	Solver *L1=malloc((N1+1)*sizeof(Solver));
	int i;
	for (i = 0; i < N1; i++) L1[i]=BaseSolver(X1[i],E);
	
	if (B->fils1->Nvar>=B->fils2->Nvar && N1<B->fils2->Nvar)
	{
		L1[N1]=copySolver(B->fils1);
		int N2;
		Solver *L2=ExhaustiveSearch(L1,N1+1,B->temps,B->memoire,&N2,E);
		Solver tmp=NULL;
		for (i = 0; i < N2; i++)
		{
			if (B->Nvar==L2[i]->Nvar) tmp=L2[i];
			else freeSolver(L2[i]);
		}
		free(L2);
		if (tmp!=NULL)
		{
			for (i = 0; i <= N1; i++) freeSolver(L1[i]);
			free(L1);
			free(X1);
			Solver B3=OptimizeSolver(tmp,E);
			freeSolver(tmp);
			return B3;
		}
	}
	
	Solver B1 = PropagateSolver2(B->fils1,E,L1,N1);
	free(X1);
	for (i = 0; i < N1; i++) freeSolver(L1[i]);
	free(L1);
	Solver B2 = copySolver(B->fils2);


	while (B2->type==1 && ContenuListesTriees(B2->fils2->var,B2->fils2->Nvar,B1->var,B1->Nvar))
	{
		Solver B3 = copySolver(B2->fils1);
		freeSolver(B2);
		B2 = B3;
	}
	while (B2->type==1 && ContenuListesTriees(B2->fils1->var,B2->fils1->Nvar,B1->var,B1->Nvar))
	{
		Solver B3 = copySolver(B2->fils2);
		freeSolver(B2);
		B2 = B3;
	}
	
	/* Partie qui foire, la fonction peut boucler indéfiniment */ 
	if (B2->type==1)
	{
		Solver B3 = UnionSolver(B1,B2->fils1,E);
		if (B3->temps<=B->temps)
		{
			Solver B4 = UnionSolver(B3,B2->fils2,E);
			freeSolver(B3);
			freeSolver(B2);
			freeSolver(B1);
			B1 = OptimizeSolver(B4,E);
			freeSolver(B4);
			return B1;
		}
		freeSolver(B3);
		B3 = UnionSolver(B1,B2->fils2,E);
		if (B3->temps<=B->temps)
		{
			Solver B4 = UnionSolver(B3,B2->fils1,E);
			freeSolver(B3);
			freeSolver(B2);
			freeSolver(B1);
			B1 = OptimizeSolver(B4,E);
			freeSolver(B4);
			return B1;
		}
		freeSolver(B3);			
	}
	if (B2->type==2)
	{
		X1=DifferenceListesTriees(B2->var,B2->Nvar,B1->var,B1->Nvar,&N1);
		L1=malloc((N1+1)*sizeof(Solver));
		for (i = 0; i < N1; i++) L1[i]=BaseSolver(X1[i],E);
		L1[N1]=copySolver(B1);
		int N2;
		Solver *L2=ExhaustiveSearch(L1,N1+1,B->temps,B2->sortie-1,&N2,E);
		Solver tmp=NULL;
		for (i = 0; i < N2; i++)
		{
			if (B->Nvar==L2[i]->Nvar) tmp=L2[i];
			else freeSolver(L2[i]);
		}
		free(L2);
		for (i = 0; i <= N1; i++) freeSolver(L1[i]);
		free(L1);
		free(X1);
		if (tmp!=NULL)
		{
			freeSolver(B1);
			freeSolver(B2);
			Solver B3=OptimizeSolver(tmp,E);
			freeSolver(tmp);
			return B3;
		}
	}

	/* Rien n'a marché */
	
	Solver B3 = OptimizeSolver(B1,E);
	freeSolver(B1);
	B1 = OptimizeSolver(B2,E);
	freeSolver(B2);
	B2 = UnionSolver(B3,B1,E);
	freeSolver(B3);
	freeSolver(B1);
	return B2;
}

Solver AjusteSolver(Solver B, SysEqLin E) // Refait la solver B par rapport à E (supprime les variables connues, ...)
{
	if (B==NULL) return NULL;
	
	if (B->type==2)
	{
		Solver B2=AjusteSolver(B->fils2,E);
		if (B2!=NULL && B2->Nvar>0)
		{
			SysEqLin F=EsachantX(B2->var,B2->Nvar,E);
			Solver B1=AjusteSolver(B->fils1,F);
			freeSysEq(F);
			if (B1!=NULL && B1->Nvar>0)
			{
				Solver tmp=NewSolver();
				tmp->var=UnionListesTriees(B1->var,B1->Nvar,B2->var,B2->Nvar,&(tmp->Nvar));
				tmp->sortie=B1->sortie+max(B2->sortie,0);
				tmp->temps=max(B2->temps,max(B2->sortie,0)+B1->temps);
				tmp->memoire=max(B1->memoire,B2->memoire);
				tmp->type=2;
				//tmp->parents=1;
				tmp->fils1=B1;
				tmp->fils2=B2;
				return tmp;
			}
			else
			{
				freeSolver(B1);
				return B2;
			}
		}
		else
		{
			freeSolver(B2);
			return AjusteSolver(B->fils1,E);
		}
	}
	
	if (B->type==1)
	{
		Solver B1=AjusteSolver(B->fils1,E);
		Solver B2=AjusteSolver(B->fils2,E);
		Solver tmp=UnionSolver(B1,B2,E);
		freeSolver(B1);
		freeSolver(B2);
		return tmp;
	}
	
	Solver tmp=NewSolver();
	tmp->Nvar=0;
	tmp->var=malloc(B->Nvar*sizeof(int));
	int i;
	for (i = 0; i < B->Nvar; i++) if (E->lignevar[B->var[i]]*E->gen<E->a) tmp->var[tmp->Nvar++]=B->var[i];
	if (tmp->Nvar==0) 
	{
		/* On libère la solver ( ATTENTION !!) */
		free(tmp->var);
		tmp->parents=0;
		return NULL;
	}
	#if 0
	tmp->initialisation=E;
	tmp->mat=InfoSortie_VariablesVersion(tmp->var,tmp->Nvar,E);
	tmp->sortie=tmp->mat->nombre_de_lignes - (E->gen-1)*tmp->Nvar;
	#else
	B->sortie=TailleE(B->var,B->Nvar,E);
	#endif
	if (tmp->Nvar<=B->temps)
	{
		tmp->temps=tmp->Nvar;
		tmp->memoire=0;
	}
	else
	{
		tmp->temps=B->temps;
		tmp->memoire=B->memoire;
	}
	tmp->type=0;
	//tmp->parents=1;
	tmp->fils1=NULL;
	tmp->fils2=NULL;
	return tmp;
}	


/* **************************************************************************** */
/* 																				*/
/*                   Fonctions diverses sur les solvers                          */
/* 																				*/
/* **************************************************************************** */
	

Equation * ExtraireEquationUnion(Solver B, SysEqLin E, int *taille) // Renvoi les equations de E(X1 u X2) - (E(X1) + E(X2)).
{
	// Attention : les équation sont de la forme Eq[i] = Eq[i + *taille], Eq[i] est en les variables X2

	/* A améliorer, elle est beaucoup trop lente et c'est la fonction la plus utilisée dans l'écriture du code C. */

	
	
	const int a=E->gen*VARIABLES;

	__restrict SysEqLin F = ExtraireEX2(B->var,B->Nvar,E);

	unsigned char ** __restrict m1=malloc(a*sizeof(unsigned char *));
	unsigned char ** __restrict m2=malloc(a*sizeof(unsigned char *));
	unsigned char ** __restrict m3=malloc(a*sizeof(unsigned char *));
	int i;
	for (i = 0; i < a; i++)
	{
		m1[i]=malloc(F->b*sizeof(unsigned char));
		m2[i]=malloc(F->b*sizeof(unsigned char));
		m3[i]=malloc(F->b*sizeof(unsigned char));
	}

	int b1=ExtraireEX(m1,B->fils1->var,B->fils1->Nvar,F);
	int b2=ExtraireEX(m2,B->fils2->var,B->fils2->Nvar,F);	
	
	int b3=Somme(m3,m1,b1,m2,a,b2);
	//b1=ExtraireEX(m1,B->var,B->Nvar,E);
	
	b2=Supplementaire(m2,m3,b3,F->mat,a,F->b);
	for (i = 0; i < a; i++)
	{
		free(m1[i]);
		free(m3[i]);
	}
	free(m1);
	free(m3);
	__restrict SysEqLin G=malloc(sizeof(SYSEQLIN));
	G->mat=m2;
	G->gen=F->gen;
	G->a=F->a;
	G->b=b2;
	//->lignevar=malloc((VARIABLES)*sizeof(int));
	//for (i = 0; i < VARIABLES; i++) F->lignevar[i]=E->lignevar[i];
	G->lignevar=F->lignevar;
	Equation * __restrict Eq=ConversionSys2(B->fils2->var,B->fils2->Nvar,G);
	//free(F->lignevar);
	for (i = 0; i < a; i++) free(m2[i]);
	free(m2);
	freeSysEq(F);
	free(G);
	*taille=b2;
	return Eq;
}

int EnleveFaussesEq(Solver *B1, int N1, SysEqLin E) // Prend en compte le fait que x = y soit equivalent à S(x) = S(y)
{
	/*
	 * Doit être lancé avec les solvers 1 issues de MakeSolver1.
     * Cela permet de savoir si oui ou non il y a une equation entre 2 variables.
     * Il ne reste donc plus qu'à déterminer si cette équation est x = y ou équivalent.
     * Si c'est le cas, on remplace toutes les occurences de y par x, sauf pour cette équation.
     */
	int trouve = 0; // indique si l'on trouve une équation d'égalité entre 2 variables
	
	int a=E->gen*VARIABLES; // nombre de lignes de E->mat
	
	unsigned char **m=malloc(a*sizeof(unsigned char *));

	int i;
	for (i = 0; i < a; i++)
	{
		m[i]=malloc((E->b+E->gen)*sizeof(unsigned char));
		int j;
		for (j = 0; j < E->b; j++) m[i][j]=E->mat[i][j];
		for (j = 0; j < E->gen; j++) m[i][j+E->b]=0;
	}
	
	int c=Dimension(m,a,E->b,1);
	
	int *X=malloc(VARIABLES*sizeof(int));
	
	for (i = 0; i < N1; i++)
	{
		int pos=0,j;
		/* Si une variable apparait "linéairement" c'est inutile */
		for (j = 0; j < B1[i]->Nvar; j++) if (!ApparaitLineairement(B1[i]->var[j],E)) X[pos++]=B1[i]->var[j];
		for (j = 0; j < pos; j++)
		{
			int k;
			for (k = j+1; k < pos; k++)
			{
				#if 1
				int l;
				for (l = 0; l < E->gen; l++)
				{
					m[E->gen*E->lignevar[X[j]]+l][E->b]=1;
					m[E->gen*E->lignevar[X[k]]+l][E->b]=1;
					int d=Dimension(m,a,E->b+1,1);
					m[E->gen*E->lignevar[X[j]]+l][E->b]=0;
					m[E->gen*E->lignevar[X[k]]+l][E->b]=0;
					if (d<c+1) break;				
				}
				if (l<E->gen)
				{
					int b=l;
					for (l = 0; l < E->gen; l++)
					{
						if (l!=b) // Permet de garder l'équation x = y ou S(x) = S(y)
						{
							int p;
							for (p = 0; p < E->b; p++)
							{
								if (m[E->gen*E->lignevar[X[k]]+l][p])
								{
									m[E->gen*E->lignevar[X[j]]+l][p]^=m[E->gen*E->lignevar[X[k]]+l][p];
									m[E->gen*E->lignevar[X[k]]+l][p]=0;
								}
							}
						}
					}
					trouve=1;
					c=Dimension(m,a,E->b,1);
					pos--;
					for (l = k; l < pos; l++) X[l]=X[l+1];
					k--;
				}
				#else // Ce serait mieux : on regarde si il exite une fonction f tel que f(x) = f(y), il faudrait vérifier si f est bijective.
				int l;
				for (l = 0; l < E->gen; l++)
				{
					m[E->gen*E->lignevar[X[j]]+l][E->b+l]=1;
					m[E->gen*E->lignevar[X[k]]+l][E->b+l]=1;			
				}
				int d=Dimension(m,a,E->b+E->gen,1);
				for (l = 0; l < E->gen; l++)
				{
					m[E->gen*E->lignevar[X[j]]+l][E->b+l]=0;
					m[E->gen*E->lignevar[X[k]]+l][E->b+l]=0;			
				}
				if (d<c+E->gen)
				{
					for (l = 0; l < E->gen; l++)
					{
						int p;
						for (p = 0; p < E->b; p++)
						{
							if (m[E->gen*E->lignevar[X[k]]+l][p])
							{
								m[E->gen*E->lignevar[X[j]]+l][p]^=m[E->gen*E->lignevar[X[k]]+l][p];
								m[E->gen*E->lignevar[X[k]]+l][p]=0;
							}
						}
					}
					trouve=1;
					c=Dimension(m,a,E->b,1);
					pos--;
					for (l = k; l < pos; l++) X[l]=X[l+1];
					k--;
				}
				#endif
				
			}
		}
	}
	free(X);
	
	int j=Base(E->mat,m,a,E->b);
	
	for (i = 0; i < a; i++) free(m[i]);
	free(m);

	if (!trouve && j==E->b) return 0; // Il n'y avait pas d'équations du type x = y ou S(x) = S(y)
	
	E->b=j;
	
	if (E->initialise)
	{
		for (i = 0; i < E->a/E->gen; i++)
		{
			freeMatriceCreuse(E->sousmat[E->varligne[i]]);
		}
		free(E->sousmat);
		E->initialise=0;
	}

	#if 0 // Activer si l'on supprime des variables
	int *dedans=malloc(VARIABLES*sizeof(int));
	for (i = 0; i < VARIABLES; i++)
	{
		for (j = 0; j < E->gen; j++)
		{
			int k=0;
			while (k<E->b && E->mat[E->lignevar[i]*E->gen+j][k]==0) k++;
			if (k<E->b) break;
		}
		if (j<E->gen) dedans[i]=1;
		else dedans[i]=0;
	}
	for (i = 0; i < VARIABLES; i++)
	{
		if (dedans[i]==0 && E->gen*E->lignevar[i]<E->a)
		{
			E->a=E->a-E->gen;
			int l;
			for (l = 0; l < E->gen; l++) echangeligne(E->gen*E->lignevar[i]+l,E->a+l,E->mat,E->gen*VARIABLES,E->b);
			j=E->varligne[E->a/E->gen];
			int k=E->lignevar[i];
			E->lignevar[i]=E->lignevar[j];
			E->lignevar[j]=k;
			E->varligne[E->a/E->gen]=i;
			E->varligne[k]=j;
		}
	}
	free(dedans);
	#endif
	return 1;
}


 
