#include <stdio.h>
#include <stdlib.h>
#include "Bazar.h"
#include "CorpsK.h"
#include "Algebre.h"
#include "SysEqLin.h"

#define DISPLAY_EQUATIONS 0

extern const int VARIABLES;

#define TAILLE_BLOC_POUR_ALLOCATION_Espace 10000

/* **************************************************************************** */
/* 																				*/
/*                       Gestion mémoire Ligne Creuse                           */
/* 																				*/
/* **************************************************************************** */

int Nombre_de_LigneCreuse_disponible_pour_allocation = 0, Taille_Espace_LigneCreuse_pour_allocation = 0;
LigneCreuse * __restrict Espace_LigneCreuse_pour_allocation = NULL;

LigneCreuse NouvelleLigneCreuse(void) // Renvoi l'adresse d'une SOLVER disponible
{
	static int Initialiser_Espace = 1;
		
	if (Nombre_de_LigneCreuse_disponible_pour_allocation) return Espace_LigneCreuse_pour_allocation[--Nombre_de_LigneCreuse_disponible_pour_allocation];
	else
	{
		if (Initialiser_Espace)
		{
			Initialiser_Espace = 0;
			Espace_LigneCreuse_pour_allocation = malloc(TAILLE_BLOC_POUR_ALLOCATION_Espace*sizeof(LigneCreuse));
			Taille_Espace_LigneCreuse_pour_allocation = TAILLE_BLOC_POUR_ALLOCATION_Espace;
		}
		LIGNECREUSE * __restrict tmp = malloc(TAILLE_BLOC_POUR_ALLOCATION_Espace*sizeof(LIGNECREUSE));
		int i;
		for (i = 0; i < TAILLE_BLOC_POUR_ALLOCATION_Espace; i++)
		{
			Espace_LigneCreuse_pour_allocation[i]=tmp++;
			Espace_LigneCreuse_pour_allocation[i]->parents=1;
		}
		Nombre_de_LigneCreuse_disponible_pour_allocation = TAILLE_BLOC_POUR_ALLOCATION_Espace;
		return Espace_LigneCreuse_pour_allocation[--Nombre_de_LigneCreuse_disponible_pour_allocation];
	}
}
	
LigneCreuse copyLigneCreuse(LigneCreuse a) // copie une MatriceCreuse "pour de faux" (NE SURTOUT PAS MODIDIFIER UNE LigneCreuse DIRECTEMENT)
{
	if (a!=NULL) a->parents=a->parents+1; // mise à jour du nombre de pointeurs vers cette LigneCreuse
	return a;
}

void freeLigneCreuse(LigneCreuse a) // libère la mémoire associée à une LigneCreuse
{
	if (a!=NULL)
	{
		(a->parents)--;
		if (!a->parents) // elle n'est plus utilisée
		{
			free(a->elements);
			a->parents=1;
			if (Taille_Espace_LigneCreuse_pour_allocation == Nombre_de_LigneCreuse_disponible_pour_allocation)
			{
				Taille_Espace_LigneCreuse_pour_allocation+= TAILLE_BLOC_POUR_ALLOCATION_Espace;
				Espace_LigneCreuse_pour_allocation = realloc(Espace_LigneCreuse_pour_allocation, Taille_Espace_LigneCreuse_pour_allocation*sizeof(LigneCreuse));
			}
			Espace_LigneCreuse_pour_allocation[Nombre_de_LigneCreuse_disponible_pour_allocation++]=a;
		}
	}
}

/* **************************************************************************** */
/* 																				*/
/*                       Gestion mémoire Matrice Creuse                         */
/* 																				*/
/* **************************************************************************** */

int Nombre_de_MatriceCreuse_disponible_pour_allocation = 0, Taille_Espace_MatriceCreuse_pour_allocation = 0;
MatriceCreuse * __restrict Espace_MatriceCreuse_pour_allocation = NULL;

MatriceCreuse NouvelleMatriceCreuse(void) // Renvoi l'adresse d'une SOLVER disponible
{
	static int Initialiser_Espace = 1;
		
	if (Nombre_de_MatriceCreuse_disponible_pour_allocation) return Espace_MatriceCreuse_pour_allocation[--Nombre_de_MatriceCreuse_disponible_pour_allocation];
	else
	{
		if (Initialiser_Espace)
		{
			Initialiser_Espace = 0;
			Espace_MatriceCreuse_pour_allocation = malloc(TAILLE_BLOC_POUR_ALLOCATION_Espace*sizeof(MatriceCreuse));
			Taille_Espace_MatriceCreuse_pour_allocation = TAILLE_BLOC_POUR_ALLOCATION_Espace;
		}
		MATRICECREUSE * __restrict tmp = malloc(TAILLE_BLOC_POUR_ALLOCATION_Espace*sizeof(MATRICECREUSE));
		int i;
		for (i = 0; i < TAILLE_BLOC_POUR_ALLOCATION_Espace; i++)
		{
			Espace_MatriceCreuse_pour_allocation[i]=tmp++;
			Espace_MatriceCreuse_pour_allocation[i]->parents=1;
		}
		Nombre_de_MatriceCreuse_disponible_pour_allocation = TAILLE_BLOC_POUR_ALLOCATION_Espace;
		return Espace_MatriceCreuse_pour_allocation[--Nombre_de_MatriceCreuse_disponible_pour_allocation];
	}
}
	
MatriceCreuse copyMatriceCreuse(MatriceCreuse a) // copie une MatriceCreuse "pour de faux" (NE SURTOUT PAS MODIDIFIER UNE MatriceCreuse DIRECTEMENT)
{
	if (a!=NULL) a->parents=a->parents+1; // mise à jour du nombre de pointeurs vers cette MatriceCreuse
	return a;
}

void freeMatriceCreuse(MatriceCreuse a) // libère la mémoire associée à une MatriceCreuse
{
	if (a!=NULL)
	{
		(a->parents)--;
		if (!a->parents) // elle n'est plus utilisée
		{
			int i;
			for (i = 0; i < a->nombre_de_lignes; i++) freeLigneCreuse(a->ligne[i]);
			free(a->ligne);
			a->parents=1;
			if (Taille_Espace_MatriceCreuse_pour_allocation == Nombre_de_MatriceCreuse_disponible_pour_allocation)
			{
				Taille_Espace_MatriceCreuse_pour_allocation+= TAILLE_BLOC_POUR_ALLOCATION_Espace;
				Espace_MatriceCreuse_pour_allocation = realloc(Espace_MatriceCreuse_pour_allocation, Taille_Espace_MatriceCreuse_pour_allocation*sizeof(MatriceCreuse));
			}
			Espace_MatriceCreuse_pour_allocation[Nombre_de_MatriceCreuse_disponible_pour_allocation++]=a;
		}
	}
}

