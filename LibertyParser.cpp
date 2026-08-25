#include "LibertyParser.h"

// =====================================================================================================================
LibertyParser::LibertyParser(const std::string fileName){

    m_fileStream.open(fileName);
}

// =====================================================================================================================
LibertyParser::~LibertyParser(){}

// =====================================================================================================================
void LibertyParser::deleteComments() {
    std::string line;
    bool isInBlockComment = false;   // whether inside /* */

    while (std::getline(m_fileStream, line)) {
        // ---- Handle multi-line block comments ----
        if (isInBlockComment) {
            // Look for the end marker */
            size_t endPos = line.find("*/");
            if (endPos != std::string::npos) {
                // Found the end, keep the part after it
                line = line.substr(endPos + 2);  // skip "*/"
                isInBlockComment = false;
                // line may now be empty or only whitespace, continue processing
            }
            else {
                // End not found, this whole line is comment
                continue;
            }
        }

        // ---- Handle line comments (//), highest priority ----
        size_t commentPos = line.find("//");
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);  // keep everything before the comment
        }

        // ---- Handle inline block comments (/* */) ----
        while (true) {
            size_t blockCommentStart = line.find("/*");
            if (blockCommentStart == std::string::npos) {
                break;
            }

            // Look for the closing */
            size_t blockCommentEnd = line.find("*/", blockCommentStart + 2);
            if (blockCommentEnd != std::string::npos) {
                // Closed on the same line, erase it
                line.erase(blockCommentStart, blockCommentEnd - blockCommentStart + 2);
            }
            else {
                // Not closed, so it spans multiple lines
                isInBlockComment = true;
                line.erase(blockCommentStart);  // remove from start to end of line
                break;
            }
        }

        // Skip lines that are empty or only whitespace
        if(line.empty() || line.find_first_not_of(" \t") == std::string::npos) {
            continue;
        }

        m_parsedFile.push_back(line);
        std::cout << line << std::endl;
    }
}

