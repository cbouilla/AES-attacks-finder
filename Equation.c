#include <stdlib.h>
#include <stdio.h>
#include "Equation.h"
#include "parser_machinery.h"
#include "Bazar.h"
#include "CorpsK.h"

extern int VARIABLES;

void PrintEquationWithNames(Equation a, string_list_t *names)
{
  int i;
  for (i = 0; i < a->nV; i++)
    {
      if (i!=0) printf(" + ");
      
      if (a->var[i] == 0) 
	printf("%02x",a->coefV[i]);
      else {
	if (a->coefV[i] != 0x01) printf("%02x*",a->coefV[i]);
	printf("%s", find_var_name(names, a->var[i]));
      }
    }
  if (a->nV!=0 && a->nS!=0) printf(" + ");

  for (i = 0; i < a->nS; i++)
    {
      if (i!=0) printf(" + ");
      if (a->coefS[i] != 0x01) printf("%02x*",a->coefS[i]);
      if (a->sbox[i]) printf("R(");
      else printf("S(");
      PrintEquationWithNames(a->S[i], names);
      printf(")");
    }
}

void PrintEq(Equation a)
{
  int i;
  for (i = 0; i < a->nV; i++)
    {
      if (i!=0) printf(" + ");
      
      if (a->coefV[i] != 0x01) printf("%02x*",a->coefV[i]);
      printf("x[%u]",a->var[i]);
      /*if (a->coefV[i]!=0x01) printf(")");*/
    }
  if (a->nV!=0 && a->nS!=0) printf(" + ");
  for (i = 0; i < a->nS; i++)
    {
      if (i!=0) printf(" + ");
      if (a->coefS[i]!=0x01) printf("%02x*",a->coefS[i]);
      if (a->sbox[i]) printf("R(");
      else printf("S(");
      PrintEq(a->S[i]);
      printf(")");
      if (a->coefS[i]!=0x01) printf(")");
    }
}

Equation NulEq(void)
{
	Equation p=malloc(sizeof(EQUATION));
	p->nV=0;
	p->nS=0;
	p->var=NULL;
	p->coefV=NULL;
	p->coefS=NULL;
	p->sbox=NULL;
	p->S=NULL;
	return p;
}

inline int EstNulEq(Equation a)
{
	return (a->nS==0 && a->nV==0);
}

void freeEq(Equation a)
{
	int i;
	if (a!=NULL)
	{
		if (a->nV!=0)
		{
			free(a->var);
			free(a->coefV);
		}
		if (a->nS!=0)
		{
			for (i = 0; i < a->nS; i++) freeEq(a->S[i]);
			free(a->coefS);
			free(a->sbox);
			free(a->S);
		}
		free(a);
		a=NULL;
	}
}

Equation copyEq(Equation a)
{
	Equation p=malloc(sizeof(EQUATION));
	p->nS=a->nS;
	if (p->nS!=0)
	{
		p->S=malloc(p->nS*sizeof(Equation));
		p->sbox=malloc(p->nS*sizeof(unsigned char));
		p->coefS=malloc(p->nS*sizeof(unsigned char));
		int i;
		for (i = 0; i < p->nS; i++)
		{
			p->S[i]=copyEq(a->S[i]);
			p->coefS[i]=a->coefS[i];
			p->sbox[i]=a->sbox[i];
		}
	}
	else
	{
		p->S=NULL;
		p->sbox=NULL;
		p->coefS=NULL;
	}
	p->nV=a->nV;
	if (p->nV!=0)
	{
		p->var=malloc(p->nV*sizeof(int));
		p->coefV=malloc(p->nV*sizeof(unsigned char));
		int i;
		for (i = 0; i < p->nV; i++)
		{
			p->var[i]=a->var[i];
			p->coefV[i]=a->coefV[i];
		}
	}
	else
	{
		p->var=NULL;
		p->coefV=NULL;
	}
	return p;
}

