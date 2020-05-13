SRC=CorpsK.c Bazar.c Algebre.c Equation.c EquationAES.c Solver.c SysEqLin.c main.c parser_machinery.c parser.tab.c lexer.tab.c Algo.c Search.c Symetric.c
OBJ=$(SRC:.c=.o)
#icc flags
#CFLAGS=-fast -inline-level=2 -unroll-falign-functions -fno-alias -fno-fnalias -fargument-noalias-global -opt-multi-version-aggressive -unroll-aggressive-vec-report3 -opt-report3 -Wall -openmp -openmp-report2 #-p -g

CFLAGS=-O3 -Wall

#CFLAGS=-Os -Wall -fopenmp 
#CFLAGS=-g -Wall #-fopenmp 
#CFLAGS=-g -Wall -fopenmp -std=c99
#CFLAGS=-g -Wall -fopenmp -std=gnu99
#CFLAGS=-g -Wall -fopenmp #-ansi -pedantic

YACC=bison -d
LEX=flex
#if icc is available, the resulting program will be a bit faster
#CC=icc
CC=gcc

#parser_machinery: parser_machinery.o parser.tab.o lexer.tab.o
#	$(CC) $(CFLAGS) -o parser  parser_machinery.o parser.tab.o lexer.tab.o


# Fichier executable Resolution
Resolution: $(OBJ)
	$(CC) $(CFLAGS) -o Resolution $(OBJ)

#Nettoyage
clean:
	rm -f Resolution Resolution.db *.o *.do *.P lexer.tab.c parser.tab.c parser.tab.h

# Lex/Yacc parser
parser.tab.c parser.tab.h: parser.y
	$(YACC) parser.y


lexer.tab.c: lexer.l
	$(LEX) lexer.l
	mv lex.yy.c lexer.tab.c

################# Black Magic starts here


depend:  $(SRC:.c=.P)

.PHONY: clean depend

DEPDIR = .deps
df = $(DEPDIR)/$(*F)

MAKEDEPEND = gcc -M $(CPPFLAGS) -o $*.d $<

%.o : %.c
	$(COMPILE.c) -MD -o $@ $<
	@cp $*.d $*.P; \
	sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	-e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	rm -f $*.d


-include $(SRCS:.c=.P)
