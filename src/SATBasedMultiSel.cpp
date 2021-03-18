//
// Created by 娄辰飞 on 2021/3/8.
//

#include "sasimi.h"


// transfer all the fanouts of <pNodeFrom> to <pNodeTo>
static void Abc_ObjTransferFanoutsExceptMux ( IN Abc_Obj_t * pNodeFrom, IN Abc_Obj_t * pNodeTo, IN Abc_Obj_t * pMux );
// to collect all the fanouts of <pNode> except for the <pMux>, and then store them in <vNodes>
static void Abc_NodeCollectFanoutsExceptMux ( IN Abc_Obj_t * pNode, IN Vec_Ptr_t * vNodes, IN Abc_Obj_t * pMux );
// add muxes that takes inputs as the node and its LAC, and outputs to original fanouts of the old nodes
static void AddMuxes ( IN Abc_Ntk_t * pOriNtk, IN std::vector <LAC_t> & candLACs );
// create a miter network that combines each PO from <pNtk1> and <pNtk2> using Xors
static Abc_Ntk_t * CreateMiterXorMulti ( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2 );
// print the relative information for the added MUX
static void printMuxInfo( Abc_Obj_t * pTS, Abc_Obj_t * pSS, Abc_Obj_t * pMUX );

static void printFirstXPData( Abc_Ntk_t * pNtk, int printNum );

using namespace std;
using namespace std::filesystem;

void SASIMI_Manager_t::SATBasedMultiSelection ( IN Abc_Ntk_t * pOriNtk, IN std::string outPrefix)
{
    // init
    string cnfPrefix = "AppTestOutput/MUXedCnf";
    Abc_Ntk_t * pAppNtk = Abc_NtkDup(pOriNtk);
    PatchConst(pOriNtk); // add 0s and 1s to the original circuit
    PatchConst(pAppNtk); // add 0s and 1s to the approximate circuit
    Simulator_t oriSmlt(pOriNtk, nFrame); // initialize the oriSimulator
//    printFirstXPData( pAppNtk, 10 );    // print the pData for the first 10 nodes in <pAppNtk>

    // iteration
    double error = 0;
    vector < vector <tVec> > bds;   // cpm[PO][obj][frameBlock], grouped with length of 64
    vector <Vec_Ptr_t *> vMffcs;    // the vector of Mffcs
    vector <LAC_t> nodeLACs;        // all the LACs
    vector <LAC_t> candLACs;        // all the candidates
    random_device rd;

    cntRound = 0;
    cout << "--------------- SAT based Multiple LAC Selection now begins ---------------" << endl;
    Simulator_t * pAppSmlt = new Simulator_t(pAppNtk, nFrame);
    // unsigned seed = static_cast <unsigned> (rd());   // random generation
    unsigned seed = 2531465778;         // a fixed generation
    cout << "seed = " << seed << endl;
    cout << "maxLevel = " << maxLevel << endl;
    oriSmlt.Input(seed);        // initialize PI for original simulator
    oriSmlt.Simulate();
    pAppSmlt->Input(seed);      // initialize PI for approximate simulator
    pAppSmlt->Simulate();
    GetCPMOneCut(oriSmlt, *pAppSmlt, bds);
    CollectMFFC(*pAppSmlt, vMffcs);     // collect the Mffcs of the approximate simulator
    if (metricType == Metric_t::ER)     // get the LACs
        CollectAllLACsUnderER(oriSmlt, *pAppSmlt, bds, vMffcs, nodeLACs);
    else
        CollectAllLACsUnderNMED(oriSmlt, *pAppSmlt, bds, vMffcs, nodeLACs);
    FreeMFFC(vMffcs);                   // free the memory
    // sort the LACs according to their scores
    SortCandLACs(nodeLACs, pAppSmlt->GetFrameNum(), candLACs);

    // generate CNF expression according to the muxed network
    CreateMuxedCNF(pAppNtk, candLACs, cnfPrefix);

    // not consider them yet
//    // solve the CNF using SAT and return the rersult thorugh <res>
//    int res = SatSolveApply(cnfPrefix, outPrefix);
//    if (!res)
//        cout << "no available LAC exists" << endl;
//    cout << "time = " << clock() - st << " us" << endl;
}

