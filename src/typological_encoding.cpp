#include "Typological_Structure_Encoded_Sat_Based_Multi_Sel_Manager.h"
#include <algorithm>

using namespace std;

// create a adjacent matrix where 
void 
SBMSM_t::get_transitive_fanin_cone_using_matrix()
{
    /* declarations */
    if ( pAppNtk == nullptr )
    {
        cout << "get_transitive_fanin_cone_using_matrix: no network specified!" << endl;
        exit( 0 );
    }
    int max_id = get_max_id();
    Abc_Obj_t * pObj = nullptr, * pPI = nullptr, * pFanin = nullptr;
    int i = 0, j = 0;
    int object_num_div_64 = ( max_id >> 6 ) + 1;

    /* resize <transitive_fanin_cone_matrix> to the correct size */
    transitive_fanin_cone_matrix.resize( max_id + 1 );
    for ( i = 0; i < transitive_fanin_cone_matrix.size(); ++i )
    // Abc_NtkForEachObj( pAppNtk, pObj, i )
        transitive_fanin_cone_matrix[i].resize( object_num_div_64, 0 );
    
    /* set PIs as 1 */
    Abc_NtkForEachPi( pAppNtk, pPI, i )
        Ckt_SetBit( transitive_fanin_cone_matrix[pPI->Id][pPI->Id >> 6], pPI->Id );   // transitive_fanin_cone_matrix[pObj->Id][i/64] 的第 i%64 位设置成 1

    /* updating all the transitive fanin cones in DFS order */
    Vec_Ptr_t * vNodes = Abc_NtkDfs( pAppNtk, 0 );
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i ) {
        Abc_ObjForEachFanin( pObj, pFanin, j ) {
            Ckt_SetBit( transitive_fanin_cone_matrix[pObj->Id][pFanin->Id >> 6], pFanin->Id );  // set up the flag for the fanins
            Ckt_SetBit( transitive_fanin_cone_matrix[pObj->Id][pObj->Id >> 6], pObj->Id );      // set up the flag for the node itself
            for ( int k = 0; k < object_num_div_64; ++k )
            {
                transitive_fanin_cone_matrix[pObj->Id][k] |= transitive_fanin_cone_matrix[pFanin->Id][k];
            }
        }
    }
}

int 
SBMSM_t::get_max_id()
{
    Abc_Obj_t * pObj; 
    int i = 0; 
    int maxId = -1; 
    Abc_NtkForEachObj(pAppNtk, pObj, i) 
        maxId = max(maxId, pObj->Id);
    return maxId;
}

void
SBMSM_t::print_transitive_fanin_cone_matrix()
{
    if ( transitive_fanin_cone_matrix.empty() )
    {
        cout << "print_transitive_fanin_cone_matrix: empty matrix!" << endl;
    }
    cout << "now printing the transitive fanin cone matrix!" << endl;
    for ( int i = 0; i < transitive_fanin_cone_matrix.size(); ++i )
    {
        for ( int j = 0; j < transitive_fanin_cone_matrix.size(); ++j )
        {
            cout << Ckt_GetBit( transitive_fanin_cone_matrix[i][j>>6], j ) << " ";
        }
        cout << endl;
    }
}

/* C_IJ 
 * = [[ LAC i can be applied before LAC j ]] 
 * = ( assume LAC i: x --> y; LAC j: z --> w )
 * = [[ z not in MFFC(x)\TFC(x) && w not in MFFC(x)\TFC(x) ]] && [[ z not in MFFC(y)\TFC(y) && w not in MFFC(y)\TFC(y) ]]
 * = NOT [[ z in MFFC(x)\TFC(x) || w in MFFC(x)\TFC(x) ]] && NOT [[ z in MFFC(y)\TFC(y) || w in MFFC(y)\TFC(y) ]]
 * = NOT [[ z in MFFC(x) && z not in TFC(x) || w in MFFC(x) && w not in TFC(x) ]] && NOT [[ z in MFFC(y) && z not in TFC(y) || w in MFFC(y) && w not in TFC(y) ]] 
 * = NOT [[ z in MFFC(x) && z not in TFC(x) || w in MFFC(x) && w not in TFC(x) || z in MFFC(y) && z not in TFC(y) || w in MFFC(y) && w not in TFC(y) ]] 
 * = NOT 
        [[ 
            z in MFFC(x) && z not in TFC(x) || 
            w in MFFC(x) && w not in TFC(x) || 
            z in MFFC(y) && z not in TFC(y) || 
            w in MFFC(y) && w not in TFC(y) 
        ]] 
 */
