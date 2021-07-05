#include "Sat_Based_Multi_Sel_Manager.h"

using namespace std::filesystem;
static int vec_to_int( std::vector<int> vec );

SBMSM_t::SBMSM_t( IN Abc_Ntk_t * _pOriNtk, int _threshold[], SASIMI_Manager_t & _sasimiMng )
: sasimiMng( _sasimiMng ), pMiter( nullptr ), OriPINum( 0 ), MUXPINum( 0 )
{
    // assigning file names
    strcpy( cnfFileName, "intermediate-results/Final_Miter_CNF.cnf" );
    strcpy( SATResultFileName, "intermediate-results/SAT_Problem_Out.qdimacs" );
    strcpy( SATProblemFileName, "intermediate-results/SAT_Problem_In.qdimacs" );
    strcpy( aigFileName, "intermediate-results/Alexanderia_Final_Miter_AIG.aiger" );
    strcpy( naiveMiterName, "intermediate-results/Alexanderia_Naive_Miter.blif" );
    strcpy( aigFileNameDup, "intermediate-results/Final_Miter_DupAig.aiger" );
    strcpy( cnfFileNameDup, "intermediate-results/Miter_Dup_ANDed_CNF_FindLeast_Miter_From_File.cnf" );
    strcpy( verifyMiterName, "intermediate-results/Alexanderia_Verify_Miter.blif" );
    strcpy( cnfFileNameVerifyMiter, "intermediate-results/Alexanderia_Verify_Miter_CNF.cnf" );
    // copying networks
    pOriNtk = Abc_NtkDup(_pOriNtk);
    pAppNtk = Abc_NtkDup(pOriNtk);
    Abc_Ntk_Assign_Level();
    /* debug begin */
    // Abc_Obj_t * pObj;
    // int i;
    // std::cout << "for input network: " << std::endl;
    // Abc_NtkForEachObj( _pOriNtk, pObj, i )
    // {
    //     std::cout << Abc_ObjName( pObj ) << ": " << Abc_ObjLevel( pObj ) << std::endl;
    // }
    // Ckt_WriteBlif( _pOriNtk, "intermediate-results/_pOriNtk_b1.blif" );
    // std::cout << std::endl << "for ori network: " << std::endl;
    // Abc_NtkForEachObj( pOriNtk, pObj, i )
    // {
    //     std::cout << std::endl << "fanins: ";
    //     Abc_Obj_t * pFanin; int j;
    //     Abc_ObjForEachFanin( pObj, pFanin, j )
    //     {
    //         std::cout << Abc_ObjName( pFanin ) << " ";
    //     }
    //     std::cout << std::endl << Abc_ObjName( pObj ) << ": " << Abc_ObjLevel( pObj ) << std::endl;
    // }
    // Ckt_WriteBlif( pOriNtk, "intermediate-results/pOriNtk_b1.blif" );
    /* debug end */
    Abc_Ntk_Dup_Level();
    std::cout << "4" << std::endl;
    pAppNtkDup = Abc_NtkDup(pOriNtk);
    std::cout << "5" << std::endl;
    // initializing sasimi manager
    sasimiMng.PatchConst(pOriNtk); // add 0s and 1s to the original circuit
    sasimiMng.PatchConst(pAppNtk); // add 0s and 1s to the approximate circuit
    sasimiMng.PatchConst(pAppNtkDup); // add 0s and 1s to the approximate circuit
    std::cout << "6" << std::endl;
    // initializing cnf parameters
    OriPIIDs = (int *) malloc( 0 ); 
    MUXPIIDs = (int *) malloc( 0 );
    nCnfVars = 0;
    // initializing threshold
    int size = Abc_NtkPoNum( pOriNtk );
    if ( size == 1 )
    {
        std::cout << "the number of primary outputs must be larger than 1!" << std::endl;
        exit( 0 );
    }
    threshold = new int[size];
    // assign the threshold
    for ( int i = 0; i < size; ++i )
        threshold[i] = _threshold[i];
    // std::cout << "hey here" << std::endl;
    // std::cout << "threshold array = ";
    // for ( int i = 0; i < size; ++i )
    // {
    //     std::cout << threshold[i];
    // }
    // std::cout << std::endl;
    std::vector<int> temp_vec( threshold, threshold + size );
    threshold_value = vec_to_int( temp_vec );
    // std::cout << "threshold value = " << threshold_value << std::endl;
}

static int vec_to_int( std::vector<int> vec )
{
    assert( ! vec.empty() );
    if ( vec.size() == 1 )
        return vec[0];
    int ones = vec.back();
    vec.pop_back();
    return ones + 2 * vec_to_int( vec );
}

SBMSM_t::~SBMSM_t()
{
    delete [] threshold;
    delete [] OriPIIDs;
    delete [] MUXPIIDs;
}

void 
SBMSM_t::SAT_Based_Multi_Selection()
{
    double totalSavedArea;

    collect_LACs();
    std::cout << "--- the number of candidate LACs is " << candLACs.size() << std::endl;
    // print_candLAC(20);
    if ( !candLAC_check_size() ) return;
    candLAC_truncate_size( truncateSize );
    // print_candLAC();

    // transform map to sop
    Abc_NtkMapToSop( pOriNtk );
    Abc_NtkMapToSop( pAppNtk );

    // generate CNF expression according to the muxed network
    std::cout << "# Creating MUXed CNF! ------------------------------ " << std::endl;
    if ( !create_final_CNF() )
    {
        fprintf( stderr, "no LAC can be applied!\n" );
        return;
    }

    // call library depqbf
    std::cout << "# Solving Quantified SAT Problem! ------------------------------ " << std::endl;
    QDPLL *depqbf = qdpll_create ();
    qdpll_configure (depqbf, "--dep-man=simple");
    qdpll_configure (depqbf, "--incremental-use");

    std::cout << "------- Loading CNF to Depqbf!" << std::endl;
    Cnf_DataFile2Depqbf( cnfFileName, depqbf, OriPIIDs, MUXPIIDs );
    std::cout << "------- Solving the Result!" << std::endl;
    QDPLLResult res = QDPLL_SolveSatWriteAssignments( depqbf, SATResultFileName, SATProblemFileName );

    // evaluate the area saved by this approximate local change.
    std::cout << "------- Calculating Total Recuded Area!" << std::endl;
    totalSavedArea = lacApplyMode == 0 ? calculate_total_reduced_area() : calculate_total_reduced_area_s_a_a_d();
    if ( totalSavedArea == -1 )
        // std::cout << "area calculation failed!" << std::endl;
        fprintf( stderr, "\tarea calculation failed!\n" );
    else
        // std::cout << "the area saved is " << totalSavedArea << std::endl;
        fprintf( stderr, "\tthe area saved is %f\n", totalSavedArea );

    // free the dynamically allocated memory
    std::cout << "------- Clearing Memory!" << std::endl;
    /* Delete solver instance */
    qdpll_delete ( depqbf );
    // verify
    std::cout << "------- Begin Verify!" << std::endl;
    build_verify_miter();
    solve_verify_miter_cnf();
}

