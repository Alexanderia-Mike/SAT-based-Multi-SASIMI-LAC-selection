#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "qdpll.h"
#include "headers.h"

// read the cnf from a "cnf" file, then store the data in <depqbf>
extern void Cnf_DataFile2Depqbf( IN char * pFileName, OUT QDPLL * depqbf, int * OriPIIDs, int * MUXPIIDs );

// solve the sat problem stored in <depqbf> and if SAT or UNSAT, return the assignment of variables in the file named <pFileName>
//  Require: depqbf should not be empty.
extern QDPLLResult QDPLL_SolveSatWriteAssignments( IN QDPLL * depqbf, IN char * pFileNameOut, IN char * pFileNameIn );

extern void cnf_file_to_depqbf_for_verify_miter( IN char * pFileName, OUT QDPLL * depqbf, int * OriPIIDs );