int EgaleEq(Equation a, Equation b)
{
	int i,j,res;
	if (a->nV==b->nV && a->nS==b->nS)
	{
		for (i = 0; i < a->nV; i++)
		{
			res=0;
			for (j = 0; j < b->nV; j++)
			{
				if (a->var[i]==b->var[j])
				{
					res=(a->coefV[i]==b->coefV[j]);
					break;
				} 
			}
			if (!res) return 0;
		}
		for (i = 0; i < a->nS; i++)
		{
			res=0;
			for (j = 0; j < b->nS; j++)
			{
				if (a->sbox[i]==b->sbox[j] && EgaleEq(a->S[i],b->S[j]))
				{
					res=(a->coefS[i]==b->coefS[j]);
					break;
				}
			}
			if (!res) return 0;
		}
		return 1;
	}
	else return 0;
}

Equation varEq(int i)
{
	Equation p=malloc(sizeof(EQUATION));
	if (p==NULL)
	{
		printf("ERROR!!");
		getchar();
	}
	p->nV=1;
	p->var=malloc(sizeof(int));
	if (p->var==NULL)
	{
		printf("ERROR!!");
		getchar();
	}
	p->coefV=malloc(sizeof(unsigned char));
	if (p->coefV==NULL)
	{
		printf("ERROR!!");
		getchar();
	}
	p->var[0]=i;
	p->coefV[0]=0x01;
	p->nS=0;
	p->S=NULL;
	p->coefS=NULL;
	p->sbox=NULL;
	return p;
}

Equation mulEq(unsigned char m, Equation a)
{
	if (m==0) return NulEq();
	Equation p=malloc(sizeof(EQUATION));
	p->nV=a->nV;
	p->nS=a->nS;
	if (p->nV>0)
	{
		p->var=malloc(p->nV*sizeof(int));
		p->coefV=malloc(p->nV*sizeof(unsigned char));
		int i;
		for (i = 0; i < p->nV; i++)
		{
			p->var[i]=a->var[i];
			p->coefV[i]=Multiply(a->coefV[i],m);
		}
	}
	else
	{
		p->var=NULL;
		p->coefV=NULL;
	}
	if (p->nS>0)
	{
		p->S=malloc(p->nS*sizeof(Equation));
		p->sbox=malloc(p->nS*sizeof(unsigned char));
		p->coefS=malloc(p->nS*sizeof(unsigned char));
		int i;
		for (i = 0; i < p->nS; i++)
		{
			p->sbox[i]=a->sbox[i];
			p->coefS[i]=Multiply(a->coefS[i],m);
			p->S[i]=copyEq(a->S[i]);
		}
	}
	else
	{
		p->S=NULL;
		p->coefS=NULL;
		p->sbox=NULL;
	}
	return p;
}

