#ifndef __COMPETITION_H__
#define __COMPETITION_H__

#include <string>
#include <vector>

// 4 2 1 0
// 并列12：(4+2)/2=3
// 并列23：(2+1)/2=1.5
// 并列34：(1+0)/2=0.5
// 并列123：(4+2+1)/3=2.333
// 并列234：(2+1+0)/3=1
// 并列1234：(4+2+1+0)/4=1.75

enum RANK_TYPE {
    RANK_1, RANK_TIED_12, RANK_TIED_123, RANK_TIED_1234,
    RANK_2, RANK_TIED_23, RANK_TIED_234,
    RANK_3, RANK_TIED_34,
    RANK_4
};

// *12 使得所有分化为整数
static const unsigned standard_score_12[] = {
    48, 36, 28, 21,
    24, 18, 12,
    12, 6,
    0
};

class CompetitionPlayer {
public:
    unsigned serial;  // 编号
    std::string name;  // 姓名
    std::vector<RANK_TYPE> ranks;  // 每一轮的名次（用于标准分）
    std::vector<unsigned> scores;  // 比赛分
    size_t team_index;  // 队伍索引
};

class CompetitionTeam {
public:
    unsigned serial;  // 编号
    std::string name;  // 队名
    std::vector<size_t> player_indices;  // 队员
    std::vector<unsigned> standard_scores;  // 标准分
    std::vector<unsigned> scores;  // 比赛分
};

class CompetitionData {
public:
    std::string name;  // 赛事名称
    unsigned round_count;  // 总轮数
    unsigned current_round;  // 当前轮数
    std::vector<CompetitionPlayer> players;  // 参赛选手
    std::vector<CompetitionTeam> teams;  // 参赛队伍
};

struct CompetitionTable {
    unsigned serial;  // 编号
    size_t player_indices[4];  // 参赛选手
};

#endif
