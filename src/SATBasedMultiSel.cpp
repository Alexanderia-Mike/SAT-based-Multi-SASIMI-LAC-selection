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
    string cnfPrefix = "AppTestOutput/MUXedCnf";
    Abc_Ntk_t * pAppNtk = Abc_NtkDup(pOriNtk);
    PatchConst(pOriNtk); // add 0s and 1s to the original circuit
    PatchConst(pAppNtk); // add 0s and 1s to the approximate circuit
    Simulator_t oriSmlt(pOriNtk, nFrame); // initialize the

    Abc_NtkMapToSop( pAppNtk );
    Abc_NtkSopToAig( pAppNtk ); // transform to AIG

    // iteration
    double error = 0;
    vector < vector <tVec> > bds;   // cpm[PO][obj][frameBlock], grouped with length of 64
    vector <Vec_Ptr_t *> vMffcs;    // the vector of Mffcs
    vector <LAC_t> nodeLACs;        // all the LACs
    vector <LAC_t> candLACs;        // all the candidates
    random_device rd;
    clock_t st = clock();
    cntRound = 0;
    cout << "--------------- round " << ++cntRound << " ---------------" << endl;
    // if (cntRound == 4)
    //     break;
    // initialize the simulator with the approximate circuit
    Simulator_t * pAppSmlt = new Simulator_t(pAppNtk, nFrame);
    // unsigned seed = static_cast <unsigned> (rd());   // random generation
    unsigned seed = 2531465778;         // a fixed generation
    cout << "seed = " << seed << endl;
    cout << "maxLevel = " << maxLevel << endl;
    oriSmlt.Input(seed);        // initialize PI for original simulator
    oriSmlt.Simulate();
    pAppSmlt->Input(seed);      // initialize PI for approximate simulator
    pAppSmlt->Simulate();
    // GetCPM(oriSmlt, *pAppSmlt, bds);
    GetCPMOneCut(oriSmlt, *pAppSmlt, bds);
//    Abc_NtkDelayTrace(pAppNtk, nullptr, nullptr, 0);    // update the delay time
    CollectMFFC(*pAppSmlt, vMffcs);     // collect the Mffcs of the approximate simulator
    if (metricType == Metric_t::ER)     // get the LACs
        CollectAllLACsUnderER(oriSmlt, *pAppSmlt, bds, vMffcs, nodeLACs);
    else
        CollectAllLACsUnderNMED(oriSmlt, *pAppSmlt, bds, vMffcs, nodeLACs);
    FreeMFFC(vMffcs);                   // free the memory
    // sort the LACs according to their scores
    SortCandLACs(nodeLACs, pAppSmlt->GetFrameNum(), candLACs);

    // generate CNF expression according to the muxed network

    Ckt_WriteBlif( pAppNtk, "appNtk/pAppNtk.blif" );
    CreateMuxedCNF(pAppNtk, candLACs, cnfPrefix);

    // not consider them yet
//    // solve the CNF using SAT and return the rersult thorugh <res>
//    int res = SatSolveApply(cnfPrefix, outPrefix);
//    if (!res)
//        cout << "no available LAC exists" << endl;
//    cout << "time = " << clock() - st << " us" << endl;
}

// not finished
void SASIMI_Manager_t::CreateMuxedCNF ( IN Abc_Ntk_t * pMUXedNtk, IN std::vector <LAC_t> & candLACs, IN std::string cnfPrefix )
{
    // add muxes to all the nodes with LAC candidates
    Abc_Ntk_t * pOriNtk = Abc_NtkDup( pMUXedNtk );
    AddMuxes( pMUXedNtk, candLACs );      // done
    Ckt_WriteBlif( pOriNtk, "appNtk/Alexanderia_original.blif" );
    Ckt_WriteBlif( pMUXedNtk, "appNtk/Alexanderia_MUXAdded.blif" );
//    Abc_Obj_t * pObj;
//    int i, j;
//    Abc_Obj_t * pFanin;
//    Abc_NtkForEachObj( pMUXedNtk, pObj, i ) {
//        cout << Abc_ObjName( pObj ) << ":";
//        Abc_ObjForEachFanin( pObj, pFanin, j )
//            cout << Abc_ObjName( pFanin ) << " ";
//        cout << endl;
//    }

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
        Abc_Obj_t * pMux = Abc_NtkCreateNodeMux( pOriNtk, selection, candLAC.GetSS(), candLAC.GetTS() );
        Abc_ObjTransferFanoutsExceptMux( candLAC.GetTS(), pMux, pMux );
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