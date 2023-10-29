/****************************************************************************
 Copyright (c) 2016-2023 Jeff Wang <summer_insects@163.com>

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
#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <algorithm>
#include <iterator>
#include "standard_tiles.h"

/**
 * 算番流程概述：
 * 1. 判断特殊和型
 *   (1) 门清状态——七对、十三幺、组合龙、全不靠、九莲宝灯
 *   (2) 1副露状态——组合龙
 * 2. 按基本和型划分（留意七对）
 * 3. 对第2步中的划分结果进行算番，取最大值

 * 对划分后的结果算番流程：
 * 1. 计数顺子个数和刻子个数，对刻子自身算番（涉及：幺九刻、箭刻）
 * 2. 分情况对面子算番
 *   (1) 4顺——先判断三色/一色双龙会，没有再计算4顺的番
 *   (2) 3顺1刻——计算3顺的番
 *   (3) 2顺2刻——计算2顺的番，计算2刻的番
 *   (4) 1顺3刻——计算3刻的番
 * 3. 根据和牌方式调整——涉及：不求人、全求人
 * 4. 根据雀头调整——涉及：平和、小三元、小四喜
 * 5. 根据牌组特征调整——涉及：全带幺、全带五、全双刻
 * 6. 根据花色调整——涉及：无字、缺一门、混一色、清一色、五门齐
 * 7. 根据牌特性调整——涉及：断幺、推不倒、绿一色、字一色、清幺九、混幺九
 * 8. 根据数牌的范围调整——涉及：大于五、小于五、全大、全中、全小
 * 9. 计算四归一
 * 10. 根据听牌方式调整——涉及：边张、嵌张、单钓将
 * 11. 统一调整规则中规定不计的
 * 12. 最后调整圈风门风
 * 13. 以上流程走完，得到算番结果。如果为0番，则调整为无番和
 */

#define MAX_DIVISION_CNT 20  // 一副牌最多也没有20种划分吧，够用了

#if 0
#define LOG(fmt_, ...) printf(fmt_, ##__VA_ARGS__)
#else
#define LOG(...) ((void)0)
#endif

//#define STRICT_98_RULE

namespace mahjong {

#if 0  // Debug
extern intptr_t packs_to_string(const pack_t *packs, intptr_t pack_cnt, char *str, intptr_t max_size);
#endif

// 对牌进行打表
static void map_tiles(const tile_t *tiles, intptr_t cnt, tile_table_t &tile_table) {
    for (intptr_t i = 0; i < cnt; ++i) {
        ++tile_table[tiles[i]];
    }
}

// 对牌组进行打表
static void map_packs(const pack_t *packs, intptr_t cnt, tile_table_t &tile_table) {
    for (intptr_t i = 0; i < cnt; ++i) {
        tile_t tile = pack_get_tile(packs[i]);
        switch (pack_get_type(packs[i])) {
        case PACK_TYPE_CHOW:
            ++tile_table[tile - 1];
            ++tile_table[tile];
            ++tile_table[tile + 1];
            break;
        case PACK_TYPE_PUNG:
            tile_table[tile] += 3;
            break;
        case PACK_TYPE_KONG:
            tile_table[tile] += 4;
            break;
        case PACK_TYPE_PAIR:
            tile_table[tile] += 2;
            break;
        default:
            UNREACHABLE();
            break;
        }
    }
}

// 获取本手的唯一牌（用来快速检测花色、范围等相关番种）
static intptr_t table_unique(const tile_table_t &fixed_table, const tile_table_t &standing_table, tile_t *tile) {
    tile_t *p = tile;
    for (int i = 0; i < 34; ++i) {
        tile_t t = all_tiles[i];
        if (fixed_table[t] || standing_table[t]) {
            *p++ = t;
        }
    }
    return p - tile;
}

//-------------------------------- 划分 --------------------------------

namespace {

    // 划分
    struct division_t {
        pack_t packs[5];  // 牌组。4面子1雀头，共5组
    };

