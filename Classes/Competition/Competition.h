#ifndef __COMPETITION_H__
#define __COMPETITION_H__


// 4 2 1 0
// 并列12：(4+2)/2=3
// 并列23：(2+1)/2=1.5
// 并列34：(1+0)/2=0.5
// 并列123：(4+2+1)/3=2.333
// 并列234：(2+1+0)/3=1
// 并列1234：(4+2+1+0)/4=1.75

enum RANK_TYPE {
    RANK_1, RANK_TIED_12, RANK_TIED_123, RANK_TIED_1234,
    RANK_2, RANK_TIED_23, RANK_TIED_234,
    RANK_3, RANK_TIED_34,
    RANK_4
};

// *12 使得所有分化为整数
static const unsigned standard_score_12[] = {
    48, 36, 28, 21,
    24, 18, 12,
    12, 6,
    0
};

struct CompetitionPlayer {
    unsigned serial;
    char name[255];
    unsigned standard_score_table[10];
    unsigned competition_scores;
};

struct CompetitionData {
    char name[128];
    unsigned round_count;
    unsigned current_round;
    size_t player_count;
    CompetitionPlayer *players;
};

#endif
