#include "pathAnalyzer.h"
#include <iostream>
#include <iomanip>

// =====================================================================================================================
PathAnalyzer::PathAnalyzer(LibertyParser& libParser) : m_libParser(libParser), m_cells(libParser.getCells()) {} 

// =====================================================================================================================
std::vector<PathSegment> PathAnalyzer::loadConfigFromFileToSegments(const AnalysisConfig& config)
{
    std::vector<PathSegment> segmentsArray;
    PathSegment mainSeg;
    mainSeg.cellName = config.cellName;
    mainSeg.cellName = config.cellName;
    mainSeg.fromPin = config.inputPinName;
    mainSeg.toPin = config.outputPinName;
    mainSeg.tableType = config.tableType;
    mainSeg.transition = config.transitionValue;
    mainSeg.load = config.loadValue;

    segmentsArray.push_back(mainSeg);

    for(const auto& extra : config.extraSegPaths) {
        PathSegment seg;

        mainSeg.cellName = std::get<0>(extra);
        mainSeg.fromPin = std::get<1>(extra);
        mainSeg.toPin = std::get<2>(extra);
        mainSeg.tableType = std::get<3>(extra);
        mainSeg.transition = config.transitionValue;
        mainSeg.load = config.loadValue;

        segmentsArray.push_back(seg);
    }

    return segmentsArray;
}

// =====================================================================================================================
double PathAnalyzer::accumulateSegAndTotalPathDelay(std::vector<PathSegment>& segments)
{
    if(segments.empty()) return 0.0;
    double totalDelay =  0.0;
    double currentTransition = segments[0].transition;
    TimingArc* targetArc = nullptr;

    for(const auto& seg : segments) {
        std::string cellName = seg.cellName;
        auto cellIt = m_cells.find(cellName);

        if(cellIt == m_cells.end()) {
            std::cerr << "Error: Cell '" << cellName << "' not found in Liberty data." << std::endl;
            continue;
        }
        
        Cell cell = cellIt->second;

        for(const auto& pin : cell.pins) {
            if(pin.name == seg.toPin) {
                for(auto& arc : pin.timing_arcs) {
                    if(arc.related_pin == seg.fromPin) {
                        targetArc = const_cast<TimingArc*>(&arc);
                        break;
                    }
                }
                break;
            }  
        }

        if (targetArc == nullptr) {
            std::cerr << "Error: No timing arc found for cell '" << cellName << "' from pin '" << seg.fromPin << "' to pin '" << seg.toPin << "'." << std::endl;
            continue;
        }

        LookupTable delayTable;
        if(seg.tableType == "cell_rise") {
            delayTable = (targetArc->cell_rise);
        } else if(seg.tableType == "cell_fall") {
            delayTable = (targetArc->cell_fall);
        } else {
            std::cerr << "Error: Unknown table type '" << seg.tableType << "' for cell '" << cellName << "'." << std::endl;
            continue;
        }

        double segDelay = m_libParser.interpolateDelay(delayTable, currentTransition, seg.load);
        totalDelay += segDelay;

        LookupTable slewTable;
        if(seg.tableType == "cell_rise") {
            slewTable = (targetArc->rise_transition);
        } else if(seg.tableType == "cell_fall") {
            slewTable = (targetArc->fall_transition);
        }

        double outputSlew = m_libParser.interpolateDelay(slewTable, currentTransition, seg.load);
        currentTransition = outputSlew;
    }
    
    return totalDelay;
}

// =====================================================================================================================
TotalTimingPath PathAnalyzer::analyzeTiming(const std::string& currentPathName,
                                          const AnalysisConfig& config,
                                          TimingCheckType checkType)
{
    TotalTimingPath pathResult;
    auto segPath = loadConfigFromFileToSegments(config);
    pathResult.checkType = checkType;
    pathResult.pathName = currentPathName;
    pathResult.clockPeriod = config.clockPeriod;
    pathResult.setupRequiredTime = config.setupRequiredTime;
    pathResult.holdRequiredTime = config.holdRequiredTime;
    pathResult.totalDelay = accumulateSegAndTotalPathDelay(segPath);
    pathResult.segments = segPath;

    double clockSkew = config.captureClockDelay - config.launchClockDelay;

    if(checkType == TimingCheckType::SETUP) {
        pathResult.slack = pathResult.clockPeriod - pathResult.totalDelay - pathResult.setupRequiredTime + clockSkew;
        pathResult.isMet = (pathResult.slack >= 0.0);
    } else if(checkType == TimingCheckType::HOLD) {
        pathResult.slack = pathResult.totalDelay - pathResult.holdRequiredTime - clockSkew;
        pathResult.isMet = (pathResult.slack >= 0.0);
    }
    
    return pathResult;
}

// =====================================================================================================================
void PathAnalyzer::printPathResult(const TotalTimingPath& path, TimingCheckType checkType)
{
    std::cout << "\n============ Timing path analysis ============" << std::endl;
    std::cout << "Path Name" << path.pathName << std::endl;
    
    std::cout << "Clock Period" << path.clockPeriod << std::endl;
    if(checkType == TimingCheckType::SETUP) {
        std::cout << "Check Type" << "Setup"  << std::endl;
        std::cout << "Setup Required Time" << path.setupRequiredTime << std::endl;
        std::cout << "Setup Slack" << path.slack << std::endl;
        std::cout << "Setup is Met" << (path.isMet ? "Yes" : "No") << std::endl;
    } else if(checkType == TimingCheckType::HOLD) {
        std::cout << "Check Type" << "Hold" << std::endl;
        std::cout << "Hold Required Time" << path.holdRequiredTime << std::endl;
        std::cout << "Hold Slack" << path.slack << std::endl;
        std::cout << "Hold is Met" << (path.isMet ? "Yes" : "No") << std::endl;
    }
    std::cout << "Total Delay" << path.totalDelay << std::endl;

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
    std::cout << "================================================" << std::endl;
}
