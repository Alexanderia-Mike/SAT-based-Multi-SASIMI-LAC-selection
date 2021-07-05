#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "qdpll.h"
#include "abcApi.h"
#include <iostream>

#include <unistd.h>
#include <ctype.h>
#include <sys/resource.h>
#include "qdpll_mem.h"
#include "qdpll_pcnf.h"
#include "qdpll_exit.h"
#include "qdpll_stack.h"
#include "qdpll_internals.h"
#include "qdpll_dep_man_generic.h"
#include "qdpll_dep_man_qdag.h"
#include "qdpll_config.h"
#include "qdpll_dynamic_nenofex.h"
#include "../depqbf/picosat/picosat.h"

#define QDPLL_ABORT_QDPLL(cond,msg)					\
  do {									\
    if (cond)								\
      {									\
        fprintf (stderr, "[QDPLL] %s at line %d: %s\n", __func__,	\
                 __LINE__, msg);                                        \
        fflush (stderr);                                                \
        abort();                                                        \
      }									\
  } while (0)

  /* Returns non-zero iff the scope 's' has at least one variable which is NOT
   internal and which is free, i.e. a free user var. */
static int
has_scope_free_user_var (QDPLL *qdpll, Scope *s)
{
  VarID *p, *e;
  for (p = s->vars.start, e = s->vars.top; p < e; p++)
    {
      VarID id = *p;
      assert (id);
      Var *var = VARID2VARPTR(qdpll->pcnf.vars, id);
      assert (var->id == id);
      /* Free variables do not have a 'var->user_scope'. See also
         'declare_and_init_variable'. */
      if (!var->is_internal && !var->user_scope)
        return 1;
    }
  return 0;
}


// add the selection signals in <MUXPIIDs> to existential quantifiers, and add Original PIs in <OriPIIDs> to universal quantifiers
static void QDPLL_AddQuantifiers( QDPLL * depqbf, int * OriPIIDs, int * MUXPIIDs, int nVars );

static void QDPLL_ddd_quantifiers_for_verify_miter( QDPLL * depqbf, int * OriPIIDs, int nVars );
// read the cnf information from file with the name <pFileName>, and store the information in vectors
//  If reading is successful, return true, otherwise return false
static bool Cnf_DataFromFile ( char * pFileName, Vec_Int_t ** pvClas, Vec_Int_t ** pvLits, char * pBuffer, FILE * pFile, int MaxLine, int * pnVars );
// finish reading the file and free the vectors
static void Cnf_FinishReadingFile ( FILE * pFile, Vec_Int_t ** pvClas, Vec_Int_t ** pvLits, char * pBuffer );

void qdpll_print_qdimacs_output_to_file (QDPLL * qdpll, char * fileName);

// read the cnf from a "cnf" file, then store the data in <depqbf>
void Cnf_DataFile2Depqbf( char * pFileName, QDPLL * depqbf, int * OriPIIDs, int * MUXPIIDs )
{
    Vec_Int_t ** pvClas = new Vec_Int_t *;   // vector of clauses
    Vec_Int_t ** pvLits = new Vec_Int_t *;   // vector of literals
    int MaxLine = 1000000, nVars = -1;
    char * pBuffer = ABC_ALLOC( char, MaxLine );
    int Entry, i;
    FILE * pFile = fopen( pFileName, "rb" );

    // read cnf information from <pFileName>
    bool ReadSuc = Cnf_DataFromFile( pFileName, pvClas, pvLits, pBuffer, pFile, MaxLine, & nVars );
    Vec_Int_t * vClas = * pvClas;
    Vec_Int_t * vLits = * pvLits;

    if ( !ReadSuc ) goto finish;

    // create quantifiers
    QDPLL_AddQuantifiers( depqbf, OriPIIDs, MUXPIIDs, nVars );

    // add clauses
    Vec_IntForEachEntry( vClas, Entry, i )
    {
        if ( i == Vec_IntSize( vClas ) - 1 )    // the last index in <vClas> is invalid
            break;
        for ( int j = Entry; j < Vec_IntEntry( vClas, i + 1 ); ++j )
            qdpll_add( depqbf, Vec_IntEntry( vLits, j ) );  // add the corresponding literals in <vLits>
        qdpll_add( depqbf, 0 ); // add the terminal 0
    }

    // close the file and free the memory
    finish:
    Cnf_FinishReadingFile( pFile, pvClas, pvLits, pBuffer );
}