void InitialiseSousMat_tmp(SysEqLin E) // initialise E->sousmat
{

	/*	
	 *	 | * * * * | *          |		 | * * * * | * * * * * * |
	 * 	 | * * * * |   *        |		 |   * * * | * * * * * * |
	 *	 | * * * * |     *      | 		 |     * * | * * * * * * | 
	 *	 | * * * * |       *    |  - >   |       * | * * * * * * |
	 *	 | * * * * |         *  |	     |         | *   * * * * | | -> Sousmat
	 *	 | * * * * |           *|        |         |   * * * * * | | ->
	 * 												 ___.___.___	
	 *									variable:     0   1   2		
	 */

	/* Initialisation Big Matrice */
	const int nombre_ligne_big_matrix = E->a;
	const int nombre_colone_big_matrix = E->a+E->b;
	unsigned char ** __restrict big_matrix=malloc(nombre_ligne_big_matrix*sizeof(unsigned char *));
	unsigned char * __restrict espace_big_matrix=malloc(nombre_colone_big_matrix*nombre_ligne_big_matrix*sizeof(unsigned char));

	int i;
	for (i = 0; i < nombre_ligne_big_matrix; i++) big_matrix[i]=espace_big_matrix + i*nombre_colone_big_matrix;
	for (i = 0; i < nombre_ligne_big_matrix; i++)
	{
		int j;
		for (j = 0; j < E->b; j++) big_matrix[i][j]=E->mat[i][j];
		for (j = E->b; j < nombre_colone_big_matrix; j++) big_matrix[i][j]=0;
	}
	const int nombre_de_variables = E->a/E->gen;
	for (i = 0; i < nombre_de_variables; i++)
	{
		int j;
		for (j = 0; j < E->gen; j++) big_matrix[E->gen*i+j][E->b+E->gen*i+j]=1;
	}
	
	//printf(" Big matrice \n");
	//PrintMat(big_matrix,nombre_ligne_big_matrix,nombre_colone_big_matrix);
	//getchar();

	/* Pivot sur le block gauche de taille E->a x E->b  (E->mat) */
	int dim = 0;
	const int colonne_max_pour_pivot = E->b;
	int pivot_c;
	for (pivot_c = 0; pivot_c < colonne_max_pour_pivot; pivot_c++)
	{
		int new_pivot_l;
		for (new_pivot_l = dim; new_pivot_l < nombre_ligne_big_matrix; new_pivot_l++)
		{
			if (big_matrix[new_pivot_l][pivot_c])
			{
				if (new_pivot_l!=dim)
				{
					unsigned char *tmp = big_matrix[dim];
					big_matrix[dim]=big_matrix[new_pivot_l];
					big_matrix[new_pivot_l]=tmp;
				}
				int ligne;
				for (ligne = new_pivot_l+1; ligne < nombre_ligne_big_matrix; ligne++)
				{
					if (big_matrix[ligne][pivot_c])
					{
						int colonne;
						for (colonne = pivot_c+1; colonne < nombre_colone_big_matrix; colonne++)
						{
							big_matrix[ligne][colonne]=Multiply(big_matrix[dim][pivot_c],big_matrix[ligne][colonne]) ^ Multiply(big_matrix[ligne][pivot_c],big_matrix[dim][colonne]);
						}
						//big_matrix[ligne][pivot_c]=0;
					}
				}
				dim++;
				break;
			}
		}
	}
	//printf(" Big matrice - après premier pivot\n");
	//PrintMat(big_matrix,nombre_ligne_big_matrix,nombre_colone_big_matrix);
	//getchar();
	
	/* Partie sur block en bas à droite de taille (E->a - dim) x E->a */
	const int dim_save = dim;
	for (pivot_c = colonne_max_pour_pivot; pivot_c < nombre_colone_big_matrix; pivot_c++)
	{
		int new_pivot_l;
		for (new_pivot_l = dim; new_pivot_l < nombre_ligne_big_matrix; new_pivot_l++)
		{
			if (big_matrix[new_pivot_l][pivot_c])
			{
				if (new_pivot_l!=dim)
				{
					unsigned char *tmp = big_matrix[dim];
					big_matrix[dim]=big_matrix[new_pivot_l];
					big_matrix[new_pivot_l]=tmp;
				}
				const unsigned char inv = Inverse(big_matrix[dim][pivot_c]);
				int colonne;
				for (colonne = pivot_c; colonne < nombre_colone_big_matrix; colonne++) big_matrix[dim][colonne]=Multiply(inv,big_matrix[dim][colonne]);
				int ligne;
				for (ligne = dim_save; ligne < dim; ligne++)
				{
					if (big_matrix[ligne][pivot_c])
					{
						for (colonne = pivot_c+1; colonne < nombre_colone_big_matrix; colonne++)
						{
							big_matrix[ligne][colonne]^=Multiply(big_matrix[ligne][pivot_c],big_matrix[dim][colonne]);
						}
						big_matrix[ligne][pivot_c]=0;
					}
				}
				for (ligne = new_pivot_l+1; ligne < nombre_ligne_big_matrix; ligne++)
				{
					if (big_matrix[ligne][pivot_c])
					{
						for (colonne = pivot_c+1; colonne < nombre_colone_big_matrix; colonne++)
						{
							big_matrix[ligne][colonne]^=Multiply(big_matrix[ligne][pivot_c],big_matrix[dim][colonne]);
						}
						big_matrix[ligne][pivot_c]=0;
					}
				}
				dim++;
				break;
			}
		}
	}
	//printf(" Big matrice - après second pivot\n");
	//PrintMat(big_matrix,nombre_ligne_big_matrix,nombre_colone_big_matrix);
	//getchar();

	#if 0
	/* On remplit les sous-matrices */
	E->sousmat = malloc(VARIABLES*sizeof(MatriceCreuse));
	for (i = 0; i < nombre_de_variables; i++)
	{
		const int var = E->varligne[i];
		E->sousmat[var] = NouvelleMatriceCreuse();
		/* on compte le nombre d'éléments non nuls */
		E->sousmat[var]->nnz = 0;
		int ligne;
		for (ligne = dim_save; ligne < dim; ligne++)
		{
			int colonne;
			for (colonne = 0; colonne < E->gen ; colonne++) if (big_matrix[ligne][colonne_max_pour_pivot+E->gen*i+colonne]) E->sousmat[var]->nnz++;
		}
		E->sousmat[var]->IA=malloc((E->gen+1)*sizeof(int));
		E->sousmat[var]->A=malloc(E->sousmat[var]->nnz*sizeof(unsigned char));
		E->sousmat[var]->JA=malloc(E->sousmat[var]->nnz*sizeof(int));
		E->sousmat[var]->ligne=0;
		E->sousmat[var]->IA[0]=0;
		int nnz = 0;
		int colonne;
		for (colonne = 0; colonne < E->gen ; colonne++)
		{
			const int nnz_save = nnz;
			for (ligne = dim_save; ligne < dim; ligne++)
			{
				if (big_matrix[ligne][colonne_max_pour_pivot+E->gen*i+colonne])
				{
					E->sousmat[var]->A[nnz]=big_matrix[ligne][colonne_max_pour_pivot+E->gen*i+colonne];
					E->sousmat[var]->JA[nnz]=ligne-dim_save;
					nnz++;
				}
			}
			if (nnz!=nnz_save)
			{
				E->sousmat[var]->ligne++;
				E->sousmat[var]->IA[E->sousmat[var]->ligne] = nnz;
			}
		}
		//printf("Sous-mat %d : \n",i);
		//PrintMat2(E->sousmat[var]);
	}
	#else

	/* On remplit les sous-matrices */
	E->sousmat = malloc(VARIABLES*sizeof(MatriceCreuse));
	for (i = 0; i < nombre_de_variables; i++)
	{
		const int var = E->varligne[i];
		E->sousmat[var] = NouvelleMatriceCreuse();
		E->sousmat[var]->ligne=malloc(E->gen*sizeof(LigneCreuse));
		E->sousmat[var]->nombre_de_lignes = 0;
		int colonne;
		for (colonne = 0; colonne < E->gen ; colonne++)
		{
			int nnz=0;
			/* on compte le nombre d'éléments non nuls */
			int ligne;
			for (ligne = dim_save; ligne < dim; ligne++)
			{
				if (big_matrix[ligne][colonne_max_pour_pivot+E->gen*i+colonne]) nnz++;
			}
			if (nnz)
			{
				E->sousmat[var]->ligne[E->sousmat[var]->nombre_de_lignes]=NouvelleLigneCreuse();
				E->sousmat[var]->ligne[E->sousmat[var]->nombre_de_lignes]->elements=malloc(nnz*sizeof(COL_COEFF));
				nnz=0;
				for (ligne = dim_save; ligne < dim; ligne++)
				{
					if (big_matrix[ligne][colonne_max_pour_pivot+E->gen*i+colonne])
					{
						E->sousmat[var]->ligne[E->sousmat[var]->nombre_de_lignes]->elements[nnz].coeff=big_matrix[ligne][colonne_max_pour_pivot+E->gen*i+colonne];
						E->sousmat[var]->ligne[E->sousmat[var]->nombre_de_lignes]->elements[nnz].colonne=ligne-dim_save;
						nnz++;
					}
				}
				E->sousmat[var]->ligne[E->sousmat[var]->nombre_de_lignes]->nnz=nnz;
				E->sousmat[var]->nombre_de_lignes++;
			}
		}
	}
	#endif
	E->col_max = dim-dim_save;
	//printf("E->col_max = %d",E->col_max);
	//getchar();
	/* Liberation de la memoire */

	free(big_matrix);
	free(espace_big_matrix);
}

void InitialiseSousMat(SysEqLin E) // Initialise E->sousmat
{
	if (!E->initialise)
	{
		InitialiseSousMat_tmp(E);
		E->initialise=1;
	}
}

#if 1

void TriLignesCreusesByNNZ(LigneCreuse * __restrict tableau, const int longueur)
{
	const int gap[8]={1, 4, 10, 23, 57, 132, 301, 701};
	int i=2;
	while (i<8 && gap[i]<longueur) i++;
    while(i>0)
    {
         i--;
         int j;
         for (j=gap[i];j<longueur;j++)
         {
             LigneCreuse valeur=tableau[j];
             int k=j;
             while((k>(gap[i]-1)) && (tableau[k-gap[i]]->nnz>valeur->nnz))
             {
				 tableau[k]=tableau[k-gap[i]];
				 k=k-gap[i];
             }
             tableau[k]=valeur;
         }
	 }
}
	
