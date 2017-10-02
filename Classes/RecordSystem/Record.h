#ifndef __RECORD_H__
#define __RECORD_H__

#include <stddef.h>
#include <time.h>
#include <stdint.h>
#include <string.h>
#include <string>

#include "json/document.h"

#define NAME_SIZE 64

struct Record {
    char name[4][NAME_SIZE];// 选手姓名
    struct Detail {
        uint8_t win_claim;  // 和牌标记（4567bit）/点炮标记（0123bit）
        uint8_t false_win;  // 错和标记
        uint8_t packed_fan; // 小番组合
        uint32_t score;     // 番数
        uint64_t fan_flag;  // 标记番种
    } detail[16];           // 每一盘的详情
    size_t current_index;   // 当前打到第几盘
    time_t start_time;      // 开始时间
    time_t end_time;        // 结束时间
};

static bool operator==(const Record &left, const Record &right) {
    return memcmp(&left, &right, sizeof(Record)) == 0;
}

#define SET_WIN(wc_, n_) ((wc_) |= (1 << ((n_) + 4)))
#define TEST_WIN(wc_, n_) !!((wc_) & (1 << ((n_) + 4)))

#define SET_CLAIM(wc_, n_) ((wc_) |= (1 << (n_)))
#define TEST_CLAIM(wc_, n_) !!((wc_) & (1 << (n_)))

#define WIN_INDEX(wc_) (((wc_) & 0x80) ? 3 : ((wc_) & 0x40) ? 2 : ((wc_) & 0x20) ? 1 : ((wc_) & 0x10) ? 0 : -1)
#define CLAIM_INDEX(wc_) (((wc_) & 0x8) ? 3 : ((wc_) & 0x4) ? 2 : ((wc_) & 0x2) ? 1 : ((wc_) & 0x1) ? 0 : -1)

#define SET_FALSE_WIN(fw_, n_) ((fw_) |= (1 << (n_)))
#define TEST_FALSE_WIN(fw_, n_) !!((fw_) & (1 << (n_)))

#define SET_FAN(flag_, fan_) ((flag_) |= (1ULL << (mahjong::LAST_TILE - (fan_))))
#define RESET_FAN(flag_, fan_) ((flag_) &= ~(1ULL << (mahjong::LAST_TILE - (fan_))))
#define TEST_FAN(flag_, fan_) !!((flag_) & (1ULL << (mahjong::LAST_TILE - (fan_))))

void JsonToRecord(const rapidjson::Value &json, Record &record);
void RecordToJson(const Record &record, rapidjson::Value &json, rapidjson::Value::AllocatorType &alloc);

void TranslateDetailToScoreTable(const Record::Detail &detail, int (&scoreTable)[4]);
const char *GetShortFanText(const Record::Detail &detail);
std::string GetLongFanText(const Record::Detail &detail);
const char *GetPackedFanText(uint8_t packedFan);

#endif
