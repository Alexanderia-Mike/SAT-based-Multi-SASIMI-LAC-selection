
// Created by 娄辰飞 on 2021/3/9.
#include "headers.h"
#include "cmdline.h"
#include "sasimi.h"
#include "Sat_Based_Multi_Sel_Manager.h"
#include <cmath>

LACSortMethod_t sortingMethod;
AreaEncodeMode_t areaEncodeMode;
float errorScale;
int truncateSize;
int lacApplyMode;
float normalized_error_threshold;


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
    option.add <string> ("output", 'o', "path to output circuit files", false, "appNtk/");
    option.add <string> ("library", 'l', "standard cell library", false, "data/library/mcnc.genlib");
    option.add <string> ("metricType", 'm', "error metric type, er, nmed, maxed, area", false, "area");
    option.add <float>  ("errorBound", 'e', "error metric upper bound", false, 0.02, range(0.0, 1.0));
    option.add <int>    ("frameNumber", 'f', "simulation round", false, 100000, range(1, INT_MAX));
    option.add <int>    ("maxLevel", 'v', "max TFO cut level", false, INT_MAX, range(1, INT_MAX));
    option.add <string>    ("LACSortMethod", 's', "sorting method for LAC candidates, random, area, areaError", false, "areaError");
    option.add <string>    ("areaEncodeMode", 'a', "sorting network area encoded, noEncode, areaEncode, areaEncodeApprox", false, "areaEncodeApprox");
    option.add <float>    ("errorBoundScale", 'c', "scale the errorbound, a float number", false, 1.0);
    option.add <int>    ("LACTruncateSize", 't', "the number of LACs that would be truncated", false, 20);
    option.add <int>    ("lacApplyMode", 'p', "0 for apply and skip, and 1 for substitute and delete", false, 0);
    option.add <float>    ("normalizedErrorThreshold", 'd', "the normalized maxed error threshold, a float between 0 and 1", false, 0.125);
    option.parse_check(argc, argv);
    return option;
}

void resynthesize( Abc_Frame_t * pAbc );

void run_on_network( const char * file_name, string library, int frameNumber, int maxLevel, Metric_t metricType, float errorBound );