int TailleE(int * __restrict const X, const int N, SysEqLin E) // Renvoi N - dim(E(X)),  N = |X|
{
	//return TailleE(X,N,E);
	/* Beaucoup de variables */
	static int AlloueN = 0;
	static LigneCreuse * __restrict L = NULL;
	static int * __restrict nombre_nnz_colonne = NULL;
	static int AlloueCOL = 0;
	static int AlloueNCOL = 0;
	static int AlloueLIGNE_mat = 0;
	static unsigned char ** __restrict m = NULL;
	static int * __restrict nouvelle_colonne = NULL;
	static int * __restrict colonne_changer = NULL;
	//static int compteur1 = 0, compteur2 = 0, compteur3 = 0, compteur4 = 0;

	
	if (!E->initialise) InitialiseSousMat(E);
		
	if (N*E->gen>AlloueN)
	{
		free(L);
		AlloueN=N*E->gen;
		//printf("AlloueN : %d\n",AlloueN);
		L=malloc(AlloueN*sizeof(LigneCreuse));
	}

	if (E->col_max>AlloueCOL)
	{
		nouvelle_colonne=realloc(nouvelle_colonne,E->col_max*sizeof(int));
		int i;
		for (i = AlloueCOL; i < E->col_max; i++) nouvelle_colonne[i]=-1;
		free(nombre_nnz_colonne);
		AlloueCOL=E->col_max;
		//printf("AlloueCOL : %d\n",AlloueCOL);
		nombre_nnz_colonne=calloc(AlloueCOL,sizeof(int));
	}

	int NombreDeLignes =  0;
	int taille=0,i;
	for (i = 0; i < N; i++)
	{
		if (E->lignevar[X[i]]*E->gen<E->a)
		{
			int j;
			for (j = 0; j < E->sousmat[X[i]]->nombre_de_lignes; j++) L[NombreDeLignes++]=E->sousmat[X[i]]->ligne[j];
			taille++;
		}
	}

	/* 
	 * On compte le nombre d'éléments non-nuls sur chaque colonne
	 */
	int NombreDeColonnes = 0;
	for (i = 0; i < NombreDeLignes; i++)
	{
		int j;
		for (j = 0; j < L[i]->nnz; j++)
		{
			if (!nombre_nnz_colonne[L[i]->elements[j].colonne]) NombreDeColonnes++;
			nombre_nnz_colonne[L[i]->elements[j].colonne]++;
		}
	}

	/*
	 * On va essayer de faire une partie du pivot sur la matrice creuse
	 * (c'est rentable de part la forme des sous-mat)
	 */
	int dim = (1-E->gen)*taille;
	//int cpt=NombreDeLignes;
	int ligne = 0;
	//while (cpt)
	while (ligne<NombreDeLignes)
	{
		//if (ligne==NombreDeLignes) ligne=0;
		int nombre_element_ligne = 0;
		int colonne_a_enlever;
		int j;
		for (j = 0; j < L[ligne]->nnz; j++)
		{
			if (nombre_nnz_colonne[L[ligne]->elements[j].colonne])
			{
				if (nombre_nnz_colonne[L[ligne]->elements[j].colonne]==1) // 1 seul élément non nul sur la colonne -> très bon pivot pour matrice creuse
				{
					int l;
					for (l = 0; l < L[ligne]->nnz; l++)
					{
						if (nombre_nnz_colonne[L[ligne]->elements[l].colonne])
						{
							if (nombre_nnz_colonne[L[ligne]->elements[l].colonne] == 1) NombreDeColonnes--;
							nombre_nnz_colonne[L[ligne]->elements[l].colonne]--; // On met à jour le nombre d'éléments dans les colonnes
						}
					}
					goto suppr_ligne;
				}
				else
				{
					nombre_element_ligne++;
					colonne_a_enlever = j;
				}
			}
		}
		if (nombre_element_ligne<=1)
		{
			if (nombre_element_ligne==1) // 1 seul élément sur la ligne
			{
				nombre_nnz_colonne[L[ligne]->elements[colonne_a_enlever].colonne] = 0; // on supprime la colonne
				NombreDeColonnes--;
				suppr_ligne: // on met à jour la dim et on supprime la ligne
				dim++;
				//cpt=NombreDeLignes;
			}
			NombreDeLignes--;
			L[ligne]=L[NombreDeLignes];
		}
		else ligne++;
		//cpt--;
	}
	
	if (NombreDeColonnes>AlloueNCOL)
	{
		AlloueNCOL=NombreDeColonnes;
		for (i = 0; i < AlloueLIGNE_mat; i++)
		{
			free(m[i]);
			m[i]=malloc(AlloueNCOL*sizeof(unsigned char));
		}
		free(colonne_changer);
		colonne_changer=malloc((AlloueNCOL+1)*sizeof(int));
	}
	if (NombreDeLignes>AlloueLIGNE_mat)
	{
		m = realloc(m,NombreDeLignes*sizeof(unsigned char *));
		for (i = AlloueLIGNE_mat; i < NombreDeLignes; i++) m[i] = malloc(AlloueNCOL*sizeof(unsigned char));
		AlloueLIGNE_mat = NombreDeLignes;
		//printf("AlloueLIGNE_mat : %d\n",AlloueLIGNE_mat);
	}
	
	int pos=0;
	int DerniereColonne = NombreDeColonnes-1;
	int Ncol = 0;
	for (i = 0; i < NombreDeLignes; i++)
	{
		int k;
		unsigned char * __restrict ligne_matrice = m[i];
		for (k = 0; k < NombreDeColonnes; k++) ligne_matrice[k]=0;
		int premier = 1;
		for (k = 0; k < L[i]->nnz; k++)
		{
			if (nombre_nnz_colonne[L[i]->elements[k].colonne])
			{
				nombre_nnz_colonne[L[i]->elements[k].colonne]--;
				colonne_changer[pos]=L[i]->elements[k].colonne;
				int colonne;
				if (nouvelle_colonne[colonne_changer[pos]]>=0) colonne=nouvelle_colonne[colonne_changer[pos]];
				else
				{
					if (!premier)
					{
						nouvelle_colonne[colonne_changer[pos++]]=DerniereColonne;
						colonne=DerniereColonne--;
					}
					else
					{
						nouvelle_colonne[colonne_changer[pos++]]=Ncol;
						colonne=Ncol++;
						premier=0;
					}
				}
				ligne_matrice[colonne]=L[i]->elements[k].coeff;								
			}
		}
	}
			
	for (i = 0; i < pos; i++) nouvelle_colonne[colonne_changer[i]]=-1; // on remet le tableau à zero pour la prochaine execution

	//compteur4++;
	//printf("cpt1 = %d, cpt2 = %d, cpt3 = %d, cpt4 = %d\n",compteur1,compteur2,compteur3,compteur4);
															
	return dim+Dimension_tmp(m,NombreDeLignes,NombreDeColonnes);
}

int TailleE_Borne(int * __restrict const X, const int N, SysEqLin E, const int borne) // Renvoi N - dim(E(X)),  N = |X|
{
	//return TailleE(X,N,E);
	/* Beaucoup de variables */
	static int AlloueN = 0;
	static LigneCreuse * __restrict L = NULL;
	static int * __restrict nombre_nnz_colonne = NULL;
	static int AlloueCOL = 0;
	static int AlloueNCOL = 0;
	static int AlloueLIGNE_mat = 0;
	static unsigned char ** __restrict m = NULL;
	static int * __restrict nouvelle_colonne = NULL;
	static int * __restrict colonne_changer = NULL;
	//static int compteur1 = 0, compteur2 = 0, compteur3 = 0, compteur4 = 0;

	if (!E->initialise) InitialiseSousMat(E);
	
	if (N*E->gen>AlloueN)
	{
		free(L);
		AlloueN=N*E->gen;
		//printf("AlloueN : %d\n",AlloueN);
		L=malloc(AlloueN*sizeof(LigneCreuse));
	}
	
	if (E->col_max>AlloueCOL)
	{
		nouvelle_colonne=realloc(nouvelle_colonne,E->col_max*sizeof(int));
		int i;
		for (i = AlloueCOL; i < E->col_max; i++) nouvelle_colonne[i]=-1;
		free(nombre_nnz_colonne);
		AlloueCOL=E->col_max;
		//printf("AlloueCOL : %d\n",AlloueCOL);
		nombre_nnz_colonne=calloc(AlloueCOL,sizeof(int));
	}

	int NombreDeLignes =  0;
	int taille=0,i;
	for (i = 0; i < N; i++)
	{
		if (E->lignevar[X[i]]*E->gen<E->a)
		{
			int j;
			for (j = 0; j < E->sousmat[X[i]]->nombre_de_lignes; j++) L[NombreDeLignes++]=E->sousmat[X[i]]->ligne[j];
			taille++;
		}
	}
	TriLignesCreusesByNNZ(L,NombreDeLignes);
	
	/* 
	 * On compte le nombre d'éléments non-nuls sur chaque colonne
	 */
	int dim = (1-E->gen)*taille;
	int NombreDeColonnes = 0;
	for (i = 0; i < NombreDeLignes; i++)
	{
		unsigned char pivot_present = 0;
		int j;
		for (j = 0; j < L[i]->nnz; j++)
		{
			if (!nombre_nnz_colonne[L[i]->elements[j].colonne])
			{
				pivot_present = 1;
				NombreDeColonnes++;
			}
			nombre_nnz_colonne[L[i]->elements[j].colonne]++;
		}
		if (pivot_present)
		{
			dim++;
			if (dim>borne) // on est sur que la sortie sera plus grande que borne
			{
				for (j = 0; j <= i ; j++) // on remet le tableau à zero
				{
					int k;
					for (k = 0; k < L[j]->nnz; k++) nombre_nnz_colonne[L[j]->elements[k].colonne]=0;
				}
				//compteur1++;
				//printf("cpt1 = %d, cpt2 = %d, cpt3 = %d, cpt4 = %d\n",compteur1,compteur2,compteur3,compteur4);
				return dim;
			}
		}
	}
	
	dim = (1-E->gen)*taille;
	int ligne = 0;
	while (ligne < NombreDeLignes)
	{
		int nombre_element_ligne = 0;
		int colonne_a_enlever;
		int j;
		for (j = 0; j < L[ligne]->nnz; j++)
		{
			if (nombre_nnz_colonne[L[ligne]->elements[j].colonne])
			{
				if (nombre_nnz_colonne[L[ligne]->elements[j].colonne]==1) // 1 seul élément non nul sur la colonne -> très bon pivot pour matrice creuse
				{
					int l;
					for (l = 0; l < L[ligne]->nnz; l++)
					{
						if (nombre_nnz_colonne[L[ligne]->elements[l].colonne])
						{
							if (nombre_nnz_colonne[L[ligne]->elements[l].colonne] == 1) NombreDeColonnes--;
							nombre_nnz_colonne[L[ligne]->elements[l].colonne]--; // On met à jour le nombre d'éléments dans les colonnes
						}
					}
					goto suppr_ligne;
				}
				else
				{
					nombre_element_ligne++;
					colonne_a_enlever = j;
				}
			}
		}
		if (nombre_element_ligne<=1)
		{
			if (nombre_element_ligne==1) // 1 seul élément sur la ligne
			{
				nombre_nnz_colonne[L[ligne]->elements[colonne_a_enlever].colonne] = 0; // on supprime la colonne
				NombreDeColonnes--;
				suppr_ligne: // on met à jour la dim et on supprime la ligne
				dim++;
				if (dim>borne) goto remise_a_zero;
			}
			NombreDeLignes--;
			L[ligne]=L[NombreDeLignes];
		}
		else ligne++;
	}

	{

		if (NombreDeColonnes>AlloueNCOL)
		{
			AlloueNCOL=NombreDeColonnes;
			for (i = 0; i < AlloueLIGNE_mat; i++)
			{
				free(m[i]);
				m[i]=malloc(AlloueNCOL*sizeof(unsigned char));
			}
			free(colonne_changer);
			colonne_changer=malloc((AlloueNCOL+1)*sizeof(int));
		}
		
		if (NombreDeLignes>AlloueLIGNE_mat)
		{
			m = realloc(m,NombreDeLignes*sizeof(unsigned char *));
			for (i = AlloueLIGNE_mat; i < NombreDeLignes; i++) m[i] = malloc(AlloueNCOL*sizeof(unsigned char));
			AlloueLIGNE_mat = NombreDeLignes;
			//printf("AlloueLIGNE_mat : %d\n",AlloueLIGNE_mat);
		}
		
		int pos=0;
		int DerniereColonne = NombreDeColonnes-1;
		int Ncol = 0;
		const int new_borne = borne - dim + 1;
		for (i = 0; i < NombreDeLignes; i++)
		{
			int k;
			unsigned char * __restrict ligne_matrice = m[i];
			for (k = 0; k < NombreDeColonnes; k++) ligne_matrice[k]=0;
			int premier = 1;
			for (k = 0; k < L[i]->nnz; k++)
			{
				if (nombre_nnz_colonne[L[i]->elements[k].colonne])
				{
					nombre_nnz_colonne[L[i]->elements[k].colonne]--;
					int colonne;
					colonne_changer[pos]=L[i]->elements[k].colonne;
					if (nouvelle_colonne[colonne_changer[pos]]>=0) colonne=nouvelle_colonne[colonne_changer[pos]];
					else
					{
						if (!premier)
						{
							nouvelle_colonne[colonne_changer[pos++]]=DerniereColonne;
							colonne=DerniereColonne--;
						}
						else
						{
							nouvelle_colonne[colonne_changer[pos++]]=Ncol;
							colonne=Ncol++;
							if (Ncol != new_borne) premier=0;
							else // on est sur que la sortie sera plus grande que borne
							{
								for (i = i; i < NombreDeLignes; i++)
								{
									for (k = 0; k < L[i]->nnz; k++)	nombre_nnz_colonne[L[i]->elements[k].colonne]=0;								
								}
								for (i = 0; i < pos; i++) nouvelle_colonne[colonne_changer[i]]=-1;
								return borne+1;
							}
						}
					}
					ligne_matrice[colonne]=L[i]->elements[k].coeff;								
				}
			}
		}
				
		for (i = 0; i < pos; i++) nouvelle_colonne[colonne_changer[i]]=-1; // on remet le tableau à zero pour la prochaine execution

		//compteur4++;
		//printf("cpt1 = %d, cpt2 = %d, cpt3 = %d, cpt4 = %d\n",compteur1,compteur2,compteur3,compteur4);
																
		return dim+DimensionBorne_tmp(m,NombreDeLignes,NombreDeColonnes,borne-dim);

	}

	remise_a_zero :
		
	for (ligne = 0; ligne < NombreDeLignes; ligne++)
	{
		int k;
		for (k = 0; k < L[ligne]->nnz; k++) nombre_nnz_colonne[L[ligne]->elements[k].colonne]=0;
	}
	return dim;
}