void 
SBMSM_t::Abc_Ntk_Assign_Level()
{
    Abc_Obj_t * pObj, * pFanin;
    int i, j;
    Abc_NtkForEachObj( pOriNtk, pObj, i )
    {
        pObj->Level = 0;
    }
    Vec_Ptr_t * vNodes = Abc_NtkDfs( pOriNtk, 0 );
    Vec_PtrForEachEntry(Abc_Obj_t *, vNodes, pObj, i)
    {
        if ( Abc_ObjFaninNum( pObj ) == 0 )
        {
            pObj->Level = 0;
            continue;
        }
        if ( Abc_ObjFanin0( pObj )->Type == ABC_OBJ_PI && Abc_ObjFaninNum( pObj ) == 1 )
        {
            pObj->Level = 1;
            continue;
        }
        assert( Abc_ObjFanin0( pObj )->Level >= 1 || Abc_ObjFaninNum( pObj ) >= 2 );
        unsigned int max_level = 0;
        Abc_ObjForEachFanin( pObj, pFanin, j )
        {
            if ( pFanin->Level > max_level )
            {
                max_level = pFanin->Level;
            }
        }
        pObj->Level = max_level + 1;
    }
    Abc_NtkForEachPi( pOriNtk, pObj, i )
    {
        pObj->Level = 0;
    }
    Abc_NtkForEachPo( pOriNtk, pObj, i )
    {
        pObj->Level = 0;
    }
    Vec_PtrFree(vNodes);
}

void 
SBMSM_t::Abc_Ntk_Dup_Level()
{
    Abc_Obj_t * pOriObj, * pAppObj;
    int i;
    Abc_NtkForEachObj( pOriNtk, pOriObj, i )
    {
        pAppObj = Abc_NtkObj( pAppNtk, i );
        assert( pOriObj->pCopy == pAppObj );
        pAppObj->Level = pOriObj->Level;
    }
}

void 
SBMSM_t::collect_LACs()
{
    std::vector < std::vector <tVec> > bds;   // cpm[PO][obj][frameBlock], grouped with length of 64
    std::vector <Vec_Ptr_t *> vMffcs;    // the vector of Mffcs
    std::vector <LAC_t> nodeLACs;        // all the LACs
    std::vector <LAC_t> nodeLACsDup;        // all the LACs
    std::random_device rd;

    Simulator_t oriSmlt(pOriNtk, sasimiMng.nFrame);
    Simulator_t * pAppSmlt = new Simulator_t(pAppNtk, sasimiMng.nFrame);
    unsigned seed = static_cast <unsigned> (rd());   // random generation
    // unsigned seed = 2531465778;         // a fixed generation
    std::cout << "--- seed = " << seed << std::endl;
    oriSmlt.Input(seed);        // initialize PI for original simulator
    oriSmlt.Simulate();
    pAppSmlt->Input(seed);      // initialize PI for approximate simulator
    pAppSmlt->Simulate();
    std::cout << "--- getting CPM-one-cut" << std::endl;
    sasimiMng.GetCPMOneCut(oriSmlt, *pAppSmlt, bds);
    std::cout << "--- collecting mFFC" << std::endl;
    sasimiMng.CollectMFFC(*pAppSmlt, vMffcs);     // collect the Mffcs of the approximate simulator
    
    std::cout << "--- collecting LACs" << std::endl;
    if (sasimiMng.metricType == Metric_t::ER)     // get the LACs
        sasimiMng.Alexanderia_CollectAllLACsUnderER(oriSmlt, *pAppSmlt, bds, vMffcs, nodeLACs, nodeLACsDup, pAppNtkDup);
    else if ( sasimiMng.metricType == Metric_t::NMED )
        sasimiMng.Alexanderia_CollectAllLACsUnderNMED(oriSmlt, *pAppSmlt, bds, vMffcs, nodeLACs, nodeLACsDup, pAppNtkDup);
    else if ( sasimiMng.metricType == Metric_t::MaxED )
        sasimiMng.Alexanderia_CollectAllLACsUnderMaxED(oriSmlt, *pAppSmlt, bds, vMffcs, nodeLACs, nodeLACsDup, pAppNtkDup, threshold_value);
    else
        sasimiMng.Alexanderia_CollectAllLACsUnderArea( oriSmlt, *pAppSmlt, vMffcs, nodeLACs, nodeLACsDup, pAppNtkDup );
    sasimiMng.FreeMFFC(vMffcs);                   // free the memory

    // sort the LACs according to their scores
    std::cout << "--- sorting LACs" << std::endl;
    sasimiMng.SortCandLACs(nodeLACs, pAppSmlt->GetFrameNum(), candLACs);
    sasimiMng.SortCandLACs(nodeLACsDup, pAppSmlt->GetFrameNum(), candLACsDup);
}

bool
SBMSM_t::candLAC_check_size()
{
    if ( candLACs.size() == 0 )
    {
        std::cout << "no possible LAC exists!" << std::endl;
        return false;
    }
    return true;
}

void 
SBMSM_t::candLAC_truncate_size( unsigned int size )
{
    if ( candLACs.size() > size )
    {
        candLACs.resize( size );
        candLACsDup.resize( size );
    }
}

void 
SBMSM_t::print_candLAC( const int maximum_range )
{
    std::vector<int>    original_darea_list;
    std::vector<int>    range_mapped_darea_list;

    for ( int i = 0; i < candLACs.size(); ++i ) original_darea_list.push_back( candLACs[i].GetDArea() );
    range_map( range_mapped_darea_list, original_darea_list, maximum_range );

    std::cout << "the reduced area of all LAC candidates are listed as follows:" << std::endl;
    // cout << "the network type is: " << candLACs[0].GetSS()->pNtk->ntkType << endl;
    for ( int i = 0; i < candLACs.size(); ++i )
    {
        assert( candLACs[i].GetDArea() == candLACsDup[i].GetDArea() );
        std::cout << "DArea: " << original_darea_list[i] << " mapped_area:" << range_mapped_darea_list[i] << " DError" << candLACs[i].GetDError() 
        << " FOM: " << candLACs[i].GetFOM() << " ";
        std::cout << "SS name: " << Abc_ObjName( candLACs[i].GetSS() ) << ", TS name: " << Abc_ObjName( candLACs[i].GetTS() ) << std::endl;
    }
    std::cout << std::endl;
}

bool 
SBMSM_t::create_final_CNF()
{
    if ( !create_miter_finalize() )
        return false;

    // transform the miter to cnf and write cnf into file
    transform_miter_to_cnf();
    return true;
}

bool
SBMSM_t::create_miter_finalize()
{
    int selectionIndex;
    std::vector<Abc_Obj_t *> sortedSelectionSignals;
    int maximum_range = 20;

    create_naive_miter();
    
    // add the sorting network.
    std::cout << "------- Adding Sorting Network!" << std::endl;
    if ( areaEncodeMode == AreaEncodeMode_t::NOENCODE )
        miter_add_sorting_network( sortedSelectionSignals );
    else if ( areaEncodeMode == AreaEncodeMode_t::AREAENCODE )
        miter_add_sorting_network_area_encoded( sortedSelectionSignals, false, 0 );
    else
        miter_add_sorting_network_area_encoded( sortedSelectionSignals, true, maximum_range );
    print_candLAC( maximum_range );
    
    assert( Abc_NtkPoNum( pMiter ) == 1 );

    // find the least index of the selection signal that could be ANDed with the miter output
    std::cout << "------- Choosing the Least Valid Selection Signal!" << std::endl;
    selectionIndex = sorted_selection_find_least( sortedSelectionSignals );
    std::cout << "---------- The least index of the valid selection signal is: " << selectionIndex << std::endl;
    // if UNSAT
    if ( selectionIndex >= sortedSelectionSignals.size() )
    {
        std::cout << "------------- the least index has exceeds the boundary!" << std::endl;
        return false;
    }    

    // AND the selection signal with the output
    std::cout << "------- ANDing the Selection Signal with the Original Output!" << std::endl;
    create_sorted_miter( sortedSelectionSignals, selectionIndex );
    Ckt_WriteBlif( pMiter, "intermediate-results/Final_Miter_BLIF.blif" );
    return true;
}

