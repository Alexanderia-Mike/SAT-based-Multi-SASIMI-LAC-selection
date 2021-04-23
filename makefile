IDIR1 = inc
IDIR2 = abc/src
IDIR3 = depqbf
LDIR1 = abc
LDIR2 = depqbf/nenofex
LDIR3 = depqbf/picosat
LDIR4 = depqbf
IDIR = $(IDIR1) $(IDIR2) $(IDIR3)
LDIR = $(LDIR1) $(LDIR2) $(LDIR3) $(LDIR4)
CC = g++ -v
INCFLAGS = $(foreach d, $(IDIR), -I$d)
DIRFLAGS = $(foreach d, $(LDIR), -L$d)
DEF = -DLIN64 -g -Wall -O3 -std=c++17

ODIR = src/obj
SDIR = src
# LDIR = ../lib

LIBS=-labc -lqdpll -lnenofex -lpicosat -lm -ldl -rdynamic -lreadline -ltermcap -lpthread -lstdc++ -lrt -lstdc++fs

_DEPS1 = abcApi.h cktBit.h cktUtil.h cmdline.h cnf2Depqbf.h debugAssert.h headers.h sasimi.h simulator.h
DEPS = $(patsubst %,$(IDIR1)/%,$(_DEPS1))

_OBJ1 = cktUtil.o main.o test.o sasimi.o SATBasedMultiSel.o simulator.o strashtest.o cnf2Depqbf.o sortingNetwork.o sorting_test.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ1))


$(ODIR)/%.o : $(SDIR)/%.cpp $(DEPS)
	$(CC) -c -o $@ $< $(INCFLAGS) $(DIRFLAGS) $(DEF)

sasimi-vecbee : $(OBJ)
	$(CC) -o $@ $^ $(DIRFLAGS) $(LIBS) $(INCFLAGS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~
