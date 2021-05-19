
// Created by 娄辰飞 on 2021/3/9.
#include "headers.h"
#include "cmdline.h"
#include "sasimi.h"
#include "Sat_Based_Multi_Sel_Manager.h"


using namespace std;
using namespace std::filesystem;
using namespace cmdline;

extern "C" {
Abc_Ntk_t * Abc_NtkMap( Abc_Ntk_t * pNtk, double DelayTarget, double AreaMulti, double DelayMulti, float LogFan, float Slew, float Gain, int nGatesMin, int fRecovery, int fSwitching, int fSkipFanout, int fUseProfile, int fUseBuffs, int fVerbose );
}


parser Cmdline_Parser(int argc, char * argv[])
{
    parser option;
    option.add <string> ("input", 'i', "original circuit file", false, "data/su/c880.blif");
    option.add <string> ("output", 'o', "path to output circuit files", false, "appNtk/");
    option.add <string> ("library", 'l', "standard cell library", false, "data/library/mcnc.genlib");
    // option.add <string> ("metricType", '\0', "error metric type, er, nmed", false, "er");
    option.add <string> ("metricType", '\0', "error metric type, er, nmed", false, "nmed");
    option.add <float>  ("errorBound", 'e', "error metric upper bound", false, 0.02, range(0.0, 1.0));
    // option.add <float>  ("errorBound", 'e', "error metric upper bound", false, 0.26, range(0.0, 1.0));
    option.add <int>    ("frameNumber", 'f', "simulation round", false, 100000, range(1, INT_MAX));
    option.add <int>    ("maxLevel", '\0', "max TFO cut level", false, INT_MAX, range(1, INT_MAX));
    option.parse_check(argc, argv);
    return option;
}


int main(int argc, char * argv[])
{
    // command line parser
    parser option = Cmdline_Parser(argc, argv);
    string input = option.get <string> ("input");
    string output = option.get <string> ("output");
    string library = option.get <string> ("library");
    Metric_t metricType = (option.get <string> ("metricType") == "er")? Metric_t::ER: Metric_t::NMED;
    float errorBound = option.get <float> ("errorBound");
    int frameNumber = option.get <int> ("frameNumber");
    int maxLevel = option.get <int> ("maxLevel");

    // deal with IO
    path inputPath(input);
    DASSERT(exists(inputPath));
    path outputPath(output);
    create_directory(outputPath);
    path outPrefix(outputPath);
    outPrefix += inputPath.stem().string();

    clock_t time;
    time = clock();
    // start abc
    Abc_Start();

    Abc_Frame_t * pAbc = Abc_FrameGetGlobalFrame();
    ostringstream command("");
    command << "read " << library;
    cout << "abc command " << command.str() << endl;
    DASSERT(!Cmd_CommandExecute(pAbc, command.str().c_str()));
    command.str("");

    char * fileName1 = "data/arithmetic/adder.blif";    // cannot use this network, because it's too big and the memory usage is out of tolerance.
    char * fileName2 = "data/su/c1908.blif";    // cannot open. Don't know why
    char * fileName3 = "in/Alexanderia_test_input.blif";
    char * fileName4 = "in/2_bit_accurate_multiplier.blif";
    char * fileName5 = "in/Alexanderia_test_bench_2.blif";
    char * fileName6 = "in/Alexanderia_test_bench_3.blif";
    char * fileName7 = "benchmarks/cavlc_depth_2018.blif";
    char * fileName8 = "benchmarks/ctrl_depth_2017.blif";   // abc/src/misc/vec/vecPtr.h:388: void* Vec_PtrEntry(Vec_Ptr_t*, int): Assertion `i >= 0 && i < p->nSize' failed.
    char * fileName9 = "benchmarks/int2float_depth_2018.blif";
    char * fileName10 = "benchmarks/priority_depth_2018.blif";
    char * fileName11 = "benchmarks/router_depth_2018.blif";    // cannot execute, because Abc_NtkDup does some optimization
    Abc_Ntk_t * pNtkNetlist = Io_ReadBlif( fileName10, 1 );
    cout << "the benchmark: " << Abc_NtkName( pNtkNetlist ) << endl << 
        "number of PIs: " << Abc_NtkPiNum( pNtkNetlist ) << endl <<
        "number of POs: " << Abc_NtkPoNum( pNtkNetlist ) << endl <<
        "number of nodes:" << Abc_NtkNodeNum( pNtkNetlist ) << endl <<
        "number of levels: " << Abc_NtkLevel( pNtkNetlist ) << endl;

    // sasimi + vecbee
//    Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);
    Abc_Ntk_t * pNtkLogic = Abc_NtkToLogic( pNtkNetlist );
    SASIMI_Manager_t sasimiMng(pNtkLogic, frameNumber, maxLevel, metricType, errorBound);
//    sasimiMng.GreedySelection(pNtk, outPrefix.string());

    // // debug begin
    // cout << "GetDArea: network type is " << pNtkLogic->ntkType << ", and network function is " << pNtkLogic->ntkFunc << endl;
    // //debug end


    int size = Abc_NtkPoNum( pNtkLogic );
    if ( size == 1 )
    {
        cout << "the number of primary outputs must be larger than 1!" << endl;
        return 0;
    }
    int threshold[size];
    // assign the threshold
    for ( int i = 0; i < size; ++i )
        threshold[i] = 0;
    if ( size/3 != 0 )
        threshold[size/3] = 1;
    else
        threshold[ 1 ] = 1;
    // threshold[1] = 1;
    // threshold[0] = 1;
    
    Abc_Ntk_t * pNtkStrash = Abc_NtkStrash( pNtkLogic, 0, 0, 0 );
    Abc_Ntk_t * pNtkMappedLogic = Abc_NtkMap( pNtkStrash, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 );

    // double dArea = 0; Abc_Obj_t * pObj; int i;
    // Abc_NtkForEachObj( pNtkMappedLogic, pObj, i )
    //    dArea += Mio_GateReadArea( (Mio_Gate_t *)pObj->pData );

    cout << "the total area: " << Abc_NtkGetMappedArea( pNtkMappedLogic ) << endl;

    // debug begin
    cout << "the network functionality is " << pNtkMappedLogic->ntkFunc << ", and the network type is " << pNtkMappedLogic->ntkType << endl;
    // debug end

    // sasimiMng.SATBasedMultiSelection( pNtkLogic, outPrefix.string(), threshold );
    // sasimiMng.SATBasedMultiSelection( pNtkMappedLogic, outPrefix.string(), threshold );
    SBMSM_t sbms_manager( pNtkMappedLogic, threshold, sasimiMng );
    sbms_manager.SAT_Based_Multi_Selection();

    // stop abc
    Abc_Stop();
    time = clock() - time;
    printf( "time consumed: %6.2f sec\n", (float)(time) / (float)(CLOCKS_PER_SEC) );

    return 0;
}