void
SBMSM_t::create_naive_miter()
{
    std::cout << "------- Adding MUX!" << std::endl;
    add_muxes();
    Ckt_WriteBlif( pAppNtk, "intermediate-results/Alexanderia_MUXed_Network.blif" );   // no problem

    std::cout << "------- Strashing Original and MUXed Network!" << std::endl;
    pOriNtk = Abc_NtkStrash( pOriNtk, 0, 0, 0 );
    std::cout << "original strash succeeded" << std::endl;
    pAppNtk = Abc_NtkStrash( pAppNtk, 0, 0, 0 );
    std::cout << "muxed strash succeeded" << std::endl;

    // create the miter
    std::cout << "------- Creating Miter!" << std::endl;
    create_subtractor_comparator_miter();
    assert( Abc_NtkPiNum( pMiter ) == Abc_NtkPiNum( pAppNtk ) );
    std::cout << "------- Writing Miter Blif!" << std::endl;
    Ckt_WriteBlif( pMiter, naiveMiterName );

    std::cout << "-------------- Miter is created successfully!" << std::endl 
        << "-------------- the number of PI in original network is " << Abc_NtkPiNum( pOriNtk ) << std::endl 
        << "-------------- the number of PI in miter is " << Abc_NtkPiNum( pMiter ) << std::endl;
    OriPINum = Abc_NtkPiNum( pOriNtk );   
    MUXPINum = Abc_NtkPiNum( pMiter ) - OriPINum;
}

void
SBMSM_t::create_sorted_miter( std::vector<Abc_Obj_t *> & sortedSelectionSignals, int selectionIndex )
{
    Abc_Obj_t * pNewOutput, * pOriOutput, * pOriOutputFanin;
    pOriOutput = Abc_NtkPo( pMiter, 0 );    // old output
    pOriOutputFanin = Abc_ObjFanin0( pOriOutput );  // get the fanin of the old output
    pNewOutput = Abc_NtkCreatePo( pMiter ); // create the new output
    // add the AND(fanin, selection-signal) to the new output
    Abc_ObjAddFanin( pNewOutput, Abc_AigAnd( ( Abc_Aig_t * ) pMiter->pManFunc, pOriOutputFanin, sortedSelectionSignals[selectionIndex] ) );
    Abc_NtkDeleteObj( pOriOutput ); // delete the old output
    assert( Abc_NtkPoNum( pMiter ) == 1 );
}

void
SBMSM_t::create_sorted_miter_dup( std::vector<Abc_Obj_t *> & sortedSelectionSignalsDup, int pointer, Abc_Ntk_t * pMiterDup )
{
    Abc_Obj_t * pNewOutput, * pOriOutput, * pOriOutputFanin;
    pOriOutput = Abc_NtkPo( pMiterDup, 0 );
    pOriOutputFanin = Abc_ObjFanin0( pOriOutput );
    pNewOutput = Abc_NtkCreatePo( pMiterDup );
    // add fanin to the new output
    Abc_ObjAddFanin( pNewOutput, Abc_AigAnd( ( Abc_Aig_t * ) pMiterDup->pManFunc, pOriOutputFanin, sortedSelectionSignalsDup[pointer] ) );
    assert( Abc_NtkPoNum( pMiterDup ) == 2 );
    // delete the old output
    Abc_NtkDeleteObj( pOriOutput );
    assert( Abc_NtkPoNum( pMiterDup ) == 1 );
}

void
SBMSM_t::add_muxes()
{
    int count = 0, candLACNum = candLACs.size();
    Abc_Obj_t * pMux;
    double previousProcession = 0, currentProcession = 0;
    for ( auto & candLAC : candLACs ) {
        // /* debug begin */
        // Abc_Obj_t * pFanout;
        // int i;
        // std::cout << "outputs for pTS: " << std::endl;
        // Abc_ObjForEachFanout( candLAC.GetTS(), pFanout, i )
        // {
        //     std::cout << "    " << i << ". " << Abc_ObjName( pFanout ) << std::endl;
        // }
        // /* debug end */
        Abc_Obj_t * selection = Abc_NtkCreatePi( pAppNtk );
        if ( candLAC.GetIsInv() )
        {
            Abc_Obj_t * pInvSS = Abc_NtkCreateNodeInv( pAppNtk, candLAC.GetSS() );
            pMux = Abc_NtkCreateNodeMux( pAppNtk, selection, pInvSS, candLAC.GetTS() );
        }
        else
        {
            pMux = Abc_NtkCreateNodeMux( pAppNtk, selection, candLAC.GetSS(), candLAC.GetTS() );
        }
        // transfer all the fanouts of TS to MUX, except for the MUX itself
        abc_obj_transfer_fanouts_except_mux( candLAC.GetTS(), pMux, pMux );

        count ++;
        currentProcession = count / (double) candLACNum;
        if( currentProcession >= 0.1 && currentProcession - previousProcession >= 0.1 )
        {
            std::cout << "-------------- " << count << " (out of " << candLACNum << ") MUXes finished" << std::endl;
            previousProcession = count / (double) candLACNum;
        }

        // /* debug begin */
        // std::cout << "mux " << count << std::endl;
        // std::cout << "    input 1: " << Abc_ObjName( candLAC.GetTS() ) << std::endl;
        // std::cout << "    input 2: " << Abc_ObjName( candLAC.GetSS() ) << std::endl;
        // std::cout << "    outputs: " << std::endl;
        // Abc_ObjForEachFanout( pMux, pFanout, i )
        // {
        //     std::cout << "        " << i << ". " << Abc_ObjName( pFanout ) << std::endl;
        // }
        // if ( count >= 3 )
        // {
        //     break;
        // }
        // /* debug end */
    }
}

void 
SBMSM_t::abc_obj_transfer_fanouts_except_mux ( IN Abc_Obj_t * pNodeFrom, IN Abc_Obj_t * pNodeTo, IN Abc_Obj_t * pMux )
{
    Vec_Ptr_t * vFanouts;
    int nFanoutsOld, i;
    assert( !Abc_ObjIsComplement(pNodeFrom) );
    assert( !Abc_ObjIsComplement(pNodeTo) );
    assert( !Abc_ObjIsPo(pNodeFrom) && !Abc_ObjIsPo(pNodeTo) );
    assert( pNodeFrom->pNtk == pNodeTo->pNtk );
    assert( pNodeFrom != pNodeTo );
    assert( !Abc_ObjIsNode(pNodeFrom) || Abc_ObjFanoutNum(pNodeFrom) > 0 );

    // get the fanouts of the old node
    nFanoutsOld = Abc_ObjFanoutNum(pNodeTo);
    vFanouts = Vec_PtrAlloc( nFanoutsOld );
    abc_node_collect_fanouts_except_mux( pNodeFrom, vFanouts, pMux );
    // patch the fanin of each of them
    for ( i = 0; i < vFanouts->nSize; i++ )
        // replace the path "pArray[i]==>pNodeFrom" to "pArray[i]==>pNodeTo"
        Abc_ObjPatchFanin( (Abc_Obj_t *)vFanouts->pArray[i], pNodeFrom, pNodeTo );
    assert( Abc_ObjFanoutNum(pNodeFrom) == 1 );
    assert( Abc_ObjFanoutNum(pNodeTo) == nFanoutsOld + vFanouts->nSize );
    Vec_PtrFree( vFanouts );
}

