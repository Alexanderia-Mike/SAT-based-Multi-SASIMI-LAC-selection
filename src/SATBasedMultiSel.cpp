//
// Created by 娄辰飞 on 2021/3/8.
//

#include "sasimi.h"
#include "cnf2Depqbf.h"
#include "sortingNetwork.h"


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
// build a comparator that compares <X> and <Y>. output a single object that values 1 if <X> < <Y> and
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
//      Note that <OriPIIDs[0]> stores the number of OriPIs, and the <OriPIIDs[1~OriPINum]> stores the data for indices.
//      The same for <MUXPIIDs[]>
void assignPIIDs( Cnf_Dat_t * miterCnfData, int * OriPIIDs, int * MUXPIIDs, int OriPINum, int MUXPINum, bool print );
// return true if all the nodes in <pNtk> have level=0
static bool AllLevelZero( Abc_Ntk_t * pNtk, bool print );
// adding the sorting network to all the selection signals in the miter
// return the vector of the sorted selection signals
static void Miter_Add_Sorting_Network( Abc_Ntk_t * pMiter, int oriPINum, std::vector<Abc_Obj_t *> &sortedSelectionSignals );
// use bisection algorithm to find the sorted selection signal with the least index which can be 1 with the presupposition
// that the output of the miter is 1, under the quantification of inputs in cnf
static int Sorted_Selection_Find_Least ( Abc_Ntk_t * pMiter, std::vector<Abc_Obj_t *> &sortedSelectionSignals, int OriPINum, int MUXPINum );
// to copy the level information from originnal network to copied network
static void Abc_NtkDupLevel( Abc_Ntk_t * pOriNtk, Abc_Ntk_t * pCopiedNtk );


using namespace std;
using namespace std::filesystem;