#else

void PrintLigneCreuse(LigneCreuse L)
{
	int i;
	for (i = 0; i < L->nnz; i++)
	{
		printf("(%02x,%d) ",L->elements[i].coeff,L->elements[i].colonne);
	}
	printf("\n");
}

void PrintMatCreuse(MatriceCreuse M)
{
	int i;
	for (i = 0; i < M->nombre_de_lignes; i++)
	{
		PrintLigneCreuse(M->ligne[i]);
	}
	printf("\n");
}

LigneCreuse AddWithCoeff(const LigneCreuse ligne1,const LigneCreuse ligne2, const unsigned char coeff)
{
	COL_COEFF * __restrict tmp = malloc((ligne1->nnz+ligne2->nnz)*sizeof(COL_COEFF));
	int i = 0;
	int j = 0;
	int pos = 0;
	while (i!=ligne1->nnz && j!=ligne2->nnz)
	{
		if (ligne1->elements[i].colonne<=ligne2->elements[j].colonne)
		{
			if (ligne1->elements[i].colonne == ligne2->elements[j].colonne)
			{
				tmp[pos].coeff=ligne1->elements[i++].coeff^Multiply(coeff,ligne2->elements[j].coeff);
				if (tmp[pos].coeff) tmp[pos++].colonne = ligne2->elements[j].colonne;
				j++;
			}
			else tmp[pos++]=ligne1->elements[i++];
		}
		else
		{
			tmp[pos].coeff=Multiply(coeff,ligne2->elements[j].coeff);
			tmp[pos++].colonne=ligne2->elements[j++].colonne;
		}
	}
	int k;
	for (k = i; k < ligne1->nnz; k++) tmp[pos++]=ligne1->elements[k];
	for (k = j; k < ligne2->nnz; k++)
	{
		tmp[pos].coeff=Multiply(coeff,ligne2->elements[k].coeff);
		tmp[pos++].colonne=ligne2->elements[k].colonne;
	}
	LigneCreuse L = NouvelleLigneCreuse();
	L->nnz=pos;
	L->elements=tmp;
	return L;
}

void AjouterLigneTas(LigneCreuse L, LigneCreuse * __restrict Tas, int * __restrict N)
{
	(*N)++;
	int new_position = *N;
	int parents = new_position/2;
	const int c = L->elements[0].colonne;
	const int nnz = L->nnz;
	while (parents && (c < Tas[parents]->elements[0].colonne || (c == Tas[parents]->elements[0].colonne && nnz < Tas[parents]->nnz)))
	{
		Tas[new_position]=Tas[parents];
		new_position=parents;
		parents=parents/2;
	}
	Tas[new_position]=L;
}

void TamiserTas(LigneCreuse * __restrict Tas, const int N)
{
	int new_position = 1;
	LigneCreuse cst = Tas[1];
	int fils = 2;
	while (fils <= N )
	{
		if (fils < N && (Tas[fils]->elements[0].colonne>Tas[fils+1]->elements[0].colonne || (Tas[fils]->elements[0].colonne == Tas[fils+1]->elements[0].colonne && Tas[fils]->nnz > Tas[fils+1]->nnz))) fils++;
		if ((Tas[fils]->elements[0].colonne<cst->elements[0].colonne || (Tas[fils]->elements[0].colonne==cst->elements[0].colonne && Tas[fils]->nnz<cst->nnz )))
		{
			Tas[new_position]=Tas[fils];
			new_position=fils;
			fils=2*fils;
		}
		else break;
	}
	Tas[new_position]=cst;
}

LigneCreuse PremierElementTas(LigneCreuse * __restrict Tas, int * __restrict N)
{
	LigneCreuse tmp = Tas[1];
	Tas[1]=Tas[*N];
	(*N)--;
	TamiserTas(Tas,*N);
	return tmp;
}

void EchelonnerMatriceCreuse(MatriceCreuse M)
{
	static LigneCreuse * __restrict Tas=NULL;
	static int Alloue = 0;

	if (M->nombre_de_lignes>Alloue)
	{
		Alloue=M->nombre_de_lignes;
		free(Tas);
		Tas=malloc((Alloue+1)*sizeof(LigneCreuse));
	}

	//printf("Matrice avant pivot :\n");
	//PrintMatCreuse(M);
	//getchar();

	int N = 0;
	int pos;
	for (pos = 0; pos < M->nombre_de_lignes; pos++) AjouterLigneTas(M->ligne[pos],Tas,&N);

	/*printf("Tas :\n");
	int i;
	for (i = 1; i <= N; i++)
	{
		printf("	%d : ",i);
		PrintLigneCreuse(Tas[i]);
	}
	printf("\n");*/
	
	

	pos = 0;
	while (N)
	{
		M->ligne[pos]=PremierElementTas(Tas,&N);
		while (N && Tas[1]->elements[0].colonne==M->ligne[pos]->elements[0].colonne)
		{
			const unsigned char coeff=Multiply(Inverse(M->ligne[pos]->elements[0].coeff),Tas[1]->elements[0].coeff);
			LigneCreuse tmp = AddWithCoeff(Tas[1],M->ligne[pos],coeff);
			freeLigneCreuse(Tas[1]);
			if (tmp->nnz) Tas[1]=tmp;
			else
			{
				freeLigneCreuse(tmp);
				Tas[1]=Tas[N--];
			}
			TamiserTas(Tas,N);
		}
		pos++;
		/*printf("Tas :\n");
		for (i = 1; i <= N; i++)
		{
			printf("	%d : ",i);
			PrintLigneCreuse(Tas[i]);
		}
		printf("\n");*/
	}
	M->nombre_de_lignes = pos;

	//printf("Matrice après pivot :\n");
	//PrintMatCreuse(M);
	//getchar();
}