void 
SBMSM_t::abc_node_collect_fanouts_except_mux ( IN Abc_Obj_t * pNode, IN Vec_Ptr_t * vNodes, IN Abc_Obj_t * pMux )
{
    Abc_Obj_t * pFanout;
    int i;
    Vec_PtrClear(vNodes);
    Abc_ObjForEachFanout( pNode, pFanout, i ) {
        if ( pFanout->Id != pMux->Id )    // not the pMux
            Vec_PtrPush(vNodes, pFanout);
    }
}

void
SBMSM_t::create_subtractor_comparator_miter () {
    char Buffer[1000];

    assert( Abc_NtkIsStrash(pAppNtk) );
    assert( Abc_NtkIsStrash(pOriNtk) );

    // start the new network
    pMiter = Abc_NtkAlloc( ABC_NTK_STRASH, ABC_FUNC_AIG, 1 );
    sprintf( Buffer, "%s_%s_miter", pAppNtk->pName, pOriNtk->pName );
    pMiter->pName = Extra_UtilStrsav(Buffer);

    /************************************* Abc_NtkMiterPrepare *************************************/
    Abc_Obj_t * pObj, * pObjNew;
    int i;
    Abc_AigConst1(pAppNtk)->pCopy = Abc_AigConst1(pMiter);
    Abc_AigConst1(pOriNtk)->pCopy = Abc_AigConst1(pMiter);
    // create new PIs and remember them in the old PIs
    Abc_NtkForEachPi( pAppNtk, pObj, i )
    {
        pObjNew = Abc_NtkCreatePi( pMiter );
        // remember this PI in the old PIs
        pObj->pCopy = pObjNew;
        if ( i < Abc_NtkCiNum( pOriNtk ) ) {
            pObj = Abc_NtkCi(pOriNtk, i);
            pObj->pCopy = pObjNew;
        }
        // add name
        Abc_ObjAssignName( pObjNew, Abc_ObjName(pObj), NULL );
    }
    // create the latches
    Abc_NtkForEachLatch( pAppNtk, pObj, i )
    {
        pObjNew = Abc_NtkDupBox( pMiter, pObj, 0 );
        // add names
        Abc_ObjAssignName( pObjNew, Abc_ObjName(pObj), "_1" );
        Abc_ObjAssignName( Abc_ObjFanin0(pObjNew),  Abc_ObjName(Abc_ObjFanin0(pObj)), "_1" );
        Abc_ObjAssignName( Abc_ObjFanout0(pObjNew), Abc_ObjName(Abc_ObjFanout0(pObj)), "_1" );
    }
    Abc_NtkForEachLatch( pOriNtk, pObj, i )
    {
        pObjNew = Abc_NtkDupBox( pMiter, pObj, 0 );
        // add name
        Abc_ObjAssignName( pObjNew, Abc_ObjName(pObj), "_2" );
        Abc_ObjAssignName( Abc_ObjFanin0(pObjNew),  Abc_ObjName(Abc_ObjFanin0(pObj)), "_2" );
        Abc_ObjAssignName( Abc_ObjFanout0(pObjNew), Abc_ObjName(Abc_ObjFanout0(pObj)), "_2" );
    }
    
    /************************************* Abc_NtkMiterAddOne *************************************/
    Abc_Obj_t * pNode;
    assert( Abc_NtkIsDfsOrdered(pAppNtk) );
    Abc_AigForEachAnd( pAppNtk, pNode, i )
        pNode->pCopy = Abc_AigAnd( (Abc_Aig_t *)pMiter->pManFunc, Abc_ObjChild0Copy(pNode), Abc_ObjChild1Copy(pNode) );
    assert( Abc_NtkIsDfsOrdered(pOriNtk) );
    Abc_AigForEachAnd( pOriNtk, pNode, i )
        pNode->pCopy = Abc_AigAnd( (Abc_Aig_t *)pMiter->pManFunc, Abc_ObjChild0Copy(pNode), Abc_ObjChild1Copy(pNode) );

    /************************************* Abc_NtkMiterFinalize *************************************/
    Vec_Ptr_t * vPairs; vPairs = Vec_PtrAlloc( 100 );
    // collect the PO nodes for the miter
    Abc_NtkForEachPo( pAppNtk, pNode, i )
    {
        Vec_PtrPush( vPairs, Abc_ObjChild0Copy(pNode) );
        pNode = Abc_NtkPo( pOriNtk, i );
        Vec_PtrPush( vPairs, Abc_ObjChild0Copy(pNode) );
    }
    Abc_NtkForEachLatch( pAppNtk, pNode, i )
        Abc_ObjAddFanin( Abc_ObjFanin0(pNode)->pCopy, Abc_ObjChild0Copy(Abc_ObjFanin0(pNode)) );
    Abc_NtkForEachLatch( pOriNtk, pNode, i )
        Abc_ObjAddFanin( Abc_ObjFanin0(pNode)->pCopy, Abc_ObjChild0Copy(Abc_ObjFanin0(pNode)) );
    // build the output vectors of two networks
    Abc_Obj_t ** X = new Abc_Obj_t *[vPairs->nSize / 2];    // MUXed
    Abc_Obj_t ** Y = new Abc_Obj_t *[vPairs->nSize / 2];    // original
    for(i = 0; i < vPairs->nSize; i += 2)
    {
        X[i / 2] = (Abc_Obj_t *) vPairs->pArray[i];
        Y[i / 2] = (Abc_Obj_t *) vPairs->pArray[i + 1];
    }
    // calculate the weighted difference of the outputs from two networks
    Abc_Obj_t ** Diff = X_subtract_Y_absolute(pMiter, X, Y, vPairs->nSize / 2);
    delete[] X;
    delete[] Y;
    // builds the comparator
    int size = Abc_NtkPoNum( pAppNtk );
    Abc_Obj_t ** thresholdNodes = const_nodes( pMiter, threshold, size );
    Abc_Obj_t * compareRes = X_less_than_Y( pMiter, Diff, thresholdNodes,  size );
    // create the final output
    Abc_Obj_t * result = Abc_NtkCreatePo( pMiter );
    Abc_ObjAddFanin( result, compareRes );

    /************************************* rest of Abc_NtkMiterInt *************************************/
    delete[] Diff;
    Vec_PtrFree( vPairs );
    Abc_AigCleanup((Abc_Aig_t *)pMiter->pManFunc);

    // make sure that everything is okay
    if ( !Abc_NtkCheck( pMiter ) )
    {
        printf( "Abc_NtkMiter: The network check has failed.\n" );
        Abc_NtkDelete( pMiter );
        pMiter = NULL;
    }
    miter_check();
}