// =====================================================================================================================
void LibertyParser::parseFileToBlock() {
    // Structurally parse the lib file, storing it block by block
    std::string pendingLine; // leftover content to be treated as a new line
    bool isUsePending = false;
    std::string currentLine; // the actual current line when iterating

    std::string currentLibName;
    std::string currentTemplName;
    std::string currentCellName;
    std::string currentPinName;
    std::string currentTableTemplate;
    std::pair<std::string, std::string> splitedStringPair;
    TableType currentTableType;

    // Use while instead of for because after a '{' there may be leftover content
    // that should be processed as a new line rather than reading the next line.
    size_t lineIndex = 0;
    while(lineIndex < m_parsedFile.size()) {
        // Check if we need to process leftover from a '{'
        if(isUsePending == true) {
            currentLine = pendingLine;
            isUsePending = false;
        }
        else{
            currentLine = m_parsedFile[lineIndex];
            lineIndex ++;
        }

        // Start structured parsing according to:
        // lu_table_template block, cell block -> { pin block -> (timing block), FF block }, and their attributes.
        // Check if it's a block start, containing '{'
        if(currentLine.find('{') != std::string::npos) {
            // Each block has its own type
            // Handle block name, type, and initialization
            if(currentLine.find("library") != std::string::npos) {
                m_blockStack.push_back(BlockName::LIBRARY);
                currentLibName = extractBlockName(currentLine);
            }
            else if(currentLine.find("lu_table_template") != std::string::npos) {
                m_blockStack.push_back(BlockName::TEMPLATE);
                currentTemplName = extractBlockName(currentLine);
                m_templateDefs[currentTemplName] = Template{}; // placeholder
            }
            else if(currentLine.find("cell_") == std::string::npos && currentLine.find("cell") != std::string::npos) {
                m_blockStack.push_back(BlockName::CELL);
                currentCellName = extractBlockName(currentLine);
                m_cells[currentCellName] =  Cell{currentCellName, {}, 0.0, 0.0};
            }
            else if(currentLine.find("pin") != std::string::npos) {
                m_blockStack.push_back(BlockName::PIN);
                currentPinName = extractBlockName(currentLine);
                // Since a cell has a limited number of pins and order matters,
                // vector is used instead of map; map lookup would be overkill.
                m_pins.push_back(Pin{currentPinName, "", 0, false, "", {} });
            }
            else if(currentLine.find("timing()") != std::string::npos) {
                m_blockStack.push_back(BlockName::TIMING);
                m_timingArcs.push_back(TimingArc{});
            }

            else if(currentLine.find("FF") != std::string::npos) {
                m_blockStack.push_back(BlockName::FF);
                // FF block is currently deleted from the input lib.
            }
            else if(currentLine.find("cell_rise") != std::string::npos ||
                    currentLine.find("cell_fall") != std::string::npos ||
                    currentLine.find("rise_transition") != std::string::npos ||
                    currentLine.find("fall_transition") != std::string::npos
                    ) {

                m_blockStack.push_back(BlockName::TABLE);

                if(currentLine.find("cell_rise") != std::string::npos) {
                    // Create a new LookupTable for "cell_rise" in the m_lookTables map
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
                // Store the template name for the current table block
                currentTableTemplate = extractBlockName(currentLine);
            }

            // Process leftover content after the '{' on the same line
            size_t bracePos = currentLine.find('{');
            std::string rest = currentLine.substr(bracePos +1);
            while(!rest.empty() && (rest.front() == ' ' || rest.front() == '\t')) {
                rest.erase(0, 1);
            }
            if(!rest.empty()) {
                pendingLine = rest;
                isUsePending = true;
            }
        }
        // -- Normal attribute line -- Parse attributes inside a block.
        // This must be placed before '}' handling so that attributes are not missed.
        else if(!m_blockStack.empty() && trimString(currentLine) != "}") {
            if(currentLine.find('}') !=std::string::npos) {
                pendingLine = '}';
                isUsePending = true;
            }
            
            // Get the last element of the stack, which is the current block
            BlockName top = m_blockStack.back();

            if(top == BlockName::LIBRARY) {
                // Library name is already stored; other global attributes can be handled here
            }
            else if(top == BlockName::TEMPLATE) {
                std::string splitBaseStr      = ";";
                splitedStringPair             = splitStringToPair(currentLine, splitBaseStr);
                std::string currentAttrString = splitedStringPair.first;
                std::string rest              = splitedStringPair.second;

                if(!rest.empty()) {
                    pendingLine = rest;
                    isUsePending = true;
                }

                if(currentAttrString.find("variable_1") != std::string::npos) {
                    std::string splitBaseStr    = ":";
                    splitedStringPair           = splitStringToPair(currentLine, splitBaseStr);
                    m_templateDefs[currentTemplName].variable_1 = splitedStringPair.second;
                }
                else if(currentAttrString.find("variable_2") != std::string::npos) {
                    std::string splitBaseStr    = ":";
                    splitedStringPair           = splitStringToPair(currentLine, splitBaseStr);
                    m_templateDefs[currentTemplName].variable_2 = splitedStringPair.second;
                }
                else if(currentAttrString.find("index_1") != std::string::npos) {
                        std::vector<std::vector<double>> matrix1 = parseValuesToMatrix(currentAttrString);
                        m_templateDefs[currentTemplName].index1 = matrix1.back();
                }
                else if(currentAttrString.find("index_2") != std::string::npos) {
                        std::vector<std::vector<double>> matrix2 = parseValuesToMatrix(currentAttrString);
                        m_templateDefs[currentTemplName].index2 = matrix2.back();
                }
            }
            else if(top == BlockName::CELL) {
                // Cell-level attributes (if any)
            }
            else if(top == BlockName::PIN) {
                // Handle multiple attributes on the same line separated by ';' in PIN block.
                std::string splitBaseStr      = ";";
                splitedStringPair             = splitStringToPair(currentLine, splitBaseStr);
                std::string currentAttrString = splitedStringPair.first;
                std::string rest              = splitedStringPair.second;

                if(!rest.empty()) {
                    pendingLine = rest;
                    isUsePending = true;
                }
                // Now process the single attribute
                if(currentAttrString.find("direction") != std::string::npos) {
                    m_pins.back().direction = getAttrValue(currentAttrString);
                }
                
                else if(currentAttrString.find("capacitance") != std::string::npos) {
                    try {
                            m_pins.back().capacitance = std::stod(getAttrValue(currentAttrString));
                    }
                    catch(const std::exception& e) {
                        std::cout << "Error converting capacitance to double for pin " << currentPinName << ": " << e.what() << std::endl;
                    }
                }
                else if(currentAttrString.find("clock") != std::string::npos) {
                    std::string clockValue = getAttrValue(currentAttrString);
                    if(clockValue == "true") {
                        m_pins.back().is_clock = true;
                    }
                    else {
                        m_pins.back().is_clock = false;
                    }
                }
                else if(currentAttrString.find("function") != std::string::npos) {
                    m_pins.back().function = getAttrValue(currentAttrString);
                }
            }
            else if(top == BlockName::TIMING) {
                // Handle multiple attributes on the same line separated by ';' in TIMING block.
                // But NOT handle multiple attributes on the same line separated by '}'.
                std::string splitBaseStr      = ";";
                splitedStringPair             = splitStringToPair(currentLine, splitBaseStr);
                std::string currentAttrString = splitedStringPair.first;
                std::string rest              = splitedStringPair.second;
                // Handle multiple attributes on the same line separated by ';'
                if(!rest.empty()) {
                    pendingLine = rest;
                    isUsePending = true;
                }
                if (currentAttrString.find("related_pin") != std::string::npos) {
                    std::string relatedPinName = getAttrValue(currentAttrString);
                    m_timingArcs.back().related_pin = relatedPinName;
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
                    std::vector<std::vector<double>> matrix = parseValuesToMatrix(currentLine);

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
        
        // Check if it's a block end, containing '}'
        else if(currentLine.find('}') != std::string::npos) {
            // Block end handling (pop stack, save data if needed)
            if(!m_blockStack.empty()) {
                BlockName topName = m_blockStack.back();
                // TABLE
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

                // TIMING
                else if(topName == BlockName::TIMING) {
                    m_pins.back().timing_arcs.push_back(m_timingArcs.back());
                }
                // PIN
                else if(topName == BlockName::PIN) {
                    m_cells[currentCellName].pins.push_back(m_pins.back());
                }
            }

            m_blockStack.pop_back(); // pop the block stack
        }
        
    }
    // Print parsed results for verification
    std::cout << "========================= Parsed Lib =========================";

    for(const auto & [cellName, cell] : m_cells) {
        std::cout << "\nCell:" << cellName << std::endl;
        std::cout << "{" << std::endl;
        std::cout << "\tSetup: " << cell.setup <<", " << " Hold: " << cell.hold << std::endl;
        for (const auto & pin : cell.pins) {
            std::cout << " \tPin: " << pin.name << ", " 
                        << " Direction: " << pin.direction  << ", "
                        << " Capacitance: " << pin.capacitance;
                      
            if(pin.is_clock == true) {
                std::cout << ", "<< " Is Clock: " << "true";
            }
            if(pin.function != "") {
                std::cout << ", "<< " Function: " << pin.function;
            }
            std::cout << std::endl;
            for (const auto & timingArc : pin.timing_arcs) {
                std::cout << "\t{\n\t\tTiming Arc:" << std::endl;
                std::cout << "\t\t{\n\t\t\tRelated Pin: " << timingArc.related_pin;
                if(timingArc.timing_sense != "") {
                    std::cout << ",  Timing Sense: " << timingArc.timing_sense;
                }
                if(timingArc.timing_type != "") {
                    std::cout << ",  Timing Type: " << timingArc.timing_type;
                }
                          
                std::cout<< std::endl;

                if (!timingArc.cell_rise.values.empty()) {
                    std::cout << "\t\t\tCell Rise Values: " << std::endl;
                    std::cout << "\t\t\t{" << std::endl;
                    for (const auto & row : timingArc.cell_rise.values) {
                        std::cout << "\t\t\t\t";
                        for (const auto & val : row) {                            
                             std::cout<< val << " ";
                        }
                        std::cout << std::endl;
                    }
                    std::cout << "\t\t\t}" << std::endl ;
                }
                if (!timingArc.cell_fall.values.empty()) {
                    std::cout << "\t\t\tCell Fall Values: " << std::endl;
                    std::cout << "\t\t\t{" << std::endl;
                    for (const auto & row : timingArc.cell_fall.values) {
                        std::cout << "\t\t\t\t";
                        for (const auto & val : row) {
                            std::cout << val << " ";
                        }
                        std::cout << std::endl;
                    }
                    std::cout << "\t\t\t}" << std::endl ;
                }
                if (!timingArc.rise_transition.values.empty()) {
                    std::cout << "\t\t\tRise Transition Values: " << std::endl;
                    std::cout << "\t\t\t{" << std::endl;
                    for (const auto & row : timingArc.rise_transition.values) {
                        std::cout << "\t\t\t\t";
                        for (const auto & val : row) {
                            std::cout << val << " ";
                        }
                        std::cout << std::endl;
                    }
                    std::cout << "\t\t\t}" << std::endl ;
                }
                if (!timingArc.fall_transition.values.empty()) {
                    std::cout << "\t\t\tFall Transition Values: " << std::endl;
                    std::cout << "\t\t\t{" << std::endl;
                    for (const auto & row : timingArc.fall_transition.values) {
                        std::cout << "\t\t\t\t";
                        for (const auto & val : row) {
                            std::cout << val << " ";
                        }
                        std::cout << std::endl;
                    }
                    std::cout << "\t\t\t}" << std::endl ;
                }
            }
        }
        std::cout << "\n\t}" << std::endl;
    }
    std::cout << "\n}" << std::endl;
}

// =====================================================================================================================
std::vector<std::vector<double>> LibertyParser::parseValuesToMatrix(const std::string& valuesString) {
    std::vector<std::vector<double>> matrix;
    size_t startPos = valuesString.find('(');
    size_t endPos   = valuesString.find(')');
    if (startPos == std::string::npos || endPos == std::string::npos || endPos <= startPos) {
        std::cerr << "Error: Invalid values string format: " << valuesString << std::endl;
        return matrix; // Return empty matrix on error
    }
    std::string valuesContent = valuesString.substr(startPos + 1, endPos - startPos - 1);
    std::stringstream valueStream(valuesContent);
    std::string segmentString;
    while(std::getline(valueStream, segmentString, '\"')) {
        if(segmentString.empty() || segmentString.find_first_of(" ,\t") == std::string::npos) {
            continue; // Skip empty segments or segments without numbers
        }
        std::vector<double> row;
        std::stringstream stream(segmentString);
        std::string numberString;
        while(std::getline(stream, numberString, ',')) {
            numberString.erase(0, numberString.find_first_not_of(" \t")); // Trim leading whitespace
            numberString.erase(numberString.find_last_not_of(" \t") + 1); 
            if(!numberString.empty()) {
                row.push_back(std::stod(numberString));
            }
        }
        if(!row.empty()) {
            matrix.push_back(row);
        }   
    }

    return matrix;
}

// =====================================================================================================================
std::pair<std::string, std::string> LibertyParser::splitStringToPair(const std::string& originString, const std::string& splitBaseStr) {    
    std::string firstString;
    std::string secondString;
    size_t pos = originString.find(splitBaseStr);
    if(pos != std::string::npos){
        size_t length            = pos - 0;
        firstString  = originString.substr(0, length);
        secondString = originString.substr(length + splitBaseStr.length());
        firstString = trimString(firstString);
        secondString = trimString(secondString);
        return {firstString, secondString};
    }
    

    return {trimString(originString), ""};
}

// =====================================================================================================================
std::string LibertyParser::getAttrValue(const std::string& line) {
    std::string splitBaseStr = ":";
    std::pair<std::string, std::string> splitedStringPair = splitStringToPair(line, splitBaseStr);
    std::string currentAttrString = splitedStringPair.first;
    std::string rest              = splitedStringPair.second;
    rest = trimString(rest);

    return rest;
}

// =====================================================================================================================
// Remove leading/trailing whitespace (spaces, tabs) and leading/trailing double or single quotes
std::string LibertyParser::trimString(const std::string& str) {
    std::string result = str;

    // 1. Remove leading whitespace
    while (!result.empty() && (result.front() == ' ' || result.front() == '\t'))
        result.erase(0, 1);
    // 2. Remove trailing whitespace
    while (!result.empty() && (result.back() == ' ' || result.back() == '\t'))
        result.pop_back();

    // 3. Remove leading/trailing double or single quotes
    if (!result.empty() && (result.front() == '\"' || result.front() == '\''))
        result.erase(0, 1);
    if (!result.empty() && (result.back() == '\"' || result.back() == '\''))
        result.pop_back();

    // 4. Remove any whitespace that may have been exposed after quote removal
    while (!result.empty() && (result.front() == ' ' || result.front() == '\t'))
        result.erase(0, 1);
    while (!result.empty() && (result.back() == ' ' || result.back() == '\t'))
        result.pop_back();

    return result;
}

// =====================================================================================================================
std::string LibertyParser::extractBlockName(const std::string& currentLine) {
    size_t start = currentLine.find('(') +1;
    size_t end   = currentLine.find(')', start);

    // Get the raw name
    std::string rawName = currentLine.substr(start, end - start);
    rawName = trimString(rawName);

    return rawName;
}

// ===================================================================================================================== // 暂时用不上
const char* LibertyParser::blockNameToString(BlockName name) {
    switch (name) {
        case BlockName::LIBRARY:  return "LIBRARY";
        case BlockName::TEMPLATE: return "TEMPLATE";
        case BlockName::CELL:     return "CELL";
        case BlockName::PIN:      return "PIN";
        case BlockName::TIMING:   return "TIMING";
        case BlockName::FF:       return "FF";
        case BlockName::TABLE:    return "TABLE";
        default:                  return "UNKNOWN";
    }
}

// =====================================================================================================================
double LibertyParser::interpolateDelay(const LookupTable& table, double x, double y)
{
    // 这里把table.index_1.empty() == true 换成table.index_1.size() < 2, 是因为在多数情况下都是至少2个值，所以一个值的情况就不在这里处理了。
    if (table.index1.size() < 2 || table.index2.size() < 2 || table.values.empty()) {
        std::cout << "The LookupTable is wrong!" << std::endl; 
        return 0.0;
    }
    const auto& index_X = table.index1;
    const auto& index_Y = table.index2;
    const auto& values = table.values;

    size_t sizeIndex_X = index_X.size();
    size_t sizeIndex_Y = index_Y.size();

    size_t x_idx; // 这是X左边的索引点
    if(x < index_X.front()) {
        x_idx = 0;
    } else if (x > index_X.back()) {
        x_idx = sizeIndex_X -2 ;
    }
    else {
        // The "lower_bound" function returns an iterator pointing to the first element greater than or equal to x.
        auto it = std::lower_bound(index_X.begin(), index_X.end(), x);
        x_idx = std::distance(index_X.begin(), it) -1;
    }
    
    // 在极端情况下，x_idx 可能是大于有边界的索引，所以需要限制。
    if(x_idx >=  table.index1.size() -1) {
        x_idx = table.index1.size() -2;
    }

    double x1 = table.index1[x_idx];
    double x2 = table.index1[x_idx +1];

    size_t y_idx; // 这是Y左边的索引点
    if(y < index_Y.front()) {
        y_idx = 0;
    } else if (y > index_Y.back()) {
        y_idx = sizeIndex_Y - 2 ;
    }
    else {
        // The "lower_bound" function returns an iterator pointing to the first element greater than or equal to x.
        auto it = std::lower_bound(index_Y.begin(), index_Y.end(), y);
        y_idx = std::distance(index_Y.begin(), it) -1;
    }
    
    // 在极端情况下，y_idx 可能是大于有边界的索引，所以需要限制。
    if(y_idx >=  table.index2.size() -1) {
        y_idx = table.index2.size() -2;
    }

    double y1 = table.index2[y_idx];
    double y2 = table.index2[y_idx +1];

    double dx = (x2 - x1 == 0) ? 0 : (x - x1)/(x2-x1);
    double dy = (y2 - y1 == 0) ? 0 : (y - y1)/(y2 -y1);

    double value11 = values[x_idx][y_idx];
    double value12 = values[x_idx][y_idx +1];
    double value21 = values[x_idx +1][y_idx];
    double value22 = values[x_idx+1][y_idx +1];

    double result = value11 * (1 - dx) * (1 - dy) + 
                    value12 * (1 - dx) * dy +
                    value21 * dx * (1 - dy) +
                    value22 * dx * dy;

    return result;
}
