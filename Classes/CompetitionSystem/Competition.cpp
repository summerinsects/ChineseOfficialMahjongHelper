#include "Competition.h"
#include <algorithm>
#include <iterator>
#include "json/document.h"
#include "json/stringbuffer.h"
#if defined(COCOS2D_DEBUG) && (COCOS2D_DEBUG > 0)
#include "json/prettywriter.h"
#else
#include "json/writer.h"
#endif
#include "../utils/common.h"

// 标准分转换为字符串
std::string CompetitionResult::standardScoreToString(float ss) {
    char str[64];  // FLT_MAX输出为340282346638528859811704183484516925440.00000010
    int len = snprintf(str, sizeof(str), "%.3f", ss);

    const char *dot = strchr(str, '.');  // 找到小数点
    if (LIKELY(dot != nullptr)) {
        char *p = str + len - 1;  // 反向查找'0'
        while (*p == '0') --p;
        if (p != dot) ++p;
        *p = '\0';  // 截断
    }

    return str;
}

// 获取指定一轮总成绩
std::pair<float, int> CompetitionPlayer::getTotalScoresByRound(size_t round) const {
    float ss = 0;
    int cs = 0;
    for (size_t i = 0; i <= round; ++i) {
        ss += competition_results[i].standard_score;
        cs += competition_results[i].competition_score;
    }
    return std::make_pair(ss, cs);
}

// 获取指定一轮单轮成绩
std::pair<float, int> CompetitionPlayer::getCurrentScoresByRound(size_t round) const {
    return std::make_pair(competition_results[round].standard_score, competition_results[round].competition_score);
}

namespace {
    // 排序附加信息，用来保存标准分和比赛分，使之仅计算一次
    struct ScoresSortInfo {
        const CompetitionPlayer *player = nullptr;
        float standard_score = 0.0f;
        int competition_score = 0;
        unsigned rank_cnt[4];
        int max_score;
    };
}

// 高高碰排序
void CompetitionRound::sortPlayers(size_t round, const std::vector<CompetitionPlayer> &players, std::vector<const CompetitionPlayer *> &output) {
    // 1. 逐个计算标准分和比赛分
    std::vector<ScoresSortInfo> temp;
    temp.reserve(players.size());
    std::transform(players.begin(), players.end(), std::back_inserter(temp), [round](const CompetitionPlayer &player) {
        ScoresSortInfo ret;
        memset(&ret, 0, sizeof(ret));
        ret.player = &player;
        for (size_t i = 0; i <= round; ++i) {
            float ss = player.competition_results[i].standard_score;
            int cs = player.competition_results[i].competition_score;
            unsigned rank = player.competition_results[i].rank;

            ret.standard_score += ss;
            ret.competition_score += cs;
            ++ret.rank_cnt[rank - 1];
            ret.max_score = std::max(ret.max_score, cs);
        }

        return ret;
    });

    // 2. 转换为指针数组
    std::vector<ScoresSortInfo *> ptemp;
    ptemp.reserve(temp.size());
    std::transform(temp.begin(), temp.end(), std::back_inserter(ptemp), [](ScoresSortInfo &ssi) { return &ssi; });

    // 3. 排序指针数组
    std::sort(ptemp.begin(), ptemp.end(), [](const ScoresSortInfo *a, const ScoresSortInfo *b) {
        const float ss1 = a->standard_score, ss2 = b->standard_score;
        if (ss1 > ss2) return true;  // 标准分
        if (ss1 == ss2) {
            const int cs1 = a->competition_score, cs2 = b->competition_score;
            if (cs1 > cs2) return true;  // 比赛分
            if (cs1 == cs2) {
                if (a->rank_cnt[0] > b->rank_cnt[0]) return true;  // 1位数
                if (a->rank_cnt[0] == b->rank_cnt[0]) {
                    if (a->rank_cnt[1] > b->rank_cnt[1]) return true;  // 2位数
                    if (a->rank_cnt[1] == b->rank_cnt[1]) {
                        if (a->rank_cnt[2] > b->rank_cnt[2]) return true;  // 3位数
                        if (a->rank_cnt[2] == b->rank_cnt[2]) {
                            if (a->rank_cnt[3] > b->rank_cnt[3]) return true;  // 4位数
                            if (a->rank_cnt[3] == b->rank_cnt[3]) {
                                if (a->max_score > b->max_score) return true;  // 单局最高分
                                if (a->max_score == b->max_score) {
                                    return a->player->serial < b->player->serial;  // 编号
                                }
                            }
                        }
                    }
                }
            }
        }
        return false;
    });

    // 4. 输出
    output.clear();
    output.reserve(ptemp.size());
    std::transform(ptemp.begin(), ptemp.end(), std::back_inserter(output), [](ScoresSortInfo *pssi) { return pssi->player; });
}

