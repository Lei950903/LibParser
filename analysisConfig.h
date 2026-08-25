#ifndef ANALYSIS_CONFIG_H
#define ANALYSIS_CONFIG_H

#include <string>
#include <vector>
#include <tuple>

struct AnalysisConfig {
    // ----- 主路径参数（描述"被测路径"）-----
    std::string cellName;
    std::string inputPinName;
    std::string outputPinName;
    std::string tableType;

     // ----- 工作条件参数（描述"输入激励"）-----
    double transitionValue = 0.0;
    double loadValue = 0.0;

    // ----- 时序约束参数（描述"目标要求"）-----
    double clockPeriod = 0.0;
    double setupRequiredTime = 0.0;
    double holdRequiredTime = 0.0;

    // ----- 高级参数（预留工业级扩展）-----
    double launchClockDelay = 0.0;
    double captureClockDelay = 0.0;
    double clockUncertainty = 0.0;

    // ----- 多级路径扩展（用于链式累加）-----
    // 每个元素为 {cellName, fromPin, toPin, tableType}
    std::vector<std::tuple<std::string, std::string, std::string, std::string>> extraSegPaths;
};

#endif