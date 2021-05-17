#include "LEXSAT.h"
// This LEXSAT procedure should be called with a set of literals (pLits, nLits),
// which defines both (1) variable order, and (2) assignment to begin search from.
// It retuns the LEXSAT assigment that is the same or larger than the given one.
// (It assumes that there is no smaller assignment than the one given!)
// The resulting assignment is returned in the same set of literals (pLits, nLits).
// It pushes/pops assumptions internally and will undo them before terminating.
int sat_solver_solve_lexsat_xwh( sat_solver* s, int * pLits, int nLits )
{
    int i, iLitFail = -1;
    lbool status;
    assert( nLits > 0 );
    // help the SAT solver by setting desirable polarity
    sat_solver_set_literal_polarity( s, pLits, nLits );
    // check if there exists a satisfying assignment
    status = sat_solver_solve_internal( s );
    if ( status != l_True ) // no assignment
        return status;
    // there is at least one satisfying assignment
    assert( status == l_True );
    // find the first mismatching literal
    for ( i = 0; i < nLits; i++ )
        if ( pLits[i] != sat_solver_var_literal(s, Abc_Lit2Var(pLits[i])) )
            break;
    if ( i == nLits ) // no mismatch - the current assignment is the minimum one!
        return l_True;
    // mismatch happens in literal i
    iLitFail = i;
    // create assumptions up to this literal (as in pLits) - including this literal!
    for ( i = 0; i <= iLitFail; i++ )
        if ( !sat_solver_push(s, pLits[i]) ) // can become UNSAT while adding the last assumption
            break;
    if ( i < iLitFail + 1 ) // the solver became UNSAT while adding assumptions
        status = l_False;
    else // solve under the assumptions
        status = sat_solver_solve_internal( s );
    if ( status == l_True )
    {
        // we proved that there is a sat assignment with literal (iLitFail) having polarity as in pLits
        // continue solving recursively 
        if ( iLitFail + 1 < nLits )
            status = sat_solver_solve_lexsat_xwh( s, pLits + iLitFail + 1, nLits - iLitFail - 1 );
    }
    else if ( status == l_False )
    {
        // we proved that there is no assignment with iLitFail having polarity as in pLits
        assert( Abc_LitIsCompl(pLits[iLitFail]) ); // literal is 0 
        // (this assert may fail only if there is a sat assignment smaller than one originally given in pLits)
        // now we flip this literal (make it 1), change the last assumption
        // and contiue looking for the 000...0-assignment of other literals
        sat_solver_pop( s );
        pLits[iLitFail] = Abc_LitNot(pLits[iLitFail]);
        if ( !sat_solver_push(s, pLits[iLitFail]) )
            printf( "sat_solver_solve_lexsat(): A satisfying assignment should exist.\n" ); // because we know that the problem is satisfiable
        // update other literals to be 000...0
        for ( i = iLitFail + 1; i < nLits; i++ )
            pLits[i] = Abc_LitNot( Abc_LitRegular(pLits[i]) ); //ensure that there is no satisfying assignment less than pLits
        // continue solving recursively
        if ( iLitFail + 1 < nLits )
            status = sat_solver_solve_lexsat_xwh( s, pLits + iLitFail + 1, nLits - iLitFail - 1 );
        else
            status = l_True;
    }
    // undo the assumptions
    for ( i = iLitFail; i >= 0; i-- )
        sat_solver_pop( s );
    return status;
}

