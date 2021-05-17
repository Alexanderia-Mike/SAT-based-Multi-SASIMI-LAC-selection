/**CFile****************************************************************

  FileName    [main.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [ABC as a static library.]

  Synopsis    [LEXSAT implementation for approximation verfication.]

  Author      [Weihua Xiao]
  
  Affiliation [SJTU]

  Date        [Ver. 0.0. Started - Dec 17, 2020.]

***********************************************************************/

#include <stdio.h>
#include <time.h>
#include <sat/bsat/satSolver.h>
#include <sat/cnf/cnf.h>
#include <misc/util/abc_global.h>
#include <base/io/ioAbc.h>
#include <iostream>
#include <random>
#include <aig/ioa/ioa.h>
#include <sat/bsat/satStore.h>
#include "LEXSAT.h"
#include "createPLA.h"
#include "Miter_Abs_Diff.h"
////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////
using namespace std;
#if defined(ABC_NAMESPACE)
namespace ABC_NAMESPACE
{
#elif defined(__cplusplus)
extern "C"
{
#endif

// procedures to start and stop the ABC framework
// (should be called before and after the ABC procedures are called)
void   Abc_Start();
void   Abc_Stop();

// procedures to get the ABC framework and execute commands in it
typedef struct Abc_Frame_t_ Abc_Frame_t;

Abc_Frame_t * Abc_FrameGetGlobalFrame();
int    Cmd_CommandExecute( Abc_Frame_t * pAbc, const char * sCommand );

#if defined(ABC_NAMESPACE)
}
using namespace ABC_NAMESPACE;
#elif defined(__cplusplus)
}
#endif

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [The main() procedure.]

  Description [This procedure compiles into a stand-alone program for 
  DAG-aware rewriting of the AIGs. A BLIF or PLA file to be considered
  for rewriting should be given as a command-line argument. Implementation 
  of the rewriting is inspired by the paper: Per Bjesse, Arne Boralv, 
  "DAG-aware circuit compression for formal verification", Proc. ICCAD 2004.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int main( int argc, char * argv[] )
{
    // parameters
    int fUseResyn2  = 0;
    int fPrintStats = 1;
    int fVerify     = 1;
    // variables
    Abc_Frame_t * pAbc;
    char * pFileName;
    char Command[1000];
    clock_t clkGenPLA, clkRead, clkResyn, clkVer, clkLexSAT, clk;
    Sto_Man_t * Clauses;
    char* FilePath; int nInput, nOutput; vector<int> Onset;
    Abc_Ntk_t * pMiter;
    Aig_Man_t * p;  Cnf_Dat_t *cnf; sat_solver* s;
    int pLits[5];   int iLits[5];  int nLits;
    int * sol;  int iCiVarBeg;
    //////////////////////////////////////////////////////////////////////////
    // generate the .PLA
    // nInput=5; nOutput=1;
    // FilePath="verifi.pla";
    // clk=clock();
    // Onset={5,6,7};
    // createPLA(FilePath,nInput,nOutput,Onset);
    // clkGenPLA=clock()-clk;

    //////////////////////////////////////////////////////////////////////////
    // get the input file name
    // if ( argc != 2 )
    // {
    //     printf( "Wrong number of command-line arguments.\n" );
    //     return 1;
    // }
    // pFileName = argv[1];

    //////////////////////////////////////////////////////////////////////////
    // read the .PLA
    Abc_Ntk_t * pNtk = Io_ReadBlif("new.blif", 1);
    Abc_Ntk_t * pNtk0 = Io_ReadBlif( "new-lcf.blif", 1);
    cout<<pNtk->ntkType<<" "<<pNtk->ntkFunc<<endl;
    //convert into AIG network
    pNtk = Abc_NtkToLogic(pNtk);
    pNtk0 = Abc_NtkToLogic(pNtk0);
    pNtk = Abc_NtkStrash( pNtk, 1, 1, 1);
    pNtk0 = Abc_NtkStrash( pNtk0, 1, 1, 1);
    pMiter=CreateMiterXorMulti(pNtk0,pNtk);

    // debug begin
    Abc_Obj_t * debug_pObj; int debug_i;
    cout << "for po of pNtk:" << endl;
    Abc_NtkForEachPo( pNtk, debug_pObj, debug_i )
        cout << Abc_ObjName( debug_pObj ) << " ";
    cout << endl;
    cout << "for po of pNtk0:" << endl;
    Abc_NtkForEachPo( pNtk0, debug_pObj, debug_i )
        cout << Abc_ObjName( debug_pObj ) << " ";
    cout << endl;
    // debug end

    pFileName="verifi.aig";
    char * pBlifFileName="verifi.blif";
    Io_Write(pMiter, pFileName, IO_FILE_AIGER );
    Io_Write(pMiter, pBlifFileName, IO_FILE_BLIF);
    //derive cnf from the AIG network
    p = Ioa_ReadAiger(pFileName,1);
    Aig_ManPrintStats(p);
    //cnf = Cnf_DeriveOther( p, 1 );
    cnf = Cnf_DeriveSimple_xwh(p,0);
    Cnf_DataWriteIntoFile( cnf, "1.cnf", 0, NULL, NULL );
    printf( "CNF stats: Vars = %6d. Clauses = %7d. Literals = %8d. \n", cnf->nVars, cnf->nClauses, cnf->nLiterals );
    
    //Aig_ManForEachCi( p->pManAig, pObj, i )     
    // convert into SAT solver
    s = sat_solver_new();
    s = (sat_solver *)Cnf_DataWriteIntoSolver( cnf, 1, 0 );
    //start lexsat
    clk = clock();
    iCiVarBeg = cnf->nVars - nInput - 1; // obtain the PIs' begin index
    // create min-trivial assignment
    // for (int k = 0; k < nInput; k++ )
    // {
    //     iLits[k] = iCiVarBeg + k;
    //     pLits[k] = Abc_Var2Lit(iCiVarBeg + k, 1);
    //     cout<<iLits[k]<<" "<<pLits[k]<<" "<<(Abc_LitIsCompl(pLits[k])?("neg"):("pos"))<<endl;
    // }
    // nLits = nInput;
    // sat_solver_solve_lexsat_xwh(s, pLits, nLits);
    // clkLexSAT = clock() - clk;
    // //return the final LEX-min solution
    // sol = s->model;
    // cout<<"The on-set is : { ";
    // for(int i = 0; i <Onset.size();i++)
    // {
    //     cout<<Onset[i]<<" ";
    // }
    // cout<<"}"<<endl;
    // cout<< "The final LEX-min solution: ";
    // for(int i=0;i<nInput;i++)
    // {
    //     if(Abc_LitIsCompl(pLits[i]))
    //         cout<<"0 ";
    //     else
    //         cout<<"1 ";
    // }
    // cout<<endl;
    // create max-trivial assignment
    for (int k = 0; k < Abc_NtkPoNum(pMiter); k++ )
    {
        //iLits[k] = cnf->nVars - Abc_NtkPiNum(pMiter) - 1 - Abc_NtkPoNum(pMiter) + k;
        iLits[k] = k;
        pLits[k] = Abc_Var2Lit(iLits[k], 0);
        cout<<iLits[k]<<" "<<pLits[k]<<" "<<(Abc_LitIsCompl(pLits[k])?("neg"):("pos"))<<endl;
    }

    // // debug begin
    // pLits[0] = Abc_Var2Lit( iLits[0], 1 );
    // pLits[1] = Abc_Var2Lit( iLits[1], 0 );
    // // debug end

    sat_solver_solve_maxlexsat_xwh(s,pLits,Abc_NtkPoNum(pMiter));
    cout << "pLits values are " << pLits[0] << " " << pLits[1] << endl;
    cout<< "The final LEX-max solution: ";
    for(int i=0;i<Abc_NtkPoNum(pMiter);i++)
    {
        if(Abc_LitIsCompl(pLits[i]))
            cout<<"0 ";
        else
            cout<<"1 ";
    }
    cout<<endl;
    cout<<"Successfully!!!!"<<endl;
    //////////////////////////////////////////////////////////////////////////
    // start the ABC framework    
    Abc_Start();
    pAbc = Abc_FrameGetGlobalFrame();

    clk = clock();
    
    iLits[0] = 15;
    pLits[0] = Abc_Var2Lit(iLits[0],1);
    default_random_engine e;
    uniform_int_distribution<unsigned> u(1, 20);
    for(int i=1;i<5;i++)
    {
        iLits[i]=u(e);
        pLits[i] = Abc_Var2Lit(iLits[i],1);
    }
    //char polarity[5]={};
    char* pFileName1 = "uf20-02.cnf";
    Cnf_Dat_t * pCnf = Cnf_DataReadFromFile( pFileName1 );
    printf( "CNF stats: Vars = %6d. Clauses = %7d. Literals = %8d. \n", pCnf->nVars, pCnf->nClauses, pCnf->nLiterals );
    // convert into SAT solver
    s = sat_solver_new();
    s = (sat_solver *)Cnf_DataWriteIntoSolver( pCnf, 1, 0 );

    //s->polarity = polarity;
    sat_solver_solve_lexsat(s, pLits, nLits);
    sol = s->model;
    for(int i=0;i<5;i++)
    {
        cout << *(sol+iLits[i]) << " ";
    }
    cout<<endl;
    clkRead = clock() - clk;

    //////////////////////////////////////////////////////////////////////////
    // stop the ABC framework
    Abc_Stop();
    return 0;
}

