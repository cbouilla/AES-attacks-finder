#include <stdio.h>
#include <stdlib.h>
#include "Algebre.h"
#include "Bazar.h"
#include "CorpsK.h"


/*
 * Les matrices ci-dessous seront souvent considérer comme des listes de vecteurs colonnes.
 * On peut donc parler de l'intersection et du supplémentaire.
 * Attention : les matrices doivent impérativement être des listes de vecteurs lignes.
 */ 

void echangeligne(int i, int j, unsigned char **mat, int a, int b) // Echange les lignes i et j de la matrice mat
{
	if (i<a && j<a)
	{
		unsigned char *tmp1=mat[i];
		mat[i]=mat[j];
		mat[j]=tmp1;
	}
}

void echangecolonne(int i, int j, unsigned char **mat, int a, int b) // Echange les colonnes i et j de la matrice mat
{
	if (i<b && j<b)
	{
		int k;
		for (k = 0; k < a; k++)
		{
			unsigned char tmp=mat[k][i];
			mat[k][i]=mat[k][j];
			mat[k][j]=tmp;
		}
	}
}

void PrintMat(unsigned char **mat, const int a, const int b) // Print la matrice mat de taille a x b 
{
	int i;
	for (i = 0; i < a; i++)
	{
		int j;
		for (j = 0; j < b; j++) printf("%02x ",mat[i][j]);
		printf("\n");
	}
	printf("\n");
}

int DimensionBorne_tmp(unsigned char ** __restrict m, const int a, const int b, const int borne) // calcul le rang de la matrice mat de taille a x b, ie la dimension de l'espace engendré par les b vecteurs colonnes.
{
	//PrintMat(m,a,b);
	//getchar();
	int dim = 0;
	const int dim_max = min(a-1,borne);
	int pivot_c;
	for (pivot_c = 0; pivot_c < b; pivot_c++)
	{
		int new_pivot_l;
		for (new_pivot_l = dim; new_pivot_l < a; new_pivot_l++) // on cherche un pivot sur la colonne
		{
			if (m[new_pivot_l][pivot_c]) // on en a trouvé un
			{
				if (dim!=dim_max)
				{
					if (new_pivot_l!=dim) // on echange les lignes
					{
						unsigned char * __restrict const tmp = m[dim];
						m[dim]=m[new_pivot_l];
						m[new_pivot_l]=tmp;
					}
							
					const unsigned char * const __restrict ligne_pivot = m[dim];
					int ligne;
					for (ligne = new_pivot_l+1; ligne < a; ligne++) // on fait le pivot de Gauss
					{
						if (m[ligne][pivot_c])
						{
							unsigned char * __restrict ligne_need_update = m[ligne];
							const unsigned char coeff = Multiply(Inverse(ligne_pivot[pivot_c]),ligne_need_update[pivot_c]);
							int colonne;
							for (colonne = pivot_c+1; colonne < b; colonne++)
							{
								//m[ligne][colonne]=Multiply(m[dim][pivot_c],m[ligne][colonne]) ^ Multiply(m[ligne][pivot_c],m[dim][colonne]);
								ligne_need_update[colonne]^=Multiply(coeff,ligne_pivot[colonne]);
							}
						}
					}
					dim++;
					break;
				}
				else return dim+1;
			}
		}
	}
	return dim;	
}