Equation reduitEq(Equation a)
{	
	Equation p=NulEq();
	if (a->nV>0)
	{
		unsigned char *ajout=malloc(a->nV*sizeof(unsigned char));
		p->var=malloc(a->nV*sizeof(int));
		p->coefV=malloc(a->nV*sizeof(unsigned char));
		int i;
		for (i = 0; i < a->nV; i++) ajout[i]=1;
		for (i = 0; i < a->nV; i++)
		{
			if (ajout[i])
			{
				unsigned char coeff=a->coefV[i];
				int j;
				for (j = i+1; j < a->nV; j++)
				{
					if (a->var[j]==a->var[i])
					{
						ajout[j]=0;
						coeff^=a->coefV[j];
					}
				}
				if (coeff!=0)
				{
					p->var[p->nV]=a->var[i];
					p->coefV[p->nV]=coeff;
					p->nV++;
				}
			}
		}
		free(ajout);
		if (p->nV==0)
		{
			free(p->var);
			free(p->coefV);
		}
	}
	
	if (a->nS>0)
	{
		unsigned char *ajout=malloc(a->nS*sizeof(unsigned char));
		p->S=malloc(a->nS*sizeof(Equation));
		p->coefS=malloc(a->nS*sizeof(unsigned char));
		p->sbox=malloc(a->nS*sizeof(unsigned char));
		Equation *red=malloc(a->nS*sizeof(Equation));
		int i;
		for (i = 0; i < a->nS; i++)
		{
			red[i]=reduitEq(a->S[i]);
			ajout[i]=1;
		}
		for (i = 0; i < a->nS; i++)
		{
			if (ajout[i])
			{
				unsigned char coeff=a->coefS[i];
				int j;
				for (j = i+1; j < a->nS; j++)
				{
					if (a->sbox[j]==a->sbox[i] && EgaleEq(red[i],red[j]))
					{
						ajout[j]=0;
						coeff^=a->coefS[j];
					}
				}
				if (coeff!=0)
				{
					p->coefS[p->nS]=coeff;
					p->sbox[p->nS]=a->sbox[i];
					p->S[p->nS]=copyEq(red[i]);
					p->nS++;
				}
			}
		}
		free(ajout);
		for (i = 0; i < a->nS; i++) freeEq(red[i]);
		free(red);
		if (p->nS==0)
		{
			free(p->S);
			free(p->sbox);
			free(p->coefS);
		}
	}
	
	if (p->nS==0) return p;
	else
	{
		Equation pp=malloc(sizeof(EQUATION));
		pp->nV=p->nV;
		if (p->nV>0)
		{
			pp->var=malloc(p->nV*sizeof(int));
			pp->coefV=malloc(p->nV*sizeof(unsigned char));
			int i;
			for (i = 0; i < p->nV; i++)
			{
				pp->var[i]=p->var[i];
				pp->coefV[i]=p->coefV[i];
			}
		}
		else
		{
			pp->var=NULL;
			pp->coefV=NULL;
		}
		pp->nS=0;
		pp->S=malloc(p->nS*sizeof(Equation));
		pp->sbox=malloc(p->nS*sizeof(unsigned char));
		pp->coefS=malloc(p->nS*sizeof(unsigned char));
		Equation tmp=NulEq();
		int i;
		for (i = 0; i < p->nS; i++)
		{
			int res=(p->S[i]->nS==1 && p->S[i]->nV==0);
			if (res) res=(p->S[i]->coefS[0]==0x01 && p->S[i]->sbox[0]!=p->sbox[i]);
			if (res)
			{
				Equation tmp1=mulEq(p->coefS[i],p->S[i]->S[0]);
				Equation tmp2=addEq(tmp1,tmp);
				freeEq(tmp);
				freeEq(tmp1);
				tmp=tmp2;
			}
			else
			{
				pp->S[pp->nS]=copyEq(p->S[i]);
				pp->coefS[pp->nS]=p->coefS[i];
				pp->sbox[pp->nS]=p->sbox[i];
				pp->nS++;
			}
		}
		if (pp->nS==0)
		{
			free(pp->S);
			free(pp->sbox);
			free(pp->coefS);
		}
		freeEq(p);
		if (EstNulEq(tmp))
		{
			freeEq(tmp);
			return pp;
		}
		else
		{
			p=addEq(tmp,pp);
			freeEq(tmp);
			freeEq(pp);
			return p;
		}
	}
}

Equation addEq(Equation a, Equation b)
{	
	Equation p=malloc(sizeof(EQUATION));
	p->nV=a->nV+b->nV;
	if (p->nV!=0)
	{
		p->var=malloc(p->nV*sizeof(int));
		p->coefV=malloc(p->nV*sizeof(unsigned char));
		int i;
		for (i = 0; i < a->nV; i++)
		{
			p->var[i]=a->var[i];
			p->coefV[i]=a->coefV[i];
		}
		int k=a->nV;
		for (i = 0; i < b->nV; i++)
		{
			p->var[i+k]=b->var[i];
			p->coefV[i+k]=b->coefV[i];
		}
	}
	else
	{
		p->var=NULL;
		p->coefV=NULL;
	}
	p->nS=a->nS+b->nS;
	if (p->nS!=0)
	{
		p->S=malloc(p->nS*sizeof(Equation));
		p->coefS=malloc(p->nS*sizeof(unsigned char));
		p->sbox=malloc(p->nS*sizeof(unsigned char));
		int i;
		for (i = 0; i < a->nS; i++)
		{
			p->sbox[i]=a->sbox[i];
			p->coefS[i]=a->coefS[i];
			p->S[i]=copyEq(a->S[i]);
		}
		int k=a->nS;
		for (i = 0; i < b->nS; i++)
		{
			p->sbox[i+k]=b->sbox[i];
			p->coefS[i+k]=b->coefS[i];
			p->S[i+k]=copyEq(b->S[i]);
		}
	}
	else
	{
		p->S=NULL;
		p->coefS=NULL;
		p->sbox=NULL;
	}
	Equation pp=reduitEq(p);
	freeEq(p);
	return pp;
}

