/****************************************************************************
 Copyright (c) 2016-2017 Jeff Wang <summer_insects@163.com>

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 ****************************************************************************/

#include "fan_calculator.h"
#include "shanten.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <algorithm>

#define MAX_DIVISION_CNT 20  // 一副牌最多也没有20种划分吧，够用了

#if 0
#define LOG(fmt_, ...) printf(fmt_, ##__VA_ARGS__)
#else
#define LOG(...) do { } while (0)
#endif

#ifndef UINT8_C
#define UINT8_C(x)   (x)
#endif

#ifndef UINT16_C
#define UINT16_C(x)  (x)
#endif

//#define STRICT_98_RULE

namespace mahjong {

extern long packs_to_string(const pack_t *packs, long pack_cnt, char *str, long max_size);

struct one_division_t {  // 每一种划分
    pack_t packs[5];
};

// 所有的划分
struct divisions_t {
    one_division_t divisions[MAX_DIVISION_CNT];
    long count;  // 划分方式总数
};

static void divide_tail_add_pair(tile_t tile, long fixed_cnt, one_division_t *work_division, divisions_t *result) {
    // 这2张作为雀头
    work_division->packs[4] = make_pack(0, PACK_TYPE_PAIR, tile);

    // 拷贝一份当前的划分出来的面子，并排序暗手的面子
    // 这里不能直接在work_division.packs上排序，否则会破坏递归外层的数据
    one_division_t temp;
    memcpy(&temp, work_division, sizeof(temp));
    std::sort(temp.packs + fixed_cnt, temp.packs + 4);

    // 检查这种划分是否已经存在了
    if (std::none_of(&result->divisions[0], &result->divisions[result->count],
        [&temp, fixed_cnt](const one_division_t &od) {
        return std::equal(&od.packs[fixed_cnt], &od.packs[4], &temp.packs[fixed_cnt]);
    })) {
        memcpy(&result->divisions[result->count], &temp, sizeof(temp));
        ++result->count;
    }
    else {
        LOG("same case");
    }
}

static bool divide_tail(tile_table_t &cnt_table, long fixed_cnt, one_division_t *work_division, divisions_t *result) {
    for (int i = 0; i < 34; ++i) {
        tile_t t = all_tiles[i];
        if (cnt_table[t] < 2) {
            continue;
        }

        cnt_table[t] -= 2;
        // 全部使用完毕
        if (std::all_of(std::begin(cnt_table), std::end(cnt_table), [](int n) { return n == 0; })) {
            cnt_table[t] += 2;
            divide_tail_add_pair(t, fixed_cnt, work_division, result);
            return true;
        }
        cnt_table[t] += 2;
    }

    return false;
}

static bool is_division_branch_exist(long fixed_cnt, long step, const one_division_t *work_division, const divisions_t *result) {
    if (result->count <= 0 || fixed_cnt + step < 3) {
        return false;
    }

    // std::includes要求有序
    one_division_t temp;
    memcpy(&temp.packs[fixed_cnt], &work_division->packs[fixed_cnt], step * sizeof(pack_t));
    std::sort(&temp.packs[fixed_cnt], &temp.packs[fixed_cnt + step]);

    // 只需要比较面子是否重复分支，雀头不参与比较，所以下标是4
    return std::any_of(&result->divisions[0], &result->divisions[result->count],
        [&temp, fixed_cnt, step](const one_division_t &od) {
        return std::includes(&od.packs[fixed_cnt], &od.packs[4], &temp.packs[fixed_cnt], &temp.packs[fixed_cnt + step]);
    });
}

static bool divide_recursively(tile_table_t &cnt_table, long fixed_cnt, long step, one_division_t *work_division, divisions_t *result) {
    const long idx = step + fixed_cnt;
    if (idx == 4) {  // 4组面子都有了
        return divide_tail(cnt_table, fixed_cnt, work_division, result);
    }

    bool ret = false;
    for (int i = 0; i < 34; ++i) {
        tile_t t = all_tiles[i];
        if (cnt_table[t] < 1) {
            continue;
        }

        // 刻子
        if (cnt_table[t] > 2) {
            work_division->packs[idx] = make_pack(0, PACK_TYPE_PUNG, t);
            if (!is_division_branch_exist(fixed_cnt, step + 1, work_division, result)) {
                // 削减这组刻子，递归
                cnt_table[t] -= 3;
                if (divide_recursively(cnt_table, fixed_cnt, step + 1, work_division, result)) {
                    ret = true;
                }
                cnt_table[t] += 3;
            }
        }

        // 顺子（只能是数牌）
        bool is_numbered = is_numbered_suit(t);
        if (is_numbered) {
            if (tile_rank(t) < 8 && cnt_table[t + 1] && cnt_table[t + 2]) {
                work_division->packs[idx] = make_pack(0, PACK_TYPE_CHOW, t + 1);
                if (!is_division_branch_exist(fixed_cnt, step + 1, work_division, result)) {
                    // 削减这组顺子，递归
                    --cnt_table[t];
                    --cnt_table[t + 1];
                    --cnt_table[t + 2];
                    if (divide_recursively(cnt_table, fixed_cnt, step + 1, work_division, result)) {
                        ret = true;
                    }
                    ++cnt_table[t];
                    ++cnt_table[t + 1];
                    ++cnt_table[t + 2];
                }
            }
        }
    }

    return ret;
}

static bool divide_win_hand(const tile_t *standing_tiles, const pack_t *fixed_packs, long fixed_cnt, divisions_t *result) {
    long standing_cnt = 14 - fixed_cnt * 3;

    // 对立牌的种类进行打表
    tile_table_t cnt_table;
    map_tiles(standing_tiles, standing_cnt, cnt_table);

    result->count = 0;

    // 复制副露的面子
    one_division_t work_division;
    memcpy(work_division.packs, fixed_packs, fixed_cnt * sizeof(pack_t));
    return divide_recursively(cnt_table, fixed_cnt, 0, &work_division, result);
}

// 一色四同顺
static forceinline bool is_quadruple_chow(tile_t t0, tile_t t1, tile_t t2, tile_t t3) {
    return (t0 == t1 && t0 == t2 && t0 == t3);
}

// 一色四节高
static forceinline bool is_four_pure_shifted_pungs(tile_t t0, tile_t t1, tile_t t2, tile_t t3) {
    if (is_numbered_suit_quick(t0)) {
        return (t0 + 1 == t1 && t1 + 1 == t2 && t2 + 1 == t3);
    }
    return false;
}

// 一色四步高
static forceinline bool is_four_pure_shifted_chows(tile_t t0, tile_t t1, tile_t t2, tile_t t3) {
    // 递增2的必然是：123 345 567 789
    // 递增1的最小为：123 234 345 456 最大为：456 567 678 789
    return ((t0 + 2 == t1 && t1 + 2 == t2 && t2 + 2 == t3)
        || (t0 + 1 == t1 && t1 + 1 == t2 && t2 + 1 == t3));
}

// 一色三同顺
static forceinline bool is_pure_triple_chow(tile_t t0, tile_t t1, tile_t t2) {
    return (t0 == t1 && t0 == t2);
}

// 一色三节高
static forceinline bool is_pure_shifted_pungs(tile_t t0, tile_t t1, tile_t t2) {
    return (is_numbered_suit_quick(t0) && t0 + 1 == t1 && t1 + 1 == t2);
}

// 清龙
static forceinline bool is_pure_straight(tile_t t0, tile_t t1, tile_t t2) {
    return (tile_rank(t0) == 2 && t0 + 3 == t1 && t1 + 3 == t2);
}

// 一色三步高
static forceinline bool is_pure_shifted_chows(tile_t t0, tile_t t1, tile_t t2) {
    return ((t0 + 2 == t1 && t1 + 2 == t2) || (t0 + 1 == t1 && t1 + 1 == t2));
}

// 花龙
static bool is_mixed_straight(tile_t t0, tile_t t1, tile_t t2) {
    suit_t s0 = tile_suit(t0);
    suit_t s1 = tile_suit(t1);
    suit_t s2 = tile_suit(t2);
    if (s0 != s1 && s0 != s2 && s1 != s2) {
        rank_t r0 = tile_rank(t0);
        rank_t r1 = tile_rank(t1);
        rank_t r2 = tile_rank(t2);
        return ((r0 == 2 && r1 == 5 && r2 == 8)
            || (r0 == 2 && r1 == 8 && r2 == 5)
            || (r0 == 5 && r1 == 2 && r2 == 8)
            || (r0 == 5 && r1 == 8 && r2 == 2)
            || (r0 == 8 && r1 == 2 && r2 == 5)
            || (r0 == 8 && r1 == 5 && r2 == 2));
    }
    return false;
}

// 三色三同顺或者三同刻
static bool is_mixed_triple_chow_or_pung(tile_t t0, tile_t t1, tile_t t2) {
    if (!is_numbered_suit_quick(t0) || !is_numbered_suit_quick(t1) || !is_numbered_suit_quick(t2)) {
        return false;
    }
    suit_t s0 = tile_suit(t0);
    suit_t s1 = tile_suit(t1);
    suit_t s2 = tile_suit(t2);
    if (s0 != s1 && s0 != s2 && s1 != s2) {
        rank_t r0 = tile_rank(t0);
        rank_t r1 = tile_rank(t1);
        rank_t r2 = tile_rank(t2);
        return (r0 == r1 && r0 == r2);
    }
    return false;
}

// 三色三节高或者三色三步高
static bool is_mixed_shifted_chow_or_pung(tile_t t0, tile_t t1, tile_t t2) {
    if (!is_numbered_suit_quick(t0) || !is_numbered_suit_quick(t1) || !is_numbered_suit_quick(t2)) {
        return false;
    }
    suit_t s0 = tile_suit(t0);
    suit_t s1 = tile_suit(t1);
    suit_t s2 = tile_suit(t2);
    if (s0 != s1 && s0 != s2 && s1 != s2) {
        rank_t r0 = tile_rank(t0);
        rank_t r1 = tile_rank(t1);
        rank_t r2 = tile_rank(t2);
        return ((r1 + 1 == r0 && r0 + 1 == r2)
            || (r2 + 1 == r0 && r0 + 1 == r1)
            || (r0 + 1 == r1 && r1 + 1 == r2)
            || (r2 + 1 == r1 && r1 + 1 == r0)
            || (r0 + 1 == r2 && r2 + 1 == r1)
            || (r1 + 1 == r2 && r2 + 1 == r0));
    }
    return false;
}

// 一般高
static forceinline bool is_pure_double_chow(tile_t t0, tile_t t1) {
    return t0 == t1;
}

// 喜相逢
static forceinline bool is_mixed_double_chow(tile_t t0, tile_t t1) {
    return (is_rank_equal_quick(t0, t1) && !is_suit_equal_quick(t0, t1));
}

// 连六
static forceinline bool is_short_straight(tile_t t0, tile_t t1) {
    return (t0 + 3 == t1 || t1 + 3 == t0);
}

// 老少副
static bool is_two_terminal_chows(tile_t t0, tile_t t1) {
    if (is_suit_equal_quick(t0, t1)) {
        rank_t r0 = tile_rank(t0);
        rank_t r1 = tile_rank(t1);
        return ((r0 == 2 && r1 == 8) || (r0 == 8 && r1 == 2));
    }
    return false;
}

// 4组顺子的番
static fan_t get_4_chows_fan(tile_t t0, tile_t t1, tile_t t2, tile_t t3) {
    // 按出现频率顺序
    // 一色四步高
    if (is_four_pure_shifted_chows(t0, t1, t2, t3)) {
        return FOUR_PURE_SHIFTED_CHOWS;
    }
    // 一色四同顺
    if (is_quadruple_chow(t0, t1, t2, t3)) {
        return QUADRUPLE_CHOW;
    }
    // 以上都没有
    return FAN_NONE;
}

// 3组顺子的番
static fan_t get_3_chows_fan(tile_t t0, tile_t t1, tile_t t2) {
    // 按出现频率顺序
    // 三色三步高
    if (is_mixed_shifted_chow_or_pung(t0, t1, t2)) {
        return MIXED_SHIFTED_CHOWS;
    }
    // 三色三同顺
    if (is_mixed_triple_chow_or_pung(t0, t1, t2)) {
        return MIXED_TRIPLE_CHOW;
    }
    // 花龙
    if (is_mixed_straight(t0, t1, t2)) {
        return MIXED_STRAIGHT;
    }
    // 清龙
    if (is_pure_straight(t0, t1, t2)) {
        return PURE_STRAIGHT;
    }
    // 一色三步高
    if (is_pure_shifted_chows(t0, t1, t2)) {
        return PURE_SHIFTED_CHOWS;
    }
    // 一色三同顺
    if (is_pure_triple_chow(t0, t1, t2)) {
        return PURE_TRIPLE_CHOW;
    }
    // 以上都没有
    return FAN_NONE;
}

// 2组顺子的番
static fan_t get_2_chows_fan(tile_t t0, tile_t t1) {
    // 按出现频率顺序
    // 喜相逢
    if (is_mixed_double_chow(t0, t1)) {
        return MIXED_DOUBLE_CHOW;
    }
    // 连六
    if (is_short_straight(t0, t1)) {
        return SHORT_STRAIGHT;
    }
    // 老少副
    if (is_two_terminal_chows(t0, t1)) {
        return TWO_TERMINAL_CHOWS;
    }
    // 一般高
    if (is_pure_double_chow(t0, t1)) {
        return PURE_DOUBLE_CHOW;
    }
    // 以上都没有
    return FAN_NONE;
}

// 大四喜
static forceinline bool is_big_four_winds(tile_t t0, tile_t t1, tile_t t2, tile_t t3) {
    return (is_winds(t0) && is_winds(t1) && is_winds(t2) && is_winds(t3));
}

// 大三元
static forceinline bool is_big_three_dragons(tile_t t0, tile_t t1, tile_t t2) {
    return (is_dragons(t0) && is_dragons(t1) && is_dragons(t2));
}

// 三风刻
static forceinline bool is_big_three_winds(tile_t t0, tile_t t1, tile_t t2) {
    return (is_winds(t0) && is_winds(t1) && is_winds(t2));
}

// 双箭刻
static forceinline bool is_two_dragons_pungs(tile_t t0, tile_t t1) {
    return (is_dragons(t0) && is_dragons(t1));
}

// 双同刻
static forceinline bool is_double_pung(tile_t t0, tile_t t1) {
    return (is_numbered_suit_quick(t0) && is_numbered_suit_quick(t1)
        && is_rank_equal_quick(t0, t1));
}

// 4组刻子的番
static fan_t get_4_pungs_fan(tile_t t0, tile_t t1, tile_t t2, tile_t t3) {
    // 一色四节高
    if (is_four_pure_shifted_pungs(t0, t1, t2, t3)) {
        return FOUR_PURE_SHIFTED_PUNGS;
    }
    // 大四喜
    if (is_big_four_winds(t0, t1, t2, t3)) {
        return BIG_FOUR_WINDS;
    }
    // 以上都没有
    return FAN_NONE;
}

// 3组刻子的番
static fan_t get_3_pungs_fan(tile_t t0, tile_t t1, tile_t t2) {
    // 按出现频率顺序
    // 三色三节高
    if (is_mixed_shifted_chow_or_pung(t0, t1, t2)) {
        return MIXED_SHIFTED_PUNGS;
    }
    // 一色三节高
    if (is_pure_shifted_pungs(t0, t1, t2)) {
        return PURE_SHIFTED_PUNGS;
    }
    // 三同刻
    if (is_mixed_triple_chow_or_pung(t0, t1, t2)) {
        return TRIPLE_PUNG;
    }
    // 三风刻
    if (is_big_three_winds(t0, t1, t2)) {
        return BIG_THREE_WINDS;
    }
    // 大三元
    if (is_big_three_dragons(t0, t1, t2)) {
        return BIG_THREE_DRAGONS;
    }
    // 以上都没有
    return FAN_NONE;
}

// 2组刻子的番
static fan_t get_2_pungs_fan(tile_t t0, tile_t t1) {
    // 按出现频率顺序
    // 双同刻
    if (is_double_pung(t0, t1)) {
        return DOUBLE_PUNG;
    }
    // 双箭刻
    if (is_two_dragons_pungs(t0, t1)) {
        return TWO_DRAGONS_PUNGS;
    }
    // 以上都没有
    return FAN_NONE;
}

// 1组刻子的番
static fan_t get_1_pung_fan(tile_t mid_tile) {
    // 箭刻
    if (is_dragons(mid_tile)) {
        return DRAGON_PUNG;
    }
    // 幺九刻
    if (is_terminal(mid_tile) || is_winds(mid_tile)) {
        return PUNG_OF_TERMINALS_OR_HONORS;
    }
    // 以上都没有
    return FAN_NONE;
}

// 存在3组顺子的番种时，余下的第4组顺子最多算1番
static fan_t get_1_chow_extra(tile_t tile0, tile_t tile1, tile_t tile2, tile_t tile_extra) {
    fan_t fan0 = get_2_chows_fan(tile0, tile_extra);
    fan_t fan1 = get_2_chows_fan(tile1, tile_extra);
    fan_t fan2 = get_2_chows_fan(tile2, tile_extra);

#define TEST_RETURN_FAN(fan_) \
    if (fan0 == fan_ || fan1 == fan_ || fan2 == fan_) return fan_

    TEST_RETURN_FAN(PURE_DOUBLE_CHOW);  // 一般高
    TEST_RETURN_FAN(MIXED_DOUBLE_CHOW);  // 喜相逢
    TEST_RETURN_FAN(SHORT_STRAIGHT);  // 连六
    TEST_RETURN_FAN(TWO_TERMINAL_CHOWS);  // 老少副

#undef TEST_RETURN_FAN
    return FAN_NONE;
}

template <long _Size>
static fan_t *pairwise_test_chows(const tile_t (&chows_mid_tile)[_Size], fan_t *selected_fan) {
    fan_t all_fan[_Size][_Size] = { { FAN_NONE } };

    // 初始化矩阵
    for (int i = 0; i < _Size; ++i) {
        for (int j = 0; j < i - 1; ++j) {  // 这是对称矩阵
            all_fan[i][j] = all_fan[j][i];
        }
        for (int j = i + 1; j < _Size; ++j) {  // 获取所有两两组合的番种
            all_fan[i][j] = get_2_chows_fan(chows_mid_tile[i], chows_mid_tile[j]);
        }
    }

    // 套算一次原则：
    // 如果有尚未组合过的一副牌，只可同已组合过的相应的一副牌套算一次
    //
    // 不得相同原则：
    // 凡已经合过某一番种的牌，不能再同其他一副牌组成相同的番种计分
    //
    // 根据套算一次原则，234567s234567p，只能计为“喜相逢*2 连六*1”或者“喜相逢*1 连六*2”，而不是“喜相逢*2 连六*2”
    // 根据以上两点，234s223344567p，只能计为：“喜相逢、一般高、连六”，而不是“喜相逢*2、连六”
    unsigned used_flag[_Size] = { 0 };
    for (int i = 0; i < _Size; ++i) {
        for (int j = 0; j < _Size; ++j) {
            if (i == j) {
                continue;
            }

            // 套算一次原则，两组都已经使用过，就不再组合
            if (used_flag[i] && used_flag[j]) {
                continue;
            }
            fan_t pt = all_fan[i][j];
            if (pt != FAN_NONE) {
                int idx = pt - PURE_DOUBLE_CHOW;  // 般逢连老
                // 不得相同原则，如果i和j都没算过某一种番，则算这种番
                if ((used_flag[i] & (1 << idx)) == 0 && (used_flag[j] & (1 << idx)) == 0) {
                    used_flag[i] |= (1 << (pt - PURE_DOUBLE_CHOW));
                    used_flag[j] |= (1 << (pt - PURE_DOUBLE_CHOW));
                    *selected_fan = pt;  // 写入这个番
                    ++selected_fan;
                }
            }
        }
    }
    return selected_fan;
}

// 4组顺子算番
static void calculate_4_chows(const pack_t chow_packs[4], fan_table_t &fan_table) {
    tile_t tiles[4];
    tiles[0] = pack_tile(chow_packs[0]);
    tiles[1] = pack_tile(chow_packs[1]);
    tiles[2] = pack_tile(chow_packs[2]);
    tiles[3] = pack_tile(chow_packs[3]);
    std::sort(std::begin(tiles), std::end(tiles));

    fan_t fan;
    // 存在4组顺子的番种时，不再检测其他的了
    if ((fan = get_4_chows_fan(tiles[0], tiles[1], tiles[2], tiles[3]))) {
        fan_table[fan] = 1;
        return;
    }

    // 3组顺子判断
    // 012构成3组顺子的番种
    if ((fan = get_3_chows_fan(tiles[0], tiles[1], tiles[2])) != FAN_NONE) {
        fan_table[fan] = 1;
        if ((fan = get_1_chow_extra(tiles[0], tiles[1], tiles[2], tiles[3])) != FAN_NONE) {
            fan_table[fan] = 1;
        }
        return;
    }
    // 013构成3组顺子的番种
    else if ((fan = get_3_chows_fan(tiles[0], tiles[1], tiles[3])) != FAN_NONE) {
        fan_table[fan] = 1;
        if ((fan = get_1_chow_extra(tiles[0], tiles[1], tiles[3], tiles[2])) != FAN_NONE) {
            fan_table[fan] = 1;
        }
        return;
    }
    // 023构成3组顺子的番种
    else if ((fan = get_3_chows_fan(tiles[0], tiles[2], tiles[3])) != FAN_NONE) {
        fan_table[fan] = 1;
        if ((fan = get_1_chow_extra(tiles[0], tiles[2], tiles[3], tiles[1])) != FAN_NONE) {
            fan_table[fan] = 1;
        }
        return;
    }
    // 123构成3组顺子的番种
    else if ((fan = get_3_chows_fan(tiles[1], tiles[2], tiles[3])) != FAN_NONE) {
        fan_table[fan] = 1;
        if ((fan = get_1_chow_extra(tiles[1], tiles[2], tiles[3], tiles[0])) != FAN_NONE) {
            fan_table[fan] = 1;
        }
        return;
    }

    // 不存在3组顺子的番种时，4组顺子最多3番
    fan_t selected_fan[3] = { FAN_NONE };
    pairwise_test_chows(tiles, selected_fan);
    for (long i = 0; i < 3; ++i) {
        if (selected_fan[i] != FAN_NONE) {
            ++fan_table[selected_fan[i]];
        }
    }
}

// 3组顺子算番
static void calculate_3_chows(const pack_t chow_packs[3], fan_table_t &fan_table) {
    tile_t tiles[3];
    tiles[0] = pack_tile(chow_packs[0]);
    tiles[1] = pack_tile(chow_packs[1]);
    tiles[2] = pack_tile(chow_packs[2]);
    std::sort(std::begin(tiles), std::end(tiles));

    fan_t fan;

    // 存在3组顺子的番种时，不再检测其他的
    if ((fan = get_3_chows_fan(tiles[0], tiles[1], tiles[2])) != FAN_NONE) {
        fan_table[fan] = 1;
        return;
    }

    // 不存在上述番种时，3组顺子最多2番
    fan_t selected_fan[2] = { FAN_NONE };
    pairwise_test_chows(tiles, selected_fan);
    for (long i = 0; i < 2; ++i) {
        if (selected_fan[i] != FAN_NONE) {
            ++fan_table[selected_fan[i]];
        }
    }
}

// 2组顺子算番
static void calculate_2_chows(const pack_t chow_packs[2], fan_table_t &fan_table) {
    tile_t tiles[2];
    tiles[0] = pack_tile(chow_packs[0]);
    tiles[1] = pack_tile(chow_packs[1]);
    fan_t fan;
    if ((fan = get_2_chows_fan(tiles[0], tiles[1])) != FAN_NONE) {
        ++fan_table[fan];
    }
}

// 刻子（杠）算番
static void calculate_kongs(const pack_t *pung_packs, long pung_cnt, fan_table_t &fan_table) {
    // 统计明杠 暗杠 明刻 暗刻
    int melded_kong_cnt = 0;
    int concealed_kong_cnt = 0;
    int concealed_pung_cnt = 0;
    for (long i = 0; i < pung_cnt; ++i) {
        if (is_pack_melded(pung_packs[i])) {
            if (pack_type(pung_packs[i]) == PACK_TYPE_KONG) {
                ++melded_kong_cnt;
            }
        }
        else {
            if (pack_type(pung_packs[i]) == PACK_TYPE_KONG) {
                ++concealed_kong_cnt;
            }
            else {
                ++concealed_pung_cnt;
            }
        }
    }

    // 规则
    // 三杠
    // 明杠 明杠 暗杠 暗刻 -> 三杠+双暗刻+碰碰和
    // 明杠 暗杠 暗杠 明刻 -> 三杠+双暗刻+碰碰和
    // 明杠 暗杠 暗杠 暗刻 -> 三杠+三暗刻+碰碰和
    // 暗杠 暗杠 暗杠 明刻 -> 三杠+三暗刻+碰碰和
    // 暗杠 暗杠 暗杠 暗刻 -> 三杠+四暗刻
    //
    // 四杠
    // 暗杠 明杠 明杠 明杠 -> 四杠
    // 暗杠 暗杠 明杠 明杠 -> 四杠+双暗刻
    // 暗杠 暗杠 暗杠 明杠 -> 四杠+三暗刻
    // 暗杠 暗杠 暗杠 暗杠 -> 四杠+四暗刻
    //

    int kong_cnt = melded_kong_cnt + concealed_kong_cnt;
    switch (kong_cnt) {
    case 0:  // 0个杠
        switch (concealed_pung_cnt) {  // 暗刻的个数
        case 0: break;
        case 1: break;
        case 2: fan_table[TWO_CONCEALED_PUNGS] = 1; break;
        case 3: fan_table[THREE_CONCEALED_PUNGS] = 1; break;
        case 4: fan_table[FOUR_CONCEALED_PUNGS] = 1; break;
        default: assert(0); break;
        }
        break;
    case 1:  // 1个杠
        if (melded_kong_cnt == 1) {  // 明杠
            fan_table[MELDED_KONG] = 1;
            switch (concealed_pung_cnt) {  // 暗刻的个数
            case 0: break;
            case 1: break;
            case 2: fan_table[TWO_CONCEALED_PUNGS] = 1; break;
            case 3: fan_table[THREE_CONCEALED_PUNGS] = 1; break;
            default: assert(0); break;
            }
        }
        else {  // 暗杠
            fan_table[CONCEALED_KONG] = 1;
            switch (concealed_pung_cnt) {  // 暗刻的个数
            case 0: break;
            case 1: fan_table[TWO_CONCEALED_PUNGS] = 1; break;
            case 2: fan_table[THREE_CONCEALED_PUNGS] = 1; break;
            case 3: fan_table[FOUR_CONCEALED_PUNGS] = 1; break;
            default: assert(0); break;
            }
        }
        break;
    case 2:  // 2个杠
        switch (concealed_kong_cnt) {
        case 0:  // 双明杠
            fan_table[TWO_MELDED_KONGS] = 1;
            switch (concealed_pung_cnt) {  // 暗刻的个数
            case 0: break;
            case 1: break;
            case 2: fan_table[TWO_CONCEALED_PUNGS] = 1; break;
            default: assert(0); break;
            }
            break;
        case 1:  // 明暗杠
#if SUPPORT_CONCEALED_KONG_AND_MELDED_KONG
            fan_table[CONCEALED_KONG_AND_MELDED_KONG] = 1;
#else
            fan_table[MELDED_KONG] = 1;
            fan_table[CONCEALED_KONG] = 1;
#endif
            switch (concealed_pung_cnt) {  // 暗刻的个数
            case 0: break;
            case 1: fan_table[TWO_CONCEALED_PUNGS] = 1; break;
            case 2: fan_table[THREE_CONCEALED_PUNGS] = 1; break;
            default: assert(0); break;
            }
            break;
        case 2:  // 双暗杠
            fan_table[TWO_CONCEALED_KONGS] = 1;
            switch (concealed_pung_cnt) {  // 暗刻的个数
            case 0: break;
            case 1: fan_table[THREE_CONCEALED_PUNGS] = 1; break;
            case 2: fan_table[FOUR_CONCEALED_PUNGS] = 1; break;
            default: assert(0); break;
            }
            break;
        default:
            assert(0);
            break;
        }
        break;
    case 3:  // 3个杠
        fan_table[THREE_KONGS] = 1;
        switch (concealed_kong_cnt) {  // 暗刻的个数
        case 0: break;
        case 1:
            if (concealed_pung_cnt > 0) {
                fan_table[TWO_CONCEALED_PUNGS] = 1;
            }
            break;
        case 2:
            if (concealed_pung_cnt == 0) {
                fan_table[TWO_CONCEALED_PUNGS] = 1;
            }
            else {
                fan_table[THREE_CONCEALED_PUNGS] = 1;
            }
            break;
        case 3:
            if (concealed_pung_cnt == 0) {
                fan_table[THREE_CONCEALED_PUNGS] = 1;
            }
            else {
                fan_table[FOUR_CONCEALED_PUNGS] = 1;
            }
            break;
        default:
            assert(0);
            break;
        }
        break;
    case 4:  // 4个杠
        fan_table[FOUR_KONGS] = 1;
        switch (concealed_kong_cnt) {
        case 0: break;
        case 1: break;
        case 2: fan_table[TWO_CONCEALED_PUNGS] = 1; break;
        case 3: fan_table[THREE_CONCEALED_PUNGS] = 1; break;
        case 4: fan_table[FOUR_CONCEALED_PUNGS] = 1; break;
        default: assert(0); break;
        }
        break;
    default:
        assert(0);
        break;
    }

    // 四杠和四暗刻不计碰碰和，其他先加上碰碰和的番
    if (pung_cnt == 4) {
        if (fan_table[FOUR_KONGS] == 0 && fan_table[FOUR_CONCEALED_PUNGS] == 0) {
            fan_table[ALL_PUNGS] = 1;
        }
    }

    // 逐组刻子的番（箭刻、幺九刻）
    for (long i = 0; i < pung_cnt; ++i) {
        fan_t fan = get_1_pung_fan(pack_tile(pung_packs[i]));
        if (fan != FAN_NONE) {
            ++fan_table[fan];
        }
    }
}

// 4组刻子算番
static void calculate_4_pungs(const pack_t pung_packs[4], fan_table_t &fan_table) {
    tile_t tiles[4];
    tiles[0] = pack_tile(pung_packs[0]);
    tiles[1] = pack_tile(pung_packs[1]);
    tiles[2] = pack_tile(pung_packs[2]);
    tiles[3] = pack_tile(pung_packs[3]);
    std::sort(std::begin(tiles), std::end(tiles));

    calculate_kongs(pung_packs, 4, fan_table);

    fan_t fan;
    // 存在4组刻子的番种时，不再检测其他的了
    if ((fan = get_4_pungs_fan(tiles[0], tiles[1], tiles[2], tiles[3])) != FAN_NONE) {
        fan_table[fan] = 1;
        return;
    }

    // 3组刻子判断
    bool _3_pungs_has_fan = false;
    long free_pack_idx = -1;  // 未使用的1组刻子
    // 012构成3组刻子的番种
    if ((fan = get_3_pungs_fan(tiles[0], tiles[1], tiles[2])) != FAN_NONE) {
        fan_table[fan] = 1;
        free_pack_idx = 3;
        _3_pungs_has_fan = true;
    }
    // 013构成3组刻子的番种
    else if ((fan = get_3_pungs_fan(tiles[0], tiles[1], tiles[3])) != FAN_NONE) {
        fan_table[fan] = 1;
        free_pack_idx = 2;
        _3_pungs_has_fan = true;
    }
    // 023构成3组刻子的番种
    else if ((fan = get_3_pungs_fan(tiles[0], tiles[2], tiles[3])) != FAN_NONE) {
        fan_table[fan] = 1;
        free_pack_idx = 1;
        _3_pungs_has_fan = true;
    }
    // 123构成3组刻子的番种
    else if ((fan = get_3_pungs_fan(tiles[1], tiles[2], tiles[3])) != FAN_NONE) {
        fan_table[fan] = 1;
        free_pack_idx = 0;
        _3_pungs_has_fan = true;
    }

    // 存在3组刻子的番种时，余下的第4组刻子只能组合一次
    if (_3_pungs_has_fan) {
        for (long i = 0; i < 4; ++i) {
            if (i == free_pack_idx) {
                continue;
            }
            // 依次与未使用的这组刻子测试番种
            if ((fan = get_2_pungs_fan(tiles[i], tiles[free_pack_idx])) != FAN_NONE) {
                ++fan_table[fan];
                break;
            }
        }
        return;
    }

    // 不存在3组刻子的番种时，两两计算番种
    if ((fan = get_2_pungs_fan(tiles[0], tiles[1])) != FAN_NONE) {
        ++fan_table[fan];
    }
    if ((fan = get_2_pungs_fan(tiles[0], tiles[2])) != FAN_NONE) {
        ++fan_table[fan];
    }
    if ((fan = get_2_pungs_fan(tiles[0], tiles[3])) != FAN_NONE) {
        ++fan_table[fan];
    }
    if ((fan = get_2_pungs_fan(tiles[1], tiles[2])) != FAN_NONE) {
        ++fan_table[fan];
    }
    if ((fan = get_2_pungs_fan(tiles[1], tiles[3])) != FAN_NONE) {
        ++fan_table[fan];
    }
    if ((fan = get_2_pungs_fan(tiles[2], tiles[3])) != FAN_NONE) {
        ++fan_table[fan];
    }
}

// 3组刻子算番
static void calculate_3_pungs(const pack_t pung_packs[3], fan_table_t &fan_table) {
    tile_t tiles[3];
    tiles[0] = pack_tile(pung_packs[0]);
    tiles[1] = pack_tile(pung_packs[1]);
    tiles[2] = pack_tile(pung_packs[2]);

    calculate_kongs(pung_packs, 3, fan_table);

    fan_t fan;

    // 存在3组刻子的番种（三节高 三同刻 三风刻 大三元）时，不再检测其他的
    if ((fan = get_3_pungs_fan(tiles[0], tiles[1], tiles[2])) != FAN_NONE) {
        fan_table[fan] = 1;
        return;
    }

    // 不存在3组刻子的番种时，两两计算番种
    if ((fan = get_2_pungs_fan(tiles[0], tiles[1])) != FAN_NONE) {
        ++fan_table[fan];
    }
    if ((fan = get_2_pungs_fan(tiles[0], tiles[2])) != FAN_NONE) {
        ++fan_table[fan];
    }
    if ((fan = get_2_pungs_fan(tiles[1], tiles[2])) != FAN_NONE) {
        ++fan_table[fan];
    }
}

// 2组刻子算番
static void calculate_2_pungs(const pack_t pung_packs[2], fan_table_t &fan_table) {
    calculate_kongs(pung_packs, 2, fan_table);
    fan_t fan = get_2_pungs_fan(pack_tile(pung_packs[0]), pack_tile(pung_packs[1]));
    if (fan != FAN_NONE) {
        ++fan_table[fan];
    }
}

// 1组刻子算番
static void calculate_1_pung(pack_t pung_pack, fan_table_t &fan_table) {
    calculate_kongs(&pung_pack, 1, fan_table);
}

// 九莲宝灯
static bool is_nine_gates(const tile_t (&tiles)[14], tile_t win_tile) {
    // 对立牌的种类进行打表
    tile_table_t cnt_table;
    map_tiles(tiles, 14, cnt_table);
    // 去除和牌张
    --cnt_table[win_tile];

    // 1、9各三枚，2~8各一枚
#define IS_NINE_GATES(s_) \
    (cnt_table[0x ## s_ ## 1] == 3 && cnt_table[0x ## s_ ## 9] == 3 \
    && std::all_of(cnt_table + 0x ## s_ ## 2, cnt_table + 0x ## s_ ## 9, [](int n) { return n == 1; }))
    return IS_NINE_GATES(1) || IS_NINE_GATES(2) || IS_NINE_GATES(3);
#undef IS_NINE_GATES
}

// 一色双龙会
static bool is_pure_terminal_chows(const pack_t (&chow_packs)[4], pack_t pair_pack) {
    if (tile_rank(pack_tile(pair_pack)) != 5) {  // 5作雀头
        return false;
    }

    int _123_cnt = 0, _789_cnt = 0;
    suit_t pair_suit = tile_suit(pack_tile(pair_pack));
    for (long i = 0; i < 4; ++i) {
        suit_t suit = tile_suit(pack_tile(chow_packs[i]));
        if (suit != pair_suit) {  // 花色与雀头相同
            return false;
        }
        rank_t rank = tile_rank(pack_tile(chow_packs[i]));
        switch (rank) {
        case 2: ++_123_cnt; break;
        case 8: ++_789_cnt; break;
        default: return false;
        }
    }
    return (_123_cnt == 2 && _789_cnt == 2);  // 123和789各2组
}

// 三色双龙会
static bool is_three_suited_terminal_chows(const pack_t (&chow_packs)[4], pack_t pair_pack) {
    if (tile_rank(pack_tile(pair_pack)) != 5) {  // 5作雀头
        return false;
    }

    int _123_suit_table[4] = { 0 };
    int _789_suit_table[4] = { 0 };
    suit_t pair_suit = tile_suit(pack_tile(pair_pack));
    for (long i = 0; i < 4; ++i) {
        suit_t suit = tile_suit(pack_tile(chow_packs[i]));
        if (suit == pair_suit) {  // 花色与雀头不相同
            return false;
        }
        rank_t rank = tile_rank(pack_tile(chow_packs[i]));
        switch (rank) {
        case 2: ++_123_suit_table[suit]; break;
        case 8: ++_789_suit_table[suit]; break;
        default: return false;
        }
    }

    // 与雀头花色不同的两色123和789各一组
    switch (pair_suit) {
    case 1: return (_123_suit_table[2] && _123_suit_table[3] && _789_suit_table[2] && _789_suit_table[3]);
    case 2: return (_123_suit_table[1] && _123_suit_table[3] && _789_suit_table[1] && _789_suit_table[3]);
    case 3: return (_123_suit_table[1] && _123_suit_table[2] && _789_suit_table[1] && _789_suit_table[2]);
    default: return false;
    }
}

// 检测不求人、全求人
static void check_melded_or_concealed_hand(const pack_t (&packs)[5], long fixed_cnt, bool self_drawn, fan_table_t &fan_table) {
    long melded_cnt = std::count_if(&packs[0], &packs[fixed_cnt], &is_pack_melded);  // 明副露的组数

    switch (melded_cnt) {
    case 0:  // 0组明的，自摸为不求人，点和为门前清
        fan_table[self_drawn ? FULLY_CONCEALED_HAND : CONCEALED_HAND] = 1;
        break;
    case 4:
        // 4组明的，自摸还是自摸，点和为全求人
        fan_table[self_drawn ? SELF_DRAWN : MELDED_HAND] = 1;
        break;
    default:
        if (self_drawn) {
            fan_table[SELF_DRAWN] = 1;
        }
        break;
    }
}

// 检测雀头，确定平和、小三元、小四喜
static void check_pair_tile(tile_t pair_tile, long chow_cnt, fan_table_t &fan_table) {
    if (chow_cnt == 4) {  // 4组都是顺子
        if (is_numbered_suit_quick(pair_tile)) {  // 数牌雀头
            fan_table[ALL_CHOWS] = 1;  // 平和
        }
        return;
    }

    // 在双箭刻的基础上，如果雀头是箭牌，则修正为小三元
    if (fan_table[TWO_DRAGONS_PUNGS]) {
        if (is_dragons(pair_tile)) {
            fan_table[LITTLE_THREE_DRAGONS] = 1;
            fan_table[TWO_DRAGONS_PUNGS] = 0;
        }
        return;
    }
    // 在三风刻的基础上，如果雀头是风牌，则修正为小四喜
    if (fan_table[BIG_THREE_WINDS]) {
        if (is_winds(pair_tile)) {
            fan_table[LITTLE_FOUR_WINDS] = 1;
            fan_table[BIG_THREE_WINDS] = 0;
        }
        return;
    }
}

// 检测门（五门齐的门）
static void check_tiles_suits(const tile_t *tiles, long tile_cnt, fan_table_t &fan_table) {
    // 打表标记有哪些花色
    uint8_t suit_flag = 0;
    for (long i = 0; i < tile_cnt; ++i) {
        suit_flag |= (1 << tile_suit(tiles[i]));
    }

    // 1111 0001
    if (!(suit_flag & UINT8_C(0xF1))) {
        fan_table[NO_HONORS] = 1;  // 无字
    }

    // 1110 0011
    if (!(suit_flag & UINT8_C(0xE3))) {
        ++fan_table[ONE_VOIDED_SUIT];  // 缺一门（万）
    }
    // 1110 0101
    if (!(suit_flag & UINT8_C(0xE5))) {
        ++fan_table[ONE_VOIDED_SUIT];  // 缺一门（条）
    }
    // 1110 1001
    if (!(suit_flag & UINT8_C(0xE9))) {
        ++fan_table[ONE_VOIDED_SUIT];  // 缺一门（饼）
    }

    // 当缺2门时，根据有字和无字，修正为混一色和清一色
    if (fan_table[ONE_VOIDED_SUIT] == 2) {
        fan_table[ONE_VOIDED_SUIT] = 0;
        fan_table[suit_flag & UINT8_C(0xF1) ? HALF_FLUSH : FULL_FLUSH] = 1;
    }

    // 0001 1110
    if (suit_flag == UINT8_C(0x1E)) {  // 三门数牌和字牌都有
        if (std::any_of(tiles, tiles + tile_cnt, &is_winds)
            && std::any_of(tiles, tiles + tile_cnt, &is_dragons)) {
            fan_table[ALL_TYPES] = 1;  // 五门齐
        }
    }
}

// 检测数牌的范围（大于五、小于五、全大、全中、全小）
static void check_tiles_rank_range(const tile_t *tiles, long tile_cnt, fan_table_t &fan_table) {
#ifdef STRICT_98_RULE
    if (fan_table[SEVEN_PAIRS]) {
        return;  // 严格98规则的七对不支持叠加这些
    }
#endif

    // 打表标记有哪些数
    uint16_t rank_flag = 0;
    for (long i = 0; i < tile_cnt; ++i) {
        if (!is_numbered_suit_quick(tiles[i])) {
            return;
        }
        rank_flag |= (1 << tile_rank(tiles[i]));
    }

    // 1111 1111 1110 0001
    // 检测是否只包含1234
    if (!(rank_flag & UINT16_C(0xFFE1))) {
        // 包含4为小于五，否则为全小
        fan_table[rank_flag & UINT16_C(0x0010) ? LOWER_FOUR : LOWER_TILES] = 1;
        return;
    }
    // 1111 1100 0011 1111
    // 检测是否只包含6789
    if (!(rank_flag & UINT16_C(0xFC3F))) {
        // 包含6为大于五，否则为全大
        fan_table[rank_flag & UINT16_C(0x0040) ? UPPER_FOUR : UPPER_TILES] = 1;
        return;
    }
    // 1111 1111 1000 1111
    // 检测是否只包含456
    if (!(rank_flag & UINT16_C(0xFF8F))) {
        // 全中
        fan_table[MIDDLE_TILES] = 1;
    }
}

// 判断一组牌是否包含数牌19
static bool is_pack_contains_terminal_tile(pack_t pack) {
    tile_t tile = pack_tile(pack);
    if (!is_numbered_suit_quick(tile)) {
        return false;
    }
    rank_t rank = tile_rank(tile);
    if (pack_type(pack) == PACK_TYPE_CHOW) {
        return (rank == 2 || rank == 8);
    }
    else {
        return (rank == 1 || rank == 9);
    }
}

// 判断一组牌是否包含数牌5
static bool is_pack_contains_5(pack_t pack) {
    tile_t tile = pack_tile(pack);
    if (!is_numbered_suit_quick(tile)) {
        return false;
    }
    rank_t rank = tile_rank(tile);
    if (pack_type(pack) == PACK_TYPE_CHOW) {
        return (rank >= 4 && rank <= 6);
    }
    else {
        return rank == 5;
    }
}

// 检测全带幺、全带五、全双刻
static void check_tiles_rank_by_pack(const pack_t (&packs)[5], fan_table_t &fan_table) {
    // 统计包含数牌19、字牌、5、双数牌的组数
    int terminal_cnt = 0;
    int honor_cnt = 0;
    int _5_cnt = 0;
    int even_pung_cnt = 0;
    for (long i = 0; i < 5; ++i) {
        tile_t tile = pack_tile(packs[i]);
        if (is_pack_contains_terminal_tile(packs[i])) {
            ++terminal_cnt;  // 数牌19
        }
        else if (pack_type(packs[i]) != PACK_TYPE_CHOW
            && is_honor(tile)) {
            ++honor_cnt;  // 字牌
        }
        else if (is_pack_contains_5(packs[i])) {
            ++_5_cnt;  // 5
        }
        else if (pack_type(packs[i]) != PACK_TYPE_CHOW
            && is_numbered_suit_quick(tile) && (tile & 1) == 0) {
            ++even_pung_cnt;  // 双数牌刻子
        }
    }

    // 5组牌都包含数牌19和字牌，先暂时计为全带幺
    if (terminal_cnt + honor_cnt == 5) {
        fan_table[OUTSIDE_HAND] = 1;
        return;
    }
    // 全带五
    if (_5_cnt == 5) {
        fan_table[ALL_FIVE] = 1;
        return;
    }
    // 全双刻
    if (even_pung_cnt == 5) {
        fan_table[ALL_EVEN_PUNGS] = 1;
    }
}

// 检测特性（断幺、推不倒、绿一色、字一色、清幺九、混幺九）
static void check_tiles_traits(const tile_t *tiles, long tile_cnt, fan_table_t &fan_table) {
    // 断幺
    if (!std::any_of(tiles, tiles + tile_cnt, &is_terminal_or_honor)) {
        fan_table[ALL_SIMPLES] = 1;
    }

    // 推不倒
    if (std::all_of(tiles, tiles + tile_cnt, &is_reversible)) {
        fan_table[REVERSIBLE_TILES] = 1;
    }

#ifdef STRICT_98_RULE
    if (fan_table[SEVEN_PAIRS]) {
        return;  // 严格98规则的七对不支持绿一色、清幺九、混幺九
    }
#endif

    // 绿一色
    if (std::all_of(tiles, tiles + tile_cnt, &is_green)) {
        fan_table[ALL_GREEN] = 1;
    }

    // 如果断幺了就没必要检测字一色、清幺九、混幺九了
    if (fan_table[ALL_SIMPLES] != 0) {
        return;
    }

    // 字一色
    if (std::all_of(tiles, tiles + tile_cnt, &is_honor)) {
        fan_table[ALL_HONORS] = 1;
        return;
    }
    // 清幺九
    if (std::all_of(tiles, tiles + tile_cnt, &is_terminal)) {
        fan_table[ALL_TERMINALS] = 1;
        return;
    }
    // 混幺九
    if (std::all_of(tiles, tiles + tile_cnt, &is_terminal_or_honor)) {
        fan_table[ALL_TERMINALS_AND_HONORS] = 1;
    }
}

// 检测四归一
static void check_tiles_hog(const tile_t *tiles, long tile_cnt, fan_table_t &fan_table) {
    long kong_cnt = tile_cnt - 14;  // 标准和牌14张，多出几张就说明有几个杠
    tile_table_t cnt_table;
    map_tiles(tiles, tile_cnt, cnt_table);
    // 有多少种已经用去4张的牌减去杠的数量，即为四归一的数量
    long _4_cnt = std::count(std::begin(cnt_table), std::end(cnt_table), 4);
    fan_table[TILE_HOG] = _4_cnt - kong_cnt;
}

// 移除和牌
static void remove_win_tile(tile_t *standing_tiles, long tile_cnt, tile_t win_tile) {
    tile_t *it = std::find(standing_tiles, standing_tiles + tile_cnt, win_tile);
    long idx = it - standing_tiles;
    if (idx != tile_cnt) {
        memmove(it, it + 1, (tile_cnt - idx - 1) * sizeof(tile_t));
    }
}

// 检测边嵌钓
static void check_edge_closed_single_wait(const pack_t *concealed_packs, long pack_cnt, tile_t win_tile, fan_table_t &fan_table) {
    // 全求人和四杠不计单钓将，也不可能有边张、嵌张
    if (fan_table[MELDED_HAND] || fan_table[FOUR_KONGS]) {
        return;
    }

    // 恢复成一张一张的牌
    tile_t standing_tiles[14];
    long tile_cnt = packs_to_tiles(concealed_packs, pack_cnt, standing_tiles, 14);
    std::sort(standing_tiles, standing_tiles + tile_cnt);
    remove_win_tile(standing_tiles, tile_cnt, win_tile);
    --tile_cnt;

    useful_table_t waiting_table;  // 听牌标记表
    is_basic_type_wait(standing_tiles, tile_cnt, &waiting_table);

    if (pack_cnt == 5) {
        // 判断是否为七对听牌
        useful_table_t temp_table;
        if (is_seven_pairs_wait(standing_tiles, tile_cnt, &temp_table)) {
            // 合并听牌标记表
            std::transform(std::begin(temp_table), std::end(temp_table), std::begin(waiting_table),
                std::begin(waiting_table), [](bool w, bool t) { return w || t; });
        }
    }

    // 统计听牌张数
    long waiting_cnt = std::count(std::begin(waiting_table), std::end(waiting_table), true);

    // 听牌数大于1张，不计边嵌钓
    if (waiting_cnt != 1) {
        return;
    }

    // 听1张的情况，看和牌张处于什么位置
    bool maybe_edge = false;
    bool maybe_closed = false;
    bool maybe_single = false;

    for (long i = 0; i < pack_cnt; ++i) {
        switch (pack_type(concealed_packs[i])) {
        case PACK_TYPE_CHOW:
            if (pack_tile(concealed_packs[i]) == win_tile) {
                maybe_closed = true;  // 嵌张
            }
            else if (pack_tile(concealed_packs[i]) + 1 == win_tile
                || pack_tile(concealed_packs[i]) - 1 == win_tile) {
                maybe_edge = true;  // 边张
            }
            break;
        case PACK_TYPE_PAIR:
            if (pack_tile(concealed_packs[i]) == win_tile) {
                maybe_single = true;  // 单钓
            }
            break;
        default:
            break;
        }
    }

    // 当多种可能存在时，只能计其中一种
    if (maybe_edge) {
        fan_table[EDGE_WAIT] = 1;
        return;
    }
    if (maybe_closed) {
        fan_table[CLOSED_WAIT] = 1;
        return;
    }
    if (maybe_single) {
        fan_table[SINGLE_WAIT] = 1;
        return;
    }
}

// 检测圈风刻、门风刻
static void check_wind_pungs(pack_t packs, wind_t prevalent_wind, wind_t seat_wind, fan_table_t &fan_table) {
    uint8_t type = pack_type(packs);
    if (type == PACK_TYPE_PUNG || type == PACK_TYPE_KONG) {
        rank_t delta = pack_tile(packs) - 0x41;
        if (delta == (int)prevalent_wind - (int)wind_t::EAST) {
            fan_table[PREVALENT_WIND] = 1;
        }
        if (delta == (int)seat_wind - (int)wind_t::EAST) {
            fan_table[SEAT_WIND] = 1;
        }
    }
}

// 统一校正一些不计的
static void correction_fan_table(fan_table_t &fan_table, bool prevalent_eq_seat) {
    // 大四喜不计三风刻、碰碰和、圈风刻、门风刻、幺九刻
    if (fan_table[BIG_FOUR_WINDS]) {
        fan_table[BIG_THREE_WINDS] = 0;
        fan_table[ALL_PUNGS] = 0;
        fan_table[PREVALENT_WIND] = 0;
        fan_table[SEAT_WIND] = 0;
        fan_table[PUNG_OF_TERMINALS_OR_HONORS] = 0;
    }
    // 大三元不计双箭刻、箭刻（严格98规则不计缺一门）
    if (fan_table[BIG_THREE_DRAGONS]) {
        fan_table[TWO_DRAGONS_PUNGS] = 0;
        fan_table[DRAGON_PUNG] = 0;
#ifdef STRICT_98_RULE
        fan_table[ONE_VOIDED_SUIT] = 0;
#endif
    }
    // 绿一色不计混一色、缺一门
    if (fan_table[ALL_GREEN]) {
        fan_table[HALF_FLUSH] = 0;
        fan_table[ONE_VOIDED_SUIT] = 0;
    }
    // 九莲宝灯不计清一色、门前清、缺一门、无字，减计1个幺九刻，把不求人修正为自摸
    if (fan_table[NINE_GATES]) {
        fan_table[FULL_FLUSH] = 0;
        fan_table[CONCEALED_HAND] = 0;
        --fan_table[PUNG_OF_TERMINALS_OR_HONORS];
        fan_table[ONE_VOIDED_SUIT] = 0;
        fan_table[NO_HONORS] = 0;
        if (fan_table[FULLY_CONCEALED_HAND]) {
            fan_table[FULLY_CONCEALED_HAND] = 0;
            fan_table[SELF_DRAWN] = 1;
        }
    }
    // 四杠不计单钓将
    if (fan_table[FOUR_KONGS]) {
        fan_table[SINGLE_WAIT] = 0;
    }
    // 连七对不计七对、清一色、门前清、缺一门、无字
    if (fan_table[SEVEN_SHIFTED_PAIRS]) {
        fan_table[SEVEN_PAIRS] = 0;
        fan_table[FULL_FLUSH] = 0;
        fan_table[CONCEALED_HAND] = 0;
        fan_table[ONE_VOIDED_SUIT] = 0;
        fan_table[NO_HONORS] = 0;
    }
    // 十三幺不计五门齐、门前清、单钓将
    if (fan_table[THIRTEEN_ORPHANS]) {
        fan_table[ALL_TYPES] = 0;
        fan_table[CONCEALED_HAND] = 0;
        fan_table[SINGLE_WAIT] = 0;
    }

    // 清幺九不计混幺九、碰碰胡、全带幺、幺九刻、无字（严格98规则不计双同刻、不计三同刻）
    if (fan_table[ALL_TERMINALS]) {
        fan_table[ALL_TERMINALS_AND_HONORS] = 0;
        fan_table[ALL_PUNGS] = 0;
        fan_table[OUTSIDE_HAND] = 0;
        fan_table[PUNG_OF_TERMINALS_OR_HONORS] = 0;
        fan_table[NO_HONORS] = 0;
        fan_table[DOUBLE_PUNG] = 0;  // 通行计法不计双同刻
#ifdef STRICT_98_RULE
        fan_table[TRIPLE_PUNG] = 0;
        fan_table[DOUBLE_PUNG] = 0;
#endif
    }

    // 小四喜不计三风刻
    if (fan_table[LITTLE_FOUR_WINDS]) {
        fan_table[BIG_THREE_WINDS] = 0;
        // 小四喜的第四组牌如果是19的刻子，则是混幺九；如果是箭刻则是字一色；这两种都是不计幺九刻的
        // 如果是顺子或者2-8的刻子，则不存在多余的幺九刻
        // 所以这里将幺九刻置为0
        fan_table[PUNG_OF_TERMINALS_OR_HONORS] = 0;
    }

    // 小三元不计双箭刻、箭刻（严格98规则不计缺一门）
    if (fan_table[LITTLE_THREE_DRAGONS]) {
        fan_table[TWO_DRAGONS_PUNGS] = 0;
        fan_table[DRAGON_PUNG] = 0;
#ifdef STRICT_98_RULE
        fan_table[ONE_VOIDED_SUIT] = 0;
#endif
    }

    // 字一色不计混幺九、碰碰胡、全带幺、幺九刻、缺一门
    if (fan_table[ALL_HONORS]) {
        fan_table[ALL_TERMINALS_AND_HONORS] = 0;
        fan_table[ALL_PUNGS] = 0;
        fan_table[OUTSIDE_HAND] = 0;
        fan_table[PUNG_OF_TERMINALS_OR_HONORS] = 0;
        fan_table[ONE_VOIDED_SUIT] = 0;
    }
    // 四暗刻不计碰碰和、门前清，把不求人修正为自摸
    if (fan_table[FOUR_CONCEALED_PUNGS]) {
        fan_table[ALL_PUNGS] = 0;
        fan_table[CONCEALED_HAND] = 0;
        if (fan_table[FULLY_CONCEALED_HAND]) {
            fan_table[FULLY_CONCEALED_HAND] = 0;
            fan_table[SELF_DRAWN] = 1;
        }
    }
    // 一色双龙会不计七对、清一色、平和、一般高、老少副、缺一门、无字
    if (fan_table[PURE_TERMINAL_CHOWS]) {
        fan_table[SEVEN_PAIRS] = 0;
        fan_table[FULL_FLUSH] = 0;
        fan_table[ALL_CHOWS] = 0;
        fan_table[PURE_DOUBLE_CHOW] = 0;
        fan_table[TWO_TERMINAL_CHOWS] = 0;
        fan_table[ONE_VOIDED_SUIT] = 0;
        fan_table[NO_HONORS] = 0;
    }

    // 一色四同顺不计一色三同顺、一般高、四归一（严格98规则不计缺一门）
    if (fan_table[QUADRUPLE_CHOW]) {
        fan_table[PURE_SHIFTED_PUNGS] = 0;
        fan_table[TILE_HOG] = 0;
        fan_table[PURE_DOUBLE_CHOW] = 0;
#ifdef STRICT_98_RULE
        fan_table[ONE_VOIDED_SUIT] = 0;
#endif
    }
    // 一色四节高不计一色三节高、碰碰和（严格98规则不计缺一门）
    if (fan_table[FOUR_PURE_SHIFTED_PUNGS]) {
        fan_table[PURE_TRIPLE_CHOW] = 0;
        fan_table[ALL_PUNGS] = 0;
#ifdef STRICT_98_RULE
        fan_table[ONE_VOIDED_SUIT] = 0;
#endif
    }

    // 一色四步高不计一色三步高、老少副、连六（严格98规则不计缺一门）
    if (fan_table[FOUR_PURE_SHIFTED_CHOWS]) {
        fan_table[PURE_SHIFTED_CHOWS] = 0;
        fan_table[TWO_TERMINAL_CHOWS] = 0;
        fan_table[SHORT_STRAIGHT] = 0;
#ifdef STRICT_98_RULE
        fan_table[ONE_VOIDED_SUIT] = 0;
#endif
    }

    // 混幺九不计碰碰和、全带幺、幺九刻
    if (fan_table[ALL_TERMINALS_AND_HONORS]) {
        fan_table[ALL_PUNGS] = 0;
        fan_table[OUTSIDE_HAND] = 0;
        fan_table[PUNG_OF_TERMINALS_OR_HONORS] = 0;
    }

    // 七对不计门前清、单钓将
    if (fan_table[SEVEN_PAIRS]) {
        fan_table[CONCEALED_HAND] = 0;
        fan_table[SINGLE_WAIT] = 0;
    }
    // 七星不靠不计五门齐、门前清
    if (fan_table[GREATER_HONORS_AND_KNITTED_TILES]) {
        fan_table[ALL_TYPES] = 0;
        fan_table[CONCEALED_HAND] = 0;
    }
    // 全双刻不计碰碰胡、断幺、无字
    if (fan_table[ALL_EVEN_PUNGS]) {
        fan_table[ALL_PUNGS] = 0;
        fan_table[ALL_SIMPLES] = 0;
        fan_table[NO_HONORS] = 0;
    }
    // 清一色不计缺一门、无字
    if (fan_table[FULL_FLUSH]) {
        fan_table[ONE_VOIDED_SUIT] = 0;
        fan_table[NO_HONORS] = 0;
    }
    // 一色三同顺不计一色三节高、一般高
    if (fan_table[PURE_TRIPLE_CHOW]) {
        fan_table[PURE_SHIFTED_PUNGS] = 0;
        fan_table[PURE_DOUBLE_CHOW] = 0;
    }
    // 一色三节高不计一色三同顺
    if (fan_table[PURE_SHIFTED_PUNGS]) {
        fan_table[PURE_TRIPLE_CHOW] = 0;
    }
    // 全大不计大于五、无字
    if (fan_table[UPPER_TILES]) {
        fan_table[UPPER_FOUR] = 0;
        fan_table[NO_HONORS] = 0;
    }
    // 全中不计断幺
    if (fan_table[MIDDLE_TILES]) {
        fan_table[ALL_SIMPLES] = 0;
        fan_table[NO_HONORS] = 0;
    }
    // 全小不计小于五、无字
    if (fan_table[LOWER_TILES]) {
        fan_table[LOWER_FOUR] = 0;
        fan_table[NO_HONORS] = 0;
    }

    // 三色双龙会不计平和、无字、喜相逢、老少副
    if (fan_table[THREE_SUITED_TERMINAL_CHOWS]) {
        fan_table[ALL_CHOWS] = 0;
        fan_table[NO_HONORS] = 0;
        fan_table[MIXED_DOUBLE_CHOW] = 0;
        fan_table[TWO_TERMINAL_CHOWS] = 0;
    }
    // 全带五不计断幺、无字
    if (fan_table[ALL_FIVE]) {
        fan_table[ALL_SIMPLES] = 0;
        fan_table[NO_HONORS] = 0;
    }

    // 七星不靠不计五门齐、门前清
    if (fan_table[LESSER_HONORS_AND_KNITTED_TILES]) {
        fan_table[ALL_TYPES] = 0;
        fan_table[CONCEALED_HAND] = 0;
    }
    // 大于五不计无字
    if (fan_table[UPPER_FOUR]) {
        fan_table[NO_HONORS] = 0;
    }
    // 小于五不计无字
    if (fan_table[LOWER_FOUR]) {
        fan_table[NO_HONORS] = 0;
    }
    // 三风刻内部不再计幺九刻（严格98规则不计缺一门）
    if (fan_table[BIG_THREE_WINDS]) {
        // 如果不是字一色或混幺九，则要减去3个幺九刻
        if (!fan_table[ALL_HONORS] && !fan_table[ALL_TERMINALS_AND_HONORS]) {
            assert(fan_table[PUNG_OF_TERMINALS_OR_HONORS] >= 3);
            fan_table[PUNG_OF_TERMINALS_OR_HONORS] -= 3;
        }
#ifdef STRICT_98_RULE
        fan_table[ONE_VOIDED_SUIT] = 0;
#endif
    }

    // 推不倒不计缺一门
    if (fan_table[REVERSIBLE_TILES]) {
        fan_table[ONE_VOIDED_SUIT] = 0;
    }
    // 妙手回春不计自摸
    if (fan_table[LAST_TILE_DRAW]) {
        fan_table[SELF_DRAWN] = 0;
    }
    // 杠上开花不计自摸
    if (fan_table[OUT_WITH_REPLACEMENT_TILE]) {
        fan_table[SELF_DRAWN] = 0;
    }
    // 抢杠和不计和绝张
    if (fan_table[ROBBING_THE_KONG]) {
        fan_table[LAST_TILE] = 0;
    }
    // 双暗杠不计暗杠
    if (fan_table[TWO_CONCEALED_KONGS]) {
        fan_table[CONCEALED_KONG] = 0;
    }

    // 混一色不计缺一门
    if (fan_table[HALF_FLUSH]) {
        fan_table[ONE_VOIDED_SUIT] = 0;
    }
    // 全求人不计单钓将
    if (fan_table[MELDED_HAND]) {
        fan_table[SINGLE_WAIT] = 0;
    }
    // 双箭刻不计箭刻
    if (fan_table[TWO_DRAGONS_PUNGS]) {
        fan_table[DRAGON_PUNG] = 0;
    }

    // 不求人不计自摸
    if (fan_table[FULLY_CONCEALED_HAND]) {
        fan_table[SELF_DRAWN] = 0;
    }
    // 双明杠不计明杠
    if (fan_table[TWO_MELDED_KONGS]) {
        fan_table[MELDED_KONG] = 0;
    }

    // 圈风刻自己不计幺九刻
    if (fan_table[PREVALENT_WIND]) {
        // 如果不是三风刻、小四喜、字一色、混幺九，则要减去1个幺九刻
        if (!fan_table[BIG_THREE_WINDS] && !fan_table[LITTLE_FOUR_WINDS]
            && !fan_table[ALL_HONORS] && !fan_table[ALL_TERMINALS_AND_HONORS]) {
            assert(fan_table[PUNG_OF_TERMINALS_OR_HONORS] > 0);
            --fan_table[PUNG_OF_TERMINALS_OR_HONORS];
        }
    }
    // 门风刻自己不计幺九刻
    if (fan_table[SEAT_WIND]) {
        // 如果圈风与门风不相同，并且不是三风刻、小四喜、字一色、混幺九，则要减去1个幺九刻
        if (!prevalent_eq_seat && !fan_table[BIG_THREE_WINDS] && !fan_table[LITTLE_FOUR_WINDS]
            && !fan_table[ALL_HONORS] && !fan_table[ALL_TERMINALS_AND_HONORS]) {
            assert(fan_table[PUNG_OF_TERMINALS_OR_HONORS] > 0);
            --fan_table[PUNG_OF_TERMINALS_OR_HONORS];
        }
    }
    // 平和不计无字
    if (fan_table[ALL_CHOWS]) {
        fan_table[NO_HONORS] = 0;
    }
    // 断幺不计无字
    if (fan_table[ALL_SIMPLES]) {
        fan_table[NO_HONORS] = 0;
    }
}

// 检测和绝张、妙手回春、海底捞月、自摸
static void check_win_flag(win_flag_t win_flag, fan_table_t &fan_table) {
    if (win_flag & WIN_FLAG_4TH_TILE) {
        fan_table[LAST_TILE] = 1;
    }
    if (win_flag & WIN_FLAG_WALL_LAST) {
        fan_table[win_flag & WIN_FLAG_SELF_DRAWN ? LAST_TILE_DRAW : LAST_TILE_CLAIM] = 1;
    }
    if (win_flag & WIN_FLAG_ABOUT_KONG) {
        fan_table[win_flag & WIN_FLAG_SELF_DRAWN ? OUT_WITH_REPLACEMENT_TILE : ROBBING_THE_KONG] = 1;
    }
    if (win_flag & WIN_FLAG_SELF_DRAWN) {
        fan_table[SELF_DRAWN] = 1;
    }
}

// 和牌是否为解释为明顺
static bool is_win_tile_in_concealed_chow_packs(const pack_t *chow_packs, long chow_cnt, tile_t win_tile) {
    return std::any_of(chow_packs, chow_packs + chow_cnt, [win_tile](pack_t chow_pack) {
        tile_t tile = pack_tile(chow_pack);
        return !is_pack_melded(chow_pack)
            && (tile - 1 == win_tile || tile == win_tile || tile + 1 == win_tile);
    });
}

// 基本和型算番
static void calculate_basic_type_fan(const pack_t (&packs)[5], long fixed_cnt, tile_t win_tile,
    win_flag_t win_flag, wind_t prevalent_wind, wind_t seat_wind, fan_table_t &fan_table) {
    pack_t pair_pack = 0;
    pack_t chow_packs[4];
    pack_t pung_packs[4];
    long chow_cnt = 0;
    long pung_cnt = 0;
    for (long i = 0; i < 5; ++i) {
        switch (pack_type(packs[i])) {
        case PACK_TYPE_CHOW: chow_packs[chow_cnt++] = packs[i]; break;
        case PACK_TYPE_PUNG:
        case PACK_TYPE_KONG: pung_packs[pung_cnt++] = packs[i]; break;
        case PACK_TYPE_PAIR: pair_pack = packs[i]; break;
        default: assert(0); return;
        }
    }

    if (pair_pack == 0 || chow_cnt + pung_cnt != 4) {
        return;
    }

    tile_t tiles[18];
    long tile_cnt = packs_to_tiles(packs, 5, tiles, 18);

    // 九莲宝灯
    if (fixed_cnt == 0 && tile_cnt == 14) {
        if (is_nine_gates((const tile_t (&)[14])tiles, win_tile)) {
            fan_table[NINE_GATES] = 1;
        }
    }

    check_win_flag(win_flag, fan_table);

    // 点和的牌张，如果不能解释为暗顺中的一张，那么将其解释为刻子，并标记这个刻子为明刻
    if ((win_flag & WIN_FLAG_SELF_DRAWN) == 0) {
        if (!is_win_tile_in_concealed_chow_packs(chow_packs, chow_cnt, win_tile)) {
            for (long i = 0; i < pung_cnt; ++i) {
                if (pack_tile(pung_packs[i]) == win_tile && !is_pack_melded(pung_packs[i])) {
                    pung_packs[i] |= (1 << 12);  // 标记为明副露
                }
            }
        }
    }

    switch (chow_cnt) {
    case 4:  // 4组顺子
        // 检测三色/一色双龙会
        if (is_three_suited_terminal_chows(chow_packs, pair_pack)) {
            fan_table[THREE_SUITED_TERMINAL_CHOWS] = 1;
            break;
        }
        if (is_pure_terminal_chows(chow_packs, pair_pack)) {
            fan_table[PURE_TERMINAL_CHOWS] = 1;
            break;
        }
        calculate_4_chows(chow_packs, fan_table);
        break;
    case 3:  // 3组顺子+1组刻子
        calculate_3_chows(chow_packs, fan_table);
        calculate_1_pung(pung_packs[0], fan_table);
        break;
    case 2:  // 2组顺子+2组刻子
        calculate_2_chows(chow_packs, fan_table);
        calculate_2_pungs(pung_packs, fan_table);
        break;
    case 1:  // 1组顺子+3组刻子
        calculate_3_pungs(pung_packs, fan_table);
        break;
    case 0:  // 4组刻子
        calculate_4_pungs(pung_packs, fan_table);
        break;
    default:
        break;
    }

    // 检测不求人、全求人
    check_melded_or_concealed_hand(packs, fixed_cnt, win_flag & WIN_FLAG_SELF_DRAWN, fan_table);
    // 检测雀头，确定平和、小三元、小四喜
    check_pair_tile(pack_tile(pair_pack), chow_cnt, fan_table);
    // 检测全带幺、全带五、全双刻
    check_tiles_rank_by_pack(packs, fan_table);

    // 检测门（五门齐的门）
    check_tiles_suits(tiles, tile_cnt, fan_table);
    // 检测特性（断幺、推不倒、绿一色、字一色、清幺九、混幺九）
    check_tiles_traits(tiles, tile_cnt, fan_table);
    // 检测数牌的范围（大于五、小于五、全大、全中、全小）
    check_tiles_rank_range(tiles, tile_cnt, fan_table);
    // 检测四归一
    check_tiles_hog(tiles, tile_cnt, fan_table);
    // 检测边嵌钓
    check_edge_closed_single_wait(packs + fixed_cnt, 5 - fixed_cnt, win_tile, fan_table);

    // 检测圈风刻、门风刻
    for (int i = 0; i < 5; ++i) {
        check_wind_pungs(packs[i], prevalent_wind, seat_wind, fan_table);
    }

    // 统一校正一些不计的
    correction_fan_table(fan_table, prevalent_wind == seat_wind);

    // 如果什么番都没有，则计为无番和
    if (std::all_of(std::begin(fan_table), std::end(fan_table), [](long p) { return p == 0; })) {
        fan_table[CHICKEN_HAND] = 1;
    }
}

// “组合龙+面子+雀头”和型算番
static bool calculate_knitted_straight_in_basic_type_fan(const hand_tiles_t *hand_tiles, tile_t win_tile,
    win_flag_t win_flag, wind_t prevalent_wind, wind_t seat_wind, fan_table_t &fan_table) {
    long fixed_cnt = hand_tiles->pack_count;
    if (fixed_cnt > 1) {
        return false;
    }

    const pack_t *fixed_packs = hand_tiles->fixed_packs;
    long standing_cnt = hand_tiles->tile_count;

    // 对立牌和和牌的种类进行打表
    tile_table_t cnt_table;
    map_tiles(hand_tiles->standing_tiles, standing_cnt, cnt_table);
    ++cnt_table[win_tile];

    // 匹配组合龙
    const tile_t (*matched_seq)[9] = std::find_if(&standard_knitted_straight[0], &standard_knitted_straight[6],
        [&cnt_table](const tile_t (&seq)[9]) {
        return std::all_of(std::begin(seq), std::end(seq), [&cnt_table](tile_t t) { return cnt_table[t] > 0; });
    });

    if (matched_seq == &standard_knitted_straight[6]) {
        return false;
    }

    // 剔除组合龙
    std::for_each(std::begin(*matched_seq), std::end(*matched_seq), [&cnt_table](tile_t t) { --cnt_table[t]; });

    // 按基本和型划分
    divisions_t result;
    result.count = 0;
    one_division_t work_division;
    memset(&work_division, 0, sizeof(work_division));

    // 此处逻辑为：将组合龙9张牌当作是已经完成的3组面子，占据了0 1 2下标处的3组
    // 如果第4组是副露的，将其放在下标3处
    // 然后按基本和型从从fixed_cnt + 3开始递归
    if (fixed_cnt == 1) {
        work_division.packs[3] = fixed_packs[0];
    }
    divide_recursively(cnt_table, fixed_cnt + 3, 0, &work_division, &result);
    if (result.count != 1) {
        return false;
    }

    pack_t *temp_pack = result.divisions[0].packs;

    fan_table[KNITTED_STRAIGHT] = 1;  // 组合龙
    if (pack_type(temp_pack[3]) == PACK_TYPE_CHOW) {
        if (is_numbered_suit_quick(pack_tile(temp_pack[4]))) {
            fan_table[ALL_CHOWS] = 1;  // 第4组是顺子，雀头是数牌时，为平和
        }
    }
    else {
        calculate_1_pung(temp_pack[3], fan_table);
    }

    check_win_flag(win_flag, fan_table);
    // 门前清（暗杠不影响）
    if (fixed_cnt == 0 || (pack_type(temp_pack[3]) == PACK_TYPE_KONG && !is_pack_melded(temp_pack[3]))) {
        if (win_flag & WIN_FLAG_SELF_DRAWN) {
            fan_table[FULLY_CONCEALED_HAND] = 1;
        }
        else {
            fan_table[CONCEALED_HAND] = 1;
        }
    }

    // 还原牌
    tile_t tiles[15];  // 第四组可能为杠，所以最多为15张
    memcpy(tiles, matched_seq, sizeof(*matched_seq));  // 组合龙的部分
    long tile_cnt = packs_to_tiles(&temp_pack[3], 2, tiles + 9, 6);  // 一组面子+一对雀头 最多6张牌
    tile_cnt += 9;

    // 检测门（五门齐的门）
    check_tiles_suits(tiles, tile_cnt, fan_table);
    // 特性和数牌的范围不用检测了，绝对不可能是有断幺、推不倒、绿一色、字一色、清幺九、混幺九
    // 检测四归一
    check_tiles_hog(tiles, tile_cnt, fan_table);

    // 和牌张是组合龙范围的牌，不计边嵌钓
    if (std::none_of(std::begin(*matched_seq), std::end(*matched_seq), [win_tile](tile_t t) { return t == win_tile; })) {
        if (fixed_cnt == 0) {
            check_edge_closed_single_wait(temp_pack + 3, 2, win_tile, fan_table);
        }
        else {
            fan_table[SINGLE_WAIT] = 1;
        }
    }

    // 检测圈风刻、门风刻
    check_wind_pungs(temp_pack[3], prevalent_wind, seat_wind, fan_table);
    // 统一校正一些不计的
    correction_fan_table(fan_table, prevalent_wind == seat_wind);
    return true;
}

// 十三幺
static bool is_thirteen_orphans(const tile_t (&tiles)[14]) {
    return std::all_of(std::begin(tiles), std::end(tiles), &is_terminal_or_honor)
        && std::includes(std::begin(tiles), std::end(tiles),
        std::begin(standard_thirteen_orphans), std::end(standard_thirteen_orphans));
}

// 全不靠/七星不靠算番
bool caculate_honors_and_knitted_tiles(const tile_t (&standing_tiles)[14], fan_table_t &fan_table) {
    const tile_t *it = std::find_if(std::begin(standing_tiles), std::end(standing_tiles), &is_honor);
    long numbered_cnt = it - standing_tiles;
    // 数牌张数大于9或者小于7必然不可能是全不靠
    if (numbered_cnt > 9 || numbered_cnt < 7) {
        return false;
    }

    // 匹配组合龙
    if (std::none_of(&standard_knitted_straight[0], &standard_knitted_straight[6],
        [&standing_tiles, it](const tile_t (&seq)[9]) {
        return std::includes(std::begin(seq), std::end(seq), std::begin(standing_tiles), it);
    })) {
        return false;
    }

    static const tile_t seven_honors[] = { TILE_E, TILE_S, TILE_W, TILE_N, TILE_C, TILE_F, TILE_P };
    if (numbered_cnt == 7 && std::equal(std::begin(seven_honors), std::end(seven_honors), standing_tiles + 7)) {
        // 七种字牌齐，为七星不靠
        fan_table[GREATER_HONORS_AND_KNITTED_TILES] = 1;
        return true;
    }
    else if (std::includes(std::begin(seven_honors), std::end(seven_honors), it, std::end(standing_tiles))) {
        // 全不靠
        fan_table[LESSER_HONORS_AND_KNITTED_TILES] = 1;
        if (numbered_cnt == 9) {  // 有9张数牌，为带组合龙的全不靠
            fan_table[KNITTED_STRAIGHT] = 1;
        }
        return true;
    }

    return false;
}

// 特殊和型算番
static bool calculate_special_type_fan(const tile_t (&standing_tiles)[14], win_flag_t win_flag, fan_table_t &fan_table) {
    // 七对
    if (standing_tiles[0] == standing_tiles[1]
        && standing_tiles[2] == standing_tiles[3]
        && standing_tiles[4] == standing_tiles[5]
        && standing_tiles[6] == standing_tiles[7]
        && standing_tiles[8] == standing_tiles[9]
        && standing_tiles[10] == standing_tiles[11]
        && standing_tiles[12] == standing_tiles[13]) {

        if (is_numbered_suit_quick(standing_tiles[0])
            && standing_tiles[0] + 1 == standing_tiles[2]
            && standing_tiles[2] + 1 == standing_tiles[4]
            && standing_tiles[4] + 1 == standing_tiles[6]
            && standing_tiles[6] + 1 == standing_tiles[8]
            && standing_tiles[8] + 1 == standing_tiles[10]
            && standing_tiles[10] + 1 == standing_tiles[12]) {
            // 连七对
            fan_table[SEVEN_SHIFTED_PAIRS] = 1;
            check_tiles_traits(standing_tiles, 14, fan_table);
        }
        else {
            // 普通七对
            fan_table[SEVEN_PAIRS] = 1;

            // 检测门（五门齐的门）
            check_tiles_suits(standing_tiles, 14, fan_table);
            // 检测特性（断幺、推不倒、绿一色、字一色、清幺九、混幺九）
            check_tiles_traits(standing_tiles, 14, fan_table);
            // 检测数牌的范围（大于五、小于五、全大、全中、全小）
            check_tiles_rank_range(standing_tiles, 14, fan_table);
            // 检测四归一
            check_tiles_hog(standing_tiles, 14, fan_table);
        }
    }
    // 十三幺
    else if (is_thirteen_orphans(standing_tiles)) {
        fan_table[THIRTEEN_ORPHANS] = 1;
    }
    // 全不靠/七星不靠
    else if (caculate_honors_and_knitted_tiles(standing_tiles, fan_table)) {
    }
    else {
        return false;
    }

    check_win_flag(win_flag, fan_table);
    // 圈风刻、门风刻没必要检测了，这些特殊和型都没有面子
    // 统一校正一些不计的
    correction_fan_table(fan_table, false);
    return true;
}

static int get_fan_by_table(const fan_table_t &fan_table) {
    int fan = 0;
    for (int i = 1; i < FAN_TABLE_SIZE; ++i) {
        if (fan_table[i] == 0) {
            continue;
        }
        fan += fan_value_table[i] * fan_table[i];
        if (fan_table[i] == 1) {
            LOG("%s %d\n", fan_name[i], fan_value_table[i]);
        }
        else {
            LOG("%s %d*%ld\n", fan_name[i], fan_value_table[i], fan_table[i]);
        }
    }
    return fan;
}

bool is_standing_tiles_contains_win_tile(const tile_t *standing_tiles, long standing_cnt, tile_t win_tile) {
    return std::any_of(standing_tiles, standing_tiles + standing_cnt,
        [win_tile](tile_t tile) { return tile == win_tile; });
}

size_t count_win_tile_in_fixed_packs(const pack_t *fixed_packs, long fixed_cnt, tile_t win_tile) {
    size_t cnt = 0;
    for (long i = 0; i < fixed_cnt; ++i) {
        pack_t pack = fixed_packs[i];
        tile_t tile = pack_tile(pack);
        switch (pack_type(pack)) {
        case PACK_TYPE_CHOW:
            if (win_tile == tile - 1 || win_tile == tile || win_tile == tile + 1) {
                ++cnt;
            }
            break;
        case PACK_TYPE_PUNG:
            if (win_tile == tile) {
                cnt += 3;
            }
            break;
        case PACK_TYPE_KONG:
            if (win_tile == tile) {
                cnt += 4;
            }
            break;
        default:
            break;
        }
    }
    return cnt;
}

bool is_fixed_packs_contains_kong(const pack_t *fixed_packs, long fixed_cnt) {
    return std::any_of(fixed_packs, fixed_packs + fixed_cnt,
        [](pack_t pack) { return pack_type(pack) == PACK_TYPE_KONG; });
}

int check_calculator_input(const hand_tiles_t *hand_tiles, tile_t win_tile) {
    // 打表
    tile_table_t cnt_table;
    if (!map_hand_tiles(hand_tiles, cnt_table)) {
        return ERROR_WRONG_TILES_COUNT;
    }
    if (win_tile != 0) {
        ++cnt_table[win_tile];
    }

    // 如果某张牌超过4
    if (std::any_of(std::begin(cnt_table), std::end(cnt_table), [](int cnt) { return cnt > 4; })) {
        return ERROR_TILE_COUNT_GREATER_THAN_4;
    }

    return 0;
}

int calculate_fan(const calculate_param_t *calculate_param, fan_table_t &fan_table) {
    const hand_tiles_t *hand_tiles = &calculate_param->hand_tiles;
    tile_t win_tile = calculate_param->win_tile;
    win_flag_t win_flag = calculate_param->win_flag;
    wind_t prevalent_wind = calculate_param->prevalent_wind;
    wind_t seat_wind = calculate_param->seat_wind;

    if (int ret = check_calculator_input(hand_tiles, win_tile)) {
        return ret;
    }

    long fixed_cnt = hand_tiles->pack_count;
    long standing_cnt = hand_tiles->tile_count;

    // 校正和牌标记
    // 如果立牌包含和牌，则必然不是和绝张
    const bool standing_tiles_contains_win_tile = is_standing_tiles_contains_win_tile(hand_tiles->standing_tiles, standing_cnt, win_tile);
    if (standing_tiles_contains_win_tile) {
        win_flag &= ~WIN_FLAG_4TH_TILE;
    }

    // 如果和牌在副露中出现3张，则必然为和绝张
    const size_t win_tile_in_fixed_packs = count_win_tile_in_fixed_packs(hand_tiles->fixed_packs, fixed_cnt, win_tile);
    if (3 == win_tile_in_fixed_packs) {
        win_flag |= WIN_FLAG_4TH_TILE;
    }

    // 附加杠标记
    if (win_flag & WIN_FLAG_ABOUT_KONG) {
        if (win_flag & WIN_FLAG_SELF_DRAWN) {  // 自摸
            // 如果手牌没有杠，则必然不是杠上开花
            if (!is_fixed_packs_contains_kong(hand_tiles->fixed_packs, fixed_cnt)) {
                win_flag &= ~WIN_FLAG_ABOUT_KONG;
            }
        }
        else {  // 点和
            // 如果和牌在手牌范围内出现过，则必然不是抢杠和
            if (win_tile_in_fixed_packs > 0 || standing_tiles_contains_win_tile) {
                win_flag &= ~WIN_FLAG_ABOUT_KONG;
            }
        }
    }

    tile_t standing_tiles[14];
    divisions_t result;

    // 合并得到14张牌
    memcpy(standing_tiles, hand_tiles->standing_tiles, standing_cnt * sizeof(tile_t));
    standing_tiles[standing_cnt] = win_tile;
    std::sort(standing_tiles, standing_tiles + standing_cnt + 1);

    divide_win_hand(standing_tiles, hand_tiles->fixed_packs, fixed_cnt, &result);

    for (long i = 0; i < result.count; ++i) {
        std::sort(&result.divisions[i].packs[fixed_cnt], &result.divisions[i].packs[4]);
    }

    fan_table_t fan_tables[MAX_DIVISION_CNT] = { { 0 } };
    int max_fan = 0;
    long max_idx = -1;

    if (fixed_cnt == 0) {  // 门清状态，有可能是基本和型组合龙
        if (calculate_knitted_straight_in_basic_type_fan(hand_tiles,
            win_tile, win_flag, prevalent_wind, seat_wind, fan_tables[result.count])) {
            int current_fan = get_fan_by_table(fan_tables[result.count]);
            if (current_fan > max_fan) {
                max_fan = current_fan;
                max_idx = result.count;
            }
            LOG("fan = %d\n\n", current_fan);
        }
        else if (calculate_special_type_fan(standing_tiles, win_flag, fan_tables[result.count])) {
            int current_fan = get_fan_by_table(fan_tables[result.count]);
            if (current_fan > max_fan) {
                max_fan = current_fan;
                max_idx = result.count;
            }
            LOG("fan = %d\n\n", current_fan);
            if (fan_tables[result.count][SEVEN_SHIFTED_PAIRS]) {
                result.count = 0;
            }
        }
    }
    else if (fixed_cnt == 1 && result.count == 0) {
        // 1副露状态，有可能是基本和型组合龙
        if (calculate_knitted_straight_in_basic_type_fan(hand_tiles, win_tile,
            win_flag, prevalent_wind, seat_wind, fan_tables[0])) {
            int current_fan = get_fan_by_table(fan_tables[0]);
            if (current_fan > max_fan) {
                max_fan = current_fan;
                max_idx = result.count;
            }
            LOG("fan = %d\n\n", current_fan);
        }
    }

    // 遍历各种划分方式，分别算番，找出最大的番的划分方式
    for (long i = 0; i < result.count; ++i) {
#if 0  // Debug
        char str[64];
        packs_to_string(result.divisions[i].packs, 5, str, sizeof(str));
        puts(str);
#endif
        calculate_basic_type_fan(result.divisions[i].packs, fixed_cnt, win_tile,
            win_flag, prevalent_wind, seat_wind, fan_tables[i]);
        int current_fan = get_fan_by_table(fan_tables[i]);
        if (current_fan > max_fan) {
            max_fan = current_fan;
            max_idx = i;
        }
        LOG("fan = %d\n\n", current_fan);
    }
    if (max_idx == -1) {
        return ERROR_NOT_WIN;
    }

    memcpy(fan_table, fan_tables[max_idx], sizeof(fan_table));

    // 加花牌
    max_fan += calculate_param->flower_count;
    fan_table[FLOWER_TILES] = calculate_param->flower_count;

    return max_fan;
}

}
