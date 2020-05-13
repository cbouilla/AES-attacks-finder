#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Bazar.h"

int * UnionListesTriees(int * const L1, const int t1, int * const L2, const int t2, int *taille)
{
	int * __restrict Final=malloc((max(0,t1)+max(0,t2))*sizeof(int));
	int i=0, j=0, cpt=0;
	while (i<t1 && j<t2)
	{
		if (L1[i]<=L2[j])
		{
			if (L1[i]==L2[j]) j++;
			Final[cpt++]=L1[i++];
		}
		else Final[cpt++]=L2[j++];
	}
	for (i = i; i < t1; i++) Final[cpt++]=L1[i];
	for (j = j; j < t2; j++) Final[cpt++]=L2[j];
	*taille=cpt;
	return Final;
}

int UnionListesTriees2(int * const L1, const int t1, int * const L2, const int t2, int * const Final)
{
	int i=0, j=0, cpt=0;
	while (i<t1 && j<t2)
	{
		if (L1[i]<L2[j]) Final[cpt++]=L1[i++];
		else
		{
			if (L1[i]==L2[j]) i++;
			Final[cpt++]=L2[j++];
		}
	}
	for (i = i; i < t1; i++) Final[cpt++]=L1[i];
	for (j = j; j < t2; j++) Final[cpt++]=L2[j];
	return cpt;
}

int OrdreListe_tmp(int * const L1, const int t1, int * const L2, const int t2)
{
	if (t1==0)
	{
		if (t2==0) return 0;
		else return 1;
	}
	if (*L1<*L2) return 1;
	if (*L1>*L2) return -1;
	return OrdreListe_tmp(L1+1,t1-1,L2+1,t2-1);
}
	
int OrdreListe(int * const L1, const int t1, int * const L2, const int t2) /* retourne 0 si les listes sont egales, 1 si L1<L2 et -1 si L2<L1 */
{
	if (t1<=t2) return OrdreListe_tmp(L1,t1,L2,t2);
	else return -OrdreListe_tmp(L2,t2,L1,t1);
}
	

int * IntersectionListesTriees(int * const L1, const int t1, int * const L2, const int t2, int *taille)
{
	int * __restrict Final=malloc(max(min(t1,t2),0)*sizeof(int));
	int i=0, j=0, cpt=0;
	while (i<t1 && j<t2)
	{
		if (L1[i]<L2[j]) i++;
		else
		{
			if (L1[i]==L2[j]) Final[cpt++]=L1[i++];
			j++;
		}
	}
	*taille=cpt;
	return Final;
}

int * DifferenceListesTriees(int * const L1, const int t1, int * const L2, const int t2, int *taille) /* L1 - L2 */
{
	int * __restrict Final=malloc(max(t1,0)*sizeof(int));
	int i=0, j=0, cpt=0;
	while (i<t1 && j<t2)
	{
		if (L1[i]<L2[j]) Final[cpt++]=L1[i++];
		else
		{
			if (L1[i]==L2[j]) i++;
			j++;
		}
	}
	for (j = i; j < t1; j++) Final[cpt++]=L1[j];
	*taille=cpt;
	return Final;
}

int SearchElement(const int a, int * __restrict const L, const int N)
{
	int debut = 0, fin = N;
	while (debut<fin)
	{
		const int pivot=(debut+fin)/2;
		if (L[pivot]==a) return pivot;
		if (L[pivot]>a) fin=pivot;
		else debut=pivot+1;
	}
	return -1;
}

int compare_ints( const void* a, const void* b )
{
     int* arg1 = (int*) a;
     int* arg2 = (int*) b;
     if( *arg1 < *arg2 ) return -1;
     else if( *arg1 == *arg2 ) return 0;
     else return 1;
}

void Tri_tmp(int *L1, int taille, int *L)
{
	int N1=taille/2;
	if (N1>2) Tri_tmp(L1,N1,L);
	if (taille-N1>2) Tri_tmp(L1+N1,taille-N1,L);
	int i;
	for (i = 0; i < N1; i++) L[i]=L1[i];
	i=0;
	int j=N1, cpt=0;
	while (i<N1 && j<taille)
	{
		if (L[i]<L1[j]) L1[cpt++]=L[i++];
		else
		{
			if (L[i]>L1[j]) L1[cpt++]=L1[j++];
			else L1[cpt++]=L[i++];
		}
	}
	for (j = i; j < N1; j++) L1[cpt++]=L[j];
}

void Tri(int *L1, int taille)
{
	if (taille>=2)
	{
		/*int *L;
		L=malloc((taille/2)*sizeof(int));
		Tri_tmp(L1,taille,L);
		free(L);*/
		qsort(L1,taille,sizeof(int),compare_ints);
	}
}

void PrintListe(int *L, int taille)
{
	int i;
	for (i = 0; i < taille; i++) printf("%d ",L[i]);
	printf("\n");
}