    // 划分结果
    struct division_result_t {
        division_t divisions[MAX_DIVISION_CNT];  // 每一种划分
        intptr_t count;  // 划分方式总数
    };
}

// 递归划分算法的最后一步，添加划分
static void divide_tail_add_division(intptr_t fixed_cnt, const division_t *work_division, division_result_t *result) {
    // 拷贝一份当前的划分出来的面子，并排序暗手的面子
    // 这里不能直接在work_division->packs上排序，否则会破坏递归外层的数据
    division_t temp;
    memcpy(&temp, work_division, sizeof(temp));
    std::sort(temp.packs + fixed_cnt, temp.packs + 4);

    // 如果这种划分是否已经存在了
    if (std::none_of(&result->divisions[0], &result->divisions[result->count],
        [&temp, fixed_cnt](const division_t &od) {
        return std::equal(&od.packs[fixed_cnt], &od.packs[4], &temp.packs[fixed_cnt]);
    })) {
        // 写入划分结果里
        memcpy(&result->divisions[result->count], &temp, sizeof(temp));
        ++result->count;
    }
    else {
        LOG("same case");
    }
}

// 递归划分的最后一步
static bool divide_tail(tile_table_t &tile_table, intptr_t fixed_cnt, division_t *work_division, division_result_t *result) {
    for (int i = 0; i < 34; ++i) {
        tile_t t = all_tiles[i];
        if (tile_table[t] < 2) {
            continue;
        }

        tile_table[t] -= 2;  // 削减
        // 所有牌全部使用完毕
        if (std::all_of(std::begin(tile_table), std::end(tile_table), [](int n) { return n == 0; })) {
            tile_table[t] += 2;  // 还原

            // 这2张作为雀头
            work_division->packs[4] = make_pack(0, PACK_TYPE_PAIR, t);
            divide_tail_add_division(fixed_cnt, work_division, result);  // 记录
            return true;
        }
        tile_table[t] += 2;  // 还原
    }

    return false;
}

// 递归划分
static bool divide_recursively(tile_table_t &tile_table, intptr_t fixed_cnt, intptr_t step, eigen_t prev_eigen, division_t *work_division, division_result_t *result) {
    const intptr_t idx = step + fixed_cnt;
    if (idx == 4) {  // 4组面子都有了
        return divide_tail(tile_table, fixed_cnt, work_division, result);
    }

    bool ret = false;

    // 按牌表张遍历牌
    for (int i = 0; i < 34; ++i) {
        tile_t t = all_tiles[i];
        if (tile_table[t] < 1) {
            continue;
        }

        // 刻子
        if (tile_table[t] > 2) {
            // 如果当前刻子特征值小于上一组，说明这条路径已经来过了
            eigen_t eigen = make_eigen(t, t, t);
            if (eigen > prev_eigen) {
                work_division->packs[idx] = make_pack(0, PACK_TYPE_PUNG, t);  // 记录刻子
                // 削减这组刻子，递归
                tile_table[t] -= 3;
                if (divide_recursively(tile_table, fixed_cnt, step + 1, eigen, work_division, result)) {
                    ret = true;
                }
                // 还原
                tile_table[t] += 3;
            }
        }

        // 顺子（只能是数牌）
        if (is_numbered_suit(t)) {
            if (tile_get_rank(t) < 8 && tile_table[t + 1] && tile_table[t + 2]) {
                // 如果当前顺子特征值小于上一组，说明这条路径已经来过了
                eigen_t eigen = make_eigen(t, t + 1, t + 2);
                if (eigen >= prev_eigen) {
                    work_division->packs[idx] = make_pack(0, PACK_TYPE_CHOW, static_cast<tile_t>(t + 1));  // 记录顺子
                    // 削减这组顺子，递归
                    --tile_table[t];
                    --tile_table[t + 1];
                    --tile_table[t + 2];
                    if (divide_recursively(tile_table, fixed_cnt, step + 1, eigen, work_division, result)) {
                        ret = true;
                    }
                    // 还原
                    ++tile_table[t];
                    ++tile_table[t + 1];
                    ++tile_table[t + 2];
                }
            }
        }
    }

    return ret;
}

static bool has_pair(const tile_table_t &tile_table) {
    for (int i = 0; i < 34; ++i) {
        if (tile_table[all_tiles[i]] > 1) {
            return true;
        }
    }
    return false;
}

// 划分一手牌
static bool divide_win_hand(tile_table_t &standing_table, const pack_t *fixed_packs, intptr_t fixed_cnt, division_result_t *result) {
    result->count = 0;

    if (!has_pair(standing_table)) {
        return false;
    }

    // 复制副露的面子
    division_t work_division;
    memcpy(work_division.packs, fixed_packs, fixed_cnt * sizeof(pack_t));
    return divide_recursively(standing_table, fixed_cnt, 0, 0, &work_division, result);
}

//-------------------------------- 算番 --------------------------------

// 4组递增1
static FORCE_INLINE bool is_four_shifted_1(rank_t r0, rank_t r1, rank_t r2, rank_t r3) {
    return (r0 + 1 == r1 && r1 + 1 == r2 && r2 + 1 == r3);
}

// 4组递增2
static FORCE_INLINE bool is_four_shifted_2(rank_t r0, rank_t r1, rank_t r2, rank_t r3) {
    return (r0 + 2 == r1 && r1 + 2 == r2 && r2 + 2 == r3);
}

// 3组递增1
static FORCE_INLINE bool is_shifted_1(rank_t r0, rank_t r1, rank_t r2) {
    return (r0 + 1 == r1 && r1 + 1 == r2);
}

// 3组递增2
static FORCE_INLINE bool is_shifted_2(rank_t r0, rank_t r1, rank_t r2) {
    return (r0 + 2 == r1 && r1 + 2 == r2);
}

// 三色
static FORCE_INLINE bool is_mixed(suit_t s0, suit_t s1, suit_t s2) {
    return (s0 != s1 && s0 != s2 && s1 != s2);
}

// 3组递增1无序
static FORCE_INLINE bool is_shifted_1_unordered(rank_t r0, rank_t r1, rank_t r2) {
    return is_shifted_1(r1, r0, r2) || is_shifted_1(r2, r0, r1) || is_shifted_1(r0, r1, r2)
        || is_shifted_1(r2, r1, r0) || is_shifted_1(r0, r2, r1) || is_shifted_1(r1, r2, r0);
}

// 4组顺子的番
static fan_t get_4_chows_fan(tile_t t0, tile_t t1, tile_t t2, tile_t t3) {
    // 按出现频率顺序

    // 一色四步高
    if (is_four_shifted_2(t0, t1, t2, t3)) {
#if DISTINGUISH_PURE_SHIFTED_CHOWS
        return FOUR_PURE_SHIFTED_CHOWS_2;
#else
        return FOUR_PURE_SHIFTED_CHOWS;
#endif
    }
    if (is_four_shifted_1(t0, t1, t2, t3)) {
#if DISTINGUISH_PURE_SHIFTED_CHOWS
        return FOUR_PURE_SHIFTED_CHOWS_1;
#else
        return FOUR_PURE_SHIFTED_CHOWS;
#endif
    }
    // 一色四同顺
    if (t0 == t1 && t0 == t2 && t0 == t3) {
        return QUADRUPLE_CHOW;
    }
    // 以上都没有
    return FAN_NONE;
}

// 3组顺子的番
static fan_t get_3_chows_fan(tile_t t0, tile_t t1, tile_t t2) {
    suit_t s0 = tile_get_suit(t0);
    suit_t s1 = tile_get_suit(t1);
    suit_t s2 = tile_get_suit(t2);

    rank_t r0 = tile_get_rank(t0);
    rank_t r1 = tile_get_rank(t1);
    rank_t r2 = tile_get_rank(t2);

    // 按出现频率顺序

    if (is_mixed(s0, s1, s2)) {  // 三色
        // 三色三步高
        if (is_shifted_1_unordered(r1, r0, r2)) {
            return MIXED_SHIFTED_CHOWS;
        }
        // 三色三同顺
        if (r0 == r1 && r1 == r2) {
            return MIXED_TRIPLE_CHOW;
        }
        // 花龙
        if ((r0 == 2 && r1 == 5 && r2 == 8) || (r0 == 2 && r1 == 8 && r2 == 5)
            || (r0 == 5 && r1 == 2 && r2 == 8) || (r0 == 5 && r1 == 8 && r2 == 2)
            || (r0 == 8 && r1 == 2 && r2 == 5) || (r0 == 8 && r1 == 5 && r2 == 2)) {
            return MIXED_STRAIGHT;
        }
    }
    else {  // 一色
        // 清龙
        if (t0 + 3 == t1 && t1 + 3 == t2) {
            return PURE_STRAIGHT;
        }
        // 一色三步高
        if (is_shifted_2(t0, t1, t2)) {
#if DISTINGUISH_PURE_SHIFTED_CHOWS
            return PURE_SHIFTED_CHOWS_2;
#else
            return PURE_SHIFTED_CHOWS;
#endif
        }
        if (is_shifted_1(t0, t1, t2)) {
#if DISTINGUISH_PURE_SHIFTED_CHOWS
            return PURE_SHIFTED_CHOWS_1;
#else
            return PURE_SHIFTED_CHOWS;
#endif
        }
        // 一色三同顺
        if (t0 == t1 && t0 == t2) {
            return PURE_TRIPLE_CHOW;
        }
    }
    // 以上都没有
    return FAN_NONE;
}

// 2组顺子的番
static fan_t get_2_chows_fan_unordered(tile_t t0, tile_t t1) {
    // 按出现频率顺序

    if (!is_suit_equal_quick(t0, t1)) {  // 两色
        // 喜相逢
        if (is_rank_equal_quick(t0, t1)) {
            return MIXED_DOUBLE_CHOW;
        }
    }
    else {  // 一色
        // 连六
        if (t0 + 3 == t1 || t1 + 3 == t0) {
            return SHORT_STRAIGHT;
        }
        // 老少副
        rank_t r0 = tile_get_rank(t0);
        rank_t r1 = tile_get_rank(t1);
        if ((r0 == 2 && r1 == 8) || (r0 == 8 && r1 == 2)) {
            return TWO_TERMINAL_CHOWS;
        }
        // 一般高
        if (t0 == t1) {
            return PURE_DOUBLE_CHOW;
        }
    }
    // 以上都没有
    return FAN_NONE;
}

// 4组刻子的番
static fan_t get_4_pungs_fan(tile_t t0, tile_t t1, tile_t t2, tile_t t3) {
    // 一色四节高
    if (is_numbered_suit_quick(t0) && t0 + 1 == t1 && t1 + 1 == t2 && t2 + 1 == t3) {
        return FOUR_PURE_SHIFTED_PUNGS;
    }
    // 大四喜
    if (t0 == TILE_E && t1 == TILE_S && t2 == TILE_W && t3 == TILE_N) {
        return BIG_FOUR_WINDS;
    }
    // 以上都没有
    return FAN_NONE;
}

// 3组刻子的番
static fan_t get_3_pungs_fan(tile_t t0, tile_t t1, tile_t t2) {
    // 按出现频率顺序

    if (is_numbered_suit_quick(t0) && is_numbered_suit_quick(t1) && is_numbered_suit_quick(t2)) {  // 数牌
        suit_t s0 = tile_get_suit(t0);
        suit_t s1 = tile_get_suit(t1);
        suit_t s2 = tile_get_suit(t2);

        rank_t r0 = tile_get_rank(t0);
        rank_t r1 = tile_get_rank(t1);
        rank_t r2 = tile_get_rank(t2);

        if (is_mixed(s0, s1, s2)) {  // 三色
            // 三色三节高
            if (is_shifted_1_unordered(r1, r0, r2)) {
                return MIXED_SHIFTED_PUNGS;
            }
            // 三同刻
            if (r0 == r1 && r1 == r2) {
                return TRIPLE_PUNG;
            }
        }
        else {
            // 一色三节高
            if (t0 + 1 == t1 && t1 + 1 == t2) {
                return PURE_SHIFTED_PUNGS;
            }
        }
    }
    else {
        // 三风刻
        if ((t0 == TILE_E && t1 == TILE_S && t2 == TILE_W)
            || (t0 == TILE_E && t1 == TILE_S && t2 == TILE_N)
            || (t0 == TILE_E && t1 == TILE_W && t2 == TILE_N)
            || (t0 == TILE_S && t1 == TILE_W && t2 == TILE_N)) {
            return BIG_THREE_WINDS;
        }
        // 大三元
        if (t0 == TILE_C && t1 == TILE_F && t2 == TILE_P) {
            return BIG_THREE_DRAGONS;
        }
    }

    // 以上都没有
    return FAN_NONE;
}

// 2组刻子的番
static fan_t get_2_pungs_fan_unordered(tile_t t0, tile_t t1) {
    // 按出现频率顺序
    if (is_numbered_suit_quick(t0) && is_numbered_suit_quick(t1)) {  // 数牌
        // 双同刻
        if (is_rank_equal_quick(t0, t1)) {
            return DOUBLE_PUNG;
        }
    }
    else {
        // 双箭刻
        if (is_dragons(t0) && is_dragons(t1)) {
            return TWO_DRAGONS_PUNGS;
        }
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
static fan_t get_1_chow_extra_fan(tile_t tile0, tile_t tile1, tile_t tile2, tile_t tile_extra) {
    fan_t fan0 = get_2_chows_fan_unordered(tile0, tile_extra);
    fan_t fan1 = get_2_chows_fan_unordered(tile1, tile_extra);
    fan_t fan2 = get_2_chows_fan_unordered(tile2, tile_extra);

    // 按以下顺序返回
    // 一般高
    if (fan0 == PURE_DOUBLE_CHOW || fan1 == PURE_DOUBLE_CHOW || fan2 == PURE_DOUBLE_CHOW) {
        return PURE_DOUBLE_CHOW;
    }
    // 喜相逢
    if (fan0 == MIXED_DOUBLE_CHOW || fan1 == MIXED_DOUBLE_CHOW || fan2 == MIXED_DOUBLE_CHOW) {
        return MIXED_DOUBLE_CHOW;
    }
    // 连六
    if (fan0 == SHORT_STRAIGHT || fan1 == SHORT_STRAIGHT || fan2 == SHORT_STRAIGHT) {
        return SHORT_STRAIGHT;
    }
    // 老少副
    if (fan0 == TWO_TERMINAL_CHOWS || fan1 == TWO_TERMINAL_CHOWS || fan2 == TWO_TERMINAL_CHOWS) {
        return TWO_TERMINAL_CHOWS;
    }

    return FAN_NONE;
}

// 套算一次原则：
// 如果有尚未组合过的一副牌，只可同已组合过的相应的一副牌套算一次
//
// 不得相同原则：
// 凡已经合过某一番种的牌，不能再同其他一副牌组成相同的番种计分
//
// 根据套算一次原则，234567s234567p，只能计为“喜相逢*2 连六*1”或者“喜相逢*1 连六*2”，而不是“喜相逢*2 连六*2”
// 根据以上两点，234s223344567p，只能计为：“一般高、喜相逢、连六”，而不是“喜相逢*2、连六”
//
// 直接按规则来写，差不多是图的算法，太过复杂
// 这里简便处理：先统计有多少番，当超过时规则允许的数目时，从重复的开始削减
static void exclusionary_rule(const fan_t *all_fans, long fan_cnt, long max_cnt, fan_table_t &fan_table) {
    // 统计有多少番
    uint16_t table[4] = { 0 };
    long cnt = 0;
    for (long i = 0; i < fan_cnt; ++i) {
        if (all_fans[i] != FAN_NONE) {
            ++cnt;
            ++table[all_fans[i] - PURE_DOUBLE_CHOW];
        }
    }

    // 当超过时，从重复的开始削减
    int limit_cnt = 1;
    // 第一轮先削减剩下1，第二轮削减剩下0
    while (cnt > max_cnt && limit_cnt >= 0) {
        int idx = 4;  // 从老少副开始削减
        while (cnt > max_cnt && idx-- > 0) {
            while (static_cast<int>(table[idx]) > limit_cnt && cnt > max_cnt) {
                --table[idx];
                --cnt;
            }
        }
        --limit_cnt;
    }

    fan_table[PURE_DOUBLE_CHOW] = table[0];
    fan_table[MIXED_DOUBLE_CHOW] = table[1];
    fan_table[SHORT_STRAIGHT] = table[2];
    fan_table[TWO_TERMINAL_CHOWS] = table[3];
}

// 4组顺子算番（3+1）结构
static bool calculate_3_of_4_chows(tile_t tile0, tile_t tile1, tile_t tile2, tile_t tile_extra, fan_table_t &fan_table) {
    fan_t fan;
    if ((fan = get_3_chows_fan(tile0, tile1, tile2)) != FAN_NONE) {
        fan_table[fan] = 1;
        if ((fan = get_1_chow_extra_fan(tile0, tile1, tile2, tile_extra)) != FAN_NONE) {
            fan_table[fan] = 1;
        }
        return true;
    }
    return false;
}

// 4组顺子算番
static void calculate_4_chows(const tile_t (&mid_tiles)[4], fan_table_t &fan_table) {
    fan_t fan;
    // 存在4组顺子的番种时，不再检测其他的了
    if ((fan = get_4_chows_fan(mid_tiles[0], mid_tiles[1], mid_tiles[2], mid_tiles[3])) != FAN_NONE) {
        fan_table[fan] = 1;
        return;
    }

    // 3组顺子判断
    if (calculate_3_of_4_chows(mid_tiles[0], mid_tiles[1], mid_tiles[2], mid_tiles[3], fan_table)  // 012构成3组顺子的番种
        || calculate_3_of_4_chows(mid_tiles[0], mid_tiles[1], mid_tiles[3], mid_tiles[2], fan_table)  // 013构成3组顺子的番种
        || calculate_3_of_4_chows(mid_tiles[0], mid_tiles[2], mid_tiles[3], mid_tiles[1], fan_table)  // 023构成3组顺子的番种
        || calculate_3_of_4_chows(mid_tiles[1], mid_tiles[2], mid_tiles[3], mid_tiles[0], fan_table)  // 123构成3组顺子的番种
        ) {
#if DISTINGUISH_PURE_SHIFTED_CHOWS
        // 一色三步高（宽）+连六，有可能是窄三步
        if (fan_table[PURE_SHIFTED_CHOWS_2] && fan_table[SHORT_STRAIGHT]) {
            if (is_shifted_1(mid_tiles[0], mid_tiles[1], mid_tiles[2])
                || is_shifted_1(mid_tiles[0], mid_tiles[1], mid_tiles[3])
                || is_shifted_1(mid_tiles[0], mid_tiles[2], mid_tiles[3])
                || is_shifted_1(mid_tiles[1], mid_tiles[2], mid_tiles[3])) {
                fan_table[PURE_SHIFTED_CHOWS_2] = 0;
                fan_table[PURE_SHIFTED_CHOWS_1] = 1;
            }
        }
#endif
        return;
    }

    // 不存在3组顺子的番种时，4组顺子最多3番
    fan_t all_fans[6] = {
        get_2_chows_fan_unordered(mid_tiles[0], mid_tiles[1]),
        get_2_chows_fan_unordered(mid_tiles[0], mid_tiles[2]),
        get_2_chows_fan_unordered(mid_tiles[0], mid_tiles[3]),
        get_2_chows_fan_unordered(mid_tiles[1], mid_tiles[2]),
        get_2_chows_fan_unordered(mid_tiles[1], mid_tiles[3]),
        get_2_chows_fan_unordered(mid_tiles[2], mid_tiles[3])
    };

    int max_cnt = 3;

    // 0与其他3组顺子无任何关系
    if (all_fans[0] == FAN_NONE && all_fans[1] == FAN_NONE && all_fans[2] == FAN_NONE) {
        --max_cnt;
    }

    // 1与其他3组顺子无任何关系
    if (all_fans[0] == FAN_NONE && all_fans[3] == FAN_NONE && all_fans[4] == FAN_NONE) {
        --max_cnt;
    }

    // 2与其他3组顺子无任何关系
    if (all_fans[1] == FAN_NONE && all_fans[3] == FAN_NONE && all_fans[5] == FAN_NONE) {
        --max_cnt;
    }

    // 3与其他3组顺子无任何关系
    if (all_fans[2] == FAN_NONE && all_fans[4] == FAN_NONE && all_fans[5] == FAN_NONE) {
        --max_cnt;
    }

    if (max_cnt > 0) {
        exclusionary_rule(all_fans, 6, max_cnt, fan_table);
    }
}

// 3组顺子算番
static void calculate_3_chows(const tile_t (&mid_tiles)[3], fan_table_t &fan_table) {
    fan_t fan;

    // 存在3组顺子的番种时，不再检测其他的
    if ((fan = get_3_chows_fan(mid_tiles[0], mid_tiles[1], mid_tiles[2])) != FAN_NONE) {
        fan_table[fan] = 1;
        return;
    }

    // 不存在上述番种时，3组顺子最多2番
    fan_t all_fans[3] = {
        get_2_chows_fan_unordered(mid_tiles[0], mid_tiles[1]),
        get_2_chows_fan_unordered(mid_tiles[0], mid_tiles[2]),
        get_2_chows_fan_unordered(mid_tiles[1], mid_tiles[2])
    };
    exclusionary_rule(all_fans, 3, 2, fan_table);
}

// 2组顺子算番
static void calculate_2_chows_unordered(const tile_t (&mid_tiles)[2], fan_table_t &fan_table) {
    fan_t fan;
    if ((fan = get_2_chows_fan_unordered(mid_tiles[0], mid_tiles[1])) != FAN_NONE) {
        ++fan_table[fan];
    }
}

// 杠与暗刻算番
static void calculate_kongs(int concealed_pung_cnt, int melded_kong_cnt, int concealed_kong_cnt, fan_table_t &fan_table) {
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
        case 2: fan_table[TWO_CONCEALED_PUNGS] = 1; break;
        case 3: fan_table[THREE_CONCEALED_PUNGS] = 1; break;
        case 4: fan_table[FOUR_CONCEALED_PUNGS] = 1; break;
        default: break;
        }
        break;
    case 1:  // 1个杠
        if (melded_kong_cnt == 1) {  // 明杠
            fan_table[MELDED_KONG] = 1;
            switch (concealed_pung_cnt) {  // 暗刻的个数
            case 2: fan_table[TWO_CONCEALED_PUNGS] = 1; break;
            case 3: fan_table[THREE_CONCEALED_PUNGS] = 1; break;
            default: break;
            }
        }
        else {  // 暗杠
            fan_table[CONCEALED_KONG] = 1;
            switch (concealed_pung_cnt) {  // 暗刻的个数
            case 1: fan_table[TWO_CONCEALED_PUNGS] = 1; break;
            case 2: fan_table[THREE_CONCEALED_PUNGS] = 1; break;
            case 3: fan_table[FOUR_CONCEALED_PUNGS] = 1; break;
            default: break;
            }
        }
        break;
    case 2:  // 2个杠
        switch (concealed_kong_cnt) {
        case 0:  // 双明杠
            fan_table[TWO_MELDED_KONGS] = 1;
            if (concealed_pung_cnt == 2) {  // 暗刻的个数
                fan_table[TWO_CONCEALED_PUNGS] = 1;
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
            case 1: fan_table[TWO_CONCEALED_PUNGS] = 1; break;
            case 2: fan_table[THREE_CONCEALED_PUNGS] = 1; break;
            default: break;
            }
            break;
        case 2:  // 双暗杠
            fan_table[TWO_CONCEALED_KONGS] = 1;
            switch (concealed_pung_cnt) {  // 暗刻的个数
            case 1: fan_table[THREE_CONCEALED_PUNGS] = 1; break;
            case 2: fan_table[FOUR_CONCEALED_PUNGS] = 1; break;
            default: break;
            }
            break;
        default:
            break;
        }
        break;
    case 3:  // 3个杠
        fan_table[THREE_KONGS] = 1;
        switch (concealed_kong_cnt) {  // 暗刻的个数
        case 0:  // 3明杠
            break;
        case 1:  // 1暗杠2明杠
            if (concealed_pung_cnt > 0) {
                fan_table[TWO_CONCEALED_PUNGS] = 1;
            }
            break;
        case 2:  // 2暗杠1明杠
            if (concealed_pung_cnt == 0) {
                fan_table[TWO_CONCEALED_PUNGS] = 1;
            }
            else {
                fan_table[THREE_CONCEALED_PUNGS] = 1;
            }
            break;
        case 3:  // 3暗杠
            if (concealed_pung_cnt == 0) {
                fan_table[THREE_CONCEALED_PUNGS] = 1;
            }
            else {
                fan_table[FOUR_CONCEALED_PUNGS] = 1;
            }
            break;
        default:
            break;
        }
        break;
    case 4:  // 4个杠
        fan_table[FOUR_KONGS] = 1;
        switch (concealed_kong_cnt) {
        case 2: fan_table[TWO_CONCEALED_PUNGS] = 1; break;
        case 3: fan_table[THREE_CONCEALED_PUNGS] = 1; break;
        case 4: fan_table[FOUR_CONCEALED_PUNGS] = 1; break;
        default: break;
        }
        break;
    default:
        UNREACHABLE();
        break;
    }
}

// 4组刻子算番
static void calculate_4_pungs(const tile_t (&mid_tiles)[4], fan_table_t &fan_table) {
    fan_t fan;
    // 存在4组刻子的番种时，不再检测其他的了
    if ((fan = get_4_pungs_fan(mid_tiles[0], mid_tiles[1], mid_tiles[2], mid_tiles[3])) != FAN_NONE) {
        fan_table[fan] = 1;
        return;
    }

    // 3组刻子判断
    bool has_3_pungs_fan = false;
    int free_pack_idx = -1;  // 未使用的1组刻子
    // 012构成3组刻子的番种
    if ((fan = get_3_pungs_fan(mid_tiles[0], mid_tiles[1], mid_tiles[2])) != FAN_NONE) {
        fan_table[fan] = 1;
        free_pack_idx = 3;
        has_3_pungs_fan = true;
    }
    // 013构成3组刻子的番种
    else if ((fan = get_3_pungs_fan(mid_tiles[0], mid_tiles[1], mid_tiles[3])) != FAN_NONE) {
        fan_table[fan] = 1;
        free_pack_idx = 2;
        has_3_pungs_fan = true;
    }
    // 023构成3组刻子的番种
    else if ((fan = get_3_pungs_fan(mid_tiles[0], mid_tiles[2], mid_tiles[3])) != FAN_NONE) {
        fan_table[fan] = 1;
        free_pack_idx = 1;
        has_3_pungs_fan = true;
    }
    // 123构成3组刻子的番种
    else if ((fan = get_3_pungs_fan(mid_tiles[1], mid_tiles[2], mid_tiles[3])) != FAN_NONE) {
        fan_table[fan] = 1;
        free_pack_idx = 0;
        has_3_pungs_fan = true;
    }

    // 存在3组刻子的番种时，余下的第4组刻子只能组合一次
    if (has_3_pungs_fan) {
        for (int i = 0; i < 4; ++i) {
            if (i == free_pack_idx) {
                continue;
            }
            // 依次与未使用的这组刻子测试番种
            if ((fan = get_2_pungs_fan_unordered(mid_tiles[i], mid_tiles[free_pack_idx])) != FAN_NONE) {
                ++fan_table[fan];
                break;
            }
        }
        return;
    }

    // 不存在3组刻子的番种时，两两计算番种
    if ((fan = get_2_pungs_fan_unordered(mid_tiles[0], mid_tiles[1])) != FAN_NONE) {
        ++fan_table[fan];
    }
    if ((fan = get_2_pungs_fan_unordered(mid_tiles[0], mid_tiles[2])) != FAN_NONE) {
        ++fan_table[fan];
    }
    if ((fan = get_2_pungs_fan_unordered(mid_tiles[0], mid_tiles[3])) != FAN_NONE) {
        ++fan_table[fan];
    }
    if ((fan = get_2_pungs_fan_unordered(mid_tiles[1], mid_tiles[2])) != FAN_NONE) {
        ++fan_table[fan];
    }
    if ((fan = get_2_pungs_fan_unordered(mid_tiles[1], mid_tiles[3])) != FAN_NONE) {
        ++fan_table[fan];
    }
    if ((fan = get_2_pungs_fan_unordered(mid_tiles[2], mid_tiles[3])) != FAN_NONE) {
        ++fan_table[fan];
    }
}

// 3组刻子算番
static void calculate_3_pungs(const tile_t (&mid_tiles)[3], fan_table_t &fan_table) {
    fan_t fan;

    // 存在3组刻子的番种（三节高 三同刻 三风刻 大三元）时，不再检测其他的
    if ((fan = get_3_pungs_fan(mid_tiles[0], mid_tiles[1], mid_tiles[2])) != FAN_NONE) {
        fan_table[fan] = 1;
        return;
    }

    // 不存在3组刻子的番种时，两两计算番种
    if ((fan = get_2_pungs_fan_unordered(mid_tiles[0], mid_tiles[1])) != FAN_NONE) {
        ++fan_table[fan];
    }
    if ((fan = get_2_pungs_fan_unordered(mid_tiles[0], mid_tiles[2])) != FAN_NONE) {
        ++fan_table[fan];
    }
    if ((fan = get_2_pungs_fan_unordered(mid_tiles[1], mid_tiles[2])) != FAN_NONE) {
        ++fan_table[fan];
    }
}

// 2组刻子算番
static void calculate_2_pungs_unordered(const tile_t (&mid_tiles)[2], fan_table_t &fan_table) {
    fan_t fan = get_2_pungs_fan_unordered(mid_tiles[0], mid_tiles[1]);
    if (fan != FAN_NONE) {
        ++fan_table[fan];
    }
}

// 一色双龙会
static bool is_pure_terminal_chows(const pack_t (&chow_packs)[4], pack_t pair_pack) {
    if (tile_get_rank(pack_get_tile(pair_pack)) != 5) {  // 5作雀头
        return false;
    }

    int cnt_123 = 0, cnt_789 = 0;
    suit_t pair_suit = tile_get_suit(pack_get_tile(pair_pack));
    for (int i = 0; i < 4; ++i) {
        suit_t suit = tile_get_suit(pack_get_tile(chow_packs[i]));
        if (suit != pair_suit) {  // 花色与雀头相同
            return false;
        }
        rank_t rank = tile_get_rank(pack_get_tile(chow_packs[i]));
        switch (rank) {
        case 2: ++cnt_123; break;
        case 8: ++cnt_789; break;
        default: return false;
        }
    }
    return (cnt_123 == 2 && cnt_789 == 2);  // 123和789各2组
}

// 三色双龙会
static bool is_three_suited_terminal_chows(const pack_t (&chow_packs)[4], pack_t pair_pack) {
    if (tile_get_rank(pack_get_tile(pair_pack)) != 5) {  // 5作雀头
        return false;
    }

    int suit_table_123[4] = { 0 };
    int suit_table_789[4] = { 0 };
    suit_t pair_suit = tile_get_suit(pack_get_tile(pair_pack));
    for (int i = 0; i < 4; ++i) {
        suit_t suit = tile_get_suit(pack_get_tile(chow_packs[i]));
        if (suit == pair_suit) {  // 花色与雀头不相同
            return false;
        }
        rank_t rank = tile_get_rank(pack_get_tile(chow_packs[i]));
        switch (rank) {
        case 2: ++suit_table_123[suit]; break;
        case 8: ++suit_table_789[suit]; break;
        default: return false;
        }
    }

    // 与雀头花色不同的两色123和789各一组
    switch (pair_suit) {
    case 1: return (suit_table_123[2] && suit_table_123[3] && suit_table_789[2] && suit_table_789[3]);
    case 2: return (suit_table_123[1] && suit_table_123[3] && suit_table_789[1] && suit_table_789[3]);
    case 3: return (suit_table_123[1] && suit_table_123[2] && suit_table_789[1] && suit_table_789[2]);
    default: return false;
    }
}

// 检查七对听牌
static void check_seven_pairs_waiting(const tile_table_t &standing_table, tile_table_t &waiting_table) {
    int pairs = 0;
    for (int i = 0; i < 34; ++i) {
        uint16_t cnt = standing_table[all_tiles[i]];
        // 成对的牌
        if (cnt == 2 || cnt == 3) ++pairs;
        else if (cnt == 4) pairs += 2;
    }

    if (pairs == 6) {
        for (int i = 0; i < 34; ++i) {
            tile_t t = all_tiles[i];
            uint16_t cnt = standing_table[t];
            if (cnt == 1 || cnt == 3) {
                waiting_table[t] = 1;  // 听此张
                break;
            }
        }
    }
}

// 获取搭子听牌
static void get_ragular_pack_waiting(const tile_table_t &tile_table, tile_table_t &waiting_table) {
    for (int i = 0; i < 34; ++i) {
        tile_t t = all_tiles[i];
        if (tile_table[t] < 1) {
            continue;
        }
        if (tile_table[t] > 1) {
            waiting_table[t] = 1;  // 对倒
            return;
        }
        if (is_numbered_suit_quick(t)) {  // 数牌搭子
            rank_t r = tile_get_rank(t);
            if (r > 1 && tile_table[t - 1]) {  // 两面或者边张
                if (r < 9) waiting_table[t + 1] = 1;
                if (r > 2) waiting_table[t - 2] = 1;
                return;
            }
            if (r > 2 && tile_table[t - 2]) {  // 嵌张
                waiting_table[t - 1] = 1;
                return;
            }
        }
    }
}

// 检查基本和型听牌
static void check_regular_waiting(tile_table_t &tile_table, intptr_t tile_cnt, eigen_t prev_eigen, tile_table_t &waiting_table) {
    // 1张牌必然听雀头
    if (tile_cnt == 1) {
        for (int i = 0; i < 34; ++i) {
            tile_t t = all_tiles[i];
            uint16_t cnt = tile_table[t];
            if (cnt == 1) {
                waiting_table[t] = 1;
                break;
            }
        }
        return;
    }

    // 4张牌可能是有雀头听搭子
    if (tile_cnt == 4) {
        // 削减雀头
        for (int i = 0; i < 34; ++i) {
            tile_t t = all_tiles[i];
            if (tile_table[t] < 2) {
                continue;
            }

            // 削减雀头，获取搭子的听牌
            tile_table[t] -= 2;
            get_ragular_pack_waiting(tile_table, waiting_table);
            // 还原
            tile_table[t] += 2;
        }
    }

    // 递归削减面子
    for (int i = 0; i < 34; ++i) {
        tile_t t = all_tiles[i];
        if (tile_table[t] < 1) {
            continue;
        }

        // 刻子
        if (tile_table[t] > 2) {
            // 如果当前刻子特征值小于上一组，说明这条路径已经来过了
            eigen_t eigen = make_eigen(t, t, t);
            if (eigen > prev_eigen) {
                // 削减这组刻子，递归
                tile_table[t] -= 3;
                check_regular_waiting(tile_table, tile_cnt - 3, eigen, waiting_table);
                // 还原
                tile_table[t] += 3;
            }
        }

        // 顺子（只能是数牌）
        if (is_numbered_suit(t)) {
            // 顺子t t+1 t+2，显然t不能是8点以上的数牌
            if (tile_get_rank(t) < 8 && tile_table[t + 1] && tile_table[t + 2]) {
                // 如果当前顺子特征值小于上一组，说明这条路径已经来过了
                eigen_t eigen = make_eigen(t, t + 1, t + 2);
                if (eigen >= prev_eigen) {
                    // 削减这组顺子，递归
                    --tile_table[t];
                    --tile_table[t + 1];
                    --tile_table[t + 2];
                    check_regular_waiting(tile_table, tile_cnt - 3, eigen, waiting_table);
                    // 还原
                    ++tile_table[t];
                    ++tile_table[t + 1];
                    ++tile_table[t + 2];
                }
            }
        }
    }
}

// 是否独听
static bool is_unique_waiting(tile_table_t &standing_table, intptr_t tile_cnt, tile_t win_tile) {
    tile_table_t waiting_table{};

    // 削减和牌张
    --standing_table[win_tile];

    // 13张的情况下，要判断七对
    if (tile_cnt == 13) {
        check_seven_pairs_waiting(standing_table, waiting_table);
    }

    // 基本和牌型
    check_regular_waiting(standing_table, tile_cnt, 0, waiting_table);

    // 恢复和牌张
    ++standing_table[win_tile];

    bool waiting = false;
    for (int i = 0; i < 34; ++i) {
        if (waiting_table[all_tiles[i]]) {
            if (waiting) {
                // 如果已经有听面了，说明不是独听
                return false;
            }
            waiting = true;
        }
    }
    return waiting;
}

// 根据和牌方式调整——涉及番种：不求人、全求人
static void adjust_by_self_drawn(const pack_t (&packs)[5], intptr_t fixed_cnt, bool self_drawn, fan_table_t &fan_table) {
    ptrdiff_t melded_cnt = std::count_if(&packs[0], &packs[fixed_cnt], &is_pack_melded);  // 明副露的组数

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

// 根据雀头调整——涉及番种：平和、小三元、小四喜
static void adjust_by_pair_tile(tile_t pair_tile, intptr_t chow_cnt, fan_table_t &fan_table) {
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

// 根据花色调整——涉及番种：无字、缺一门、混一色、清一色、五门齐
static void adjust_by_suits(const tile_t *tiles, intptr_t tile_cnt, fan_table_t &fan_table) {
    // 打表标记有哪些花色，用bit操作
    uint8_t suit_flag = 0;
    for (intptr_t i = 0; i < tile_cnt; ++i) {
        suit_flag |= (1 << tile_get_suit(tiles[i]));
    }

    // 1111 0001
    if (!(suit_flag & 0xF1U)) {
        fan_table[NO_HONORS] = 1;  // 无字
    }

    // 1110 0011
    if (!(suit_flag & 0xE3U)) {
        ++fan_table[ONE_VOIDED_SUIT];  // 缺一门（万）
    }
    // 1110 0101
    if (!(suit_flag & 0xE5U)) {
        ++fan_table[ONE_VOIDED_SUIT];  // 缺一门（条）
    }
    // 1110 1001
    if (!(suit_flag & 0xE9U)) {
        ++fan_table[ONE_VOIDED_SUIT];  // 缺一门（饼）
    }

    // 当缺2门时，根据有字和无字，修正为混一色和清一色
    if (fan_table[ONE_VOIDED_SUIT] == 2) {
        fan_table[ONE_VOIDED_SUIT] = 0;
        if (fan_table[NO_HONORS] == 0) {
            fan_table[HALF_FLUSH] = 1;
        }
        else {
            fan_table[FULL_FLUSH] = 1;
            fan_table[NO_HONORS] = 0;
        }
    }

    // 0001 1110
    if (suit_flag == 0x1EU) {  // 三门数牌和字牌都有
        if (std::any_of(tiles, tiles + tile_cnt, &is_winds)
            && std::any_of(tiles, tiles + tile_cnt, &is_dragons)) {
            fan_table[ALL_TYPES] = 1;  // 五门齐
        }
    }
}

// 根据数牌的范围调整——涉及番种：大于五、小于五、全大、全中、全小
static void adjust_by_rank_range(const tile_t *tiles, intptr_t tile_cnt, fan_table_t &fan_table) {
#ifdef STRICT_98_RULE
    if (fan_table[SEVEN_PAIRS]) {
        return;  // 严格98规则的七对不支持叠加这些
    }
#endif

    // 打表标记有哪些数
    uint16_t rank_flag = 0;
    for (intptr_t i = 0; i < tile_cnt; ++i) {
        if (!is_numbered_suit_quick(tiles[i])) {
            return;
        }
        rank_flag |= (1 << tile_get_rank(tiles[i]));
    }

    // 1111 1111 1110 0001
    // 检测是否只包含1234
    if (!(rank_flag & 0xFFE1)) {
        // 包含4为小于五，否则为全小
        fan_table[rank_flag & 0x0010 ? LOWER_FOUR : LOWER_TILES] = 1;
        return;
    }
    // 1111 1100 0011 1111
    // 检测是否只包含6789
    if (!(rank_flag & 0xFC3F)) {
        // 包含6为大于五，否则为全大
        fan_table[rank_flag & 0x0040 ? UPPER_FOUR : UPPER_TILES] = 1;
        return;
    }
    // 1111 1111 1000 1111
    // 检测是否只包含456
    if (!(rank_flag & 0xFF8F)) {
        // 全中
        fan_table[MIDDLE_TILES] = 1;
    }
}

// 根据牌组特征调整——涉及番种：全带幺、全带五、全双刻
static void adjust_by_packs_traits(const pack_t (&packs)[5], fan_table_t &fan_table) {
    // 统计包含数牌19、字牌、5、双数牌的组数
    int terminal_pack = 0;
    int honor_pack = 0;
    int five_pack = 0;
    int even_pack = 0;
    for (int i = 0; i < 5; ++i) {
        tile_t tile = pack_get_tile(packs[i]);
        if (is_numbered_suit_quick(tile)) {  // 数牌
            rank_t rank = tile_get_rank(tile);
            if (pack_get_type(packs[i]) == PACK_TYPE_CHOW) {  // 顺子
                switch (rank) {
                case 2: case 8: ++terminal_pack; break;  // 数牌19
                case 4: case 5: case 6: ++five_pack; break;  // 带五
                default: break;
                }
            }
            else {  // 刻子或雀头
                switch (rank) {
                case 1: case 9: ++terminal_pack; break;  // 数牌19
                case 5: ++five_pack; break;  // 带五
                case 2: case 4: case 6: case 8: ++even_pack; break;  // 双刻
                default: break;
                }
            }
        }
        else {
            ++honor_pack;  // 字牌
        }
    }

    // 5组牌都包含数牌19和字牌，先暂时计为全带幺
    if (terminal_pack + honor_pack == 5) {
        fan_table[OUTSIDE_HAND] = 1;
        return;
    }
    // 全带五
    if (five_pack == 5) {
        fan_table[ALL_FIVE] = 1;
        return;
    }
    // 全双刻
    if (even_pack == 5) {
        fan_table[ALL_EVEN_PUNGS] = 1;
    }
}

// 根据牌特性调整——涉及番种：断幺、推不倒、绿一色、字一色、清幺九、混幺九
static void adjust_by_tiles_traits(const tile_t *tiles, intptr_t tile_cnt, fan_table_t &fan_table) {
    // 断幺
    if (std::none_of(tiles, tiles + tile_cnt, &is_terminal_or_honor)) {
        fan_table[ALL_SIMPLES] = 1;
    }

    // 推不倒
    if (std::all_of(tiles, tiles + tile_cnt, &is_reversible)) {
        fan_table[REVERSIBLE_TILES] = 1;
    }

#ifdef STRICT_98_RULE
    if (fan_table[SEVEN_PAIRS]) {
        return;  // 严格98规则的七对不支持绿一色、字一色、清幺九、混幺九
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

// 四归一调整
static void adjust_by_tiles_hog(const tile_table_t &tile_table, intptr_t kong_cnt, fan_table_t &fan_table) {
    // 有多少种已经用去4张的牌减去杠的数量，即为四归一的数量
    intptr_t cnt = std::count(std::begin(tile_table), std::end(tile_table), 4);
    fan_table[TILE_HOG] = static_cast<uint8_t>(cnt - kong_cnt);
}

// 根据听牌方式调整——涉及番种：边张、嵌张、单钓将
static void adjust_by_waiting_form(const pack_t *concealed_packs, intptr_t pack_cnt, tile_t win_tile, fan_table_t &fan_table) {
    // 全求人和四杠不计单钓将，也不可能有边张、嵌张
    if (fan_table[MELDED_HAND] || fan_table[FOUR_KONGS]) {
        return;
    }

    // 听1张的情况，看和牌张处于什么位置
    // 边张0x01 嵌张0x02 单钓将0x04
    uint8_t pos_flag = 0;

    for (intptr_t i = 0; i < pack_cnt; ++i) {
        switch (pack_get_type(concealed_packs[i])) {
        case PACK_TYPE_CHOW: {
            tile_t mid_tile = pack_get_tile(concealed_packs[i]);
            if (mid_tile == win_tile) {
                pos_flag |= 0x02U;  // 嵌张
            }
            else if (mid_tile + 1 == win_tile || mid_tile - 1 == win_tile) {
                pos_flag |= 0x01U;  // 边张
            }
            break;
        }
        case PACK_TYPE_PAIR: {
            tile_t mid_tile = pack_get_tile(concealed_packs[i]);
            if (mid_tile == win_tile) {
                pos_flag |= 0x04U;  // 单钓将
            }
            break;
        }
        default:
            break;
        }
    }

    // 当多种可能存在时，只能计其中一种
    if (pos_flag & 0x01U) {
        fan_table[EDGE_WAIT] = 1;
    }
    else if (pos_flag & 0x02U) {
        fan_table[CLOSED_WAIT] = 1;
    }
    else if (pos_flag & 0x04U) {
        fan_table[SINGLE_WAIT] = 1;
    }
}

// 统一调整一些不计的
static void final_adjust(fan_table_t &fan_table) {
    // 大四喜不计三风刻、碰碰和、圈风刻、门风刻、幺九刻
    if (fan_table[BIG_FOUR_WINDS]) {
        fan_table[ALL_PUNGS] = 0;
        fan_table[PUNG_OF_TERMINALS_OR_HONORS] = 0;
    }
    // 大三元不计双箭刻、箭刻（严格98规则不计缺一门）
    if (fan_table[BIG_THREE_DRAGONS]) {
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
    // 四杠不计单钓将
    if (fan_table[FOUR_KONGS]) {
        fan_table[SINGLE_WAIT] = 0;
    }

    // 清幺九不计混幺九、碰碰胡、全带幺、幺九刻、无字（严格98规则不计双同刻、不计三同刻）
    if (fan_table[ALL_TERMINALS]) {
        fan_table[ALL_PUNGS] = 0;
        fan_table[OUTSIDE_HAND] = 0;
        fan_table[PUNG_OF_TERMINALS_OR_HONORS] = 0;
        fan_table[NO_HONORS] = 0;
        fan_table[DOUBLE_PUNG] = 0;  // 通行计法不计双同刻
#ifdef STRICT_98_RULE
        fan_table[TRIPLE_PUNG] = 0;
#endif
    }

    // 小四喜不计三风刻
    if (fan_table[LITTLE_FOUR_WINDS]) {
        // 小四喜的第四组牌如果是19的刻子，则是混幺九；如果是箭刻则是字一色；这两种都是不计幺九刻的
        // 如果是顺子或者2-8的刻子，则不存在多余的幺九刻
        // 所以这里将幺九刻置为0
        fan_table[PUNG_OF_TERMINALS_OR_HONORS] = 0;
    }

    // 小三元不计双箭刻、箭刻（严格98规则不计缺一门）
    if (fan_table[LITTLE_THREE_DRAGONS]) {
        fan_table[DRAGON_PUNG] = 0;
#ifdef STRICT_98_RULE
        fan_table[ONE_VOIDED_SUIT] = 0;
#endif
    }

    // 字一色不计混幺九、碰碰胡、全带幺、幺九刻、缺一门
    if (fan_table[ALL_HONORS]) {
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
        fan_table[FULL_FLUSH] = 0;
        fan_table[ALL_CHOWS] = 0;
        fan_table[NO_HONORS] = 0;
    }
#ifdef STRICT_98_RULE
    // 一色四同顺不计一色三同顺、一般高、四归一（严格98规则不计缺一门）
    if (fan_table[QUADRUPLE_CHOW]) {
        fan_table[ONE_VOIDED_SUIT] = 0;
    }
#endif
    // 一色四节高不计一色三节高、碰碰和（严格98规则不计缺一门）
    if (fan_table[FOUR_PURE_SHIFTED_PUNGS]) {
        fan_table[ALL_PUNGS] = 0;
#ifdef STRICT_98_RULE
        fan_table[ONE_VOIDED_SUIT] = 0;
#endif
    }

#ifdef STRICT_98_RULE
    // 一色四步高不计一色三步高、老少副、连六（严格98规则不计缺一门）
    if (fan_table[FOUR_PURE_SHIFTED_CHOWS]) {
        fan_table[ONE_VOIDED_SUIT] = 0;
    }
#endif

    // 混幺九不计碰碰和、全带幺、幺九刻
    if (fan_table[ALL_TERMINALS_AND_HONORS]) {
        fan_table[ALL_PUNGS] = 0;
        fan_table[OUTSIDE_HAND] = 0;
        fan_table[PUNG_OF_TERMINALS_OR_HONORS] = 0;
    }

    // 全双刻不计碰碰胡、断幺、无字
    if (fan_table[ALL_EVEN_PUNGS]) {
        fan_table[ALL_PUNGS] = 0;
        fan_table[ALL_SIMPLES] = 0;
        fan_table[NO_HONORS] = 0;
    }
    // 全大不计大于五、无字
    if (fan_table[UPPER_TILES]) {
        fan_table[NO_HONORS] = 0;
    }
    // 全中不计断幺
    if (fan_table[MIDDLE_TILES]) {
        fan_table[ALL_SIMPLES] = 0;
        fan_table[NO_HONORS] = 0;
    }
    // 全小不计小于五、无字
    if (fan_table[LOWER_TILES]) {
        fan_table[NO_HONORS] = 0;
    }

    // 三色双龙会不计平和、无字、喜相逢、老少副
    if (fan_table[THREE_SUITED_TERMINAL_CHOWS]) {
        fan_table[ALL_CHOWS] = 0;
        fan_table[NO_HONORS] = 0;
    }
    // 全带五不计断幺、无字
    if (fan_table[ALL_FIVE]) {
        fan_table[ALL_SIMPLES] = 0;
        fan_table[NO_HONORS] = 0;
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

    // 平和不计无字
    if (fan_table[ALL_CHOWS]) {
        fan_table[NO_HONORS] = 0;
    }
    // 断幺不计无字
    if (fan_table[ALL_SIMPLES]) {
        fan_table[NO_HONORS] = 0;
    }
}


// 调整圈风刻、门风刻
static void adjust_by_winds(tile_t tile, wind_t prevalent_wind, wind_t seat_wind, fan_table_t &fan_table) {
    // 三风刻、混幺九、字一色、小四喜，这些番种已经扣除过幺九刻了
    bool is_deducted = (fan_table[BIG_THREE_WINDS] || fan_table[ALL_TERMINALS_AND_HONORS] || fan_table[ALL_HONORS] || fan_table[LITTLE_FOUR_WINDS]);

    rank_t delta = tile - TILE_E;
    if (delta == static_cast<int>(prevalent_wind) - static_cast<int>(wind_t::EAST)) {
        fan_table[PREVALENT_WIND] = 1;
        if (!is_deducted) {
            --fan_table[PUNG_OF_TERMINALS_OR_HONORS];
        }
    }
    if (delta == static_cast<int>(seat_wind) - static_cast<int>(wind_t::EAST)) {
        fan_table[SEAT_WIND] = 1;
        // 当圈风门风相同时，已经扣除过幺九刻了
        if (seat_wind != prevalent_wind && !is_deducted) {
            --fan_table[PUNG_OF_TERMINALS_OR_HONORS];
        }
    }
}

// 根据和牌标记调整——涉及番种：和绝张、妙手回春、海底捞月、自摸
static void adjust_by_win_flag(win_flag_t win_flag, fan_table_t &fan_table) {
    if (win_flag & WIN_FLAG_LAST_TILE) {
        fan_table[LAST_TILE] = 1;
    }
    if (win_flag & WIN_FLAG_SELF_DRAWN) {
        fan_table[SELF_DRAWN] = 1;
        if (win_flag & WIN_FLAG_WALL_LAST) {
            fan_table[LAST_TILE_DRAW] = 1;
            fan_table[SELF_DRAWN] = 0;
        }
        if (win_flag & WIN_FLAG_KONG_INVOLVED) {
            fan_table[OUT_WITH_REPLACEMENT_TILE] = 1;
            fan_table[SELF_DRAWN] = 0;
        }
    }
    else {
        if (win_flag & WIN_FLAG_WALL_LAST) {
            fan_table[LAST_TILE_CLAIM] = 1;
        }
        if (win_flag & WIN_FLAG_KONG_INVOLVED) {
            fan_table[ROBBING_THE_KONG] = 1;
            fan_table[LAST_TILE] = 0;
        }
    }
}

#if SUPPORT_BLESSINGS
static void adjust_by_initial_hands(bool east, win_flag_t win_flag, fan_table_t &fan_table) {
    // 自摸时，庄家为天和、闲家为人和Ⅱ
    if (win_flag & WIN_FLAG_SELF_DRAWN) {
        fan_table[east ? BLESSING_OF_HEAVEN : BLESSING_OF_HUMAN_2] = 1;
        fan_table[FULLY_CONCEALED_HAND] = 0;
        fan_table[SELF_DRAWN] = 0;
    }
    // 点炮时，庄家点的为地和、闲家点的为人和Ⅰ
    // NOTE: 这里统一当作地和，需要外部再区分
    else {
        // 庄家没有地和、人和
        if (!east) {
            fan_table[BLESSING_OF_EARTH] = 1;
            fan_table[CONCEALED_HAND] = 0;
        }
    }
}
#endif

// 基本和型算番
static void calculate_regular_fan(const pack_t (&packs)[5], const tile_table_t &fixed_table, const tile_table_t &standing_table,
    const tile_t *unique_tiles, intptr_t unique_cnt,
    const calculate_param_t *calculate_param, bool unique_waiting, win_flag_t win_flag, fan_table_t &fan_table) {

    pack_t pair_pack = 0, chow_packs[4], pung_packs[4];
    int chow_cnt = 0, pung_cnt = 0, concealed_pung_cnt = 0, melded_kong_cnt = 0, concealed_kong_cnt = 0;
    for (int i = 0; i < 5; ++i) {
        pack_t pack = packs[i];
        switch (pack_get_type(pack)) {
        case PACK_TYPE_CHOW:
            chow_packs[chow_cnt++] = pack;
            break;
        case PACK_TYPE_PUNG:
            pung_packs[pung_cnt++] = pack;
            if (!is_pack_melded(pack)) {
                ++concealed_pung_cnt;
            }
            break;
        case PACK_TYPE_KONG:
            pung_packs[pung_cnt++] = pack;
            ++(is_pack_melded(pack) ? melded_kong_cnt : concealed_kong_cnt);
            break;
        case PACK_TYPE_PAIR:
            pair_pack = packs[i];
            break;
        default:
            UNREACHABLE();
            return;
        }
    }

    if (pair_pack == 0 || chow_cnt + pung_cnt != 4) {
        return;
    }

    const tile_t win_tile = calculate_param->win_tile;

    // 根据和牌标记调整——涉及番种：和绝张、妙手回春、海底捞月、自摸
    adjust_by_win_flag(win_flag, fan_table);

    // 点和的牌张，如果不能解释为顺子中的一张，那么将其解释为刻子，并标记这个刻子为明刻
    if ((win_flag & WIN_FLAG_SELF_DRAWN) == 0) {
        // 和牌不能解释为顺子中的一张
        if (std::none_of(chow_packs, chow_packs + chow_cnt, [win_tile](pack_t chow_pack) {
            tile_t tile = pack_get_tile(chow_pack);
            return !is_pack_melded(chow_pack)
                && (tile - 1 == win_tile || tile == win_tile || tile + 1 == win_tile);
        })) {
            for (intptr_t i = 0; i < pung_cnt; ++i) {
                if (pack_get_tile(pung_packs[i]) == win_tile && !is_pack_melded(pung_packs[i])) {
                    --concealed_pung_cnt;
                }
            }
        }
    }

    if (pung_cnt != 0) { // 有刻子
        calculate_kongs(concealed_pung_cnt, melded_kong_cnt, concealed_kong_cnt, fan_table);

        // 四杠和四暗刻不计碰碰和，其他先加上碰碰和的番
        if (pung_cnt == 4) {
            if (fan_table[FOUR_KONGS] == 0 && fan_table[FOUR_CONCEALED_PUNGS] == 0) {
                fan_table[ALL_PUNGS] = 1;
            }
        }

        // 逐组刻子的番（箭刻、幺九刻）
        for (intptr_t i = 0; i < pung_cnt; ++i) {
            fan_t fan = get_1_pung_fan(pack_get_tile(pung_packs[i]));
            if (fan != FAN_NONE) {
                ++fan_table[fan];
            }
        }
    }

    switch (chow_cnt) {
    case 4: {  // 4组顺子
        // 检测三色/一色双龙会
        if (is_three_suited_terminal_chows(chow_packs, pair_pack)) {
            fan_table[THREE_SUITED_TERMINAL_CHOWS] = 1;
            break;
        }
        if (is_pure_terminal_chows(chow_packs, pair_pack)) {
            fan_table[PURE_TERMINAL_CHOWS] = 1;
            break;
        }

        tile_t mid_tiles[4];
        mid_tiles[0] = pack_get_tile(chow_packs[0]);
        mid_tiles[1] = pack_get_tile(chow_packs[1]);
        mid_tiles[2] = pack_get_tile(chow_packs[2]);
        mid_tiles[3] = pack_get_tile(chow_packs[3]);
        std::sort(std::begin(mid_tiles), std::end(mid_tiles));

        calculate_4_chows(mid_tiles, fan_table);
        break;
    }
    case 3: {  // 3组顺子+1组刻子
        tile_t mid_tiles[3];
        mid_tiles[0] = pack_get_tile(chow_packs[0]);
        mid_tiles[1] = pack_get_tile(chow_packs[1]);
        mid_tiles[2] = pack_get_tile(chow_packs[2]);
        std::sort(std::begin(mid_tiles), std::end(mid_tiles));

        calculate_3_chows(mid_tiles, fan_table);
        break;
    }
    case 2: {  // 2组顺子+2组刻子
        tile_t mid_tiles_chow[2];
        mid_tiles_chow[0] = pack_get_tile(chow_packs[0]);
        mid_tiles_chow[1] = pack_get_tile(chow_packs[1]);

        tile_t mid_tiles_pung[2];
        mid_tiles_pung[0] = pack_get_tile(pung_packs[0]);
        mid_tiles_pung[1] = pack_get_tile(pung_packs[1]);

        calculate_2_chows_unordered(mid_tiles_chow, fan_table);
        calculate_2_pungs_unordered(mid_tiles_pung, fan_table);
        break;
    }
    case 1: {  // 1组顺子+3组刻子
        tile_t mid_tiles[3];
        mid_tiles[0] = pack_get_tile(pung_packs[0]);
        mid_tiles[1] = pack_get_tile(pung_packs[1]);
        mid_tiles[2] = pack_get_tile(pung_packs[2]);
        std::sort(std::begin(mid_tiles), std::end(mid_tiles));

        calculate_3_pungs(mid_tiles, fan_table);
        break;
    }
    case 0: {  // 4组刻子
        tile_t mid_tiles[4];
        mid_tiles[0] = pack_get_tile(pung_packs[0]);
        mid_tiles[1] = pack_get_tile(pung_packs[1]);
        mid_tiles[2] = pack_get_tile(pung_packs[2]);
        mid_tiles[3] = pack_get_tile(pung_packs[3]);
        std::sort(std::begin(mid_tiles), std::end(mid_tiles));

        calculate_4_pungs(mid_tiles, fan_table);
        break;
    }
    default:
        UNREACHABLE();
        break;
    }

    intptr_t fixed_cnt = calculate_param->hand_tiles.pack_count;

    // 根据和牌方式调整——涉及番种：不求人、全求人
    adjust_by_self_drawn(packs, fixed_cnt, (win_flag & WIN_FLAG_SELF_DRAWN) != 0, fan_table);

#if SUPPORT_BLESSINGS
    // 天和、地和、人和
    if (fixed_cnt == 0 && (win_flag & WIN_FLAG_INITIAL)) {
        adjust_by_initial_hands(calculate_param->seat_wind == wind_t::EAST, win_flag, fan_table);
    }
#endif

    // 根据雀头调整——涉及番种：平和、小三元、小四喜
    adjust_by_pair_tile(pack_get_tile(pair_pack), chow_cnt, fan_table);
    // 根据牌组特征调整——涉及番种：全带幺、全带五、全双刻
    adjust_by_packs_traits(packs, fan_table);

    tile_table_t merged_table{};
    std::transform(std::begin(standing_table), std::end(standing_table), std::begin(fixed_table), std::begin(merged_table),
        [](uint16_t a, uint16_t b) { return a + b; });

    // 根据花色调整——涉及番种：无字、缺一门、混一色、清一色、五门齐
    adjust_by_suits(unique_tiles, unique_cnt, fan_table);
    // 根据牌特性调整——涉及番种：断幺、推不倒、绿一色、字一色、清幺九、混幺九
    adjust_by_tiles_traits(unique_tiles, unique_cnt, fan_table);
    // 根据数牌的范围调整——涉及番种：大于五、小于五、全大、全中、全小
    adjust_by_rank_range(unique_tiles, unique_cnt, fan_table);
    // 四归一调整（一色四同顺不计四归一）
    if (fan_table[QUADRUPLE_CHOW] == 0) {
        adjust_by_tiles_hog(merged_table, melded_kong_cnt + concealed_kong_cnt, fan_table);
    }

    if (unique_waiting) {
        // 根据听牌方式调整——涉及番种：边张、嵌张、单钓将
        adjust_by_waiting_form(packs + fixed_cnt, 5 - fixed_cnt, win_tile, fan_table);
    }

    // 统一调整一些不计的
    final_adjust(fan_table);

    // 调整圈风刻、门风刻（大四喜不计圈风刻、门风刻）
    if (fan_table[BIG_FOUR_WINDS] == 0) {
        wind_t prevalent_wind = calculate_param->prevalent_wind;
        wind_t seat_wind = calculate_param->seat_wind;

        for (intptr_t i = 0; i < pung_cnt; ++i) {
            tile_t tile = pack_get_tile(pung_packs[i]);
            if (is_winds(tile)) {
                adjust_by_winds(tile, prevalent_wind, seat_wind, fan_table);
            }
        }
    }

    // 如果什么番都没有，则计为无番和
    if (std::all_of(std::begin(fan_table), std::end(fan_table), [](uint16_t p) { return p == 0; })) {
        fan_table[CHICKEN_HAND] = 1;
    }
}

// “组合龙+面子+雀头”和型算番
static bool calculate_knitted_straight_fan(const tile_table_t &fixed_table, const tile_table_t &standing_table,
    const calculate_param_t *calculate_param, win_flag_t win_flag, fan_table_t &fan_table) {

    const hand_tiles_t *hand_tiles = &calculate_param->hand_tiles;
    const pack_t *fixed_packs = hand_tiles->fixed_packs;
    const tile_t win_tile = calculate_param->win_tile;

    if (!has_pair(standing_table)) {
        return false;
    }

    // 匹配组合龙
    const tile_t (*matched_seq)[9] = std::find_if(&standard_knitted_straight[0], &standard_knitted_straight[6],
        [&standing_table](const tile_t (&seq)[9]) {
        return std::all_of(std::begin(seq), std::end(seq), [&standing_table](tile_t t) { return standing_table[t] > 0; });
    });

    if (matched_seq == &standard_knitted_straight[6]) {
        return false;
    }

    // 剔除组合龙
    tile_table_t tile_table;
    memcpy(tile_table, standing_table, sizeof(tile_table));
    std::for_each(std::begin(*matched_seq), std::end(*matched_seq), [&tile_table](tile_t t) { --tile_table[t]; });

    // 按基本和型划分
    division_result_t result;
    result.count = 0;
    division_t work_division;
    memset(&work_division, 0, sizeof(work_division));

    // 此处逻辑为：将组合龙9张牌当作是已经完成的3组面子，空出0 1 2下标处的3组
    // 如果第4组是副露的，将其放在下标3处
    // 然后按基本和型从从fixed_cnt + 3开始递归
    // 划分后的结果，下标3处为第四组面子，下标4处为雀头
    const intptr_t fixed_cnt = hand_tiles->pack_count;
    if (fixed_cnt == 1) {
        work_division.packs[3] = fixed_packs[0];
    }
    divide_recursively(tile_table, fixed_cnt + 3, 0, 0, &work_division, &result);
    if (result.count != 1) {
        return false;
    }

    const pack_t *packs = result.divisions[0].packs;  // packs的0 1 2下标都是没用的
    const pack_t &involved_pack = packs[3];  // 无关第4组
    const tile_t pair_tile = pack_get_tile(packs[4]);  // 雀头牌

    // 标记番
    fan_table[KNITTED_STRAIGHT] = 1;  // 组合龙

    // 根据第4组做一些调整
    const uint8_t involved_type = pack_get_type(involved_pack);
    if (involved_type == PACK_TYPE_CHOW) {  // 第4组是顺子
        // 雀头是数牌时，为平和
        if (is_numbered_suit_quick(pair_tile)) {
            fan_table[ALL_CHOWS] = 1;
        }

        // 只有雀头可能形成四归一
        if (fixed_table[pair_tile] + standing_table[pair_tile] == 4) {
            fan_table[TILE_HOG] = 1;
        }
    }
    else {  // 第4组是刻子
        const tile_t involved_tile = pack_get_tile(involved_pack);
        if (is_honor(involved_tile)) {  // 字牌刻子
            if (is_winds(involved_tile)) {
                // 先标记为幺九刻，再根据圈风、门风调整
                fan_table[PUNG_OF_TERMINALS_OR_HONORS] = 1;
                adjust_by_winds(involved_tile, calculate_param->prevalent_wind, calculate_param->seat_wind, fan_table);

                // 风牌刻子+箭牌雀头时，为五门齐
                if (is_dragons(pair_tile)) {
                    fan_table[ALL_TYPES] = 1;
                }
            }
            else {
                fan_table[DRAGON_PUNG] = 1;

                // 箭牌刻子+风牌雀头时，为五门齐
                if (is_winds(pair_tile)) {
                    fan_table[ALL_TYPES] = 1;
                }
            }
        }
        else {  // 数牌刻子
            if (is_terminal(involved_tile)) {  // 数牌19刻子
                fan_table[PUNG_OF_TERMINALS_OR_HONORS] = 1;
            }

            // 数牌雀头时，为无字
            if (!is_honor(pair_tile)) {
                fan_table[NO_HONORS] = 1;
            }

            // 只数牌刻子可能形成四归一
            if (involved_type != PACK_TYPE_KONG && fixed_table[involved_tile] + standing_table[involved_tile] == 4) {
                fan_table[TILE_HOG] = 1;
            }
        }
    }

    adjust_by_win_flag(win_flag, fan_table);

    // 根据第4组的明暗调整
    if (is_pack_melded(involved_pack)) {
        if (involved_type == PACK_TYPE_KONG) {
            fan_table[MELDED_KONG] = 1;
        }
    }
    else {
        if (involved_type == PACK_TYPE_KONG) {
            fan_table[CONCEALED_KONG] = 1;
        }

        // 增加门前清或不求人
        if (win_flag & WIN_FLAG_SELF_DRAWN) {
            fan_table[FULLY_CONCEALED_HAND] = 1;
            fan_table[SELF_DRAWN] = 0;
        }
        else {
            fan_table[CONCEALED_HAND] = 1;
        }
    }

#if SUPPORT_BLESSINGS
    // 天和、地和、人和
    if (fixed_cnt == 0 && (win_flag & WIN_FLAG_INITIAL)) {
        adjust_by_initial_hands(calculate_param->seat_wind == wind_t::EAST, win_flag, fan_table);
    }
#endif

    const bool heavenly = calculate_param->seat_wind == wind_t::EAST
        && fixed_cnt == 0
        && (win_flag & (WIN_FLAG_INITIAL | WIN_FLAG_SELF_DRAWN)) == (WIN_FLAG_INITIAL | WIN_FLAG_SELF_DRAWN);

#if KNITTED_STRAIGHT_BODY_WITH_ECS == 0
    // 和牌张是组合龙范围的牌，不计边张、嵌张、单钓将
    if (std::none_of(std::begin(*matched_seq), std::end(*matched_seq), [win_tile](tile_t t) { return t == win_tile; })) {
        if (fixed_cnt == 0) {  // 门清的牌有可能存在边张、嵌张、单钓将
            // 天和不计边张、嵌张、单钓将
            if (!heavenly) {
                if (is_unique_waiting(tile_table, 4, win_tile)) {
                    // 根据听牌方式调整——涉及番种：边张、嵌张、单钓将
                    adjust_by_waiting_form(packs + 3, 2, win_tile, fan_table);
                }
            }
        }
        else {
            // 非门清状态如果听牌不在组合龙范围内，必然是单钓将
            fan_table[SINGLE_WAIT] = 1;
        }
    }
#else
    // 如果听牌既可以解释为组合龙的部分，也可以解释为边张、嵌张、单钓将，可加计边张、嵌张、单钓将
    if (fixed_cnt == 0) {  // 门清的牌有可能存在边张、嵌张、单钓将
        // 天和不计边张、嵌张、单钓将
        if (!heavenly) {
            if (is_unique_waiting(tile_table, 4, win_tile)) {
                // 根据听牌方式调整——涉及番种：边张、嵌张、单钓将
                adjust_by_waiting_form(packs + 3, 2, win_tile, fan_table);
            }
        }
    }
    else {
        // 非门清状态如果听牌不在组合龙范围内，必然是单钓将
        if (std::none_of(std::begin(*matched_seq), std::end(*matched_seq), [win_tile](tile_t t) { return t == win_tile; })) {
            fan_table[SINGLE_WAIT] = 1;
        }
        else {
            // 非门清状态如果在龙身上钓将，那么该牌会出现3张
            if (standing_table[win_tile] == 3) {
                fan_table[SINGLE_WAIT] = 1;
            }
        }
    }
#endif

    // 统一调整一些不计的
    final_adjust(fan_table);

    return true;
}

// 七对
static bool is_seven_pairs(const tile_table_t &tile_table) {
    for (int i = 0; i < 34; ++i) {
        tile_t t = all_tiles[i];
        if (tile_table[t] & 1) {
            return false;
        }
    }
    return true;
}

// 连七对
static bool is_seven_shifted_pairs(const tile_table_t &tile_table, suit_t suit) {
    if (suit == TILE_SUIT_HONORS) {
        return false;
    }

    // 连七对只有三种，1~7、2~8、3~9，无论哪种，3~7都是有一对的
    tile_t t3 = make_tile(suit, 3);
    if (tile_table[t3] == 2 && tile_table[t3 + 1] == 2 && tile_table[t3 + 2] == 2 && tile_table[t3 + 3] == 2 && tile_table[t3 + 4] == 2) {
        if (tile_table[t3 - 1] == 2) {
            // 有2，可能是1~7或者2~8
            return (tile_table[t3 - 2] == 2 || tile_table[t3 + 5] == 2);
        }

        // 无2，只能是3~9
        return (tile_table[t3 + 5] == 2 && tile_table[t3 + 6] == 2);
    }
    return false;
}

// 十三幺
static FORCE_INLINE bool is_thirteen_orphans(const tile_t *unique_tiles, intptr_t unique_cnt) {
    return unique_cnt == 13 && std::equal(standard_thirteen_orphans, standard_thirteen_orphans + 13, unique_tiles);
}

// 全不靠/七星不靠算番
static bool calculate_honors_and_knitted_tiles(const tile_t *unique_tiles, intptr_t unique_cnt, fan_table_t &fan_table) {
    if (unique_cnt != 14) {
        return false;
    }

    const tile_t *honor_begin = std::find_if(unique_tiles, unique_tiles + 14, &is_honor);
    int numbered_cnt = static_cast<int>(honor_begin - unique_tiles);
    // 数牌张数大于9或者小于7必然不可能是全不靠
    if (numbered_cnt > 9 || numbered_cnt < 7) {
        return false;
    }

    // 匹配组合龙
    if (std::none_of(&standard_knitted_straight[0], &standard_knitted_straight[6],
        [&unique_tiles, honor_begin](const tile_t (&seq)[9]) {
        return std::includes(std::begin(seq), std::end(seq), unique_tiles, honor_begin);
    })) {
        return false;
    }

    if (numbered_cnt == 7 && std::equal(std::begin(standard_thirteen_orphans) + 6, std::end(standard_thirteen_orphans), unique_tiles + 7)) {
        // 七种字牌齐，为七星不靠
        fan_table[GREATER_HONORS_AND_KNITTED_TILES] = 1;
        return true;
    }
    else if (std::includes(std::begin(standard_thirteen_orphans) + 6, std::end(standard_thirteen_orphans), honor_begin, unique_tiles + 14)) {
        // 全不靠
        fan_table[LESSER_HONORS_AND_KNITTED_TILES] = 1;
        if (numbered_cnt == 9) {  // 有9张数牌，为带组合龙的全不靠
            fan_table[KNITTED_STRAIGHT] = 1;
        }
        return true;
    }

    return false;
}

static void adjust_by_win_flag_4_special_form(wind_t seat_wind, win_flag_t win_flag, fan_table_t &fan_table) {
    adjust_by_win_flag(win_flag, fan_table);
#if SUPPORT_BLESSINGS
    // 天和、地和、人和
    if (win_flag & WIN_FLAG_INITIAL) {
        adjust_by_initial_hands(seat_wind == wind_t::EAST, win_flag, fan_table);
    }
#else
    (void)seat_wind;
#endif
}

// 特殊和型算番
static bool calculate_special_form_fan(const tile_table_t &standing_table, tile_t win_tile,
    const tile_t *unique_tiles, intptr_t unique_cnt, wind_t seat_wind, win_flag_t win_flag, fan_table_t &fan_table) {

    // 按出现频率顺序

    // 七对
    if (is_seven_pairs(standing_table)) {
        suit_t s = tile_get_suit(win_tile);
        if (is_seven_shifted_pairs(standing_table, s)) {
            // 连七对
            fan_table[SEVEN_SHIFTED_PAIRS] = 1;

            // 2~8的连七对加计断幺
            if (standing_table[make_tile(s, 1)] == 0 && standing_table[make_tile(s, 9)] == 0) {
                fan_table[ALL_SIMPLES] = 1;
            }

            adjust_by_win_flag_4_special_form(seat_wind, win_flag, fan_table);
        }
        else {
            // 普通七对
            fan_table[SEVEN_PAIRS] = 1;

            // 根据花色调整——涉及番种：无字、缺一门、混一色、清一色、五门齐
            adjust_by_suits(unique_tiles, unique_cnt, fan_table);
            // 根据牌特性调整——涉及番种：断幺、推不倒、绿一色、字一色、清幺九、混幺九
            adjust_by_tiles_traits(unique_tiles, unique_cnt, fan_table);
            // 根据数牌的范围调整——涉及番种：大于五、小于五、全大、全中、全小
            adjust_by_rank_range(unique_tiles, unique_cnt, fan_table);
            // 四归一调整
            adjust_by_tiles_hog(standing_table, 0, fan_table);

            adjust_by_win_flag_4_special_form(seat_wind, win_flag, fan_table);

            // 统一调整一些不计的
            final_adjust(fan_table);
        }

        return true;
    }

    // 全不靠/七星不靠
    if (calculate_honors_and_knitted_tiles(unique_tiles, unique_cnt, fan_table)) {
        adjust_by_win_flag_4_special_form(seat_wind, win_flag, fan_table);
        return true;
    }

    // 十三幺
    if (is_thirteen_orphans(unique_tiles, unique_cnt)) {
        fan_table[THIRTEEN_ORPHANS] = 1;
        adjust_by_win_flag_4_special_form(seat_wind, win_flag, fan_table);
        return true;
    }

    return false;
}

// 九莲宝灯算番
static bool calculate_nine_gates_fan(const tile_table_t &standing_table, tile_t win_tile, wind_t seat_wind, win_flag_t win_flag, fan_table_t &fan_table) {
    suit_t s;
    rank_t r;
    const bool heavenly = seat_wind == wind_t::EAST
        && (win_flag & (WIN_FLAG_INITIAL | WIN_FLAG_SELF_DRAWN)) == (WIN_FLAG_INITIAL | WIN_FLAG_SELF_DRAWN);
    if (heavenly) {
#if NINE_GATES_WHEN_BLESSING_OF_HEAVEN
        // NOTE: 天和的九莲宝灯与常规的判断方式不一样

        s = tile_get_suit(win_tile);

        // 查找2~8的牌，出现2张的，若出现缺张，则必定不是九莲宝灯
        tile_t win_tile2 = 0;
        for (rank_t i = 2; i < 9; ++i) {
            tile_t tmp = make_tile(s, i);
            const uint16_t cnt = standing_table[tmp];
            if (cnt == 0) {
                return false;
            }
            else if (cnt == 2) {
                win_tile2 = tmp;
            }
        }

        if (win_tile2 != 0) {
            // 如果有出现2张的，且1和9都有3张，则是九莲宝灯
            if (standing_table[make_tile(s, 1)] == 3 && standing_table[make_tile(s, 9)] == 3) {
                r = tile_get_rank(win_tile2);
            }
            else {
                return false;
            }
        }
        else {
            // 如果2~8都只有1张，那么1和9其中一种要4张，另外一种要3张
            if (standing_table[make_tile(s, 1)] == 4 && standing_table[make_tile(s, 9)] == 3) {
                r = 1;
            }
            else if (standing_table[make_tile(s, 1)] == 3 && standing_table[make_tile(s, 9)] == 4) {
                r = 2;
            }
            else {
                return false;
            }
        }
#else
        return false;
#endif
    }
    else {
        s = tile_get_suit(win_tile);
        r = tile_get_rank(win_tile);
        if (r == 1) {
            if (standing_table[win_tile] != 4
                || standing_table[make_tile(s, 9)] != 3
                || std::any_of(standing_table + make_tile(s, 2), standing_table + make_tile(s, 9), [](int n) { return n != 1; })) {
                return false;
            }
        }
        else if (r == 9) {
            if (standing_table[win_tile] != 4
                || standing_table[make_tile(s, 1)] != 3
                || std::any_of(standing_table + make_tile(s, 2), standing_table + make_tile(s, 9), [](int n) { return n != 1; })) {
                return false;
            }
        }
        else {
            if (standing_table[win_tile] != 2
                || standing_table[make_tile(s, 1)] != 3
                || standing_table[make_tile(s, 9)] != 3
                || std::any_of(standing_table + make_tile(s, 2), standing_table + make_tile(s, r), [](int n) { return n != 1; })
                || std::any_of(standing_table + make_tile(s, r + 1), standing_table + make_tile(s, 9), [](int n) { return n != 1; })) {
                return false;
            }
        }
    }

    fan_table[NINE_GATES] = 1;

    // 和1、9加计清龙、四归一
    // 和2、5、8加计双暗刻、幺九刻
    // 和2、3、4、6、7、8加计连六
    switch (r) {
    case 1: case 9:
        fan_table[PURE_STRAIGHT] = 1;
        fan_table[TILE_HOG] = 1;
        break;
    case 2: case 8:
        fan_table[TWO_CONCEALED_PUNGS] = 1;
        fan_table[SHORT_STRAIGHT] = 1;
        fan_table[PUNG_OF_TERMINALS_OR_HONORS] = 1;
        break;
    case 5:
        fan_table[TWO_CONCEALED_PUNGS] = 1;
        fan_table[PUNG_OF_TERMINALS_OR_HONORS] = 1;
        break;
    case 4: case 7: case 3: case 6:
        fan_table[SHORT_STRAIGHT] = 1;
        break;
    default:
        break;
    }

    adjust_by_win_flag_4_special_form(seat_wind, win_flag, fan_table);

    return true;
}

// 从番表计算番数
static int get_fan_by_table(const fan_table_t &fan_table) {
    int fan = 0;
    for (int i = 1; i < FAN_TABLE_SIZE; ++i) {
        if (fan_table[i] == 0) {
            continue;
        }
        fan += fan_value_table[i] * fan_table[i];
#if 0  // Debug
        if (fan_table[i] == 1) {
            LOG("%s %hu\n", fan_name[i], fan_value_table[i]);
        }
        else {
            LOG("%s %hu*%hu\n", fan_name[i], fan_value_table[i], fan_table[i]);
        }
#endif
    }
    return fan;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// 算番
//
int calculate_fan(const calculate_param_t *calculate_param, fan_table_t *fan_table) {
    const hand_tiles_t *hand_tiles = &calculate_param->hand_tiles;
    const intptr_t fixed_cnt = hand_tiles->pack_count;
    const intptr_t standing_cnt = hand_tiles->tile_count;

    // 将每一组副露当作3张牌来算，那么总张数=13
    if (standing_cnt <= 0 || fixed_cnt < 0 || fixed_cnt > 4 || fixed_cnt * 3 + standing_cnt != 13) {
        return ERROR_WRONG_TILES_COUNT;
    }

    const tile_t win_tile = calculate_param->win_tile;

    tile_table_t fixed_table{}, standing_table{};
    map_packs(hand_tiles->fixed_packs, fixed_cnt, fixed_table);
    map_tiles(hand_tiles->standing_tiles, standing_cnt, standing_table);
    ++standing_table[win_tile];  // 把和牌张也加入手牌

    tile_t unique_tiles[18];
    const intptr_t unique_cnt = table_unique(fixed_table, standing_table, unique_tiles);

    win_flag_t win_flag = calculate_param->win_flag;

    // 校正和牌标记
    // 如果立牌包含和牌，则必然不是和绝张
    if (standing_table[win_tile] != 1) {
        win_flag &= ~WIN_FLAG_LAST_TILE;
    }

    // 如果和牌在副露中出现3张，则必然为和绝张
    if (3 == fixed_table[win_tile]) {
        win_flag |= WIN_FLAG_LAST_TILE;
    }

    // 附加杠标记
    if (win_flag & WIN_FLAG_KONG_INVOLVED) {
        if (win_flag & WIN_FLAG_SELF_DRAWN) {  // 自摸
            // 如果手牌没有杠，则必然不是杠上开花
            if (std::none_of(hand_tiles->fixed_packs, hand_tiles->fixed_packs + fixed_cnt,
                [](pack_t pack) { return pack_get_type(pack) == PACK_TYPE_KONG; })) {
                win_flag &= ~WIN_FLAG_KONG_INVOLVED;
            }
        }
        else {  // 点和
            // 如果和牌在手牌范围内出现过，则必然不是抢杠和
            if (fixed_table[win_tile] != 0 || standing_table[win_tile] != 1) {
                win_flag &= ~WIN_FLAG_KONG_INVOLVED;
            }
        }
    }

    // 最大番标记
    int max_fan = 0;
    fan_table_t tmp_table = { 0 };

    // 先判断各种特殊和型
    if (fixed_cnt == 0) {  // 门清状态，有可能是基本和型组合龙
        // 出现频率：七对>组合龙>九莲宝灯
        if (calculate_special_form_fan(standing_table, win_tile, unique_tiles, unique_cnt, calculate_param->seat_wind, win_flag, tmp_table)
            || calculate_knitted_straight_fan(fixed_table, standing_table, calculate_param, win_flag, tmp_table)
            || calculate_nine_gates_fan(standing_table, win_tile, calculate_param->seat_wind, win_flag, tmp_table)) {
            max_fan = get_fan_by_table(tmp_table);
            LOG("fan = %d\n\n", max_fan);
        }
    }
    else if (fixed_cnt == 1) {  // 1副露状态，有可能是基本和型组合龙
        if (calculate_knitted_straight_fan(fixed_table, standing_table, calculate_param, win_flag, tmp_table)) {
            max_fan = get_fan_by_table(tmp_table);
            LOG("fan = %d\n\n", max_fan);
        }
    }

    // 无法构成特殊和型或者为七对
    // 七对也要按基本和型划分，因为极端情况下，基本和型的番会超过七对的番
    if (max_fan == 0 || tmp_table[SEVEN_PAIRS] == 1) {
        const bool heavenly = calculate_param->seat_wind == wind_t::EAST
            && fixed_cnt == 0
            && (win_flag & (WIN_FLAG_INITIAL | WIN_FLAG_SELF_DRAWN)) == (WIN_FLAG_INITIAL | WIN_FLAG_SELF_DRAWN);
        const bool unique_waiting = !heavenly && is_unique_waiting(standing_table, standing_cnt, win_tile);

        // 划分
        division_result_t result;
        if (divide_win_hand(standing_table, hand_tiles->fixed_packs, fixed_cnt, &result)) {
            fan_table_t fan_tables[MAX_DIVISION_CNT] = { { 0 } };
            const fan_table_t *selected_fan_table = nullptr;

            // 遍历各种划分方式，分别算番，找出最大的番的划分方式
            for (intptr_t i = 0; i < result.count; ++i) {
#if 0  // Debug
                char str[64];
                packs_to_string(result.divisions[i].packs, 5, str, sizeof(str));
                puts(str);
#endif
                fan_table_t &current_table = fan_tables[i];
                calculate_regular_fan(result.divisions[i].packs, fixed_table, standing_table, unique_tiles, unique_cnt,
                    calculate_param, unique_waiting, win_flag, current_table);
                int current_fan = get_fan_by_table(current_table);
                if (current_fan > max_fan) {
                    max_fan = current_fan;
                    selected_fan_table = &current_table;
                }
                else if (current_fan == max_fan) {
                    // 2022.3.7 优先选择一色三同顺、全中全带五
                    // 2023.9.2 优先选择三同刻
                    if (current_table[PURE_TRIPLE_CHOW] == 1 || tmp_table[SEVEN_PAIRS] == 1 || current_table[TRIPLE_PUNG]) {
                        selected_fan_table = &current_table;
                    }
                }
                LOG("fan = %d\n\n", current_fan);
            }

            if (fan_table != nullptr && selected_fan_table != nullptr) {
                memcpy(tmp_table, *selected_fan_table, sizeof(tmp_table));
            }
        }
    }

    if (max_fan == 0) {
        return ERROR_NOT_WIN;
    }

    // 加花牌
    max_fan += calculate_param->flower_count;

    if (fan_table != nullptr) {
        memcpy(*fan_table, tmp_table, sizeof(*fan_table));
        (*fan_table)[FLOWER_TILES] = calculate_param->flower_count;
    }

    return max_fan;
}

#if SUPPORT_BLESSINGS
// 判断必然门清（状态）
static bool inevitably_concealed(const fan_table_t &fan_table) {
    return fan_table[SEVEN_PAIRS]
        || fan_table[LESSER_HONORS_AND_KNITTED_TILES]
        || fan_table[GREATER_HONORS_AND_KNITTED_TILES]
        || fan_table[FOUR_CONCEALED_PUNGS]
        || fan_table[THIRTEEN_ORPHANS]
        || fan_table[SEVEN_SHIFTED_PAIRS]
        || fan_table[NINE_GATES];
}

// 撤销天地人和
int revoke_blessings(fan_table_t &fan_table) {
    if (fan_table[BLESSING_OF_HEAVEN]) {
        int res = fan_value_table[BLESSING_OF_HEAVEN];
        fan_table[BLESSING_OF_HEAVEN] = 0;
        if (!inevitably_concealed(fan_table)) {
            fan_table[FULLY_CONCEALED_HAND] = 1;
            return res - fan_value_table[FULLY_CONCEALED_HAND];
        }
        else {
            fan_table[SELF_DRAWN] = 1;
            return res - fan_value_table[SELF_DRAWN];
        }
    }
    else if (fan_table[BLESSING_OF_HUMAN_2]) {
        int res = fan_value_table[BLESSING_OF_HUMAN_2];
        fan_table[BLESSING_OF_HUMAN_2] = 0;
        if (!inevitably_concealed(fan_table)) {
            fan_table[FULLY_CONCEALED_HAND] = 1;
            return res - fan_value_table[FULLY_CONCEALED_HAND];
        }
        else {
            fan_table[SELF_DRAWN] = 1;
            return res - fan_value_table[SELF_DRAWN];
        }
    }
    else if (fan_table[BLESSING_OF_EARTH]) {
        int res = fan_value_table[BLESSING_OF_EARTH];
        fan_table[BLESSING_OF_EARTH] = 0;
        if (!inevitably_concealed(fan_table)) {
            fan_table[CONCEALED_HAND] = 1;
            return res - fan_value_table[CONCEALED_HAND];
        }
        else {
            return res;
        }
    }
    else if (fan_table[BLESSING_OF_HUMAN_1]) {
        int res = fan_value_table[BLESSING_OF_HUMAN_1];
        fan_table[BLESSING_OF_HUMAN_1] = 0;
        if (!inevitably_concealed(fan_table)) {
            fan_table[CONCEALED_HAND] = 1;
            return res - fan_value_table[CONCEALED_HAND];
        }
        else {
            return res;
        }
    }
    return 0;
}
#endif

}