MatriceCreuse InfoSortie_VariablesVersion(int * __restrict X, const int N, SysEqLin E)
{
	if (!E->initialise)
	{
		InitialiseSousMat_tmp(E);
		E->initialise=1;
	}
	MatriceCreuse M = NouvelleMatriceCreuse();
	M->ligne = malloc(E->gen*N*sizeof(LigneCreuse));
	int pos = 0;
	int i;
	for (i = 0; i < N; i++)
	{
		if (E->lignevar[X[i]]*E->gen<E->a)
		{
			MatriceCreuse tmp = E->sousmat[X[i]];
			int j;
			for (j = 0; j < tmp->nombre_de_lignes; j++) M->ligne[pos++]=copyLigneCreuse(tmp->ligne[j]);
		}
	}
	M->nombre_de_lignes=pos;
	EchelonnerMatriceCreuse(M);
	return M;
}

MatriceCreuse InfoSortie_UnionVersion(Solver B1, Solver B2, SysEqLin E)
{
	static int Alloue = 0;
	static MatriceCreuse * __restrict L = NULL;
	static MatriceCreuse * __restrict L_tmp = NULL;
	static int * __restrict pos = NULL;
	static int * __restrict pivot = NULL;

	
	if (B1->initialisation!=E)
	{
		freeMatriceCreuse(B1->mat);
		B1->mat=InfoSortie_VariablesVersion(B1->var,B1->Nvar,E);
		B1->initialisation=E;
	}
	if (B2->initialisation!=E)
	{
		freeMatriceCreuse(B2->mat);
		B2->mat=InfoSortie_VariablesVersion(B2->var,B2->Nvar,E);
		B2->initialisation=E;
	}

	int Nombre_de_lignes_max = B1->mat->nombre_de_lignes + B2->mat->nombre_de_lignes;

	if (Nombre_de_lignes_max>Alloue)
	{
		int i;
		for (i = 0; i < Alloue; i++)
		{
			L_tmp[i]->parents=1;
			freeMatriceCreuse(L_tmp[i]);
		}
		free(L_tmp);
		free(L);
		free(pos);
		free(pivot);
		Alloue = Nombre_de_lignes_max;
		L=malloc(Alloue*sizeof(MatriceCreuse));
		L_tmp=malloc(Alloue*sizeof(MatriceCreuse));
		for (i = 0; i < Alloue; i++)
		{
			L_tmp[i] = NouvelleMatriceCreuse();
			L_tmp[i]->ligne=malloc(Alloue*sizeof(LigneCreuse));
			L_tmp[i]->nombre_de_lignes=0;
			L_tmp[i]->parents=0;
		}
		pos=malloc(Alloue*sizeof(int));
		pivot=malloc(Alloue*sizeof(int));
	}

	if (B1->mat->nombre_de_lignes && B2->mat->nombre_de_lignes)
	{
		MatriceCreuse M = NouvelleMatriceCreuse();
		M->ligne=malloc(Nombre_de_lignes_max*sizeof(LigneCreuse));
		L[0]=B1->mat;
		pos[0]=0;
		L[1]=B2->mat;
		pos[1]=0;
		int pos_tmp = 0;
		int nb_ligne=0;
		int N = 2;
		int i;
		while (N>1)
		{
			int plus_petite_colonne = L[0]->ligne[pos[0]]->elements[0].colonne;
			pivot[0]=0;
			int nb_pivot=1;
			for (i = 1; i < N; i++)
			{
				if (L[i]->ligne[pos[i]]->elements[0].colonne<=plus_petite_colonne)
				{
					if (L[i]->ligne[pos[i]]->elements[0].colonne != plus_petite_colonne)
					{
						plus_petite_colonne = L[i]->ligne[pos[i]]->elements[0].colonne;
						nb_pivot=0;
					}
					pivot[nb_pivot++]=i;
				}
			}
			M->ligne[nb_ligne]=copyLigneCreuse(L[pivot[0]]->ligne[pos[pivot[0]]++]);
			if (nb_pivot>1)
			{
				L[N]=L_tmp[pos_tmp];
				for (i = 1; i < nb_pivot; i++)
				{
					const unsigned char cst = Multiply(Inverse(M->ligne[nb_ligne]->elements[0].coeff),L[pivot[i]]->ligne[pos[pivot[i]]]->elements[0].coeff);
					L[N]->ligne[L[N]->nombre_de_lignes] = AddWithCoeff(L[pivot[i]]->ligne[pos[pivot[i]]++],M->ligne[nb_ligne],cst);
					if (L[N]->ligne[L[N]->nombre_de_lignes]->nnz) L[N]->nombre_de_lignes++;
					else freeLigneCreuse(L[N]->ligne[L[N]->nombre_de_lignes]);
				}
				if (L[N]->nombre_de_lignes)
				{
					EchelonnerMatriceCreuse(L[N]);
					pos[N]=0;
					N++;
					pos_tmp++;
				}
			}
			for (i = nb_pivot-1; i >=0 ; i--)
			{
				if (pos[pivot[i]]==L[pivot[i]]->nombre_de_lignes)
				{
					if (!L[pivot[i]]->parents)
					{
						int j;
						for (j = 0; j < L[pivot[i]]->nombre_de_lignes; j++) freeLigneCreuse(L[pivot[i]]->ligne[j]);
						L[pivot[i]]->nombre_de_lignes=0;
					}
					L[pivot[i]] = L[--N];
					pos[pivot[i]]=pos[N];
				}
			}			
			nb_ligne++;
		}
		for (i = pos[0]; i < L[0]->nombre_de_lignes; i++) M->ligne[nb_ligne++]=copyLigneCreuse(L[0]->ligne[i]);
		if (!L[0]->parents)
		{
			for (i = 0; i < L[0]->nombre_de_lignes; i++) freeLigneCreuse(L[0]->ligne[i]);
			L[0]->nombre_de_lignes=0;
		}
		M->nombre_de_lignes=nb_ligne;
		return M;
	}
	else
	{
		if (B1->mat->nombre_de_lignes) return copyMatriceCreuse(B1->mat);
		else return copyMatriceCreuse(B2->mat);
	}
}

