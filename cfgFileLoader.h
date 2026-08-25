#ifndef CFG_FILE_LOADER_H
#define CFG_FILE_LOADER_H

#include "analysisConfig.h"

namespace CfgFileLoader
{
    AnalysisConfig loadFromCfg(const std::string& fileName);
} // namespace CfgFileLoader

#endif