int sat_solver_solve_maxlexsat_xwh( sat_solver* s, int * pLits, int nLits )
{
    int i, iLitFail = -1;
    lbool status;
    assert( nLits > 0 );
    // help the SAT solver by setting desirable polarity
    sat_solver_set_literal_polarity( s, pLits, nLits );
    // check if there exists a satisfying assignment
    status = sat_solver_solve_internal( s );
    if ( status != l_True ) // no assignment
        return status;
    // there is at least one satisfying assignment
    assert( status == l_True );
    // find the first mismatching literal
    for ( i = 0; i < nLits; i++ )
        if ( pLits[i] != sat_solver_var_literal(s, Abc_Lit2Var(pLits[i])) )
            break;
    if ( i == nLits ) // no mismatch - the current assignment is the minimum one!
        return l_True;
    // mismatch happens in literal i
    iLitFail = i;
    // create assumptions up to this literal (as in pLits) - including this literal!
    for ( i = 0; i <= iLitFail; i++ )
        if ( !sat_solver_push(s, pLits[i]) ) // can become UNSAT while adding the last assumption
            break;
    if ( i < iLitFail + 1 ) // the solver became UNSAT while adding assumptions
        status = l_False;
    else // solve under the assumptions
        status = sat_solver_solve_internal( s );
    if ( status == l_True )
    {
        // we proved that there is a sat assignment with literal (iLitFail) having polarity as in pLits
        // continue solving recursively 
        if ( iLitFail + 1 < nLits )
            status = sat_solver_solve_maxlexsat_xwh( s, pLits + iLitFail + 1, nLits - iLitFail - 1 );
    }
    else if ( status == l_False )
    {
        // we proved that there is no assignment with iLitFail having polarity as in pLits
        assert( !Abc_LitIsCompl(pLits[iLitFail]) ); // literal is 1 
        // (this assert may fail only if there is a sat assignment larger than one originally given in pLits)
        // now we flip this literal (make it 0), change the last assumption
        // and contiue looking for the 111...1-assignment of other literals
        sat_solver_pop( s );
        pLits[iLitFail] = Abc_LitNot(pLits[iLitFail]);
        if ( !sat_solver_push(s, pLits[iLitFail]) )
            printf( "sat_solver_solve_lexsat(): A satisfying assignment should exist.\n" ); // because we know that the problem is satisfiable
        // update other literals to be 111...1
        for ( i = iLitFail + 1; i < nLits; i++ )
            pLits[i] = Abc_LitRegular(pLits[i]); //ensure that there is no satisfying assignment less than pLits
        // continue solving recursively
        if ( iLitFail + 1 < nLits )
            status = sat_solver_solve_maxlexsat_xwh( s, pLits + iLitFail + 1, nLits - iLitFail - 1 );
        else
            status = l_True;
    }
    // undo the assumptions
    for ( i = iLitFail; i >= 0; i-- )
        sat_solver_pop( s );
    return status;
}