Abc_Obj_t ** 
SBMSM_t::X_subtract_Y_absolute(Abc_Ntk_t * pNtk, Abc_Obj_t * X[], Abc_Obj_t * Y[], int n) {
    if(n <= 0) return nullptr;
    Abc_Obj_t ** R = new Abc_Obj_t *[n];
    Abc_Obj_t ** Cout = new Abc_Obj_t *[n];
    R[0] = Abc_AigXor((Abc_Aig_t *) pNtk->pManFunc, X[0], Y[0]);
    Cout[0] = Abc_AigAnd((Abc_Aig_t *) pNtk->pManFunc, Abc_ObjNot(X[0]), Y[0]);
    for(int i=1; i<n; i++)
    {
        R[i] = Abc_AigXor((Abc_Aig_t *) pNtk->pManFunc,
                          Abc_AigXor((Abc_Aig_t *) pNtk->pManFunc, Cout[i-1], X[i]),
                          Y[i]
        );
        Abc_Obj_t * temp1 = Abc_AigAnd((Abc_Aig_t *) pNtk->pManFunc, Abc_ObjNot(X[i]), Y[i]);
        Abc_Obj_t * temp2 = Abc_AigOr((Abc_Aig_t *) pNtk->pManFunc, Abc_ObjNot(X[i]), Y[i]);
        Cout[i] = Abc_AigOr((Abc_Aig_t *) pNtk->pManFunc,
                            Abc_AigAnd((Abc_Aig_t *) pNtk->pManFunc, temp1, Abc_ObjNot(Cout[i-1])),
                            Abc_AigAnd((Abc_Aig_t *) pNtk->pManFunc, temp2, Cout[i-1])
        );
    }

    // 2's complement of R ( = R' + 1 )
    Abc_Obj_t ** R_2Complement = new Abc_Obj_t *[n];
    Abc_Obj_t ** Cout_2 = new Abc_Obj_t * [n];
    R_2Complement[0] = R[0];
    Cout_2[0] = Abc_ObjNot(R[0]);
    for(int k=1; k<n; k++)
    {
        R_2Complement[k] = Abc_AigXor((Abc_Aig_t *) pNtk->pManFunc, Cout_2[k-1], Abc_ObjNot(R[k]));
        Cout_2[k] = Abc_AigAnd((Abc_Aig_t *) pNtk->pManFunc, Cout_2[k-1], Abc_ObjNot(R[k]));
    }
    Abc_Obj_t ** res = new Abc_Obj_t *[n];
    for(int k=0; k<n; k++)
        res[k] = Abc_AigMux((Abc_Aig_t *) pNtk->pManFunc, Cout[n-1], R_2Complement[k], R[k]);
    delete[] R_2Complement;
    delete[] Cout;
    delete[] Cout_2;
    delete[] R;
    return res;
}

Abc_Obj_t * 
SBMSM_t::X_less_than_Y(Abc_Ntk_t * pNtk, Abc_Obj_t * X[], Abc_Obj_t * Y[], int n) {
    /* implementation */
    if(n <= 0) return nullptr;
    std::vector<Abc_Obj_t *> PreBitsComp(n, nullptr);
    PreBitsComp[0] = Abc_AigAnd((Abc_Aig_t *) pNtk->pManFunc, Abc_ObjNot(X[0]), Y[0]);
    for(int i=1; i<n; i++)
    {
        PreBitsComp[i] =
            Abc_AigOr((Abc_Aig_t *) pNtk->pManFunc,
                Abc_AigAnd((Abc_Aig_t *) pNtk->pManFunc, Abc_ObjNot(X[i]), Y[i]),
                Abc_AigAnd((Abc_Aig_t *) pNtk->pManFunc, PreBitsComp[i-1],
                    Abc_ObjNot(Abc_AigXor((Abc_Aig_t *) pNtk->pManFunc, X[i], Y[i]))
                )
            );
    }
    return PreBitsComp[n - 1];
}

Abc_Obj_t * 
SBMSM_t::const_node ( Abc_Ntk_t * pNtk, int n ) {
    return n ? Abc_AigConst1( pNtk ) : Abc_ObjNot( const_node( pNtk, 1 ) );
}

Abc_Obj_t ** 
SBMSM_t::const_nodes ( Abc_Ntk_t * pNtk, int threshold[], int size ) {
    Abc_Obj_t ** thresholdNodes = new Abc_Obj_t *[size];
    for ( int i = 0; i < size; ++i )
        thresholdNodes[i] = const_node( pNtk, threshold[i] );
    return thresholdNodes;
}

void 
SBMSM_t::miter_check () 
{
    if ( Abc_NtkPoNum( pMiter ) != 1 ) {
        std::cout << "the number of output of pNtk1 is " << Abc_NtkPoNum( pOriNtk ) << std::endl;
        std::cout << "the number of output is not 1, but " << Abc_NtkPoNum( pMiter )
             << " instead. Miter creation is NOT successful!" << std::endl;
    }
}

void 
SBMSM_t::miter_add_sorting_network ( std::vector<Abc_Obj_t *> &sortedSelectionSignals )
{
    Abc_Obj_t * pObj;
    int i, miterPINum, selectionNum;
    std::vector<Abc_Obj_t *> selectionSignals;
    
    miterPINum = Abc_NtkPiNum( pMiter );
    selectionNum = miterPINum - OriPINum;
    if ( !sortedSelectionSignals.empty() )  sortedSelectionSignals.clear();
    sortedSelectionSignals.resize( selectionNum, nullptr );
    // if no selection signals in the first place, nothing needs done
    if ( selectionNum == 0 )    return;
    assert( selectionNum > 0 );

    // selection signals are PIs with ids larger or equal to <oriPINum>
    Abc_NtkForEachPi( pMiter, pObj, i )
    {
        if ( i < OriPINum )
            continue;
        selectionSignals.push_back( pObj );
    }
    assert( selectionSignals.size() == selectionNum );

    // add the sorting network and connect the outputs.
    N_Input_Sorting_Network( pMiter, selectionSignals, selectionNum, sortedSelectionSignals );
}

void 
SBMSM_t::miter_add_sorting_network_area_encoded ( std::vector<Abc_Obj_t *> &sortedSelectionSignals, const bool restrict_range, const int maximum_range )
{
    Abc_Obj_t * pObj;
    int i, miterPINum, selectionNum;
    std::vector<int> original_darea_list;
    std::vector<int> range_mapped_darea_list;
    std::vector<int> loop_limit_list;
    std::vector<Abc_Obj_t *> selectionSignals;
    
    miterPINum = Abc_NtkPiNum( pMiter );
    selectionNum = miterPINum - OriPINum;
    assert( candLACs.size() == selectionNum );
    if ( !sortedSelectionSignals.empty() )  sortedSelectionSignals.clear();
    sortedSelectionSignals.resize( selectionNum, nullptr );
    for ( int i = 0; i < candLACs.size(); ++i ) original_darea_list.push_back( candLACs[i].GetDArea() );
    range_map( range_mapped_darea_list, original_darea_list, maximum_range );
    // if no selection signals in the first place, nothing needs done
    if ( selectionNum == 0 )    return;
    assert( selectionNum > 0 );

    // selection signals are PIs with ids larger than or equal to <oriPINum>
    loop_limit_list = restrict_range ? range_mapped_darea_list : original_darea_list;
    Abc_NtkForEachPi( pMiter, pObj, i )
    {
        if ( i < OriPINum )
            continue;
        for 
        ( 
            int j = 0; 
            // j < candLACs[i-oriPINum].GetDArea(); 
            j < loop_limit_list[i-OriPINum]; 
            ++j
        )
            selectionSignals.push_back( pObj );
    }
    // assert( selectionSignals.size() == selectionNum );

    // add the sorting network and connect the outputs.
    N_Input_Sorting_Network( pMiter, selectionSignals, selectionSignals.size(), sortedSelectionSignals );
}

