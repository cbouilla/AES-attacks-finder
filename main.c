//      main.c
//      
//      Copyright 2011 Patrick <patrick@patrick-desktop>
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
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <getopt.h>

#include "EquationAES.h"
#include "SysEqLin.h"
#include "Solver.h"
#include "parser_machinery.h"
#include "Bazar.h"
#include "Algo.h"
#include "Search.h"

extern int VARIABLES;

/*
 * Needed for parsing the command line 
 */
static struct option long_options[] = {
/* Required parameters */
{"equations",   required_argument, 0, 'e'}, /* location of equation file */
{"time",  		  required_argument, 0, 't'}, /* Time complexity */
/* Optional parameters */
{"memory", 		  optional_argument, 0, 'm'}, /* Memory complexity */
{"type",  			optional_argument, 0, 'r'}, /* Type of the search */
{"xml",  				optional_argument, 0, 'X'}, /* XML Generation */
{"code",  			optional_argument, 0, 'C'}, /* C Code Generation */
{"noxml",  			optional_argument, 0, 'x'}, /* no XML Generation */
{"nocode",  		optional_argument, 0, 'c'}, /* no C Code Generation */
/*{"reducemem",  	optional_argument, 0, 'M'},*/ /* Try to decrease memory complexity */
{"help",	  	  optional_argument, 0, 'h'}, /* usage() */
{0, 0, 0, 0}
};

/*
 * Default number for the number of messages
 * Default verbose behavior
 */
#define DEFAULT_TYPE					 0
#define DEFAULT_XML 					-1
#define DEFAULT_CODE 					-1
#define DEFAULT_REDUCEMEM 		-1


/*
 * Parameters for the solver
 */
char *eq_file;
int temps;
int memoire;
int r;
int xml;
int code;
int reducemem;



/*
 * Usage display 
 */
void display_usage(char *s) {
 	printf("Usage: %s -e <equations> -t <time> [-m <memory>] [-r <type>] [-X|-x] [-C|-c] [-h]\n", s); 
  printf("Options:\n");
  printf("\t-e, --equations\t   File of the system of equations to solve\n");
  printf("\t-t, --time\t   Max time complexity\n");
  printf("\t-m, --memory\t   Memory complexity (default: time = memory)\n");
  printf("\t-r, --type\t   Type of the search. See below (default: 3)\n");
  printf("\t-X, --xml\t   Generate the XML tree of the attack\n");
  printf("\t-x, --noxml\t   Do not generate the XML tree of the attack\n");
  printf("\t-C, --code\t   Do not generate the code of the attack\n");
  printf("\t-c, --nocode\t   Generate the code of the attack\n");
  /*printf("\t-M, --reducemem\t   Try to decrease the memory complexity\n");*/
  printf("\t-h, --help\t   Display this help menu\n");
  printf("\n");
  printf("Algorithms :\n"
         "\t0 : Randomized Search (default) \n"
         "\t1 : Exhaustive Search (Balanced Solver)\n"
         "\t2 : Exhaustive Search for Guess-and-Determine Solver\n"
         "\t3 : Randomized Search (Symetric) \n"
         "\t4 : Manual Search\n"
         );
}

/*
 * Initialize the PRNG
 */
void init_PRNG() {

	#if 0
	int seed = 0;
  int fd;

  
	if((fd = open("/dev/random", 0)) < 0) {
	  printf("/dev/random pas disponible ?\n");
	  seed = time(NULL) + getpid(); 
	} else {
	  read(fd, &seed, sizeof(int));
	  close(fd);
	}
	#else
	int seed =0x7E86D5CE;
	//int seed = 1;
	#endif
	srand(seed);
	printf("[+] PRNG initialized to 0x%08X\n", seed);
}

/*
 * Deduce the XML filename for the found attack from the equation file
 */
char* getXML(int t, int m, int r) {
  int 	len;
  char 	*sz;  
  char 	*sz2; 
  char 	*res;
  char  *tmp;
  
  len=strlen(eq_file);
  sz=eq_file;
  sz+=len-1;
	while(*--sz!='/');
  sz2=++sz;  
	while(*sz2++!='.');
  
  tmp=(char *)calloc(sz2-sz, sizeof(*res));
  strncpy(tmp, sz, sz2-sz-1);
  
  res=(char *)calloc(3+1+sz2-sz-1+1+2+1+2+1+2+4+1, sizeof(*res));
	sprintf(res, "xml/%s_%d-%d_%d.xml", tmp, t, m, r);
  return res;
}

/*
 * Deduce the C Code filename for the found attack from the equation file
 */
char* getCFile(int t, int m, int r) {
  int 	len;
  char 	*sz;  
  char 	*sz2; 
  char 	*res;
  char  *tmp;
  
  len=strlen(eq_file);
  sz=eq_file;
  sz+=len-1;
	while(*--sz!='/');
  sz2=++sz;  
	while(*sz2++!='.');
  
  tmp=(char *)calloc(sz2-sz, sizeof(*res));
  strncpy(tmp, sz, sz2-sz-1);
  
  res=(char *)calloc(sz2-sz-1+1+2+1+2+1+2+1, sizeof(*res));
	sprintf(res, "%s_%d-%d_%d", tmp, t, m, r);
  return res;
}


