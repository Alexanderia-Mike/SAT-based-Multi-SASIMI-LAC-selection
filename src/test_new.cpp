
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

static inline double get_area( Abc_Frame_t * pAbc )
{
    return Abc_NtkGetMappedArea( Abc_NtkMap( Abc_NtkStrash( Abc_FrameReadNtk( pAbc ), 0, 0, 0 ), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ) );
}

parser Cmdline_Parser(int argc, char * argv[])
{
    parser option;
    option.add <string> ("input", 'i', "original circuit file", false, "data/su/c880.blif");
    option.add <string> ("output", 'o', "path to output circuit files", false, "appNtk/");
    option.add <string> ("library", 'l', "standard cell library", false, "data/library/mcnc.genlib");
    // option.add <string> ("metricType", '\0', "error metric type, er, nmed", false, "er");
    option.add <string> ("metricType", '\0', "error metric type, er, nmed, maxed, area", false, "area");
    option.add <float>  ("errorBound", 'e', "error metric upper bound", false, 0.02, range(0.0, 1.0));
    // option.add <float>  ("errorBound", 'e', "error metric upper bound", false, 0.26, range(0.0, 1.0));
    option.add <int>    ("frameNumber", 'f', "simulation round", false, 100000, range(1, INT_MAX));
    option.add <int>    ("maxLevel", '\0', "max TFO cut level", false, INT_MAX, range(1, INT_MAX));
    option.parse_check(argc, argv);
    return option;
}

void resynthesize( Abc_Frame_t * pAbc );

void run_on_network( const char * file_name, string library, int frameNumber, int maxLevel, Metric_t metricType, float errorBound );

int main(int argc, char * argv[])
{
    // command line parser
    parser option = Cmdline_Parser(argc, argv);
    string input = option.get <string> ("input");
    string output = option.get <string> ("output");
    string library = option.get <string> ("library");
    Metric_t metricType;
    if ( option.get <string> ("metricType") == "er" )
        metricType = Metric_t::ER;
    else if ( option.get <string> ("metricType") == "nmed" )
        metricType = Metric_t::NMED;
    else if ( option.get <string> ("metricType") == "maxed" )
        metricType = Metric_t::MaxED;
    else
        metricType = Metric_t::Area;
    float errorBound = option.get <float> ("errorBound");
    int frameNumber = option.get <int> ("frameNumber");
    int maxLevel = option.get <int> ("maxLevel");

    run_on_network( "bench_from_veri/v17.blif", library, frameNumber, maxLevel, metricType, errorBound );
    run_on_network( "bench_from_veri/v432.blif", library, frameNumber, maxLevel, metricType, errorBound );
    run_on_network( "bench_from_veri/v499.blif", library, frameNumber, maxLevel, metricType, errorBound );
    run_on_network( "benchmarks/priority_depth_2018.blif", library, frameNumber, maxLevel, metricType, errorBound );
    run_on_network( "benchmarks/int2float_depth_2018.blif", library, frameNumber, maxLevel, metricType, errorBound );
    // run_on_network( "blif/s344.blif", library, frameNumber, maxLevel, metricType, errorBound );
    // run_on_network( "blif/s349.blif", library, frameNumber, maxLevel, metricType, errorBound );

    // deal with IO
    path inputPath(input);
    DASSERT(exists(inputPath));
    path outputPath(output);
    create_directory(outputPath);
    path outPrefix(outputPath);
    outPrefix += inputPath.stem().string();

    

    return 0;
}

void resynthesize( Abc_Frame_t * pAbc )
{
    ostringstream command("");
    double area;
    double previous_area;

    if ( pAbc == nullptr )
    {
        cout << "resynthesize: framework is null!" << endl;
        exit( 0 );
    }
    Abc_Ntk_t * pNtkLogic = Abc_FrameReadNtk(pAbc);
    area = get_area( pAbc );
    previous_area = area + 1;
    cout << "the initial area is " << area << endl;

    /* doing resynthesis until the area is not smaller */
    while ( area < previous_area )
    {
        previous_area = area;
        command << "balance; rewrite; refactor; balance; rewrite; rewrite -z; balance; refactor -z; rewrite -z; balance";
        cout << "abc command " << command.str() << endl;
        DASSERT(!Cmd_CommandExecute(pAbc, command.str().c_str()));
        command.str("");

        area = get_area( pAbc );
        cout << "new area is " << area << endl;
    }
    cout << "the final area is " << area << endl;
}

void run_on_network( const char * file_name, string library, int frameNumber, int maxLevel, Metric_t metricType, float errorBound )
{
    fprintf( stderr, "running on the network %s\n", file_name );
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

    /* read blif using global frame */
    command << "read_blif " << file_name;
    cout << "abc command " << command.str() << endl;
    DASSERT(!Cmd_CommandExecute(pAbc, command.str().c_str()));
    command.str("");
    command << "print_stats";
    cout << "abc command " << command.str() << endl;
    DASSERT(!Cmd_CommandExecute(pAbc, command.str().c_str()));
    command.str("");

    resynthesize( pAbc );


    Abc_Ntk_t * pNtkLogic = Abc_FrameReadNtk(pAbc);
    cout << "the network functionality is " << pNtkLogic->ntkFunc << ", and the network type is " << pNtkLogic->ntkType << endl;

    SASIMI_Manager_t sasimiMng(pNtkLogic, frameNumber, maxLevel, metricType, errorBound);

    int size = Abc_NtkPoNum( pNtkLogic );
    if ( size == 1 )
    {
        cout << "the number of primary outputs must be larger than 1!" << endl;
        return;
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
    fprintf( stderr, "time consumed: %6.2f sec\n", (float)(time) / (float)(CLOCKS_PER_SEC) );
}