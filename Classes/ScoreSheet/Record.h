#ifndef __RECORD_H__
#define __RECORD_H__

#include <stddef.h>
#include <time.h>
#include <stdint.h>
#include <string.h>

struct Record {
    char name[4][255];
    int scores[16][4];
    uint64_t pointsFlag[16];
    size_t currentIndex;
    time_t startTime;
    time_t endTime;
};

static bool operator==(const Record &left, const Record &right) {
    return memcmp(&left, &right, sizeof(Record)) == 0;
}

#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
#define PRId64 "lld"
#endif
#include "../wheels/cppJSON.hpp"

static void fromJson(Record *record, const jw::cppJSON &json) {
    jw::cppJSON::const_iterator it = json.find("name");
    if (it != json.end()) {
        auto vec = it->as<std::vector<std::string> >();
        if (vec.size() == 4) {
            for (int i = 0; i < 4; ++i) {
                strncpy(record->name[i], vec[i].c_str(), vec[i].length());
            }
        }
    }
    it = json.find("record");
    if (it != json.end()) {
        auto vec = it->as<std::vector<std::vector<int> > >();
        record->currentIndex = vec.size();
        for (size_t i = 0; i < record->currentIndex; ++i) {
            memcpy(record->scores[i], &vec[i][0], sizeof(record->scores[i]));
        }
    }
    it = json.find("points_flag");
    if (it != json.end()) {
        auto vec = it->as<std::vector<uint64_t> >();
        memcpy(record->pointsFlag, &vec[0], sizeof(uint64_t) * vec.size());
    }
    it = json.find("start_time");
    if (it != json.end()) {
        record->startTime = it->as<time_t>();
    }
    it = json.find("end_time");
    if (it != json.end()) {
        record->endTime = it->as<time_t>();
    }
}

static void toJson(const Record &record, jw::cppJSON *json) {
    std::vector<std::string> nameVec;
    for (int i = 0; i < 4; ++i) {
        nameVec.push_back(std::string(record.name[i]));
    }
    json->insert(std::make_pair("name", std::move(nameVec)));

    std::vector<std::vector<int> > scoreVec;
    scoreVec.reserve(16);
    for (size_t k = 0; k < record.currentIndex; ++k) {
        scoreVec.push_back(std::vector<int>({ record.scores[k][0], record.scores[k][1], record.scores[k][2], record.scores[k][3] }));
    }
    json->insert(std::make_pair("record", std::move(scoreVec)));

    std::vector<uint64_t> pointsFlagVec;
    pointsFlagVec.resize(record.currentIndex);
    memcpy(&pointsFlagVec[0], record.pointsFlag, sizeof(uint64_t) * record.currentIndex);
    json->insert(std::make_pair("points_flag", std::move(pointsFlagVec)));

    json->insert(std::make_pair("start_time", record.startTime));
    json->insert(std::make_pair("end_time", record.endTime));
}

#endif
