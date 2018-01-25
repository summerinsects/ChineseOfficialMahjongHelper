#include "Record.h"
#include <algorithm>
#include <iterator>
#include "json/document.h"
#include "json/stringbuffer.h"
#if defined(COCOS2D_DEBUG) && (COCOS2D_DEBUG > 0)
#include "json/prettywriter.h"
#else
#include "json/writer.h"
#endif
#include "../mahjong-algorithm/fan_calculator.h"
#include "../utils/common.h"

namespace {

void JsonToRecord(const rapidjson::Value &json, Record &record) {
    memset(&record, 0, sizeof(Record));

    rapidjson::Value::ConstMemberIterator it = json.FindMember("name");
    if (it != json.MemberEnd() && it->value.IsArray()) {
        rapidjson::Value::ConstArray name = it->value.GetArray();
        if (name.Size() == 4) {
            for (int i = 0; i < 4; ++i) {
                strncpy(record.name[i], name[i].GetString(), NAME_SIZE - 1);
            }
        }
    }

    it = json.FindMember("detail");
    if (it != json.MemberEnd() && it->value.IsArray()) {
        rapidjson::Value::ConstArray detail = it->value.GetArray();
        record.current_index = detail.Size();

        for (size_t i = 0; i < record.current_index; ++i) {
            Record::Detail &detail_data = record.detail[i];
            const rapidjson::Value &detail_json = detail[static_cast<rapidjson::SizeType>(i)];

            // 兼容旧的key until 2017.12.29 7eed14a
            it = detail_json.FindMember("win_claim");
            if (it != detail_json.MemberEnd() && it->value.IsUint()) {
                uint32_t wc = it->value.GetUint();
                detail_data.win_flag = static_cast<uint8_t>((wc >> 4) & 0xF);
                detail_data.claim_flag = static_cast<uint8_t>(wc & 0xF);
            }

            it = detail_json.FindMember("win_flag");
            if (it != detail_json.MemberEnd() && it->value.IsUint()) {
                detail_data.win_flag = static_cast<uint8_t>(it->value.GetUint());
            }

            it = detail_json.FindMember("claim_flag");
            if (it != detail_json.MemberEnd() && it->value.IsUint()) {
                detail_data.claim_flag = static_cast<uint8_t>(it->value.GetUint());
            }

            // 兼容旧的key until 2017.12.17 a3dffdc
            it = detail_json.FindMember("false_win");
            if (it != detail_json.MemberEnd() && it->value.IsUint()) {
                uint32_t false_win = it->value.GetUint();
                for (int k = 0; k < 4; ++k) {
                    if (false_win & (1 << k)) {
                        detail_data.penalty_scores[k] -= 30;
                        for (int j = 0; j < 4; ++j) {
                            if (j == k) continue;
                            detail_data.penalty_scores[j] += 10;
                        }
                    }
                }
            }

            // 兼容旧的key until 2017.12.30 31cd281
            it = detail_json.FindMember("packed_fan");
            if (it != detail_json.MemberEnd() && it->value.IsUint()) {
                uint32_t packed_fan = it->value.GetUint();
                uint16_t unique_fan = 0;
                uint64_t multiple_fan = 0;
                switch (packed_fan) {
                case 1:  // 门断平
                    SET_UNIQUE_FAN(unique_fan, UNIQUE_FAN_CONCEALED_HAND);
                    SET_UNIQUE_FAN(unique_fan, UNIQUE_FAN_ALL_SIMPLES);
                    SET_UNIQUE_FAN(unique_fan, UNIQUE_FAN_ALL_CHOWS);
                    break;
                case 2:  // 门清平和
                    SET_UNIQUE_FAN(unique_fan, UNIQUE_FAN_CONCEALED_HAND);
                    SET_UNIQUE_FAN(unique_fan, UNIQUE_FAN_ALL_CHOWS);
                    break;
                case 3:  // 断幺平和
                    SET_UNIQUE_FAN(unique_fan, UNIQUE_FAN_ALL_SIMPLES);
                    SET_UNIQUE_FAN(unique_fan, UNIQUE_FAN_ALL_CHOWS);
                    break;
                case 4:  // 连风刻
                    SET_UNIQUE_FAN(unique_fan, UNIQUE_FAN_PREVALENT_WIND);
                    SET_UNIQUE_FAN(unique_fan, UNIQUE_FAN_SEAT_WIND);
                    break;
                case 5:  // 番牌暗杠
                    SET_UNIQUE_FAN(unique_fan, UNIQUE_FAN_CONCEALED_KONG);
                    break;
                case 6:  // 双同幺九
                    SET_MULTIPLE_FAN(multiple_fan, MULTIPLE_FAN_DOUBLE_PUNG, 1);
                    SET_MULTIPLE_FAN(multiple_fan, MULTIPLE_FAN_PUNG_OF_TERMINALS_OR_HONORS, 2);
                    break;
                case 7:  // 门清双暗
                    SET_UNIQUE_FAN(unique_fan, UNIQUE_FAN_CONCEALED_HAND);
                    SET_UNIQUE_FAN(unique_fan, UNIQUE_FAN_TWO_CONCEALED_PUNGS);
                    break;
                case 8:  // 双暗暗杠
                    SET_UNIQUE_FAN(unique_fan, UNIQUE_FAN_CONCEALED_KONG);
                    SET_UNIQUE_FAN(unique_fan, UNIQUE_FAN_TWO_CONCEALED_PUNGS);
                    break;
                default:
                    break;
                }
                detail_data.unique_fan = unique_fan;
                detail_data.multiple_fan = multiple_fan;
            }

            it = detail_json.FindMember("fan");
            if (it != detail_json.MemberEnd() && it->value.IsUint()) {
                detail_data.fan = static_cast<uint16_t>(it->value.GetUint());
            }

            // 兼容旧的key until 2017.12.17 b2e18bc
            it = detail_json.FindMember("score");
            if (it != detail_json.MemberEnd() && it->value.IsUint()) {
                detail_data.fan = static_cast<uint16_t>(it->value.GetUint());
            }

            it = detail_json.FindMember("fan_flag");
            if (it != detail_json.MemberEnd() && it->value.IsUint64()) {
                detail_data.fan_flag = it->value.GetUint64();
            }

            it = detail_json.FindMember("unique_fan");
            if (it != detail_json.MemberEnd() && it->value.IsUint()) {
                detail_data.unique_fan = static_cast<uint16_t>(it->value.GetUint());
            }

            it = detail_json.FindMember("multiple_fan");
            if (it != detail_json.MemberEnd() && it->value.IsUint64()) {
                detail_data.multiple_fan = it->value.GetUint64();
            }

            it = detail_json.FindMember("penalty_scores");
            if (it != detail_json.MemberEnd() && it->value.IsArray()) {
                rapidjson::Value::ConstArray penalty_scores = it->value.GetArray();
                if (penalty_scores.Size() == 4) {
                    for (int n = 0; n < 4; ++n) {
                        detail_data.penalty_scores[n] = static_cast<int16_t>(penalty_scores[n].GetInt());
                    }
                }
            }

            it = detail_json.FindMember("win_hand");
            if (it != detail_json.MemberEnd() && it->value.IsObject()) {
                Record::Detail::WinHand &win_hand_data = detail_data.win_hand;

                const rapidjson::Value::ConstObject &win_hand_json = it->value.GetObject();
                it = win_hand_json.FindMember("tiles");
                if (it != win_hand_json.MemberEnd() && it->value.IsString()) {
                    strncpy(win_hand_data.tiles, it->value.GetString(), sizeof(win_hand_data.tiles) - 1);
                }

                it = win_hand_json.FindMember("win_flag");
                if (it != win_hand_json.MemberEnd() && it->value.IsUint()) {
                    win_hand_data.win_flag = static_cast<uint8_t>(it->value.GetUint());
                }

                it = win_hand_json.FindMember("flower_count");
                if (it != win_hand_json.MemberEnd() && it->value.IsUint()) {
                    win_hand_data.flower_count = static_cast<uint8_t>(it->value.GetUint());
                }
            }
        }
    }

    it = json.FindMember("start_time");
    if (it != json.MemberEnd() && it->value.IsUint64()) {
        record.start_time = static_cast<time_t>(it->value.GetUint64());
    }

    it = json.FindMember("end_time");
    if (it != json.MemberEnd() && it->value.IsUint64()) {
        record.end_time = static_cast<time_t>(it->value.GetUint64());
    }
}

void RecordToJson(const Record &record, rapidjson::Value &json, rapidjson::Value::AllocatorType &alloc) {
    rapidjson::Value name(rapidjson::Type::kArrayType);
    name.Reserve(4, alloc);
    for (int i = 0; i < 4; ++i) {
        name.PushBack(rapidjson::StringRef(record.name[i]), alloc);
    }
    json.AddMember("name", std::move(name), alloc);

    rapidjson::Value detail(rapidjson::Type::kArrayType);
    detail.Reserve(static_cast<rapidjson::SizeType>(record.current_index), alloc);
    for (size_t i = 0; i < record.current_index; ++i) {
        const Record::Detail &detail_data = record.detail[i];
        rapidjson::Value detail_json(rapidjson::Type::kObjectType);
        detail_json.AddMember("win_flag", rapidjson::Value(detail_data.win_flag), alloc);
        detail_json.AddMember("claim_flag", rapidjson::Value(detail_data.claim_flag), alloc);
        detail_json.AddMember("fan", rapidjson::Value(detail_data.fan), alloc);
        detail_json.AddMember("fan_flag", rapidjson::Value(detail_data.fan_flag), alloc);
        detail_json.AddMember("unique_fan", rapidjson::Value(detail_data.unique_fan), alloc);
        detail_json.AddMember("multiple_fan", rapidjson::Value(detail_data.multiple_fan), alloc);

        rapidjson::Value penalty_scores(rapidjson::Type::kArrayType);
        penalty_scores.Reserve(4, alloc);
        for (int n = 0; n < 4; ++n) {
            penalty_scores.PushBack(rapidjson::Value(detail_data.penalty_scores[n]), alloc);
        }
        detail_json.AddMember("penalty_scores", std::move(penalty_scores), alloc);

        const Record::Detail::WinHand &win_hand_data = detail_data.win_hand;
        rapidjson::Value win_hand_json(rapidjson::Type::kObjectType);
        win_hand_json.AddMember("tiles", rapidjson::StringRef(win_hand_data.tiles), alloc);
        win_hand_json.AddMember("win_flag", rapidjson::Value(win_hand_data.win_flag), alloc);
        win_hand_json.AddMember("flower_count", rapidjson::Value(win_hand_data.flower_count), alloc);

        detail_json.AddMember("win_hand", std::move(win_hand_json), alloc);

        detail.PushBack(std::move(detail_json), alloc);
    }
    json.AddMember("detail", std::move(detail), alloc);

    json.AddMember("start_time", rapidjson::Value(static_cast<uint64_t>(record.start_time)), alloc);
    json.AddMember("end_time", rapidjson::Value(static_cast<uint64_t>(record.end_time)), alloc);
}

void SortRecords(std::vector<Record> &records) {
    // 用指针排序
    std::vector<Record *> ptrs(records.size());
    std::transform(records.begin(), records.end(), ptrs.begin(), [](Record &r) { return &r; });
    std::sort(ptrs.begin(), ptrs.end(), [](const Record *r1, const Record *r2) { return r1->start_time > r2->start_time; });
    std::vector<Record> temp;
    temp.reserve(records.size());
    std::for_each(ptrs.begin(), ptrs.end(), [&temp](Record *r) {
        temp.emplace_back();
        std::swap(temp.back(), *r);
    });
    records.swap(temp);
}

}

