#include "Competition.h"
#include <sstream>
#include <algorithm>
#include "json/stringbuffer.h"
#include "json/prettywriter.h"
#include "../common.h"

// 标准分转换为字符串
std::string CompetitionResult::standardScoreToString(float ss) {
    std::ostringstream os;
    os << ss;
    std::string ret1 = os.str();
    std::string ret2 = Common::format<32>("%.3f", ss);
    return ret1.length() < ret2.length() ? ret1 : ret2;
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

// 排序附加信息，用来保存标准分和比赛分，使之仅计算一次
struct ScoresSortInfo {
    const CompetitionPlayer *player = nullptr;
    float standard_score = 0.0f;
    int competition_score = 0;
};

// 高高碰排序
void CompetitionRound::sortPlayers(unsigned round, const std::vector<CompetitionPlayer> &players, std::vector<const CompetitionPlayer *> &output) {
    // 1. 逐个计算标准分和比赛分
    std::vector<ScoresSortInfo> temp;
    temp.reserve(players.size());
    std::transform(players.begin(), players.end(), std::back_inserter(temp), [round](const CompetitionPlayer &player) {
        ScoresSortInfo ret;
        ret.player = &player;
        auto s = player.getTotalScoresByRound(round);
        ret.standard_score = s.first;
        ret.competition_score = s.second;
        return ret;
    });

    // 2. 转换为指针数组
    std::vector<ScoresSortInfo *> ptemp;
    ptemp.reserve(temp.size());
    std::transform(temp.begin(), temp.end(), std::back_inserter(ptemp), [](ScoresSortInfo &ssi) { return &ssi; });

    // 3. 排序指针数组
    std::sort(ptemp.begin(), ptemp.end(), [](const ScoresSortInfo *a, const ScoresSortInfo *b) {
        if (a->standard_score > b->standard_score) return true;
        if (a->standard_score == b->standard_score && a->competition_score > b->competition_score) return true;
        return false;
    });

    // 4. 输出
    output.clear();
    output.reserve(ptemp.size());
    std::transform(ptemp.begin(), ptemp.end(), std::back_inserter(output), [](ScoresSortInfo *pssi) { return pssi->player; });
}

// 准备
void CompetitionData::prepare(const std::string &name, unsigned player, unsigned round) {
    this->name = name;
    players.swap(std::vector<CompetitionPlayer>(player));
    teams.clear();
    rounds.swap(std::vector<CompetitionRound>(round));
    current_round = 0;
    start_time = time(nullptr);
    finish_time = 0;

    for (size_t i = 0, cnt = players.size(); i < cnt; ++i) {
        CompetitionPlayer &player = players[i];
        player.serial = static_cast<unsigned>(1 + i);
        player.competition_results.resize(round);
    }
}

// 报名是否截止
bool CompetitionData::isEnrollmentOver() const {
    return std::all_of(players.begin(), players.end(), [](const CompetitionPlayer &p) { return !p.name.empty(); });
}

// 一轮是否已经开始
bool CompetitionData::isRoundStarted(unsigned round) const {
    return std::any_of(players.begin(), players.end(), [round](const CompetitionPlayer &player) {
        return player.competition_results[round].rank != 0;
    });
}

// 一轮是否已经结束
bool CompetitionData::isRoundFinished(unsigned round) const {
    return std::all_of(players.begin(), players.end(), [round](const CompetitionPlayer &player) {
        return player.competition_results[round].rank != 0;
    });
}

// 按编号排桌
void CompetitionData::rankTablesBySerial(unsigned round) {
    const size_t cnt = players.size();
    std::vector<CompetitionTable> &tables = rounds[round].tables;
    tables.resize(cnt / 4);
    for (size_t i = 0; i < cnt; ) {
        CompetitionTable &table = tables[i / 4];
        table.player_indices[0] = i++;
        table.player_indices[1] = i++;
        table.player_indices[2] = i++;
        table.player_indices[3] = i++;
    }
}

// 按编号蛇形排桌
void CompetitionData::rankTablesBySerialSnake(unsigned round) {
    const size_t cnt = players.size();
    std::vector<CompetitionTable> &tables = rounds[round].tables;
    tables.resize(cnt / 4);

    size_t east = 0;
    size_t south = cnt / 2 - 1;
    size_t west = south + 1;
    size_t north = cnt - 1;
    for (size_t i = 0; i < cnt; i += 4) {
        CompetitionTable &table = tables[i / 4];
        table.player_indices[0] = east++;
        table.player_indices[1] = south--;
        table.player_indices[2] = west++;
        table.player_indices[3] = north--;
    }
}

// 随机排桌
void CompetitionData::rankTablesByRandom(unsigned round) {
    const size_t cnt = players.size();
    std::vector<CompetitionTable> &tables = rounds[round].tables;
    tables.resize(cnt / 4);

    std::vector<CompetitionPlayer *> temp;
    temp.reserve(players.size());
    std::transform(players.begin(), players.end(), std::back_inserter(temp), [](CompetitionPlayer &p) { return &p; });
    std::random_shuffle(temp.begin(), temp.end());
    
    for (size_t i = 0; i < cnt; ) {
        CompetitionTable &table = tables[i / 4];
        table.player_indices[0] = temp[i++] - &players[0];
        table.player_indices[1] = temp[i++] - &players[0];
        table.player_indices[2] = temp[i++] - &players[0];
        table.player_indices[3] = temp[i++] - &players[0];
    }
}

// 高高碰排桌
void CompetitionData::rankTablesByScores(unsigned round) {
    const size_t cnt = players.size();
    std::vector<CompetitionTable> &tables = rounds[round].tables;
    tables.resize(cnt / 4);
    
    std::vector<const CompetitionPlayer *> output;
    CompetitionRound::sortPlayers(round, players, output);

    for (size_t i = 0; i < cnt; ) {
        CompetitionTable &table = tables[i / 4];
        table.player_indices[0] = output[i++] - &players[0];
        table.player_indices[1] = output[i++] - &players[0];
        table.player_indices[2] = output[i++] - &players[0];
        table.player_indices[3] = output[i++] - &players[0];
    }
}

// 蛇形名次排桌
void CompetitionData::rankTablesByScoresSnake(unsigned round) {
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
        CompetitionTable &table = tables[i / 4];
        table.player_indices[0] = output[east++] - &players[0];
        table.player_indices[1] = output[south--] - &players[0];
        table.player_indices[2] = output[west++] - &players[0];
        table.player_indices[3] = output[north--] - &players[0];
    }
}



