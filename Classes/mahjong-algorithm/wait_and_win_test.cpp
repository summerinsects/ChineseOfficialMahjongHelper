#include "wait_and_win_test.h"

#include <assert.h>

bool is_basic_type_1_wait(const TILE *concealed_tiles, bool (&waiting_table)[6][10]) {
    waiting_table[tile_suit(concealed_tiles[0])][tile_rank(concealed_tiles[0])] = true;
    return true;
}

bool is_basic_type_1(const TILE *concealed_tiles, TILE test_tile) {
    return (concealed_tiles[0] == test_tile);
}

static bool is_two_numbered_tiles_wait(TILE tile0, TILE tile1, bool (&waiting_table)[6][10]) {
    SUIT_TYPE suit0 = tile_suit(tile0);

    RANK_TYPE rank0 = tile_rank(tile0);
    RANK_TYPE rank1 = tile_rank(tile1);
    switch (rank1 - rank0) {
    case 2:
        // Closed wait: Waiting solely for a tile whose number is "inside" to form a chow
        waiting_table[suit0][rank0 + 1] = true;
        return true;
    case 1:
        // Edge wait or Both sides wait
        if (rank0 >= 2) waiting_table[suit0][rank0 - 1] = true;
        if (rank1 <= 8) waiting_table[suit0][rank1 + 1] = true;
        return true;
    default:
        return false;
    }
}

bool is_basic_type_4_wait(const TILE *concealed_tiles, bool (&waiting_table)[6][10]) {
    bool ret = false;

    // if 0 1 2 is a completed set, it waits the remain tile for the pair
    if (is_concealed_set_completed(concealed_tiles[0], concealed_tiles[1], concealed_tiles[2])) {
        // Single wait: Waiting solely for a tile to form a pair
        waiting_table[tile_suit(concealed_tiles[3])][tile_rank(concealed_tiles[3])] = true;
        ret = true;
    }

    // if 1 2 3 is a completed set, it waits the remain tile for the pair
    if (is_concealed_set_completed(concealed_tiles[1], concealed_tiles[2], concealed_tiles[3])) {
        // Single wait: Waiting solely for a tile to form a pair
        waiting_table[tile_suit(concealed_tiles[0])][tile_rank(concealed_tiles[0])] = true;
        ret = true;
    }

    // if 0 1 is the pair, check 2 3
    if (concealed_tiles[0] == concealed_tiles[1]) {
        // if 2 3 is the pair, this is called two pungs wait
        if (concealed_tiles[2] == concealed_tiles[3]) {
            waiting_table[tile_suit(concealed_tiles[2])][tile_rank(concealed_tiles[2])] = true;
            return true;
        }

        // for two numbered tiles, it is possible to wait some other tiles
        if (is_suit_equal_quick(concealed_tiles[2], concealed_tiles[3])) {
            if (is_numbered_suit_quick(concealed_tiles[2])) {
                if (is_two_numbered_tiles_wait(concealed_tiles[2], concealed_tiles[3], waiting_table)) {
                    ret = true;
                }
            }
        }
    }

    // if 1 2 is the pair, check 0 3
    if (concealed_tiles[1] == concealed_tiles[2]) {
        if (is_suit_equal_quick(concealed_tiles[0], concealed_tiles[3])) {
            if (is_numbered_suit_quick(concealed_tiles[0])) {
                if (is_two_numbered_tiles_wait(concealed_tiles[0], concealed_tiles[3], waiting_table)) {
                    ret = true;
                }
            }
        }
    }

    // if 2 3 is the pair, check 0 1
    if (concealed_tiles[2] == concealed_tiles[3]) {
        if (is_suit_equal_quick(concealed_tiles[0], concealed_tiles[1])) {
            if (is_numbered_suit_quick(concealed_tiles[0])) {
                if (is_two_numbered_tiles_wait(concealed_tiles[0], concealed_tiles[1], waiting_table)) {
                    ret = true;
                }
            }
        }
    }

    return ret;
}

bool is_basic_type_4(const TILE *concealed_tiles, TILE test_tile) {
    bool waiting_table[6][10] = { { false } };
    return (is_basic_type_4_wait(concealed_tiles, waiting_table)
        && waiting_table[tile_suit(test_tile)][tile_rank(test_tile)]);
}

template <long _Count, bool (*_NextStepFunction)(const TILE *concealed_tiles, bool (&waiting_table)[6][10])>
static bool is_basic_type_N_wait(const TILE *concealed_tiles, bool (&waiting_table)[6][10]) {
    bool ret = false;

    long i = 0;
    while (i < _Count) {
        TILE tile_i = concealed_tiles[i];

        long j = i + 1;
        while (j < _Count) {
            TILE tile_j = concealed_tiles[j];
            if (tile_j - tile_i >= 3) break;  // It is impossible to make a chow or pung

            long k = j + 1;
            while (k < _Count) {
                TILE tile_k = concealed_tiles[k];
                if (tile_k - tile_i >= 3) break;  // It is impossible to make a chow or pung

                if (is_concealed_set_completed(tile_i, tile_j, tile_k)) {
                    // Reduced the completed pung or chow
                    TILE remains[_Count - 3];
                    for (long n = 0, c = 0; n < _Count; ++n) {
                        if (n == i || n == j || n == k) {
                            continue;
                        }
                        remains[c++] = concealed_tiles[n];
                    }
                    // recursive call
                    if (_NextStepFunction(remains, waiting_table)) {
                        ret = true;
                    }
                }

                do ++k; while (k < _Count && concealed_tiles[k] == tile_k);  // quick skip the same case
            }

            do ++j; while (j < _Count && concealed_tiles[j] == tile_j);  // quick skip the same case
        }

        do ++i; while (i < _Count && concealed_tiles[i] == tile_i);  // quick skip the same case
    }

    return ret;
}

