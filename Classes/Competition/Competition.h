#ifndef __COMPETITION_H__
#define __COMPETITION_H__

#include <stddef.h>
#include <time.h>
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

    static std::string standardScoreToString(float ss);  // 标准分转换为字符串

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

    std::pair<float, int> getTotalScoresByRound(size_t round) const;  // 获取指定一轮总成绩
    std::pair<float, int> getCurrentScoresByRound(size_t round) const;  // 获取指定一轮单轮成绩

    static void fromJson(const rapidjson::Value &json, CompetitionPlayer &player);
    static void toJson(const CompetitionPlayer &player, rapidjson::Value &json, rapidjson::Value::AllocatorType &alloc);
};

// 队伍
class CompetitionTeam {
public:
    unsigned serial = 0;  // 编号
    std::string name;  // 队名
    std::vector<ptrdiff_t> player_indices;  // 队员

    static void fromJson(const rapidjson::Value &json, CompetitionTeam &team);
    static void toJson(const CompetitionTeam &team, rapidjson::Value &json, rapidjson::Value::AllocatorType &alloc);
};

// 桌
struct CompetitionTable {
    unsigned serial = 0;  // 编号
    ptrdiff_t player_indices[4];  // 参赛选手

    CompetitionTable() {
        player_indices[0] = INVALID_INDEX;
        player_indices[1] = INVALID_INDEX;
        player_indices[2] = INVALID_INDEX;
        player_indices[3] = INVALID_INDEX;
    }

    static void fromJson(const rapidjson::Value &json, CompetitionTable &table);
    static void toJson(const CompetitionTable &table, rapidjson::Value &json, rapidjson::Value::AllocatorType &alloc);
};

// 轮
class CompetitionRound {
public:
    std::vector<CompetitionTable> tables;  // 桌

    static void fromJson(const rapidjson::Value &json, CompetitionRound &round);
    static void toJson(const CompetitionRound &round, rapidjson::Value &json, rapidjson::Value::AllocatorType &alloc);

    // 高高碰排序
    static void sortPlayers(unsigned round, const std::vector<CompetitionPlayer> &players, std::vector<const CompetitionPlayer *> &output);
};

class CompetitionData {
public:
    std::string name;  // 赛事名称
    std::vector<CompetitionPlayer> players;  // 参赛选手
    std::vector<CompetitionTeam> teams;  // 参赛队伍
    std::vector<CompetitionRound> rounds;  // 每一轮数据
    unsigned current_round = 0;  // 当前轮数
    time_t start_time = 0;  // 开始时间
    time_t finish_time = 0;  // 结束时间

    static void fromJson(const rapidjson::Value &json, CompetitionData &data);
    static void toJson(const CompetitionData &data, rapidjson::Value &json, rapidjson::Value::AllocatorType &alloc);

    bool readFromFile(const std::string &file);
    bool writeToFile(const std::string &file) const;

    void prepare();  // 准备

    bool isRegistrationFull() const;  // 报名是否已满
    bool isRoundStarted(unsigned round) const;
};

#endif