int Dimension_tmp(unsigned char ** __restrict m, const int a, const int b) // calcul le rang de la matrice mat de taille a x b, ie la dimension de l'espace engendré par les b vecteurs colonnes.
{
	int dim = 0;
	int pivot_c;
	for (pivot_c = 0; pivot_c < b; pivot_c++)
	{
		int new_pivot_l;
		for (new_pivot_l = dim; new_pivot_l < a; new_pivot_l++)
		{
			if (m[new_pivot_l][pivot_c])
			{
				if (new_pivot_l!=dim)
				{
					unsigned char * __restrict const tmp = m[dim];
					m[dim]=m[new_pivot_l];
					m[new_pivot_l]=tmp;
				}

				const unsigned char * __restrict const ligne_pivot = m[dim];
				int ligne;
				for (ligne = new_pivot_l+1; ligne < a; ligne++) // on fait le pivot de Gauss
				{
					if (m[ligne][pivot_c])
					{
						int colonne;
						unsigned char * __restrict ligne_need_update = m[ligne];
						const unsigned char coeff = Multiply(Inverse(ligne_pivot[pivot_c]),ligne_need_update[pivot_c]);
						for (colonne = pivot_c+1; colonne < b; colonne++)
						{
							//m[ligne][colonne]=Multiply(m[dim][pivot_c],m[ligne][colonne]) ^ Multiply(m[ligne][pivot_c],m[dim][colonne]);
							ligne_need_update[colonne]^=Multiply(coeff,ligne_pivot[colonne]);
						}
					}
				}
				dim++;
				if (dim != a) break;
				else return dim;
			}
		}
	}
	return dim;	
}

int Dimension(unsigned char ** __restrict mat, const int a, const int b, const int copy) // calcul le rang de la matrice mat de taille a x b, ie la dimension de l'espace engendré par les b vecteurs colonnes.
{
	if (!copy) return Dimension_tmp(mat,a,b); // Attention : la matrice mat sera modifiée, aucune propriété conservée !!
	else
	{
		unsigned char ** __restrict m=malloc(a*sizeof(unsigned char *));
		unsigned char * __restrict espace=malloc(a*b*sizeof(unsigned char));
		int i;
		for (i = 0; i < a; i++)
		{
			m[i]=espace+i*b;
			int j;
			for (j = 0; j < b; j++) m[i][j]=mat[i][j];
		}
		int dim = Dimension_tmp(m,a,b);
		free(m);
		free(espace);
		return dim;
	}
}


int Base(unsigned char ** __restrict dst, unsigned char ** __restrict const mat, const int a, const int b) // Met dans dst une famille libre contenue dans la liste de vecteurs mat. Renvoi le nombre de colone de dst.
{
	if (b>0)
	{
		unsigned char ** __restrict m=malloc(a*sizeof(unsigned char *));
		unsigned char * __restrict espace=malloc(a*b*sizeof(unsigned char));
		
		{
			int i;
			for (i = 0; i < a; i++)
			{
				m[i]=espace+i*b;
				int j;
				for (j = 0; j < b; j++) m[i][j]=mat[i][j];
			}
		}
		
		int dim = 0;
		int pivot_c;
		for (pivot_c = 0; pivot_c < b; pivot_c++)
		{
			int new_pivot_l;
			for (new_pivot_l = dim; new_pivot_l < a; new_pivot_l++)
			{
				if (m[new_pivot_l][pivot_c])
				{
					if (new_pivot_l!=dim)
					{
						unsigned char *tmp = m[dim];
						m[dim]=m[new_pivot_l];
						m[new_pivot_l]=tmp;
					}

					int ligne;
					unsigned char * __restrict ligne_pivot = m[dim];
					const unsigned char invp = Inverse(ligne_pivot[pivot_c]);
					for (ligne = new_pivot_l+1; ligne < a; ligne++) // on fait le pivot de Gauss
					{
						if (m[ligne][pivot_c])
						{
							int colonne;
							unsigned char * __restrict ligne_need_update = m[ligne];
							const unsigned char coeff = Multiply(invp,ligne_need_update[pivot_c]);
							for (colonne = pivot_c+1; colonne < b; colonne++)
							{
								//m[ligne][colonne]=Multiply(m[dim][pivot_c],m[ligne][colonne]) ^ Multiply(m[ligne][pivot_c],m[dim][colonne]);
								ligne_need_update[colonne]^=Multiply(coeff,ligne_pivot[colonne]);
							}
						}
					}
					for (ligne = 0; ligne < a; ligne++) dst[ligne][dim]=mat[ligne][pivot_c];
					dim++;
					if (dim != a) break;
					else
					{
						free(espace);
						free(m);
						return dim;
					}
				}
			}
		}
		free(espace);
		free(m);
		return dim;	
	}
	else return 0;
}

/*
 * Prend en compte que les équations entre les connues sont satisfaites.
 */