int main(int argc, char * argv[])
{
    // command line parser
    parser option = Cmdline_Parser(argc, argv);
    string output = option.get <string> ("output");
    string library = option.get <string> ("library");
    Metric_t metricType;
    /* METRIC TYPE */
    if ( option.get <string> ("metricType") == "er" )
    {
        metricType = Metric_t::ER;
        cout << "metric type is er" << endl;
    }
    else if ( option.get <string> ("metricType") == "nmed" )
    {
        metricType = Metric_t::NMED;
        cout << "metric type is nmed" << endl;
    }
    else if ( option.get <string> ("metricType") == "maxed" )
    {
        metricType = Metric_t::MaxED;
        cout << "metric type is maxed" << endl;
    }
    else
    {
        metricType = Metric_t::Area;
        cout << "metric type is area" << endl;
    }
    /* LAC SORTING METHOD */
    if ( option.get <string> ("LACSortMethod") == "random" )
        sortingMethod = LACSortMethod_t::RANDOM;
    else if ( option.get <string> ("LACSortMethod") == "area" )
        sortingMethod = LACSortMethod_t::AREA;
    else
        sortingMethod = LACSortMethod_t::AREAERROR;
    /* AREA ENCODE MODE */
    if ( option.get <string> ("areaEncodeMode") == "noEncode" )
        areaEncodeMode = AreaEncodeMode_t::NOENCODE;
    else if ( option.get <string> ("areaEncodeMode") == "areaEncode" )
        areaEncodeMode = AreaEncodeMode_t::AREAENCODE;
    else
        areaEncodeMode = AreaEncodeMode_t::AREAENCODEAPPROX;
    errorScale = option.get <float> ("errorBoundScale");
    truncateSize = option.get <int> ("LACTruncateSize");
    lacApplyMode = option.get <int> ("lacApplyMode");
    normalized_error_threshold = option.get <float> ("normalizedErrorThreshold");
    float errorBound = option.get <float> ("errorBound");
    int frameNumber = option.get <int> ("frameNumber");
    int maxLevel = option.get <int> ("maxLevel");
    // fprintf( stderr, "\testimate LAC error = %s\n", option.get <string> ("metricType") );
    // fprintf( stderr, "\tsort LAC method = %s\n", option.get <string> ("LACSortMethod") );
    // fprintf( stderr, "\tare encode mode = %s\n", option.get <string> ("areaEncodeMode") );

    // run_on_network( "vtr_benchmarks_blif/adder.blif", library, frameNumber, maxLevel, metricType, errorBound ); // too many pos
    // run_on_network( "vtr_benchmarks_blif/multiclock.blif", library, frameNumber, maxLevel, metricType, errorBound );
    run_on_network( "benchmarks/priority_depth_2018.blif", library, frameNumber, maxLevel, metricType, errorBound ); // from here, too slow when first 40 LACs are chosen
    run_on_network( "benchmarks/int2float_depth_2018.blif", library, frameNumber, maxLevel, metricType, errorBound );
    run_on_network( "vtr_benchmarks_blif/3/C17.blif", library, frameNumber, maxLevel, metricType, errorBound );
    run_on_network( "vtr_benchmarks_blif/2/b1.blif", library, frameNumber, maxLevel, metricType, errorBound ); // ok
    run_on_network( "vtr_benchmarks_blif/3/5xp1.blif", library, frameNumber, maxLevel, metricType, errorBound ); // 
    run_on_network( "vtr_benchmarks_blif/3/b1.blif", library, frameNumber, maxLevel, metricType, errorBound );
    run_on_network( "vtr_benchmarks_blif/3/c8.blif", library, frameNumber, maxLevel, metricType, errorBound ); // end here, too slow when first 40 LACs are chosen
    // run_on_network( "vtr_benchmarks_blif/2/b9.blif", library, frameNumber, maxLevel, metricType, errorBound );
    // run_on_network( "vtr_benchmarks_blif/2/bbara.blif", library, frameNumber, maxLevel, metricType, errorBound );
    // run_on_network( "vtr_benchmarks_blif/2/bbtas.blif", library, frameNumber, maxLevel, metricType, errorBound );
    // run_on_network( "vtr_benchmarks_blif/3/C432.blif", library, frameNumber, maxLevel, metricType, errorBound ); // too slow
    // run_on_network( "in/Alexanderia_test_bench_3.blif", library, frameNumber, maxLevel, metricType, errorBound );       // a naive test bench
    // run_on_network( "bench_from_veri/v17.blif", library, frameNumber, maxLevel, metricType, errorBound ); // overlap with c17
    // run_on_network( "bench_from_veri/v432.blif", library, frameNumber, maxLevel, metricType, errorBound );   // too slow
    // run_on_network( "bench_from_veri/v499.blif", library, frameNumber, maxLevel, metricType, errorBound );   // too slow

    // deal with IO
    path outputPath(output);
    create_directory(outputPath);
    path outPrefix(outputPath);

    

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
    fprintf( stderr, "* area:\n" );
    fprintf( stderr, "\tthe initial area = %d\n", (int) area );

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
    fprintf( stderr, "\n===running on the network %s\n", file_name );
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

    fprintf( stderr, "* the network property:\n" );
    fprintf
    ( 
        stderr, "\tpi = %d, po = %d, node = %d, level = %d\n", 
        Abc_NtkPiNum( pNtkLogic ),  Abc_NtkPoNum( pNtkLogic ), Abc_NtkNodeNum( pNtkLogic ), Abc_NtkLevel( pNtkLogic )
    );

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
    // if ( size/3 != 0 )
    //     threshold[0] = 1;
    // else
    //     threshold[ 1 ] = 1;
    // threshold[1] = 1;
    // threshold[0] = 1;

    int threshold_int = (int) floor( normalized_error_threshold * pow( 2, size ) ) + 1;
    // int threshold_int = 0;
    fprintf( stderr, "threshold_int = %d\n", threshold_int );
    for ( int i = 0; i < size; ++i )
    {
        threshold[i] = threshold_int & 01;
        threshold_int >>= 1;
    }

    std::cout << "threshold size = " << size << std::endl;
    // for ( int i = size - 1; i >= 0; --i )
    // {
    //     std::cout << threshold[i];
    //     fprintf( stderr, "%d", threshold[i] );
    // }
    std::cout << std::endl;
    // fprintf( stderr, "\n" );
    
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