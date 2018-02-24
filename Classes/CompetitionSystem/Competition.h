#ifndef __COMPETITION_H__
#define __COMPETITION_H__

#include <stddef.h>
#include <time.h>
#include <string>
#include <vector>
#include <utility>

#define INVALID_INDEX ((ptrdiff_t)-1)

// 成绩
struct CompetitionResult {
    unsigned rank = 0;  // 顺位，合法值为1 2 3 4
    unsigned standard_score12 = 0;  // 标准分
    int competition_score = 0;  // 比赛分

    static std::string standardScoreToString(unsigned ss12);  // 标准分转换为字符串
};

// 队员
class CompetitionPlayer {
public:
    size_t serial = 0;  // 编号
    std::string name;  // 姓名
    std::vector<CompetitionResult> competition_results;  // 每轮成绩
    ptrdiff_t team_index = INVALID_INDEX;  // 所在队伍

    std::pair<unsigned, int> getTotalScoresByRound(size_t round) const;  // 获取指定一轮总成绩
    std::pair<unsigned, int> getCurrentScoresByRound(size_t round) const;  // 获取指定一轮单轮成绩
};

// 队伍
class CompetitionTeam {
public:
    size_t serial = 0;  // 编号
    std::string name;  // 队名
    std::vector<ptrdiff_t> player_indices;  // 队员
};

// 桌
struct CompetitionTable {
    size_t serial = 0;  // 编号
    ptrdiff_t player_indices[4];  // 参赛选手

    CompetitionTable() {
        player_indices[0] = INVALID_INDEX;
        player_indices[1] = INVALID_INDEX;
        player_indices[2] = INVALID_INDEX;
        player_indices[3] = INVALID_INDEX;
    }
};

// 轮
class CompetitionRound {
public:
    std::vector<CompetitionTable> tables;  // 桌

    // 高高碰排序
    static void sortPlayers(size_t round, const std::vector<CompetitionPlayer> &players, std::vector<const CompetitionPlayer *> &output);
};

class CompetitionData {
public:
    std::string name;  // 赛事名称
    std::vector<CompetitionPlayer> players;  // 参赛选手
    std::vector<CompetitionTeam> teams;  // 参赛队伍
    std::vector<CompetitionRound> rounds;  // 每一轮数据
    size_t round_count = 0;  // 总轮数
    time_t start_time = 0;  // 开始时间
    time_t finish_time = 0;  // 结束时间

    std::string associated_file;
    void (*modify_callback)(const CompetitionData *) = nullptr;
    bool readFromFile();  // 从文件中读
    bool writeToFile() const;  // 写入到文件

    void prepare(const std::string &name, size_t player, size_t round);  // 准备

    bool isEnrollmentOver() const;  // 报名是否截止

    bool startNewRound();  // 开始新一轮
    bool isRoundStarted(size_t round) const;  // 一轮是否已经开始
    bool isRoundFinished(size_t round) const;  // 一轮是否已经结束

    void rankTablesBySerial(size_t round);  // 按编号排桌
    void rankTablesByRandom(size_t round);  // 随机排桌
    void rankTablesByScores(size_t round);  // 高高碰排桌
    void rankTablesBySnake(size_t round);  // 蛇形名次排桌
};

void LoadHistoryCompetitions(const char *file, std::vector<CompetitionData> &competitions);
void SaveHistoryCompetitions(const char *file, const std::vector<CompetitionData> &competitions);
void ModifyCompetitionInHistory(std::vector<CompetitionData> &competitions, const CompetitionData *data);

#endif
