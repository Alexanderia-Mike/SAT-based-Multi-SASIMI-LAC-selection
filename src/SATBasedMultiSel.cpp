//
// Created by 娄辰飞 on 2021/3/8.
//

#include "sasimi.h"
#include "cnf2Depqbf.h"


// transfer all the fanouts of <pNodeFrom> to <pNodeTo>
static void Abc_ObjTransferFanoutsExceptMux ( IN Abc_Obj_t * pNodeFrom, IN Abc_Obj_t * pNodeTo, IN Abc_Obj_t * pMux );
// to collect all the fanouts of <pNode> except for the <pMux>, and then store them in <vNodes>
static void Abc_NodeCollectFanoutsExceptMux ( IN Abc_Obj_t * pNode, IN Vec_Ptr_t * vNodes, IN Abc_Obj_t * pMux );
// add muxes that takes inputs as the node and its LAC, and outputs to original fanouts of the old nodes
static void AddMuxes ( IN Abc_Ntk_t * pOriNtk, IN std::vector <LAC_t> & candLACs );
// create a miter network that takes the absolute weighted difference of the outputs of two networks.
//      then compares the difference with the <threshold>.
// REQUIRE: the size of <threshold> should be the same as the number of primary outputs of either network.
static Abc_Ntk_t * CreateMiterXorMulti ( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int threshold[] );
// print the relative information for the added MUX
static void printMuxInfo( Abc_Obj_t * pTS, Abc_Obj_t * pSS, Abc_Obj_t * pMUX );
// print out the pData for the first <printNum> nodes in <pNtk>
static void printFirstXPData( Abc_Ntk_t * pNtk, int printNum );
// build a subtractor that subtracting <X> by <Y>, with both <X> and <Y> represented in unsigned form.
//      The output is also in unsigned form. the integer <n> represents the bit size of <X> and <Y>.
// REQUIRE: <X> and <Y> are vectors of Abc_Obj * that belogns to the network <pNtk>
static Abc_Obj_t ** X_subtract_Y_abs(Abc_Ntk_t * pNtk, Abc_Obj_t * X[], Abc_Obj_t * Y[], int n);
// build a comparator that compares <X> and <Y>. output a single object that values 1 if <X> > <Y> and
//      0 otherwise. the integer <n> represents the bit size of <X> and <Y>.
// REQUIRE: <X> and <Y> should belong to <pNtk>
static Abc_Obj_t * X_lt_Y(Abc_Ntk_t * pNtk, Abc_Obj_t * X[], Abc_Obj_t * Y[], int n);
// return ConstNode 1 if n == 1 and ConstNode 0 if n == 0
static Abc_Obj_t * ConstNode ( Abc_Ntk_t * pNtk, int n );
// return the corresponding array of const nodes with respect to the integer array <threshold>.
// REQUIRE: the size of <threshold> should be the same as <size>
static Abc_Obj_t ** ConstNodes ( Abc_Ntk_t * pNtk, int threshold[], int size );
// to check that everything is Okay with the created Miter
static void MiterCheck ( Abc_Ntk_t * pNtkMiter, Abc_Ntk_t * pNtk1 );
// to extract the IDs of original PIs and MUX PIs in cnf and store them in <OriPIIDs> and <MUXPIIDs>, respectively
void assignPIIDs( Cnf_Dat_t * miterCnfData, int * OriPIIDs, int * MUXPIIDs, int OriPINum, int MUXPINum, bool print );


using namespace std;
using namespace std::filesystem;

void SASIMI_Manager_t::SATBasedMultiSelection ( IN Abc_Ntk_t * pOriNtk, IN std::string outPrefix, int threshold[] )
{
    // init
    char * cnfFileName = "intermediate-results/MiterCNF.cnf";
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
    int ** OriPIIDs = new int *, ** MUXPIIDs = new int *;
    * OriPIIDs = (int *) malloc( 0 ); * MUXPIIDs = (int *) malloc( 0 );

    CreateMuxedCNF( pAppNtk, candLACs, cnfFileName, threshold, OriPIIDs, MUXPIIDs );

    //for (int i = 0; i < 5; ++i)
    //    cout << "the i th OriPI's ID of pMiter is " << (* OriPIIDs)[i] << endl;
    //for (int i = 0; i < 5; ++i)
    //    cout << "the i th MUXPI's ID of pMiter is " << (* MUXPIIDs)[i] << endl;

    // call library depqbf
    QDPLL *depqbf = qdpll_create ();
    Cnf_DataFile2Depqbf( cnfFileName, depqbf );

    // TODO: analyze the result returned by depqbf's SAT solver and generate the approximate circuit, which should
    //  then be stored into a new blif file

    delete [] * OriPIIDs;   delete OriPIIDs;
    delete [] * MUXPIIDs;   delete MUXPIIDs;
}

