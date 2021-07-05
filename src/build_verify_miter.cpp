#include "Sat_Based_Multi_Sel_Manager.h"

void 
SBMSM_t::build_verify_miter()
{
    Abc_NtkDelete( pMiter );
    create_naive_miter_from_networks( pOriNtk, pAppNtkDup );
    Ckt_WriteBlif( pMiter, "intermediate-results/Verify_Miter_BLIF.blif" );
    transform_verify_miter_to_cnf();
}

void
SBMSM_t::solve_verify_miter_cnf()
{
    std::cout << "# Solving Quantified SAT Problem! ------------------------------ " << std::endl;
    QDPLL *depqbf = qdpll_create ();
    qdpll_configure (depqbf, "--dep-man=simple");
    qdpll_configure (depqbf, "--incremental-use");

    std::cout << "------- Loading CNF to Depqbf!" << std::endl;
    cnf_file_to_depqbf_for_verify_miter( cnfFileNameVerifyMiter, depqbf, OriPIIDs );
    std::cout << "------- Solving the Result!" << std::endl;
    QDPLLResult res = QDPLL_SolveSatWriteAssignments( depqbf, "intermediate-results/sat_problem_verify_out.qdimacs", "intermediate-results/sat_problem_verify_in.qdimacs" );
    if ( res == QDPLL_RESULT_SAT )
    {
        fprintf( stderr, "verify succeeds\n" );
    }
    else if ( res == QDPLL_RESULT_UNSAT )
    {
        fprintf( stderr, "verify fails\n" );
    }
    else
    {
        fprintf( stderr, "cannot verify\n" );
    }

    std::cout << "------- Clearing Memory!" << std::endl;
    /* Delete solver instance */
    qdpll_delete ( depqbf );
}

void
SBMSM_t::create_naive_miter_from_networks( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2 )
{
    std::cout << "------- Strashing Networks!" << std::endl;
    Abc_Ntk_t * pNtk1Strashed = Abc_NtkStrash( pNtk1, 0, 0, 0 );
    Abc_Ntk_t * pNtk2Strashed = Abc_NtkStrash( pNtk2, 0, 0, 0 );
    assert( Abc_NtkPiNum( pNtk1Strashed ) == Abc_NtkPiNum( pNtk2Strashed ) );

    // create the miter
    std::cout << "------- Creating Verify Miter!" << std::endl;
    create_subtractor_comparator_miter_from_networks( pNtk1Strashed, pNtk2Strashed );
    assert( Abc_NtkPiNum( pMiter ) == Abc_NtkPiNum( pNtk1Strashed ) );
    std::cout << "------- Writing Verify Miter Blif!" << std::endl;
    Ckt_WriteBlif( pMiter, verifyMiterName );

    std::cout << "-------------- Verify Miter is created successfully!" << std::endl;
    OriPINum = Abc_NtkPiNum( pOriNtk );
}