// 准备
void CompetitionData::prepare(const std::string &name, size_t player, size_t round) {
    this->name = name;
    teams.clear();
    teams.shrink_to_fit();
    rounds.clear();
    rounds.shrink_to_fit();
    rounds.reserve(round);
    round_count = round;
    start_time = time(nullptr);
    finish_time = 0;

    std::vector<CompetitionPlayer> temp(player);
    for (size_t i = 0; i < player; ++i) {
        temp[i].serial = i;
    }
    players.swap(temp);
}

// 报名是否截止
bool CompetitionData::isEnrollmentOver() const {
    return std::all_of(players.begin(), players.end(), [](const CompetitionPlayer &p) { return !p.name.empty(); });
}

// 开始新一轮
bool CompetitionData::startNewRound() {
    if (rounds.size() >= round_count) {
        return false;
    }

    rounds.emplace_back();
    std::for_each(players.begin(), players.end(), [](CompetitionPlayer &player) {
        player.competition_results.emplace_back();
    });

    const size_t cnt = players.size() / 4;
    std::vector<CompetitionTable> &tables = rounds.back().tables;
    tables.resize(cnt);
    for (size_t i = 0; i < cnt; ++i) {
        tables[i].serial = i;
    }

    return true;
}

// 一轮是否已经开始
bool CompetitionData::isRoundStarted(size_t round) const {
    return std::any_of(players.begin(), players.end(), [round](const CompetitionPlayer &player) {
        return player.competition_results[round].rank != 0;
    });
}

// 一轮是否已经结束
bool CompetitionData::isRoundFinished(size_t round) const {
    return std::all_of(players.begin(), players.end(), [round](const CompetitionPlayer &player) {
        return player.competition_results[round].rank != 0;
    });
}

// 按编号排桌
void CompetitionData::rankTablesBySerial(size_t round) {
    const size_t cnt = players.size();
    std::vector<CompetitionTable> &tables = rounds[round].tables;
    tables.resize(cnt / 4);
    for (size_t i = 0; i < cnt; ) {
        size_t serial = i / 4;
        CompetitionTable &table = tables[serial];
        table.serial = serial;
        table.player_indices[0] = i++;
        table.player_indices[1] = i++;
        table.player_indices[2] = i++;
        table.player_indices[3] = i++;
    }
}

// 随机排桌
void CompetitionData::rankTablesByRandom(size_t round) {
    srand(static_cast<unsigned>(time(nullptr)));

    const size_t cnt = players.size();
    std::vector<CompetitionTable> &tables = rounds[round].tables;
    tables.resize(cnt / 4);

    std::vector<CompetitionPlayer *> temp;
    temp.reserve(players.size());
    std::transform(players.begin(), players.end(), std::back_inserter(temp), [](CompetitionPlayer &p) { return &p; });
    std::random_shuffle(temp.begin(), temp.end());

    for (size_t i = 0; i < cnt; ) {
        size_t serial = i / 4;
        CompetitionTable &table = tables[serial];
        table.serial = serial;
        table.player_indices[0] = temp[i++] - &players[0];
        table.player_indices[1] = temp[i++] - &players[0];
        table.player_indices[2] = temp[i++] - &players[0];
        table.player_indices[3] = temp[i++] - &players[0];
    }
}

// 高高碰排桌
void CompetitionData::rankTablesByScores(size_t round) {
    const size_t cnt = players.size();
    std::vector<CompetitionTable> &tables = rounds[round].tables;
    tables.resize(cnt / 4);

    std::vector<const CompetitionPlayer *> output;
    CompetitionRound::sortPlayers(round, players, output);

    for (size_t i = 0; i < cnt; ) {
        size_t serial = i / 4;
        CompetitionTable &table = tables[serial];
        table.serial = serial;
        table.player_indices[0] = output[i++] - &players[0];
        table.player_indices[1] = output[i++] - &players[0];
        table.player_indices[2] = output[i++] - &players[0];
        table.player_indices[3] = output[i++] - &players[0];
    }
}

