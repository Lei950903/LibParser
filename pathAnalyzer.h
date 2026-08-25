#ifndef PATH_ANALYZER_H
#define PATH_ANALYZER_H

#include "LibertyParser.h"
#include "cfgFileLoader.h"


class PathAnalyzer {
public:
    explicit PathAnalyzer(LibertyParser& libParser);
    ~PathAnalyzer() = default;

    double accumulateSegAndTotalPathDelay(std::vector<PathSegment>& segments);
    TotalTimingPath analyzeTiming(const std::string& currentPathName, const AnalysisConfig& config, TimingCheckType checkType);

    // 解析文件到PathSegment结构体，和使用结构体中数据来计算
    std::vector<PathSegment> loadConfigFromFileToSegments(const AnalysisConfig& config);

    void printPathResult(const TotalTimingPath& path,TimingCheckType checkType);

private:
    LibertyParser& m_libParser;
    const std::unordered_map<std::string, Cell>& m_cells;
};

#endif