/**Function*************************************************************

  Synopsis    [Derives a simple CNF for the AIG.]

  Description [The last argument lists the number of last outputs
  of the manager, which will not be converted into clauses. 
  New variables will be introduced for these outputs.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cnf_Dat_t * Cnf_DeriveSimple_xwh( Aig_Man_t * p, int nOutputs )
{
    Aig_Obj_t * pObj;
    Cnf_Dat_t * pCnf;
    int OutVar, PoVar, pVars[32], * pLits, ** pClas;
    int i, nLiterals, nClauses, Number;

    // count the number of literals and clauses
    nLiterals = 1 + 7 * Aig_ManNodeNum(p) + Aig_ManCoNum( p ) + 3 * nOutputs;
    nClauses = 1 + 3 * Aig_ManNodeNum(p) + Aig_ManCoNum( p ) + nOutputs;

    // allocate CNF
    pCnf = ABC_ALLOC( Cnf_Dat_t, 1 );
    memset( pCnf, 0, sizeof(Cnf_Dat_t) );
    pCnf->pMan = p;
    pCnf->nLiterals = nLiterals;
    pCnf->nClauses = nClauses;
    pCnf->pClauses = ABC_ALLOC( int *, nClauses + 1 );
    pCnf->pClauses[0] = ABC_ALLOC( int, nLiterals );
    pCnf->pClauses[nClauses] = pCnf->pClauses[0] + nLiterals;

    // create room for variable numbers
    pCnf->pVarNums = ABC_ALLOC( int, Aig_ManObjNumMax(p) );
//    memset( pCnf->pVarNums, 0xff, sizeof(int) * Aig_ManObjNumMax(p) );
    for ( i = 0; i < Aig_ManObjNumMax(p); i++ )
        pCnf->pVarNums[i] = -1;
    // assign variables to the last (nOutputs) POs
    Number = 0;
    if ( nOutputs )
    {
//        assert( nOutputs == Aig_ManRegNum(p) );
//        Aig_ManForEachLiSeq( p, pObj, i )
//            pCnf->pVarNums[pObj->Id] = Number++;
        Aig_ManForEachCo( p, pObj, i )
            pCnf->pVarNums[pObj->Id] = Number++;
    }
    // assign variables to the internal nodes
    Aig_ManForEachNode( p, pObj, i )
        pCnf->pVarNums[pObj->Id] = Number++;
    // assign variables to the PIs and constant node
    Aig_ManForEachCi( p, pObj, i )
        pCnf->pVarNums[pObj->Id] = Number++;
    pCnf->pVarNums[Aig_ManConst1(p)->Id] = Number++;
    pCnf->nVars = Number;
/*
    // print CNF numbers
    printf( "SAT numbers of each node:\n" );
    Aig_ManForEachObj( p, pObj, i )
        printf( "%d=%d ", pObj->Id, pCnf->pVarNums[pObj->Id] );
    printf( "\n" );
*/
    // assign the clauses
    pLits = pCnf->pClauses[0];
    pClas = pCnf->pClauses;
    Aig_ManForEachNode( p, pObj, i )
    {
        OutVar   = pCnf->pVarNums[ pObj->Id ];
        pVars[0] = pCnf->pVarNums[ Aig_ObjFanin0(pObj)->Id ];
        pVars[1] = pCnf->pVarNums[ Aig_ObjFanin1(pObj)->Id ];

        // positive phase
        *pClas++ = pLits;
        *pLits++ = 2 * OutVar; 
        *pLits++ = 2 * pVars[0] + !Aig_ObjFaninC0(pObj); 
        *pLits++ = 2 * pVars[1] + !Aig_ObjFaninC1(pObj); 
        // negative phase
        *pClas++ = pLits;
        *pLits++ = 2 * OutVar + 1; 
        *pLits++ = 2 * pVars[0] + Aig_ObjFaninC0(pObj); 
        *pClas++ = pLits;
        *pLits++ = 2 * OutVar + 1; 
        *pLits++ = 2 * pVars[1] + Aig_ObjFaninC1(pObj); 
    }
 
    // write the constant literal
    OutVar = pCnf->pVarNums[ Aig_ManConst1(p)->Id ];
    assert( OutVar <= Aig_ManObjNumMax(p) );
    *pClas++ = pLits;
    *pLits++ = 2 * OutVar;  

    // write the output literals
    Aig_ManForEachCo( p, pObj, i )
    {
        OutVar = pCnf->pVarNums[ Aig_ObjFanin0(pObj)->Id ];
        if ( i < Aig_ManCoNum(p) - nOutputs )
        {
            *pClas++ = pLits;
            *pLits++ = 2 * OutVar + Aig_ObjFaninC0(pObj); 
        }
        else
        {
            PoVar  = pCnf->pVarNums[ pObj->Id ];
            // first clause
            *pClas++ = pLits;
            *pLits++ = 2 * PoVar; 
            *pLits++ = 2 * OutVar + !Aig_ObjFaninC0(pObj); 
            // second clause
            *pClas++ = pLits;
            *pLits++ = 2 * PoVar + 1; 
            *pLits++ = 2 * OutVar + Aig_ObjFaninC0(pObj); 
        }
    }

    // verify that the correct number of literals and clauses was written
    assert( pLits - pCnf->pClauses[0] == nLiterals );
    assert( pClas - pCnf->pClauses == nClauses );
    return pCnf;
}