void ReadRecordFromFile(const char *file, Record &record) {
    std::string str = Common::getStringFromFile(file);

    try {
        rapidjson::Document doc;
        doc.Parse<0>(str.c_str());
        if (doc.HasParseError()) {
            return;
        }

        JsonToRecord(doc, record);
    }
    catch (std::exception &e) {
        MYLOG("%s %s", __FUNCTION__, e.what());
    }
}

void WriteRecordToFile(const char *file, const Record &record) {
    try {
        rapidjson::Document doc(rapidjson::Type::kObjectType);
        RecordToJson(record, doc, doc.GetAllocator());

        rapidjson::StringBuffer buf;
#ifdef COCOS2D_DEBUG
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buf);
#else
        rapidjson::Writer<rapidjson::StringBuffer> writer(buf);
#endif
        doc.Accept(writer);

        MYLOG("%.*s", static_cast<int>(buf.GetSize()), buf.GetString());

        FILE *fp = fopen(file, "wb");
        if (LIKELY(fp != nullptr)) {
            fwrite(buf.GetString(), 1, buf.GetSize(), fp);
            fclose(fp);
        }
    }
    catch (std::exception &e) {
        MYLOG("%s %s", __FUNCTION__, e.what());
    }
}

void LoadHistoryRecords(const char *file, std::vector<Record> &records) {
    std::string str = Common::getStringFromFile(file);

    try {
        rapidjson::Document doc;
        doc.Parse<0>(str.c_str());
        if (doc.HasParseError() || !doc.IsArray()) {
            return;
        }

        records.clear();
        records.reserve(doc.Size());
        std::transform(doc.Begin(), doc.End(), std::back_inserter(records), [](const rapidjson::Value &json) {
            Record record;
            JsonToRecord(json, record);
            return record;
        });

        SortRecords(records);
    }
    catch (std::exception &e) {
        MYLOG("%s %s", __FUNCTION__, e.what());
    }
}

