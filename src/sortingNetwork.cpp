#include "sortingNetwork.h"

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

void N_Input_Sorting_Network ( IN Abc_Ntk_t * pNtk, IN const std::vector<Abc_Obj_t *> &a, IN const int n, OUT std::vector<Abc_Obj_t *> &result )
{
    assert( pNtk );
    assert( a.size() == n );
    assert( n >= 1 );
    double intPart;
    if ( std::modf( log2( (double) n ), &intPart ) == 0.0 )     // n == 2^intPart + <the value returned by "modf">
    {
        Two_M_Input_Sorting_Module( pNtk, a, intPart, result );
        return;
    }
    
    if ( !result.empty() )  result.clear();
    result.resize( n, nullptr );
    std::vector<Abc_Obj_t *> Expanded_a( pow( 2, intPart + 1 ), nullptr );  // expand the input array with constant 1s
    std::vector<Abc_Obj_t *> tempResult;

    for ( int i = 0; i < n; ++i )
        Expanded_a[i] = a[i];
    for ( int i = n; i < pow( 2, intPart + 1 ); ++i )
        Expanded_a[i] = Abc_AigConst1( pNtk );  // constant 1s
    
    Two_M_Input_Sorting_Module( pNtk, Expanded_a, intPart + 1, tempResult );
    // to extract the first n elements as the result, and then return the result
    for ( int i = 0; i < n; ++i )
        result[i] = tempResult[i];
    check_result( result, n, pNtk );
}

void Elementary_Sorting_Unit ( IN Abc_Ntk_t * pNtk, IN Abc_Obj_t * a, IN Abc_Obj_t * b, OUT std::vector<Abc_Obj_t *> &result )
{
    assert( a && b && pNtk );
    if ( !result.empty() )  result.clear();
    result.resize( 2, nullptr );

    result[0] = Abc_AigAnd ( (Abc_Aig_t *) pNtk->pManFunc, a, b );

    /*
    // debug begin
    Abc_Obj_t * pTemp = Abc_AigAnd( (Abc_Aig_t *) pNtk->pManFunc, Abc_ObjNot( a ), Abc_ObjNot( b ) );
    Abc_Obj_t * pTemp2 = Abc_ObjNot( pTemp );
    assert( pTemp->pNtk == pNtk );
    assert( pTemp2->pNtk == pNtk );
    // debug end
    */

    result[1] = Abc_AigOr  ( (Abc_Aig_t *) pNtk->pManFunc, a, b );
    check_result( result, 2, pNtk );
}

void k_Merging_Unit ( IN Abc_Ntk_t * pNtk, IN const std::vector<Abc_Obj_t *> &a, IN const std::vector<Abc_Obj_t *> &b, OUT const int k, std::vector<Abc_Obj_t *> &result )
{
    assert( pNtk );
    assert( a.size() == pow( 2, k ) );
    assert( b.size() == pow( 2, k ) );
    assert( k >= 0 );

    if ( !result.empty() )  result.clear();
    result.resize( pow( 2, k+1 ), nullptr );

    // base case, elementary sorting unit
    if ( k == 0 )
    {
        Elementary_Sorting_Unit( pNtk, a[0], b[0], result );
        return;
    }
    
    assert( k >= 1 );
    std::vector<Abc_Obj_t *> tempInput1A( pow( 2, k-1 ), nullptr );
    std::vector<Abc_Obj_t *> tempInput1B( pow( 2, k-1 ), nullptr );
    std::vector<Abc_Obj_t *> tempInput2A( pow( 2, k-1 ), nullptr );
    std::vector<Abc_Obj_t *> tempInput2B( pow( 2, k-1 ), nullptr );
    std::vector<Abc_Obj_t *> tempResult1;
    std::vector<Abc_Obj_t *> tempResult2;

    // assign the values for inputs of two smaller merging units in recursive calls
    for ( int i = 0; i < pow( 2, k-1 ); ++i )
    {
        tempInput1A[i] = a[2*i];
        tempInput2A[i] = a[2*i+1];
        tempInput1B[i] = b[2*i];
        tempInput2B[i] = b[2*i+1];
    }

    // recursively call the function itself
    k_Merging_Unit( pNtk, tempInput1A, tempInput1B, k-1, tempResult1 );
    k_Merging_Unit( pNtk, tempInput2A, tempInput2B, k-1, tempResult2 );

    // deal with the results returned by two smaller merging units using elementary sorting units
    std::vector<Abc_Obj_t *> tempElementaryUnitResult( 2, nullptr );
    result[0]               = tempResult1[0];
    result[pow( 2, k+1 )-1] = tempResult2[pow( 2, k )-1];
    for ( int i = 1; i < pow( 2, k ); ++i )
    {
        Elementary_Sorting_Unit( pNtk, tempResult1[i], tempResult2[i-1], tempElementaryUnitResult );
        result[2*i-1]   = tempElementaryUnitResult[0];
        result[2*i]     = tempElementaryUnitResult[1];
    }
    check_result( result, pow( 2, k+1 ), pNtk );
}

void Two_M_Input_Sorting_Module ( IN Abc_Ntk_t * pNtk, IN const std::vector<Abc_Obj_t *> &a, IN const int m, OUT std::vector<Abc_Obj_t *> &result )
{
    assert( pNtk );
    assert( a.size() == pow( 2, m ) );
    assert( m >= 1 );

    if ( !result.empty() )  result.clear();
    result.resize( pow( 2, m ), nullptr );

    // base case
    if ( m == 1 )
    {
        Elementary_Sorting_Unit( pNtk, a[0], a[1], result );
        return;
    }

    assert( m >= 2 );
    std::vector<Abc_Obj_t *> tempInput1( pow( 2, m-1 ), nullptr );
    std::vector<Abc_Obj_t *> tempInput2( pow( 2, m-1 ), nullptr );
    std::vector<Abc_Obj_t *> tempResult1;
    std::vector<Abc_Obj_t *> tempResult2;

    // assign the values for inputs of two smaller sorting networks in recursive calls
    for ( int i = 0; i < pow( 2, m-1 ); ++i )
    {
        tempInput1[i] = a[i];
        tempInput2[i] = a[pow(2, m-1)+i];
    }

    // recursively call the function itself
    Two_M_Input_Sorting_Module( pNtk, tempInput1, m-1, tempResult1 );
    Two_M_Input_Sorting_Module( pNtk, tempInput2, m-1, tempResult2 );

    // deal with the results returned by two smaller sorting network using k_merging_unit
    k_Merging_Unit( pNtk, tempResult1, tempResult2, m-1, result );
    check_result( result, pow( 2, m ), pNtk );
}

void check_result ( IN std::vector<Abc_Obj_t *> &result, IN int size, IN Abc_Ntk_t * pNtk )
{
    // size matches the expectation
    assert( result.size() == size );
    // no element is a null pointer
    for ( int i = 0; i < size; ++i )
    {
        assert( result[i] );
        assert( Abc_ObjRegular( result[i] )->pNtk == pNtk );
    }
}