bool is_basic_type_7_wait(const TILE *concealed_tiles, bool (&waiting_table)[6][10]) {
    return is_basic_type_N_wait<7, is_basic_type_4_wait>(concealed_tiles, waiting_table);
}

bool is_basic_type_7(const TILE *concealed_tiles, TILE test_tile) {
    bool waiting_table[6][10] = { { false } };
    return (is_basic_type_7_wait(concealed_tiles, waiting_table)
        && waiting_table[tile_suit(test_tile)][tile_rank(test_tile)]);
}

bool get_knitted_straight_missing_tiles(const TILE *concealed_tiles, long cnt, TILE *missing_tiles) {
    assert(missing_tiles != nullptr);
    const TILE *matched_seq = nullptr;
    for (int i = 0; i < 6; ++i) {
        if (std::includes(std::begin(standard_knitted_straight[i]), std::end(standard_knitted_straight[i]),
            concealed_tiles, concealed_tiles + cnt)) {
            matched_seq = standard_knitted_straight[i];  // match a knitted straight
            break;
        }
    }
    if (matched_seq == nullptr) {
        return false;
    }

    // statistics the missing tiles
    TILE remains[9] = { 0 };
    TILE *it = copy_exclude(matched_seq, matched_seq + 9, concealed_tiles, concealed_tiles + cnt, remains);
    long n = it - remains;
    if (n + cnt == 9) {
        memcpy(missing_tiles, remains, n * sizeof(TILE));
        return true;
    }
    return false;
}

template <long _Count, bool (*_ReducedCallback)(const TILE *concealed_tiles, bool (&waiting_table)[6][10])>
static bool is_completed_knitted_straight_wait(const TILE *concealed_tiles, bool (&waiting_table)[6][10]) {
    static_assert(_Count > 9, "_Count must greater than 9");

    const TILE *matched_seq = nullptr;
    for (long i = 0; i < 6; ++i) {
        if (std::includes(concealed_tiles, concealed_tiles + _Count,
            std::begin(standard_knitted_straight[i]), std::end(standard_knitted_straight[i]))) {
            matched_seq = standard_knitted_straight[i];  // match a knitted straight
            break;
        }
    }
    if (matched_seq == nullptr) {
        return false;
    }

    // remove tiles in knitted straight
    TILE remains[_Count];
    TILE *it = copy_exclude(concealed_tiles, concealed_tiles + _Count,
        matched_seq, matched_seq + 9, remains);
    long n = it - remains;
    if (n == _Count - 9) {  // then, check the remaining tiles
        return _ReducedCallback(remains, waiting_table);
    }

    return false;
}

bool is_basic_type_10_wait(const TILE *concealed_tiles, bool (&waiting_table)[6][10]) {
    if (is_basic_type_N_wait<10, is_basic_type_7_wait>(concealed_tiles, waiting_table)) {
        return true;
    }

    // wait the tile in knitted straight
    const TILE *last = concealed_tiles + 10;
    const TILE *it = std::adjacent_find(concealed_tiles, last);  // find the pair
    while (it != last) {
        const TILE pair_tile = *it;
        TILE pair[2] = { pair_tile, pair_tile };
        TILE remains[8];  // 8 tiles except the pair
        copy_exclude(concealed_tiles, last, std::begin(pair), std::end(pair), remains);

        TILE waiting;  // only one missing
        if (get_knitted_straight_missing_tiles(remains, 8, &waiting)) {
            waiting_table[tile_suit(waiting)][tile_rank(waiting)] = true;
            return true;
        }

        do ++it; while (it != last && *it == pair_tile);
        it = std::adjacent_find(it, last);
    }

    return is_completed_knitted_straight_wait<10, is_basic_type_1_wait>(concealed_tiles, waiting_table);
}

bool is_basic_type_10(const TILE *concealed_tiles, TILE test_tile) {
    bool waiting_table[6][10] = { { false } };
    return (is_basic_type_10_wait(concealed_tiles, waiting_table)
        && waiting_table[tile_suit(test_tile)][tile_rank(test_tile)]);
}

bool is_basic_type_13_wait(const TILE *concealed_tiles, bool (&waiting_table)[6][10]) {
    if (is_basic_type_N_wait<13, is_basic_type_10_wait>(concealed_tiles, waiting_table)) {
        return true;
    }
    return is_completed_knitted_straight_wait<13, is_basic_type_4_wait>(concealed_tiles, waiting_table);
}