// 蛇形排桌
void CompetitionData::rankTablesBySnake(size_t round) {
    const size_t cnt = players.size();
    std::vector<CompetitionTable> &tables = rounds[round].tables;
    tables.resize(cnt / 4);

    std::vector<const CompetitionPlayer *> output;
    CompetitionRound::sortPlayers(round, players, output);

    size_t east = 0;
    size_t south = cnt / 2 - 1;
    size_t west = south + 1;
    size_t north = cnt - 1;
    for (size_t i = 0; i < cnt; i += 4) {
        size_t serial = i / 4;
        CompetitionTable &table = tables[serial];
        table.serial = serial;
        table.player_indices[0] = output[east++] - &players[0];
        table.player_indices[1] = output[south--] - &players[0];
        table.player_indices[2] = output[west++] - &players[0];
        table.player_indices[3] = output[north--] - &players[0];
    }
}

namespace {

void CompetitionResultFromJson(const rapidjson::Value &json, CompetitionResult &result) {
    rapidjson::Value::ConstMemberIterator it = json.FindMember("rank");
    if (it != json.MemberEnd() && it->value.IsUint()) {
        result.rank = it->value.GetUint();
    }

    it = json.FindMember("standard_score");
    if (it != json.MemberEnd() && it->value.IsFloat()) {
        result.standard_score = it->value.GetFloat();
    }

    it = json.FindMember("competition_score");
    if (it != json.MemberEnd() && it->value.IsInt()) {
        result.competition_score = it->value.GetInt();
    }
}

void CompetitionResultToJson(const CompetitionResult &result, rapidjson::Value &json, rapidjson::Value::AllocatorType &alloc) {
    json.AddMember("rank", rapidjson::Value(result.rank), alloc);
    json.AddMember("standard_score", rapidjson::Value(result.standard_score), alloc);
    json.AddMember("competition_score", rapidjson::Value(result.competition_score), alloc);
}

void CompetitionPlayerFromJson(const rapidjson::Value &json, CompetitionPlayer &player) {
    rapidjson::Value::ConstMemberIterator it = json.FindMember("serial");
    if (it != json.MemberEnd() && it->value.IsUint()) {
        player.serial = it->value.GetUint();
    }

    it = json.FindMember("name");
    if (it != json.MemberEnd() && it->value.IsString()) {
        player.name = it->value.GetString();
    }

    it = json.FindMember("competition_results");
    if (it != json.MemberEnd() && it->value.IsArray()) {
        rapidjson::Value::ConstArray results = it->value.GetArray();
        player.competition_results.clear();
        player.competition_results.reserve(results.Size());
        std::for_each(results.Begin(), results.End(), [&player](const rapidjson::Value &json) {
            player.competition_results.emplace_back();
            CompetitionResultFromJson(json, player.competition_results.back());
        });
    }

    it = json.FindMember("team_index");
    if (it != json.MemberEnd() && it->value.IsInt64()) {
        player.team_index = static_cast<ptrdiff_t>(it->value.GetInt64());
    }
}

void CompetitionPlayerToJson(const CompetitionPlayer &player, rapidjson::Value &json, rapidjson::Value::AllocatorType &alloc) {
    json.AddMember("serial", rapidjson::Value(static_cast<uint64_t>(player.serial)), alloc);
    json.AddMember("name", rapidjson::StringRef(player.name.c_str()), alloc);

    rapidjson::Value results(rapidjson::Type::kArrayType);
    results.Reserve(static_cast<rapidjson::SizeType>(player.competition_results.size()), alloc);
    std::for_each(player.competition_results.begin(), player.competition_results.end(), [&results, &alloc](const CompetitionResult &result) {
        rapidjson::Value json(rapidjson::Type::kObjectType);
        CompetitionResultToJson(result, json, alloc);
        results.PushBack(std::move(json), alloc);
    });
    json.AddMember("competition_results", std::move(results), alloc);

    json.AddMember("team_index", rapidjson::Value(static_cast<int64_t>(player.team_index)), alloc);
}

void CompetitionTeamFromJson(const rapidjson::Value &json, CompetitionTeam &team) {
    rapidjson::Value::ConstMemberIterator it = json.FindMember("serial");
    if (it != json.MemberEnd() && it->value.IsUint()) {
        team.serial = it->value.GetUint();
    }

    it = json.FindMember("name");
    if (it != json.MemberEnd() && it->value.IsString()) {
        team.name = it->value.GetString();
    }

    it = json.FindMember("player_indices");
    if (it != json.MemberEnd() && it->value.IsArray()) {
        rapidjson::Value::ConstArray indices = it->value.GetArray();
        team.player_indices.clear();
        team.player_indices.reserve(indices.Size());
        std::for_each(indices.Begin(), indices.End(), [&team](const rapidjson::Value &json) {
            team.player_indices.push_back(static_cast<ptrdiff_t>(json.GetInt64()));
        });
    }
}

void CompetitionTeamToJson(const CompetitionTeam &team, rapidjson::Value &json, rapidjson::Value::AllocatorType &alloc) {
    json.AddMember("serial", rapidjson::Value(static_cast<uint64_t>(team.serial)), alloc);
    json.AddMember("name", rapidjson::StringRef(team.name.c_str()), alloc);

    rapidjson::Value indices(rapidjson::Type::kArrayType);
    indices.Reserve(static_cast<rapidjson::SizeType>(team.player_indices.size()), alloc);
    std::for_each(team.player_indices.begin(), team.player_indices.end(), [&indices, &alloc](ptrdiff_t idx) {
        indices.PushBack(rapidjson::Value(static_cast<int64_t>(idx)), alloc);
    });
    json.AddMember("player_indices", std::move(indices), alloc);
}

void CompetitionTableFromJson(const rapidjson::Value &json, CompetitionTable &table) {
    rapidjson::Value::ConstMemberIterator it = json.FindMember("serial");
    if (it != json.MemberEnd() && it->value.IsUint()) {
        table.serial = it->value.GetUint();
    }

    it = json.FindMember("player_indices");
    if (it != json.MemberEnd() && it->value.IsArray()) {
        rapidjson::Value::ConstArray indices = it->value.GetArray();
        memset(table.player_indices, -1, sizeof(table.player_indices));

        rapidjson::SizeType i = 0, cnt = indices.Size();
        for (; i < cnt; ++i) {
            const rapidjson::Value &value = indices[i];
            if (value.IsInt64()) {
                table.player_indices[i] = value.GetInt64();
            }
        }
    }
}

void CompetitionTableToJson(const CompetitionTable &table, rapidjson::Value &json, rapidjson::Value::AllocatorType &alloc) {
    json.AddMember("serial", rapidjson::Value(static_cast<uint64_t>(table.serial)), alloc);

    rapidjson::Value indices(rapidjson::Type::kArrayType);
    indices.Reserve(4, alloc);
    std::for_each(std::begin(table.player_indices), std::end(table.player_indices), [&indices, &alloc](ptrdiff_t idx) {
        indices.PushBack(rapidjson::Value(static_cast<int64_t>(idx)), alloc);
    });
    json.AddMember("player_indices", std::move(indices), alloc);
}

void CompetitionRoundFromJson(const rapidjson::Value &json, CompetitionRound &round) {
    rapidjson::Value::ConstMemberIterator it = json.FindMember("tables");
    if (it != json.MemberEnd() && it->value.IsArray()) {
        rapidjson::Value::ConstArray tables = it->value.GetArray();
        std::for_each(tables.Begin(), tables.End(), [&round](const rapidjson::Value &json) {
            round.tables.emplace_back();
            CompetitionTableFromJson(json, round.tables.back());
        });
    }
}

void CompetitionRoundToJson(const CompetitionRound &round, rapidjson::Value &json, rapidjson::Value::AllocatorType &alloc) {
    rapidjson::Value tables(rapidjson::Type::kArrayType);
    tables.Reserve(static_cast<rapidjson::SizeType>(round.tables.size()), alloc);
    std::for_each(round.tables.begin(), round.tables.end(), [&tables, &alloc](const CompetitionTable &table) {
        rapidjson::Value json(rapidjson::Type::kObjectType);
        CompetitionTableToJson(table, json, alloc);
        tables.PushBack(std::move(json), alloc);
    });
    json.AddMember("tables", std::move(tables), alloc);
}

void CompetitionDataFromJson(const rapidjson::Value &json, CompetitionData &data) {
    rapidjson::Value::ConstMemberIterator it = json.FindMember("name");
    if (it != json.MemberEnd() && it->value.IsString()) {
        data.name = it->value.GetString();
    }

    it = json.FindMember("players");
    if (it != json.MemberEnd() && it->value.IsArray()) {
        rapidjson::Value::ConstArray players = it->value.GetArray();
        data.players.clear();
        data.players.reserve(players.Size());
        std::for_each(players.Begin(), players.End(), [&data](const rapidjson::Value &json) {
            data.players.emplace_back();
            CompetitionPlayerFromJson(json, data.players.back());
        });
    }

    it = json.FindMember("teams");
    if (it != json.MemberEnd() && it->value.IsArray()) {
        rapidjson::Value::ConstArray teams = it->value.GetArray();
        data.teams.clear();
        data.teams.reserve(teams.Size());
        std::for_each(teams.Begin(), teams.End(), [&data](const rapidjson::Value &json) {
            data.teams.emplace_back();
            CompetitionTeamFromJson(json, data.teams.back());
        });
    }

    it = json.FindMember("rounds");
    if (it != json.MemberEnd() && it->value.IsArray()) {
        rapidjson::Value::ConstArray players = it->value.GetArray();
        data.rounds.clear();
        data.rounds.reserve(players.Size());
        std::for_each(players.Begin(), players.End(), [&data](const rapidjson::Value &json) {
            data.rounds.emplace_back();
            CompetitionRoundFromJson(json, data.rounds.back());
        });
    }

    it = json.FindMember("round_count");
    if (it != json.MemberEnd() && it->value.IsUint64()) {
        data.round_count = it->value.GetUint64();
    }

    it = json.FindMember("start_time");
    if (it != json.MemberEnd() && it->value.IsUint64()) {
        data.start_time = static_cast<time_t>(it->value.GetUint64());
    }

    it = json.FindMember("finish_time");
    if (it != json.MemberEnd() && it->value.IsUint()) {
        data.finish_time = static_cast<time_t>(it->value.GetUint64());
    }
}

void CompetitionDataToJson(const CompetitionData &data, rapidjson::Value &json, rapidjson::Value::AllocatorType &alloc) {
    json.AddMember("name", rapidjson::StringRef(data.name.c_str()), alloc);

    rapidjson::Value players(rapidjson::Type::kArrayType);
    players.Reserve(static_cast<rapidjson::SizeType>(data.players.size()), alloc);
    std::for_each(data.players.begin(), data.players.end(), [&players, &alloc](const CompetitionPlayer &player) {
        rapidjson::Value json(rapidjson::Type::kObjectType);
        CompetitionPlayerToJson(player, json, alloc);
        players.PushBack(std::move(json), alloc);
    });
    json.AddMember("players", std::move(players), alloc);

    rapidjson::Value teams(rapidjson::Type::kArrayType);
    teams.Reserve(static_cast<rapidjson::SizeType>(data.teams.size()), alloc);
    std::for_each(data.teams.begin(), data.teams.end(), [&teams, &alloc](const CompetitionTeam &team) {
        rapidjson::Value json(rapidjson::Type::kObjectType);
        CompetitionTeamToJson(team, json, alloc);
        teams.PushBack(std::move(json), alloc);
    });
    json.AddMember("teams", std::move(teams), alloc);

    rapidjson::Value rounds(rapidjson::Type::kArrayType);
    rounds.Reserve(static_cast<rapidjson::SizeType>(data.rounds.size()), alloc);
    std::for_each(data.rounds.begin(), data.rounds.end(), [&rounds, &alloc](const CompetitionRound &round) {
        rapidjson::Value json(rapidjson::Type::kObjectType);
        CompetitionRoundToJson(round, json, alloc);
        rounds.PushBack(std::move(json), alloc);
    });
    json.AddMember("rounds", std::move(rounds), alloc);

    json.AddMember("round_count", rapidjson::Value(static_cast<uint64_t>(data.round_count)), alloc);
    json.AddMember("start_time", rapidjson::Value(static_cast<uint64_t>(data.start_time)), alloc);
    json.AddMember("finish_time", rapidjson::Value(static_cast<uint64_t>(data.finish_time)), alloc);
}

void SortCompetitions(std::vector<CompetitionData> &competitions) {
    // 用指针排序
    std::vector<CompetitionData *> ptrs(competitions.size());
    std::transform(competitions.begin(), competitions.end(), ptrs.begin(), [](CompetitionData &c) { return &c; });
    std::sort(ptrs.begin(), ptrs.end(), [](const CompetitionData *c1, const CompetitionData *c2) { return c1->start_time > c2->start_time; });
    std::vector<CompetitionData> temp;
    temp.reserve(competitions.size());
    std::for_each(ptrs.begin(), ptrs.end(), [&temp](CompetitionData *c) {
        temp.emplace_back();
        std::swap(temp.back(), *c);
    });
    competitions.swap(temp);
}

}