int BaseSpeciale(unsigned char **dst, unsigned char **mat, int a, int b, int Known)
{
	if (b==0) return 0;
	if (Known==0) return 0;
	unsigned char **m=malloc(a*sizeof(unsigned char *));
	int i;
	for (i = 0; i < a; i++)
	{
		m[i]=malloc(b*sizeof(unsigned char));
		int j;
		for (j = 0; j < b; j++) m[i][j]=mat[i][j];
	}
	for (i = 0; i < min(Known,b); i++)
	{
		int k1;
		for (k1 = i; k1 < b; k1++)
		{
			int k2;
			for (k2 = i; k2 < Known; k2++)
			{
				if (m[k2][k1]!=0)
				{
					if (k2!=i) echangeligne(k2,i,m,a,b);
					if (k1!=i)
					{
						echangecolonne(k1,i,m,a,b);
						echangecolonne(k1,i,mat,a,b);
					}
					break;
				}
			}
			if (m[i][i]!=0) break;
		}
		unsigned char p=m[i][i];
		if (p==0) break;
		else
		{
			int j;
			for (j = i+1; j < a; j++)
			{
				unsigned char c=m[j][i];
				if (c!=0)
				{
					int k;
					for (k = i; k < b; k++) m[j][k]=Multiply(p,m[j][k])^Multiply(c,m[i][k]);
				}
			}
		}	
	}
	int tmp=i;
	for (i = 0; i < tmp; i++)
	{
		int j;
		for (j = 0; j < a; j++) dst[j][i]=mat[j][i];
	}
	for (i = 0; i < a; i++) free(m[i]);
	free(m);
	return tmp;
}

int Somme(unsigned char **dst, unsigned char **mat1, int b1, unsigned char **mat2, int a, int b2) // Renvoie une base de <mat1> + <mat2>
{
	unsigned char ** __restrict m=malloc(a*sizeof(unsigned char *));
	int b=b1+b2, i;
	for (i = 0; i < a; i++)
	{
		m[i]=malloc(b*sizeof(unsigned char));
		int j;
		for (j = 0; j < b1; j++) m[i][j]=mat1[i][j];
		for (j = 0; j < b2; j++) m[i][j+b1]=mat2[i][j];	
	}
	b=Base(dst,m,a,b);
	for (i = 0; i < a; i++) free(m[i]);
	free(m);
	return b;
}


int Supplementaire_tmp(unsigned char **dst, unsigned char **mat1, int b1, unsigned char **mat2, int a, int b2)
{
	if (b2==0) return 0;
	if (b1==0)
	{
		int i,j;
		for (i = 0; i < a; i++) for ( j = 0; j < b2; j++) dst[i][j]=mat2[i][j];
		return b2;
	}
	int b=b1+b2,i;
	unsigned char **m=malloc(a*sizeof(unsigned char *));
	for (i = 0; i < a; i++) m[i]=malloc(b*sizeof(unsigned char)); 
	for (i = 0; i < a; i++)
	{
		int j;
		for (j = 0; j < b1; j++) m[i][j]=mat1[i][j];
		for (j = 0; j < b2; j++) m[i][j+b1]=mat2[i][j];	
	}
	for (i = 0; i < min(a,b1); i++)
	{
		int k1;
		for (k1 = i; k1 < b1; k1++)
		{
			int k2;
			for (k2 = i; k2 < a; k2++)
			{
				if (m[k2][k1]!=0)
				{
					if (k2!=i) echangeligne(k2,i,m,a,b);
					if (k1!=i) echangecolonne(k1,i,m,a,b);
					break;
				}
			}
			if (m[i][i]!=0) break;
		}
		unsigned char p=m[i][i];
		if (p==0) break;
		else
		{
			int j;
			for (j = i+1; j < a; j++)
			{
				unsigned char c=m[j][i];
				if (c!=0)
				{
					int k;
					for (k = i; k < b; k++) m[j][k]=Multiply(p,m[j][k])^Multiply(c,m[i][k]);
				}
			}
		}	
	}
	int b3=b2-b1;
	for (i = b1; i < min(a,b); i++)
	{
		int k1;
		for (k1 = i; k1 < b; k1++)
		{
			int k2;
			for (k2 = i; k2 < a; k2++)
			{
				if (m[k2][k1]!=0)
				{
					if (k2!=i) echangeligne(k2,i,m,a,b);
					if (k1!=i)
					{
						echangecolonne(k1,i,m,a,b);
						echangecolonne(k1-b1,i-b1,mat2,a,b2);
					}
					break;
				}
			}
			if (m[i][i]!=0) break;
		}
		unsigned char p=m[i][i];
		if (p==0) break;
		else
		{
			int j;
			for (j = i+1; j < a; j++)
			{
				unsigned char c=m[j][i];
				if (c!=0)
				{
					int k;
					for (k = i; k < b; k++) m[j][k]=Multiply(p,m[j][k])^Multiply(c,m[i][k]);
				}
			}
		}	
	}
	for (i = 0; i < b3; i++)
	{
		int j;
		for (j = 0; j < a; j++) dst[j][i]=mat2[j][i];
	}
	for (i = 0; i < a; i++) free(m[i]);
	free(m);
	return b3;
}

