#include "headers.h"
#include "cmdline.h"
#include "sasimi.h"

#include "Typological_Structure_Encoded_Sat_Based_Multi_Sel_Manager.h"

using namespace std;
using namespace std::filesystem;

extern "C" {
Abc_Ntk_t * Abc_NtkMap( Abc_Ntk_t * pNtk, double DelayTarget, double AreaMulti, double DelayMulti, float LogFan, float Slew, float Gain, int nGatesMin, int fRecovery, int fSwitching, int fSkipFanout, int fUseProfile, int fUseBuffs, int fVerbose );
}

int main()
{
    Abc_Start();

    Abc_Ntk_t * pNtkNetlist = Io_ReadBlif( "in/Alexanderia_test_bench_3.blif", 1 );
    Abc_Ntk_t * pNtkLogic = Abc_NtkToLogic( pNtkNetlist );
    SASIMI_Manager_t sasimiMng( pNtkLogic, 100000, INT_MAX, Metric_t::ER, 0.02 );
    Abc_Ntk_t * pNtkStrash = Abc_NtkStrash( pNtkLogic, 0, 0, 0 );
    Abc_Ntk_t * pNtkMappedLogic = Abc_NtkMap( pNtkStrash, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 );

    int size = Abc_NtkPoNum( pNtkLogic );
    if ( size == 1 )
    {
        cout << "the number of primary outputs must be larger than 1!" << endl;
        return 0;
    }
    int threshold[size] = { 0 };

    SBMSM_t sbms_manager( pNtkMappedLogic, threshold, sasimiMng );
    sbms_manager.get_transitive_fanin_cone_using_matrix();
    sbms_manager.print_transitive_fanin_cone_matrix();

    Abc_Stop();

    return 0;
}