void SASIMI_Manager_t::SATBasedMultiSelection ( IN Abc_Ntk_t * pOriNtk, IN std::string outPrefix, int threshold[] )
{
    // init
    char * cnfFileName = "intermediate-results/Final_Miter_CNF.cnf";
    char * SATResultFileName = "intermediate-results/SAT_Problem_QDIMACS.qdimacs";
    Abc_Ntk_t * pAppNtk = Abc_NtkDup(pOriNtk);
    Abc_NtkDupLevel( pOriNtk, pAppNtk );
    if ( AllLevelZero( pAppNtk, false ) )
        cout << "all the objects have level 0!" << endl;
    PatchConst(pOriNtk); // add 0s and 1s to the original circuit
    PatchConst(pAppNtk); // add 0s and 1s to the approximate circuit
    Simulator_t oriSmlt(pOriNtk, nFrame); // initialize the oriSimulator
    int ** pOriPIIDs = new int *, ** pMUXPIIDs = new int *;
//    printFirstXPData( pAppNtk, 10 );    // print the pData for the first 10 nodes in <pAppNtk>

    // iteration
    double error = 0;
    vector < vector <tVec> > bds;   // cpm[PO][obj][frameBlock], grouped with length of 64
    vector <Vec_Ptr_t *> vMffcs;    // the vector of Mffcs
    vector <LAC_t> nodeLACs;        // all the LACs
    vector <LAC_t> candLACs;        // all the candidates
    random_device rd;

    cntRound = 0;
    cout << "# SAT based Multiple LAC Selection now begins ---------------" << endl;
    Simulator_t * pAppSmlt = new Simulator_t(pAppNtk, nFrame);
    unsigned seed = static_cast <unsigned> (rd());   // random generation
    // unsigned seed = 2531465778;         // a fixed generation
    cout << "--- seed = " << seed << endl;
    cout << "--- maxLevel = " << maxLevel << endl;
    oriSmlt.Input(seed);        // initialize PI for original simulator
    oriSmlt.Simulate();
    pAppSmlt->Input(seed);      // initialize PI for approximate simulator
    pAppSmlt->Simulate();
    cout << "--- getting CPM-one-cut" << endl;
    GetCPMOneCut(oriSmlt, *pAppSmlt, bds);
    cout << "--- collecting mFFC" << endl;
    CollectMFFC(*pAppSmlt, vMffcs);     // collect the Mffcs of the approximate simulator
    
    cout << "--- collecting LACs" << endl;
    if (metricType == Metric_t::ER)     // get the LACs
        CollectAllLACsUnderER(oriSmlt, *pAppSmlt, bds, vMffcs, nodeLACs);
    else
        CollectAllLACsUnderNMED(oriSmlt, *pAppSmlt, bds, vMffcs, nodeLACs);
    FreeMFFC(vMffcs);                   // free the memory
    // sort the LACs according to their scores
    cout << "--- sorting LACs" << endl;
    SortCandLACs(nodeLACs, pAppSmlt->GetFrameNum(), candLACs);
    cout << "--- the number of candidate LACs is " << candLACs.size() << endl;

    // generate CNF expression according to the muxed network
    cout << "# Creating MUXed CNF! ------------------------------ " << endl;
    * pOriPIIDs = (int *) malloc( 0 ); * pMUXPIIDs = (int *) malloc( 0 );
    CreateMuxedCNF( pAppNtk, candLACs, cnfFileName, threshold, pOriPIIDs, pMUXPIIDs );

    // call library depqbf
    cout << "# Solving Quantified SAT Problem! ------------------------------ " << endl;
    QDPLL *depqbf = qdpll_create ();
    qdpll_configure (depqbf, "--dep-man=simple");
    qdpll_configure (depqbf, "--incremental-use");

    cout << "------- Loading CNF to Depqbf!" << endl;
    Cnf_DataFile2Depqbf( cnfFileName, depqbf, * pOriPIIDs, * pMUXPIIDs );
    // solve the sat and store the results into the file <SATResultFileName>.
    // qdpll_reset( depqbf );
    cout << "------- Solving the Result!" << endl;
    QDPLLResult res = QDPLL_SolveSatWriteAssignments( depqbf, SATResultFileName );

    // TODO: generate the resulting approximate network and store it into a new blif file

    // TODO: evaluate the area saved by this approximate local change.

    // free the dynamically allocated memory
    cout << "------- Clearing Memory!" << endl;
    delete [] * pOriPIIDs;   delete pOriPIIDs;
    delete [] * pMUXPIIDs;   delete pMUXPIIDs;
    /* Delete solver instance. */
    qdpll_delete ( depqbf );
}