void 
SBMSM_t::range_map ( std::vector<int> & range_mapped_list, const std::vector<int> & original_list, const int maximum_range )
{
    if ( !range_mapped_list.empty() ) range_mapped_list.clear();
    
    // find the maximum value in the list <rangeMappedList>
    int original_maximum_value = 0;
    for ( int i = 0; i < original_list.size(); ++i )
        if ( original_list[i] > original_maximum_value )
            original_maximum_value = original_list[i];
    
    // map every value in the list
    double compression_ratio = (double) maximum_range / (double) original_maximum_value;
    // original list is already small enough
    if ( compression_ratio >= 1 )
    {
        range_mapped_list = original_list;
        return;
    }
    for ( int i = 0; i < original_list.size(); ++i )
    {
        double new_value_double = original_list[i] * compression_ratio;
        range_mapped_list.push_back( (int) ceil( new_value_double ) );
    }
}

void 
SBMSM_t::transform_miter_to_cnf ()
{
    Cnf_Dat_t * miterCnfData;
    std::cout << "------- Transforming to AIG!" << std::endl << "---------- ";
    Io_Write(pMiter, aigFileName, IO_FILE_AIGER );  // the index of PIs are indexed from 1 to <NumCIs>. Temporarily stored in <pObj->pCopy>. See lines 699 ~ 704 in "src/base/io/ioWriteAiger.c" for more details
    Aig_Man_t * miterAigMan = Ioa_ReadAiger( aigFileName, 1 );
    Aig_ManPrintStats( miterAigMan );

    // transform the aig to cnf
    std::cout << "------- Transforming to CNF!" << std::endl;
    miterCnfData = Cnf_DeriveSimple( miterAigMan, 0 );  // the variable index in cnf is derived from iterating all COs, Internal nodes, CIs in the aig manager. For more details, see lines 624 ~ 630 in "src/sat/cnf/cnfWrite.c"
    nCnfVars = miterCnfData->nVars;

    // get PI's IDs
    std::cout << "------- Assigning PIIDs!" << std::endl;
    OriPIIDs = (int *) realloc( OriPIIDs, ( OriPINum + 1 ) * sizeof( int ) );
    MUXPIIDs = (int *) realloc( MUXPIIDs, ( MUXPINum + 1 ) * sizeof( int ) );
    assign_PIIDs( miterCnfData, false );

    // write the cnf into the file
    std::cout << "------- Writing Miter CNF!" << std::endl;
    Cnf_DataWriteIntoFile( miterCnfData, (char *) cnfFileName, 1, NULL, NULL );  // since fReadable=1, all the variables can be read by file without any trouble.
}

void 
SBMSM_t::transform_miter_to_cnf_dup ( Abc_Ntk_t * pMiterDup )
{   
    Cnf_Dat_t * miterCnfData;
    Aig_Man_t * miterAigMan;
    std::cout << "-------------- Transforming to AIG!" << std::endl << "---------- ";
    Io_Write(pMiterDup, aigFileNameDup, IO_FILE_AIGER );  // the index of PIs are indexed from 1 to <NumCIs>. Temporarily stored in <pObj->pCopy>. See lines 699 ~ 704 in "src/base/io/ioWriteAiger.c" for more details
    miterAigMan = Ioa_ReadAiger( aigFileNameDup, 1 );
    Aig_ManPrintStats( miterAigMan );

    // transform the aig to cnf
    std::cout << "-------------- Transforming to CNF!" << std::endl;
    miterCnfData = Cnf_DeriveSimple( miterAigMan, 0 );  // the variable index in cnf is derived from iterating all COs, Internal nodes, CIs in the aig manager. For more details, see lines 624 ~ 630 in "src/sat/cnf/cnfWrite.c"
    nCnfVars = miterCnfData->nVars;

    // get PI's IDs
    std::cout << "-------------- Assigning PIIDs!" << std::endl;
    OriPIIDs = (int *) realloc( OriPIIDs, ( OriPINum + 1 ) * sizeof( int ) );
    MUXPIIDs = (int *) realloc( MUXPIIDs, ( MUXPINum + 1 ) * sizeof( int ) );
    assign_PIIDs( miterCnfData, false );

    // write the cnf into the file
    std::cout << "-------------- Writing Miter CNF!" << std::endl;
    Cnf_DataWriteIntoFile( miterCnfData, (char *) cnfFileNameDup, 1, NULL, NULL );  // since fReadable=1, all the variables can be read by file without any trouble.
}

double 
SBMSM_t::calculate_total_reduced_area ()
{
    std::ifstream       ifStream;
    std::istringstream  isStream;
    std::string         iLine, trashBin1, trashBin2;
    int                 LACIndex;
    const std::string   stringV( "V" ), string0( "0" );
    std::vector<int>    LACIndices;
    double              originalArea, reducedArea;
    LAC_t               lac;
    
    // open the file that stores the qSAT solution
    ifStream.open( SATResultFileName, std::ifstream::in );
    if ( !ifStream.is_open() )
    {
        printf( "Cannot open file \"%s\" for writing.\n", SATResultFileName );
        return -1;
    }
    if ( !LACIndices.empty() )  LACIndices.clear();
    std::cout << "---------- calculate_total_reduced_area: reading the file!" << std::endl;
    // read the file and stores all the positive literals into the vector <LACIndices>
    while ( std::getline( ifStream, iLine ) )
    {
        isStream.str( iLine );
        isStream >> trashBin1 >> LACIndex >> trashBin2;
        assert( trashBin1.compare( stringV ) == 0 );
        assert( trashBin2.compare( string0 ) == 0 );
        if ( LACIndex > 0 ) 
            LACIndices.push_back( LACIndex );
        isStream.clear();
    }

    std::cout << "---------- calculate_total_reduced_area: applying LACs!" << std::endl;
    // get the corresponding indices in <candLACs>
    originalArea = Abc_NtkGetMappedArea( pAppNtkDup );
    for ( int i = 0; i < LACIndices.size() && i < MUXPINum; ++i )
    {
        LACIndices[i] -= nCnfVars - candLACsDup.size() - 1;
        lac = candLACsDup[LACIndices[i]];
        apply_lac( lac );
    }
    reducedArea = Abc_NtkGetMappedArea( pAppNtkDup );
    std::cout << "original area = " << originalArea << " ";
    std::cout << "reduced area = " << reducedArea << std::endl;
    fprintf( stderr, "\tarea after resynthesis = %f\n", originalArea );
    fprintf( stderr, "\treduction = %f\n", reducedArea );
    return originalArea - reducedArea;
}

