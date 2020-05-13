#include <stdio.h>
#include <stdlib.h>
#include "EquationAES.h"

int VARIABLES = 0;

int MakeKS(Equation *Eq, int Nk, int Tours, int Final)
{
	Equation tmp,tmp1,tmp2,tmp3;
	int i,j,pos;
	pos=0;
	for (i = Nk; i < (Tours+Final+1)*4; i++)
	{
		for (j = 0; j < 4; j++)
		{
			if (i%Nk==0)
			{
				tmp=varEq(4*(i-1)+(j+1)%4);
				tmp1=appliqueS(tmp,0);
				freeEq(tmp);
				if (j==0)
				{
					tmp2=varEq(VARIABLES-(i/Nk));
					tmp=addEq(tmp1,tmp2);
					freeEq(tmp1);
					freeEq(tmp2);
				}
				else tmp=tmp1;
				tmp1=varEq(4*(i-Nk)+j);
				tmp2=varEq(4*i+j);
				tmp3=addEq(tmp1,tmp2);
				Eq[pos++]=addEq(tmp,tmp3);
				freeEq(tmp);
				freeEq(tmp1);
				freeEq(tmp2);
				freeEq(tmp3);
			}
			else
			{
				if (Nk>6 && (i%Nk)==4)
				{
					tmp=varEq(4*(i-1)+j);
					tmp1=appliqueS(tmp,0);
					freeEq(tmp);
					tmp=tmp1;
					tmp1=varEq(4*(i-Nk)+j);
					tmp2=varEq(4*i+j);
					tmp3=addEq(tmp1,tmp2);
					Eq[pos++]=addEq(tmp,tmp3);
					freeEq(tmp);
					freeEq(tmp1);
					freeEq(tmp2);
					freeEq(tmp3);
				}
				else
				{
					tmp=varEq(4*(i-1)+j);
					tmp1=varEq(4*(i-Nk)+j);
					tmp2=varEq(4*i+j);
					tmp3=addEq(tmp1,tmp2);
					Eq[pos++]=addEq(tmp,tmp3);
					freeEq(tmp);
					freeEq(tmp1);
					freeEq(tmp2);
					freeEq(tmp3);
				}
			}
		}
	}
	return pos;
}

int MakeAK(Equation *Eq, int key, int state1, int state2)
{
	int i,j,pos;
	Equation tmp,tmp1,tmp2;
	pos=0;
	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 4; j++)
		{
			tmp=varEq(16*key+4*i+j);
			tmp1=varEq(16*state1+4*i+j);
			tmp2=addEq(tmp,tmp1);
			freeEq(tmp);
			freeEq(tmp1);
			tmp=varEq(16*state2+4*i+j);
			Eq[pos++]=addEq(tmp,tmp2);
			freeEq(tmp);
			freeEq(tmp2);
		}
	}
	return pos;
}

int MakeMC(Equation *Eq, int state1, int state2)
{
	int i,j,k,pos;
	pos=0;
	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 4; j++)
		{
			Eq[pos]=malloc(sizeof(EQUATION));
			Eq[pos]->nV=5;
			Eq[pos]->var=malloc(5*sizeof(int));
			Eq[pos]->coefV=malloc(5*sizeof(unsigned char));
			for (k = 0; k < 4; k++) Eq[pos]->var[k]=16*state1+4*i+k;
			Eq[pos]->var[k]=16*state2+4*i+j;
			Eq[pos]->coefV[k]=1;
			Eq[pos]->coefV[j]=0x02;
			Eq[pos]->coefV[(j+1)%4]=0x03;
			Eq[pos]->coefV[(j+2)%4]=0x01;
			Eq[pos]->coefV[(j+3)%4]=0x01;
			Eq[pos]->nS=0;
			Eq[pos]->S=NULL;
			Eq[pos]->sbox=NULL;
			pos++;
		}
	}
	return pos;
}

int MakeSS(Equation *Eq, int key, int state1, int state2)
{
	int i,j,pos;
	Equation tmp,tmp1,tmp2;
	pos=0;
	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 4; j++)
		{
			tmp=varEq(16*state2+4*i+j);
			tmp1=appliqueS(tmp,1);
			freeEq(tmp);
			tmp=varEq(16*state1+4*((i+j)%4)+j);
			tmp2=addEq(tmp,tmp1);
			freeEq(tmp);
			freeEq(tmp1);
			tmp=varEq(16*key+4*((i+j)%4)+j);
			Eq[pos++]=addEq(tmp,tmp2);
			freeEq(tmp);
			freeEq(tmp2);
		}
	}
	return pos;
}