// 从文件中读
bool CompetitionData::readFromFile() {
    std::string str;
    FILE *fp = fopen(associated_file.c_str(), "rb");
    if (LIKELY(fp != nullptr)) {
        fseek(fp, 0, SEEK_END);
        long size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        try {
            str.resize(size + 1);
            fread(&str[0], sizeof(char), size, fp);
        }
        catch (...) {
        }
        fclose(fp);
    }

    try {
        MYLOG("%s", str.c_str());
        rapidjson::Document doc;
        doc.Parse<0>(str.c_str());
        if (doc.HasParseError()) {
            return false;
        }

        CompetitionDataFromJson(doc, *this);
        return true;
    }
    catch (std::exception &e) {
        MYLOG("%s %s", __FUNCTION__, e.what());
        return false;
    }
}

// 写入到文件
bool CompetitionData::writeToFile() const {
    try {
        rapidjson::Document doc(rapidjson::Type::kObjectType);
        CompetitionDataToJson(*this, doc, doc.GetAllocator());

        rapidjson::StringBuffer buf;
#if defined(COCOS2D_DEBUG) && (COCOS2D_DEBUG > 0)
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buf);
#else
        rapidjson::Writer<rapidjson::StringBuffer> writer(buf);
#endif
        doc.Accept(writer);

        MYLOG("%.*s", static_cast<int>(buf.GetSize()), buf.GetString());

        if (modify_callback) {
            modify_callback(this);
        }

        FILE *fp = fopen(associated_file.c_str(), "wb");
        if (LIKELY(fp != nullptr)) {
            fwrite(buf.GetString(), sizeof(char), buf.GetSize(), fp);
            fclose(fp);
            return true;
        }

        return false;
    }
    catch (std::exception &e) {
        MYLOG("%s %s", __FUNCTION__, e.what());
        return false;
    }
}

