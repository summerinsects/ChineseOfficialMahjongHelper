#ifndef __COMPETITION_H__
#define __COMPETITION_H__

#include <string>
#include <vector>
#include <utility>

// 成绩
struct CompetitionResult {
    unsigned rank;  // 顺位
    float standard_score;  // 标准分
    int competition_score;  // 比赛分

    static std::string getStandardScoreString(float ss);
};

class CompetitionTeam;

// 队员
class CompetitionPlayer {
public:
    unsigned serial;  // 编号
    std::string name;  // 姓名
    std::vector<CompetitionResult> competition_results;  // 每轮成绩
    CompetitionTeam *team;  // 所在队伍

    std::pair<float, int> getTotalScoresByRound(size_t round) const;
    std::pair<float, int> getCurrentScoresByRound(size_t round) const;
};

// 队伍
class CompetitionTeam {
public:
    unsigned serial;  // 编号
    std::string name;  // 队名
    std::vector<CompetitionPlayer *> player_indices;  // 队员
    std::vector<CompetitionResult> scores;  // 全队成绩
};

// 桌
struct CompetitionTable {
    unsigned serial;  // 编号
    CompetitionPlayer *players[4];  // 参赛选手
};

// 轮
class CompetitionRound {
public:
    std::vector<CompetitionTable> tables;  // 桌
};

class CompetitionData {
public:
    std::string name;  // 赛事名称
    std::vector<CompetitionPlayer> players;  // 参赛选手
    std::vector<CompetitionTeam> teams;  // 参赛队伍
    std::vector<CompetitionRound> round;  // 每一轮数据
    unsigned current_round;  // 当前轮数
};

#endif