void SaveHistoryRecords(const char *file, const std::vector<Record> &records) {
    FILE *fp = fopen(file, "wb");
    if (LIKELY(fp != nullptr)) {
        try {
            rapidjson::Document doc(rapidjson::Type::kArrayType);
            doc.Reserve(static_cast<rapidjson::SizeType>(records.size()), doc.GetAllocator());
            std::for_each(records.begin(), records.end(), [&doc](const Record &record) {
                rapidjson::Value json(rapidjson::Type::kObjectType);
                RecordToJson(record, json, doc.GetAllocator());
                doc.PushBack(std::move(json), doc.GetAllocator());
            });

            rapidjson::StringBuffer buf;
#if defined(COCOS2D_DEBUG) && (COCOS2D_DEBUG > 0)
            rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buf);
#else
            rapidjson::Writer<rapidjson::StringBuffer> writer(buf);
#endif
            doc.Accept(writer);

            fwrite(buf.GetString(), 1, buf.GetSize(), fp);
        }
        catch (std::exception &e) {
            MYLOG("%s %s", __FUNCTION__, e.what());
        }
        fclose(fp);
    }
}

void ModifyRecordInHistory(std::vector<Record> &records, const Record *r) {
    // 我们认为开始时间相同的为同一个记录
    time_t start_time = r->start_time;
    auto it = std::find_if(records.begin(), records.end(), [start_time](const Record &rr) {
        return (rr.start_time == start_time);
    });

    // 未找到，则添加；找到，则覆盖
    if (it == records.end()) {
        records.push_back(*r);
    }
    else {
        memcpy(&*it, r, sizeof(*r));
    }

    SortRecords(records);
}

