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

static const char *packedFanNames[] = {
    "门断平", "门清平和", "断幺平和", "连风刻", "番牌暗杠", "双同幺九", "门清双暗", "双暗暗杠"
};

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

            it = detail_json.FindMember("win_claim");
            if (it != detail_json.MemberEnd() && it->value.IsUint()) {
                detail_data.win_claim = it->value.GetUint();
            }

            it = detail_json.FindMember("false_win");
            if (it != detail_json.MemberEnd() && it->value.IsUint()) {
                detail_data.false_win = it->value.GetUint();
            }

            it = detail_json.FindMember("packed_fan");
            if (it != detail_json.MemberEnd() && it->value.IsUint()) {
                detail_data.packed_fan = it->value.GetUint();
            }

            it = detail_json.FindMember("score");
            if (it != detail_json.MemberEnd() && it->value.IsUint()) {
                detail_data.score = it->value.GetUint();
            }

            it = detail_json.FindMember("fan_flag");
            if (it != detail_json.MemberEnd() && it->value.IsUint64()) {
                detail_data.fan_flag = it->value.GetUint64();
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
                    win_hand_data.win_flag = it->value.GetUint();
                }

                it = win_hand_json.FindMember("flower_count");
                if (it != win_hand_json.MemberEnd() && it->value.IsUint()) {
                    win_hand_data.flower_count = it->value.GetUint();
                }
            }
        }
    }

    it = json.FindMember("start_time");
    if (it != json.MemberEnd() && it->value.IsUint64()) {
        record.start_time = it->value.GetUint64();
    }

    it = json.FindMember("end_time");
    if (it != json.MemberEnd() && it->value.IsUint64()) {
        record.end_time = it->value.GetUint64();
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
        detail_json.AddMember("win_claim", rapidjson::Value(detail_data.win_claim), alloc);
        detail_json.AddMember("false_win", rapidjson::Value(detail_data.false_win), alloc);
        detail_json.AddMember("packed_fan", rapidjson::Value(detail_data.packed_fan), alloc);
        detail_json.AddMember("score", rapidjson::Value(detail_data.score), alloc);
        detail_json.AddMember("fan_flag", rapidjson::Value(detail_data.fan_flag), alloc);

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
    auto it = std::find_if(records.begin(), records.end(), [start_time](const Record &r) {
        return (r.start_time == start_time);
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

void CompetitionScoreToStandardScore(const int (&cs)[4], float (&ss)[4]) {
    unsigned ranks[4];
    CalculateRankFromScore(cs, ranks);
    RankToStandardScore(ranks, ss);
}

void SummarizeRecords(const std::vector<int8_t> &flags, const std::vector<Record> &records, RecordsStatistic *result) {
    memset(result, 0, sizeof(*result));

    for (size_t i = 0, cnt = std::min<size_t>(flags.size(), records.size()); i < cnt; ++i) {
        const Record &record = records[i];
        if (record.end_time == 0) {
            continue;
        }

        int8_t idx = flags[i];
        if (idx == -1) {
            continue;
        }

        int totalScores[4] = { 0 };

        for (int k = 0; k < 16; ++k) {
            const Record::Detail &detail = record.detail[k];

            int scoreTable[4];
            TranslateDetailToScoreTable(detail, scoreTable);
            for (int n = 0; n < 4; ++n) {
                totalScores[n] += scoreTable[n];
            }

            uint8_t wc = detail.win_claim;
            int winIndex = WIN_INDEX(wc);
            int claimIndex = CLAIM_INDEX(wc);
            if (winIndex == idx) {
                ++result->win;
                if (claimIndex == idx) {
                    ++result->self_drawn;
                }
                result->win_fan += detail.score;
                result->max_fan = std::max<uint16_t>(result->max_fan, detail.score);
            }
            else if (claimIndex == idx) {
                ++result->claim;
                result->claim_fan += detail.score;
            }
        }

        unsigned ranks[4];
        CalculateRankFromScore(totalScores, ranks);
        ++result->rank[ranks[idx]];
        result->competition_score += totalScores[idx];

        float ss[4];
        RankToStandardScore(ranks, ss);
        result->standard_score += ss[idx];
    }
}

static const char *fan_name2[mahjong::LAST_TILE + 1][mahjong::LAST_TILE + 1] = {
    { nullptr },

    { "大四喜",
    "大四喜", nullptr, nullptr, nullptr, "大四喜四杠", nullptr, nullptr,
    nullptr, nullptr, nullptr, "大四喜字一色", "大四喜四暗", nullptr,
    nullptr, nullptr,
    nullptr, "大四喜三杠", "大四喜混幺九",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, "大四喜三暗",
    nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, "妙手大四喜", "海底大四喜", "杠开大四喜", nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr },  // 大四喜

    { "大三元",
    nullptr, "大三元", nullptr, nullptr, "大三元四杠", nullptr, nullptr,
    nullptr, nullptr, nullptr, "大三元字一色", "大三元四暗", nullptr,
    nullptr, nullptr,
    nullptr, "大三元三杠", "大三元混幺九",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, "大三元三暗",
    nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, "妙手大三元", "海底大三元", "杠开大三元", "大三元抢杠",
    "大三元碰碰和", "大三元混一色", nullptr, nullptr, nullptr, nullptr, nullptr,
    "大三元全带幺", nullptr, nullptr, nullptr },  // 大三元

    { "绿一色",
    nullptr, nullptr, "绿一色", nullptr, "绿一色四杠", nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, "绿一色四暗", nullptr,
    "绿一色四同顺", nullptr,
    nullptr, "绿一色三杠", nullptr,
    "绿一色七对", nullptr, nullptr, "绿一色清一色", "绿一色三同顺", "绿一色三节", nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, "绿一色三暗",
    nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, "绿一色推不倒", nullptr, nullptr, nullptr, "妙手绿一色", "海底绿一色", "杠开绿一色", "绿一色抢杠",
    "绿一色碰碰和", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr },  // 绿一色

    { "九莲宝灯",
    nullptr, nullptr, nullptr, "九莲宝灯", nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr,
    nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    "九莲宝灯清龙", nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, "妙手九莲宝灯", "海底九莲宝灯", nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr },  // 九莲宝灯

    { "四杠",
    nullptr, nullptr, nullptr, nullptr, "四杠", nullptr, nullptr,
    "清幺九四杠", "小四喜四杠", "小三元四杠", "字一色四杠", "四暗杠", nullptr,
    nullptr, "四节四杠",
    nullptr, nullptr, "混幺九四杠",
    nullptr, nullptr, "全双刻四杠", "清一色四杠", nullptr, nullptr, "全大四杠", "全中四杠", "全小四杠",
    nullptr, nullptr, nullptr, nullptr, "三同刻四杠", "四杠三暗",
    nullptr, nullptr, "大于五四杠", "小于五四杠", "四杠三风刻",
    nullptr, "推不倒四杠", nullptr, nullptr, nullptr, "妙手四杠", "海底四杠", "四杠杠开", nullptr,
    nullptr, "混一色四杠", nullptr, "五门四杠", "全求人四杠", nullptr, "四杠双箭刻",
    nullptr, nullptr, nullptr, nullptr },  // 四杠

    { "连七对",
    nullptr, nullptr, nullptr, nullptr, nullptr, "连七对", nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr,
    nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, "妙手连七对", "海底连七对", nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr },  // 连七对

    { "十三幺",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, "十三幺",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr,
    nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, "妙手十三幺", "海底十三幺", nullptr, "抢杠十三幺",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, "绝张十三幺" },  // 十三幺


    { "清幺九",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    "清幺九", nullptr, nullptr, nullptr, "清幺九四暗", nullptr,
    nullptr, nullptr,
    nullptr, "清幺九三杠", nullptr,
    "清幺九七对", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, "清幺九三同刻", "清幺九三暗",
    nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, "妙手清幺九", "海底清幺九", "杠开清幺九", nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr },  // 清幺九

    { "小四喜",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, "小四喜", nullptr, "小四喜字一色", "小四喜四暗", nullptr,
    nullptr, nullptr,
    nullptr, "小四喜三杠", "小四喜混幺九",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, "小四喜三暗",
    nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, "妙手小四喜", "海底小四喜", "杠开小四喜", "小四喜抢杠",
    "小四喜碰碰和", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    "小四喜全带幺", nullptr, nullptr, nullptr },  // 小四喜

    { "小三元",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, "小三元", "小三元字一色", "小三元四暗", nullptr,
    nullptr, nullptr,
    nullptr, "小三元三杠", "小三元混幺九",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, "小三元三暗",
    nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, "妙手小三元", "海底小三元", "杠开小三元", "小三元抢杠",
    "小三元碰碰和", "小三元混一色", nullptr, nullptr, nullptr, nullptr, nullptr,
    "小三元全带幺", nullptr, nullptr, nullptr },  // 小三元

    { "字一色",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, "字一色", "字一色四暗", nullptr,
    nullptr, nullptr,
    nullptr, "字一色三杠", nullptr,
    "字一色七对", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, "字一色三暗",
    nullptr, nullptr, nullptr, nullptr, "字一色三风刻",
    nullptr, nullptr, nullptr, nullptr, nullptr, "妙手字一色", "海底字一色", "杠开字一色", nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, "字一色双箭刻",
    nullptr, nullptr, nullptr, nullptr },  // 字一色

    { "四暗刻",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, "四暗刻", nullptr,
    nullptr, "四暗四节",
    nullptr, "四暗三杠", "混幺九四暗",
    nullptr, nullptr, "全双刻四暗", "清一色四暗", nullptr, "四暗一色三节", "全大四暗", "全中四暗", "全小四暗",
    nullptr, nullptr, nullptr, nullptr, "三同刻四暗", nullptr,
    nullptr, nullptr, "大于五四暗", "小于五四暗", "三风刻四暗",
    nullptr, "推不倒四暗", nullptr, "四暗三色三", nullptr, "妙手四暗", "海底四暗", "杠开四暗", nullptr,
    nullptr, "混一色四暗", nullptr, "五门四暗", nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr },  // 四暗刻

    { "一色双龙会",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, "一色双龙会",
    nullptr, nullptr,
    nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, "妙手一色双龙", "海底一色双龙", nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, "绝张一色双龙" },  // 一色双龙会


    { "一色四同顺",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    "一色四同顺", nullptr,
    nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, "清一色四同顺", nullptr, nullptr, "全大四同顺", "全中四同顺", "全小四同顺",
    nullptr, nullptr, nullptr, "四同顺全带五", nullptr, nullptr,
    nullptr, nullptr, "大于五四同顺", "小于五四同顺", nullptr,
    nullptr, "推不倒四同顺", nullptr, nullptr, nullptr, "妙手四同顺", "海底四同顺", nullptr, nullptr,
    nullptr, "混一色四同顺", nullptr, nullptr, nullptr, nullptr, nullptr,
    "全带幺四同顺", nullptr, nullptr, nullptr },  // 一色四同顺

    { "一色四节高",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, "一色四节高",
    nullptr, "四节三杠", nullptr,
    nullptr, nullptr, nullptr, "清一色四节", nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, "四节三暗",
    nullptr, nullptr, "大于五四节", "小于五四节", nullptr,
    nullptr, "推不倒四节", nullptr, nullptr, nullptr, "妙手四节", "海底四节", "杠开四节", nullptr,
    nullptr, "混一色四节", nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr },  // 一色四节高


    { "一色四步高",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr,
    "一色四步高", nullptr, nullptr,
    nullptr, nullptr, nullptr, "清一色四步", nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, "妙手四步", "海底四步", nullptr, "抢杠四步",
    nullptr, "混一色四步", nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr },  // 一色四步高

    { "三杠",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr,
    nullptr, "三杠", "三杠混幺九",
    nullptr, nullptr, "三杠全双刻", "三杠清一色", nullptr, nullptr, "三杠全大", "三杠全中", "三杠全小",
    nullptr, nullptr, nullptr, nullptr, "三杠三同刻", "三杠三暗",
    nullptr, nullptr, "三杠大于五", "三杠小于五", "三杠三风刻",
    nullptr, "三杠推不倒", nullptr, nullptr, nullptr, "三杠妙手", "三杠海底", "三杠杠开", "三杠抢杠",
    "三杠碰碰和", "三杠混一色", nullptr, "五门三杠", nullptr, nullptr, "三杠双箭刻",
    "三杠全带幺", nullptr, nullptr, nullptr },  // 三杠

    { "混幺九",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr,
    nullptr, nullptr, "混幺九",
    "混幺九七对", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, "混幺九三同刻", "混幺九三暗",
    nullptr, nullptr, nullptr, nullptr, "混幺九三风刻",
    nullptr, "混幺九推不倒", nullptr, nullptr, nullptr, "妙手混幺九", "海底混幺九", "杠开混幺九", nullptr,
    nullptr, "混幺九混一色", nullptr, "五门混幺九", nullptr, nullptr, "混幺九双箭刻",
    nullptr, nullptr, nullptr, nullptr },  // 混幺九


    { "七对",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr,
    nullptr, nullptr, nullptr,
    "七对", nullptr, nullptr, "清一色七对", nullptr, nullptr, "全大七对", "全中七对", "全小七对",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, "大于五七对", "小于五七对", nullptr,
    nullptr, "推不倒七对", nullptr, nullptr, nullptr, "妙手七对", "海底七对", nullptr, nullptr,
    nullptr, "混一色七对", nullptr, "五门七对", nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr },  // 七对

    { "七星不靠",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr,
    nullptr, nullptr, nullptr,
    nullptr, "七星不靠", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, "妙手七星", "海底七星", nullptr, "七星抢杠",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, "绝张七星" },  // 七星不靠

    { "全双刻",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr,
    nullptr, nullptr, nullptr,
    nullptr, nullptr, "全双刻", nullptr, nullptr, nullptr, nullptr, "全中全双刻", nullptr,
    nullptr, nullptr, nullptr, nullptr, "三同刻全双刻", "三暗全双刻",
    nullptr, nullptr, "大于五全双刻", "小于五全双刻", nullptr,
    nullptr, "推不倒全双刻", nullptr, nullptr, nullptr, "妙手全双刻", "海底全双刻", "杠开全双刻", nullptr,
    nullptr, nullptr, nullptr, nullptr, "全双刻全求人", nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr },  // 全双刻

    { "清一色",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr,
    nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, "清一色", "清一色三同顺", "清一色三节", nullptr, nullptr, nullptr,
    "清一色清龙", nullptr, "清一色三步", nullptr, nullptr, "清一色三暗",
    nullptr, nullptr, "清一色大于五", "清一色小于五", nullptr,
    nullptr, "清一色推不倒", nullptr, nullptr, nullptr, "妙手清一色", "海底清一色", "杠开清一色", "清一色抢杠",
    "清一色碰碰和", nullptr, nullptr, nullptr, "清一色全求人", nullptr, nullptr,
    "清一色全带幺", "清一色不求人", nullptr, "绝张清一色" },  // 清一色

    { "一色三同顺",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr,
    nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, "一色三同顺", nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, "混一色三同顺", nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr },  // 一色三同顺

    { "一色三节高",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr,
    nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, "一色三节高", nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, "混一色三节", nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr },  // 一色三节高

    { "全大",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr,
    nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, "全大", nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, "全大三同刻", "全大三暗",
    nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, "推不倒", nullptr, nullptr, nullptr, "妙手全大", "海底全大", "杠开全大", "全大抢杠",
    "全大碰碰和", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    "全大全带幺", "全大不求人", nullptr, nullptr },  // 全大

    { "全中",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr,
    nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, "全中", nullptr,
    nullptr, nullptr, nullptr, "全中全带五", "全中三同刻", "全中三暗",
    nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, "全中推不倒", nullptr, nullptr, nullptr, "妙手全中", "海底全中", "杠开全中", "全中抢杠",
    "全中碰碰和", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, "全中不求人", nullptr, nullptr },  // 全中

    { "全小",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr,
    nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, "全小",
    nullptr, nullptr, nullptr, nullptr, "全小三同刻", "全小三暗",
    nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, "全小推不倒", nullptr, nullptr, nullptr, "妙手全小", "海底全小", "杠开全小", "全小抢杠",
    "全小碰碰和", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    "全小全带幺", "全小不求人", nullptr, nullptr },  // 全小


    { "清龙",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr,
    nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    "清龙", nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, "妙手清龙", "海底清龙", "杠开清龙", "清龙抢杠",
    nullptr, "清龙混一色", nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, "清龙不求人", nullptr, "绝张清龙" },  // 清龙

    { "三色双龙会",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr,
    nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, "三色双龙会", nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, "妙手三色双龙", "海底三色双龙", nullptr, "三色双龙抢杠",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, "绝张三色双龙" },  // 三色双龙会

    { "一色三步高",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr,
    nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, "一色三步高", nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, "混一色三步", nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr },  // 一色三步高

    { "全带五",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr,
    nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, "全带五", nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, "推不倒全带五", nullptr, nullptr, nullptr, "妙手全带五", "海底全带五", "杠开全带五", "全带五抢杠",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, "全带五不求人", nullptr, nullptr },  // 全带五

    { "三同刻",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr,
    nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, "三同刻", "三同刻三暗",
    nullptr, nullptr, "大于五三同刻", "小于五三同刻", nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, "妙手三同刻", "海底三同刻", "杠开三同刻", "三同刻抢杠",
    "碰碰和三同刻", nullptr, nullptr, "五门三同刻", nullptr, nullptr, nullptr,
    "全带幺三同刻", nullptr, nullptr, nullptr },  // 三同刻

    { "三暗刻",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr,
    nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, "三暗刻",
    nullptr, nullptr, "大于五三暗", "小于五三暗", "三风刻三暗",
    nullptr, "推不倒三暗", nullptr, "三暗三色三", nullptr, "妙手三暗", "海底三暗", "杠开三暗", "三暗抢杠",
    "三暗碰碰和", "混一色三暗", nullptr, "五门三暗", nullptr, nullptr, nullptr,
    "三暗全带幺", "三暗不求人", nullptr, "绝张三暗" },  // 三暗刻


    { "全不靠",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr,
    nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    "全不靠", "全不靠组合龙", nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, "妙手全不靠", "海底全不靠", nullptr, "全不靠抢杠",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, "绝张全不靠" },  // 全不靠

    { "组合龙",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr,
    nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, "组合龙", nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, "妙手组合龙", "海底组合龙", "杠开组合龙", "组合龙抢杠",
    nullptr, nullptr, nullptr, "五门组合龙", nullptr, nullptr, nullptr,
    nullptr, "组合龙不求人", nullptr, "绝张组合龙" },  // 组合龙

    { "大于五",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr,
    nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, "大于五", nullptr, nullptr,
    nullptr, "大于五推不倒", nullptr, nullptr, nullptr, "妙手大于五", "海底大于五", "杠开大于五", "大于五抢杠",
    "大于五碰碰和", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, "大于五不求人", nullptr, "绝张大于五" },  // 大于五

    { "小于五",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr,
    nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, "小于五", nullptr,
    nullptr, "小于五推不倒", nullptr, nullptr, nullptr, "妙手小于五", "海底小于五", "杠开小于五", "小于五抢杠",
    "小于五碰碰和", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, "小于五不求人", nullptr, "绝张小于五" },  // 小于五

    { "三风刻",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr,
    nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, "三风刻",
    nullptr, nullptr, nullptr, nullptr, nullptr, "妙手三风刻", "海底三风刻", "杠开三风刻", "三风刻抢杠",
    "三风刻碰碰和", "三风刻混一色", nullptr, nullptr, nullptr, nullptr, nullptr,
    "三风刻全带幺", nullptr, nullptr, "绝张三风刻" },  // 三风刻


    { "花龙",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr,
    nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,
    "花龙", nullptr, nullptr, nullptr, nullptr, "妙手花龙", "海底花龙", "杠开花龙", "花龙抢杠",
    nullptr, nullptr, nullptr, "五门花龙", "花龙全求人", nullptr, nullptr,
    nullptr, "花龙不求人", nullptr, "绝张花龙" },  // 花龙

    { "推不倒",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr,
    nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, "推不倒", nullptr, nullptr, nullptr, "妙手推不倒", "海底推不倒", "杠开推不倒", "推不倒抢杠",
    "推不倒碰碰和", "推不倒混一色", nullptr, nullptr, "推不倒全求人", nullptr, nullptr,
    "推不倒全带幺", "推不倒不求人", nullptr, "绝张推不倒" },  // 推不倒

    { "三色三同顺",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr,
    nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, "三色三同顺", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, "五门三同顺", nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr },  // 三色三同顺

    { "三色三节高",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr,
    nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, "三色三节高", nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, "五门三节", nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr },  // 三色三节高

    { "无番和",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr,
    nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, "无番和", nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr },  // 无番和

    { "妙手回春",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr,
    nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, "妙手回春", nullptr, "妙手杠开", nullptr,
    "妙手碰碰和", "妙手混一色", nullptr, "妙手五门齐", nullptr, "妙手双暗杠", "妙手双箭刻",
    "妙手全带幺", "妙手不求人", nullptr, "妙手绝张" },  // 妙手回春

    { "海底捞月",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr,
    nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, "海底捞月", nullptr, "海底抢杠",
    "海底碰碰和", "海底混一色", nullptr, "海底五门齐", "海底全求人", nullptr, "海底双箭刻",
    "海底全带幺", nullptr, nullptr, "海底绝张" },  // 海底捞月

    { "杠上开花",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr,
    nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, "杠上开花", nullptr,
    "杠开碰碰和", "杠开混一色", nullptr, "杠开五门齐", nullptr, "双暗杠杠开", "杠开双箭刻",
    "杠开全带幺", "杠开不求人", "双明杠杠开", "杠开绝张" },  // 杠上开花

    { "抢杠和",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr,
    nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, "抢杠和",
    nullptr, "混一色抢杠", nullptr, "五门齐抢杠", nullptr, nullptr, "双箭刻抢杠",
    "全带幺抢杠", nullptr, nullptr, nullptr },  // 抢杠和


    { "碰碰和",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr,
    nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    "碰碰和", "碰碰和混一色", nullptr, "五门碰碰和", "碰碰和全求人", "碰碰和双暗杠", "碰碰和双箭刻",
    nullptr, nullptr, "碰碰和双明杠", nullptr },  // 碰碰和

    { "混一色",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr,
    nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, "混一色", nullptr, nullptr, "混一色全求人", nullptr, "混一色双箭刻",
    "混一色全带幺", "混一色不求人", "混一色双明杠", "绝张混一色" },  // 混一色

    { "三色三步高",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr,
    nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, "三色三步高", "五门三步", nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr },  // 三色三步高

    { "五门齐",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr,
    nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, "五门齐", "五门全求人", nullptr, nullptr,
    "五门全带幺", "五门不求人", "五门双明杠", "绝张五门齐" },  // 五门齐

    { "全求人",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr,
    nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, "全求人", nullptr, "全求人双箭刻",
    "全求人全带幺", nullptr, "全求人双明杠", nullptr },  // 全求人

    { "双暗杠",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr,
    nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr },  // 双暗杠

    { "双箭刻",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr,
    nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, "双箭刻",
    nullptr, nullptr, nullptr, nullptr },  // 双箭刻


    { "全带幺",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr,
    nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    "全带幺", "全带幺不求人", "全带幺双明杠", "绝张全带幺" },  // 全带幺

    { "不求人",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr,
    nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, "不求人", nullptr, "绝张不求人" },  // 不求人

    { "双明杠",
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr,
    nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, "双明杠", "绝张双明杠" },  // 双明杠

    { "和绝张" }  // 和绝张
};

const char *GetShortFanText(const Record::Detail &detail) {
    if (detail.score == 0) {
        return "荒庄";
    }

    uint64_t fanFlag = detail.fan_flag;
    if (fanFlag != 0) {
        // 选取标记的最大的两个番种显示出来
        unsigned fan0 = 0, fan1 = 0;
        for (unsigned n = mahjong::BIG_FOUR_WINDS; n < mahjong::DRAGON_PUNG; ++n) {
            if (TEST_FAN(fanFlag, n)) {
                fan0 = n;
                break;
            }
        }
        for (unsigned n = fan0 + 1; n < mahjong::DRAGON_PUNG; ++n) {
            if (TEST_FAN(fanFlag, n)) {
                fan1 = n;
                break;
            }
        }

        if (fan0 != 0) {
            const char *ret = fan_name2[fan0][fan1];
            return ret != nullptr ? ret : mahjong::fan_name[fan0];
        }
    }

    uint8_t packedFan = detail.packed_fan;
    if (packedFan > 0 && packedFan <= 8) {
        return packedFanNames[packedFan - 1];
    }

    return "未标记番种";
}

std::string GetLongFanText(const Record::Detail &detail) {
    std::string fanText;

    uint8_t packedFan = detail.packed_fan;
    uint64_t fanFlag = detail.fan_flag;
    if (fanFlag != 0) {
        for (unsigned n = mahjong::BIG_FOUR_WINDS; n < mahjong::DRAGON_PUNG; ++n) {
            if (TEST_FAN(fanFlag, n)) {
                unsigned idx = n;
                fanText.append("「");
                fanText.append(mahjong::fan_name[idx]);
                fanText.append("」");
            }
        }

        if (!fanText.empty()) {
            fanText.append("等");
        }
    }
    else if (packedFan > 0 && packedFan <= 8) {
        fanText.append("「");
        fanText.append(packedFanNames[packedFan - 1]);
        fanText.append("」");
    }

    return fanText;
}