MatriceCreuse InfoSortie_UnionBorneVersion(Solver B1, Solver B2, SysEqLin E, int borne)
{
	static int Alloue = 0;
	static MatriceCreuse * __restrict L = NULL;
	static MatriceCreuse * __restrict L_tmp = NULL;
	static int * __restrict pos = NULL;
	static int * __restrict pivot = NULL;

	
	if (B1->initialisation!=E)
	{
		freeMatriceCreuse(B1->mat);
		B1->mat=InfoSortie_VariablesVersion(B1->var,B1->Nvar,E);
		B1->initialisation=E;
	}
	if (B2->initialisation!=E)
	{
		freeMatriceCreuse(B2->mat);
		B2->mat=InfoSortie_VariablesVersion(B2->var,B2->Nvar,E);
		B2->initialisation=E;
	}

	/*printf("Les deux blocs :\n");
	printf("	- Premier bloc :\n");
	PrintMatCreuse(B1->mat);
	printf("	- Deuxième bloc :\n");
	PrintMatCreuse(B2->mat);*/
	//getchar();

	int Nombre_de_lignes_max = B1->mat->nombre_de_lignes + B2->mat->nombre_de_lignes;

	if (Nombre_de_lignes_max>Alloue)
	{
		int i;
		for (i = 0; i < Alloue; i++)
		{
			L_tmp[i]->parents=1;
			freeMatriceCreuse(L_tmp[i]);
		}
		free(L_tmp);
		free(L);
		free(pos);
		free(pivot);
		Alloue = Nombre_de_lignes_max;
		L=malloc(Alloue*sizeof(MatriceCreuse));
		L_tmp=malloc(Alloue*sizeof(MatriceCreuse));
		for (i = 0; i < Alloue; i++)
		{
			L_tmp[i] = NouvelleMatriceCreuse();
			L_tmp[i]->ligne=malloc(Alloue*sizeof(LigneCreuse));
			L_tmp[i]->nombre_de_lignes=0;
			L_tmp[i]->parents=0;
		}
		pos=malloc(Alloue*sizeof(int));
		pivot=malloc(Alloue*sizeof(int));
	}

	if (B1->mat->nombre_de_lignes && B2->mat->nombre_de_lignes)
	{
		MatriceCreuse M = NouvelleMatriceCreuse();
		M->ligne=malloc(borne*sizeof(LigneCreuse));
		L[0]=B1->mat;
		pos[0]=0;
		L[1]=B2->mat;
		pos[1]=0;
		int pos_tmp = 0;
		int nb_ligne=0;
		int N = 2;
		int i;
		while (N>1)
		{
			int plus_petite_colonne = L[0]->ligne[pos[0]]->elements[0].colonne;
			pivot[0]=0;
			int nb_pivot=1;
			for (i = 1; i < N; i++)
			{
				if (L[i]->ligne[pos[i]]->elements[0].colonne<=plus_petite_colonne)
				{
					if (L[i]->ligne[pos[i]]->elements[0].colonne != plus_petite_colonne)
					{
						plus_petite_colonne = L[i]->ligne[pos[i]]->elements[0].colonne;
						nb_pivot=0;
					}
					pivot[nb_pivot++]=i;
				}
			}
			#if 1
			M->ligne[nb_ligne]=copyLigneCreuse(L[pivot[0]]->ligne[pos[pivot[0]]++]);
			if (nb_pivot>1)
			{
				L[N]=L_tmp[pos_tmp];
				for (i = 1; i < nb_pivot; i++)
				{
					if (M->ligne[nb_ligne]->nnz>L[pivot[i]]->ligne[pos[pivot[i]]]->nnz)
					{
						const unsigned char cst = Multiply(Inverse(L[pivot[i]]->ligne[pos[pivot[i]]]->elements[0].coeff),M->ligne[nb_ligne]->elements[0].coeff);
						L[N]->ligne[L[N]->nombre_de_lignes] = AddWithCoeff(M->ligne[nb_ligne],L[pivot[i]]->ligne[pos[pivot[i]]],cst);
						freeLigneCreuse(M->ligne[nb_ligne]);
						M->ligne[nb_ligne]=copyLigneCreuse(L[pivot[i]]->ligne[pos[pivot[i]]++]);
					}
					else
					{
						const unsigned char cst = Multiply(Inverse(M->ligne[nb_ligne]->elements[0].coeff),L[pivot[i]]->ligne[pos[pivot[i]]]->elements[0].coeff);
						L[N]->ligne[L[N]->nombre_de_lignes] = AddWithCoeff(L[pivot[i]]->ligne[pos[pivot[i]]++],M->ligne[nb_ligne],cst);
					}
					if (L[N]->ligne[L[N]->nombre_de_lignes]->nnz) L[N]->nombre_de_lignes++;
					else freeLigneCreuse(L[N]->ligne[L[N]->nombre_de_lignes]);
				}
				if (L[N]->nombre_de_lignes)
				{
					EchelonnerMatriceCreuse(L[N]);
					pos[N]=0;
					N++;
					pos_tmp++;
				}
			}
			#else
			if (nb_pivot==1) M->ligne[nb_ligne]=copyLigneCreuse(L[pivot[0]]->ligne[pos[pivot[0]]++]);
			else
			{
				L[N]=L_tmp[pos_tmp];
				for (i = 0; i < nb_pivot; i++) L[N]->ligne[L[N]->nombre_de_lignes++]=copyLigneCreuse(L[pivot[i]]->ligne[pos[pivot[i]]++]);
				EchelonnerMatriceCreuse(L[N]);
				if (L[N]->nombre_de_lignes==1)
				{
					M->ligne[nb_ligne]=L[N]->ligne[0];
					L[N]->nombre_de_lignes=0;
				}
				else
				{
					M->ligne[nb_ligne]=copyLigneCreuse(L[N]->ligne[0]);
					pos[N]=1;
					N++;
					pos_tmp++;
				}
			}			
			#endif
			borne--;
			int debut = N-1;
			for (i = nb_pivot-1; i>=0 ; i--)
			{
				const int borne1 = pivot[i];
				int j;
				for (j = debut; j > borne1 ; j--) if (L[j]->nombre_de_lignes-pos[j]>borne) goto sortie_prematuree;
				if (pos[borne1]==L[borne1]->nombre_de_lignes)
				{
					if (!L[borne1]->parents)
					{
						for (j = 0; j < L[borne1]->nombre_de_lignes; j++) freeLigneCreuse(L[borne1]->ligne[j]);
						L[borne1]->nombre_de_lignes=0;
					}
					L[borne1] = L[--N];
					pos[borne1]=pos[N];
				}
				debut=borne1-1;
			}
			for (i = debut; i >=0 ; i--) if (L[i]->nombre_de_lignes-pos[i]>borne) goto sortie_prematuree;
					
			nb_ligne++;
		}
		for (i = pos[0]; i < L[0]->nombre_de_lignes; i++) M->ligne[nb_ligne++]=copyLigneCreuse(L[0]->ligne[i]);
		if (!L[0]->parents)
		{
			for (i = 0; i < L[0]->nombre_de_lignes; i++) freeLigneCreuse(L[0]->ligne[i]);
			L[0]->nombre_de_lignes=0;
		}
		M->nombre_de_lignes=nb_ligne;

		return M;
		
		sortie_prematuree :

		for (i = 0; i < N; i++)
		{
			if (!L[i]->parents)
			{
				int j;
				for (j = 0; j < L[i]->nombre_de_lignes; j++) freeLigneCreuse(L[i]->ligne[j]);
				L[i]->nombre_de_lignes=0;
			}
		}
		for (i = 0; i <= nb_ligne; i++) freeLigneCreuse(M->ligne[i]);
		M->nombre_de_lignes = -1;
		return M;
		
	}
	else
	{
		if (B1->mat->nombre_de_lignes) return copyMatriceCreuse(B1->mat);
		else return copyMatriceCreuse(B2->mat);
	}
}

int TailleE(int *X, int N, SysEqLin E) // Renvoi n - dim(E(X)),  n = |X|
{
	static MATRICECREUSE M={0,NULL,1};
	static int Alloue=0;

	if (!E->initialise)
	{
		InitialiseSousMat_tmp(E);
		E->initialise=1;
	}

	if (E->gen*N>Alloue)
	{
		free(M.ligne);
		Alloue=E->gen*N;
		M.ligne=malloc(Alloue*sizeof(LigneCreuse));
	}
	
	int pos = 0;
	int cpt = 0;
	int i;
	for (i = 0; i < N; i++)
	{
		if (E->lignevar[X[i]]*E->gen<E->a)
		{
			MatriceCreuse tmp = E->sousmat[X[i]];
			int j;
			for (j = 0; j < tmp->nombre_de_lignes; j++) M.ligne[pos++]=copyLigneCreuse(tmp->ligne[j]);
			cpt++;
		}
	}
	M.nombre_de_lignes=pos;
	EchelonnerMatriceCreuse(&M);
	for (i = 0; i < M.nombre_de_lignes; i++) freeLigneCreuse(M.ligne[i]);
	return M.nombre_de_lignes-cpt*(E->gen-1);
}

#endif

SysEqLin Transform(Equation *Eq, int taille, int gen) // Transforme une système d'équations en SysEqLin
{
	int i;
#if DISPLAY_EQUATIONS==1
	for (i = 0; i < taille; i++)
	{
		printf("Eq[%d] : ",i);
		PrintEq(Eq[i]);
		printf("\n");
	}
#endif
	SysEqLin E=malloc(sizeof(SYSEQLIN));
	E->a=gen*VARIABLES;
	E->b=taille;
	E->gen=gen;
	E->mat=malloc(E->a*sizeof(unsigned char *));
	int * __restrict dedans=malloc(VARIABLES*sizeof(int));
	for (i = 0; i < VARIABLES; i++) dedans[i]=0;
	for (i = 0; i < E->a; i++)
	{
		E->mat[i]=malloc(taille*sizeof(unsigned char));
		int j;
		for (j = 0; j < taille; j++) E->mat[i][j]=0;
	}
	for (i = 0; i < taille; i++)
	{
		int j;
		for (j = 0; j < Eq[i]->nV; j++)
		{
			E->mat[gen*Eq[i]->var[j]][i]^=Eq[i]->coefV[j];
			dedans[Eq[i]->var[j]]=1;
		}
		for (j = 0; j < Eq[i]->nS; j++)
		{
			E->mat[gen*Eq[i]->S[j]->var[0]+Eq[i]->sbox[j]+1][i]^=Eq[i]->coefS[j];
			dedans[Eq[i]->S[j]->var[0]]=1;
		}
	}
	E->varligne=malloc(VARIABLES*sizeof(int));
	E->lignevar=malloc(VARIABLES*sizeof(int));
	for (i = 0; i < VARIABLES; i++) E->varligne[i]=i;
	for (i = 0; i < VARIABLES; i++) E->lignevar[i]=i;
	for (i = 0; i < VARIABLES; i++)
	{
		if (dedans[i]==0)
		{
			E->a=E->a-gen;
			PermuterVar(i,E->varligne[E->a/gen],E);
		}
	}
	E->initialise=0;
	return E;
}


SysEqLin Transform2(Equation *Eq, int taille, int gen) // Transforme une système d'équations en SysEqLin
{
	Equation *A;
	SysEqLin E;
	int i,*dedans,j,NA;
	A=Linearise(Eq,taille,&NA); // Probablement inutile maintenant
#if DISPLAY_EQUATIONS==1
	for (i = 0; i < NA; i++)
	{
		printf("Eq[%d] : ",i);
		PrintEq(A[i]);
		printf("\n");
	}
#endif
	E=malloc(sizeof(SYSEQLIN));
	E->a=gen*VARIABLES;
	E->b=NA;
	E->gen=gen;
	E->mat=malloc(E->a*sizeof(unsigned char *));
	dedans=malloc(VARIABLES*sizeof(int));
	for (i = 0; i < VARIABLES; i++) dedans[i]=0;
	for (i = 0; i < E->a; i++)
	{
		E->mat[i]=malloc(NA*sizeof(unsigned char));
		for (j = 0; j < NA; j++) E->mat[i][j]=0;
	}
	for (i = 0; i < NA; i++)
	{
		for (j = 0; j < A[i]->nV; j++)
		{
			E->mat[gen*A[i]->var[j]][i]^=A[i]->coefV[j];
			dedans[A[i]->var[j]]=1;
		}
		for (j = 0; j < A[i]->nS; j++)
		{
			E->mat[gen*A[i]->S[j]->var[0]+A[i]->sbox[j]+1][i]^=A[i]->coefS[j];
			dedans[A[i]->S[j]->var[0]]=1;
		}
		freeEq(A[i]);
	}
	free(A);
	E->varligne=malloc(VARIABLES*sizeof(int));
	E->lignevar=malloc(VARIABLES*sizeof(int));
	for (i = 0; i < VARIABLES; i++) E->varligne[i]=i;
	for (i = 0; i < VARIABLES; i++) E->lignevar[i]=i;
	for (i = 0; i < VARIABLES; i++)
	{
		if (dedans[i]==0)
		{
			E->a=E->a-gen;
			PermuterVar(i,E->varligne[E->a/gen],E);
		}
	}
	E->initialise=0;
	return E;
}

