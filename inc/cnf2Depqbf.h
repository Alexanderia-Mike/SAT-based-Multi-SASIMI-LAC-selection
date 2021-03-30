#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "qdpll.h"
#include "headers.h"

// read the cnf from a "cnf" file, then store the data in <depqbf>
extern void Cnf_DataFile2Depqbf( IN char * pFileName, OUT QDPLL * depqbf );