void CompetitionResult::fromJson(const rapidjson::Value &json, CompetitionResult &result) {
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

void CompetitionResult::toJson(const CompetitionResult &result, rapidjson::Value &json, rapidjson::Value::AllocatorType &alloc) {
    json.AddMember("rank", rapidjson::Value(result.rank), alloc);
    json.AddMember("standard_score", rapidjson::Value(result.standard_score), alloc);
    json.AddMember("competition_score", rapidjson::Value(result.competition_score), alloc);
}

void CompetitionPlayer::fromJson(const rapidjson::Value &json, CompetitionPlayer &player) {
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
            player.competition_results.push_back(CompetitionResult());
            CompetitionResult::fromJson(json, player.competition_results.back());
        });
    }

    it = json.FindMember("team_index");
    if (it != json.MemberEnd() && it->value.IsInt64()) {
        player.team_index = static_cast<ptrdiff_t>(it->value.GetInt64());
    }
}

void CompetitionPlayer::toJson(const CompetitionPlayer &player, rapidjson::Value &json, rapidjson::Value::AllocatorType &alloc) {
    json.AddMember("serial", rapidjson::Value(player.serial), alloc);
    json.AddMember("name", rapidjson::StringRef(player.name.c_str()), alloc);

    rapidjson::Value results(rapidjson::Type::kArrayType);
    results.Reserve(static_cast<rapidjson::SizeType>(player.competition_results.size()), alloc);
    std::for_each(player.competition_results.begin(), player.competition_results.end(), [&results, &alloc](const CompetitionResult &result) {
        rapidjson::Value json(rapidjson::Type::kObjectType);
        CompetitionResult::toJson(result, json, alloc);
        results.PushBack(std::move(json), alloc);
    });
    json.AddMember("competition_results", std::move(results), alloc);

    json.AddMember("team_index", rapidjson::Value(static_cast<int64_t>(player.team_index)), alloc);
}