void cnf_file_to_depqbf_for_verify_miter( char * pFileName, QDPLL * depqbf, int * OriPIIDs )
{
    Vec_Int_t ** pvClas = new Vec_Int_t *;   // vector of clauses
    Vec_Int_t ** pvLits = new Vec_Int_t *;   // vector of literals
    int MaxLine = 1000000, nVars = -1;
    char * pBuffer = ABC_ALLOC( char, MaxLine );
    int Entry, i;
    FILE * pFile = fopen( pFileName, "rb" );

    // read cnf information from <pFileName>
    bool ReadSuc = Cnf_DataFromFile( pFileName, pvClas, pvLits, pBuffer, pFile, MaxLine, & nVars );
    Vec_Int_t * vClas = * pvClas;
    Vec_Int_t * vLits = * pvLits;

    if ( !ReadSuc ) goto finish;

    // create quantifiers
    QDPLL_ddd_quantifiers_for_verify_miter( depqbf, OriPIIDs, nVars );

    // add clauses
    Vec_IntForEachEntry( vClas, Entry, i )
    {
        if ( i == Vec_IntSize( vClas ) - 1 )    // the last index in <vClas> is invalid
            break;
        for ( int j = Entry; j < Vec_IntEntry( vClas, i + 1 ); ++j )
            qdpll_add( depqbf, Vec_IntEntry( vLits, j ) );  // add the corresponding literals in <vLits>
        qdpll_add( depqbf, 0 ); // add the terminal 0
    }

    // close the file and free the memory
    finish:
    Cnf_FinishReadingFile( pFile, pvClas, pvLits, pBuffer );
}

static void QDPLL_ddd_quantifiers_for_verify_miter( QDPLL * depqbf, int * OriPIIDs, int nVars )
{
    qdpll_new_scope_at_nesting (depqbf, QDPLL_QTYPE_FORALL, 1);
    for ( int i = 0; i < OriPIIDs[0]; ++i )
        qdpll_add( depqbf, OriPIIDs[i+1] );
    qdpll_add( depqbf, 0 );
    // add existential quantifiers to all other variables
    qdpll_new_scope_at_nesting (depqbf, QDPLL_QTYPE_EXISTS, 2);
    for ( int i = 1; i < nVars; ++i )
    {
        // exclude the previously-added variables
        if ( i >= OriPIIDs[1] && i <= OriPIIDs[OriPIIDs[0]] )
            continue;
        qdpll_add( depqbf, i );
    }
    qdpll_add( depqbf, 0 );
}

// solve the sat problem stored in <depqbf> and if SAT or UNSAT, return the assignment of variables in the file named <pFileName>
QDPLLResult QDPLL_SolveSatWriteAssignments( QDPLL * depqbf, char * pFileNameOut, char * pFileNameIn )
{
    FILE * pOut;

    // solve the SAT problem
    QDPLLResult res = qdpll_sat (depqbf);
    if ( res == QDPLL_RESULT_UNKNOWN )
        std::cout << "UNKNOWN! Cannot figure out whether there exists a LAC!" << std::endl;
    else
    {
        if ( res == QDPLL_RESULT_SAT )
            std::cout << "---------- SAT!" << std::endl << "The solution is:" << std::endl;
        else
            std::cout << "---------- UNSAT!" << std::endl << "The UNSAT core is:" << std::endl;
        // qdpll_print_qdimacs_output ( depqbf );
        qdpll_print_qdimacs_output_to_file ( depqbf, pFileNameOut );
        // qdpll_print_qdimacs_output ( depqbf );
    }
    qdpll_reset( depqbf );
    pOut = fopen( pFileNameIn, "w+" );
    qdpll_print ( depqbf, pOut );
    fclose( pOut );
    return res;
}


/************************************ HELPING FUNCTIONS ************************************/

