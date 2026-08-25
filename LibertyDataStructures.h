#ifndef LIBERTY_DATA_STRUCTURES_H
#define LIBERTY_DATA_STRUCTURES_H

#include <vector> 
#include <string>

// 用于存储 一个用于查找表的 数据
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
    double capacitance;
    bool is_clock;
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

// Path Analysis
struct PathSegment {
    std::string cellName;   // Cell 名称，如 "AND2_X1"z
    std::string fromPin;    // 输入引脚，如 "A"    
    std::string toPin;      // 输出引脚，如 "Y"    
    std::string tableType;  // 表类型："cell_rise" 或 "cell_fall"
    double transition;      // 该段输入转换时间 (ps)    
    double load;            // 该段输出负载电容 (fF)    
    double delay;           // 计算后填写的延迟值 (ps) —— 用于打印明细    
};

enum class TimingCheckType {
    SETUP = 1,
    HOLD
};

struct TotalTimingPath {
    std::string pathName; //当前的整个路径名称
    std::vector<PathSegment> segments; // 路径上的所有段
    double totalDelay; // 总延迟 (ps) —— 用于打印明细
    double clockPeriod; // 时钟周期 (ps) —— 用于打印明细
    double setupRequiredTime; // 建立时序约束要求 (ps) —— 用于打印明细
    double holdRequiredTime; // 保持时序约束要求 (ps) —— 用于打印明细
    TimingCheckType checkType; // 时序检查类型 (SETUP 或 HOLD) —— 用于打印明细
    double slack; // 时间裕度 (ps)
    bool isMet; // 是否满足"建立/保持"时序约束要求
};


#endif