// add muxes to all the nodes with LAC candidates
void SASIMI_Manager_t::CreateMuxedCNF ( IN Abc_Ntk_t * pMUXedNtk, IN std::vector <LAC_t> & candLACs, IN std::string cnfPrefix )
{
    Abc_Ntk_t * pOriNtk = Abc_NtkDup( pMUXedNtk );
    AddMuxes( pMUXedNtk, candLACs );      // done
    Ckt_WriteBlif( pOriNtk, "appNtk/Alexanderia_original.blif" ); // no problem
    Ckt_WriteBlif( pMUXedNtk, "appNtk/Alexanderia_MUXAdded.blif" );   // no problem
    cout << "the checking result of oriNtk is " << Abc_NtkCheck( pOriNtk ) << endl;
    cout << "the checking result of MUXedNtk is " << Abc_NtkCheck( pMUXedNtk ) << endl;

    Abc_Ntk_t * pNtkOriStrash = Abc_NtkStrash( pOriNtk, 0, 0, 0 );
    Abc_Ntk_t * pNtkMUXedStrash = Abc_NtkStrash( pMUXedNtk, 0, 0, 0 );

    cout << "strash is successful!" << endl;

    Abc_Ntk_t * pMiter = CreateMiterXorMulti ( pNtkOriStrash, pNtkMUXedStrash );
//    Abc_Ntk_t * pMiter = CreateMiterXorMulti ( pOriNtk, pMUXedNtk );
    Ckt_WriteBlif( pMiter, "appNtk/Alexanderia_Miter.blif" );

    // not consider them yet
    // write the networks to aiger format
//    char * pFileNameMUXed = "MUXedNtk.aig", * pFileNameOri = "OriNtk.aig";
//    Io_Write(pMUXedNtk, pFileNameMUXed, IO_FILE_AIGER );
//    Io_Write(pOriNtk, pFileNameOri, IO_FILE_AIGER );
//    // read the aig files just created
//    Aig_Man_t * aigManMUXed = Ioa_ReadAiger(pFileNameMUXed, 1);
//    Aig_ManPrintStats(aigManMUXed);
//    Aig_Man_t * aigManOri = Ioa_ReadAiger(pFileNameOri, 1);
//    Aig_ManPrintStats(aigManOri);
//    // create a mitor of the Muxed network and the original network
//    Aig_Man_t * aigManMiter = Aig_ManCreateMiter();
//    // transform the aig to cnf
//    Cnf_Data_t * cnfExpr = Cnf_DeriveSimple(aigMan,0);
//    // add additional clauses to <cnfExpr> corresponding to universal PI quantitfication
//    AddUnivPIQnt(cnfExpr);
//    // using sat solver to solve the cnf
//    sat_solver * solver = sat_solver_new();
//    solver = (sat_solver *) Cnf_DataWriteIntoSolver(cnfExpr, 1, 0);
}

void AddMuxes ( IN Abc_Ntk_t * pOriNtk, IN std::vector <LAC_t> & candLACs ) {
    int count = 0; int i;
    bool continue_flag; // if true, continue the outer loop
    Abc_Obj_t * pObj;
    for ( auto & candLAC : candLACs ) {
        string test;
        continue_flag = false;
        Abc_ObjForEachFanout( candLAC.GetTS(), pObj, i )
            if ( pObj->Id == candLAC.GetSS()->Id )
                { continue_flag = true;   break; }
        if ( continue_flag )    continue;
        Abc_Obj_t * selection = Abc_NtkCreatePi(pOriNtk);
        Abc_Obj_t * pMux = Abc_NtkCreateNodeMux( pOriNtk, selection, candLAC.GetSS(), candLAC.GetTS() );
        // print out informations
//        cout << "the mux is created, the name is: " << Abc_ObjName( pMux ) << endl;
//        cout << "--the name of the TS and SS is " << Abc_ObjName( candLAC.GetTS() ) << " and " << Abc_ObjName( candLAC.GetSS() ) << ", respectively" << endl;
//        cout << "--before the fanout transformation:" << endl;
//        printMuxInfo( candLAC.GetTS(), candLAC.GetSS(), pMux );
        // transfer all the fanouts of TS to MUX, except for the MUX itself
        Abc_ObjTransferFanoutsExceptMux( candLAC.GetTS(), pMux, pMux );
        // print out information
//        cout << "--after the fanout transformation:" << endl << endl;
//        printMuxInfo( candLAC.GetTS(), candLAC.GetSS(), pMux );

        count ++;
        test = (char *) pMux->pData;
        cout << "--pData for MUX is " << endl << test << endl;

//        if ( count >= 1 )
//            break;
    }
}