void CompetitionTeam::fromJson(const rapidjson::Value &json, CompetitionTeam &team) {
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

void CompetitionTeam::toJson(const CompetitionTeam &team, rapidjson::Value &json, rapidjson::Value::AllocatorType &alloc) {
    json.AddMember("serial", rapidjson::Value(team.serial), alloc);
    json.AddMember("name", rapidjson::StringRef(team.name.c_str()), alloc);

    rapidjson::Value indices(rapidjson::Type::kArrayType);
    indices.Reserve(static_cast<rapidjson::SizeType>(team.player_indices.size()), alloc);
    std::for_each(team.player_indices.begin(), team.player_indices.end(), [&indices, &alloc](ptrdiff_t idx) {
        indices.PushBack(rapidjson::Value(static_cast<int64_t>(idx)), alloc);
    });
    json.AddMember("player_indices", std::move(indices), alloc);
}

void CompetitionTable::fromJson(const rapidjson::Value &json, CompetitionTable &table) {
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

void CompetitionTable::toJson(const CompetitionTable &table, rapidjson::Value &json, rapidjson::Value::AllocatorType &alloc) {
    json.AddMember("serial", rapidjson::Value(table.serial), alloc);

    rapidjson::Value indices(rapidjson::Type::kArrayType);
    indices.Reserve(4, alloc);
    std::for_each(std::begin(table.player_indices), std::end(table.player_indices), [&indices, &alloc](ptrdiff_t idx) {
        indices.PushBack(rapidjson::Value(static_cast<int64_t>(idx)), alloc);
    });
    json.AddMember("player_indices", std::move(indices), alloc);
}

void CompetitionRound::fromJson(const rapidjson::Value &json, CompetitionRound &round) {
    rapidjson::Value::ConstMemberIterator it = json.FindMember("tables");
    if (it != json.MemberEnd() && it->value.IsArray()) {
        rapidjson::Value::ConstArray tables = it->value.GetArray();
        std::for_each(tables.Begin(), tables.End(), [&round](const rapidjson::Value &json) {
            round.tables.push_back(CompetitionTable());
            CompetitionTable::fromJson(json, round.tables.back());
        });
    }
}

void CompetitionRound::toJson(const CompetitionRound &round, rapidjson::Value &json, rapidjson::Value::AllocatorType &alloc) {
    rapidjson::Value tables(rapidjson::Type::kArrayType);
    tables.Reserve(static_cast<rapidjson::SizeType>(round.tables.size()), alloc);
    std::for_each(round.tables.begin(), round.tables.end(), [&tables, &alloc](const CompetitionTable &table) {
        rapidjson::Value json(rapidjson::Type::kObjectType);
        CompetitionTable::toJson(table, json, alloc);
        tables.PushBack(std::move(json), alloc);
    });
    json.AddMember("tables", std::move(tables), alloc);
}

void CompetitionData::fromJson(const rapidjson::Value &json, CompetitionData &data) {
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
            data.players.push_back(CompetitionPlayer());
            CompetitionPlayer::fromJson(json, data.players.back());
        });
    }

    it = json.FindMember("teams");
    if (it != json.MemberEnd() && it->value.IsArray()) {
        rapidjson::Value::ConstArray teams = it->value.GetArray();
        data.teams.clear();
        data.teams.reserve(teams.Size());
        std::for_each(teams.Begin(), teams.End(), [&data](const rapidjson::Value &json) {
            data.teams.push_back(CompetitionTeam());
            CompetitionTeam::fromJson(json, data.teams.back());
        });
    }

    it = json.FindMember("rounds");
    if (it != json.MemberEnd() && it->value.IsArray()) {
        rapidjson::Value::ConstArray players = it->value.GetArray();
        data.rounds.clear();
        data.rounds.reserve(players.Size());
        std::for_each(players.Begin(), players.End(), [&data](const rapidjson::Value &json) {
            data.rounds.push_back(CompetitionRound());
            CompetitionRound::fromJson(json, data.rounds.back());
        });
    }

    it = json.FindMember("current_round");
    if (it != json.MemberEnd() && it->value.IsUint()) {
        data.current_round = it->value.GetUint();
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

void CompetitionData::toJson(const CompetitionData &data, rapidjson::Value &json, rapidjson::Value::AllocatorType &alloc) {
    json.AddMember("name", rapidjson::StringRef(data.name.c_str()), alloc);

    rapidjson::Value players(rapidjson::Type::kArrayType);
    players.Reserve(static_cast<rapidjson::SizeType>(data.players.size()), alloc);
    std::for_each(data.players.begin(), data.players.end(), [&players, &alloc](const CompetitionPlayer &player) {
        rapidjson::Value json(rapidjson::Type::kObjectType);
        CompetitionPlayer::toJson(player, json, alloc);
        players.PushBack(std::move(json), alloc);
    });
    json.AddMember("players", std::move(players), alloc);

    rapidjson::Value teams(rapidjson::Type::kArrayType);
    teams.Reserve(static_cast<rapidjson::SizeType>(data.teams.size()), alloc);
    std::for_each(data.teams.begin(), data.teams.end(), [&teams, &alloc](const CompetitionTeam &team) {
        rapidjson::Value json(rapidjson::Type::kObjectType);
        CompetitionTeam::toJson(team, json, alloc);
        teams.PushBack(std::move(json), alloc);
    });
    json.AddMember("teams", std::move(teams), alloc);

    rapidjson::Value rounds(rapidjson::Type::kArrayType);
    rounds.Reserve(static_cast<rapidjson::SizeType>(data.rounds.size()), alloc);
    std::for_each(data.rounds.begin(), data.rounds.end(), [&rounds, &alloc](const CompetitionRound &round) {
        rapidjson::Value json(rapidjson::Type::kObjectType);
        CompetitionRound::toJson(round, json, alloc);
        rounds.PushBack(std::move(json), alloc);
    });
    json.AddMember("rounds", std::move(rounds), alloc);

    json.AddMember("current_round", rapidjson::Value(data.current_round), alloc);
    json.AddMember("start_time", rapidjson::Value(static_cast<uint64_t>(data.start_time)), alloc);
    json.AddMember("finish_time", rapidjson::Value(static_cast<uint64_t>(data.finish_time)), alloc);
}

// 从文件中读
bool CompetitionData::readFromFile(const std::string &file) {
    std::string str;
    FILE *fp = fopen(file.c_str(), "rb");
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
        CCLOG("%s", str.c_str());
        rapidjson::Document doc;
        doc.Parse<0>(str.c_str());
        if (doc.HasParseError()) {
            return false;
        }

        fromJson(doc, *this);
        return true;
    }
    catch (std::exception &e) {
        CCLOG("%s %s", __FUNCTION__, e.what());
        return false;
    }
}

// 写入到文件
bool CompetitionData::writeToFile(const std::string &file) const {
    try {
        rapidjson::Document doc(rapidjson::Type::kObjectType);
        toJson(*this, doc, doc.GetAllocator());

        rapidjson::StringBuffer buf;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buf);
        doc.Accept(writer);

        CCLOG("%.*s", (int)buf.GetSize(), buf.GetString());

        FILE *fp = fopen(file.c_str(), "wb");
        if (LIKELY(fp != nullptr)) {
            fwrite(buf.GetString(), sizeof(char), buf.GetSize(), fp);
            fclose(fp);
            return true;
        }

        return false;
    }
    catch (std::exception &e) {
        CCLOG("%s %s", __FUNCTION__, e.what());
        return false;
    }
}
