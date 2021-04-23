// // created by Alexanderia on April 26
// // test passed!
// #include "headers.h"
// #include "sortingNetwork.h"
// #include <vector>
// #include "cktUtil.h"
// #include <iostream>
// #include "qdpll.h"
// #include "cnf2Depqbf.h"


// using namespace std;
// using namespace std::filesystem;

// // adding the sorting network to all the selection signals in the miter
// // return the vector of the sorted selection signals
// static void Miter_Add_Sorting_Network( Abc_Ntk_t * pMiter, int oriPINum, std::vector<Abc_Obj_t *> &sortedSelectionSignals );
// void assignPIIDs( Cnf_Dat_t * miterCnfData, int * OriPIIDs, int * MUXPIIDs, int OriPINum, int MUXPINum, bool print );

// int main(int argc, char * argv[])
// {
//     std::vector<Abc_Obj_t *> sortedSelectionSignals;
//     char * fileName;
//     Abc_Ntk_t * pNtkNetlist, * pNtkLogic, * pNtkStrash;
//     Abc_Obj_t * pObj, * pPI, *pPO;
//     int i, j;

//     // start abc
//     Abc_Start();

//     // read the 2 bit accurate multiplier
//     fileName = "intermediate-results/Alexanderia_Miter.blif";
//     pNtkNetlist = Io_ReadBlif( fileName, 1 );

//     // convert to logic, and strash
//     pNtkLogic = Abc_NtkToLogic( pNtkNetlist );
//     pNtkStrash = Abc_NtkStrash( pNtkLogic, 0, 0, 0 );
//     // Ckt_WriteBlif( pNtkStrash, "intermediate-results/test_sorting_original.blif" );
//     assert( Abc_NtkPiNum( pNtkLogic ) == Abc_NtkPiNum( pNtkStrash ) );
//     assert( Abc_NtkPiNum( pNtkLogic ) == 4 );

//     // output the fanouts of the original strashed network
//     cout << "before the sorting network is added! the number of objects are " << Abc_NtkObjNum( pNtkStrash ) << endl;
//     Abc_NtkForEachPi( pNtkStrash, pPI, i )
//     {
//         cout << "--- the " << i << " th PI's name is: " << Abc_ObjName( pPI ) << endl;
//         Abc_ObjForEachFanout( pPI, pObj, j )
//             cout << "--- --- the " << j << " th fanout of " << Abc_ObjName( pPI ) << " is " << Abc_ObjName( pObj ) << endl;
//     }

//     /*
//     // add the inputs to a vector
//     Abc_NtkForEachPi( pNtkStrash, pPI, i )
//         if ( i >= 2 )
//             selectionSignals.push_back( pPI );
//     assert( selectionSignals.size() == 2 );
//     */

//     // add the sorting network to the output of the network
//     Miter_Add_Sorting_Network( pNtkStrash, 2, sortedSelectionSignals );

//     // output the fanouts of the strashed network after the sorting network is added
//     cout << endl << "after the sorting network is added! the number of objects are " << Abc_NtkObjNum( pNtkStrash ) << endl;
//     Abc_NtkForEachPi( pNtkStrash, pPI, i )
//     {
//         cout << "--- the " << i << " th PI's name is: " << Abc_ObjName( pPI ) << endl;
//         Abc_ObjForEachFanout( pPI, pObj, j )
//             cout << "--- --- the " << j << " th fanout of " << Abc_ObjName( pPI ) << " is " << Abc_ObjName( pObj ) << endl;
//     }

//     /*
//     for ( i = 0; i < selectionSignals.size(); ++i )
//     {
//         pPO = Abc_NtkCreatePo( pNtkStrash );
//         Abc_ObjAddFanin( pPO, selectionSignals[i] );
//     }
//     Ckt_WriteBlif( pNtkStrash, "intermediate-results/test_sorting_sorting_network_added.blif" );
//     */
   
//     /************************************************************************************************/
//     int left, right, selectionNum, pointer, OriPINum, MUXPINum;
//     Abc_Obj_t * pNewOutput, * pOriOutput, * pOriOutputFanin;
//     Abc_Ntk_t * pMiterDup;
//     char * aigFileName = "intermediate-results/MiterDupANDedAig.aiger";
//     char * cnfFileName = "intermediate-results/Miter_Dup_ANDed_CNF_Sorting_Test.cnf";
//     std::vector<Abc_Obj_t *> sortedSelectionSignalsDup;
//     Cnf_Dat_t * miterCnfData;
//     Aig_Man_t * miterAigMan;
//     OriPINum = 2; MUXPINum = 2;
//     int OriPIIDs[OriPINum+1], MUXPIIDs[MUXPINum+1];

//     pointer = 0;
//     pMiterDup = Abc_NtkDup( pNtkStrash );
//     assert( Abc_NtkPoNum( pMiterDup ) == 1 );
//     sortedSelectionSignalsDup.clear();
//     // assign the selection signals of the duplicate network to a new vector
//     for ( int i = 0; i < sortedSelectionSignals.size(); ++i )
//     {
//         sortedSelectionSignalsDup.push_back( 
//             Abc_ObjIsComplement( sortedSelectionSignals[i] ) ?
//             Abc_ObjNot( Abc_ObjRegular( sortedSelectionSignals[i] )->pCopy ) :
//             sortedSelectionSignals[i]->pCopy
//         );
//         assert( Abc_ObjRegular( sortedSelectionSignalsDup[i] )->pNtk == pMiterDup );
//     }
    