static void QDPLL_AddQuantifiers( QDPLL * depqbf, int * OriPIIDs, int * MUXPIIDs, int nVars )
{
    // add existential quantifiers to depqbf
    qdpll_new_scope_at_nesting (depqbf, QDPLL_QTYPE_EXISTS, 1);
    for ( int i = 0; i < MUXPIIDs[0]; ++i )
        qdpll_add( depqbf, MUXPIIDs[i+1] );
    qdpll_add( depqbf, 0 );
    // add universal quantifiers to depqbf
    qdpll_new_scope_at_nesting (depqbf, QDPLL_QTYPE_FORALL, 2);
    for ( int i = 0; i < OriPIIDs[0]; ++i )
        qdpll_add( depqbf, OriPIIDs[i+1] );
    qdpll_add( depqbf, 0 );
    // add existential quantifiers to all other variables
    qdpll_new_scope_at_nesting (depqbf, QDPLL_QTYPE_EXISTS, 3);
    for ( int i = 1; i < nVars; ++i )
    {
        if // exclude the previously-added variables
        (
            ( i >= MUXPIIDs[1] && i <= MUXPIIDs[MUXPIIDs[0]] ) ||
            ( i >= OriPIIDs[1] && i <= OriPIIDs[OriPIIDs[0]] )
        )
            continue;
        qdpll_add( depqbf, i );
    }
    qdpll_add( depqbf, 0 );
}

static bool Cnf_DataFromFile ( char * pFileName, Vec_Int_t ** pvClas, Vec_Int_t ** pvLits, char * pBuffer, FILE * pFile, int MaxLine, int * pnVars )
{
    int Lit, & nVars = * pnVars, nClas = -1, iLine = 0;
    assert( &nVars == pnVars );
    char * pToken;
    if ( pFile == NULL )
    {
        printf( "Cannot open file \"%s\" for writing.\n", pFileName );
//        return NULL;
    }
    while ( fgets(pBuffer, MaxLine, pFile) != NULL )
    {
        // debug begin
        // std::cout << "the value in pBuffer is: " << pBuffer << std::endl;
        // debug end

        iLine++;
        if ( pBuffer[0] == 'c' )    // the comment line
            continue;
        if ( pBuffer[0] == 'p' )    // the header line
        {
            pToken = strtok( pBuffer+1, " \t" );
            if ( strcmp(pToken, "cnf") )    // the header line should look like "p cnf ..."
            {
                printf( "Incorrect input file.\n" );
                return false;
            }
            pToken = strtok( NULL, " \t" );
            nVars = atoi( pToken );
            pToken = strtok( NULL, " \t" );
            nClas = atoi( pToken );
            if ( nVars <= 0 || nClas <= 0 )
            {
                printf( "Incorrect parameters.\n" );
                return false;
            }
            // temp storage
            // debug begin
            // std::cout << "the value of nClas is: " << nClas << std::endl;
            // debug end

            * pvClas = Vec_IntAlloc( nClas+1 );
            * pvLits = Vec_IntAlloc( nClas*8 );
            continue;
        }
        pToken = strtok( pBuffer, " \t\r\n" );
        if ( pToken == NULL )   // the empty line
            continue;
        Vec_IntPush( * pvClas, Vec_IntSize(* pvLits) );   // the vector <vClas> actually stores the index of the first literal
        // in each clause, which is equal to the size of <vLits>
        while ( pToken )
        {
            int Var = atoi( pToken );
            Lit = ( Var >= 0 ) ? Var : -Var; // Lit is the absolute value for Var
            if ( Lit == 0 ) // 0 marks the end of a clause
                break;
//            Lit = (Var > 0) ? Abc_Var2Lit(Var-1, 0) : Abc_Var2Lit(-Var-1, 1);   // Var should minus 1. It's true but I don't know why.
            if ( Lit > nVars )
            {
                printf( "Literal %d is out-of-bound for %d variables.\n", Lit, nVars );
                return false;
            }
            Vec_IntPush( * pvLits, Var );  // add the signed literal to the vector
            pToken = strtok( NULL, " \t\r\n" ); // read the next literal in the same clause
        }
        // now all the literals have been read. The last read variable should be 0.
        if ( Lit != 0 )
        {
            printf( "There is no zero-terminator in line %d.\n", iLine );
            return false;
        }
    }
    // now all the clauses should be read
    // finalize
    if ( Vec_IntSize(* pvClas) != nClas )
        printf( "Warning! The number of clauses (%d) is different from declaration (%d).\n", Vec_IntSize(* pvClas), nClas );
    Vec_IntPush( * pvClas, Vec_IntSize(* pvLits) );
    return true;
}