void Abc_ObjTransferFanoutsExceptMux ( IN Abc_Obj_t * pNodeFrom, IN Abc_Obj_t * pNodeTo, IN Abc_Obj_t * pMux )
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
    Abc_NodeCollectFanoutsExceptMux( pNodeFrom, vFanouts, pMux );
    // patch the fanin of each of them
    for ( i = 0; i < vFanouts->nSize; i++ )
        // replace the path "pArray[i]==>pNodeFrom" to "pArray[i]==>pNodeTo"
        Abc_ObjPatchFanin( (Abc_Obj_t *)vFanouts->pArray[i], pNodeFrom, pNodeTo );
    assert( Abc_ObjFanoutNum(pNodeFrom) == 1 );
    assert( Abc_ObjFanoutNum(pNodeTo) == nFanoutsOld + vFanouts->nSize );
    Vec_PtrFree( vFanouts );

}

void Abc_NodeCollectFanoutsExceptMux ( IN Abc_Obj_t * pNode, IN Vec_Ptr_t * vNodes, IN Abc_Obj_t * pMux )
{
    Abc_Obj_t * pFanout;
    int i;
    Vec_PtrClear(vNodes);
    Abc_ObjForEachFanout( pNode, pFanout, i ) {
        if ( pFanout->Id != pMux->Id )    // not the pMux
            Vec_PtrPush(vNodes, pFanout);
    }
}

