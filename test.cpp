完整文件列表
text

lib_parser_new/
├── AnalysisConfig.h          (纯数据容器)
├── ConfigLoader.h            (配置加载器)
├── ConfigLoader.cpp          
├── LibertyDataStructures.h   (数据结构)
├── LibertyParser.h           (已加 explicit)
├── LibertyParser.cpp         
├── PathAnalyzer.h            (已加 explicit)
├── PathAnalyzer.cpp          
├── main.cpp                  
├── config.cfg                
└── simple.lib                

1️⃣ AnalysisConfig.h
cpp

#ifndef ANALYSIS_CONFIG_H
#define ANALYSIS_CONFIG_H

#include <string>
#include <vector>
#include <tuple>

struct AnalysisConfig {
    // ----- 主路径参数 -----
    std::string cellName;
    std::string inputPin;
    std::string outputPin;
    std::string tableType;          // "cell_rise" 或 "cell_fall"
    
    // ----- 工作条件 -----
    double transition = 0.0;        // 输入转换时间 (ps)
    double load = 0.0;              // 输出负载电容 (fF)

    // ----- 时序约束 -----
    double clockPeriod = 0.0;       // 时钟周期 (ps)
    double setupRequire = 0.0;      // 建立时间约束 (ps)

    // ----- 多级路径扩展 -----
    // 每个元素为 {cellName, fromPin, toPin, tableType}
    std::vector<std::tuple<std::string, std::string, std::string, std::string>> extraSegments;
};

#endif

2️⃣ ConfigLoader.h
cpp

#ifndef CONFIG_LOADER_H
#define CONFIG_LOADER_H

#include "AnalysisConfig.h"
#include <string>

namespace ConfigLoader {
    AnalysisConfig loadFromFile(const std::string& filePath);
}

#endif

3️⃣ ConfigLoader.cpp
cpp

#include "ConfigLoader.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

namespace ConfigLoader {

static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
}

AnalysisConfig loadFromFile(const std::string& filePath) {
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

        if (value.front() == '\"' && value.back() == '\"') {
            value = value.substr(1, value.size() - 2);
        }

        if (key == "cell_name") config.cellName = value;
        else if (key == "input_pin") config.inputPin = value;
        else if (key == "output_pin") config.outputPin = value;
        else if (key == "table_type") config.tableType = value;
        else if (key == "transition") config.transition = std::stod(value);
        else if (key == "load") config.load = std::stod(value);
        else if (key == "clock_period") config.clockPeriod = std::stod(value);
        else if (key == "setup_require") config.setupRequire = std::stod(value);
        else if (key == "extra_segment") {
            std::stringstream ss(value);
            std::string part;
            std::vector<std::string> parts;
            while (std::getline(ss, part, ',')) {
                parts.push_back(trim(part));
            }
            if (parts.size() >= 4) {
                config.extraSegments.emplace_back(
                    parts[0], parts[1], parts[2], parts[3]
                );
            }
        }
    }
    return config;
}

} // namespace ConfigLoader

4️⃣ LibertyDataStructures.h
cpp

#ifndef LIBERTY_DATA_STRUCTURES_H
#define LIBERTY_DATA_STRUCTURES_H

#include <vector> 
#include <string>

// ----- 原有的数据结构 -----
struct LookupTable {
    std::vector<double> index1;
    std::vector<double> index2;
    std::vector<std::vector<double>> values;
};

struct TimingArc {
    std::string related_pin;
    std::string timing_sense;
    std::string timing_type;
    LookupTable cell_rise;
    LookupTable rise_transition;
    LookupTable cell_fall;
    LookupTable fall_transition;
};

struct Pin {
    std::string name;
    std::string direction;
    double capacitance = 0.0;
    bool is_clock = false;
    std::string function;
    std::vector<TimingArc> timing_arcs;
};

struct Cell {
    std::string name;
    std::vector<Pin> pins;
    double setup = 0.0;
    double hold = 0.0;
};

struct Template {
    std::string variable_1;
    std::string variable_2;
    std::vector<double> index1;
    std::vector<double> index2;
};

// ----- 新增：路径分析数据结构 -----
struct PathSegment {
    std::string cellName;
    std::string fromPin;
    std::string toPin;
    std::string tableType;
    double transition = 0.0;
    double load = 0.0;
    double delay = 0.0;          // 计算后回填
};

struct TimingPath {
    std::string pathName;
    std::vector<PathSegment> segments;
    double totalDelay = 0.0;
    double clockPeriod = 0.0;
    double setupRequire = 0.0;
    double slack = 0.0;
    bool isSetupMet = false;
};

#endif

5️⃣ LibertyParser.h
cpp

#ifndef LIBERTY_PARSER_H
#define LIBERTY_PARSER_H

#include <unordered_map>
#include <fstream> 
#include <iostream>
#include <sstream>
#include "LibertyDataStructures.h"

enum class BlockName {
    LIBRARY = 1,
    TEMPLATE,
    CELL,
    PIN,
    TIMING,
    FF,
    TABLE
};

enum class TableType {
    cell_rise = 1,
    cell_fall,
    rise_transition,
    fall_transition
};

class LibertyParser {
public:
    explicit LibertyParser(const std::string fileName);   // 已加 explicit
    ~LibertyParser();

    void deleteComments();
    void parseFileToBlock();
    
    std::string extractBlockName(const std::string& currentLine);
    std::string getAttrValue(const std::string& line);
    std::string trimString(const std::string& str);
    std::pair<std::string, std::string> splitStringToPair(const std::string& originString, const std::string& splitBaseStr);
    std::vector<std::vector<double>> parseValuesToMatrix(const std::string& valuesString);
    double interpolateDelay(const LookupTable& table, double x, double y);
    const char* blockNameToString(BlockName name);

    // ----- 关键：返回 const 引用，零拷贝 -----
    const std::unordered_map<std::string, Cell>& getCells() const { return m_cells; }

private:
    std::ifstream m_fileStream;
    std::vector<std::string> m_parsedFile;
    std::unordered_map<std::string, Template> m_templateDefs;
    std::unordered_map<std::string, Cell> m_cells;
    std::vector<Pin> m_pins;
    std::vector<TimingArc> m_timingArcs;
    std::unordered_map<TableType, LookupTable> m_lookTables;
    std::vector<BlockName> m_blockStack;
};

#endif

6️⃣ LibertyParser.cpp
cpp

#include "LibertyParser.h"

// =====================================================================================================================
LibertyParser::LibertyParser(const std::string fileName){
    m_fileStream.open(fileName);
}

LibertyParser::~LibertyParser(){}

// =====================================================================================================================
void LibertyParser::deleteComments() {
    std::string line;
    bool isInBlockComment = false;

    while (std::getline(m_fileStream, line)) {
        if (isInBlockComment) {
            size_t endPos = line.find("*/");
            if (endPos != std::string::npos) {
                line = line.substr(endPos + 2);
                isInBlockComment = false;
            } else {
                continue;
            }
        }

        size_t commentPos = line.find("//");
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }

        while (true) {
            size_t blockCommentStart = line.find("/*");
            if (blockCommentStart == std::string::npos) break;
            size_t blockCommentEnd = line.find("*/", blockCommentStart + 2);
            if (blockCommentEnd != std::string::npos) {
                line.erase(blockCommentStart, blockCommentEnd - blockCommentStart + 2);
            } else {
                isInBlockComment = true;
                line.erase(blockCommentStart);
                break;
            }
        }

        if(line.empty() || line.find_first_not_of(" \t") == std::string::npos) {
            continue;
        }

        m_parsedFile.push_back(line);
    }
}