int ExtraireEX(unsigned char ** __restrict dst, int * __restrict var, const int N, SysEqLin E) // Calcul E(X) (sous forme de matrice)
{

	const int nombre_ligne_big_matrix = E->a;
	const int nombre_colone_big_matrix = E->b + E->gen*N;
	unsigned char ** __restrict big_matrix=malloc(nombre_ligne_big_matrix*sizeof(unsigned char *));
	unsigned char * __restrict espace_big_matrix=malloc(nombre_colone_big_matrix*nombre_ligne_big_matrix*sizeof(unsigned char));

	int i;
	for (i = 0; i < nombre_ligne_big_matrix; i++) big_matrix[i]=espace_big_matrix + i*nombre_colone_big_matrix;
	for (i = 0; i < nombre_ligne_big_matrix; i++)
	{
		int j;
		for (j = 0; j < E->b; j++) big_matrix[i][j]=E->mat[i][j];
		for (j = E->b; j < nombre_colone_big_matrix; j++) big_matrix[i][j]=0;
	}
	for (i = 0; i < N; i++)
	{
		if (E->lignevar[var[i]]<E->a/E->gen)
		{
			int j;
			for (j = 0; j < E->gen; j++) big_matrix[E->gen*E->lignevar[var[i]]+j][E->b+E->gen*i+j]=1;
		}
	}
	
	int dim = 0;
	const int colonne_max_pour_pivot = E->b;
	int * __restrict place_pivot = malloc(min(colonne_max_pour_pivot,nombre_ligne_big_matrix)*sizeof(int));
	int pivot_c;
	for (pivot_c = 0; pivot_c < colonne_max_pour_pivot; pivot_c++)
	{
		int new_pivot_l;
		for (new_pivot_l = dim; new_pivot_l < nombre_ligne_big_matrix; new_pivot_l++)
		{
			if (big_matrix[new_pivot_l][pivot_c])
			{
				if (new_pivot_l!=dim)
				{
					unsigned char *tmp = big_matrix[dim];
					big_matrix[dim]=big_matrix[new_pivot_l];
					big_matrix[new_pivot_l]=tmp;
				}
				place_pivot[dim]=pivot_c;
				const unsigned char inv = Inverse(big_matrix[dim][pivot_c]);
				int colonne;
				for (colonne = pivot_c; colonne < nombre_colone_big_matrix; colonne++) big_matrix[dim][colonne]=Multiply(inv,big_matrix[dim][colonne]);
				int ligne;
				for (ligne = 0; ligne < dim; ligne++)
				{
					if (big_matrix[ligne][pivot_c])
					{
						for (colonne = pivot_c+1; colonne < nombre_colone_big_matrix; colonne++)
						{
							big_matrix[ligne][colonne]^=Multiply(big_matrix[ligne][pivot_c],big_matrix[dim][colonne]);
						}
						big_matrix[ligne][pivot_c]=0;
					}
				}
				for (ligne = new_pivot_l+1; ligne < nombre_ligne_big_matrix; ligne++)
				{
					if (big_matrix[ligne][pivot_c])
					{
						for (colonne = pivot_c+1; colonne < nombre_colone_big_matrix; colonne++)
						{
							big_matrix[ligne][colonne]^=Multiply(big_matrix[ligne][pivot_c],big_matrix[dim][colonne]);
						}
						big_matrix[ligne][pivot_c]=0;
					}
				}
				dim++;
				break;
			}
		}
	}
	
	const int dim_save = dim;
	for (pivot_c = colonne_max_pour_pivot; pivot_c < nombre_colone_big_matrix; pivot_c++)
	{
		int new_pivot_l;
		for (new_pivot_l = dim; new_pivot_l < nombre_ligne_big_matrix; new_pivot_l++)
		{
			if (big_matrix[new_pivot_l][pivot_c])
			{
				if (new_pivot_l!=dim)
				{
					unsigned char *tmp = big_matrix[dim];
					big_matrix[dim]=big_matrix[new_pivot_l];
					big_matrix[new_pivot_l]=tmp;
				}
				const unsigned char inv = Inverse(big_matrix[dim][pivot_c]);
				int colonne;
				for (colonne = pivot_c; colonne < nombre_colone_big_matrix; colonne++) big_matrix[dim][colonne]=Multiply(inv,big_matrix[dim][colonne]);
				int ligne;
				for (ligne = 0; ligne < dim; ligne++)
				{
					if (big_matrix[ligne][pivot_c])
					{
						for (colonne = pivot_c+1; colonne < nombre_colone_big_matrix; colonne++)
						{
							big_matrix[ligne][colonne]^=Multiply(big_matrix[ligne][pivot_c],big_matrix[dim][colonne]);
						}
						big_matrix[ligne][pivot_c]=0;
					}
				}
				for (ligne = new_pivot_l+1; ligne < nombre_ligne_big_matrix; ligne++)
				{
					if (big_matrix[ligne][pivot_c])
					{
						for (colonne = pivot_c+1; colonne < nombre_colone_big_matrix; colonne++)
						{
							big_matrix[ligne][colonne]^=Multiply(big_matrix[ligne][pivot_c],big_matrix[dim][colonne]);
						}
						big_matrix[ligne][pivot_c]=0;
					}
				}
				dim++;
				break;
			}
		}
	}

	const int nombre_de_lignes_max = VARIABLES*E->gen;
	int pos = 0;
	int ligne;	
	int colonne = colonne_max_pour_pivot;
	for (ligne = dim_save; ligne < dim; ligne++)
	{
		while (colonne<nombre_colone_big_matrix && big_matrix[ligne][colonne]==0)
		{
			unsigned char premier = 0;
			int j;
			for (j = 0; j < dim_save; j++)
			{
				if (big_matrix[j][colonne])
				{
					int k;
					if (premier) for (k = 0; k < nombre_de_lignes_max; k++) dst[k][pos]^=Multiply(big_matrix[j][colonne],E->mat[k][place_pivot[j]]);
					else
					{
						for (k = 0; k < nombre_de_lignes_max; k++) dst[k][pos]=Multiply(big_matrix[j][colonne],E->mat[k][place_pivot[j]]);
						premier=1;
					}			
				}
			}
			if (premier) pos++;
			colonne++;
		}
		colonne++;
	}

	for (i = colonne; i < nombre_colone_big_matrix; i++)
	{
		unsigned char premier = 0;
		int j;
		for (j = 0; j < dim_save; j++)
		{
			if (big_matrix[j][i])
			{
				int k;
				if (premier) for (k = 0; k < nombre_de_lignes_max; k++) dst[k][pos]^=Multiply(big_matrix[j][i],E->mat[k][place_pivot[j]]);
				else
				{
					for (k = 0; k < nombre_de_lignes_max; k++) dst[k][pos]=Multiply(big_matrix[j][i],E->mat[k][place_pivot[j]]);
					premier=1;
				}			
			}
		}
		if (premier) pos++;
	}

	free(place_pivot);
	free(big_matrix);
	free(espace_big_matrix);

	return pos;	

	/* A améliorer */
	#if 0
	int ligne=VARIABLES*E->gen;
	unsigned char **m=malloc(ligne*sizeof(unsigned char *));
	int b1=(N+VARIABLES)*E->gen-E->a, i;
	for (i = 0; i < ligne; i++)
	{
		m[i]=malloc(b1*sizeof(unsigned char));
		int j;
		for (j = 0; j < b1; j++) m[i][j]=0;
	}
	int col=0;
	for (i = 0; i < N; i++)
	{
		int j;
		for (j = 0; j < E->gen; j++) m[E->gen*E->lignevar[var[i]]+j][col++]=1;
	}
	for (i = E->a; i < VARIABLES*E->gen; i++) m[i][col++]=1;
	int b=Intersection(dst,E->mat,E->b,m,ligne,b1);
	for (i = 0; i < ligne; i++) free(m[i]);
	free(m);
	return b;
	#endif
}

SysEqLin ExtraireEX2(int *var, int N, SysEqLin E) // Calcul E(X) (sous forme de SysEqLin)
{
	SysEqLin F=malloc(sizeof(SYSEQLIN));
	F->mat=malloc(E->gen*VARIABLES*sizeof(unsigned char *));
	int i;
	for (i = 0; i < E->gen*VARIABLES; i++) F->mat[i]=malloc(E->b*sizeof(unsigned char));
	F->b=ExtraireEX(F->mat,var,N,E);
	F->gen=E->gen;
	F->lignevar=malloc(VARIABLES*sizeof(int));
	F->varligne=malloc(VARIABLES*sizeof(int));
	for (i = 0; i < VARIABLES; i++) F->lignevar[i]=E->lignevar[i];
	for (i = 0; i < VARIABLES; i++) F->varligne[i]=E->varligne[i];
	for (i = 0; i < N; i++) PermuterVar(var[i],F->varligne[i],F);
	F->a=N*E->gen;
	F->initialise=0;
	return F;
}