bool is_basic_type_13(const TILE *concealed_tiles, TILE test_tile) {
    bool waiting_table[6][10] = { { false } };
    return (is_basic_type_13_wait(concealed_tiles, waiting_table)
        && waiting_table[tile_suit(test_tile)][tile_rank(test_tile)]);
}

bool is_special_type(const TILE (&concealed_tiles)[13], TILE test_tile) {
    return (is_seven_pairs(concealed_tiles, test_tile)
        || is_thirteen_orphans(concealed_tiles, test_tile)
        || is_honors_and_knitted_tiles(concealed_tiles, test_tile));
}

bool is_seven_pairs_wait(const TILE (&concealed_tiles)[13], TILE *waiting) {
    TILE single = 0;
    for (long i = 0; i < 13; ++i) {
        if (i < 12 && concealed_tiles[i] == concealed_tiles[i + 1]) {
            ++i;
        }
        else {
            if (single != 0) {
                return false;  // Only one single tile allowed
            }
            single = concealed_tiles[i];
        }
    }
    *waiting = single;
    return true;
}

bool is_seven_pairs(const TILE (&concealed_tiles)[13], TILE test_tile) {
    TILE waiting;
    return (is_seven_pairs_wait(concealed_tiles, &waiting) && waiting == test_tile);
}

bool is_thirteen_orphans_wait(const TILE (&concealed_tiles)[13], TILE *waiting, unsigned *waiting_cnt) {
    if (!std::all_of(concealed_tiles, concealed_tiles + 13, &is_terminal_or_honor)) {
        return false;
    }

    TILE temp[13];
    TILE *end = std::unique_copy(concealed_tiles, concealed_tiles + 13, temp);
    long cnt = end - temp;
    if (cnt == 12) {  // there is already a pair, get the missing tile
        copy_exclude(std::begin(standard_thirteen_orphans), std::end(standard_thirteen_orphans), temp, end, waiting);
        *waiting_cnt = 1;
        return true;
    }
    else if (cnt == 13) {  // waiting any of the terminal or honor tiles
        memcpy(waiting, standard_thirteen_orphans, sizeof(standard_thirteen_orphans));
        *waiting_cnt = 13;
        return true;
    }
    return false;
}

bool is_thirteen_orphans(const TILE (&concealed_tiles)[13], TILE test_tile) {
    TILE waiting[13] = { 0 };
    unsigned waiting_cnt;
    if (is_thirteen_orphans_wait(concealed_tiles, waiting, &waiting_cnt)) {
        TILE *end = waiting + waiting_cnt;
        return (std::find((TILE *)waiting, end, test_tile) != end);
    }
    return false;
}

bool is_honors_and_knitted_tiles_wait(const TILE (&concealed_tiles)[13], TILE *waiting) {
    // should not contain pairs
    if (std::adjacent_find(concealed_tiles, concealed_tiles + 13) != concealed_tiles + 13) {
        return false;
    }

    const TILE *p = std::find_if(concealed_tiles, concealed_tiles + 13, &is_honor);  // find the first honor tile
    const long numbered_cnt = p - concealed_tiles;

    // quick return if numbered tiles more than 9 or honor tiles more than 7 (i.e. numbered_cnt less than 6)
    if (numbered_cnt > 9 || numbered_cnt < 6) {
        return false;
    }

    // a completed knitted straight contains 9 tiles, and 7 honor tiles, that is 16
    TILE temp[16] = { 0 };
    if (!get_knitted_straight_missing_tiles(concealed_tiles, numbered_cnt, temp)) {
        return false;
    }

    // map the existing honor tiles
    unsigned winds_table[5] = { 0 };
    unsigned dragons_table[4] = { 0 };
    for (long i = numbered_cnt; i < 13; ++i) {
        SUIT_TYPE suit = tile_suit(concealed_tiles[i]);
        RANK_TYPE rank = tile_rank(concealed_tiles[i]);
        if (suit == TILE_SUIT_WINDS) {
            ++winds_table[rank];
        }
        else if (suit == TILE_SUIT_DRAGONS) {
            ++dragons_table[rank];
        }
    }

    // statistics the missing winds and dragons
    long wait_cnt = 9 - numbered_cnt;
    for (int i = 1; i <= 4; ++i) {
        if (winds_table[i] == 0) {
            temp[wait_cnt++] = make_tile(TILE_SUIT_WINDS, i);
        }
    }

    for (int i = 1; i <= 3; ++i) {
        if (dragons_table[i] == 0) {
            temp[wait_cnt++] = make_tile(TILE_SUIT_DRAGONS, i);
        }
    }

    if (wait_cnt == 3) {  // must be 3 tiles wait
        sort_tiles(temp, wait_cnt);
        memcpy(waiting, temp, 3 * sizeof(TILE));
        return true;
    }
    return false;
}

bool is_honors_and_knitted_tiles(const TILE (&concealed_tiles)[13], TILE test_tile) {
    TILE waiting[3];
    if (is_honors_and_knitted_tiles_wait(concealed_tiles, waiting)) {
        return (std::find(std::begin(waiting), std::end(waiting), test_tile) != std::end(waiting));
    }
    return false;
}
