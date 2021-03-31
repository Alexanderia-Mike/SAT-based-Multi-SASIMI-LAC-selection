#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "qdpll.h"
#include "abcApi.h"
#include <iostream>

// add the selection signals in <MUXPIIDs> to existential quantifiers, and add Original PIs in <OriPIIDs> to universal quantifiers
static void QDPLL_AddQuantifiers( QDPLL * depqbf, int * OriPIIDs, int * MUXPIIDs );
// read the cnf information from file with the name <pFileName>, and store the information in vectors
//  If reading is successful, return true, otherwise return false
static bool Cnf_DataFromFile ( char * pFileName, Vec_Int_t * vClas, Vec_Int_t * vLits, char * pBuffer, FILE * pFile );
// finish reading the file and free the vectors
static void Cnf_FinishReadingFile ( FILE * pFile, Vec_Int_t * vClas, Vec_Int_t * vLits, char * pBuffer );

// read the cnf from a "cnf" file, then store the data in <depqbf>
void Cnf_DataFile2Depqbf( char * pFileName, QDPLL * depqbf, int * OriPIIDs, int * MUXPIIDs )
{
    Vec_Int_t * vClas = NULL;   // vector of clauses
    Vec_Int_t * vLits = NULL;   // vector of literals
    char * pBuffer; int Entry, i;
    FILE * pFile = fopen( pFileName, "rb" );
    // read cnf information from <pFileName>
    if ( !Cnf_DataFromFile( pFileName, vClas, vLits, pBuffer, pFile ) )
        goto finish;
    // create quantifiers
    QDPLL_AddQuantifiers( depqbf, OriPIIDs, MUXPIIDs );
    // add clauses
    Vec_IntForEachEntry( vClas, Entry, i )
    {
        if ( i == Vec_IntSize( vClas ) - 1 )    // the last index in <vClas> is invalid
            break;
        for ( int j = Entry; j < Vec_IntEntry( vClas, i + 1 ); ++j )
            qdpll_add( depqbf, Vec_IntEntry( vLits, j ) );  // add the corresponding literals in <vLits>
        qdpll_add( depqbf, 0 ); // add the terminal 0
    }
    // print
    qdpll_print (depqbf, stdout);
    // solve the SAT problem
    QDPLLResult res = qdpll_sat (depqbf);
    if ( res == QDPLL_RESULT_SAT )
        std::cout << "SAT! There exists a possible LAC!" << std::endl;
    else if ( res == QDPLL_RESULT_UNSAT )
        std::cout << "UNSAT! No possible LAC exists!" << std::endl;
    else
        std::cout << "UNKNOWN! Cannot figure out whether there exists a LAC!" << std::endl;

    // close the file and free the memory
    finish:
    Cnf_FinishReadingFile( pFile, vClas, vLits, pBuffer );
}


/************************************ HELPING FUNCTIONS ************************************/

static void QDPLL_AddQuantifiers( QDPLL * depqbf, int * OriPIIDs, int * MUXPIIDs )
{
    // add universal quantifiers to depqbf
    qdpll_new_scope_at_nesting (depqbf, QDPLL_QTYPE_FORALL, 1);
    for ( int i = 0; i < OriPIIDs[0]; ++i )
        qdpll_add( depqbf, OriPIIDs[i+1] );
    qdpll_add( depqbf, 0 );
    // add existential quantifiers to depqbf
    qdpll_new_scope_at_nesting (depqbf, QDPLL_QTYPE_EXISTS, 1);
    for ( int i = 0; i < MUXPIIDs[0]; ++i )
        qdpll_add( depqbf, MUXPIIDs[i+1] );
    qdpll_add( depqbf, 0 );
}

static bool Cnf_DataFromFile ( char * pFileName, Vec_Int_t * vClas, Vec_Int_t * vLits, char * pBuffer, FILE * pFile )
{
    int MaxLine = 1000000;
    int Lit, nVars = -1, nClas = -1, iLine = 0;
    char * pToken;
    if ( pFile == NULL )
    {
        printf( "Cannot open file \"%s\" for writing.\n", pFileName );
//        return NULL;
    }
    pBuffer = ABC_ALLOC( char, MaxLine );
    while ( fgets(pBuffer, MaxLine, pFile) != NULL )
    {
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
            vClas = Vec_IntAlloc( nClas+1 );
            vLits = Vec_IntAlloc( nClas*8 );
            continue;
        }
        pToken = strtok( pBuffer, " \t\r\n" );
        if ( pToken == NULL )   // the empty line
            continue;
        Vec_IntPush( vClas, Vec_IntSize(vLits) );   // the vector <vClas> actually stores the index of the first literal
        // in each clause, which is equal to the size of <vLits>
        while ( pToken )
        {
            Lit = atoi( pToken );
            if ( Lit == 0 ) // 0 marks the end of a clause
                break;
//            Lit = (Var > 0) ? Abc_Var2Lit(Var-1, 0) : Abc_Var2Lit(-Var-1, 1);   // Var should minus 1. It's true but I don't know why.
            if ( Lit > nVars )
            {
                printf( "Literal %d is out-of-bound for %d variables.\n", Lit, nVars );
                return false;
            }
            Vec_IntPush( vLits, Lit );  // add the literal to the vector
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
    if ( Vec_IntSize(vClas) != nClas )
        printf( "Warning! The number of clauses (%d) is different from declaration (%d).\n", Vec_IntSize(vClas), nClas );
    Vec_IntPush( vClas, Vec_IntSize(vLits) );
    return true;
}

static void Cnf_FinishReadingFile ( FILE * pFile, Vec_Int_t * vClas, Vec_Int_t * vLits, char * pBuffer )
{
    fclose( pFile );
    Vec_IntFreeP( &vClas );
    Vec_IntFreeP( &vLits );
    ABC_FREE( pBuffer );
}