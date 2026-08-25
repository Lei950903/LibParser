#include "cfgFileLoader.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

namespace CfgFileLoader {

namespace {

std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
}

} // namespace

AnalysisConfig loadFromCfg(const std::string& filePath) {
    AnalysisConfig config;
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "[Error] Cannot open config file: " << filePath << std::endl;
        return config;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        line = trim(line);

        size_t eqPos = line.find('=');
        if (eqPos == std::string::npos) continue;

        std::string key = trim(line.substr(0, eqPos));
        std::string value = trim(line.substr(eqPos + 1));

        if (!value.empty() && value.front() == '\"' && value.back() == '\"') {
            value = value.substr(1, value.size() - 2);
        }

        if (key == "cell_name") config.cellName = value;
        else if (key == "input_pin") config.inputPinName = value;
        else if (key == "output_pin") config.outputPinName = value;
        else if (key == "table_type") config.tableType = value;
        else if (key == "transition") config.transitionValue = std::stod(value);
        else if (key == "load") config.loadValue = std::stod(value);
        else if (key == "clock_period") config.clockPeriod = std::stod(value);
        else if (key == "setup_required") config.setupRequiredTime = std::stod(value);
        else if (key == "hold_required") config.holdRequiredTime = std::stod(value);
        else if (key == "extra_segment") {
            std::stringstream ss(value);
            std::string part;
            std::vector<std::string> parts;
            while (std::getline(ss, part, ',')) {
                parts.push_back(trim(part));
            }
            if (parts.size() >= 4) {
                config.extraSegPaths.emplace_back(
                    parts[0], parts[1], parts[2], parts[3]
                );
            }
        }
    }
    return config;
}

} // namespace CfgFileLoader
