#include "abcApi.h"
#include <cmath>
#include <vector>
#include <iostream>

#define IN
#define OUT
#define INOUT

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// DESCRIPTION: to sort the input signals <a> and <b>, and output an array <out> with length 2, 
//              with <out[0]>=MIN(<a>, <b>) and <out[1]>=MAX(<z>, <b>)
static void Elementary_Sorting_Unit ( IN Abc_Ntk_t * pNtk, IN Abc_Obj_t * a, IN Abc_Obj_t * b, OUT std::vector<Abc_Obj_t *> &result );
// REQUIRES:    the input array <a[]> and <b[]> are all of length 2^k; k>=0
// DESCRIPTION: to merge two SORTED arrary of signals <a[]> and <b[]> into one sortedd array with length 2^{k+1}
static void k_Merging_Unit ( IN Abc_Ntk_t * pNtk, IN const std::vector<Abc_Obj_t *> &a, IN const std::vector<Abc_Obj_t *> &b, OUT const int k, std::vector<Abc_Obj_t *> &result );
// REQUIRES:    the input array <a[]> is of length 2^m; m>=1
// DESCRIPTION: return an array of signals <out[]> with length 2^m, which is the sorted elements in <a[]> in ascending order
static void Two_M_Input_Sorting_Module ( IN Abc_Ntk_t * pNtk, IN const std::vector<Abc_Obj_t *> &a, IN const int m, OUT std::vector<Abc_Obj_t *> &result );
// REQUIRES:    the input array <a[]> is of length n; n>=2
// DESCRIPTION: return an arrray of signals <out[]> with length n, which is the sorted elements in <a[]> in ascending order
extern void N_Input_Sorting_Network ( IN Abc_Ntk_t * pNtk, IN const std::vector<Abc_Obj_t *> &a, IN const int n, OUT std::vector<Abc_Obj_t *> &result );
// to check that the result of the sorting units or modules are correct
static void check_result ( IN std::vector<Abc_Obj_t *> &result, int size );