static void Cnf_FinishReadingFile ( FILE * pFile, Vec_Int_t ** pvClas, Vec_Int_t ** pvLits, char * pBuffer )
{
    fclose( pFile );
    Vec_IntFreeP( pvClas );
    Vec_IntFreeP( pvLits );
    ABC_FREE( pBuffer );
    delete pvClas;
    delete pvLits;
}

void
qdpll_print_qdimacs_output_to_file (QDPLL * qdpll, char * fileName)
{
  QDPLL_ABORT_QDPLL (!qdpll, "pointer to solver object is null!");
  /* NOTE: here we print the largest declared variable ID and the
     number of used original clauses. This might differ from the preamble
     of the original input file if that file was not strictly QDIMACS
     compliant or if clauses were removed. */
  const QDPLLResult result = qdpll->result;
  char *res_string;
  if (result == QDPLL_RESULT_UNKNOWN)
    res_string = "-1";
  else if (result == QDPLL_RESULT_SAT)
    res_string = "1";
  else if (result == QDPLL_RESULT_UNSAT)
    res_string = "0";
  else
    QDPLL_ABORT_QDPLL (1, "invalid result!");

  fprintf (stdout, "s cnf %s %d %d\n", res_string,
           qdpll->pcnf.max_declared_user_var_id, qdpll->pcnf.clauses.cnt);

  Scope *outer;
  assert (qdpll->pcnf.scopes.first);
  if (!qdpll->pcnf.user_scopes.first)
    {
      /* Formula is propositional; when unsatisfiable then cannot print
         countermodel. */
      if (result == QDPLL_RESULT_UNSAT)
        return;
      else
        outer = qdpll->pcnf.scopes.first;
    }
  else
    {
      if (result == QDPLL_RESULT_UNSAT)
        {
          /* Formula is unsatisfiable; cannot print countermodel if the
             outermost scope is existential or if the formula contains free
             variables, which by default are quantified existentially and
             leftmost. */
          if (qdpll->pcnf.user_scopes.first->type == QDPLL_QTYPE_EXISTS || 
              has_scope_free_user_var (qdpll, qdpll->pcnf.scopes.first))
            return;
          else
            outer = qdpll->pcnf.user_scopes.first;
        }
      else
        {
          assert (result == QDPLL_RESULT_SAT);
          /* Formula is satisfiable; cannot print countermodel if the
             outermost scope is universal AND if the formula DOES NOT contain free
             variables. */
          if (qdpll->pcnf.user_scopes.first->type == QDPLL_QTYPE_FORALL && 
              !has_scope_free_user_var (qdpll, qdpll->pcnf.scopes.first))
            return;
          else
            {
              outer = qdpll->pcnf.scopes.first;
              if (QDPLL_COUNT_STACK(qdpll->pcnf.scopes.first->vars) == 0)
                {
                  assert (outer->link.next);
                  assert (outer->link.next->type == outer->type);
                  outer = outer->link.next;
                }
            }
        }
    }

    FILE * fOut;
    fOut = fopen( fileName, "w+" );

  Var *vars = qdpll->pcnf.vars;
  VarID *p, *e;
  for (p = outer->vars.start, e = outer->vars.top; p < e; p++)
    {
      assert (*p);
      VarID id = *p;
      Var *var = VARID2VARPTR (vars, id);
      assert (!var->id || var->id == id);
      QDPLLAssignment a;
      /* FIX: Do not print assignments of internal variables, ignore also
         reset internal variables. */
      if ((!var->is_internal && id <= qdpll_get_max_declared_var_id (qdpll)) && 
          (a = qdpll_get_value (qdpll, id)) != QDPLL_ASSIGNMENT_UNDEF)
        fprintf (fOut, "V %d 0\n", 
                 a == QDPLL_ASSIGNMENT_FALSE ? -id : id);
    }
    
    fclose( fOut );
}