void 
SBMSM_t::get_LAC_priority_factor()
{
    /* declarations */
    Abc_Obj_t * x, * y, * z, * w;
    bool z_in_x_mffc = false, w_in_x_mffc = false, z_in_y_mffc = false, w_in_y_mffc = false;

    if ( candLACs.empty() )
    {
        cout << "get_LAC_priority_factor: candLAC is empty" << endl;
        exit( 0 );
    }
    unsigned int LAC_num = candLACs.size();
    unsigned int LAC_num_div_64 = ( LAC_num >> 6 ) + ( ( LAC_num & ( ( 1 << 6 ) - 1 ) ) == 0 ? 0 : 1 ) ;
    
    /* initialize the matrix for LAC priority */ 
    LAC_priority_matrix.resize( LAC_num );
    for ( int i = 0; i < LAC_num; ++i )
    {
        LAC_priority_matrix[i].resize( LAC_num_div_64 );
    }

    /* assign values to the matrix */
    for ( int i = 0; i < candLACs.size(); ++i )
    {
        x = candLACs[i].GetSS();
        y = candLACs[i].GetTS();
        for ( int j = 0; j < candLACs.size(); ++j )
        {
            z = candLACs[j].GetSS();
            w = candLACs[j].GetTS();
            /* check whether z is in MffC(x) */
            z_in_x_mffc = check_a_in_b_mffc( z, x );
            z_in_y_mffc = check_a_in_b_mffc( z, y );
            w_in_x_mffc = check_a_in_b_mffc( w, x );
            w_in_y_mffc = check_a_in_b_mffc( w, y );
            if
            ( !
                z_in_x_mffc && ! Ckt_GetBit( transitive_fanin_cone_matrix[x->Id][z->Id >> 6], z->Id ) ||
                w_in_x_mffc && ! Ckt_GetBit( transitive_fanin_cone_matrix[x->Id][w->Id >> 6], w->Id ) ||
                z_in_y_mffc && ! Ckt_GetBit( transitive_fanin_cone_matrix[y->Id][z->Id >> 6], z->Id ) ||
                w_in_y_mffc && ! Ckt_GetBit( transitive_fanin_cone_matrix[y->Id][w->Id >> 6], w->Id )
            )
            {
                Ckt_SetBit( LAC_priority_matrix[i][j>>6], j );
            }
        }
    }
}

void 
SBMSM_t::print_LAC_priority_factor()
{
    if ( LAC_priority_matrix.empty() )
    {
        cout << "print_LAC_priority_factor: empty matrix!" << endl;
    }
    cout << "now printing the LAC priority matrix!" << endl;
    for ( int i = 0; i < LAC_priority_matrix.size(); ++i )
    {
        for ( int j = 0; j < LAC_priority_matrix.size(); ++j )
        {
            cout << Ckt_GetBit( LAC_priority_matrix[i][j>>6], j ) << " ";
        }
        cout << endl;
    }
}

bool 
SBMSM_t::check_a_in_b_mffc( Abc_Obj_t * a, Abc_Obj_t * b )
{
    /* declarations */
    Vec_Ptr_t * b_mffc;
    Abc_Obj_t * pObj;
    int k;

    /* begin check */
    b_mffc = Vec_PtrAlloc(100);
    Abc_NodeDeref_rec( b );
    /* when b is not a node, use different strategies to deal with that */
    if ( Abc_ObjIsPi( b ) )
        return false;
    if ( Abc_ObjIsPo( b ) )
    {
        Abc_Obj_t * pOutFanin = Abc_ObjFanin0( b );
        return check_a_in_b_mffc( a, pOutFanin );
    }
    /* get the mffc only when b is a node */
    Abc_NodeMffcConeSupp( b, b_mffc, nullptr );
    Abc_NodeRef_rec( b );
    Vec_PtrForEachEntry( Abc_Obj_t *, b_mffc, pObj, k )
    {
        if ( pObj->Id == a->Id )
        {
            Vec_PtrFree( b_mffc );
            return true;
        }
    }
    Vec_PtrFree( b_mffc );
    return false;
}

/* add new variables in the cnf at the end of the current cnf
 * --> for c_ij != 0, add a new variable, and a corresponding new clause */
