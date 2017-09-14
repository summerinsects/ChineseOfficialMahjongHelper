#include "Competition.h"

#include <sstream>
#include <algorithm>
#include "../common.h"

std::string CompetitionResult::getStandardScoreString(float ss) {
    std::ostringstream os;
    os << ss;
    std::string ret1 = os.str();
    std::string ret2 = Common::format<32>("%.3f", ss);
    return ret1.length() < ret2.length() ? ret1 : ret2;
}

std::pair<float, int> CompetitionPlayer::getTotalScoresByRound(size_t round) const {
    float ss = 0;
    int cs = 0;
    round = std::min(competition_results.size(), round);
    for (size_t i = 0; i < round; ++i) {
        ss += competition_results[i].standard_score;
        cs += competition_results[i].competition_score;
    }
    return std::make_pair(ss, cs);
}

std::pair<float, int> CompetitionPlayer::getCurrentScoresByRound(size_t round) const {
    if (competition_results.size() > round) {
        return std::make_pair(competition_results[round].standard_score, competition_results[round].competition_score);
    }
    return std::make_pair(0.0f, 0);
}


