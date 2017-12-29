#ifndef __RECORD_H__
#define __RECORD_H__

#include <stddef.h>
#include <time.h>
#include <stdint.h>
#include <string.h>
#include <string>
#include <vector>

#define NAME_SIZE 32

struct Record {
    char name[4][NAME_SIZE];        // 选手姓名
    struct Detail {
        uint8_t win_flag;           // 和牌选手（0123bit）
        uint8_t claim_flag;         // 点炮选手（0123bit），当点炮选手=和牌选手时，为自摸
        uint16_t fan;               // 番数
        int16_t penalty_scores[4];  // 处罚分
        uint8_t packed_fan;         // 小番组合
        uint64_t fan_flag;          // 标记番种

        struct WinHand {
            char tiles[64];         // 牌
            uint8_t win_flag;       // 和牌标记
            uint8_t flower_count;   // 花牌数
        } win_hand;         // 和牌
    } detail[16];           // 每一盘的详情
    size_t current_index;   // 当前打到第几盘
    time_t start_time;      // 开始时间
    time_t end_time;        // 结束时间
};

#define SET_WIN_CLAIM(wc_, n_) ((wc_) |= (1 << (n_)))
#define TEST_WIN_CLAIM(wc_, n_) !!((wc_) & (1 << (n_)))

#define WIN_CLAIM_INDEX(wc_) (((wc_) & 0x8) ? 3 : ((wc_) & 0x4) ? 2 : ((wc_) & 0x2) ? 1 : ((wc_) & 0x1) ? 0 : -1)

#define SET_FAN(flag_, fan_) ((flag_) |= (1ULL << (mahjong::LAST_TILE - (fan_))))
#define RESET_FAN(flag_, fan_) ((flag_) &= ~(1ULL << (mahjong::LAST_TILE - (fan_))))
#define TEST_FAN(flag_, fan_) !!((flag_) & (1ULL << (mahjong::LAST_TILE - (fan_))))

void ReadRecordFromFile(const char *file, Record &record);
void WriteRecordToFile(const char *file, const Record &record);

void LoadHistoryRecords(const char *file, std::vector<Record> &records);
void SaveHistoryRecords(const char *file, const std::vector<Record> &records);
void ModifyRecordInHistory(std::vector<Record> &records, const Record *r);

void TranslateDetailToScoreTable(const Record::Detail &detail, int (&scoreTable)[4]);
void CalculateRankFromScore(const int (&scores)[4], unsigned (&ranks)[4]);
void RankToStandardScore(const unsigned (&ranks)[4], float (&ss)[4]);
void CompetitionScoreToStandardScore(const int (&cs)[4], float (&ss)[4]);

const char *GetShortFanText(const Record::Detail &detail);
std::string GetLongFanText(const Record::Detail &detail);

struct RecordsStatistic {
    size_t rank[4];
    float standard_score;
    int competition_score;
    uint16_t max_fan;
    size_t win;
    size_t self_drawn;
    size_t claim;
    size_t win_fan;
    size_t claim_fan;
};

void SummarizeRecords(const std::vector<int8_t> &flags, const std::vector<Record> &records, RecordsStatistic *result);

#endif