// <pNtk1> should be the MUXed network, while <pNtk2> is the original network
static Abc_Ntk_t * CreateMiterXorMulti ( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2 ) {
    char Buffer[1000];
    Abc_Ntk_t * pNtkMiter;

    assert( Abc_NtkIsStrash(pNtk1) );
    assert( Abc_NtkIsStrash(pNtk2) );

    // start the new network
    pNtkMiter = Abc_NtkAlloc( ABC_NTK_STRASH, ABC_FUNC_AIG, 1 );
    sprintf( Buffer, "%s_%s_miter", pNtk1->pName, pNtk2->pName );
    pNtkMiter->pName = Extra_UtilStrsav(Buffer);

    // perform strashing
    /************************************* Abc_NtkMiterPrepare *************************************/
    //    Abc_NtkMiterPrepare( pNtk1, pNtk2, pNtkMiter, fComb, nPartSize, fMulti );
    //  fComb = 1; fMulti = 1; fImplicit is not important; nPartSize is not important
    Abc_Obj_t * pObj, * pObjNew;
    int i;
    Abc_AigConst1(pNtk1)->pCopy = Abc_AigConst1(pNtkMiter);
    Abc_AigConst1(pNtk2)->pCopy = Abc_AigConst1(pNtkMiter);
    // create new PIs and remember them in the old PIs
    Abc_NtkForEachCi( pNtk1, pObj, i )
    {
        pObjNew = Abc_NtkCreatePi( pNtkMiter );
        // remember this PI in the old PIs
        pObj->pCopy = pObjNew;
        if ( i < Abc_NtkPiNum( pNtk2 ) ) {
            pObj = Abc_NtkCi(pNtk2, i);
            pObj->pCopy = pObjNew;
        }
        // add name
        Abc_ObjAssignName( pObjNew, Abc_ObjName(pObj), NULL );
    }
    // create POs
    Abc_NtkForEachCo( pNtk1, pObj, i )
    {
        pObjNew = Abc_NtkCreatePo( pNtkMiter );
        Abc_ObjAssignName( pObjNew, "miter", Abc_ObjName(pObjNew) );
    }

    /************************************* Abc_NtkMiterAddOne *************************************/
    //    Abc_NtkMiterAddOne( pNtk1, pNtkMiter );
    Abc_Obj_t * pNode;
    assert( Abc_NtkIsDfsOrdered(pNtk1) );
    Abc_AigForEachAnd( pNtk1, pNode, i )
        pNode->pCopy = Abc_AigAnd( (Abc_Aig_t *)pNtkMiter->pManFunc, Abc_ObjChild0Copy(pNode), Abc_ObjChild1Copy(pNode) );

    //    Abc_NtkMiterAddOne( pNtk2, pNtkMiter );
    assert( Abc_NtkIsDfsOrdered(pNtk2) );
    Abc_AigForEachAnd( pNtk2, pNode, i )
        pNode->pCopy = Abc_AigAnd( (Abc_Aig_t *)pNtkMiter->pManFunc, Abc_ObjChild0Copy(pNode), Abc_ObjChild1Copy(pNode) );

    /************************************* Abc_NtkMiterFinalize *************************************/
    //    Abc_NtkMiterFinalize( pNtk1, pNtk2, pNtkMiter, fComb, nPartSize, fImplic, fMulti );
    Abc_Obj_t * pMiter;
    // collect the CO nodes for the miter
    Abc_NtkForEachCo( pNtk1, pNode, i ) {
        pMiter = Abc_AigXor( (Abc_Aig_t *)pNtkMiter->pManFunc, Abc_ObjChild0Copy(pNode), Abc_ObjChild0Copy(Abc_NtkCo(pNtk2, i)) );
        Abc_ObjAddFanin( Abc_NtkPo(pNtkMiter,i), pMiter );
    }

    /************************************* rest of Abc_NtkMiterInt *************************************/

    Abc_AigCleanup((Abc_Aig_t *)pNtkMiter->pManFunc);

    // make sure that everything is okay
    if ( !Abc_NtkCheck( pNtkMiter ) )
    {
        printf( "Abc_NtkMiter: The network check has failed.\n" );
        Abc_NtkDelete( pNtkMiter );
        return NULL;
    }
    return pNtkMiter;
}

static void printMuxInfo( Abc_Obj_t * pTS, Abc_Obj_t * pSS, Abc_Obj_t * pMUX ) {
    Abc_Obj_t * pFanout, * pFanin;
    int i;
    cout << "----the number of fanouts of TS is " << Abc_ObjFanoutNum( pTS ) << endl;
    Abc_ObjForEachFanout( pTS, pFanout, i )
        cout << "------the " << i << "th fanout's name of TS is " << Abc_ObjName( pFanout ) << endl;
    cout << "----the number of fanouts of SS is " << Abc_ObjFanoutNum( pSS ) << endl;
    Abc_ObjForEachFanout( pSS, pFanout, i )
        cout << "------the " << i << "th fanout's name of SS is " << Abc_ObjName( pFanout ) << endl;
    cout << "----the number of fanins of MUX is " << Abc_ObjFaninNum( pMUX ) << endl;
    Abc_ObjForEachFanin( pMUX, pFanin, i )
        cout << "-------the " << i << "th fanin's name of MUX is " << Abc_ObjName( pFanin ) << endl;
    cout << "----the number of fanouts of MUX is " << Abc_ObjFanoutNum( pMUX ) << endl;
    Abc_ObjForEachFanout( pMUX, pFanout, i )
        cout << "-------the " << i << "th fanout's name of MUX is " << Abc_ObjName( pFanout ) << endl;
}

static void printFirstXPData( Abc_Ntk_t * pNtk, int printNum ) {
    Abc_Obj_t * pNode;
    int i, j=0;
    string test;
    cout << "the pData for the first " << printNum << " nodes for the network " << Abc_NtkName( pNtk ) << endl;
    Abc_NtkForEachNode( pNtk, pNode, i ) {
            test = (char *) pNode->pData;
            cout << "pData is " << test << endl;
            ++j;
            if ( j >= printNum )   break;
        }
}