// =====================================================================================================================
void LibertyParser::parseFileToBlock() {
    std::string pendingLine;
    bool isUsePending = false;
    std::string currentLine;

    std::string currentLibName;
    std::string currentTemplName;
    std::string currentCellName;
    std::string currentPinName;
    std::string currentTableTemplate;
    std::pair<std::string, std::string> splitedStringPair;
    TableType currentTableType;

    size_t lineIndex = 0;
    while(lineIndex < m_parsedFile.size()) {
        if(isUsePending == true) {
            currentLine = pendingLine;
            isUsePending = false;
        } else {
            currentLine = m_parsedFile[lineIndex];
            lineIndex ++;
        }

        if(currentLine.find('{') != std::string::npos) {
            if(currentLine.find("library") != std::string::npos) {
                m_blockStack.push_back(BlockName::LIBRARY);
                currentLibName = extractBlockName(currentLine);
            }
            else if(currentLine.find("lu_table_template") != std::string::npos) {
                m_blockStack.push_back(BlockName::TEMPLATE);
                currentTemplName = extractBlockName(currentLine);
                m_templateDefs[currentTemplName] = Template{};
            }
            else if(currentLine.find("cell_") == std::string::npos && currentLine.find("cell") != std::string::npos) {
                m_blockStack.push_back(BlockName::CELL);
                currentCellName = extractBlockName(currentLine);
                m_cells[currentCellName] = Cell{currentCellName, {}, 0.0, 0.0};
            }
            else if(currentLine.find("pin") != std::string::npos) {
                m_blockStack.push_back(BlockName::PIN);
                currentPinName = extractBlockName(currentLine);
                m_pins.push_back(Pin{currentPinName, "", 0.0, false, "", {}});
            }
            else if(currentLine.find("timing()") != std::string::npos) {
                m_blockStack.push_back(BlockName::TIMING);
                m_timingArcs.push_back(TimingArc{});
            }
            else if(currentLine.find("FF") != std::string::npos) {
                m_blockStack.push_back(BlockName::FF);
            }
            else if(currentLine.find("cell_rise") != std::string::npos ||
                    currentLine.find("cell_fall") != std::string::npos ||
                    currentLine.find("rise_transition") != std::string::npos ||
                    currentLine.find("fall_transition") != std::string::npos) {

                m_blockStack.push_back(BlockName::TABLE);

                if(currentLine.find("cell_rise") != std::string::npos) {
                    m_lookTables[TableType::cell_rise] = LookupTable{};
                    currentTableType = TableType::cell_rise;
                }
                else if(currentLine.find("cell_fall") != std::string::npos) {
                    m_lookTables[TableType::cell_fall] = LookupTable{};
                    currentTableType = TableType::cell_fall;
                }
                else if(currentLine.find("rise_transition") != std::string::npos) {
                    m_lookTables[TableType::rise_transition] = LookupTable{};
                    currentTableType = TableType::rise_transition;
                }
                else if(currentLine.find("fall_transition") != std::string::npos) {
                    m_lookTables[TableType::fall_transition] = LookupTable{};
                    currentTableType = TableType::fall_transition;
                }
                currentTableTemplate = extractBlockName(currentLine);
            }

            size_t bracePos = currentLine.find('{');
            std::string rest = currentLine.substr(bracePos + 1);
            while(!rest.empty() && (rest.front() == ' ' || rest.front() == '\t')) {
                rest.erase(0, 1);
            }
            if(!rest.empty()) {
                pendingLine = rest;
                isUsePending = true;
            }
        }
        else if(!m_blockStack.empty() && trimString(currentLine) != "}") {
            if(currentLine.find('}') != std::string::npos) {
                pendingLine = "}";
                isUsePending = true;
            }
            
            BlockName top = m_blockStack.back();

            if(top == BlockName::LIBRARY) {
                // Library-level attributes
            }
            else if(top == BlockName::TEMPLATE) {
                std::string splitBaseStr = ";";
                splitedStringPair = splitStringToPair(currentLine, splitBaseStr);
                std::string currentAttrString = splitedStringPair.first;
                std::string rest = splitedStringPair.second;

                if(!rest.empty()) {
                    pendingLine = rest;
                    isUsePending = true;
                }

                if(currentAttrString.find("variable_1") != std::string::npos) {
                    splitedStringPair = splitStringToPair(currentLine, ":");
                    m_templateDefs[currentTemplName].variable_1 = splitedStringPair.second;
                }
                else if(currentAttrString.find("variable_2") != std::string::npos) {
                    splitedStringPair = splitStringToPair(currentLine, ":");
                    m_templateDefs[currentTemplName].variable_2 = splitedStringPair.second;
                }
                else if(currentAttrString.find("index_1") != std::string::npos) {
                    auto matrix1 = parseValuesToMatrix(currentAttrString);
                    m_templateDefs[currentTemplName].index1 = matrix1.back();
                }
                else if(currentAttrString.find("index_2") != std::string::npos) {
                    auto matrix2 = parseValuesToMatrix(currentAttrString);
                    m_templateDefs[currentTemplName].index2 = matrix2.back();
                }
            }
            else if(top == BlockName::CELL) {
                // Cell-level attributes
            }
            else if(top == BlockName::PIN) {
                std::string splitBaseStr = ";";
                splitedStringPair = splitStringToPair(currentLine, splitBaseStr);
                std::string currentAttrString = splitedStringPair.first;
                std::string rest = splitedStringPair.second;

                if(!rest.empty()) {
                    pendingLine = rest;
                    isUsePending = true;
                }

                if(currentAttrString.find("direction") != std::string::npos) {
                    m_pins.back().direction = getAttrValue(currentAttrString);
                }
                else if(currentAttrString.find("capacitance") != std::string::npos) {
                    try {
                        m_pins.back().capacitance = std::stod(getAttrValue(currentAttrString));
                    } catch(...) {
                        std::cout << "Error converting capacitance" << std::endl;
                    }
                }
                else if(currentAttrString.find("clock") != std::string::npos) {
                    m_pins.back().is_clock = (getAttrValue(currentAttrString) == "true");
                }
                else if(currentAttrString.find("function") != std::string::npos) {
                    m_pins.back().function = getAttrValue(currentAttrString);
                }
            }
            else if(top == BlockName::TIMING) {
                std::string splitBaseStr = ";";
                splitedStringPair = splitStringToPair(currentLine, splitBaseStr);
                std::string currentAttrString = splitedStringPair.first;
                std::string rest = splitedStringPair.second;

                if(!rest.empty()) {
                    pendingLine = rest;
                    isUsePending = true;
                }

                if(currentAttrString.find("related_pin") != std::string::npos) {
                    m_timingArcs.back().related_pin = getAttrValue(currentAttrString);
                }
                else if(currentAttrString.find("timing_sense") != std::string::npos) {
                    m_timingArcs.back().timing_sense = getAttrValue(currentAttrString);
                }
                else if(currentAttrString.find("timing_type") != std::string::npos) {
                    m_timingArcs.back().timing_type = getAttrValue(currentAttrString);
                }
            }
            else if(top == BlockName::TABLE) {
                if(currentLine.find("values") != std::string::npos) {
                    auto matrix = parseValuesToMatrix(currentLine);

                    if(currentTableType == TableType::cell_rise) {
                        m_lookTables[TableType::cell_rise].values = matrix;
                        if(m_templateDefs.count(currentTableTemplate) > 0) {
                            m_lookTables[TableType::cell_rise].index1 = m_templateDefs[currentTableTemplate].index1;
                            m_lookTables[TableType::cell_rise].index2 = m_templateDefs[currentTableTemplate].index2;
                        }
                    }
                    else if(currentTableType == TableType::cell_fall) {
                        m_lookTables[TableType::cell_fall].values = matrix;
                        if(m_templateDefs.count(currentTableTemplate) > 0) {
                            m_lookTables[TableType::cell_fall].index1 = m_templateDefs[currentTableTemplate].index1;
                            m_lookTables[TableType::cell_fall].index2 = m_templateDefs[currentTableTemplate].index2;
                        }
                    }
                    else if(currentTableType == TableType::rise_transition) {
                        m_lookTables[TableType::rise_transition].values = matrix;
                        if(m_templateDefs.count(currentTableTemplate) > 0) {
                            m_lookTables[TableType::rise_transition].index1 = m_templateDefs[currentTableTemplate].index1;
                            m_lookTables[TableType::rise_transition].index2 = m_templateDefs[currentTableTemplate].index2;
                        }
                    }
                    else if(currentTableType == TableType::fall_transition) {
                        m_lookTables[TableType::fall_transition].values = matrix;
                        if(m_templateDefs.count(currentTableTemplate) > 0) {
                            m_lookTables[TableType::fall_transition].index1 = m_templateDefs[currentTableTemplate].index1;
                            m_lookTables[TableType::fall_transition].index2 = m_templateDefs[currentTableTemplate].index2;
                        }
                    }
                }
            }
        }
        else if(currentLine.find('}') != std::string::npos) {
            if(!m_blockStack.empty()) {
                BlockName topName = m_blockStack.back();

                if(topName == BlockName::TABLE) {
                    if(m_lookTables.count(TableType::cell_rise) > 0) {
                        m_timingArcs.back().cell_rise = m_lookTables[TableType::cell_rise];
                    }
                    if(m_lookTables.count(TableType::cell_fall) > 0) {
                        m_timingArcs.back().cell_fall = m_lookTables[TableType::cell_fall];
                    }
                    if(m_lookTables.count(TableType::rise_transition) > 0) {
                        m_timingArcs.back().rise_transition = m_lookTables[TableType::rise_transition];
                    }
                    if(m_lookTables.count(TableType::fall_transition) > 0) {
                        m_timingArcs.back().fall_transition = m_lookTables[TableType::fall_transition];
                    }
                }
                else if(topName == BlockName::TIMING) {
                    m_pins.back().timing_arcs.push_back(m_timingArcs.back());
                }
                else if(topName == BlockName::PIN) {
                    m_cells[currentCellName].pins.push_back(m_pins.back());
                }

                m_blockStack.pop_back();
            }
        }
    }

    // ----- 打印解析结果（用于验证）-----
    std::cout << "========================= Parsed Lib =========================";
    for(const auto & [cellName, cell] : m_cells) {
        std::cout << "\nCell:" << cellName << std::endl;
        std::cout << "{" << std::endl;
        for(const auto& pin : cell.pins) {
            std::cout << " \tPin: " << pin.name << ", Direction: " << pin.direction
                      << ", Capacitance: " << pin.capacitance;
            if(pin.is_clock) std::cout << ", Is Clock: true";
            if(!pin.function.empty()) std::cout << ", Function: " << pin.function;
            std::cout << std::endl;
            for(const auto& arc : pin.timing_arcs) {
                std::cout << "\t\tTiming Arc: related_pin=" << arc.related_pin
                          << ", sense=" << arc.timing_sense
                          << ", type=" << arc.timing_type << std::endl;
            }
        }
        std::cout << "}" << std::endl;
    }
}

