#ifndef __COMPETITION_H__
#define __COMPETITION_H__

#include <string>
#include <vector>
#include <utility>

#include "json/document.h"

#define INVALID_INDEX ((ptrdiff_t)-1)

// 成绩
struct CompetitionResult {
    unsigned rank = 0;  // 顺位
    float standard_score = 0;  // 标准分
    int competition_score = 0;  // 比赛分

    static std::string getStandardScoreString(float ss);

    static void fromJson(const rapidjson::Value &json, CompetitionResult &result);
    static void toJson(const CompetitionResult &result, rapidjson::Value &json, rapidjson::Value::AllocatorType &alloc);
};

// 队员
class CompetitionPlayer {
public:
    unsigned serial = 0;  // 编号
    std::string name;  // 姓名
    std::vector<CompetitionResult> competition_results;  // 每轮成绩
    ptrdiff_t team_index = INVALID_INDEX;  // 所在队伍

    std::pair<float, int> getTotalScoresByRound(size_t round) const;
    std::pair<float, int> getCurrentScoresByRound(size_t round) const;

    static void fromJson(const rapidjson::Value &json, CompetitionPlayer &player);
    static void toJson(const CompetitionPlayer &player, rapidjson::Value &json, rapidjson::Value::AllocatorType &alloc);
};

// 队伍
class CompetitionTeam {
public:
    unsigned serial = 0;  // 编号
    std::string name;  // 队名
    std::vector<ptrdiff_t> player_indices;  // 队员
    std::vector<CompetitionResult> scores;  // 全队成绩

    static void fromJson(const rapidjson::Value &json, CompetitionTeam &team);
    static void toJson(const CompetitionTeam &team, rapidjson::Value &json, rapidjson::Value::AllocatorType &alloc);
};

// 桌
struct CompetitionTable {
    unsigned serial = 0;  // 编号
    ptrdiff_t player_indices[4] = { INVALID_INDEX, INVALID_INDEX, INVALID_INDEX, INVALID_INDEX };  // 参赛选手

    static void fromJson(const rapidjson::Value &json, CompetitionTable &table);
    static void toJson(const CompetitionTable &table, rapidjson::Value &json, rapidjson::Value::AllocatorType &alloc);
};

// 轮
class CompetitionRound {
public:
    std::vector<CompetitionTable> tables;  // 桌

    static void fromJson(const rapidjson::Value &json, CompetitionRound &round);
    static void toJson(const CompetitionRound &round, rapidjson::Value &json, rapidjson::Value::AllocatorType &alloc);
};

class CompetitionData {
public:
    std::string name;  // 赛事名称
    std::vector<CompetitionPlayer> players;  // 参赛选手
    std::vector<CompetitionTeam> teams;  // 参赛队伍
    std::vector<CompetitionRound> rounds;  // 每一轮数据
    unsigned current_round = 0;  // 当前轮数

    static void fromJson(const rapidjson::Value &json, CompetitionData &data);
    static void toJson(const CompetitionData &data, rapidjson::Value &json, rapidjson::Value::AllocatorType &alloc);
};

#endif