int apparaitEq(Equation a, int b)
{
	int i,cpt=0;
	for (i = 0; i < a->nV; i++)
	{
		if (a->var[i]==b)
		{
			cpt++;
			break;
		}
	}
	for (i = 0; i < a->nS; i++) cpt+=apparaitEq(a->S[i],b);
	return cpt;
}

Equation appliqueS(Equation a, unsigned char m)
{	
	Equation p=malloc(sizeof(EQUATION));
	p->nV=0;
	p->var=NULL;
	p->coefV=NULL;
	p->nS=1;
	p->S=malloc(sizeof(Equation));
	p->coefS=malloc(sizeof(unsigned char));
	p->sbox=malloc(sizeof(unsigned char));
	p->S[0]=copyEq(a);
	p->coefS[0]=0x01;
	p->sbox[0]=m;
	Equation pp=reduitEq(p);
	freeEq(p);
	return pp;
}

int dependEq(Equation a, int var)
{
	int i;
	for (i = 0; i < a->nV; i++) if (a->var[i]==var) return 1;
	for (i = 0; i < a->nS; i++) if (dependEq(a->S[i],var)) return 1;
	return 0;
}

Equation remplaceEq(Equation a, int var, Equation b)
{
	int i;
	Equation p,tmp,tmp1;
	p=malloc(sizeof(EQUATION));
	p->nS=a->nS;
	if (p->nS>0)
	{
		p->S=malloc(p->nS*sizeof(Equation));
		p->coefS=malloc(p->nS*sizeof(unsigned char));
		p->sbox=malloc(p->nS*sizeof(unsigned char));
		for (i = 0; i < p->nS; i++)
		{
			p->sbox[i]=a->sbox[i];
			p->coefS[i]=a->coefS[i];
			if (dependEq(a->S[i],var)) p->S[i]=remplaceEq(a->S[i],var,b);
			else p->S[i]=copyEq(a->S[i]);
		}
	}
	else
	{
		p->S=NULL;
		p->coefS=NULL;
		p->sbox=NULL;
	}
	p->nV=0;
	tmp=NULL;
	if (a->nV>0)
	{
		p->var=malloc(a->nV*sizeof(unsigned int));
		p->coefV=malloc(a->nV*sizeof(unsigned char));
		for (i = 0; i < a->nV; i++)
		{
			if (a->var[i]==var) tmp=mulEq(a->coefV[i],b);
			else
			{
				p->var[p->nV]=a->var[i];
				p->coefV[p->nV]=a->coefV[i];
				p->nV++;
			}
		}
		if (p->nV==0)
		{
			free(p->var);
			free(p->coefV);
			p->var=NULL;
			p->coefV=NULL;
		}
	}
	else
	{
		p->var=NULL;
		p->coefV=NULL;
	}
	if (tmp==NULL)
	{
		tmp=reduitEq(p);
		freeEq(p);
		return tmp;
	}
	else
	{
		tmp1=addEq(tmp,p);
		freeEq(tmp);
		freeEq(p);
		return tmp1;
	}
}

Equation inverseEq(Equation a, int var)
{
	if (a->nV>0)
	{
		int i;
		for (i = 0; i < a->nV; i++)
		{
			if (a->var[i]==var)
			{
				Equation p=copyEq(a);
				p->nV--;
				if (a->nV==1)
				{
					free(p->var);
					free(p->coefV);
					p->var=NULL;
				}
				else
				{
					p->var[i]=a->var[p->nV];
					p->coefV[i]=a->coefV[p->nV];
				}
				Equation tmp=mulEq(Inverse(a->coefV[i]),p);
				freeEq(p);
				p=reduitEq(tmp);
				freeEq(tmp);
				return p;
			}
		}
	}

	int i;
	for (i = 0; i < a->nS; i++)
	{
		if (dependEq(a->S[i],var))
		{
			Equation p=copyEq(a);
			freeEq(p->S[i]);
			p->nS--;
			if (p->nS==0)
			{
				free(p->S);
				free(p->sbox);
				free(p->coefS);
			}
			else
			{
				if (i!=p->nS)
				{
					p->S[i]=copyEq(p->S[p->nS]);
					freeEq(p->S[p->nS]);
					p->sbox[i]=p->sbox[p->nS];
					p->coefS[i]=p->coefS[p->nS];
				}
			}
			Equation tmp1=mulEq(Inverse(a->coefS[i]),p);
			freeEq(p);
			p=appliqueS(tmp1,0x01^a->sbox[i]);
			freeEq(tmp1);
			tmp1=addEq(a->S[i],p);
			freeEq(p);
			p=inverseEq(tmp1,var);
			freeEq(tmp1);
			return p;
		}
	}
	return NULL;
}

