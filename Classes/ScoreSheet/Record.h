#ifndef __RECORD_H__
#define __RECORD_H__

#include <stddef.h>
#include <time.h>
#include <stdint.h>
#include <string.h>

#include "json/document.h"

struct Record {
    char name[4][255];
    struct Detail {
        uint8_t win_claim;
        uint8_t false_win;
        uint32_t score;
        uint64_t fan_flag;

        struct WinHand {
            uint16_t fixed_packs[5];
            uint8_t pack_count;
            uint8_t standing_tiles[13];
            uint8_t tile_count;
            uint8_t win_tile;
            uint8_t win_flag;
            uint8_t flower_count;
        } win_hand;
    } detail[16];
    size_t current_index;
    time_t start_time;
    time_t end_time;
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
const char *GetFanText(const Record::Detail &detail);

#endif