// add muxes to all the nodes with LAC candidates
void SASIMI_Manager_t::CreateMuxedCNF ( IN Abc_Ntk_t * pMUXedNtk, IN std::vector <LAC_t> & candLACs, IN char * cnfFileName, int threshold[], int ** OriPIIDs, int ** MUXPIIDs )
{
    Abc_Ntk_t * pOriNtk = Abc_NtkDup( pMUXedNtk );
    AddMuxes( pMUXedNtk, candLACs );      // done
    Ckt_WriteBlif( pOriNtk, "intermediate-results/Alexanderia_original.blif" ); // no problem
    Ckt_WriteBlif( pMUXedNtk, "intermediate-results/Alexanderia_MUXAdded.blif" );   // no problem
//    cout << "the checking result of oriNtk is " << Abc_NtkCheck( pOriNtk ) << endl;
//    cout << "the checking result of MUXedNtk is " << Abc_NtkCheck( pMUXedNtk ) << endl;

    Abc_Ntk_t * pNtkOriStrash = Abc_NtkStrash( pOriNtk, 0, 0, 0 );
    Abc_Ntk_t * pNtkMUXedStrash = Abc_NtkStrash( pMUXedNtk, 0, 0, 0 );

    cout << "strash is successful!" << endl;

    Abc_Ntk_t * pMiter = CreateMiterXorMulti ( pNtkMUXedStrash, pNtkOriStrash, threshold);
    assert( Abc_NtkPiNum( pMiter ) == Abc_NtkPiNum( pMUXedNtk ) );

    cout << "miter is created successfully!" << endl;
    Ckt_WriteBlif( pMiter, "intermediate-results/Alexanderia_Miter.blif" );

    // transform the network to cnf
    char * aigFileName = "intermediate-results/MiterAIG.aiger";
    Io_Write(pMiter, aigFileName, IO_FILE_AIGER );  // the index of PIs are indexed from 1 to <NumCIs>. Temporarily stored in <pObj->pCopy>. See lines 699 ~ 704 in "src/base/io/ioWriteAiger.c" for more details
    Aig_Man_t * miterAigMan = Ioa_ReadAiger( aigFileName, 1 );  //
    Aig_ManPrintStats( miterAigMan );
    Cnf_Dat_t * miterCnfData = Cnf_DeriveSimple( miterAigMan, 0 );  // the variable index in cnf is derived from iterating all COs, Internal nodes, CIs in the aig manager. For more details, see lines 624 ~ 630 in "src/sat/cnf/cnfWrite.c"

    // get PI's IDs
    int OriPINum = Abc_NtkPiNum( pNtkOriStrash ), MUXPINum = Abc_NtkPiNum( pMiter ) - OriPINum;
//    int OriPIIDs[OriPINum], MUXPIIDs[MUXPINum];
    * OriPIIDs = (int *) realloc( * OriPIIDs, OriPINum * sizeof( int ) );
    * MUXPIIDs = (int *) realloc( * MUXPIIDs, MUXPINum * sizeof( int ) );
    assignPIIDs( miterCnfData, * OriPIIDs, * MUXPIIDs, OriPINum, MUXPINum, false );

    // write the cnf into the file
    char * cnfFileName = "intermediate-results/MiterCNF.cnf";
    Cnf_DataWriteIntoFile( miterCnfData, (char *) cnfFileName, 0, NULL, NULL );  // since fReadable=0, all the variables are added by 1.

    // debug
//    Abc_Obj_t * pObj; int i; Aig_Obj_t * paObj;
//    cout << "for the PIs in original network, there are in total " << Abc_NtkPiNum( pOriNtk ) << " PIs, and their names are: " << endl;
//    Abc_NtkForEachPi( pNtkOriStrash, pObj, i )
//        cout << Abc_ObjName( pObj ) << ", ";
//    cout << endl << endl << "for the PIs in MUXed network, there are in total " << Abc_NtkPiNum( pMUXedNtk ) << " PIs, and their names are: " << endl;
//    Abc_NtkForEachPi( pNtkMUXedStrash, pObj, i )
//        cout << Abc_ObjName( pObj ) << ", ";
//    cout << endl << endl << "for the PIs in miter network, there are in total " << Abc_NtkPiNum( pMiter ) << " PIs, and their names are: " << endl;
//    Abc_NtkForEachPi( pMiter, pObj, i )
//        cout << Abc_ObjName( pObj ) << ", ";
//    cout << endl << endl << "for the PIs in miter aig, there are in total " << Aig_ManCiNum( miterAigMan ) << " PIs, and their names are: " << endl;
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
//        test = (char *) pMux->pData;
//        cout << "--pData for MUX is " << endl << test << endl;

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
static Abc_Ntk_t * CreateMiterXorMulti ( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int threshold[]) {
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
    // also, get the IDs of the PIs of the miter at the same time
    Abc_NtkForEachPi( pNtk1, pObj, i )
    {
        pObjNew = Abc_NtkCreatePi( pNtkMiter );
        // remember this PI in the old PIs
        pObj->pCopy = pObjNew;
        if ( i < Abc_NtkCiNum( pNtk2 ) ) {
            pObj = Abc_NtkCi(pNtk2, i);
            pObj->pCopy = pObjNew;
        }
        // add name
        Abc_ObjAssignName( pObjNew, Abc_ObjName(pObj), NULL );
    }
    // create the latches
    Abc_NtkForEachLatch( pNtk1, pObj, i )
    {
        pObjNew = Abc_NtkDupBox( pNtkMiter, pObj, 0 );
        // add names
        Abc_ObjAssignName( pObjNew, Abc_ObjName(pObj), "_1" );
        Abc_ObjAssignName( Abc_ObjFanin0(pObjNew),  Abc_ObjName(Abc_ObjFanin0(pObj)), "_1" );
        Abc_ObjAssignName( Abc_ObjFanout0(pObjNew), Abc_ObjName(Abc_ObjFanout0(pObj)), "_1" );
    }
    Abc_NtkForEachLatch( pNtk2, pObj, i )
    {
        pObjNew = Abc_NtkDupBox( pNtkMiter, pObj, 0 );
        // add name
        Abc_ObjAssignName( pObjNew, Abc_ObjName(pObj), "_2" );
        Abc_ObjAssignName( Abc_ObjFanin0(pObjNew),  Abc_ObjName(Abc_ObjFanin0(pObj)), "_2" );
        Abc_ObjAssignName( Abc_ObjFanout0(pObjNew), Abc_ObjName(Abc_ObjFanout0(pObj)), "_2" );
    }
    //    // create POs
    //    Abc_NtkForEachCo( pNtk1, pObj, i )
    //    {
    //        pObjNew = Abc_NtkCreatePo( pNtkMiter );
    //        Abc_ObjAssignName( pObjNew, "miter", Abc_ObjName(pObjNew) );
    //    }

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
    Vec_Ptr_t * vPairs; vPairs = Vec_PtrAlloc( 100 );
    // collect the PO nodes for the miter
    Abc_NtkForEachPo( pNtk1, pNode, i )
    {
        Vec_PtrPush( vPairs, Abc_ObjChild0Copy(pNode) );
        pNode = Abc_NtkPo( pNtk2, i );
        Vec_PtrPush( vPairs, Abc_ObjChild0Copy(pNode) );
    }
    Abc_NtkForEachLatch( pNtk1, pNode, i )
        Abc_ObjAddFanin( Abc_ObjFanin0(pNode)->pCopy, Abc_ObjChild0Copy(Abc_ObjFanin0(pNode)) );
    Abc_NtkForEachLatch( pNtk2, pNode, i )
        Abc_ObjAddFanin( Abc_ObjFanin0(pNode)->pCopy, Abc_ObjChild0Copy(Abc_ObjFanin0(pNode)) );
    // build the output vectors of two networks
    Abc_Obj_t ** X = new Abc_Obj_t *[vPairs->nSize / 2];
    Abc_Obj_t ** Y = new Abc_Obj_t *[vPairs->nSize / 2];
    for(i = 0; i < vPairs->nSize; i += 2)
    {
        X[i / 2] = (Abc_Obj_t *) vPairs->pArray[i];
        Y[i / 2] = (Abc_Obj_t *) vPairs->pArray[i + 1];
    }
    // calculate the weighted difference of the outputs from two networks
    Abc_Obj_t ** Diff = X_subtract_Y_abs(pNtkMiter, X, Y, vPairs->nSize / 2);
    delete[] X, Y;
    // builds the comparator
    int size = Abc_NtkPoNum( pNtk1 );
    Abc_Obj_t ** thresholdNodes = ConstNodes( pNtkMiter, threshold, size );
    Abc_Obj_t * compareRes = X_lt_Y( pNtkMiter, thresholdNodes, Diff, size );
    // create the final output
    Abc_Obj_t * result = Abc_NtkCreatePo( pNtkMiter );
    Abc_ObjAddFanin( result, compareRes );
    //    // collect the CO nodes for the miter
    //    Abc_NtkForEachCo( pNtk1, pNode, i ) {
    //        pMiter = Abc_AigXor( (Abc_Aig_t *)pNtkMiter->pManFunc, Abc_ObjChild0Copy(pNode), Abc_ObjChild0Copy(Abc_NtkCo(pNtk2, i)) );
    //        Abc_ObjAddFanin( Abc_NtkPo(pNtkMiter,i), pMiter );
    //    }

    /************************************* rest of Abc_NtkMiterInt *************************************/

    delete[] Diff;
    Vec_PtrFree( vPairs );
    Abc_AigCleanup((Abc_Aig_t *)pNtkMiter->pManFunc);

    // make sure that everything is okay
    if ( !Abc_NtkCheck( pNtkMiter ) )
    {
        printf( "Abc_NtkMiter: The network check has failed.\n" );
        Abc_NtkDelete( pNtkMiter );
        return NULL;
    }
    MiterCheck( pNtkMiter, pNtk1 );
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

static Abc_Obj_t ** X_subtract_Y_abs(Abc_Ntk_t * pNtk, Abc_Obj_t * X[], Abc_Obj_t * Y[], int n) {
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

static Abc_Obj_t * X_lt_Y(Abc_Ntk_t * pNtk, Abc_Obj_t * X[], Abc_Obj_t * Y[], int n) {
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

static Abc_Obj_t * ConstNode ( Abc_Ntk_t * pNtk, int n ) {
    return n ? Abc_AigConst1( pNtk ) : Abc_ObjNot( ConstNode( pNtk, 1 ) );
}

static Abc_Obj_t ** ConstNodes ( Abc_Ntk_t * pNtk, int threshold[], int size ) {
    Abc_Obj_t ** thresholdNodes = new Abc_Obj_t *[size];
    for ( int i = 0; i < size; ++i )
        thresholdNodes[i] = ConstNode( pNtk, threshold[i] );
    return thresholdNodes;
}

static void MiterCheck ( Abc_Ntk_t * pNtkMiter, Abc_Ntk_t * pNtk1 ) {
    if ( Abc_NtkPoNum( pNtkMiter ) != 1 ) {
        cout << "the number of output of pNtk1 is " << Abc_NtkPoNum(pNtk1) << endl;
        cout << "the number of output is not 1, but " << Abc_NtkPoNum(pNtkMiter)
             << " instead. Miter creation is NOT successful!" << endl;
    }
    // TODO: to check things like pCopy
}

void assignPIIDs( Cnf_Dat_t * miterCnfData, int * OriPIIDs, int * MUXPIIDs, int OriPINum, int MUXPINum, bool print ) {
    int i;
    for ( i = 0; i < OriPINum; ++i )
        OriPIIDs[i] = -1;
    for ( i = 0; i < MUXPINum; ++i )
        MUXPIIDs[i] = -1;
    for ( i = 0; i < OriPINum; ++i )
        OriPIIDs[i] = miterCnfData->nVars - OriPINum - MUXPINum - 1 + i;
    for ( i = 0; i < MUXPINum; ++i )
        MUXPIIDs[i] = miterCnfData->nVars - MUXPINum - 1 + i;
    // check the IDs
    if ( print )
    {
        for (i = 0; i < OriPINum; ++i)
            cout << "the i th OriPI's ID of pMiter is " << OriPIIDs[i] << endl;
        for (i = 0; i < MUXPINum; ++i)
            cout << "the i th MUXPI's ID of pMiter is " << MUXPIIDs[i] << endl;
    }
}