int Supplementaire(unsigned char **dst, unsigned char **mat1, int b1, unsigned char **mat2, int a, int b2) // Renvoie une base d'un supplémentaire de <mat1> dans <mat2>
{
	#if 0 // A activer si problème
	unsigned char **m1,**m2,**m3;
	int i,b3,b;
	m1=malloc(a*sizeof(unsigned char *));
	for (i = 0; i < a; i++) m1[i]=malloc(b1*sizeof(unsigned char));
	m2=malloc(a*sizeof(unsigned char *));
	for (i = 0; i < a; i++) m2[i]=malloc(b2*sizeof(unsigned char)); 
	b1=Base(m1,mat1,a,b1);
	b2=Base(m2,mat2,a,b2);
	m3=malloc(a*sizeof(unsigned char *));
	for (i = 0; i < a; i++) m3[i]=malloc(max(b1,b2)*sizeof(unsigned char));
	b3=Supplementaire_tmp(m3,m1,b1,m2,a,b2);
	b=Base(dst,m3,a,b3);
	for (i = 0; i < a; i++) free(m1[i]);
	free(m1);
	for (i = 0; i < a; i++) free(m2[i]);
	free(m2);
	for (i = 0; i < a; i++) free(m3[i]);
	free(m3);
	return b;
	#else
	unsigned char ** __restrict m3=malloc(a*sizeof(unsigned char *));
	int i;
	for (i = 0; i < a; i++) m3[i]=malloc(max(b1,b2)*sizeof(unsigned char));
	int b3=Supplementaire_tmp(m3,mat1,b1,mat2,a,b2);
	int b=Base(dst,m3,a,b3);
	for (i = 0; i < a; i++) free(m3[i]);
	free(m3);
	return b;
	#endif
}

