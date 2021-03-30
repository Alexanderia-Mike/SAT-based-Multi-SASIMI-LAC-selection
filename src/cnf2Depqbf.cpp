#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "qdpll.h"
#include "abcApi.h"

// read the cnf from a "cnf" file, then store the data in <depqbf>
void Cnf_DataFile2Depqbf( char * pFileName, QDPLL * depqbf )
{
    int MaxLine = 1000000;
    int Var, Lit, nVars = -1, nClas = -1, i, Entry, iLine = 0;
//    Cnf_Dat_t * pCnf = NULL;
    Vec_Int_t * vClas = NULL;
    Vec_Int_t * vLits = NULL;
    char * pBuffer, * pToken;
    FILE * pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
    {
        printf( "Cannot open file \"%s\" for writing.\n", pFileName );
//        return NULL;
    }
    pBuffer = ABC_ALLOC( char, MaxLine );
    while ( fgets(pBuffer, MaxLine, pFile) != NULL )
    {
        iLine++;
        if ( pBuffer[0] == 'c' )
            continue;
        if ( pBuffer[0] == 'p' )
        {
            pToken = strtok( pBuffer+1, " \t" );
            if ( strcmp(pToken, "cnf") )
            {
                printf( "Incorrect input file.\n" );
                goto finish;
            }
            pToken = strtok( NULL, " \t" );
            nVars = atoi( pToken );
            pToken = strtok( NULL, " \t" );
            nClas = atoi( pToken );
            if ( nVars <= 0 || nClas <= 0 )
            {
                printf( "Incorrect parameters.\n" );
                goto finish;
            }
            // temp storage
            vClas = Vec_IntAlloc( nClas+1 );
            vLits = Vec_IntAlloc( nClas*8 );
            continue;
        }
        pToken = strtok( pBuffer, " \t\r\n" );
        if ( pToken == NULL )
            continue;
        Vec_IntPush( vClas, Vec_IntSize(vLits) );
        while ( pToken )
        {
            Var = atoi( pToken );
            if ( Var == 0 )
                break;
            Lit = (Var > 0) ? Abc_Var2Lit(Var-1, 0) : Abc_Var2Lit(-Var-1, 1);
            if ( Lit >= 2*nVars )
            {
                printf( "Literal %d is out-of-bound for %d variables.\n", Lit, nVars );
                goto finish;
            }
            Vec_IntPush( vLits, Lit );
            pToken = strtok( NULL, " \t\r\n" );
        }
        if ( Var != 0 )
        {
            printf( "There is no zero-terminator in line %d.\n", iLine );
            goto finish;
        }
    }
    // finalize
    if ( Vec_IntSize(vClas) != nClas )
        printf( "Warning! The number of clauses (%d) is different from declaration (%d).\n", Vec_IntSize(vClas), nClas );
    Vec_IntPush( vClas, Vec_IntSize(vLits) );
    // create
    pCnf = ABC_CALLOC( Cnf_Dat_t, 1 );
    pCnf->nVars     = nVars;
    pCnf->nClauses  = Vec_IntSize(vClas)-1;
    pCnf->nLiterals = Vec_IntSize(vLits);
    pCnf->pClauses  = ABC_ALLOC( int *, Vec_IntSize(vClas) );
    pCnf->pClauses[0] = Vec_IntReleaseArray(vLits);
    Vec_IntForEachEntry( vClas, Entry, i )
    pCnf->pClauses[i] = pCnf->pClauses[0] + Entry;
    finish:
    fclose( pFile );
    Vec_IntFreeP( &vClas );
    Vec_IntFreeP( &vLits );
    ABC_FREE( pBuffer );
    return pCnf;
}