// =====================================================================================================================
std::vector<std::vector<double>> LibertyParser::parseValuesToMatrix(const std::string& valuesString) {
    std::vector<std::vector<double>> matrix;
    size_t startPos = valuesString.find('(');
    size_t endPos = valuesString.rfind(')');
    if(startPos == std::string::npos || endPos == std::string::npos || endPos <= startPos) {
        std::cerr << "Error: Invalid values string format" << std::endl;
        return matrix;
    }

    std::string content = valuesString.substr(startPos + 1, endPos - startPos - 1);
    std::stringstream ss(content);
    std::string segment;

    while(std::getline(ss, segment, '\"')) {
        if(segment.empty() || segment.find_first_not_of(" ,\t") == std::string::npos) continue;

        std::vector<double> row;
        std::stringstream row_ss(segment);
        std::string num;
        while(std::getline(row_ss, num, ',')) {
            num.erase(0, num.find_first_not_of(" \t"));
            num.erase(num.find_last_not_of(" \t") + 1);
            if(!num.empty()) row.push_back(std::stod(num));
        }
        if(!row.empty()) matrix.push_back(row);
    }
    return matrix;
}

// =====================================================================================================================
std::pair<std::string, std::string> LibertyParser::splitStringToPair(const std::string& originString, const std::string& splitBaseStr) {
    size_t pos = originString.find(splitBaseStr);
    if(pos != std::string::npos) {
        std::string firstString = trimString(originString.substr(0, pos));
        std::string secondString = trimString(originString.substr(pos + splitBaseStr.length()));
        return {firstString, secondString};
    }
    return {trimString(originString), ""};
}

// =====================================================================================================================
std::string LibertyParser::getAttrValue(const std::string& line) {
    auto pair = splitStringToPair(line, ":");
    return trimString(pair.second);
}

// =====================================================================================================================
std::string LibertyParser::trimString(const std::string& str) {
    std::string result = str;
    while(!result.empty() && (result.front() == ' ' || result.front() == '\t')) result.erase(0, 1);
    while(!result.empty() && (result.back() == ' ' || result.back() == '\t')) result.pop_back();
    if(!result.empty() && (result.front() == '\"' || result.front() == '\'')) result.erase(0, 1);
    if(!result.empty() && (result.back() == '\"' || result.back() == '\'')) result.pop_back();
    while(!result.empty() && (result.front() == ' ' || result.front() == '\t')) result.erase(0, 1);
    while(!result.empty() && (result.back() == ' ' || result.back() == '\t')) result.pop_back();
    return result;
}

// =====================================================================================================================
std::string LibertyParser::extractBlockName(const std::string& currentLine) {
    size_t start = currentLine.find('(') + 1;
    size_t end = currentLine.find(')', start);
    return trimString(currentLine.substr(start, end - start));
}

// =====================================================================================================================
const char* LibertyParser::blockNameToString(BlockName name) {
    switch(name) {
        case BlockName::LIBRARY: return "LIBRARY";
        case BlockName::TEMPLATE: return "TEMPLATE";
        case BlockName::CELL: return "CELL";
        case BlockName::PIN: return "PIN";
        case BlockName::TIMING: return "TIMING";
        case BlockName::FF: return "FF";
        case BlockName::TABLE: return "TABLE";
        default: return "UNKNOWN";
    }
}

// =====================================================================================================================
double LibertyParser::interpolateDelay(const LookupTable& table, double x, double y) {
    if(table.index1.size() < 2 || table.index2.size() < 2 || table.values.empty()) {
        std::cout << "The LookupTable is wrong!" << std::endl;
        return 0.0;
    }

    const auto& index_X = table.index1;
    const auto& index_Y = table.index2;
    const auto& values = table.values;

    size_t sizeX = index_X.size();
    size_t sizeY = index_Y.size();

    size_t x_idx;
    if(x <= index_X.front()) {
        x_idx = 0;
    } else if(x >= index_X.back()) {
        x_idx = sizeX - 2;
    } else {
        auto it = std::lower_bound(index_X.begin(), index_X.end(), x);
        x_idx = std::distance(index_X.begin(), it) - 1;
    }

    if(x_idx >= sizeX - 1) x_idx = sizeX - 2;
    double x1 = index_X[x_idx];
    double x2 = index_X[x_idx + 1];

    size_t y_idx;
    if(y <= index_Y.front()) {
        y_idx = 0;
    } else if(y >= index_Y.back()) {
        y_idx = sizeY - 2;
    } else {
        auto it = std::lower_bound(index_Y.begin(), index_Y.end(), y);
        y_idx = std::distance(index_Y.begin(), it) - 1;
    }

    if(y_idx >= sizeY - 1) y_idx = sizeY - 2;
    double y1 = index_Y[y_idx];
    double y2 = index_Y[y_idx + 1];

    double dx = (x2 - x1 == 0) ? 0 : (x - x1) / (x2 - x1);
    double dy = (y2 - y1 == 0) ? 0 : (y - y1) / (y2 - y1);

    double v11 = values[x_idx][y_idx];
    double v12 = values[x_idx][y_idx + 1];
    double v21 = values[x_idx + 1][y_idx];
    double v22 = values[x_idx + 1][y_idx + 1];

    return v11 * (1 - dx) * (1 - dy) +
           v12 * (1 - dx) * dy +
           v21 * dx * (1 - dy) +
           v22 * dx * dy;
}

7️⃣ PathAnalyzer.h
cpp

#ifndef PATH_ANALYZER_H
#define PATH_ANALYZER_H

#include "LibertyParser.h"
#include "AnalysisConfig.h"
#include <vector>
#include <string>