void RemplaceLinearise(Equation E, Equation a, int v)
{
	if (EgaleEq(E,a))
	{
		printf("C'est bizarre!!");
		getchar();
	}
	int i;
	for (i = 0; i < E->nS; i++)
	{
		if (EgaleEq(a,E->S[i]))
		{
			freeEq(E->S[i]);
			E->S[i]=varEq(v);
		}
		else RemplaceLinearise(E->S[i],a,v);
	}
}

Equation SupprS(Equation E)
{
	int i;
	for (i = 0; i < E->nS; i++)
	{
		if (E->sbox[i]==1)
		{
			Equation tmp=copyEq(E->S[i]);
			Equation tmp1=appliqueS(tmp,1);
			Equation tmp2=mulEq(E->coefS[i],tmp1);
			freeEq(tmp1);
			tmp1=addEq(E,tmp2);
			freeEq(tmp2);
			tmp2=mulEq(Inverse(E->coefS[i]),tmp1);
			freeEq(tmp1);
			tmp1=appliqueS(tmp2,0);
			freeEq(tmp2);
			tmp2=addEq(tmp,tmp1);
			freeEq(tmp);
			freeEq(tmp1);
			int j;
			for (j = i+1; j < E->nS; j++)
			{
				if (E->sbox[j]==1)
				{
					printf("Il reste S^-1");
					getchar();
				}
			}
			
			return tmp2;
		}
	}
	return copyEq(E);
}


Equation * Linearise(Equation *E, int taille, int *t)
{
	int cpt,i,j,newvar,k;
	Equation *tmp,*new;
	
	cpt=taille;	
	for (i = 0; i < taille; i++) cpt+=E[i]->nS;
	new=malloc((taille+1)*sizeof(Equation));
	for (i = 0; i < taille; i++) new[i]=copyEq(E[i]);
	newvar=0;
	for (i = 0; i < taille; i++)
	{
		for (j = 0; j < new[i]->nS; j++)
		{
			if (new[i]->S[j]->nS!=0 || new[i]->S[j]->nV!=1)
			{
				freeEq(new[i]->S[j]);
				new[i]->S[j]=varEq(VARIABLES+newvar);
				printf("x[%d]=",VARIABLES+newvar);
				PrintEq(E[i]->S[j]);
				new[taille+newvar]=addEq(E[i]->S[j],new[i]->S[j]);
				printf(";\n");
				newvar++;
				for (k = 0; k < taille+newvar; k++)
				{
					/*printf("Eq avant :");
					PrintEq(new[k]);
					getchar();
					printf("A remplacer par %d :",VARIABLES+newvar-1);
					PrintEq(E[i]->S[j]);
					getchar();*/
					RemplaceLinearise(new[k],E[i]->S[j],VARIABLES+newvar-1);
					/*printf("Eq apres :");
					PrintEq(new[k]);
					getchar();*/
				}
				i=taille;
				break;
			}
		}
	}
	if (newvar>0)
	{
		VARIABLES+=newvar;
		tmp=new;
		new=Linearise(tmp,taille+newvar,t);
		for (i = 0; i < taille+newvar; i++) freeEq(tmp[i]);
		free(tmp);
		return new;
	}
	else
	{
		*t=taille;
		return new;
	}
}

Equation * LineariseTotal(Equation *E, int taille, int *t) /*Bug -> incomplete*/
{
	Equation *Eq,tmp,*Eq2;
	int i,N;
	Eq=Linearise(E,taille,&N);
	for (i = 0; i < N; i++)
	{
		tmp=SupprS(Eq[i]);
		freeEq(Eq[i]);
		Eq[i]=tmp;
	}
	Eq2=Linearise(Eq,N,t);
	for (i = 0; i < N; i++) freeEq(Eq[i]);
	free(Eq);
	return Eq2;
}