int *Liste_Globale;

int compare_ints_special(const void* a, const void *b)
{
     int* arg1 = (int*) a;
     int* arg2 = (int*) b;
     if( Liste_Globale[*arg1] < Liste_Globale[*arg2] ) return -1;
     else if( Liste_Globale[*arg1] == Liste_Globale[*arg2] ) return 0;
     else return 1;
}

void TriSpecial_tmp(int *L1, int *L2, int taille, int *L)
{
	int N1=taille/2;
	if (N1>2) TriSpecial_tmp(L1,L2,N1,L);
	if (taille-N1>2) TriSpecial_tmp(L1+N1,L2,taille-N1,L);
	int i;
	for (i = 0; i < N1; i++) L[i]=L1[i];
	i=0;
	int j=N1, cpt=0;
	while (i<N1 && j<taille)
	{
		if (L2[L[i]]<L2[L1[j]]) L1[cpt++]=L[i++];
		else
		{
			if (L2[L[i]]>L2[L1[j]]) L1[cpt++]=L1[j++];
			else L1[cpt++]=L[i++];
		}
	}
	for (j = i; j < N1; j++) L1[cpt++]=L[j];	
}

void TriSpecial(int *L1, int *L2, int taille)
{
	if (taille>=2)
	{
		/*int *L;
		L=malloc((taille/2)*sizeof(int));
		TriSpecial_tmp(L1,L2,taille,L);
		free(L);*/
		Liste_Globale=L2;
		qsort(L1,taille,sizeof(int),compare_ints_special);
	}
}


#if 0
int ContenuListesTriees(int * const L1, const int t1, int * const L2, const int t2) /* L1 inclus dans L2 ? */
{
	if (t1==0) return 1;
	if (t1>t2) return 0;
	if (L1[0]<L2[0]) return 0;
	if (L1[0]==L2[0]) return ContenuListesTriees2(L1+1,t1-1,L2+1,t2-1);
	else return ContenuListesTriees2(L1,t1,L2+1,t2-1);
}

#else

int ContenuListesTriees_tmp2(int *L1, int t1, int *L2, int t2, int *L3, int t3, int *L4, int t4);

int ContenuListesTriees_tmp(int * const L1, const int t1, int * const L2, const int t2)
{
	if (t1!=t2)
	{
		const int i=t1 >> 1;
		const int j=SearchElement(L1[i],L2,t2);
		if (j==-1) return 0;
		else
		{
			const int t3=i+1;
			const int t4=j+1;
			return ContenuListesTriees_tmp2(L1,i,L2,j,L1+t3,t1-t3,L2+t4,t2-t4);
		}
	}
	else
	{
		int i;
		for (i = 0; i < t1; i++) if (L1[i]!=L2[i]) return 0;
		return 1;
	}
}

int ContenuListesTriees_tmp2(int * const L1, const int t1, int * const L2, const int t2, int * const L3, const int t3, int * const L4, const int t4)
{
	if (t1<=0) return ContenuListesTriees(L3,t3,L4,t4);
	if (t3<=0) return ContenuListesTriees(L1,t1,L2,t2);
	if (t1<=t2 && t3<=t4) return (ContenuListesTriees_tmp(L1,t1,L2,t2) && ContenuListesTriees_tmp(L3,t3,L4,t4));
	else return 0;
}

inline int ContenuListesTriees(int * const L1, const int t1, int * const L2, const int t2) /* L1 inclus dans L2 ? */
{
	return (t1<=0 || (t1<=t2 && ContenuListesTriees_tmp(L1,t1,L2,t2)));
}

#endif

int IntersectionNonVideListeTriees(int * const L1, const int t1, int * const L2, const int t2)
{
	int i=0,j=0;
	while (i<t1 && j<t2)
	{
		if (L1[i]==L2[j]) return 1;
		else
		{
			if (L1[i]<L2[j]) i++;
			else j++;
		}
	}
	return 0;
}

int Puissance(const int a, const int b) /* a puissance b */
{
	if (b==0) return 1;
	int c=Puissance(a,b/2);
	if (b & 1) return c*c;
	else return c*c*a;
}

#if 0
char *strdup(const char *str)
{
	char *cpy = NULL;
	if (str)
	{
		cpy = malloc(strlen(str)+1);
		if (cpy)
		strcpy(cpy, str);
	}
	return cpy;
}
#endif

int NextSet(int * X, int l, int n)
{
	int i;
	for (i = 1; i <= l; i++)
	{
		if (X[l-i]!=n-i)
		{
			int j;
			int add = ++X[l-i];
			for (j = l-i+1; j < l; j++) X[j] = ++add;
			return 1;
		}
	}
			
	for (i = 0; i < l; i++) X[i]=i;
	return 0;
}
	
	