class PathAnalyzer {
public:
    explicit PathAnalyzer(LibertyParser& parser);   // 已加 explicit

    TimingPath analyzeSetupTiming(const std::string& pathName,
                                   const AnalysisConfig& config);

    void printPathResult(const TimingPath& path);

private:
    LibertyParser& m_parser;
    const std::unordered_map<std::string, Cell>& m_cells;

    std::vector<PathSegment> buildSegmentsFromConfig(const AnalysisConfig& config);
    double accumulatePathDelay(std::vector<PathSegment>& segments);
};

#endif

8️⃣ PathAnalyzer.cpp
cpp

#include "PathAnalyzer.h"
#include <iostream>
#include <iomanip>
#include <tuple>

PathAnalyzer::PathAnalyzer(LibertyParser& parser)
    : m_parser(parser), m_cells(parser.getCells()) {}

std::vector<PathSegment> PathAnalyzer::buildSegmentsFromConfig(const AnalysisConfig& config) {
    std::vector<PathSegment> segments;

    PathSegment mainSeg;
    mainSeg.cellName = config.cellName;
    mainSeg.fromPin = config.inputPin;
    mainSeg.toPin = config.outputPin;
    mainSeg.tableType = config.tableType;
    mainSeg.transition = config.transition;
    mainSeg.load = config.load;
    segments.push_back(mainSeg);

    for(const auto& extra : config.extraSegments) {
        PathSegment seg;
        seg.cellName = std::get<0>(extra);
        seg.fromPin = std::get<1>(extra);
        seg.toPin = std::get<2>(extra);
        seg.tableType = std::get<3>(extra);
        seg.transition = config.transition;
        seg.load = config.load;
        segments.push_back(seg);
    }

    return segments;
}

double PathAnalyzer::accumulatePathDelay(std::vector<PathSegment>& segments) {
    double totalDelay = 0.0;
    if(segments.empty()) return 0.0;

    double currentTransition = segments[0].transition;

    for(size_t i = 0; i < segments.size(); ++i) {
        auto& seg = segments[i];

        auto it = m_cells.find(seg.cellName);
        if(it == m_cells.end()) {
            std::cerr << "[Error] Cell not found: " << seg.cellName << std::endl;
            seg.delay = 0.0;
            continue;
        }
        const Cell& cell = it->second;

        const TimingArc* targetArc = nullptr;
        for(const auto& pin : cell.pins) {
            if(pin.name == seg.toPin) {
                for(const auto& arc : pin.timing_arcs) {
                    if(arc.related_pin == seg.fromPin) {
                        targetArc = &arc;
                        break;
                    }
                }
                break;
            }
        }

        if(targetArc == nullptr) {
            std::cerr << "[Error] No arc: " << seg.fromPin << " -> " << seg.toPin << std::endl;
            seg.delay = 0.0;
            continue;
        }

        LookupTable delayTable;
        if(seg.tableType == "cell_rise") {
            delayTable = targetArc->cell_rise;
        } else if(seg.tableType == "cell_fall") {
            delayTable = targetArc->cell_fall;
        } else {
            std::cerr << "[Error] Unknown table type: " << seg.tableType << std::endl;
            seg.delay = 0.0;
            continue;
        }

        double segDelay = m_parser.interpolateDelay(delayTable, currentTransition, seg.load);
        seg.delay = segDelay;
        totalDelay += segDelay;

        // ----- 斜率传播 -----
        LookupTable slewTable;
        if(seg.tableType == "cell_rise") {
            slewTable = targetArc->rise_transition;
        } else {
            slewTable = targetArc->fall_transition;
        }
        double outputSlew = m_parser.interpolateDelay(slewTable, currentTransition, seg.load);

        if(i + 1 < segments.size()) {
            segments[i + 1].transition = outputSlew;
            std::cout << "[Slew] Stage " << i << " -> " << i+1
                      << " : " << outputSlew << " ps" << std::endl;
        }
        currentTransition = outputSlew;
    }

    return totalDelay;
}

TimingPath PathAnalyzer::analyzeSetupTiming(const std::string& pathName,
                                             const AnalysisConfig& config) {
    TimingPath path;
    path.pathName = pathName;
    path.clockPeriod = config.clockPeriod;
    path.setupRequire = config.setupRequire;

    auto segments = buildSegmentsFromConfig(config);
    path.totalDelay = accumulatePathDelay(segments);
    path.segments = segments;

    path.slack = path.clockPeriod - path.totalDelay - path.setupRequire;
    path.isSetupMet = (path.slack >= 0);

    return path;
}

void PathAnalyzer::printPathResult(const TimingPath& path) {
    std::cout << "\n========== Timing Path Analysis ==========" << std::endl;
    std::cout << "Path Name     : " << path.pathName << std::endl;
    std::cout << "Clock Period  : " << std::fixed << std::setprecision(3)
              << path.clockPeriod << " ps" << std::endl;
    std::cout << "Setup Require : " << path.setupRequire << " ps" << std::endl;
    std::cout << "Total Delay   : " << path.totalDelay << " ps" << std::endl;
    std::cout << "Slack         : " << path.slack << " ps" << std::endl;
    std::cout << "Status        : " << (path.isSetupMet ? "✅ MET" : "❌ VIOLATED") << std::endl;

    std::cout << "\n--- Path Detail (Incr / Accum) ---" << std::endl;
    double accumulated = 0.0;
    for(size_t i = 0; i < path.segments.size(); ++i) {
        const auto& seg = path.segments[i];
        accumulated += seg.delay;
        std::cout << "  [" << i << "] "
                  << seg.cellName << " " << seg.fromPin << " -> " << seg.toPin
                  << "  |  Incr: " << std::fixed << std::setprecision(3) << seg.delay
                  << " ps  |  Accum: " << accumulated << " ps" << std::endl;
    }
    std::cout << "==========================================\n" << std::endl;
}

9️⃣ main.cpp
cpp

#include "LibertyParser.h"
#include "PathAnalyzer.h"
#include "ConfigLoader.h"

int main() {
    std::string libFile = "/home/xulei/Lei_Develop/My_Sta_Lab/day1/simple.lib";

    LibertyParser parser(libFile);
    parser.deleteComments();
    parser.parseFileToBlock();

    PathAnalyzer analyzer(parser);

    AnalysisConfig config = ConfigLoader::loadFromFile("config.cfg");

    TimingPath result = analyzer.analyzeSetupTiming("MainPath", config);
    analyzer.printPathResult(result);

    return 0;
}

🔟 config.cfg
ini

# STA Configuration File
cell_name   = "AND2_X1"
input_pin   = "A"
output_pin  = "Y"
table_type  = "cell_rise"
transition  = 25.0
load        = 50.0
clock_period = 100.0
setup_require = 0.0

# 多级路径示例（取消注释即可启用）
# extra_segment = "BUF_X1,A,Y,cell_rise"
# extra_segment = "AND2_X1,A,Y,cell_fall"

🔹 编译命令
bash

g++ -std=c++17 -g \
    LibertyParser.cpp \
    PathAnalyzer.cpp \
    ConfigLoader.cpp \
    main.cpp \
    -o LibertyParser

# 或使用 Makefile（推荐）：
# make -j 4
完整文件列表
text

lib_parser_new/
├── AnalysisConfig.h          (纯数据容器)
├── ConfigLoader.h            (配置加载器)
├── ConfigLoader.cpp          
├── LibertyDataStructures.h   (数据结构)
├── LibertyParser.h           (已加 explicit)
├── LibertyParser.cpp         
├── PathAnalyzer.h            (已加 explicit)
├── PathAnalyzer.cpp          
├── main.cpp                  
├── config.cfg                
└── simple.lib                

1️⃣ AnalysisConfig.h
cpp

#ifndef ANALYSIS_CONFIG_H
#define ANALYSIS_CONFIG_H

#include <string>
#include <vector>
#include <tuple>

struct AnalysisConfig {
    // ----- 主路径参数 -----
    std::string cellName;
    std::string inputPin;
    std::string outputPin;
    std::string tableType;          // "cell_rise" 或 "cell_fall"
    