void
SBMSM_t::create_subtractor_comparator_miter_from_networks( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2 )
{
    char Buffer[1000];

    assert( Abc_NtkIsStrash(pNtk1) );
    assert( Abc_NtkIsStrash(pNtk2) );

    // start the new network
    pMiter = Abc_NtkAlloc( ABC_NTK_STRASH, ABC_FUNC_AIG, 1 );
    sprintf( Buffer, "%s_%s_miter", pNtk1->pName, pNtk2->pName );
    pMiter->pName = Extra_UtilStrsav(Buffer);

    /************************************* Abc_NtkMiterPrepare *************************************/
    Abc_Obj_t * pObj, * pObjNew;
    int i;
    Abc_AigConst1(pNtk1)->pCopy = Abc_AigConst1(pMiter);
    Abc_AigConst1(pNtk2)->pCopy = Abc_AigConst1(pMiter);
    // create new PIs and remember them in the old PIs
    Abc_NtkForEachPi( pNtk1, pObj, i )
    {
        pObjNew = Abc_NtkCreatePi( pMiter );
        // remember this PI in the old PIs
        pObj->pCopy = pObjNew;
        pObj = Abc_NtkCi(pNtk2, i);
        pObj->pCopy = pObjNew;
        // add name
        Abc_ObjAssignName( pObjNew, Abc_ObjName(pObj), NULL );
    }
    // create the latches
    Abc_NtkForEachLatch( pNtk1, pObj, i )
    {
        pObjNew = Abc_NtkDupBox( pMiter, pObj, 0 );
        // add names
        Abc_ObjAssignName( pObjNew, Abc_ObjName(pObj), "_1" );
        Abc_ObjAssignName( Abc_ObjFanin0(pObjNew),  Abc_ObjName(Abc_ObjFanin0(pObj)), "_1" );
        Abc_ObjAssignName( Abc_ObjFanout0(pObjNew), Abc_ObjName(Abc_ObjFanout0(pObj)), "_1" );
    }
    Abc_NtkForEachLatch( pNtk2, pObj, i )
    {
        pObjNew = Abc_NtkDupBox( pMiter, pObj, 0 );
        // add name
        Abc_ObjAssignName( pObjNew, Abc_ObjName(pObj), "_2" );
        Abc_ObjAssignName( Abc_ObjFanin0(pObjNew),  Abc_ObjName(Abc_ObjFanin0(pObj)), "_2" );
        Abc_ObjAssignName( Abc_ObjFanout0(pObjNew), Abc_ObjName(Abc_ObjFanout0(pObj)), "_2" );
    }
    
    /************************************* Abc_NtkMiterAddOne *************************************/
    Abc_Obj_t * pNode;
    assert( Abc_NtkIsDfsOrdered(pNtk1) );
    Abc_AigForEachAnd( pNtk1, pNode, i )
        pNode->pCopy = Abc_AigAnd( (Abc_Aig_t *)pMiter->pManFunc, Abc_ObjChild0Copy(pNode), Abc_ObjChild1Copy(pNode) );
    assert( Abc_NtkIsDfsOrdered(pNtk2) );
    Abc_AigForEachAnd( pNtk2, pNode, i )
        pNode->pCopy = Abc_AigAnd( (Abc_Aig_t *)pMiter->pManFunc, Abc_ObjChild0Copy(pNode), Abc_ObjChild1Copy(pNode) );

    /************************************* Abc_NtkMiterFinalize *************************************/
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
    Abc_Obj_t ** Diff = X_subtract_Y_absolute(pMiter, X, Y, vPairs->nSize / 2);
    delete[] X;
    delete[] Y;
    // builds the comparator
    int size = Abc_NtkPoNum( pNtk1 );
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
    assert( Abc_NtkPoNum( pMiter ) == 1 );
}

void 
SBMSM_t::transform_verify_miter_to_cnf()
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
    std::cout << "------- Assigning PIIDs for Verify Miter!" << std::endl;
    OriPIIDs = (int *) realloc( OriPIIDs, ( OriPINum + 1 ) * sizeof( int ) );
    assign_PIIDs_for_verify_miter( miterCnfData, false );

    // write the cnf into the file
    std::cout << "------- Writing Miter CNF!" << std::endl;
    Cnf_DataWriteIntoFile( miterCnfData, (char *) cnfFileNameVerifyMiter, 1, NULL, NULL );  // since fReadable=1, all the variables can be read by file without any trouble.
}

void 
SBMSM_t::assign_PIIDs_for_verify_miter( Cnf_Dat_t * miterCnfData, bool print )
{
    int i;
    for ( i = 0; i < OriPINum + 1; ++i )
        OriPIIDs[i] = -1;
    OriPIIDs[0] = OriPINum;
    for ( i = 0; i < OriPINum; ++i )
        OriPIIDs[i + 1] = miterCnfData->nVars - OriPINum - 1 + i;
    // check the IDs
    if ( print )
    {
        std::cout << "---------- the length of OriPIIDs is " << OriPIIDs[0] << std::endl;
        for (i = 1; i < OriPINum + 1; ++i)
            std::cout << "------------- the " << i << " th OriPI's ID of pMiter is " << OriPIIDs[i] << std::endl;
    }
}