// add muxes to all the nodes with LAC candidates
void SASIMI_Manager_t::CreateMuxedCNF ( IN Abc_Ntk_t * pMUXedNtk, IN std::vector <LAC_t> & candLACs, IN char * cnfFileName, int threshold[], int ** pOriPIIDs, int ** pMUXPIIDs )
{
    Abc_Ntk_t * pOriNtk = Abc_NtkDup( pMUXedNtk ), * pNtkOriStrash, * pNtkMUXedStrash, * pMiter;
    char * aigFileName = "intermediate-results/Alexanderia_Final_Miter_AIG.aiger";
    char * naiveMiterName = "intermediate-results/Alexanderia_Naive_Miter.blif";
    Cnf_Dat_t * miterCnfData;
    int OriPINum, MUXPINum, selectionIndex;
    std::vector<Abc_Obj_t *> sortedSelectionSignals;
    Abc_Obj_t * pNewOutput, * pOriOutput, * pOriOutputFanin;

    cout << "------- Adding MUX!" << endl;
    AddMuxes( pMUXedNtk, candLACs );      // done
    Ckt_WriteBlif( pMUXedNtk, "intermediate-results/Alexanderia_MUXed_Network.blif" );   // no problem

    cout << "------- Strashing Original and MUXed Network!" << endl;
    pNtkOriStrash = Abc_NtkStrash( pOriNtk, 0, 0, 0 );
    pNtkMUXedStrash = Abc_NtkStrash( pMUXedNtk, 0, 0, 0 );

    // create the miter
    cout << "------- Creating Miter!" << endl;
    pMiter = CreateMiterXorMulti ( pNtkMUXedStrash, pNtkOriStrash, threshold);
    assert( Abc_NtkPiNum( pMiter ) == Abc_NtkPiNum( pMUXedNtk ) );
    cout << "------- Writing Miter Blif!" << endl;
    Ckt_WriteBlif( pMiter, naiveMiterName );

    cout << "-------------- Miter is created successfully!" << endl 
        << "-------------- the number of PI in original network is " << Abc_NtkPiNum( pOriNtk ) << endl 
        << "-------------- the number of PI in miter is " << Abc_NtkPiNum( pMiter ) << endl;
    OriPINum = Abc_NtkPiNum( pNtkOriStrash );   MUXPINum = Abc_NtkPiNum( pMiter ) - OriPINum;
    
    // add the sorting network.
    cout << "------- Adding Sorting Network!" << endl;
    Miter_Add_Sorting_Network( pMiter, OriPINum, sortedSelectionSignals );
    assert( Abc_NtkPoNum( pMiter ) == 1 );

    // find the least index of the selection signal that could be ANDed with the miter output
    cout << "------- Choosing the Least Valid Selection Signal!" << endl;
    selectionIndex = Sorted_Selection_Find_Least( pMiter, sortedSelectionSignals, OriPINum, MUXPINum );
    cout << "---------- The least index of the valid selection signal is: " << selectionIndex << endl;

    // AND the selection signal with the output
    cout << "------- ANDing the Selection Signal with the Original Output!" << endl;
    pOriOutput = Abc_NtkPo( pMiter, 0 );    // old output
    pOriOutputFanin = Abc_ObjFanin0( pOriOutput );  // get the fanin of the old output
    pNewOutput = Abc_NtkCreatePo( pMiter ); // create the new output
    // add the AND(fanin, selection-signal) to the new output
    Abc_ObjAddFanin( pNewOutput, Abc_AigAnd( ( Abc_Aig_t * ) pMiter->pManFunc, pOriOutputFanin, sortedSelectionSignals[selectionIndex] ) );
    Abc_NtkDeleteObj( pOriOutput ); // delete the old output
    assert( Abc_NtkPoNum( pMiter ) == 1 );

    // transform the network to aig
    cout << "------- Transforming to AIG!" << endl << "---------- ";
    Io_Write(pMiter, aigFileName, IO_FILE_AIGER );  // the index of PIs are indexed from 1 to <NumCIs>. Temporarily stored in <pObj->pCopy>. See lines 699 ~ 704 in "src/base/io/ioWriteAiger.c" for more details
    Aig_Man_t * miterAigMan = Ioa_ReadAiger( aigFileName, 1 );
    Aig_ManPrintStats( miterAigMan );

    // transform the aig to cnf
    cout << "------- Transforming to CNF!" << endl;
    miterCnfData = Cnf_DeriveSimple( miterAigMan, 0 );  // the variable index in cnf is derived from iterating all COs, Internal nodes, CIs in the aig manager. For more details, see lines 624 ~ 630 in "src/sat/cnf/cnfWrite.c"

    // get PI's IDs
    cout << "------- Assigning PIIDs!" << endl;
    * pOriPIIDs = (int *) realloc( * pOriPIIDs, ( OriPINum + 1 ) * sizeof( int ) );
    * pMUXPIIDs = (int *) realloc( * pMUXPIIDs, ( MUXPINum + 1 ) * sizeof( int ) );
    assignPIIDs( miterCnfData, * pOriPIIDs, * pMUXPIIDs, OriPINum, MUXPINum, true );

    // write the cnf into the file
    cout << "------- Writing Miter CNF!" << endl;
    Cnf_DataWriteIntoFile( miterCnfData, (char *) cnfFileName, 1, NULL, NULL );  // since fReadable=1, all the variables can be read by file without any trouble.
}