    // ----- 工作条件 -----
    double transition = 0.0;        // 输入转换时间 (ps)
    double load = 0.0;              // 输出负载电容 (fF)

    // ----- 时序约束 -----
    double clockPeriod = 0.0;       // 时钟周期 (ps)
    double setupRequire = 0.0;      // 建立时间约束 (ps)

    // ----- 多级路径扩展 -----
    // 每个元素为 {cellName, fromPin, toPin, tableType}
    std::vector<std::tuple<std::string, std::string, std::string, std::string>> extraSegments;
};

#endif

2️⃣ ConfigLoader.h
cpp

#ifndef CONFIG_LOADER_H
#define CONFIG_LOADER_H

#include "AnalysisConfig.h"
#include <string>

namespace ConfigLoader {
    AnalysisConfig loadFromFile(const std::string& filePath);
}

#endif

3️⃣ ConfigLoader.cpp
cpp

#include "ConfigLoader.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

namespace ConfigLoader {

static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
}

AnalysisConfig loadFromFile(const std::string& filePath) {
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

        if (value.front() == '\"' && value.back() == '\"') {
            value = value.substr(1, value.size() - 2);
        }

        if (key == "cell_name") config.cellName = value;
        else if (key == "input_pin") config.inputPin = value;
        else if (key == "output_pin") config.outputPin = value;
        else if (key == "table_type") config.tableType = value;
        else if (key == "transition") config.transition = std::stod(value);
        else if (key == "load") config.load = std::stod(value);
        else if (key == "clock_period") config.clockPeriod = std::stod(value);
        else if (key == "setup_require") config.setupRequire = std::stod(value);
        else if (key == "extra_segment") {
            std::stringstream ss(value);
            std::string part;
            std::vector<std::string> parts;
            while (std::getline(ss, part, ',')) {
                parts.push_back(trim(part));
            }
            if (parts.size() >= 4) {
                config.extraSegments.emplace_back(
                    parts[0], parts[1], parts[2], parts[3]
                );
            }
        }
    }
    return config;
}

} // namespace ConfigLoader

4️⃣ LibertyDataStructures.h
cpp

#ifndef LIBERTY_DATA_STRUCTURES_H
#define LIBERTY_DATA_STRUCTURES_H

#include <vector> 
#include <string>

// ----- 原有的数据结构 -----
struct LookupTable {
    std::vector<double> index1;
    std::vector<double> index2;
    std::vector<std::vector<double>> values;
};

struct TimingArc {
    std::string related_pin;
    std::string timing_sense;
    std::string timing_type;
    LookupTable cell_rise;
    LookupTable rise_transition;
    LookupTable cell_fall;
    LookupTable fall_transition;
};

struct Pin {
    std::string name;
    std::string direction;
    double capacitance = 0.0;
    bool is_clock = false;
    std::string function;
    std::vector<TimingArc> timing_arcs;
};

struct Cell {
    std::string name;
    std::vector<Pin> pins;
    double setup = 0.0;
    double hold = 0.0;
};

struct Template {
    std::string variable_1;
    std::string variable_2;
    std::vector<double> index1;
    std::vector<double> index2;
};

// ----- 新增：路径分析数据结构 -----
struct PathSegment {
    std::string cellName;
    std::string fromPin;
    std::string toPin;
    std::string tableType;
    double transition = 0.0;
    double load = 0.0;
    double delay = 0.0;          // 计算后回填
};

struct TimingPath {
    std::string pathName;
    std::vector<PathSegment> segments;
    double totalDelay = 0.0;
    double clockPeriod = 0.0;
    double setupRequire = 0.0;
    double slack = 0.0;
    bool isSetupMet = false;
};

#endif

5️⃣ LibertyParser.h
cpp

#ifndef LIBERTY_PARSER_H
#define LIBERTY_PARSER_H

#include <unordered_map>
#include <fstream> 
#include <iostream>
#include <sstream>
#include "LibertyDataStructures.h"

enum class BlockName {
    LIBRARY = 1,
    TEMPLATE,
    CELL,
    PIN,
    TIMING,
    FF,
    TABLE
};

enum class TableType {
    cell_rise = 1,
    cell_fall,
    rise_transition,
    fall_transition
};

class LibertyParser {
public:
    explicit LibertyParser(const std::string fileName);   // 已加 explicit
    ~LibertyParser();

    void deleteComments();
    void parseFileToBlock();
    
    std::string extractBlockName(const std::string& currentLine);
    std::string getAttrValue(const std::string& line);
    std::string trimString(const std::string& str);
    std::pair<std::string, std::string> splitStringToPair(const std::string& originString, const std::string& splitBaseStr);
    std::vector<std::vector<double>> parseValuesToMatrix(const std::string& valuesString);
    double interpolateDelay(const LookupTable& table, double x, double y);
    const char* blockNameToString(BlockName name);

    // ----- 关键：返回 const 引用，零拷贝 -----
    const std::unordered_map<std::string, Cell>& getCells() const { return m_cells; }

private:
    std::ifstream m_fileStream;
    std::vector<std::string> m_parsedFile;
    std::unordered_map<std::string, Template> m_templateDefs;
    std::unordered_map<std::string, Cell> m_cells;
    std::vector<Pin> m_pins;
    std::vector<TimingArc> m_timingArcs;
    std::unordered_map<TableType, LookupTable> m_lookTables;
    std::vector<BlockName> m_blockStack;
};

#endif

6️⃣ LibertyParser.cpp
cpp

#include "LibertyParser.h"

// =====================================================================================================================
LibertyParser::LibertyParser(const std::string fileName){
    m_fileStream.open(fileName);
}

LibertyParser::~LibertyParser(){}

// =====================================================================================================================
void LibertyParser::deleteComments() {
    std::string line;
    bool isInBlockComment = false;

    while (std::getline(m_fileStream, line)) {
        if (isInBlockComment) {
            size_t endPos = line.find("*/");
            if (endPos != std::string::npos) {
                line = line.substr(endPos + 2);
                isInBlockComment = false;
            } else {
                continue;
            }
        }

        size_t commentPos = line.find("//");
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }

        while (true) {
            size_t blockCommentStart = line.find("/*");
            if (blockCommentStart == std::string::npos) break;
            size_t blockCommentEnd = line.find("*/", blockCommentStart + 2);
            if (blockCommentEnd != std::string::npos) {
                line.erase(blockCommentStart, blockCommentEnd - blockCommentStart + 2);
            } else {
                isInBlockComment = true;
                line.erase(blockCommentStart);
                break;
            }
        }

        if(line.empty() || line.find_first_not_of(" \t") == std::string::npos) {
            continue;
        }

        m_parsedFile.push_back(line);
    }
}

