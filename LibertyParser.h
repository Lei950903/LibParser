#ifndef LIBERTY_PARSER_H
#define LIBERTY_PARSER_H

#include <unordered_map>
#include <fstream> 
#include <iostream>
#include <sstream> // For std::stringstream
#include "LibertyDataStructures.h"

// 全局存储

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
    LibertyParser(const std::string fileName);
    ~LibertyParser();

    void deleteComments();
    std::ifstream& getFileStream() { return m_fileStream; }
    void parseFileToBlock();// 开始结构化解析
    std::string extractBlockName(const std::string& currentLine);
    std::string getAttrValue(const std::string& line);
    std::string trimString(const std::string& str);
    std::pair<std::string, std::string> splitStringToPair(const std::string& originString, const std::string& splitBaseStr);
    std::vector<std::vector<double>> parseValuesToMatrix(const std::string& valuesString);
    double interpolateDelay(const LookupTable& table, double x, double y);
    const char* blockNameToString(BlockName name);

    const std::unordered_map<std::string, Cell>& getCells() const{ return m_cells; }

private:
    std::ifstream m_fileStream;
    std::vector<std::string> m_parsedFile;
    std::unordered_map<std::string, Template> m_templateDefs;
    std::unordered_map<std::string, Cell> m_cells; // cells map
    std::vector<Pin> m_pins;
    std::vector<TimingArc> m_timingArcs;
    std::unordered_map<TableType, LookupTable>  m_lookTables; // tables map
    std::vector<BlockName> m_blockStack;  // current block stack: LIBRARY, CELL, PIN, TIMING, FF, TABLE
};


#endif