void AddMuxes ( IN Abc_Ntk_t * pOriNtk, IN std::vector <LAC_t> & candLACs ) {
    int count = 0, candLACNum = candLACs.size();
    double previousProcession = 0, currentProcession = 0;
    bool continue_flag; int i; // if true, continue the outer loop
    Abc_Obj_t * pObj;
    for ( auto & candLAC : candLACs ) {
        string test;
        
        continue_flag = false;
        Abc_ObjForEachFanout( candLAC.GetTS(), pObj, i )
            if ( pObj->Id == candLAC.GetSS()->Id )
            { 
                std::cout << "shit! the TS node is " << Abc_ObjName( candLAC.GetTS() ) <<
                    " with level " << Abc_ObjLevel( candLAC.GetTS() ) << ", whiile the SS node is " << 
                    Abc_ObjName( candLAC.GetSS() ) << " with level " << Abc_ObjLevel( candLAC.GetSS() ) << std::endl;
                // level( SS ) < level( TS ).
                continue_flag = true;
                break;
            }
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
        currentProcession = count / (double) candLACNum;
        if( currentProcession >= 0.1 && currentProcession - previousProcession >= 0.1 )
        {
            cout << "-------------- " << count << " (out of " << candLACNum << ") MUXes finished" << endl;
            previousProcession = count / (double) candLACNum;
        }
//        test = (char *) pMux->pData;
//        cout << "--pData for MUX is " << endl << test << endl;

//        if ( count >= 1 )
//            break;
    }
    // TODO: to add a conditional block telling whether the node SS needs to be complemented.
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
    Abc_Obj_t ** X = new Abc_Obj_t *[vPairs->nSize / 2];    // MUXed
    Abc_Obj_t ** Y = new Abc_Obj_t *[vPairs->nSize / 2];    // original
    for(i = 0; i < vPairs->nSize; i += 2)
    {
        X[i / 2] = (Abc_Obj_t *) vPairs->pArray[i];
        Y[i / 2] = (Abc_Obj_t *) vPairs->pArray[i + 1];
    }
    // calculate the weighted difference of the outputs from two networks
    Abc_Obj_t ** Diff = X_subtract_Y_abs(pNtkMiter, X, Y, vPairs->nSize / 2);
    delete[] X;
    delete[] Y;
    // builds the comparator
    int size = Abc_NtkPoNum( pNtk1 );
    Abc_Obj_t ** thresholdNodes = ConstNodes( pNtkMiter, threshold, size );
    Abc_Obj_t * compareRes = X_lt_Y( pNtkMiter, Diff, thresholdNodes,  size );
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
    Abc_NtkForEachNode( pNtk, pNode, i ) 
    {
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
        cout << "---------- the length of OriPIIDs is " << OriPIIDs[0] << endl;
        for (i = 1; i < OriPINum + 1; ++i)
            cout << "------------- the " << i << " th OriPI's ID of pMiter is " << OriPIIDs[i] << endl;
        cout << "---------- the length of MUXPIIDs is " << MUXPIIDs[0] << endl;
        for (i = 1; i < MUXPINum + 1; ++i)
            cout << "------------- the " << i << " th MUXPI's ID of pMiter is " << MUXPIIDs[i] << endl;
    }
}