double 
SBMSM_t::calculate_total_reduced_area_s_a_a_d ()
{
    std::ifstream       ifStream;
    std::istringstream  isStream;
    std::string         iLine, trashBin1, trashBin2;
    int                 LACIndex;
    const std::string   stringV( "V" ), string0( "0" );
    std::vector<int>    LACIndices;
    double              originalArea, reducedArea;
    LAC_t               lac;
    
    // open the file that stores the qSAT solution
    ifStream.open( SATResultFileName, std::ifstream::in );
    if ( !ifStream.is_open() )
    {
        printf( "Cannot open file \"%s\" for writing.\n", SATResultFileName );
        return -1;
    }
    if ( !LACIndices.empty() )  LACIndices.clear();
    std::cout << "---------- calculate_total_reduced_area: reading the file!" << std::endl;
    // read the file and stores all the positive literals into the vector <LACIndices>
    while ( std::getline( ifStream, iLine ) )
    {
        isStream.str( iLine );
        isStream >> trashBin1 >> LACIndex >> trashBin2;
        assert( trashBin1.compare( stringV ) == 0 );
        assert( trashBin2.compare( string0 ) == 0 );
        if ( LACIndex > 0 ) 
            LACIndices.push_back( LACIndex );
        isStream.clear();
    }

    std::cout << "---------- calculate_total_reduced_area: applying LACs!" << std::endl;
    // get the corresponding indices in <candLACs>
    originalArea = Abc_NtkGetMappedArea( pAppNtkDup );
    for ( int i = 0; i < LACIndices.size() && i < MUXPINum; ++i )
    {
        LACIndices[i] -= nCnfVars - candLACsDup.size() - 1;
        lac = candLACsDup[LACIndices[i]];
        apply_lac_s_a_a_d( lac );
    }
    batch_deletion_s_a_a_d();
    reducedArea = Abc_NtkGetMappedArea( pAppNtkDup );
    std::cout << "original area = " << originalArea << " ";
    std::cout << "reduced area = " << reducedArea << std::endl;
    fprintf( stderr, "\tarea after resynthesis = %f\n", originalArea );
    fprintf( stderr, "\treduction = %f\n", reducedArea );
    return originalArea - reducedArea;
}

void 
SBMSM_t::apply_lac ( LAC_t & lac )
{
    Abc_Obj_t * pTS, * pSS;
    bool isInv;
    pTS = lac.GetTS();
    pSS = lac.GetSS();
    if ( pTS == nullptr )
        std::cout << "bagayalu pTS" << std::endl;
    if ( pSS == nullptr )
        std::cout << "bagayalu pSS" << std::endl;
    if (pTS == nullptr || pSS == nullptr)
    {
        std::cout << "wrong!";
        return;
    }
    // probably lac does not exists anymore
    if ( !pTS->pNtk || !pSS->pNtk )
    {
        std::cout << "skipped" << std::endl;
        return;
    }
    assert( !Abc_ObjIsComplement(pTS) );
    assert( !Abc_ObjIsComplement(pSS) );
    assert( pTS->pNtk == pSS->pNtk );
    assert( pTS->pNtk == pAppNtkDup );
    isInv = lac.GetIsInv();
        // return 1;
    if (!isInv) {
        if (Abc_ObjIsNode(pSS) && Abc_NodeIsConst0(pSS))
            std::cout << Abc_ObjName(pTS) << " is replaced by zero" << std::endl;
        else if (Abc_ObjIsNode(pSS) && Abc_NodeIsConst1(pSS))
            std::cout << Abc_ObjName(pTS) << " is replaced by one" << std::endl;
        else
            std::cout << Abc_ObjName(pTS) << " is replaced by " << Abc_ObjName(pSS) << std::endl;
        // std::cout << "replacing " << Abc_ObjName( pTS ) << " with " << Abc_ObjName( pSS ) << std::endl;
        std::cout << "== deleting: " << Abc_ObjName( pTS ) << std::endl;
        alexanderia_replace_obj(pTS, pSS);
    }
    else {
        if (Abc_ObjIsNode(pSS) && Abc_NodeIsConst0(pSS))
            std::cout << Abc_ObjName(pTS) << " is replaced by zero" << std::endl;
        else if (Abc_ObjIsNode(pSS) && Abc_NodeIsConst1(pSS))
            std::cout << Abc_ObjName(pTS) << " is replaced by one" << std::endl;
        else
            std::cout << Abc_ObjName(pTS) << " is replaced by " << Abc_ObjName(pSS) << std::endl;
        // std::cout << "replacing " << Abc_ObjName( pTS ) << " with " << Abc_ObjName( pSS ) << std::endl;
        DASSERT(!(Abc_ObjIsNode(pSS) && Abc_NodeIsConst(pSS)));
        // cout << Abc_ObjName(pTS) << " is replaced by " << Abc_ObjName(pSS) << " with inverter with estimated error " << error << endl;
        Abc_Obj_t * pInv = Abc_NtkCreateNodeInv(pAppNtkDup, pSS);  // complemented...
        std::cout << "== deleting: " << Abc_ObjName( pTS ) << std::endl;
        alexanderia_replace_obj(pTS, pInv);
    }
}

void 
SBMSM_t::apply_lac_s_a_a_d ( LAC_t & lac )
{
    Abc_Obj_t * pTS, * pSS;
    bool isInv;
    pTS = lac.GetTS();
    pSS = lac.GetSS();
    if ( pTS == nullptr )
        std::cout << "bagayalu pTS" << std::endl;
    if ( pSS == nullptr )
        std::cout << "bagayalu pSS" << std::endl;
    if (pTS == nullptr || pSS == nullptr)
    {
        std::cout << "wrong!";
        return;
    }
    // probably lac does not exists anymore
    if ( !pTS->pNtk || !pSS->pNtk )
    {
        std::cout << "skipped" << std::endl;
        return;
    }
    assert( !Abc_ObjIsComplement(pTS) );
    assert( !Abc_ObjIsComplement(pSS) );
    assert( pTS->pNtk == pSS->pNtk );
    assert( pTS->pNtk == pAppNtkDup );
    isInv = lac.GetIsInv();
        // return 1;
    if (!isInv) {
        if (Abc_ObjIsNode(pSS) && Abc_NodeIsConst0(pSS))
            std::cout << Abc_ObjName(pTS) << " is replaced by zero" << std::endl;
        else if (Abc_ObjIsNode(pSS) && Abc_NodeIsConst1(pSS))
            std::cout << Abc_ObjName(pTS) << " is replaced by one" << std::endl;
        else
            std::cout << Abc_ObjName(pTS) << " is replaced by " << Abc_ObjName(pSS) << std::endl;
        // std::cout << "replacing " << Abc_ObjName( pTS ) << " with " << Abc_ObjName( pSS ) << std::endl;
        alexanderia_replace_obj_s_a_a_d(pTS, pSS);
    }
    else {
        if (Abc_ObjIsNode(pSS) && Abc_NodeIsConst0(pSS))
            std::cout << Abc_ObjName(pTS) << " is replaced by zero" << std::endl;
        else if (Abc_ObjIsNode(pSS) && Abc_NodeIsConst1(pSS))
            std::cout << Abc_ObjName(pTS) << " is replaced by one" << std::endl;
        else
            std::cout << Abc_ObjName(pTS) << " is replaced by " << Abc_ObjName(pSS) << std::endl;
        // std::cout << "replacing " << Abc_ObjName( pTS ) << " with " << Abc_ObjName( pSS ) << std::endl;
        DASSERT(!(Abc_ObjIsNode(pSS) && Abc_NodeIsConst(pSS)));
        // cout << Abc_ObjName(pTS) << " is replaced by " << Abc_ObjName(pSS) << " with inverter with estimated error " << error << endl;
        Abc_Obj_t * pInv = Abc_NtkCreateNodeInv(pAppNtkDup, pSS);  // complemented...
        alexanderia_replace_obj_s_a_a_d(pTS, pInv);
    }
}

void 
SBMSM_t::alexanderia_replace_obj( Abc_Obj_t * pNodeOld, Abc_Obj_t * pNodeNew )
{
    assert( !Abc_ObjIsComplement(pNodeOld) );
    assert( !Abc_ObjIsComplement(pNodeNew) );
    assert( pNodeOld->pNtk == pNodeNew->pNtk );
    assert( pNodeOld != pNodeNew );
    assert( Abc_ObjFanoutNum(pNodeOld) > 0 );
    // transfer the fanouts to the old node
    // std::cout << "transfering fanouts" << std::endl;
    Abc_ObjTransferFanout( pNodeOld, pNodeNew );
    // std::cout << "deleting the old node" << std::endl;
    // remove the old node
    alexanderia_delete_obj_rec( pNodeOld );
}