// =====================================================================================================================
void LibertyParser::parseFileToBlock() {
    std::string pendingLine;
    bool isUsePending = false;
    std::string currentLine;

    std::string currentLibName;
    std::string currentTemplName;
    std::string currentCellName;
    std::string currentPinName;
    std::string currentTableTemplate;
    std::pair<std::string, std::string> splitedStringPair;
    TableType currentTableType;

    size_t lineIndex = 0;
    while(lineIndex < m_parsedFile.size()) {
        if(isUsePending == true) {
            currentLine = pendingLine;
            isUsePending = false;
        } else {
            currentLine = m_parsedFile[lineIndex];
            lineIndex ++;
        }

        if(currentLine.find('{') != std::string::npos) {
            if(currentLine.find("library") != std::string::npos) {
                m_blockStack.push_back(BlockName::LIBRARY);
                currentLibName = extractBlockName(currentLine);
            }
            else if(currentLine.find("lu_table_template") != std::string::npos) {
                m_blockStack.push_back(BlockName::TEMPLATE);
                currentTemplName = extractBlockName(currentLine);
                m_templateDefs[currentTemplName] = Template{};
            }
            else if(currentLine.find("cell_") == std::string::npos && currentLine.find("cell") != std::string::npos) {
                m_blockStack.push_back(BlockName::CELL);
                currentCellName = extractBlockName(currentLine);
                m_cells[currentCellName] = Cell{currentCellName, {}, 0.0, 0.0};
            }
            else if(currentLine.find("pin") != std::string::npos) {
                m_blockStack.push_back(BlockName::PIN);
                currentPinName = extractBlockName(currentLine);
                m_pins.push_back(Pin{currentPinName, "", 0.0, false, "", {}});
            }
            else if(currentLine.find("timing()") != std::string::npos) {
                m_blockStack.push_back(BlockName::TIMING);
                m_timingArcs.push_back(TimingArc{});
            }
            else if(currentLine.find("FF") != std::string::npos) {
                m_blockStack.push_back(BlockName::FF);
            }
            else if(currentLine.find("cell_rise") != std::string::npos ||
                    currentLine.find("cell_fall") != std::string::npos ||
                    currentLine.find("rise_transition") != std::string::npos ||
                    currentLine.find("fall_transition") != std::string::npos) {

                m_blockStack.push_back(BlockName::TABLE);

                if(currentLine.find("cell_rise") != std::string::npos) {
                    m_lookTables[TableType::cell_rise] = LookupTable{};
                    currentTableType = TableType::cell_rise;
                }
                else if(currentLine.find("cell_fall") != std::string::npos) {
                    m_lookTables[TableType::cell_fall] = LookupTable{};
                    currentTableType = TableType::cell_fall;
                }
                else if(currentLine.find("rise_transition") != std::string::npos) {
                    m_lookTables[TableType::rise_transition] = LookupTable{};
                    currentTableType = TableType::rise_transition;
                }
                else if(currentLine.find("fall_transition") != std::string::npos) {
                    m_lookTables[TableType::fall_transition] = LookupTable{};
                    currentTableType = TableType::fall_transition;
                }
                currentTableTemplate = extractBlockName(currentLine);
            }

            size_t bracePos = currentLine.find('{');
            std::string rest = currentLine.substr(bracePos + 1);
            while(!rest.empty() && (rest.front() == ' ' || rest.front() == '\t')) {
                rest.erase(0, 1);
            }
            if(!rest.empty()) {
                pendingLine = rest;
                isUsePending = true;
            }
        }
        else if(!m_blockStack.empty() && trimString(currentLine) != "}") {
            if(currentLine.find('}') != std::string::npos) {
                pendingLine = "}";
                isUsePending = true;
            }
            
            BlockName top = m_blockStack.back();

            if(top == BlockName::LIBRARY) {
                // Library-level attributes
            }
            else if(top == BlockName::TEMPLATE) {
                std::string splitBaseStr = ";";
                splitedStringPair = splitStringToPair(currentLine, splitBaseStr);
                std::string currentAttrString = splitedStringPair.first;
                std::string rest = splitedStringPair.second;

                if(!rest.empty()) {
                    pendingLine = rest;
                    isUsePending = true;
                }

                if(currentAttrString.find("variable_1") != std::string::npos) {
                    splitedStringPair = splitStringToPair(currentLine, ":");
                    m_templateDefs[currentTemplName].variable_1 = splitedStringPair.second;
                }
                else if(currentAttrString.find("variable_2") != std::string::npos) {
                    splitedStringPair = splitStringToPair(currentLine, ":");
                    m_templateDefs[currentTemplName].variable_2 = splitedStringPair.second;
                }
                else if(currentAttrString.find("index_1") != std::string::npos) {
                    auto matrix1 = parseValuesToMatrix(currentAttrString);
                    m_templateDefs[currentTemplName].index1 = matrix1.back();
                }
                else if(currentAttrString.find("index_2") != std::string::npos) {
                    auto matrix2 = parseValuesToMatrix(currentAttrString);
                    m_templateDefs[currentTemplName].index2 = matrix2.back();
                }
            }
            else if(top == BlockName::CELL) {
                // Cell-level attributes
            }
            else if(top == BlockName::PIN) {
                std::string splitBaseStr = ";";
                splitedStringPair = splitStringToPair(currentLine, splitBaseStr);
                std::string currentAttrString = splitedStringPair.first;
                std::string rest = splitedStringPair.second;

                if(!rest.empty()) {
                    pendingLine = rest;
                    isUsePending = true;
                }

                if(currentAttrString.find("direction") != std::string::npos) {
                    m_pins.back().direction = getAttrValue(currentAttrString);
                }
                else if(currentAttrString.find("capacitance") != std::string::npos) {
                    try {
                        m_pins.back().capacitance = std::stod(getAttrValue(currentAttrString));
                    } catch(...) {
                        std::cout << "Error converting capacitance" << std::endl;
                    }
                }
                else if(currentAttrString.find("clock") != std::string::npos) {
                    m_pins.back().is_clock = (getAttrValue(currentAttrString) == "true");
                }
                else if(currentAttrString.find("function") != std::string::npos) {
                    m_pins.back().function = getAttrValue(currentAttrString);
                }
            }
            else if(top == BlockName::TIMING) {
                std::string splitBaseStr = ";";
                splitedStringPair = splitStringToPair(currentLine, splitBaseStr);
                std::string currentAttrString = splitedStringPair.first;
                std::string rest = splitedStringPair.second;

                if(!rest.empty()) {
                    pendingLine = rest;
                    isUsePending = true;
                }

                if(currentAttrString.find("related_pin") != std::string::npos) {
                    m_timingArcs.back().related_pin = getAttrValue(currentAttrString);
                }
                else if(currentAttrString.find("timing_sense") != std::string::npos) {
                    m_timingArcs.back().timing_sense = getAttrValue(currentAttrString);
                }
                else if(currentAttrString.find("timing_type") != std::string::npos) {
                    m_timingArcs.back().timing_type = getAttrValue(currentAttrString);
                }
            }
            else if(top == BlockName::TABLE) {
                if(currentLine.find("values") != std::string::npos) {
                    auto matrix = parseValuesToMatrix(currentLine);

                    if(currentTableType == TableType::cell_rise) {
                        m_lookTables[TableType::cell_rise].values = matrix;
                        if(m_templateDefs.count(currentTableTemplate) > 0) {
                            m_lookTables[TableType::cell_rise].index1 = m_templateDefs[currentTableTemplate].index1;
                            m_lookTables[TableType::cell_rise].index2 = m_templateDefs[currentTableTemplate].index2;
                        }
                    }
                    else if(currentTableType == TableType::cell_fall) {
                        m_lookTables[TableType::cell_fall].values = matrix;
                        if(m_templateDefs.count(currentTableTemplate) > 0) {
                            m_lookTables[TableType::cell_fall].index1 = m_templateDefs[currentTableTemplate].index1;
                            m_lookTables[TableType::cell_fall].index2 = m_templateDefs[currentTableTemplate].index2;
                        }
                    }
                    else if(currentTableType == TableType::rise_transition) {
                        m_lookTables[TableType::rise_transition].values = matrix;
                        if(m_templateDefs.count(currentTableTemplate) > 0) {
                            m_lookTables[TableType::rise_transition].index1 = m_templateDefs[currentTableTemplate].index1;
                            m_lookTables[TableType::rise_transition].index2 = m_templateDefs[currentTableTemplate].index2;
                        }
                    }
                    else if(currentTableType == TableType::fall_transition) {
                        m_lookTables[TableType::fall_transition].values = matrix;
                        if(m_templateDefs.count(currentTableTemplate) > 0) {
                            m_lookTables[TableType::fall_transition].index1 = m_templateDefs[currentTableTemplate].index1;
                            m_lookTables[TableType::fall_transition].index2 = m_templateDefs[currentTableTemplate].index2;
                        }
                    }
                }
            }
        }
        else if(currentLine.find('}') != std::string::npos) {
            if(!m_blockStack.empty()) {
                BlockName topName = m_blockStack.back();

                if(topName == BlockName::TABLE) {
                    if(m_lookTables.count(TableType::cell_rise) > 0) {
                        m_timingArcs.back().cell_rise = m_lookTables[TableType::cell_rise];
                    }
                    if(m_lookTables.count(TableType::cell_fall) > 0) {
                        m_timingArcs.back().cell_fall = m_lookTables[TableType::cell_fall];
                    }
                    if(m_lookTables.count(TableType::rise_transition) > 0) {
                        m_timingArcs.back().rise_transition = m_lookTables[TableType::rise_transition];
                    }
                    if(m_lookTables.count(TableType::fall_transition) > 0) {
                        m_timingArcs.back().fall_transition = m_lookTables[TableType::fall_transition];
                    }
                }
                else if(topName == BlockName::TIMING) {
                    m_pins.back().timing_arcs.push_back(m_timingArcs.back());
                }
                else if(topName == BlockName::PIN) {
                    m_cells[currentCellName].pins.push_back(m_pins.back());
                }

                m_blockStack.pop_back();
            }
        }
    }

    // ----- 打印解析结果（用于验证）-----
    std::cout << "========================= Parsed Lib =========================";
    for(const auto & [cellName, cell] : m_cells) {
        std::cout << "\nCell:" << cellName << std::endl;
        std::cout << "{" << std::endl;
        for(const auto& pin : cell.pins) {
            std::cout << " \tPin: " << pin.name << ", Direction: " << pin.direction
                      << ", Capacitance: " << pin.capacitance;
            if(pin.is_clock) std::cout << ", Is Clock: true";
            if(!pin.function.empty()) std::cout << ", Function: " << pin.function;
            std::cout << std::endl;
            for(const auto& arc : pin.timing_arcs) {
                std::cout << "\t\tTiming Arc: related_pin=" << arc.related_pin
                          << ", sense=" << arc.timing_sense
                          << ", type=" << arc.timing_type << std::endl;
            }
        }
        std::cout << "}" << std::endl;
    }
}