//     // get the old output and create the new output
//     pOriOutput = Abc_NtkPo( pMiterDup, 0 );
//     pOriOutputFanin = Abc_ObjFanin0( pOriOutput );
//     pNewOutput = Abc_NtkCreatePo( pMiterDup );
//     // add fanin to the new output
//     Abc_ObjAddFanin( pNewOutput, Abc_AigAnd( ( Abc_Aig_t * ) pMiterDup->pManFunc, pOriOutputFanin, sortedSelectionSignalsDup[pointer] ) );
//     assert( Abc_NtkPoNum( pMiterDup ) == 2 );
//     // delete the old output
//     Abc_NtkDeleteObj( pOriOutput );
//     assert( Abc_NtkPoNum( pMiterDup ) == 1 );
//     // write the new network into blif file for checking
//     Ckt_WriteBlif( pMiterDup, "intermediate-results/Alexanderia_AND_Added_Sorting_Network.blif" );  // no problem

//     // write into aig format
//     Io_Write( pMiterDup, aigFileName, IO_FILE_AIGER );  // the index of PIs are indexed from 1 to <NumCIs>. Temporarily stored in <pObj->pCopy>. See lines 699 ~ 704 in "src/base/io/ioWriteAiger.c" for more details
//     miterAigMan = Ioa_ReadAiger( aigFileName, 1 );
//     Aig_ManPrintStats( miterAigMan );
//     miterCnfData = Cnf_DeriveSimple( miterAigMan, 0 );  // the variable index in cnf is derived from iterating all COs, Internal nodes, CIs in the aig manager. For more details, see lines 624 ~ 630 in "src/sat/cnf/cnfWrite.c"
//     // get PI's IDs
//     assignPIIDs( miterCnfData, OriPIIDs, MUXPIIDs, OriPINum, MUXPINum, true );
//     // write the cnf into the file
//     // char * cnfFileName = "intermediate-results/MiterCNF.cnf";
//     Cnf_DataWriteIntoFile( miterCnfData, (char *) cnfFileName, 1, NULL, NULL );  // since fReadable=1, all the variables can be read by file without any trouble.

//     // solve the cnf SAT using qdpll
//     QDPLL *depqbf = qdpll_create ();
//     qdpll_configure (depqbf, "--dep-man=simple");
//     qdpll_configure (depqbf, "--incremental-use");

//     Cnf_DataFile2Depqbf( cnfFileName, depqbf, OriPIIDs, MUXPIIDs );
//     // solve the sat and store the results into the file <SATResultFileName>.
//     // qdpll_reset( depqbf );
//     // QDPLLResult res = QDPLL_SolveSatWriteAssignments( depqbf, SATResultFileName );

//     QDPLLResult res = qdpll_sat ( depqbf );
//     // print the result
//     switch ( res )
//     {
//     case QDPLL_RESULT_SAT:
//         cout << "SAT!" << endl;
//         break;
//     case QDPLL_RESULT_UNSAT:
//         cout << "UNSAT!" << endl;
//         break;
//     default:
//         cout << "UNKNOWN!" << endl;
//         break;
//     }
//     qdpll_print_qdimacs_output ( depqbf );
//     qdpll_reset( depqbf );
//     FILE * pFile = fopen( "intermediate-results/Miter_Dup_AND_Added.cnf", "w+" );
//     qdpll_print( depqbf, pFile );
//     fclose( pFile );
//     qdpll_delete ( depqbf );
//     /************************************************************************************************/

    

//     // stop abc
//     Abc_Stop();

//     return 0;
// }

// static void Miter_Add_Sorting_Network ( Abc_Ntk_t * pMiter, int oriPINum, std::vector<Abc_Obj_t *> &sortedSelectionSignals )
// {
//     Abc_Obj_t * pObj;
//     int i, miterPINum, selectionNum;
//     std::vector<Abc_Obj_t *> selectionSignals;
    
//     miterPINum = Abc_NtkPiNum( pMiter );
//     selectionNum = miterPINum - oriPINum;
//     if ( !sortedSelectionSignals.empty() )  sortedSelectionSignals.clear();
//     sortedSelectionSignals.resize( selectionNum, nullptr );
//     // if no selection signals in the first place, nothing needs done
//     if ( selectionNum == 0 )    return;
//     assert( selectionNum > 0 );

//     // selection signals are PIs with ids larger or equal to <oriPINum>
//     Abc_NtkForEachPi( pMiter, pObj, i )
//     {
//         if ( i < oriPINum )
//             continue;
//         selectionSignals.push_back( pObj );
//     }
//     assert( selectionSignals.size() == selectionNum );

//     // add the sorting network and connect the outputs.
//     N_Input_Sorting_Network( pMiter, selectionSignals, selectionNum, sortedSelectionSignals );
// }