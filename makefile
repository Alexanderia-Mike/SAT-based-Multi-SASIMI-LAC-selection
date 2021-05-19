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
ABCMAP = abc/src/base/abci/abcMap.o

ODIR = src/obj
SDIR = src
# LDIR = ../lib

LIBS=-labc -lqdpll -lnenofex -lpicosat -lm -ldl -rdynamic -lreadline -ltermcap -lpthread -lstdc++ -lrt -lstdc++fs

_DEPS1 = abcApi.h cktBit.h cktUtil.h cmdline.h cnf2Depqbf.h debugAssert.h headers.h sasimi.h simulator.h Sat_Based_Multi_Sel_Manager.h
DEPS = $(patsubst %,$(IDIR1)/%,$(_DEPS1))

_OBJ1 = cktUtil.o test_new.o sasimi.o simulator.o strashtest.o cnf2Depqbf.o sortingNetwork.o Sat_Based_Multi_Sel_Manager.o build_verify_miter.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ1))


sasimi-vecbee : $(OBJ)
	$(CC) -o $@ $^ $(DIRFLAGS) $(ABCMAP) $(LIBS) $(INCFLAGS)

$(OBJ) : $(ODIR)/%.o : $(SDIR)/%.cpp
	$(CC) -c -o $@ $< $(INCFLAGS) $(DIRFLAGS) $(DEF)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~