int Intersection_tmp(unsigned char **dst, unsigned char **mat1, int b1, unsigned char **mat2, int a, int b2)
{
	/* A améliorer */
	if (b1==0 || b2==0) return 0;
	int b=b1+b2,i;
	unsigned char **m=malloc(a*sizeof(unsigned char *));
	for (i = 0; i < a; i++) m[i]=malloc(b*sizeof(unsigned char)); 
	for (i = 0; i < a; i++)
	{
		int j;
		for (j = 0; j < b1; j++) m[i][j]=mat1[i][j];
		for (j = 0; j < b2; j++) m[i][j+b1]=mat2[i][j];	
	}
	for (i = 0; i < min(a,b1); i++)
	{
		int k1;
		for (k1 = i; k1 < b1; k1++)
		{
			int k2;
			for (k2 = i; k2 < a; k2++)
			{
				if (m[k2][k1])
				{
					if (k2!=i) echangeligne(k2,i,m,a,b);
					if (k1!=i)
					{
						echangecolonne(k1,i,m,a,b);
						echangecolonne(k1,i,mat1,a,b);
					}
					break;
				}
			}
			if (m[i][i]) break;
		}
		if (!m[i][i]) break;
		else
		{
			unsigned char invp=Inverse(m[i][i]);
			int j;
			for (j = i; j < b; j++) m[i][j]=Multiply(invp,m[i][j]);
			for (j = i+1; j < a; j++)
			{
				if (m[j][i])
				{
					int k;
					for (k = i+1; k < b; k++) if (m[i][k]) m[j][k]^=Multiply(m[j][i],m[i][k]);
					m[j][i]=0;
				}
			}
		}	
	}
	for (i = b1; i < min(a,b); i++)
	{
		int k1;
		for (k1 = i; k1 < b; k1++)
		{
			int k2;
			for (k2 = i; k2 < a; k2++)
			{
				if (m[k2][k1])
				{
					if (k2!=i) echangeligne(k2,i,m,a,b);
					if (k1!=i) echangecolonne(k1,i,m,a,b);
					break;
				}
			}
			if (m[i][i]) break;
		}
		if (!m[i][i]) break;
		else
		{
			unsigned char invp=Inverse(m[i][i]);
			int j;
			for (j = i; j < b; j++) m[i][j]=Multiply(invp,m[i][j]);
			for (j = i+1; j < a; j++)
			{
				if (m[j][i])
				{
					int k;
					for (k = i+1; k < b; k++) if (m[i][k]) m[j][k]^=Multiply(m[j][i],m[i][k]);
					m[j][i]=0;
				}
			}
		}	
	}
	int b3=b-i,j;
	unsigned char *coef=malloc(b*sizeof(unsigned char));
	for (i = 0; i < b3; i++) for (j = 0; j < a; j++) dst[j][i]=0;	
	for (i = 0; i < b3; i++)
	{
		for (j = 0; j < b; j++) coef[j]=0;
		coef[b-1-i]=1;
		for (j = b-b3-1; j >=0; j--)
		{
			int k;
			for (k = j+1; k < b; k++) if (coef[k] && m[j][k]) coef[j]^=Multiply(coef[k],m[j][k]); // 34 % du temps passé ici
		}
		for (j = 0; j < b1; j++)
		{
			int k;
			for (k = 0; k < a; k++) if (coef[j] && mat1[k][j]) dst[k][i]^=Multiply(coef[j],mat1[k][j]); // 19 % du temps passé ici
		}
	}
	for (i = 0; i < a; i++) free(m[i]);
	free(m);
	free(coef);
	return b3;
}

int Intersection(unsigned char **dst, unsigned char **mat1, int b1, unsigned char **mat2, int a, int b2) // Renvoie une base de <mat1> n <mat2>
{
	#if 0 // A activer si problème
	unsigned char **m1,**m2,**m3;
	int i,b3,b;
	m1=malloc(a*sizeof(unsigned char *));
	for (i = 0; i < a; i++) m1[i]=malloc(b1*sizeof(unsigned char));
	m2=malloc(a*sizeof(unsigned char *));
	for (i = 0; i < a; i++) m2[i]=malloc(b2*sizeof(unsigned char)); 
	b1=Base(m1,mat1,a,b1);
	b2=Base(m2,mat2,a,b2);
	m3=malloc(a*sizeof(unsigned char *));
	for (i = 0; i < a; i++) m3[i]=malloc(max(b1,b2)*sizeof(unsigned char));
	b3=Intersection_tmp(m3,m1,b1,m2,a,b2);
	b=Base(dst,m3,a,b3);
	for (i = 0; i < a; i++) free(m1[i]);
	free(m1);
	for (i = 0; i < a; i++) free(m2[i]);
	free(m2);
	for (i = 0; i < a; i++) free(m3[i]);
	free(m3);
	return b;
	#else
	unsigned char **m3=malloc(a*sizeof(unsigned char *));
	int i;
	for (i = 0; i < a; i++) m3[i]=malloc(max(b1,b2)*sizeof(unsigned char));
	int b3=Intersection_tmp(m3,mat1,b1,mat2,a,b2);
	int b=Base(dst,m3,a,b3);
	for (i = 0; i < a; i++) free(m3[i]);
	free(m3);
	return b;
	#endif
	
}