void LoadHistoryCompetitions(const char *file, std::vector<CompetitionData> &competitions) {
    std::string str = Common::getStringFromFile(file);

    try {
        rapidjson::Document doc;
        doc.Parse<0>(str.c_str());
        if (doc.HasParseError() || !doc.IsArray()) {
            return;
        }

        competitions.clear();
        competitions.reserve(doc.Size());
        std::transform(doc.Begin(), doc.End(), std::back_inserter(competitions), [](const rapidjson::Value &json) {
            CompetitionData data;
            CompetitionDataFromJson(json, data);
            return data;
        });
        SortCompetitions(competitions);
    }
    catch (std::exception &e) {
        MYLOG("%s %s", __FUNCTION__, e.what());
    }
}

void SaveHistoryCompetitions(const char *file, const std::vector<CompetitionData> &competitions) {
    FILE *fp = fopen(file, "wb");
    if (LIKELY(fp != nullptr)) {
        try {
            rapidjson::Document doc(rapidjson::Type::kArrayType);
            doc.Reserve(static_cast<rapidjson::SizeType>(competitions.size()), doc.GetAllocator());
            std::for_each(competitions.begin(), competitions.end(), [&doc](const CompetitionData &data) {
                rapidjson::Value json(rapidjson::Type::kObjectType);
                CompetitionDataToJson(data, json, doc.GetAllocator());
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

void ModifyCompetitionInHistory(std::vector<CompetitionData> &competitions, const CompetitionData *data) {
    auto it = std::find_if(competitions.begin(), competitions.end(), [data](const CompetitionData &c) {
        return (c.start_time == data->start_time && c.name == data->name);
    });

    if (it == competitions.end()) {
        competitions.push_back(*data);
    }
    else {
        *it = *data;
    }

    SortCompetitions(competitions);
}