Equation * MakeEquations(int Nk, int Tours, int Final, int Messages, int *N)
{
	int NombreEtat,NombreCstRonde,NombreEqKS,NombreEqMC,NombreEqSS,NombreEqAK;
	Equation *Eq;
	int i,j,pos,taille,state,key;
	
	NombreEtat=Tours+Final+1+Messages*(2*Tours+Final+2);
	NombreCstRonde=Tours+Final;
	VARIABLES=16*NombreEtat+NombreCstRonde;
	NombreEqKS=((Tours+Final+1)*4-Nk)*4;
	NombreEqMC=16*Tours*Messages;
	NombreEqSS=16*(Tours+Final)*Messages;
	NombreEqAK=16*Messages;
	taille=NombreEqKS+NombreEqMC+NombreEqSS+NombreEqAK;
	printf("taille = %d, Variables = %d\n",taille,VARIABLES);
	Eq=malloc(taille*sizeof(Equation));
	pos=MakeKS(Eq,Nk,Tours,Final);
	state=(Tours+Final+1);
	for (i = 0; i < Messages; i++)
	{
		key=0;
		for (j = 0; j < Tours; j++)
		{
			pos+=MakeSS(Eq+pos,key,state,state+1);
			state++;
			pos+=MakeMC(Eq+pos,state,state+1);
			state++;
			key++;
		}
		for (j = 0; j < Final; j++)
		{
			pos+=MakeSS(Eq+pos,key,state,state+1);
			state++;
			key++;
		}
		pos+=MakeAK(Eq+pos,key,state,state+1);
		state+=2;
	}
	/*for (i = 0; i < pos; i++)
	{
		printf("Eq[%d] : ",i);
		PrintEq(Eq[i]);
		printf("\n");
	}
	getchar();
	*/
	*N=taille;
	return Eq;
}

int * VariablesKnownes(int Tours, int Final, int Messages, int *N)
{
	int NombreCstRonde,*Known;
	int i,j,pos,taille,state;
	
	NombreCstRonde=Tours+Final;
	taille=32*Messages+NombreCstRonde;
	Known=malloc(2*taille*sizeof(int));
	state=(Tours+Final+1);
	pos=0;
	for (i = 0; i < Messages; i++)
	{
		for (j = 0; j < 16; j++) Known[pos++]=16*state+j;
		for (j = 0; j < Tours; j++)
		{
			state++;
			state++;
		}
		for (j = 0; j < Final; j++)
		{
			state++;
		}
		state++;
		for (j = 0; j < 16; j++) Known[pos++]=16*state+j;
		state++;
	}
	for (j = 0; j < NombreCstRonde; j++) Known[pos++]=16*state+j;
	printf("Variables Knownes : ");
	for (i = 0; i < pos; i++) printf("%u ",Known[i]);
	printf("\n");
	/*getchar();*/
	*N=taille;
	return Known;
}

VARS ConversionVars(int v, int Nk, int Tours, int Final, int Messages)
{
	int i,j,state,tmp;
	VARS V;
	state=(Tours+Final+1);
	V.round=v/16;
	V.j=(v-16*V.round)/4;
	V.i=(v-16*V.round)%4;
	V.v=v;
	if (v<16*state)
	{
		V.lettre='K';
		V.message=0;
		return V;
	}
	tmp=v/16;
	for (i = 0; i < Messages; i++)
	{
		for (j = 0; j < Tours; j++)
		{
			if (tmp==state)
			{
				V.lettre='X';
				V.round=j;
				V.message=i;
			}
			state++;
			if (tmp==state)
			{
				V.lettre='W';
				V.round=j;
				V.message=i;
				return V;
			}
			state++;
		}
		for (j = 0; j < Final; j++)
		{
			if (tmp==state)
			{
				V.lettre='X';
				V.round=Tours+j;
				V.message=i;
				return V;
			}
			state++;
			if (tmp==state)
			{
				V.lettre='W';
				V.round=Tours+j;
				V.message=i;
				return V;
			}
		}
		if (tmp==state)
		{
			V.lettre='X';
			V.round=Tours;
			V.message=i;
			return V;
		}
		state++;
		if (tmp==state)
		{
			V.lettre='Z';
			V.round=Tours+Final;
			V.message=i;
			return V;
		}
		state++;
	}
	V.lettre='C';
	V.round=VARIABLES-v;
	V.i=0;
	V.j=0;
	return V;
}

