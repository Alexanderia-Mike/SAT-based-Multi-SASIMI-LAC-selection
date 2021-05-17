// miter whose outputs are the absolute difference of two inputs
#include "Miter_Abs_Diff.h"

extern Abc_Obj_t ** X_subtract_Y_abs(Abc_Ntk_t * pNtk, Abc_Obj_t * X[], Abc_Obj_t * Y[], int n) {
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


extern Abc_Ntk_t * CreateMiterXorMulti ( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2) {
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
    // add the differences to primary outputs
    delete[] X;
    delete[] Y;
    for ( i = 0; i < vPairs->nSize / 2; ++i )
    {
        pNode = Abc_NtkCreatePo( pNtkMiter );
        Abc_ObjAddFanin( pNode, Diff[i] );
    }
    // builds the comparator
    // int size = Abc_NtkPoNum( pNtk1 );
    // Abc_Obj_t ** thresholdNodes = ConstNodes( pNtkMiter, threshold, size );
    // Abc_Obj_t * compareRes = X_lt_Y( pNtkMiter, Diff, thresholdNodes,  size );
    // create the final output
    // Abc_Obj_t * result = Abc_NtkCreatePo( pNtkMiter );
    // Abc_ObjAddFanin( result, compareRes );
    //    // collect the CO nodes for the miter
    //    Abc_NtkForEachCo( pNtk1, pNode, i ) {
    //        pMiter = Abc_AigXor( (Abc_Aig_t *)pNtkMiter->pManFunc, Abc_ObjChild0Copy(pNode), Abc_ObjChild0Copy(Abc_NtkCo(pNtk2, i)) );
    //        Abc_ObjAddFanin( Abc_NtkPo(pNtkMiter,i), pMiter );
    //    }

    /************************************* rest of Abc_NtkMiterInt *************************************/

    delete [] Diff;
    Vec_PtrFree( vPairs );
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