int * VarsDedans(Equation a, int *taille)
{
	int i,l,*tmp,*tmp1,**L,*t,n,n1;
	l=a->nS+1;
	L=malloc(l*sizeof(int *));
	t=malloc(l*sizeof(int));
	for (i = 0; i < a->nS; i++) L[i]=VarsDedans(a->S[i],t+i);
	L[a->nS]=malloc(a->nV*sizeof(int));
	for (i = 0; i < a->nV; i++) L[a->nS][i]=a->var[i];
	t[a->nS]=a->nV;
	Tri(L[a->nS],a->nV);
	n=0;
	tmp=malloc(n*sizeof(int));
	for (i = 0; i < l; i++)
	{
		tmp1=UnionListesTriees(tmp,n,L[i],t[i],&n1);
		free(tmp);
		tmp=tmp1;
		n=n1;
	}
	for (i = 0; i < l; i++) free(L[i]);
	free(L);
	*taille=n;
	return tmp;
}

Equation * RemonteSysEq(Equation *E, int N, int *N1, int *Known, int Nconnu)
{
	Equation *Eq,*Eq2,tmp,tmp1;
	int *V,*F,*F1,n,n1,n2,i,j,k,pos1,pos2;
	Eq=malloc(N*sizeof(Equation));
	for (i = 0; i < N; i++) Eq[i]=copyEq(E[i]);
	Eq2=malloc(N*sizeof(Equation));
	pos1=0;
	pos2=N-1;
	F=malloc(Nconnu*sizeof(int));
	for (i = 0; i < Nconnu; i++) F[i]=Known[i];	
	n=Nconnu;
	for (i = 0; i < N; i++)
	{
		V=VarsDedans(Eq[i],&n1);
		Tri(V,n1);
		/*
		printf("V : ");
		PrintListe(V,n1);
		printf("F : ");
		PrintListe(F,n);
		*/
		for (j = n1-1; j >= 0; j--)
		{
			if (SearchElement(V[j],F,n)==-1 && apparaitEq(Eq[i],V[j])==1)
			{
				tmp=inverseEq(Eq[i],V[j]);
				/*
				  printf("%d = ",V[j]);
				  PrintEq(tmp);
				  printf("\n");
				  getchar();
				*/
				for (k = i+1; k < N; k++)
				{
					tmp1=remplaceEq(Eq[k],V[j],tmp);
					freeEq(Eq[k]);
					Eq[k]=tmp1;
				}
				Eq2[pos2--]=copyEq(Eq[i]);
				break;
			}
		}
		if (j==-1) Eq2[pos1++]=copyEq(Eq[i]);
		F1=UnionListesTriees(F,n,V,n1,&n2);
		free(F);
		free(V);
		F=F1;
		n=n2;	
	}
	*N1=pos1;
	for (i = 0; i < N; i++) freeEq(Eq[i]);
	free(Eq);
	return Eq2;
}

Equation CoupeEq(Equation a, int *X, int N)
{
	int i,p,v;
	Equation tmp1,tmp2,tmp3;
	tmp1=NulEq();
	for (i = 0; i < a->nV; i++)
	{
		v=a->var[i];
		p=SearchElement(v,X,N);
		if (p!=-1)
		{
			tmp2=varEq(v);
			tmp3=mulEq(a->coefV[i],tmp2);
			freeEq(tmp2);
			tmp2=addEq(tmp3,tmp1);
			freeEq(tmp1);
			freeEq(tmp3);
			tmp1=tmp2;
		}
	}
	for (i = 0; i < a->nS; i++)
	{
		v=a->S[i]->var[0];
		p=SearchElement(v,X,N);
		if (p!=-1)
		{
			tmp2=varEq(v);
			tmp3=appliqueS(tmp2,a->sbox[i]);
			freeEq(tmp2);
			tmp2=mulEq(a->coefS[i],tmp3);
			freeEq(tmp3);
			tmp3=addEq(tmp2,tmp1);
			freeEq(tmp1);
			freeEq(tmp2);
			tmp1=tmp3;
		}
	}
	return tmp1;
}