Equation * MakeEquations2(int Nk, int Tours, int Final, int Messages, int *N)
{
  Equation *Eq,*Eq2,tmp1,tmp2;/*,tmp3;*/
	int i,j,dec,pos;
	/*int t;*/
	Eq=MakeEquations(Nk,Tours,Final,Messages,N);
	*N=*N+(Messages-1)*15;
	Eq2=malloc((*N+1)*sizeof(Equation));
	dec=(Tours+Final+1)*16;
	pos=0;
	for (i = 1; i < 16; i++)
	{
		for (j = 1; j < Messages; j++)
		{
			tmp1=varEq(dec+i);
			tmp2=varEq((2*(Tours+Final)+1)*16*j+dec+i);
			Eq2[pos++]=addEq(tmp1,tmp2);
			freeEq(tmp1);
			freeEq(tmp2);
		}
	}
	for (i = 0; i < *N-(Messages-1)*15; i++) Eq2[pos++]=Eq[i];
	free(Eq);
	/*for (i = dec; i < 176; i++)
	{
		printf("%d / %d\n",i-dec,176-dec);
		tmp1=varEq(i);
		tmp2=varEq(i+176-dec);
		Eq2[*N]=addEq(tmp1,tmp2);
		t=SimplifySysEq(Eq2,*N+1);
		if (t<=*N)
		{
			freeEq(tmp1);
			tmp1=varEq(i);
			for (j = 0; j < *N; j++)
			{
				tmp3=remplaceEq(Eq2[j],i+176-dec,tmp1);
				freeEq(Eq2[j]);
				Eq2[j]=tmp3;
			}
			*N=SimplifySysEq(Eq2,t);
		}
		else freeEq(Eq2[*N]);
		freeEq(tmp1);
		freeEq(tmp2);
		tmp3=varEq(i);
		tmp1=appliqueS(tmp3,0);
		freeEq(tmp3);
		tmp3=varEq(i+176-dec);
		tmp2=appliqueS(tmp3,0);
		freeEq(tmp3);
		Eq2[*N]=addEq(tmp1,tmp2);
		t=SimplifySysEq(Eq2,*N+1);
		if (t<=*N)
		{
			freeEq(tmp1);
			tmp1=varEq(i);
			for (j = 0; j < *N; j++)
			{
				tmp3=remplaceEq(Eq2[j],i+176-dec,tmp1);
				freeEq(Eq2[j]);
				Eq2[j]=tmp3;
			}
			*N=SimplifySysEq(Eq2,t);
		}
		else freeEq(Eq2[*N]);
		freeEq(tmp1);
		freeEq(tmp2);
		tmp3=varEq(i);
		tmp1=appliqueS(tmp3,1);
		freeEq(tmp3);
		tmp3=varEq(i+176-dec);
		tmp2=appliqueS(tmp3,1);
		freeEq(tmp3);
		Eq2[*N]=addEq(tmp1,tmp2);
		t=SimplifySysEq(Eq2,*N+1);
		if (t<=*N)
		{
			freeEq(tmp1);
			tmp1=varEq(i);
			for (j = 0; j < *N; j++)
			{
				tmp3=remplaceEq(Eq2[j],i+176-dec,tmp1);
				freeEq(Eq2[j]);
				Eq2[j]=tmp3;
			}
			*N=SimplifySysEq(Eq2,t);
		}
		else freeEq(Eq2[*N]);
		freeEq(tmp1);
		freeEq(tmp2);
	}*/
	return Eq2;
}

int * VariablesKnownes2(int Tours, int Final, int Messages, int *N)
{
	int NombreCstRonde,*Known;
	int i,j,pos,taille,state;
	
	NombreCstRonde=Tours+Final;
	taille=16*Messages+NombreCstRonde;
	Known=malloc(taille*sizeof(int));
	state=(Tours+Final+1);
	pos=0;
	for (i = 0; i < Messages; i++)
	{
	  /*for (j = 0; j < 16; j++) Known[pos++]=16*state+j;*/
		for (j = 0; j < Tours; j++)
		{
			state++;
			state++;
		}
		for (j = 0; j < Final; j++)
		{
			state++;
		}
		state++;
		for (j = 0; j < 16; j++) Known[pos++]=16*state+j;
		state++;
	}
	for (j = 0; j < NombreCstRonde; j++) Known[pos++]=16*state+j;
	printf("Variables Knownes : ");
	for (i = 0; i < pos; i++) printf("%u ",Known[i]);
	/*getchar();*/
	*N=taille;
	return Known;
}
