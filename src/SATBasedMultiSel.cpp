//
// Created by 娄辰飞 on 2021/3/8.
//

#include "sasimi.h"

// transfer all the fanouts of <pNodeFrom> to <pNodeTo>
static void Abc_ObjTransferFanoutsExceptMux ( IN Abc_Obj_t * pNodeFrom, IN Abc_Obj_t * pNodeTo, IN Abc_Obj_t * pMux );
// to collect all the fanouts of <pNode> except for the <pMux>, and then store them in <vNodes>
static void Abc_NodeCollectFanoutsExceptMux ( IN Abc_Obj_t * pNode, IN Vec_Ptr_t * vNodes, IN Abc_Obj_t * pMux );
// add muxes that takes inputs as the node and its LAC, and outputs to original fanouts of the old nodes
static void AddMuxes (IN Abc_Ntk_t * pOriNtk, IN std::vector <LAC_t> & candLACs);


using namespace std;
using namespace std::filesystem;

void SASIMI_Manager_t::SATBasedMultiSelection ( IN Abc_Ntk_t * pOriNtk, IN std::string outPrefix)
{
    // init
    PatchConst(pOriNtk); // add 0s and 1s to the original circuit
    Simulator_t oriSmlt(pOriNtk, nFrame);   // initialize the original simulator
    Simulator_t *pOriSmlt = &oriSmlt;       // the pointer to the simulator
    string cnfPrefix = "Muxed_CNF";

    // iteration
    double error = 0;
    vector < vector <tVec> > bds;   // cpm[PO][obj][frameBlock], grouped with length of 64
    vector <Vec_Ptr_t *> vMffcs;    // the vector of Mffcs
    vector <LAC_t> nodeLACs;        // all the LACs
    vector <LAC_t> candLACs;        // all the candidates
    random_device rd;
    clock_t st = clock();
    cntRound = 0;

    cout << "--------------- network traversal begin... ---------------" << endl;
    // unsigned seed = static_cast <unsigned> (rd());   // random generation
    unsigned seed = 2531465778;         // a fixed generation
    cout << "seed = " << seed << endl;
    cout << "maxLevel = " << maxLevel << endl;
    oriSmlt.Input(seed);        // initialize PI for original simulator
    oriSmlt.Simulate();
    // GetCPM(oriSmlt, *pAppSmlt, bds);
    GetCPMOneCut(oriSmlt, *pOriSmlt, bds);
    Abc_NtkDelayTrace(pOriNtk, nullptr, nullptr, 0);    // update the delay time
    CollectMFFC(*pOriSmlt, vMffcs);     // collect the Mffcs of the approximate simulator
    if (metricType == Metric_t::ER)     // get the LACs
        // CollectAllLACsUnderER(oriSmlt, *pAppSmlt, bds, vMffcs, nodeLACs);
        CollectAllLACsUnderER(oriSmlt, *pOriSmlt, bds, vMffcs, nodeLACs);
    else
        CollectAllLACsUnderNMED(oriSmlt, *pOriSmlt, bds, vMffcs, nodeLACs);
    FreeMFFC(vMffcs);                   // free the memory
    // sort the LACs according to their scores
     SortCandLACs(nodeLACs, pOriSmlt->GetFrameNum(), candLACs);
    // apply the best LACs. <res> = 1 if error bound is broke, and 0 if not.
    // int res = ApplyBestLAC(oriSmlt, *pAppSmlt, candLACs, 10, outPrefix, seed);

    // generate CNF expression according to the muxed network
    CreateMuxedCNF(pOriNtk, candLACs, cnfPrefix);

    // not consider them yet
//    // solve the CNF using SAT and return the rersult thorugh <res>
//    int res = SatSolveApply(cnfPrefix, outPrefix);
//    if (!res)
//        cout << "no available LAC exists" << endl;
//    cout << "time = " << clock() - st << " us" << endl;
}

// not finished
void SASIMI_Manager_t::CreateMuxedCNF ( IN Abc_Ntk_t * pOriNtk, IN std::vector <LAC_t> & candLACs, IN std::string cnfPrefix )
{
    // add muxes to all the nodes with LAC candidates
    Abc_Ntk_t * pMUXedNtk = Abc_NtkDup(pOriNtk);
    AddMuxes(pMUXedNtk, candLACs);      // done
    Ckt_WriteBlif( pMUXedNtk, "appNtk/Alexanderia_test_1.blif" );

    // not consider them yet
//    // write the networks to aiger format
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
    for ( auto & candLAC : candLACs ) {
        Abc_Obj_t * selection = Abc_NtkCreatePi(pOriNtk);
        Abc_Obj_t * pMux = Abc_NtkCreateNodeMux(pOriNtk, selection, candLAC.GetTS(), candLAC.GetSS() );
        Abc_ObjTransferFanoutsExceptMux( candLAC.GetSS(), pMux, pMux );
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