// =====================================================================================================================
std::vector<std::vector<double>> LibertyParser::parseValuesToMatrix(const std::string& valuesString) {
    std::vector<std::vector<double>> matrix;
    size_t startPos = valuesString.find('(');
    size_t endPos = valuesString.rfind(')');
    if(startPos == std::string::npos || endPos == std::string::npos || endPos <= startPos) {
        std::cerr << "Error: Invalid values string format" << std::endl;
        return matrix;
    }

    std::string content = valuesString.substr(startPos + 1, endPos - startPos - 1);
    std::stringstream ss(content);
    std::string segment;

    while(std::getline(ss, segment, '\"')) {
        if(segment.empty() || segment.find_first_not_of(" ,\t") == std::string::npos) continue;

        std::vector<double> row;
        std::stringstream row_ss(segment);
        std::string num;
        while(std::getline(row_ss, num, ',')) {
            num.erase(0, num.find_first_not_of(" \t"));
            num.erase(num.find_last_not_of(" \t") + 1);
            if(!num.empty()) row.push_back(std::stod(num));
        }
        if(!row.empty()) matrix.push_back(row);
    }
    return matrix;
}

// =====================================================================================================================
std::pair<std::string, std::string> LibertyParser::splitStringToPair(const std::string& originString, const std::string& splitBaseStr) {
    size_t pos = originString.find(splitBaseStr);
    if(pos != std::string::npos) {
        std::string firstString = trimString(originString.substr(0, pos));
        std::string secondString = trimString(originString.substr(pos + splitBaseStr.length()));
        return {firstString, secondString};
    }
    return {trimString(originString), ""};
}

// =====================================================================================================================
std::string LibertyParser::getAttrValue(const std::string& line) {
    auto pair = splitStringToPair(line, ":");
    return trimString(pair.second);
}

// =====================================================================================================================
std::string LibertyParser::trimString(const std::string& str) {
    std::string result = str;
    while(!result.empty() && (result.front() == ' ' || result.front() == '\t')) result.erase(0, 1);
    while(!result.empty() && (result.back() == ' ' || result.back() == '\t')) result.pop_back();
    if(!result.empty() && (result.front() == '\"' || result.front() == '\'')) result.erase(0, 1);
    if(!result.empty() && (result.back() == '\"' || result.back() == '\'')) result.pop_back();
    while(!result.empty() && (result.front() == ' ' || result.front() == '\t')) result.erase(0, 1);
    while(!result.empty() && (result.back() == ' ' || result.back() == '\t')) result.pop_back();
    return result;
}

// =====================================================================================================================
std::string LibertyParser::extractBlockName(const std::string& currentLine) {
    size_t start = currentLine.find('(') + 1;
    size_t end = currentLine.find(')', start);
    return trimString(currentLine.substr(start, end - start));
}

// =====================================================================================================================
const char* LibertyParser::blockNameToString(BlockName name) {
    switch(name) {
        case BlockName::LIBRARY: return "LIBRARY";
        case BlockName::TEMPLATE: return "TEMPLATE";
        case BlockName::CELL: return "CELL";
        case BlockName::PIN: return "PIN";
        case BlockName::TIMING: return "TIMING";
        case BlockName::FF: return "FF";
        case BlockName::TABLE: return "TABLE";
        default: return "UNKNOWN";
    }
}

// =====================================================================================================================
double LibertyParser::interpolateDelay(const LookupTable& table, double x, double y) {
    if(table.index1.size() < 2 || table.index2.size() < 2 || table.values.empty()) {
        std::cout << "The LookupTable is wrong!" << std::endl;
        return 0.0;
    }

    const auto& index_X = table.index1;
    const auto& index_Y = table.index2;
    const auto& values = table.values;

    size_t sizeX = index_X.size();
    size_t sizeY = index_Y.size();

    size_t x_idx;
    if(x <= index_X.front()) {
        x_idx = 0;
    } else if(x >= index_X.back()) {
        x_idx = sizeX - 2;
    } else {
        auto it = std::lower_bound(index_X.begin(), index_X.end(), x);
        x_idx = std::distance(index_X.begin(), it) - 1;
    }

    if(x_idx >= sizeX - 1) x_idx = sizeX - 2;
    double x1 = index_X[x_idx];
    double x2 = index_X[x_idx + 1];

    size_t y_idx;
    if(y <= index_Y.front()) {
        y_idx = 0;
    } else if(y >= index_Y.back()) {
        y_idx = sizeY - 2;
    } else {
        auto it = std::lower_bound(index_Y.begin(), index_Y.end(), y);
        y_idx = std::distance(index_Y.begin(), it) - 1;
    }

    if(y_idx >= sizeY - 1) y_idx = sizeY - 2;
    double y1 = index_Y[y_idx];
    double y2 = index_Y[y_idx + 1];

    double dx = (x2 - x1 == 0) ? 0 : (x - x1) / (x2 - x1);
    double dy = (y2 - y1 == 0) ? 0 : (y - y1) / (y2 - y1);

    double v11 = values[x_idx][y_idx];
    double v12 = values[x_idx][y_idx + 1];
    double v21 = values[x_idx + 1][y_idx];
    double v22 = values[x_idx + 1][y_idx + 1];

    return v11 * (1 - dx) * (1 - dy) +
           v12 * (1 - dx) * dy +
           v21 * dx * (1 - dy) +
           v22 * dx * dy;
}

7️⃣ PathAnalyzer.h
cpp

#ifndef PATH_ANALYZER_H
#define PATH_ANALYZER_H

#include "LibertyParser.h"
#include "AnalysisConfig.h"
#include <vector>
#include <string>

class PathAnalyzer {
public:
    explicit PathAnalyzer(LibertyParser& parser);   // 已加 explicit

    TimingPath analyzeSetupTiming(const std::string& pathName,
                                   const AnalysisConfig& config);

    void printPathResult(const TimingPath& path);

private:
    LibertyParser& m_parser;
    const std::unordered_map<std::string, Cell>& m_cells;

    std::vector<PathSegment> buildSegmentsFromConfig(const AnalysisConfig& config);
    double accumulatePathDelay(std::vector<PathSegment>& segments);
};

#endif

8️⃣ PathAnalyzer.cpp
cpp

#include "PathAnalyzer.h"
#include <iostream>
#include <iomanip>
#include <tuple>

PathAnalyzer::PathAnalyzer(LibertyParser& parser)
    : m_parser(parser), m_cells(parser.getCells()) {}

