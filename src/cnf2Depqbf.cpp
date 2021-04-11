#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "qdpll.h"
#include "abcApi.h"
#include <iostream>

// add the selection signals in <MUXPIIDs> to existential quantifiers, and add Original PIs in <OriPIIDs> to universal quantifiers
static void QDPLL_AddQuantifiers( QDPLL * depqbf, int * OriPIIDs, int * MUXPIIDs, int nVars );
// read the cnf information from file with the name <pFileName>, and store the information in vectors
//  If reading is successful, return true, otherwise return false
static bool Cnf_DataFromFile ( char * pFileName, Vec_Int_t ** pvClas, Vec_Int_t ** pvLits, char * pBuffer, FILE * pFile, int MaxLine, int * pnVars );
// finish reading the file and free the vectors
static void Cnf_FinishReadingFile ( FILE * pFile, Vec_Int_t ** pvClas, Vec_Int_t ** pvLits, char * pBuffer );

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

    // debug begin
    std::cout << "1. before anything: " << std::endl;
    qdpll_print (depqbf, stdout);
    std::cout << "the number of variables in cnf is " << nVars << std::endl;
    // debug end

    // create quantifiers
    QDPLL_AddQuantifiers( depqbf, OriPIIDs, MUXPIIDs, nVars );

    // debug begin
    std::cout << "2. after adding quantifiers: " << std::endl;
    qdpll_print (depqbf, stdout);
    // debug end
    
    // debug begin
    std::cout << "the values in vClas are listed as follows:" << std::endl;
    Vec_IntForEachEntry( vClas, Entry, i )
        std::cout << Entry << " ";
    std::cout << std::endl << "the values in vLits are listed as follows:" << std::endl;
    Vec_IntForEachEntry( vLits, Entry, i )
        std::cout << Entry << " ";
    std::cout << std::endl;
    // debug end

    /*
    // debug version begin
    // add clauses
    Vec_IntForEachEntry( vClas, Entry, i )
    {
        std::cout << std::endl << "NEW ROUND! the int i = " << i << "; Vec_IntSize( vClas ) = " << Vec_IntSize( vClas ) << std::endl;
        if ( i == Vec_IntSize( vClas ) - 1 )    // the last index in <vClas> is invalid
            break;
        for ( int j = Entry; j < Vec_IntEntry( vClas, i + 1 ); ++j )
        {
            std::cout << "the int j = " << j << "; Vec_IntEntry( vLits, j ) = " << Vec_IntEntry( vLits, j ) << std::endl;
            qdpll_add( depqbf, Vec_IntEntry( vLits, j ) );  // add the corresponding literals in <vLits>
        }
        qdpll_add( depqbf, 0 ); // add the terminal 0
    }
    // debug version end
    */
    
    // add clauses
    Vec_IntForEachEntry( vClas, Entry, i )
    {
        if ( i == Vec_IntSize( vClas ) - 1 )    // the last index in <vClas> is invalid
            break;
        for ( int j = Entry; j < Vec_IntEntry( vClas, i + 1 ); ++j )
            qdpll_add( depqbf, Vec_IntEntry( vLits, j ) );  // add the corresponding literals in <vLits>
        qdpll_add( depqbf, 0 ); // add the terminal 0

        // debug begin
        // qdpll_print( depqbf, stdout );
        // debug end

    }

    // print
    std::cout << "3. after adding everything: " << std::endl;
    qdpll_print (depqbf, stdout);

    // close the file and free the memory
    finish:
    Cnf_FinishReadingFile( pFile, pvClas, pvLits, pBuffer );
}

// solve the sat problem stored in <depqbf> and if SAT or UNSAT, return the assignment of variables in the file named <pFileName>
QDPLLResult QDPLL_SolveSatWriteAssignments( QDPLL * depqbf, char * pFileName )
{
    FILE * pOut;
    // solve the SAT problem
    QDPLLResult res = qdpll_sat (depqbf);
    if ( res == QDPLL_RESULT_UNKNOWN )
        std::cout << "UNKNOWN! Cannot figure out whether there exists a LAC!" << std::endl;
    else
    {
        if ( res == QDPLL_RESULT_SAT )
            std::cout << "SAT! There exists a possible LAC!" << std::endl << "The solution is:" << std::endl;
        else
            std::cout << "UNSAT! No possible LAC exists!" << std::endl << "The UNSAT core is:" << std::endl;
        qdpll_print_qdimacs_output ( depqbf );
    }
    qdpll_reset( depqbf );
    pOut = fopen( pFileName, "w+" );
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
