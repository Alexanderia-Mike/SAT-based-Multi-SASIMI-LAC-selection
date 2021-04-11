
// Created by 娄辰飞 on 2021/3/9.
#include "headers.h"
#include "cmdline.h"
#include "sasimi.h"


using namespace std;
using namespace std::filesystem;
using namespace cmdline;


parser Cmdline_Parser(int argc, char * argv[])
{
    parser option;
    option.add <string> ("input", 'i', "original circuit file", false, "data/su/c880.blif");
    option.add <string> ("output", 'o', "path to output circuit files", false, "appNtk/");
    option.add <string> ("library", 'l', "standard cell library", false, "data/library/mcnc.genlib");
    option.add <string> ("metricType", '\0', "error metric type, er, nmed", false, "er");
    option.add <float>  ("errorBound", 'e', "error metric upper bound", false, 0.02, range(0.0, 1.0));
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

    // start abc
    Abc_Start();

    char * fileName1 = "data/arithmetic/adder.blif";    // cannot use this network, because it's too big and the memory usage is out of tolerance.
    char * fileName2 = "data/su/c1908.blif";    // cannot open. Don't know why
    char * fileName3 = "in/Alexanderia_test_input.blif";
    char * fileName4 = "in/2_bit_accurate_multiplier.blif";
    char * fileName5 = "in/Alexanderia_test_bench_2.blif";
    char * fileName6 = "in/Alexanderia_test_bench_3.blif";
    Abc_Ntk_t * pNtkNetlist = Io_ReadBlif( fileName6, 1 );

    // sasimi + vecbee
//    Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);
    Abc_Ntk_t * pNtkLogic = Abc_NtkToLogic( pNtkNetlist );
    SASIMI_Manager_t sasimiMng(pNtkLogic, frameNumber, maxLevel, metricType, errorBound);
//    sasimiMng.GreedySelection(pNtk, outPrefix.string());

    int size = Abc_NtkPoNum( pNtkLogic );
    int threshold[size];
    // assign the threshold
    for ( int i = 0; i < size; ++i )
        threshold[i] = 0;
    threshold[size/3] = 1;

    sasimiMng.SATBasedMultiSelection( pNtkLogic, outPrefix.string(), threshold );

    // stop abc
    Abc_Stop();

    return 0;
}
