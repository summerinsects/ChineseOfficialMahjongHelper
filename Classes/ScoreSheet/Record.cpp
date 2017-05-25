#include "Record.h"
#include "../mahjong-algorithm/fan_calculator.h"

void JsonToRecord(const rapidjson::Value &json, Record &record) {
    memset(&record, 0, sizeof(Record));

    rapidjson::Value::ConstMemberIterator it = json.FindMember("name");
    if (it != json.MemberEnd() && it->value.IsArray()) {
        rapidjson::Value::ConstArray name = it->value.GetArray();
        if (name.Size() == 4) {
            for (int i = 0; i < 4; ++i) {
                strncpy(record.name[i], name[i].GetString(), name[i].GetStringLength());
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
                const rapidjson::Value::ConstObject &win_hand_json = it->value.GetObject();

                Record::Detail::WinHand &win_hand_data = detail_data.win_hand;
                it = win_hand_json.FindMember("fixed_packs");
                if (it != win_hand_json.MemberEnd() && it->value.IsArray()) {
                    rapidjson::Value::ConstArray fixed_packs = it->value.GetArray();
                    win_hand_data.pack_count = fixed_packs.Size();
                    for (uint8_t i = 0; i < win_hand_data.pack_count; ++i) {
                        win_hand_data.fixed_packs[i] = fixed_packs[i].GetUint();
                    }
                }

                it = win_hand_json.FindMember("standing_tiles");
                if (it != win_hand_json.MemberEnd() && it->value.IsArray()) {
                    rapidjson::Value::ConstArray standing_tiles = it->value.GetArray();
                    win_hand_data.tile_count = standing_tiles.Size();
                    for (uint8_t i = 0; i < win_hand_data.tile_count; ++i) {
                        win_hand_data.standing_tiles[i] = standing_tiles[i].GetUint();
                    }
                }

                it = win_hand_json.FindMember("win_tile");
                if (it != win_hand_json.MemberEnd() && it->value.IsUint()) {
                    win_hand_data.win_tile = it->value.GetUint();
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
    for (int i = 0; i < 4; ++i) {
        name.PushBack(rapidjson::StringRef(record.name[i]), alloc);
    }
    json.AddMember("name", std::move(name), alloc);

    rapidjson::Value detail(rapidjson::Type::kArrayType);
    for (size_t i = 0; i < record.current_index; ++i) {
        const Record::Detail &detail_data = record.detail[i];
        rapidjson::Value detail_json(rapidjson::Type::kObjectType);
        detail_json.AddMember("win_claim", rapidjson::Value(detail_data.win_claim), alloc);
        detail_json.AddMember("false_win", rapidjson::Value(detail_data.false_win), alloc);
        detail_json.AddMember("score", rapidjson::Value(detail_data.score), alloc);
        detail_json.AddMember("fan_flag", rapidjson::Value(detail_data.fan_flag), alloc);

        const Record::Detail::WinHand &win_hand_data = detail_data.win_hand;
        rapidjson::Value win_hand_json(rapidjson::Type::kObjectType);
        if (win_hand_data.pack_count > 0) {
            rapidjson::Value fixed_packs(rapidjson::Type::kArrayType);
            for (uint8_t i = 0; i < win_hand_data.pack_count; ++i) {
                fixed_packs.PushBack(rapidjson::Value(win_hand_data.fixed_packs[i]), alloc);
            }
            win_hand_json.AddMember("fixed_packs", std::move(fixed_packs), alloc);
        }
        if (win_hand_data.tile_count > 0) {
            rapidjson::Value standing_tiles(rapidjson::Type::kArrayType);
            for (uint8_t i = 0; i < win_hand_data.tile_count; ++i) {
                standing_tiles.PushBack(rapidjson::Value(win_hand_data.standing_tiles[i]), alloc);
            }
            win_hand_json.AddMember("standing_tiles", std::move(standing_tiles), alloc);
        }
        win_hand_json.AddMember("win_tile", rapidjson::Value(win_hand_data.win_tile), alloc);
        win_hand_json.AddMember("win_flag", rapidjson::Value(win_hand_data.win_flag), alloc);
        win_hand_json.AddMember("flower_count", rapidjson::Value(win_hand_data.flower_count), alloc);

        detail_json.AddMember("win_hand", std::move(win_hand_json), alloc);

        detail.PushBack(std::move(detail_json), alloc);
    }
    json.AddMember("detail", std::move(detail), alloc);

    json.AddMember("start_time", rapidjson::Value((uint64_t)record.start_time), alloc);
    json.AddMember("end_time", rapidjson::Value((uint64_t)record.end_time), alloc);
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

std::string GetFanText(const Record::Detail &detail) {
    const char *fanName = nullptr;
    if (detail.score == 0) {
        fanName = "荒庄";
    }

    // 选取标记的最大番种显示出来
    uint64_t fanFlag = detail.fan_flag;
    if (fanFlag != 0) {
        for (unsigned n = mahjong::BIG_FOUR_WINDS; n < mahjong::DRAGON_PUNG; ++n) {
            if (TEST_FAN(fanFlag, n)) {
                unsigned idx = n;
                fanName = mahjong::fan_name[idx];
                break;
            }
        }
    }

    // 将未标记番种的显示为其他凑番
    if (fanName == nullptr) {
        fanName = "其他凑番";
    }

    return fanName;
}