static bool AllLevelZero( Abc_Ntk_t * pNtk, bool print ) {
    Abc_Obj_t * pObj;
    int i;
    string types[] = {
            "ABC_OBJ_NONE",   //  0:  unknown
            "ABC_OBJ_CONST1",     //  1:  constant 1 node (AIG only)
            "ABC_OBJ_PI",         //  2:  primary input terminal
            "ABC_OBJ_PO",         //  3:  primary output terminal
            "ABC_OBJ_BI",         //  4:  box input terminal
            "ABC_OBJ_BO",         //  5:  box output terminal
            "ABC_OBJ_NET",        //  6:  net
            "ABC_OBJ_NODE",       //  7:  node
            "ABC_OBJ_LATCH",      //  8:  latch
            "ABC_OBJ_WHITEBOX",   //  9:  box with known contents
            "ABC_OBJ_BLACKBOX",   // 10:  box with unknown contents
            "ABC_OBJ_NUMBER"};
    if ( print )
        Abc_NtkForEachObj( pNtk, pObj, i )
            cout << "type: " << types[pObj->Type] << "; Level: " << Abc_ObjLevel( pObj ) << "; name: " << Abc_ObjName( pObj ) << endl;
    Abc_NtkForEachObj( pNtk, pObj, i )
        if ( Abc_ObjLevel( pObj ) != 0 )
            return false;
    return true;
}

static void Miter_Add_Sorting_Network ( Abc_Ntk_t * pMiter, int oriPINum, std::vector<Abc_Obj_t *> &sortedSelectionSignals )
{
    Abc_Obj_t * pObj;
    int i, miterPINum, selectionNum;
    std::vector<Abc_Obj_t *> selectionSignals;
    
    miterPINum = Abc_NtkPiNum( pMiter );
    selectionNum = miterPINum - oriPINum;
    if ( !sortedSelectionSignals.empty() )  sortedSelectionSignals.clear();
    sortedSelectionSignals.resize( selectionNum, nullptr );
    // if no selection signals in the first place, nothing needs done
    if ( selectionNum == 0 )    return;
    assert( selectionNum > 0 );

    // selection signals are PIs with ids larger or equal to <oriPINum>
    Abc_NtkForEachPi( pMiter, pObj, i )
    {
        if ( i < oriPINum )
            continue;
        selectionSignals.push_back( pObj );
    }
    assert( selectionSignals.size() == selectionNum );

    // add the sorting network and connect the outputs.
    N_Input_Sorting_Network( pMiter, selectionSignals, selectionNum, sortedSelectionSignals );
}

