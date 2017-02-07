#ifndef __RECORD_H__
#define __RECORD_H__

#include <stddef.h>
#include <time.h>
#include <stdint.h>
#include <string.h>

struct Record {
    char name[4][255];
    struct Detail {
        uint8_t win_claim;
        uint8_t false_win;
        uint32_t score;
        uint64_t points_flag;
    } detail[16];
    size_t current_index;
    time_t start_time;
    time_t end_time;
};

static bool operator==(const Record &left, const Record &right) {
    return memcmp(&left, &right, sizeof(Record)) == 0;
}

#define SET_WIN(wc_, n_) ((wc_) |= (1 << ((n_) + 4)))
#define SET_CLAIM(wc_, n_) ((wc_) |= (1 << (n_)))

#define WIN_INDEX(wc_) (((wc_) & 0x80) ? 3 : ((wc_) & 0x40) ? 2 : ((wc_) & 0x20) ? 1 : ((wc_) & 0x10) ? 0 : -1)
#define CLAIM_INDEX(wc_) (((wc_) & 0x8) ? 3 : ((wc_) & 0x4) ? 2 : ((wc_) & 0x2) ? 1 : ((wc_) & 0x1) ? 0 : -1)

#define SET_FALSE_WIN(fw_, n_) ((fw_) |= (1 << (n_)))
#define TEST_FALSE_WIN(fw_, n_) !!((fw_) & (1 << (n_)))

#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
#define PRId64 "lld"
#endif
#include "../wheels/cppJSON.hpp"

static void fromJson(Record *record, const jw::cppJSON &json) {
    memset(record, 0, sizeof(Record));
    auto name = json.GetValueByKeyNoThrow<std::vector<std::string> >("name");
    if (name.size() == 4) {
        for (int i = 0; i < 4; ++i) {
            strncpy(record->name[i], name[i].c_str(), name[i].length());
        }
    }

    auto detail = json.GetValueByKeyNoThrow<std::vector<std::unordered_map<std::string, uint64_t> > >("detail");
    record->current_index = detail.size();
    for (size_t i = 0; i < record->current_index; ++i) {
        std::unordered_map<std::string, uint64_t> temp = std::move(detail[i]);
        record->detail[i].win_claim = static_cast<uint8_t>(temp["win_claim"]);
        record->detail[i].false_win = static_cast<uint8_t>(temp["false_win"]);
        record->detail[i].score = static_cast<uint32_t>(temp["score"]);
        record->detail[i].points_flag = temp["points_flag"];
    }

    record->start_time = json.GetValueByKeyNoThrow<time_t>("start_time");
    record->end_time = json.GetValueByKeyNoThrow<time_t>("end_time");
}

static void toJson(const Record &record, jw::cppJSON *json) {
    std::vector<std::string> name;
    for (int i = 0; i < 4; ++i) {
        name.push_back(std::string(record.name[i]));
    }
    json->insert(std::make_pair("name", std::move(name)));

    std::vector<std::unordered_map<std::string, uint64_t> > detail;
    detail.reserve(16);
    for (size_t i = 0; i < record.current_index; ++i) {
        std::unordered_map<std::string, uint64_t> temp;
        temp.insert(std::make_pair("win_claim", record.detail[i].win_claim));
        temp.insert(std::make_pair("false_win", record.detail[i].false_win));
        temp.insert(std::make_pair("score", record.detail[i].score));
        temp.insert(std::make_pair("points_flag", record.detail[i].points_flag));
        detail.push_back(std::move(temp));
    }
    json->insert(std::make_pair("detail", std::move(detail)));

    json->insert(std::make_pair("start_time", record.start_time));
    json->insert(std::make_pair("end_time", record.end_time));
}

static void translateDetailToScoreTable(const Record::Detail &detail, int (&scoreTable)[4]) {
    memset(scoreTable, 0, sizeof(scoreTable));
    int winScore = detail.score;
    if (winScore >= 8 && !!(detail.win_claim & 0xF0)) {
        uint8_t wc = detail.win_claim;
        int winIndex = WIN_INDEX(wc);
        int claimIndex = CLAIM_INDEX(wc);
        if (winIndex == claimIndex) {  // 自摸
            for (int i = 0; i < 4; ++i) {
                scoreTable[i] = (i == winIndex) ? (winScore + 8) * 3 : (-8 - winScore);
            }
        }
        else {  // 点炮
            for (int i = 0; i < 4; ++i) {
                scoreTable[i] = (i == winIndex) ? (winScore + 24) : (i == claimIndex ? (-8 - winScore) : -8);
            }
        }
    }

    // 检查错和
    for (int i = 0; i < 4; ++i) {
        if (TEST_FALSE_WIN(detail.false_win, i)) {
            scoreTable[i] -= 30;
            for (int j = 0; j < 4; ++j) {
                if (j == i) continue;
                scoreTable[j] += 10;
            }
        }
    }
}

#endif