/*
 * Main Procedure
 */
int main(int argc, char** argv) {
  
  int		c;
  /*
	int 	Nk;
  int 	Tours;
  int 	Final;
  int 	Messages;
   */
  int 	N;
  int 	*Known;
  int 	NKnown;
  int 	i;
  SysEqLin E;
  Equation *Eq;
  string_list_t *variable_names;
  int **var_by_message;
  int n_message;
  int n_var_by_message;


  
  if(argc<5) {
    display_usage(argv[0]);
    exit(1);
  }
  
  /*
   * Default values
   */
  memoire=-1;
  r=DEFAULT_TYPE;
  xml=DEFAULT_XML;
  code=DEFAULT_CODE;
  reducemem=DEFAULT_REDUCEMEM;
  
  /*
   * Go over all the elements of the command line
   */
  while (1) {
    
    int option_index = 0;    
    c = getopt_long (argc, argv, "e:t:m:r:x::X::c::C::h::", long_options, &option_index);
    
    /*
     * Detect the end of the options
     */
    if(c==-1) break;
    
    /*
     * Parse the command line
     */
    switch (c) {
      	/* Required parameters */
      case 'e': eq_file=optarg; break;
      case 't': temps=atoi(optarg); break;
        
        /* Optional parameters */
      case 'm': memoire=atoi(optarg); break;
      case 'r': r=atoi(optarg); break;
      case 'X': xml=1; break;
      case 'x': xml=0; break;
      case 'C': code=1; break;
      case 'c': code=0; break;
      /*case 'M': reducemem=1; break;*/
        
      case 'h':
      case '?':
        display_usage(argv[0]);
        exit(1);
        
      default:
        display_usage(argv[0]);
        exit(1);
    }
  }

  /*
   * Default values
   */
  if (memoire<=0) memoire=temps;
  
  
  
  
  
	/*  
   * Lancement du programme  
   */	   
  Eq=NULL;
  parse(eq_file, &Eq, &N, &VARIABLES, &Known, &NKnown, &variable_names, &n_message, &n_var_by_message, &var_by_message);
	printf("[+] Parsed %d equations in %d variables (%d are known)\n", N, VARIABLES, NKnown);  
  
  
	/*
   * Initialize PRNG
   */
	init_PRNG();
  
	
	E=Transform(Eq,N,2);
	for (i = 0; i < N; i++) freeEq(Eq[i]);
	free(Eq);
		
		
  
#if 1
	Symetric_structure S = InitialiseSymetricStrucure(var_by_message,n_var_by_message,n_message);
	Solver B = SearchAlgo(E, temps, memoire, Known, NKnown, r, variable_names,S);
#else
	FILE *b;
	b=fopen("xml/4r_4CP_4-4_3.xml","r");
	Solver B = charge_solver(b, variable_names);
	PrintListe(B->var,B->Nvar);
	fclose(b);
	printf("Solver charged\n");
#endif

  /*
   * If found
   */
	if(B) {
		FILE *output;
    
    printf("[>] Result found : \n");
    printf("   - Time complexity : 2^%d\n", B->temps*8);
    printf("   - Memory complexity : 2^%d\n", B->memoire*8);
    printf("   - Number of solution : 2^%d\n", B->sortie*8);
  	
    /*
     * XML Tree generation
     */
    if(xml==-1) {
      char c;
      printf("[?] Do you want to save the result (XML) ? (y/n) ");fflush(stdout);
			scanf(" %c", &c);
			getchar();
      xml=(c=='y')?1:0;
    }
    
		if(xml) {
      char *xml_file; 
      xml_file=getXML(B->temps, B->memoire, r);
      
			output = fopen(xml_file, "w");
			print_solver(output, B, variable_names);
			fclose(output);
      
      printf("[+] Output XML file: \n");
      printf("\t%s\n", xml_file);
    } else {
      printf("[-] Skip XML generation\n");
    }
    
    /*
     * C Code generation
     */
    if(code==-1) {
      char c;
      printf("[?] Do you want to generate C code ? (y/n) ");fflush(stdout);
      scanf(" %c",&c);
      getchar();
      code=(c=='y')?1:0;
    }
    
    if(code) {
      char *code_file; 
      char *bin_file; 
      code_file=getCFile(B->temps, B->memoire, r);
      bin_file=strdup(code_file);
			      		
      printf("[+] Source file: %s.c\n", code_file);
      printf("[+] Compilation : \n\tgcc -Wall Programs/%s.c -o Programs/%s -O3\n\t./Programs/%s\n\n", code_file, bin_file, bin_file);
      EcrireAlgo(code_file, B, Known, NKnown, E, variable_names);
      
		} else {
      printf("[-] Skip code generation\n");
    }
    
		freeSolver(B);
	}
  
  /*
   * Free memory
   */
	freeSysEq(E);
	free(Known);
  
	return 0;
}