static int Sorted_Selection_Find_Least ( Abc_Ntk_t * pMiter, std::vector<Abc_Obj_t *> &sortedSelectionSignals, int OriPINum, int MUXPINum )
{
    int left, right, selectionNum, pointer, count = 0;
    Abc_Obj_t * pNewOutput, * pOriOutput, * pOriOutputFanin;
    Abc_Ntk_t * pMiterDup;
    char * aigFileName = "intermediate-results/Final_Miter_DupAig.aiger";
    char * cnfFileName = "intermediate-results/Miter_Dup_ANDed_CNF_FindLeast_Miter_From_File.cnf";
    std::vector<Abc_Obj_t *> sortedSelectionSignalsDup;
    Cnf_Dat_t * miterCnfData;
    Aig_Man_t * miterAigMan;
    int OriPIIDs[OriPINum+1], MUXPIIDs[MUXPINum+1];
    selectionNum = sortedSelectionSignals.size();   left = 0;   right = selectionNum - 1;
    for ( int i =0; i < sortedSelectionSignals.size(); ++i )
        assert( Abc_ObjRegular( sortedSelectionSignals[i] )->pNtk == pMiter );

    std::cout << "----------- initial condition: left = " << left << "; right = " << right << std::endl;

    while ( left <= right )
    {
        count ++;
        pointer = floor( ( left + right ) / 2.0 );
        std::cout << "----------- " << count << " round: left = " << left << 
            "; right = " << right << "; pointer = " << pointer << std::endl;
        cout << "-------------- duplicating miter" << endl;
        pMiterDup = Abc_NtkDup( pMiter );
        assert( Abc_NtkPoNum( pMiterDup ) == 1 );
        sortedSelectionSignalsDup.clear();
        // assign the selection signals of the duplicate network to a new vector
        cout << "-------------- duplicating sorted selection signals" << endl;
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
        cout << "-------------- updating output" << endl;
        pOriOutput = Abc_NtkPo( pMiterDup, 0 );
        pOriOutputFanin = Abc_ObjFanin0( pOriOutput );
        pNewOutput = Abc_NtkCreatePo( pMiterDup );
        // add fanin to the new output
        Abc_ObjAddFanin( pNewOutput, Abc_AigAnd( ( Abc_Aig_t * ) pMiterDup->pManFunc, pOriOutputFanin, sortedSelectionSignalsDup[pointer] ) );
        assert( Abc_NtkPoNum( pMiterDup ) == 2 );
        // delete the old output
        Abc_NtkDeleteObj( pOriOutput );
        assert( Abc_NtkPoNum( pMiterDup ) == 1 );
        // write the new network into blif file for checking
        cout << "-------------- writing duplicated miter blif" << endl;
        Ckt_WriteBlif( pMiterDup, "intermediate-results/Alexanderia_AND_Added_Find_Least_Temporary_Miter.blif" );  // no problem

        // write into aig format
        cout << "-------------- transforming to aig" << endl << "----------------- ";
        Io_Write(pMiterDup, aigFileName, IO_FILE_AIGER );  // the index of PIs are indexed from 1 to <NumCIs>. Temporarily stored in <pObj->pCopy>. See lines 699 ~ 704 in "src/base/io/ioWriteAiger.c" for more details
        miterAigMan = Ioa_ReadAiger( aigFileName, 1 );
        Aig_ManPrintStats( miterAigMan );
        cout << "-------------- transforming to cnf" << endl;
        miterCnfData = Cnf_DeriveSimple( miterAigMan, 0 );  // the variable index in cnf is derived from iterating all COs, Internal nodes, CIs in the aig manager. For more details, see lines 624 ~ 630 in "src/sat/cnf/cnfWrite.c"
        // get PI's IDs
        cout << "--------------  assigning PIIDs" << endl;
        assignPIIDs( miterCnfData, OriPIIDs, MUXPIIDs, OriPINum, MUXPINum, false );
        // write the cnf into the file
        // char * cnfFileName = "intermediate-results/MiterCNF.cnf";
        cout << "-------------- writing cnf into file" << endl;
        Cnf_DataWriteIntoFile( miterCnfData, (char *) cnfFileName, 1, NULL, NULL );  // since fReadable=1, all the variables can be read by file without any trouble.

        // solve the cnf SAT using qdpll
        QDPLL *depqbf = qdpll_create ();
        qdpll_configure (depqbf, "--dep-man=simple");
        qdpll_configure (depqbf, "--incremental-use");

        cout << "-------------- reading cnf into depqbf" << endl;
        Cnf_DataFile2Depqbf( cnfFileName, depqbf, OriPIIDs, MUXPIIDs );
        // solve the sat and store the results into the file <SATResultFileName>.
        // qdpll_reset( depqbf );
        // QDPLLResult res = QDPLL_SolveSatWriteAssignments( depqbf, SATResultFileName );
        cout << "-------------- solving qsat" << endl;
        QDPLLResult res = qdpll_sat ( depqbf );
        cout << "-------------- updating boundaries" << endl;
        if ( res == QDPLL_RESULT_SAT )
        {
            cout << "-------------- SAT! updating right pointer" << endl;
            right = pointer - 1;
        }
        else
        {
            cout << "-------------- UNSAT! updating left pointer" << endl;
            left = pointer + 1;
        }

        qdpll_delete ( depqbf );
    }
    return left;
}

static void Abc_NtkDupLevel( Abc_Ntk_t * pOriNtk, Abc_Ntk_t * pCopiedNtk )
{
    Abc_Obj_t * pOriObj, * pCopiedObj;
    int i;
    Abc_NtkForEachObj( pOriNtk, pOriObj, i )
    {
        pCopiedObj = Abc_NtkObj( pCopiedNtk, i );
        assert( pOriObj->pCopy == pCopiedObj );
        pCopiedObj->Level = pOriObj->Level;
    }
}