void 
SBMSM_t::alexanderia_replace_obj_s_a_a_d( Abc_Obj_t * pNodeOld, Abc_Obj_t * pNodeNew )
{
    assert( !Abc_ObjIsComplement(pNodeOld) );
    assert( !Abc_ObjIsComplement(pNodeNew) );
    assert( pNodeOld->pNtk == pNodeNew->pNtk );
    assert( pNodeOld != pNodeNew );
    assert( Abc_ObjFanoutNum(pNodeOld) > 0 );
    // transfer the fanouts to the old node
    // std::cout << "transfering fanouts" << std::endl;
    Abc_ObjTransferFanout( pNodeOld, pNodeNew );
}

void 
SBMSM_t::batch_deletion_s_a_a_d ()
{
    Abc_Obj_t * pObj; int i;
    Abc_NtkForEachNode( pAppNtkDup, pObj, i )
    {
        if ( Abc_ObjFanoutNum( pObj ) == 0 )
        {
            std::cout << "== batch deleting: " << Abc_ObjName( pObj ) << std::endl;
            alexanderia_delete_obj_rec( pObj );
        }
    }
}

void 
SBMSM_t::alexanderia_delete_obj_rec( Abc_Obj_t * pObj )
{
    Vec_Ptr_t * vNodes;
    int i;
    assert( !Abc_ObjIsComplement(pObj) );
    assert( !Abc_ObjIsPi(pObj) );
    assert( Abc_ObjFanoutNum(pObj) == 0 );
    // delete fanins and fanouts
    vNodes = Vec_PtrAlloc( 100 );
    Abc_NodeCollectFanins( pObj, vNodes );
    std::cout << "delete node: " << Abc_ObjName( pObj ) << std::endl;
    Abc_NtkDeleteObj( pObj );
    pObj->pNtk = NULL;
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
        if ( Abc_ObjIsNode(pObj) && Abc_ObjFanoutNum(pObj) == 0 && !Abc_NodeIsConst(pObj) )
            alexanderia_delete_obj_rec( pObj );
    Vec_PtrFree( vNodes );
}

void 
SBMSM_t::assign_PIIDs( Cnf_Dat_t * miterCnfData, bool print ) 
{
    int i;
    for ( i = 0; i < OriPINum + 1; ++i )
        OriPIIDs[i] = -1;
    for ( i = 0; i < MUXPINum + 1; ++i )
        MUXPIIDs[i] = -1;
    OriPIIDs[0] = OriPINum; MUXPIIDs[0] = MUXPINum;
    for ( i = 0; i < OriPINum; ++i )
        OriPIIDs[i + 1] = miterCnfData->nVars - OriPINum - MUXPINum - 1 + i;
    for ( i = 0; i < MUXPINum; ++i )
        MUXPIIDs[i + 1] = miterCnfData->nVars - MUXPINum - 1 + i;
    // check the IDs
    if ( print )
    {
        std::cout << "---------- the length of OriPIIDs is " << OriPIIDs[0] << std::endl;
        for (i = 1; i < OriPINum + 1; ++i)
            std::cout << "------------- the " << i << " th OriPI's ID of pMiter is " << OriPIIDs[i] << std::endl;
        std::cout << "---------- the length of MUXPIIDs is " << MUXPIIDs[0] << std::endl;
        for (i = 1; i < MUXPINum + 1; ++i)
            std::cout << "------------- the " << i << " th MUXPI's ID of pMiter is " << MUXPIIDs[i] << std::endl;
    }
}

int 
SBMSM_t::sorted_selection_find_least ( std::vector<Abc_Obj_t *> &sortedSelectionSignals )
{
    int left, right, selectionNum, pointer, count = 0;
    Abc_Ntk_t * pMiterDup;
    std::vector<Abc_Obj_t *> sortedSelectionSignalsDup;
    Cnf_Dat_t * miterCnfData;
    Aig_Man_t * miterAigMan;
    // int OriPIIDs[OriPINum+1], MUXPIIDs[MUXPINum+1];
    selectionNum = sortedSelectionSignals.size();   left = 0;   right = selectionNum - 1;
    fprintf( stderr, "the sorting size is %d\n", right );
    for ( int i =0; i < sortedSelectionSignals.size(); ++i )
        assert( Abc_ObjRegular( sortedSelectionSignals[i] )->pNtk == pMiter );

    std::cout << "----------- initial condition: left = " << left << "; right = " << right << std::endl;

    while ( left <= right )
    {
        count ++;
        pointer = floor( ( left + right ) / 2.0 );
        std::cout << "----------- " << count << " round: left = " << left << 
            "; right = " << right << "; pointer = " << pointer << std::endl;
        std::cout << "-------------- duplicating miter" << std::endl;
        pMiterDup = Abc_NtkDup( pMiter );
        assert( Abc_NtkPoNum( pMiterDup ) == 1 );
        sortedSelectionSignalsDup.clear();
        // assign the selection signals of the duplicate network to a new vector
        std::cout << "-------------- duplicating sorted selection signals" << std::endl;
        for ( int i = 0; i < sortedSelectionSignals.size(); ++i )
        {
            sortedSelectionSignalsDup.push_back( 
                Abc_ObjIsComplement( sortedSelectionSignals[i] ) ?
                Abc_ObjNot( Abc_ObjRegular( sortedSelectionSignals[i] )->pCopy ) :
                sortedSelectionSignals[i]->pCopy
            );
            assert( Abc_ObjRegular( sortedSelectionSignalsDup[i] )->pNtk == pMiterDup );
        }
        
        // get the old output and create the new output
        std::cout << "-------------- updating output" << std::endl;
        create_sorted_miter_dup( sortedSelectionSignalsDup, pointer, pMiterDup );
        // write the new network into blif file for checking
        std::cout << "-------------- writing duplicated miter blif" << std::endl;
        Ckt_WriteBlif( pMiterDup, "intermediate-results/Alexanderia_AND_Added_Find_Least_Temporary_Miter.blif" );  // no problem
        // transform miterDup to cnf
        transform_miter_to_cnf_dup( pMiterDup );

        // solve the cnf SAT using qdpll
        QDPLL *depqbf = qdpll_create ();
        qdpll_configure (depqbf, "--dep-man=simple");
        qdpll_configure (depqbf, "--incremental-use");

        std::cout << "-------------- reading cnf into depqbf" << std::endl;
        Cnf_DataFile2Depqbf( cnfFileNameDup, depqbf, OriPIIDs, MUXPIIDs );
        // solve the sat and store the results into the file <SATResultFileName>.
        std::cout << "-------------- solving qsat" << std::endl;
        QDPLLResult res = qdpll_sat ( depqbf );
        std::cout << "-------------- updating boundaries" << std::endl;
        if ( res == QDPLL_RESULT_SAT )
        {
            std::cout << "-------------- SAT! updating right pointer" << std::endl;
            right = pointer - 1;
        }
        else
        {
            std::cout << "-------------- UNSAT! updating left pointer" << std::endl;
            left = pointer + 1;
        }
        qdpll_reset( depqbf );
        FILE * pOut = fopen( "intermediate-results/SAT_Problem_In.qdimacs", "w+" );
        fclose( pOut );

        qdpll_delete ( depqbf );
    }
    return left;
}