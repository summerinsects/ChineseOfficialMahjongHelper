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
        uint64_t fan_flag;          // 标记番种
        uint16_t unique_fan;        // 只存在一个的番14个：箭刻、圈风刻、门风刻、门前清、平和、双暗刻、暗杠、断幺、明杠、无字、边张、嵌张、单钓将、自摸
        uint64_t multiple_fan;      // 可复计的番9个(每个番占用4bit)：四归一(3)、双同刻(2)、一般高(2)、喜相逢(2)、连六(2)、老少副(2)、幺九刻(4)、缺一门(3)、花牌(8)

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

#define UNIQUE_FAN_DRAGON_PUNG          0
#define UNIQUE_FAN_PREVALENT_WIND       1
#define UNIQUE_FAN_SEAT_WIND            2
#define UNIQUE_FAN_CONCEALED_HAND       3
#define UNIQUE_FAN_ALL_CHOWS            4
#define UNIQUE_FAN_TWO_CONCEALED_PUNGS  5
#define UNIQUE_FAN_CONCEALED_KONG       6
#define UNIQUE_FAN_ALL_SIMPLES          7
#define UNIQUE_FAN_MELDED_KONG          8
#define UNIQUE_FAN_NO_HONORS            9
#define UNIQUE_FAN_EDGE_WAIT            10
#define UNIQUE_FAN_CLOSED_WAIT          11
#define UNIQUE_FAN_SINGLE_WAIT          12
#define UNIQUE_FAN_SELF_DRAWN           13

#define SET_UNIQUE_FAN(unique_, idx_) ((unique_) |= (1U << (idx_)))
#define TEST_UNIQUE_FAN(unique_, idx_) !!((unique_) & (1U << (idx_)))

#define MULTIPLE_FAN_TILE_HOG           0
#define MULTIPLE_FAN_DOUBLE_PUNG        1
#define MULTIPLE_FAN_PURE_DOUBLE_CHOW   2
#define MULTIPLE_FAN_MIXED_DOUBLE_CHOW  3
#define MULTIPLE_FAN_SHORT_STRAIGHT     4
#define MULTIPLE_FAN_TWO_TERMINAL_CHOWS 5
#define MULTIPLE_FAN_PUNG_OF_TERMINALS_OR_HONORS    6
#define MULTIPLE_FAN_ONE_VOIDED_SUIT    7
#define MULTIPLE_FAN_FLOWER_TILES       8

#define SET_MULTIPLE_FAN(multiple_, idx_, cnt_) ((multiple_) |= (static_cast<uint64_t>(cnt_) << ((idx_) * 4)))
#define MULTIPLE_FAN_COUNT(multiple_, idx_)  static_cast<uint8_t>(((multiple_) >> ((idx_) * 4)) & 0xF)

void ReadRecordFromFile(const char *file, Record &record);
void WriteRecordToFile(const char *file, const Record &record);

void LoadHistoryRecords(const char *file, std::vector<Record> &records);
void SaveHistoryRecords(const char *file, const std::vector<Record> &records);
void ModifyRecordInHistory(std::vector<Record> &records, const Record *r);

void TranslateDetailToScoreTable(const Record::Detail &detail, int (&scoreTable)[4]);
void CalculateRankFromScore(const int (&scores)[4], unsigned (&ranks)[4]);
void RankToStandardScore(const unsigned (&ranks)[4], unsigned (&ss12)[4]);

#endif