void TranslateDetailToScoreTable(const Record::Detail &detail, int (&scoreTable)[4]) {
    memset(scoreTable, 0, sizeof(scoreTable));
    int fan = static_cast<int>(detail.fan);
    if (fan >= 8 && detail.win_flag != 0) {
        uint8_t wf = detail.win_flag;
        uint8_t cf = detail.claim_flag;
        int winIndex = WIN_CLAIM_INDEX(wf);
        int claimIndex = WIN_CLAIM_INDEX(cf);
        if (winIndex == claimIndex) {  // 自摸
            for (int i = 0; i < 4; ++i) {
                scoreTable[i] = (i == winIndex) ? (fan + 8) * 3 : (-8 - fan);
            }
        }
        else {  // 点炮
            for (int i = 0; i < 4; ++i) {
                scoreTable[i] = (i == winIndex) ? (fan + 24) : (i == claimIndex ? (-8 - fan) : -8);
            }
        }
    }

    // 加上处罚
    for (int i = 0; i < 4; ++i) {
        scoreTable[i] += detail.penalty_scores[i];
    }
}

void CalculateRankFromScore(const int (&scores)[4], unsigned (&ranks)[4]) {
    memset(ranks, 0, sizeof(ranks));
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (i == j) continue;
            if (scores[i] < scores[j]) ++ranks[i];
            //if (scores[i] == scores[j] && i > j) ++ranks[i];  // 这一行的作用是取消并列
        }
    }
}

void RankToStandardScore(const unsigned (&ranks)[4], float (&ss)[4]) {
    // 并列的数目
    unsigned rankCnt[4] = { 0 };
    for (int i = 0; i < 4; ++i) {
        ++rankCnt[ranks[i]];
    }

    static const float standardScore[] = { 4, 2, 1, 0 };
    for (int i = 0; i < 4; ++i) {
        unsigned rank = ranks[i];
        unsigned tieCnt = rankCnt[rank];  // 并列的人数

        // 累加并列的标准分
        float ss0 = standardScore[rank];
        for (unsigned n = 1, cnt = tieCnt; n < cnt; ++n) {
            ss0 += standardScore[rank + n];
        }
        ss0 /= tieCnt;
        ss[i] = ss0;
    }
}