SysEqLin ExtraireEX2Special(int *var, int N, SysEqLin E) // Calcul E(X) mais supprime les equations entre les variables connues.
{
	SysEqLin F=malloc(sizeof(SYSEQLIN));
	F->mat=malloc(E->gen*VARIABLES*sizeof(unsigned char *));
	int i;
	for (i = 0; i < E->gen*VARIABLES; i++) F->mat[i]=malloc(E->b*sizeof(unsigned char));
	F->b=ExtraireEX(F->mat,var,N,E);
	F->gen=E->gen;
	F->lignevar=malloc(VARIABLES*sizeof(int));
	F->varligne=malloc(VARIABLES*sizeof(int));
	for (i = 0; i < VARIABLES; i++) F->lignevar[i]=E->lignevar[i];
	for (i = 0; i < VARIABLES; i++) F->varligne[i]=E->varligne[i];
	for (i = 0; i < N; i++) PermuterVar(var[i],F->varligne[i],F);
	F->a=N*E->gen;
	unsigned char **m=malloc(E->gen*VARIABLES*sizeof(unsigned char *));
	for (i = 0; i < E->gen*VARIABLES; i++) m[i]=malloc(F->b*sizeof(unsigned char));
	F->b=BaseSpeciale(m,F->mat,E->gen*VARIABLES,F->b,F->a);
	for (i = 0; i < E->gen*VARIABLES; i++) free(F->mat[i]);
	free(F->mat);
	F->mat=m;
	F->initialise=0;
	return F;
}

SysEqLin EsachantX(int *var, int N, SysEqLin E) // Calcul E|_X (les variables X sont maintenant supposées connues
{
	SysEqLin F=malloc(sizeof(SYSEQLIN));
	F->a=E->a;
	F->b=E->b;
	F->gen=E->gen;
	F->mat=malloc(E->gen*VARIABLES*sizeof(unsigned char *));
	int i;
	for (i = 0; i < E->gen*VARIABLES; i++)
	{
		F->mat[i]=malloc(F->b*sizeof(unsigned char));
		int j;
		for (j = 0; j < F->b; j++) F->mat[i][j]=E->mat[i][j];		
	}
	F->varligne=malloc(VARIABLES*sizeof(int));
	F->lignevar=malloc(VARIABLES*sizeof(int));
	
	for (i = 0; i < VARIABLES; i++) F->varligne[i]=E->varligne[i];
	for (i = 0; i < VARIABLES; i++) F->lignevar[i]=E->lignevar[i];
	for (i = 0; i < N; i++)
	{
		if (F->gen*F->lignevar[var[i]]<F->a)
		{
			F->a=F->a-F->gen;
			int l;
			for (l = 0; l < F->gen; l++) echangeligne(F->gen*F->lignevar[var[i]]+l,F->a+l,F->mat,F->gen*VARIABLES,F->b);
			int j=F->varligne[F->a/F->gen];
			int k=F->lignevar[var[i]];
			F->lignevar[var[i]]=F->lignevar[j];
			F->lignevar[j]=k;
			F->varligne[F->a/F->gen]=var[i];
			F->varligne[k]=j;
		}
	}
	F->initialise=0;
	return F;
}

Equation * ConversionSys(SysEqLin E) // Convertit un SysEqLin en système d'équations, le résulat contient E->b équations.
{
	Equation * __restrict Eq=malloc(E->b*sizeof(Equation));
	int i;
	for (i = 0; i < E->b; i++)
	{
		Eq[i]=NulEq();
		int j;
		for (j = 0; j < VARIABLES; j++)
		{
			int k;
			for (k = 0; k < E->gen; k++)
			{
				Equation tmp=varEq(E->varligne[j]);
				if (k!=0)
				{
					Equation tmp1=appliqueS(tmp,k-1);
					freeEq(tmp);
					tmp=tmp1;
				}
				Equation tmp1=mulEq(E->mat[E->gen*j+k][i],tmp);
				freeEq(tmp);
				tmp=addEq(tmp1,Eq[i]);
				freeEq(Eq[i]);
				freeEq(tmp1);
				Eq[i]=tmp;
			}
		}
	}
	return Eq;
}

Equation * ConversionSys2(int *X, int n, SysEqLin E) // Convertit un SysEqLin en système d'équations
{
	/* Propriété speciale :
	 *		- le resulat a 2*E->b Equation
	 * 		- le resultat est à lire Eq[i] = Eq[ i + E->b], Eq[i] est en les variables X, 0 <= i < E->b
	 */
	 
	Tri(X,n);
	Equation * __restrict Eq=malloc(2*E->b*sizeof(Equation));
	int i;
	for (i = 0; i < E->b; i++)
	{
		Eq[i]=NulEq();
		Eq[i+E->b]=NulEq();
		int pos=0,j;
		for (j = 0; j < VARIABLES; j++)
		{
			int l;
			if (pos<n && j==X[pos])
			{
				l=i;
				pos++;
			}
			else l=i+E->b;
			int k;
			for (k = 0; k < E->gen; k++)
			{
				Equation tmp=varEq(j);
				if (k!=0)
				{
					Equation tmp1=appliqueS(tmp,k-1);
					freeEq(tmp);
					tmp=tmp1;
				}
				Equation tmp1=mulEq(E->mat[E->gen*E->lignevar[j]+k][i],tmp);
				freeEq(tmp);
				tmp=addEq(tmp1,Eq[l]);
				freeEq(Eq[l]);
				freeEq(tmp1);
				Eq[l]=tmp;
			}
		}
	}
	return Eq;
}

void PermuterVar(int a, int b, SysEqLin E) // Permute les variables a et b dans E
{
	int la=E->lignevar[a];
	int lb=E->lignevar[b];
	int i;
	for (i = 0; i < E->gen; i++) echangeligne(E->gen*la+i,E->gen*lb+i,E->mat,E->gen*VARIABLES,E->b);
	E->lignevar[a]=lb;
	E->lignevar[b]=la;
	E->varligne[la]=b;
	E->varligne[lb]=a;
}

int ApparaitLineairement(int x, SysEqLin E) // Renvoi vrai si il exite f tel que toutes les équations de E sont de la forme alpha*f(x) = ...
{
	int l=E->lignevar[x]*E->gen;
	int dim=Dimension(E->mat+l,E->gen,E->b,1);
	switch (dim)
	{
		case 0:
			return 2;
			break;
		case 1:
			return 1;
			break;
		default:
			return 0;
			break;
	}
}

#if 0
void freeSysEq(SysEqLin E) // Libère la mémoire occupée par E
{
	int i;
	for (i = 0; i < VARIABLES*E->gen; i++) free(E->mat[i]);
	free(E->mat);
	if (E->initialise)
	{
		for (i = 0; i < E->a/E->gen; i++)
		{
			free(E->sousmat[E->varligne[i]]->A);
			free(E->sousmat[E->varligne[i]]->IA);
			free(E->sousmat[E->varligne[i]]->JA);
			free(E->sousmat[E->varligne[i]]);
		}
		free(E->sousmat);
	}
	free(E->lignevar);
	free(E->varligne);
	free(E);
}

#else

void freeSysEq(SysEqLin E) // Libère la mémoire occupée par E
{
	int i;
	for (i = 0; i < VARIABLES*E->gen; i++) free(E->mat[i]);
	free(E->mat);
	if (E->initialise)
	{
		for (i = 0; i < E->a/E->gen; i++)
		{
			freeMatriceCreuse(E->sousmat[E->varligne[i]]);
		}
		free(E->sousmat);
	}
	free(E->lignevar);
	free(E->varligne);
	free(E);
}

#endif

void PrintSysEqLinManual(SysEqLin E)
{
	int * __restrict X = malloc(E->a/E->gen*sizeof(int));
	int N;
	int choix;
	SysEqLin F;
	while (1)
	{
		printf("Système d'équations : \n");
		printf("	- Variables :");
		int i;
		for (i = 0; i < E->a/E->gen; i++) printf("%d ",E->varligne[i]);
		printf("\n\n");
		printf("Actions :\n");
		printf("	- Printr le système d'équations : 1\n");
		printf("	- Supposer des variables connues : 2\n");
		printf("	- Extraire un sous-système : 3\n");
		printf("	- Exit : 0\n");
		do
		{
			printf("\r Votre choix : ");
			scanf(" %d",&choix);
			getchar();
		} while (choix<0 || choix>3);
		switch (choix)
		{
			case 1: 
				PrintMat(E->mat,E->a,E->b);
				printf("\n Appuyez sur une touche pour continuer ...");
				getchar();
				break;
			case 2:
				N=0;
				while (1)
				{
					printf("\rNouvelle variable connue (0 pour stop) : ");
					scanf(" %d",X+N);
					getchar();
					if (X[N])
					{
						if (E->lignevar[X[N]]>=E->a/E->gen) printf("Impossible!!");
						else N++;
					}
					else break;
				}
				F = EsachantX(X,N,E);
				PrintSysEqLinManual(F);
				freeSysEq(F);
				break;	
			case 3:
				N=0;
				while (1)
				{
					printf("\rNouvelle variable (0 pour stop) : ");
					scanf(" %d",X+N);
					getchar();
					if (X[N])
					{
						if (E->lignevar[X[N]]>=E->a/E->gen) printf("Impossible!!");
						else N++;
					}
					else break;
				}
				F = ExtraireEX2(X,N,E);
				PrintSysEqLinManual(F);
				freeSysEq(F);
				break;	
			default:
				goto fin;
				break;	
		}
	}
	fin :
	free(X);
}
	
	
	