std::vector<PathSegment> PathAnalyzer::buildSegmentsFromConfig(const AnalysisConfig& config) {
    std::vector<PathSegment> segments;

    PathSegment mainSeg;
    mainSeg.cellName = config.cellName;
    mainSeg.fromPin = config.inputPin;
    mainSeg.toPin = config.outputPin;
    mainSeg.tableType = config.tableType;
    mainSeg.transition = config.transition;
    mainSeg.load = config.load;
    segments.push_back(mainSeg);

    for(const auto& extra : config.extraSegments) {
        PathSegment seg;
        seg.cellName = std::get<0>(extra);
        seg.fromPin = std::get<1>(extra);
        seg.toPin = std::get<2>(extra);
        seg.tableType = std::get<3>(extra);
        seg.transition = config.transition;
        seg.load = config.load;
        segments.push_back(seg);
    }

    return segments;
}

double PathAnalyzer::accumulatePathDelay(std::vector<PathSegment>& segments) {
    double totalDelay = 0.0;
    if(segments.empty()) return 0.0;

    double currentTransition = segments[0].transition;

    for(size_t i = 0; i < segments.size(); ++i) {
        auto& seg = segments[i];

        auto it = m_cells.find(seg.cellName);
        if(it == m_cells.end()) {
            std::cerr << "[Error] Cell not found: " << seg.cellName << std::endl;
            seg.delay = 0.0;
            continue;
        }
        const Cell& cell = it->second;

        const TimingArc* targetArc = nullptr;
        for(const auto& pin : cell.pins) {
            if(pin.name == seg.toPin) {
                for(const auto& arc : pin.timing_arcs) {
                    if(arc.related_pin == seg.fromPin) {
                        targetArc = &arc;
                        break;
                    }
                }
                break;
            }
        }

        if(targetArc == nullptr) {
            std::cerr << "[Error] No arc: " << seg.fromPin << " -> " << seg.toPin << std::endl;
            seg.delay = 0.0;
            continue;
        }

        LookupTable delayTable;
        if(seg.tableType == "cell_rise") {
            delayTable = targetArc->cell_rise;
        } else if(seg.tableType == "cell_fall") {
            delayTable = targetArc->cell_fall;
        } else {
            std::cerr << "[Error] Unknown table type: " << seg.tableType << std::endl;
            seg.delay = 0.0;
            continue;
        }

        double segDelay = m_parser.interpolateDelay(delayTable, currentTransition, seg.load);
        seg.delay = segDelay;
        totalDelay += segDelay;

        // ----- 斜率传播 -----
        LookupTable slewTable;
        if(seg.tableType == "cell_rise") {
            slewTable = targetArc->rise_transition;
        } else {
            slewTable = targetArc->fall_transition;
        }
        double outputSlew = m_parser.interpolateDelay(slewTable, currentTransition, seg.load);

        if(i + 1 < segments.size()) {
            segments[i + 1].transition = outputSlew;
            std::cout << "[Slew] Stage " << i << " -> " << i+1
                      << " : " << outputSlew << " ps" << std::endl;
        }
        currentTransition = outputSlew;
    }

    return totalDelay;
}

TimingPath PathAnalyzer::analyzeSetupTiming(const std::string& pathName,
                                             const AnalysisConfig& config) {
    TimingPath path;
    path.pathName = pathName;
    path.clockPeriod = config.clockPeriod;
    path.setupRequire = config.setupRequire;

    auto segments = buildSegmentsFromConfig(config);
    path.totalDelay = accumulatePathDelay(segments);
    path.segments = segments;

    path.slack = path.clockPeriod - path.totalDelay - path.setupRequire;
    path.isSetupMet = (path.slack >= 0);

    return path;
}

void PathAnalyzer::printPathResult(const TimingPath& path) {
    std::cout << "\n========== Timing Path Analysis ==========" << std::endl;
    std::cout << "Path Name     : " << path.pathName << std::endl;
    std::cout << "Clock Period  : " << std::fixed << std::setprecision(3)
              << path.clockPeriod << " ps" << std::endl;
    std::cout << "Setup Require : " << path.setupRequire << " ps" << std::endl;
    std::cout << "Total Delay   : " << path.totalDelay << " ps" << std::endl;
    std::cout << "Slack         : " << path.slack << " ps" << std::endl;
    std::cout << "Status        : " << (path.isSetupMet ? "✅ MET" : "❌ VIOLATED") << std::endl;

    std::cout << "\n--- Path Detail (Incr / Accum) ---" << std::endl;
    double accumulated = 0.0;
    for(size_t i = 0; i < path.segments.size(); ++i) {
        const auto& seg = path.segments[i];
        accumulated += seg.delay;
        std::cout << "  [" << i << "] "
                  << seg.cellName << " " << seg.fromPin << " -> " << seg.toPin
                  << "  |  Incr: " << std::fixed << std::setprecision(3) << seg.delay
                  << " ps  |  Accum: " << accumulated << " ps" << std::endl;
    }
    std::cout << "==========================================\n" << std::endl;
}

9️⃣ main.cpp
cpp

#include "LibertyParser.h"
#include "PathAnalyzer.h"
#include "ConfigLoader.h"

int main() {
    std::string libFile = "/home/xulei/Lei_Develop/My_Sta_Lab/day1/simple.lib";

    LibertyParser parser(libFile);
    parser.deleteComments();
    parser.parseFileToBlock();

    PathAnalyzer analyzer(parser);

    AnalysisConfig config = ConfigLoader::loadFromFile("config.cfg");

    TimingPath result = analyzer.analyzeSetupTiming("MainPath", config);
    analyzer.printPathResult(result);

    return 0;
}

🔟 config.cfg
ini

# STA Configuration File
cell_name   = "AND2_X1"
input_pin   = "A"
output_pin  = "Y"
table_type  = "cell_rise"
transition  = 25.0
load        = 50.0
clock_period = 100.0
setup_require = 0.0

# 多级路径示例（取消注释即可启用）
# extra_segment = "BUF_X1,A,Y,cell_rise"
# extra_segment = "AND2_X1,A,Y,cell_fall"

🔹 编译命令
bash

g++ -std=c++17 -g \
    LibertyParser.cpp \
    PathAnalyzer.cpp \
    ConfigLoader.cpp \
    main.cpp \
    -o LibertyParser

# 或使用 Makefile（推荐）：
# make -j 4

🎯 预期输出
text

========================= Parsed Lib =========================
Cell:AND2_X1
{
        Pin: A, Direction: input, Capacitance: 1
        Pin: B, Direction: input, Capacitance: 1
        Pin: Y, Direction: output, Capacitance: 0, Function: A * B
                Timing Arc: related_pin=A, sense=positive_unate, type=
}
...

========== Timing Path Analysis ==========
Path Name     : MainPath
Clock Period  : 100.000 ps
Setup Require : 0.000 ps
Total Delay   : 65.000 ps
Slack         : 35.000 ps
Status        : ✅ MET

--- Path Detail (Incr / Accum) ---
  [0] AND2_X1 A -> Y  |  Incr: 65.000 ps  |  Accum: 65.000 ps
==========================================

所有代码已经完整交付，可以直接复制使用。如果遇到编译问题，请告诉我具体的错误信息。

🎯 预期输出
text

========================= Parsed Lib =========================
Cell:AND2_X1
{
        Pin: A, Direction: input, Capacitance: 1
        Pin: B, Direction: input, Capacitance: 1
        Pin: Y, Direction: output, Capacitance: 0, Function: A * B
                Timing Arc: related_pin=A, sense=positive_unate, type=
}
...

========== Timing Path Analysis ==========
Path Name     : MainPath
Clock Period  : 100.000 ps
Setup Require : 0.000 ps
Total Delay   : 65.000 ps
Slack         : 35.000 ps
Status        : ✅ MET

--- Path Detail (Incr / Accum) ---
  [0] AND2_X1 A -> Y  |  Incr: 65.000 ps  |  Accum: 65.000 ps
==========================================

所有代码已经完整交付，可以直接复制使用。如果遇到编译问题，请告诉我具体的错误信息。
