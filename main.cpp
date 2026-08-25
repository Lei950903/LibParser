
#include "LibertyParser.h"
#include "pathAnalyzer.h"



int main() {
    std::string fileName = "/home/xulei/Lei_Develop/My_Sta_Lab/day1/simple.lib";
    LibertyParser libParser(fileName);
    libParser.deleteComments();
    std::cout<< "======================================================" << std::endl;
    libParser.parseFileToBlock();

    PathAnalyzer pathAnalyzer(libParser);
    std::string configFile = "config.cfg";
    AnalysisConfig config = CfgFileLoader::loadFromCfg(configFile);
    TotalTimingPath setupPath = pathAnalyzer.analyzeTiming("path1", config, TimingCheckType::SETUP);
    pathAnalyzer.printPathResult(setupPath, TimingCheckType::SETUP);
    TotalTimingPath holdPath = pathAnalyzer.analyzeTiming("path1", config, TimingCheckType::HOLD);
    pathAnalyzer.printPathResult(holdPath, TimingCheckType::HOLD);



    return 0;
}