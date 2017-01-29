#include "wait_and_win_test.h"

#include <assert.h>
#include <string.h>

namespace mahjong {

bool is_basic_type_1_wait(const TILE *standing_tiles, bool (&waiting_table)[6][10]) {
    waiting_table[tile_suit(standing_tiles[0])][tile_rank(standing_tiles[0])] = true;
    return true;
}

bool is_basic_type_1_win(const TILE *standing_tiles, TILE test_tile) {
    return (standing_tiles[0] == test_tile);
}

static bool is_two_numbered_tiles_wait(TILE tile0, TILE tile1, bool (&waiting_table)[6][10]) {
    SUIT_TYPE suit0 = tile_suit(tile0);

    RANK_TYPE rank0 = tile_rank(tile0);
    RANK_TYPE rank1 = tile_rank(tile1);
    switch (rank1 - rank0) {
    case 2:
        // 坎张
        waiting_table[suit0][rank0 + 1] = true;
        return true;
    case 1:
        // 两面或者边张
        if (rank0 >= 2) waiting_table[suit0][rank0 - 1] = true;
        if (rank1 <= 8) waiting_table[suit0][rank1 + 1] = true;
        return true;
    default:
        return false;
    }
}

bool is_basic_type_4_wait(const TILE *standing_tiles, bool (&waiting_table)[6][10]) {
    bool ret = false;

    // 如果t0 t1 t2构成一组面子，那么听单钓t3
    if (is_concealed_set_completed(standing_tiles[0], standing_tiles[1], standing_tiles[2])) {
        waiting_table[tile_suit(standing_tiles[3])][tile_rank(standing_tiles[3])] = true;
        ret = true;
    }

    // 如果t1 t2 t3构成一组面子，那么听单钓t0
    if (is_concealed_set_completed(standing_tiles[1], standing_tiles[2], standing_tiles[3])) {
        waiting_table[tile_suit(standing_tiles[0])][tile_rank(standing_tiles[0])] = true;
        ret = true;
    }

    // t0 t1是一对（作为将）
    if (standing_tiles[0] == standing_tiles[1]) {
        // 如果t2 t3也是一对，这是对倒听
        if (standing_tiles[2] == standing_tiles[3]) {
            waiting_table[tile_suit(standing_tiles[2])][tile_rank(standing_tiles[2])] = true;
            return true;
        }

        // 如果t2 t3是同花色序数牌，才有可能听附近别的牌
        if (is_suit_equal_quick(standing_tiles[2], standing_tiles[3])) {
            if (is_numbered_suit_quick(standing_tiles[2])) {
                if (is_two_numbered_tiles_wait(standing_tiles[2], standing_tiles[3], waiting_table)) {
                    ret = true;
                }
            }
        }
    }

    // t1 t2是一对（作为将）
    if (standing_tiles[1] == standing_tiles[2]) {
        // 如果t0 t3是同花色序数牌，才有可能听附近别的牌
        if (is_suit_equal_quick(standing_tiles[0], standing_tiles[3])) {
            if (is_numbered_suit_quick(standing_tiles[0])) {
                if (is_two_numbered_tiles_wait(standing_tiles[0], standing_tiles[3], waiting_table)) {
                    ret = true;
                }
            }
        }
    }

    // t2 t3是一对（作为将）
    if (standing_tiles[2] == standing_tiles[3]) {
        // 如果t0 t1是同花色序数牌，才有可能听附近别的牌
        if (is_suit_equal_quick(standing_tiles[0], standing_tiles[1])) {
            if (is_numbered_suit_quick(standing_tiles[0])) {
                if (is_two_numbered_tiles_wait(standing_tiles[0], standing_tiles[1], waiting_table)) {
                    ret = true;
                }
            }
        }
    }

    return ret;
}

bool is_basic_type_4_win(const TILE *standing_tiles, TILE test_tile) {
    bool waiting_table[6][10] = { { false } };
    return (is_basic_type_4_wait(standing_tiles, waiting_table)
        && waiting_table[tile_suit(test_tile)][tile_rank(test_tile)]);
}

template <long _Count, bool (*_NextStepFunction)(const TILE *standing_tiles, bool (&waiting_table)[6][10])>
static bool is_basic_type_N_wait(const TILE *standing_tiles, bool (&waiting_table)[6][10]) {
    bool ret = false;

    long i = 0;
    while (i < _Count) {
        TILE tile_i = standing_tiles[i];

        long j = i + 1;
        while (j < _Count) {
            TILE tile_j = standing_tiles[j];
            if (tile_j - tile_i >= 3) break;  // 这已经不可能再构成面子了

            long k = j + 1;
            while (k < _Count) {
                TILE tile_k = standing_tiles[k];
                if (tile_k - tile_i >= 3) break;  // 这已经不可能再构成面子了

                if (is_concealed_set_completed(tile_i, tile_j, tile_k)) {
                    // 削减面子
                    TILE remains[_Count - 3];
                    for (long n = 0, c = 0; n < _Count; ++n) {
                        if (n == i || n == j || n == k) {
                            continue;
                        }
                        remains[c++] = standing_tiles[n];
                    }
                    // 递归
                    if (_NextStepFunction(remains, waiting_table)) {
                        ret = true;
                    }
                }

                do ++k; while (k < _Count && standing_tiles[k] == tile_k);  // 快速跳过相同的case
            }

            do ++j; while (j < _Count && standing_tiles[j] == tile_j);  // 快速跳过相同的case
        }

        do ++i; while (i < _Count && standing_tiles[i] == tile_i);  // 快速跳过相同的case
    }

    return ret;
}

bool is_basic_type_7_wait(const TILE *standing_tiles, bool (&waiting_table)[6][10]) {
    return is_basic_type_N_wait<7, is_basic_type_4_wait>(standing_tiles, waiting_table);
}

bool is_basic_type_7_win(const TILE *standing_tiles, TILE test_tile) {
    bool waiting_table[6][10] = { { false } };
    return (is_basic_type_7_wait(standing_tiles, waiting_table)
        && waiting_table[tile_suit(test_tile)][tile_rank(test_tile)]);
}

bool get_knitted_straight_missing_tiles(const TILE *standing_tiles, long cnt, TILE *missing_tiles) {
    assert(missing_tiles != nullptr);
    const TILE *matched_seq = nullptr;
    for (int i = 0; i < 6; ++i) {
        if (std::includes(std::begin(standard_knitted_straight[i]), std::end(standard_knitted_straight[i]),
            standing_tiles, standing_tiles + cnt)) {
            matched_seq = standard_knitted_straight[i];  // 匹配一种组合龙
            break;
        }
    }
    if (matched_seq == nullptr) {
        return false;
    }

    // 统计缺失的牌张
    TILE remains[9] = { 0 };
    TILE *it = copy_exclude(matched_seq, matched_seq + 9, standing_tiles, standing_tiles + cnt, remains);
    long n = it - remains;
    if (n + cnt == 9) {
        memcpy(missing_tiles, remains, n * sizeof(TILE));
        return true;
    }
    return false;
}

template <long _Count, bool (*_ReducedCallback)(const TILE *standing_tiles, bool (&waiting_table)[6][10])>
static bool is_completed_knitted_straight_wait(const TILE *standing_tiles, bool (&waiting_table)[6][10]) {
    static_assert(_Count > 9, "_Count must greater than 9");

    const TILE *matched_seq = nullptr;
    for (long i = 0; i < 6; ++i) {
        if (std::includes(standing_tiles, standing_tiles + _Count,
            std::begin(standard_knitted_straight[i]), std::end(standard_knitted_straight[i]))) {
            matched_seq = standard_knitted_straight[i];  // 匹配一种组合龙
            break;
        }
    }
    if (matched_seq == nullptr) {
        return false;
    }

    // 剔除组合龙部分
    TILE remains[_Count];
    TILE *it = copy_exclude(standing_tiles, standing_tiles + _Count,
        matched_seq, matched_seq + 9, remains);
    long n = it - remains;
    if (n == _Count - 9) {  // 检查余下的牌张
        return _ReducedCallback(remains, waiting_table);
    }

    return false;
}

bool is_basic_type_10_wait(const TILE *standing_tiles, bool (&waiting_table)[6][10]) {
    if (is_basic_type_N_wait<10, is_basic_type_7_wait>(standing_tiles, waiting_table)) {
        return true;
    }

    // 听构成组合龙部分
    const TILE *last = standing_tiles + 10;
    const TILE *it = std::adjacent_find(standing_tiles, last);  // 找到将牌
    while (it != last) {
        const TILE pair_tile = *it;
        TILE pair[2] = { pair_tile, pair_tile };
        TILE remains[8];  // 10张手牌剔除一对将，余下8张
        copy_exclude(standing_tiles, last, std::begin(pair), std::end(pair), remains);

        TILE waiting;  // 必然听一张
        if (get_knitted_straight_missing_tiles(remains, 8, &waiting)) {
            waiting_table[tile_suit(waiting)][tile_rank(waiting)] = true;
            return true;
        }

        do ++it; while (it != last && *it == pair_tile);
        it = std::adjacent_find(it, last);
    }

    return is_completed_knitted_straight_wait<10, is_basic_type_1_wait>(standing_tiles, waiting_table);
}

bool is_basic_type_10_win(const TILE *standing_tiles, TILE test_tile) {
    bool waiting_table[6][10] = { { false } };
    return (is_basic_type_10_wait(standing_tiles, waiting_table)
        && waiting_table[tile_suit(test_tile)][tile_rank(test_tile)]);
}

bool is_basic_type_13_wait(const TILE *standing_tiles, bool (&waiting_table)[6][10]) {
    if (is_basic_type_N_wait<13, is_basic_type_10_wait>(standing_tiles, waiting_table)) {
        return true;
    }
    return is_completed_knitted_straight_wait<13, is_basic_type_4_wait>(standing_tiles, waiting_table);
}

bool is_basic_type_13_win(const TILE *standing_tiles, TILE test_tile) {
    bool waiting_table[6][10] = { { false } };
    return (is_basic_type_13_wait(standing_tiles, waiting_table)
        && waiting_table[tile_suit(test_tile)][tile_rank(test_tile)]);
}

bool is_special_type_win(const TILE (&standing_tiles)[13], TILE test_tile) {
    return (is_seven_pairs_win(standing_tiles, test_tile)
        || is_thirteen_orphans_win(standing_tiles, test_tile)
        || is_honors_and_knitted_tiles_win(standing_tiles, test_tile));
}

bool is_seven_pairs_wait(const TILE (&standing_tiles)[13], TILE *waiting) {
    TILE single = 0;
    for (long i = 0; i < 13; ++i) {
        if (i < 12 && standing_tiles[i] == standing_tiles[i + 1]) {
            ++i;
        }
        else {
            if (single != 0) {
                return false;  // 有不止一张单牌，说明不是七对听牌型
            }
            single = standing_tiles[i];
        }
    }
    *waiting = single;
    return true;
}

bool is_seven_pairs_win(const TILE (&standing_tiles)[13], TILE test_tile) {
    TILE waiting;
    return (is_seven_pairs_wait(standing_tiles, &waiting) && waiting == test_tile);
}

bool is_thirteen_orphans_wait(const TILE (&standing_tiles)[13], TILE *waiting, unsigned *waiting_cnt) {
    if (!std::all_of(standing_tiles, standing_tiles + 13, &is_terminal_or_honor)) {
        return false;
    }

    TILE temp[13];
    TILE *end = std::unique_copy(standing_tiles, standing_tiles + 13, temp);
    long cnt = end - temp;
    if (cnt == 12) {  // 已经有一对，听缺少的那一张
        copy_exclude(std::begin(standard_thirteen_orphans), std::end(standard_thirteen_orphans), temp, end, waiting);
        *waiting_cnt = 1;
        return true;
    }
    else if (cnt == 13) {  // 13面听
        memcpy(waiting, standard_thirteen_orphans, sizeof(standard_thirteen_orphans));
        *waiting_cnt = 13;
        return true;
    }
    return false;
}

bool is_thirteen_orphans_win(const TILE (&standing_tiles)[13], TILE test_tile) {
    TILE waiting[13] = { 0 };
    unsigned waiting_cnt;
    if (is_thirteen_orphans_wait(standing_tiles, waiting, &waiting_cnt)) {
        TILE *end = waiting + waiting_cnt;
        return (std::find((TILE *)waiting, end, test_tile) != end);
    }
    return false;
}

bool is_honors_and_knitted_tiles_wait(const TILE (&standing_tiles)[13], TILE *waiting) {
    // 全不靠不能有对子
    if (std::adjacent_find(standing_tiles, standing_tiles + 13) != standing_tiles + 13) {
        return false;
    }

    const TILE *p = std::find_if(standing_tiles, standing_tiles + 13, &is_honor);  // 找到第一个字牌
    const long numbered_cnt = p - standing_tiles;  // 序数牌张数

    // 序数牌张数大于9或者小于6必然不可能听全不靠
    if (numbered_cnt > 9 || numbered_cnt < 6) {
        return false;
    }

    TILE temp[16] = { 0 };  // 一个完整的组合龙是9张牌，加字牌7张，一共是16张

    // 统计组合龙里面缺失的牌张，这一部分是所听牌张
    if (!get_knitted_straight_missing_tiles(standing_tiles, numbered_cnt, temp)) {
        return false;
    }

    // 统计已有的字牌
    unsigned winds_table[5] = { 0 };
    unsigned dragons_table[4] = { 0 };
    for (long i = numbered_cnt; i < 13; ++i) {
        SUIT_TYPE suit = tile_suit(standing_tiles[i]);
        RANK_TYPE rank = tile_rank(standing_tiles[i]);
        if (suit == TILE_SUIT_WINDS) {
            ++winds_table[rank];
        }
        else if (suit == TILE_SUIT_DRAGONS) {
            ++dragons_table[rank];
        }
    }

    // 统计缺失的字牌，这一部分是所听牌张
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

    if (wait_cnt == 3) {  // 必然听3张
        sort_tiles(temp, wait_cnt);
        memcpy(waiting, temp, 3 * sizeof(TILE));
        return true;
    }
    return false;
}

bool is_honors_and_knitted_tiles_win(const TILE (&standing_tiles)[13], TILE test_tile) {
    TILE waiting[3];
    if (is_honors_and_knitted_tiles_wait(standing_tiles, waiting)) {
        return (std::find(std::begin(waiting), std::end(waiting), test_tile) != std::end(waiting));
    }
    return false;
}

}