void 
SBMSM_t::miter_cnf_add_structural_clauses( Cnf_Dat_t * miter_cnf_data )
{
    /* declarations */
    unsigned int c_i_j_zero_num = 0;
    unsigned int LAC_num = candLACs.size();
    unsigned int var_for_cij;
    unsigned int out_var;
    unsigned int count = 1;
    int * pLits, ** pClas;

    /* count zeros in LAC_priority_matrix */
    for ( int i = 0; i < LAC_num; ++i )
    {
        for ( int j = 0; j < LAC_num; ++j )
        {
            if ( Ckt_GetBit( LAC_priority_matrix[i][j >> 6], j ) )
                continue;
            c_i_j_zero_num ++;
        }
    }

    /* assign cnf variable ids for structure codes */
    structure_code_ids = (int *) realloc( structure_code_ids, ( c_i_j_zero_num + 1 ) * sizeof( int ) );
    structure_code_ids[0] = c_i_j_zero_num;

    /* store some important old values for parameters */
    unsigned int original_nVars     = miter_cnf_data->nVars;
    unsigned int original_nClauses  = miter_cnf_data->nClauses;

    /* set some basic parameters in miter_cnf_data */
    miter_cnf_data->nVars       += c_i_j_zero_num;
    miter_cnf_data->nLiterals   += 3 * c_i_j_zero_num;
    miter_cnf_data->nClauses    += c_i_j_zero_num;

    /* modify pVarNum */
    int p_var_nums_buffer[original_nVars];
    for ( int i = 0; i < original_nVars; ++i )
    {
        p_var_nums_buffer[i] = miter_cnf_data->pVarNums[i];
    }
    miter_cnf_data->pVarNums = ABC_REALLOC( int, miter_cnf_data->pVarNums, miter_cnf_data->nVars );
    for ( int i = 0; i < original_nVars; ++i )
    {
        miter_cnf_data->pVarNums[i] = p_var_nums_buffer[i];
    }
    var_for_cij = get_max_id();
    for ( int i = 0; i < LAC_num; ++i )
    {
        for ( int j = 0; j < LAC_num; ++j )
        {
            if ( Ckt_GetBit( LAC_priority_matrix[i][j >> 6], j ) )
                continue;
            miter_cnf_data->pVarNums[var_for_cij++] = original_nVars++; // yeah no need to add one first. it's correct
            structure_code_ids[count++] = original_nVars;
        }
    }
    assert( miter_cnf_data->nVars == original_nVars );

    /* modify pCluases */
    miter_cnf_data->pClauses = ABC_REALLOC( int *, miter_cnf_data->pClauses, miter_cnf_data->nClauses + 1 );
    miter_cnf_data->pClauses[0] = ABC_REALLOC( int, miter_cnf_data->pClauses[0], miter_cnf_data->nLiterals );
    reassign_pclauses( miter_cnf_data );
    var_for_cij = get_max_id();
    pLits = miter_cnf_data->pClauses[original_nClauses];
    pClas = miter_cnf_data->pClauses + original_nClauses;
    for ( int i = 0; i < LAC_num; ++i )
    {
        for ( int j = 0; j < LAC_num; ++j )
        {
            if ( Ckt_GetBit( LAC_priority_matrix[i][j >> 6], j ) )
                continue;
            out_var = miter_cnf_data->pVarNums[var_for_cij++];
            * pClas ++ = pLits;
            // TODO
            * pLits ++ = MUXPIIDs[i+1];
            * pLits ++ = MUXPIIDs[j+1];
            * pLits ++ = out_var;

            miter_cnf_data->pVarNums[var_for_cij++] = original_nVars++;
        }
    }
    return;
}

void 
SBMSM_t::reassign_pclauses( Cnf_Dat_t * miter_cnf_data )
{
    /* declarations */
    int i, PoVar, OutVar, pVars[32], * pLits, ** pClas;
    Aig_Man_t * aig_manager_ptr = miter_cnf_data->pMan;
    Aig_Obj_t * pObj;

    if ( aig_manager_ptr == nullptr )
    {
        cout << "the aig manager of the network is null!" << endl;
        exit( 0 );
    }

    // assign the clauses
    pLits = miter_cnf_data->pClauses[0];
    pClas = miter_cnf_data->pClauses;
    Aig_ManForEachNode( aig_manager_ptr, pObj, i )
    {
        OutVar   = miter_cnf_data->pVarNums[ pObj->Id ];
        pVars[0] = miter_cnf_data->pVarNums[ Aig_ObjFanin0(pObj)->Id ];
        pVars[1] = miter_cnf_data->pVarNums[ Aig_ObjFanin1(pObj)->Id ];

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
    OutVar = miter_cnf_data->pVarNums[ Aig_ManConst1( aig_manager_ptr )->Id ];
    assert( OutVar <= Aig_ManObjNumMax( aig_manager_ptr ) );
    *pClas++ = pLits;
    *pLits++ = 2 * OutVar;  

    // write the output literals
    Aig_ManForEachCo( aig_manager_ptr, pObj, i )
    {
        OutVar = miter_cnf_data->pVarNums[ Aig_ObjFanin0(pObj)->Id ];
        if ( i < Aig_ManCoNum( aig_manager_ptr ) )
        {
            *pClas++ = pLits;
            *pLits++ = 2 * OutVar + Aig_ObjFaninC0(pObj); 
        }
        else
        {
            PoVar  = miter_cnf_data->pVarNums[ pObj->Id ];
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
    return;
}