#include "points_calculator.h"
#include "wait_and_win.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

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

static forceinline bool set_cmp(const SET &set1, const SET &set2) {
    return ((set1.mid_tile < set2.mid_tile)
        || (set1.mid_tile == set2.mid_tile && set1.set_type < set2.set_type));
}

static forceinline bool set_eq(const SET &set1, const SET &set2) {
    return set1.mid_tile == set2.mid_tile && set1.set_type == set2.set_type;
}

struct SEPERATIONS {
    SET sets[MAX_SEPARAION_CNT][5];
    long count;
};

static void seperate_tail_add_pair(TILE tile, long fixed_cnt, SET (&work_sets)[5], SEPERATIONS *separation) {
    // 这2张作为将
    work_sets[4].is_melded = false;
    work_sets[4].mid_tile = tile;
    work_sets[4].set_type = SET_TYPE::PAIR;

    // 拷贝一份当前的划分出来的面子，并排序暗手的面子
    // 这里不能直接在work_sets上排序，否则会破坏递归外层的数据
    SET temp[5];
    memcpy(temp, work_sets, 5 * sizeof(SET));
    std::sort(temp + fixed_cnt, temp + 4, &set_cmp);

    // 检查这种划分是否已经存在了
    if (std::none_of(&separation->sets[0], &separation->sets[separation->count],
        [&temp, fixed_cnt](const SET (&set)[5]) {
        return std::equal(&set[fixed_cnt], &set[4], &temp[fixed_cnt], &set_eq);
    })) {
        memcpy(separation->sets[separation->count], temp, 5 * sizeof(SET));
        ++separation->count;
    }
    else {
        LOG("same case");
    }
}

static bool seperate_tail(int (&cnt_table)[0x54], long fixed_cnt, SET (&work_sets)[5], SEPERATIONS *separation) {
    for (TILE t = 0x11; t <= 0x53; ++t) {
        if (cnt_table[t] < 2) {
            continue;
        }

        cnt_table[t] -= 2;
        // 全部使用完毕
        if (std::all_of(std::begin(cnt_table), std::end(cnt_table), [](int n) { return n == 0; })) {
            cnt_table[t] += 2;
            seperate_tail_add_pair(t, fixed_cnt, work_sets, separation);
            return true;
        }
        cnt_table[t] += 2;
    }

    return false;
}

static bool is_separation_branch_exist(long fixed_cnt, long step, const SET (&work_sets)[5], const SEPERATIONS *separation) {
    if (separation->count <= 0) {
        return false;
    }

    // std::includes要求有序
    SET temp[5];
    memcpy(&temp[fixed_cnt], &work_sets[fixed_cnt], step * sizeof(SET));
    std::sort(&temp[fixed_cnt], &temp[fixed_cnt + step], &set_cmp);

    // 只需要比较面子是否重复分支，将牌不参与比较，所以下标是4
    return std::any_of(&separation->sets[0], &separation->sets[separation->count],
        [&temp, fixed_cnt, step](const SET(&set)[5]) {
        return std::includes(&set[fixed_cnt], &set[4], &temp[fixed_cnt], &temp[fixed_cnt + step], &set_cmp);
    });
}

static bool seperate_recursively(int (&cnt_table)[0x54], long fixed_cnt, long step, SET (&work_sets)[5], SEPERATIONS *separation) {
    long idx = step + fixed_cnt;
    if (idx == 4) {  // 4组面子都有了
        return seperate_tail(cnt_table, fixed_cnt, work_sets, separation);
    }

    bool ret = false;
    for (TILE t = 0x11; t <= 0x53; ++t) {
        if (cnt_table[t] < 1) {
            continue;
        }

        // 刻子
        if (cnt_table[t] > 2) {
            work_sets[idx].is_melded = false;
            work_sets[idx].mid_tile = t;
            work_sets[idx].set_type = SET_TYPE::PUNG;
            if (is_separation_branch_exist(fixed_cnt, step + 1, work_sets, separation)) {
                continue;
            }

            // 削减这组刻子，递归
            cnt_table[t] -= 3;
            if (seperate_recursively(cnt_table, fixed_cnt, step + 1, work_sets, separation)) {
                ret = true;
            }
            cnt_table[t] += 3;
        }

        // 顺子（只能是序数牌）
        bool is_numbered = is_numbered_suit(t);
        if (is_numbered) {
            if (tile_rank(t) < 8 && cnt_table[t + 1] && cnt_table[t + 2]) {
                work_sets[idx].is_melded = false;
                work_sets[idx].mid_tile = t + 1;
                work_sets[idx].set_type = SET_TYPE::CHOW;
                if (is_separation_branch_exist(fixed_cnt, step + 1, work_sets, separation)) {
                    continue;
                }

                // 削减这组顺子，递归
                --cnt_table[t];
                --cnt_table[t + 1];
                --cnt_table[t + 2];
                if (seperate_recursively(cnt_table, fixed_cnt, step + 1, work_sets, separation)) {
                    ret = true;
                }
                ++cnt_table[t];
                ++cnt_table[t + 1];
                ++cnt_table[t + 2];
            }
        }
    }

    return ret;
}

static bool seperate_win_hand(const TILE *standing_tiles, const SET *fixed_sets, long fixed_cnt, SEPERATIONS *separation) {
    long standing_cnt = 14 - fixed_cnt * 3;

    // 对立牌的种类进行打表
    int cnt_table[0x54];
    map_tiles(standing_tiles, standing_cnt, cnt_table);

    separation->count = 0;

    // 复制副露的面子
    SET work_sets[5];
    memcpy(work_sets, fixed_sets, fixed_cnt * sizeof(SET));
    return seperate_recursively(cnt_table, fixed_cnt, 0, work_sets, separation);
}

// 从一组一组的牌恢复成一张一张的牌
void recovery_tiles_from_sets(const SET *sets, long set_cnt, TILE *tiles, long *tile_cnt) {
    assert(tiles != nullptr && tile_cnt != nullptr);
    *tile_cnt = 0;
    for (int i = 0; i < set_cnt; ++i) {
        switch (sets[i].set_type) {
        case SET_TYPE::CHOW:
            tiles[(*tile_cnt)++] = sets[i].mid_tile - 1;
            tiles[(*tile_cnt)++] = sets[i].mid_tile;
            tiles[(*tile_cnt)++] = sets[i].mid_tile + 1;
            break;
        case SET_TYPE::PUNG:
            tiles[(*tile_cnt)++] = sets[i].mid_tile;
            tiles[(*tile_cnt)++] = sets[i].mid_tile;
            tiles[(*tile_cnt)++] = sets[i].mid_tile;
            break;
        case SET_TYPE::KONG:
            tiles[(*tile_cnt)++] = sets[i].mid_tile;
            tiles[(*tile_cnt)++] = sets[i].mid_tile;
            tiles[(*tile_cnt)++] = sets[i].mid_tile;
            tiles[(*tile_cnt)++] = sets[i].mid_tile;
            break;
        case SET_TYPE::PAIR:
            tiles[(*tile_cnt)++] = sets[i].mid_tile;
            tiles[(*tile_cnt)++] = sets[i].mid_tile;
            break;
        default:
            assert(0);
            break;
        }
    }
}

bool map_hand_tiles(const HAND_TILES *hand_tiles, int (&cnt_table)[0x54]) {
    // 将每一次副露当作3张牌来算，那么总张数=13
    if (hand_tiles->tile_count <= 0 || hand_tiles->set_count < 0 || hand_tiles->set_count > 4
        || hand_tiles->set_count * 3 + hand_tiles->tile_count != 13) {
        return false;
    }

    // 将副露恢复成牌
    TILE tiles[18];
    long tile_cnt = 0;
    if (hand_tiles->set_count == 0) {
        memcpy(tiles, hand_tiles->standing_tiles, 13 * sizeof(TILE));
        tile_cnt = 13;
    }
    else {
        recovery_tiles_from_sets(hand_tiles->fixed_sets, hand_tiles->set_count, tiles, &tile_cnt);
        memcpy(tiles + tile_cnt, hand_tiles->standing_tiles, hand_tiles->tile_count * sizeof(TILE));
        tile_cnt += hand_tiles->tile_count;
    }

    // 打表
    map_tiles(tiles, tile_cnt, cnt_table);
    return true;
}

// 一色四同顺
static forceinline bool is_quadruple_chow(TILE t0, TILE t1, TILE t2, TILE t3) {
    return (t0 == t1 && t0 == t2 && t0 == t3);
}

// 一色四节高
static forceinline bool is_four_pure_shifted_pungs(TILE t0, TILE t1, TILE t2, TILE t3) {
    if (is_numbered_suit_quick(t0)) {
        return (t0 + 1 == t1 && t1 + 1 == t2 && t2 + 1 == t3);
    }
    return false;
}

// 一色四步高
static forceinline bool is_four_pure_shifted_chows(TILE t0, TILE t1, TILE t2, TILE t3) {
    // 递增2的必然是：123 345 567 789
    // 递增1的最小为：123 234 345 456 最大为：456 567 678 789
    return ((t0 + 2 == t1 && t1 + 2 == t2 && t2 + 2 == t3)
        || (t0 + 1 == t1 && t1 + 1 == t2 && t2 + 1 == t3));
}

// 一色三同顺
static forceinline bool is_pure_triple_chow(TILE t0, TILE t1, TILE t2) {
    return (t0 == t1 && t0 == t2);
}

// 一色三节高
static forceinline bool is_pure_shifted_pungs(TILE t0, TILE t1, TILE t2) {
    return (is_numbered_suit_quick(t0) && t0 + 1 == t1 && t1 + 1 == t2);
}

// 清龙
static forceinline bool is_pure_straight(TILE t0, TILE t1, TILE t2) {
    return (tile_rank(t0) == 2 && t0 + 3 == t1 && t1 + 3 == t2);
}

// 一色三步高
static forceinline bool is_pure_shifted_chows(TILE t0, TILE t1, TILE t2) {
    return ((t0 + 2 == t1 && t1 + 2 == t2) || (t0 + 1 == t1 && t1 + 1 == t2));
}

// 花龙
static bool is_mixed_straight(TILE t0, TILE t1, TILE t2) {
    SUIT_TYPE s0 = tile_suit(t0);
    SUIT_TYPE s1 = tile_suit(t1);
    SUIT_TYPE s2 = tile_suit(t2);
    if (s0 != s1 && s0 != s2 && s1 != s2) {
        RANK_TYPE r0 = tile_rank(t0);
        RANK_TYPE r1 = tile_rank(t1);
        RANK_TYPE r2 = tile_rank(t2);
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
static bool is_mixed_triple_chow_or_pung(TILE t0, TILE t1, TILE t2) {
    if (!is_numbered_suit_quick(t0) || !is_numbered_suit_quick(t1) || !is_numbered_suit_quick(t2)) {
        return false;
    }
    SUIT_TYPE s0 = tile_suit(t0);
    SUIT_TYPE s1 = tile_suit(t1);
    SUIT_TYPE s2 = tile_suit(t2);
    if (s0 != s1 && s0 != s2 && s1 != s2) {
        RANK_TYPE r0 = tile_rank(t0);
        RANK_TYPE r1 = tile_rank(t1);
        RANK_TYPE r2 = tile_rank(t2);
        return (r0 == r1 && r0 == r2);
    }
    return false;
}

// 三色三节高或者三色三步高
static bool is_mixed_shifted_chow_or_pung(TILE t0, TILE t1, TILE t2) {
    if (!is_numbered_suit_quick(t0) || !is_numbered_suit_quick(t1) || !is_numbered_suit_quick(t2)) {
        return false;
    }
    SUIT_TYPE s0 = tile_suit(t0);
    SUIT_TYPE s1 = tile_suit(t1);
    SUIT_TYPE s2 = tile_suit(t2);
    if (s0 != s1 && s0 != s2 && s1 != s2) {
        RANK_TYPE r0 = tile_rank(t0);
        RANK_TYPE r1 = tile_rank(t1);
        RANK_TYPE r2 = tile_rank(t2);
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
static forceinline bool is_pure_double_chow(TILE t0, TILE t1) {
    return t0 == t1;
}

// 喜相逢
static forceinline bool is_mixed_double_chow(TILE t0, TILE t1) {
    return (is_rank_equal_quick(t0, t1) && !is_suit_equal_quick(t0, t1));
}

// 连六
static forceinline bool is_short_straight(TILE t0, TILE t1) {
    return (t0 + 3 == t1 || t1 + 3 == t0);
}

// 老少副
static bool is_two_terminal_chows(TILE t0, TILE t1) {
    if (is_suit_equal_quick(t0, t1)) {
        RANK_TYPE r0 = tile_rank(t0);
        RANK_TYPE r1 = tile_rank(t1);
        return ((r0 == 2 && r1 == 8) || (r0 == 8 && r1 == 2));
    }
    return false;
}

// 4组顺子的番
static POINT_TYPE get_4_chows_points(TILE t0, TILE t1, TILE t2, TILE t3) {
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
    return NONE;
}

// 3组顺子的番
static POINT_TYPE get_3_chows_points(TILE t0, TILE t1, TILE t2) {
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
    return NONE;
}

// 2组顺子的番
static POINT_TYPE get_2_chows_points(TILE t0, TILE t1) {
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
    return NONE;
}

// 大四喜
static forceinline bool is_big_four_winds(TILE t0, TILE t1, TILE t2, TILE t3) {
    return (is_winds(t0) && is_winds(t1) && is_winds(t2) && is_winds(t3));
}

// 大三元
static forceinline bool is_big_three_dragons(TILE t0, TILE t1, TILE t2) {
    return (is_dragons(t0) && is_dragons(t1) && is_dragons(t2));
}

// 三风刻
static forceinline bool is_big_three_winds(TILE t0, TILE t1, TILE t2) {
    return (is_winds(t0) && is_winds(t1) && is_winds(t2));
}

// 双箭刻
static forceinline bool is_two_dragons_pungs(TILE t0, TILE t1) {
    return (is_winds(t0) && is_winds(t1));
}

// 双同刻
static forceinline bool is_double_pung(TILE t0, TILE t1) {
    return (is_numbered_suit_quick(t0) && is_numbered_suit_quick(t1)
        && is_rank_equal_quick(t0, t1));
}

// 4组刻子的番
static POINT_TYPE get_4_pungs_points(TILE t0, TILE t1, TILE t2, TILE t3) {
    // 一色四节高
    if (is_four_pure_shifted_pungs(t0, t1, t2, t3)) {
        return FOUR_PURE_SHIFTED_PUNGS;
    }
    // 大四喜
    if (is_big_four_winds(t0, t1, t2, t3)) {
        return BIG_FOUR_WINDS;
    }
    // 以上都没有
    return NONE;
}

// 3组刻子的番
static POINT_TYPE get_3_pungs_points(TILE t0, TILE t1, TILE t2) {
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
    return NONE;
}

// 2组刻子的番
static POINT_TYPE get_2_pungs_points(TILE t0, TILE t1) {
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
    return NONE;
}

// 1组刻子的番
static POINT_TYPE get_1_pung_points(TILE mid_tile) {
    // 箭刻
    if (is_dragons(mid_tile)) {
        return DRAGON_PUNG;
    }
    // 幺九刻
    if (is_terminal(mid_tile) || is_winds(mid_tile)) {
        return PUNG_OF_TERMINALS_OR_HONORS;
    }
    // 以上都没有
    return NONE;
}

template <long _Size>
static POINT_TYPE *pairwise_test_chows(const SET (&chows_sets)[_Size], POINT_TYPE *selected_points) {
    POINT_TYPE all_points[_Size][_Size] = { { NONE } };

    // 初始化矩阵
    for (int i = 0; i < _Size; ++i) {
        for (int j = 0; j < i - 1; ++j) {  // 这是对称矩阵
            all_points[i][j] = all_points[j][i];
        }
        for (int j = i + 1; j < _Size; ++j) {  // 获取所有两两组合的番种
            all_points[i][j] = get_2_chows_points(chows_sets[i].mid_tile, chows_sets[j].mid_tile);
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
            POINT_TYPE pt = all_points[i][j];
            if (pt != NONE) {
                int idx = pt - PURE_DOUBLE_CHOW;  // 般逢连老
                // 不得相同原则，如果i和j都没算过某一种番，则算这种番
                if ((used_flag[i] & (1 << idx)) == 0 && (used_flag[j] & (1 << idx)) == 0) {
                    used_flag[i] |= (1 << (pt - PURE_DOUBLE_CHOW));
                    used_flag[j] |= (1 << (pt - PURE_DOUBLE_CHOW));
                    *selected_points = pt;  // 写入这个番
                    ++selected_points;
                }
            }
        }
    }
    return selected_points;
}

// 4组顺子算番
static void calculate_4_chows(const SET chow_sets[4], long (&points_table)[POINT_TYPE_COUNT]) {
    // 复制并排序
    SET sets[4];
    memcpy(sets, chow_sets, sizeof(sets));
    std::sort(std::begin(sets), std::end(sets), [](const SET &set1, const SET &set2) {
        return set1.mid_tile < set2.mid_tile;
    });

    POINT_TYPE points;
    // 存在4组顺子的番种时，不再检测其他的了
    if ((points = get_4_chows_points(sets[0].mid_tile, sets[1].mid_tile, sets[2].mid_tile, sets[3].mid_tile))) {
        points_table[points] = 1;
        return;
    }

    // 3组顺子判断
    bool _3_chows_has_points = false;
    long free_set_idx = -1;  // 未使用的1组顺子
    // 012构成3组顺子的番种
    if ((points = get_3_chows_points(sets[0].mid_tile, sets[1].mid_tile, sets[2].mid_tile)) != NONE) {
        points_table[points] = 1;
        free_set_idx = 3;
        _3_chows_has_points = true;
    }
    // 013构成3组顺子的番种
    else if ((points = get_3_chows_points(sets[0].mid_tile, sets[1].mid_tile, sets[3].mid_tile)) != NONE) {
        points_table[points] = 1;
        free_set_idx = 2;
        _3_chows_has_points = true;
    }
    // 023构成3组顺子的番种
    else if ((points = get_3_chows_points(sets[0].mid_tile, sets[2].mid_tile, sets[3].mid_tile)) != NONE) {
        points_table[points] = 1;
        free_set_idx = 1;
        _3_chows_has_points = true;
    }
    // 123构成3组顺子的番种
    else if ((points = get_3_chows_points(sets[1].mid_tile, sets[2].mid_tile, sets[3].mid_tile)) != NONE) {
        points_table[points] = 1;
        free_set_idx = 0;
        _3_chows_has_points = true;
    }

    // 存在3组顺子的番种时，余下的第4组顺子最多算1番
    if (_3_chows_has_points) {
        for (long i = 0; i < 4; ++i) {
            if (i == free_set_idx) {
                continue;
            }
            // 依次与未使用的这组顺子测试番种，一旦有则不再计算了（套算一次原则）
            if ((points = get_2_chows_points(sets[i].mid_tile, sets[free_set_idx].mid_tile)) != NONE) {
                ++points_table[points];
                break;
            }
        }
        return;
    }

    // 不存在3组顺子的番种时，4组顺子最多3番
    POINT_TYPE selected_points[3] = { NONE };
    pairwise_test_chows(sets, selected_points);
    for (long i = 0; i < 3; ++i) {
        if (selected_points[i] != NONE) {
            ++points_table[selected_points[i]];
        }
    }
}

// 3组顺子算番
static void calculate_3_chows(const SET chow_sets[3], long (&points_table)[POINT_TYPE_COUNT]) {
    // 复制并排序
    SET sets[3];
    memcpy(sets, chow_sets, sizeof(sets));
    std::sort(std::begin(sets), std::end(sets), [](const SET &set1, const SET &set2) {
        return set1.mid_tile < set2.mid_tile;
    });

    POINT_TYPE points;

    // 存在3组顺子的番种时，不再检测其他的
    if ((points = get_3_chows_points(sets[0].mid_tile, sets[1].mid_tile, sets[2].mid_tile)) != NONE) {
        points_table[points] = 1;
        return;
    }

    // 不存在上述番种时，3组顺子最多2番
    POINT_TYPE selected_points[2] = { NONE };
    pairwise_test_chows(sets, selected_points);
    for (long i = 0; i < 2; ++i) {
        if (selected_points[i] != NONE) {
            ++points_table[selected_points[i]];
        }
    }
}

// 2组顺子算番
static void calculate_2_chows(const SET chow_sets[2], long (&points_table)[POINT_TYPE_COUNT]) {
    const SET *sets = chow_sets;
    POINT_TYPE points;
    if ((points = get_2_chows_points(sets[0].mid_tile, sets[1].mid_tile)) != NONE) {
        ++points_table[points];
    }
}

// 刻子（杠）算番
static void calculate_kongs(const SET *pung_sets, long pung_cnt, long (&points_table)[POINT_TYPE_COUNT]) {
    // 统计明杠 暗杠 明刻 暗刻
    int melded_kong_cnt = 0;
    int concealed_kong_cnt = 0;
    int concealed_pung_cnt = 0;
    for (long i = 0; i < pung_cnt; ++i) {
        if (pung_sets[i].is_melded) {
            if (pung_sets[i].set_type == SET_TYPE::KONG) {
                ++melded_kong_cnt;
            }
        }
        else {
            if (pung_sets[i].set_type == SET_TYPE::KONG) {
                ++concealed_kong_cnt;
            }
            else {
                ++concealed_pung_cnt;
            }
        }
    }

    // 规则
    // 三杠
    // 明杠 明杠 暗杠 暗刻 -> 三杠+暗杠+双暗刻+碰碰和
    // 明杠 暗杠 暗杠 明刻 -> 三杠+双暗杠+碰碰和
    // 明杠 暗杠 暗杠 暗刻 -> 三杠+三暗刻+双暗杠+碰碰和
    // 暗杠 暗杠 暗杠 明刻 -> 三杠+三暗刻+碰碰和
    // 暗杠 暗杠 暗杠 暗刻 -> 三杠+四暗刻
    //
    // 四杠
    // 暗杠 明杠 明杠 明杠 -> 四杠+暗杠
    // 暗杠 暗杠 明杠 明杠 -> 四杠+双暗杠
    // 暗杠 暗杠 暗杠 明杠 -> 四杠+三暗刻
    // 暗杠 暗杠 暗杠 暗杠 -> 四杠+四暗刻
    //

    int kong_cnt = melded_kong_cnt + concealed_kong_cnt;
    switch (kong_cnt) {
    case 0:  // 0个杠
        switch (concealed_pung_cnt) {  // 暗刻的个数
        case 0: break;
        case 1: break;
        case 2: points_table[TWO_CONCEALED_PUNGS] = 1; break;
        case 3: points_table[THREE_CONCEALED_PUNGS] = 1; break;
        case 4: points_table[FOUR_CONCEALED_PUNGS] = 1; break;
        default: assert(0); break;
        }
        break;
    case 1:  // 1个杠
        if (melded_kong_cnt == 1) {  // 明杠
            points_table[MELDED_KONG] = 1;
            switch (concealed_pung_cnt) {  // 暗刻的个数
            case 0: break;
            case 1: break;
            case 2: points_table[TWO_CONCEALED_PUNGS] = 1; break;
            case 3: points_table[THREE_CONCEALED_PUNGS] = 1; break;
            default: assert(0); break;
            }
        }
        else {  // 暗杠
            points_table[CONCEALED_KONG] = 1;
            switch (concealed_pung_cnt) {  // 暗刻的个数
            case 0: break;
            case 1: points_table[TWO_CONCEALED_PUNGS] = 1; break;
            case 2: points_table[THREE_CONCEALED_PUNGS] = 1; break;
            case 3: points_table[FOUR_CONCEALED_PUNGS] = 1; break;
            default: assert(0); break;
            }
        }
        break;
    case 2:  // 2个杠
        switch (concealed_kong_cnt) {
        case 0:  // 双明杠
            points_table[TWO_MELDED_KONGS] = 1;
            switch (concealed_pung_cnt) {  // 暗刻的个数
            case 0: break;
            case 1: break;
            case 2: points_table[TWO_CONCEALED_PUNGS] = 1; break;
            default: assert(0); break;
            }
            break;
        case 1:  // 明暗杠
#if HAS_CONCEALED_KONG_AND_MELDED_KONG
            points_table[CONCEALED_KONG_AND_MELDED_KONG] = 1;
#else
            points_table[MELDED_KONG] = 1;
            points_table[CONCEALED_KONG] = 1;
#endif
            switch (concealed_pung_cnt) {  // 暗刻的个数
            case 0: break;
            case 1: points_table[TWO_CONCEALED_PUNGS] = 1; break;
            case 2: points_table[THREE_CONCEALED_PUNGS] = 1; break;
            default: assert(0); break;
            }
            break;
        case 2:  // 双暗杠
            points_table[TWO_CONCEALED_KONGS] = 1;
            switch (concealed_pung_cnt) {  // 暗刻的个数
            case 0: break;
            case 1: points_table[THREE_CONCEALED_PUNGS] = 1; break;
            case 2: points_table[FOUR_CONCEALED_PUNGS] = 1; break;
            default: assert(0); break;
            }
            break;
        default:
            assert(0);
            break;
        }
        break;
    case 3:  // 3个杠
        points_table[THREE_KONGS] = 1;
        switch (concealed_kong_cnt) {  // 暗刻的个数
        case 0: break;
        case 1:
            if (concealed_pung_cnt == 0) {
                points_table[CONCEALED_KONG] = 1;
            }
            else {
                points_table[CONCEALED_KONG] = 1;
                points_table[TWO_CONCEALED_PUNGS] = 1;
            }
            break;
        case 2:
            if (concealed_pung_cnt == 0) {
                points_table[TWO_CONCEALED_KONGS] = 1;
            }
            else {
                points_table[THREE_CONCEALED_PUNGS] = 1;
                points_table[TWO_CONCEALED_KONGS] = 1;
            }
            break;
        case 3:
            if (concealed_pung_cnt == 0) {
                points_table[THREE_CONCEALED_PUNGS] = 1;
            }
            else {
                points_table[FOUR_CONCEALED_PUNGS] = 1;
            }
            break;
        default:
            assert(0);
            break;
        }
        break;
    case 4:  // 4个杠
        points_table[FOUR_KONGS] = 1;
        switch (concealed_kong_cnt) {
        case 0: break;
        case 1: points_table[CONCEALED_KONG] = 1; break;
        case 2: points_table[TWO_CONCEALED_KONGS] = 1; break;
        case 3: points_table[THREE_CONCEALED_PUNGS] = 1; break;
        case 4: points_table[FOUR_CONCEALED_PUNGS] = 1; break;
        default: assert(0); break;
        }
        break;
    default:
        assert(0);
        break;
    }

    // 四杠和四暗刻不计碰碰和，其他先加上碰碰和的番
    if (pung_cnt == 4) {
        if (points_table[FOUR_KONGS] == 0 && points_table[FOUR_CONCEALED_PUNGS] == 0) {
            points_table[ALL_PUNGS] = 1;
        }
    }

    // 逐组刻子的番（箭刻、幺九刻）
    for (long i = 0; i < pung_cnt; ++i) {
        POINT_TYPE points = get_1_pung_points(pung_sets[i].mid_tile);
        if (points != NONE) {
            ++points_table[points];
        }
    }
}

// 4组刻子算番
static void calculate_4_pungs(const SET pung_sets[4], long (&points_table)[POINT_TYPE_COUNT]) {
    // 复制并排序
    SET sets[4];
    memcpy(sets, pung_sets, sizeof(sets));
    std::sort(std::begin(sets), std::end(sets), [](const SET &set1, const SET &set2) {
        return set1.mid_tile < set2.mid_tile;
    });

    calculate_kongs(sets, 4, points_table);

    POINT_TYPE points;
    // 存在4组刻子的番种时，不再检测其他的了
    if ((points = get_4_pungs_points(sets[0].mid_tile, sets[1].mid_tile, sets[2].mid_tile, sets[3].mid_tile)) != NONE) {
        points_table[points] = 1;
        return;
    }

    // 3组刻子判断
    bool _3_pungs_has_points = false;
    long free_set_idx = -1;  // 未使用的1组刻子
    // 012构成3组刻子的番种
    if ((points = get_3_pungs_points(sets[0].mid_tile, sets[1].mid_tile, sets[2].mid_tile)) != NONE) {
        points_table[points] = 1;
        free_set_idx = 3;
        _3_pungs_has_points = true;
    }
    // 013构成3组刻子的番种
    else if ((points = get_3_pungs_points(sets[0].mid_tile, sets[1].mid_tile, sets[3].mid_tile)) != NONE) {
        points_table[points] = 1;
        free_set_idx = 2;
        _3_pungs_has_points = true;
    }
    // 023构成3组刻子的番种
    else if ((points = get_3_pungs_points(sets[0].mid_tile, sets[2].mid_tile, sets[3].mid_tile)) != NONE) {
        points_table[points] = 1;
        free_set_idx = 1;
        _3_pungs_has_points = true;
    }
    // 123构成3组刻子的番种
    else if ((points = get_3_pungs_points(sets[1].mid_tile, sets[2].mid_tile, sets[3].mid_tile)) != NONE) {
        points_table[points] = 1;
        free_set_idx = 0;
        _3_pungs_has_points = true;
    }

    // 存在3组刻子的番种时，余下的第4组刻子只能组合一次
    if (_3_pungs_has_points) {
        for (long i = 0; i < 4; ++i) {
            if (i == free_set_idx) {
                continue;
            }
            // 依次与未使用的这组刻子测试番种
            if ((points = get_2_pungs_points(sets[i].mid_tile, sets[free_set_idx].mid_tile)) != NONE) {
                ++points_table[points];
                break;
            }
        }
        return;
    }

    // 不存在3组刻子的番种时，两两计算番种
    if ((points = get_2_pungs_points(sets[0].mid_tile, sets[1].mid_tile)) != NONE) {
        ++points_table[points];
    }
    if ((points = get_2_pungs_points(sets[0].mid_tile, sets[2].mid_tile)) != NONE) {
        ++points_table[points];
    }
    if ((points = get_2_pungs_points(sets[0].mid_tile, sets[3].mid_tile)) != NONE) {
        ++points_table[points];
    }
    if ((points = get_2_pungs_points(sets[1].mid_tile, sets[2].mid_tile)) != NONE) {
        ++points_table[points];
    }
    if ((points = get_2_pungs_points(sets[1].mid_tile, sets[3].mid_tile)) != NONE) {
        ++points_table[points];
    }
    if ((points = get_2_pungs_points(sets[2].mid_tile, sets[3].mid_tile)) != NONE) {
        ++points_table[points];
    }
}

// 3组刻子算番
static void calculate_3_pungs(const SET pung_sets[3], long (&points_table)[POINT_TYPE_COUNT]) {
    SET sets[3];
    memcpy(sets, pung_sets, sizeof(sets));
    std::sort(std::begin(sets), std::end(sets), [](const SET &set1, const SET &set2) {
        return set1.mid_tile < set2.mid_tile;
    });

    calculate_kongs(sets, 3, points_table);

    POINT_TYPE points;

    // 存在3组刻子的番种（三节高 三同刻 三风刻 大三元）时，不再检测其他的
    if ((points = get_3_pungs_points(sets[0].mid_tile, sets[1].mid_tile, sets[2].mid_tile)) != NONE) {
        points_table[points] = 1;
        return;
    }

    // 不存在3组刻子的番种时，两两计算番种
    if ((points = get_2_pungs_points(sets[0].mid_tile, sets[1].mid_tile)) != NONE) {
        ++points_table[points];
    }
    if ((points = get_2_pungs_points(sets[0].mid_tile, sets[2].mid_tile)) != NONE) {
        ++points_table[points];
    }
    if ((points = get_2_pungs_points(sets[1].mid_tile, sets[2].mid_tile)) != NONE) {
        ++points_table[points];
    }
}

// 2组刻子算番
static void calculate_2_pungs(const SET pung_sets[2], long (&points_table)[POINT_TYPE_COUNT]) {
    calculate_kongs(pung_sets, 2, points_table);
    POINT_TYPE points = get_2_pungs_points(pung_sets[0].mid_tile, pung_sets[1].mid_tile);
    if (points != NONE) {
        ++points_table[points];
    }
}

// 1组刻子算番
static void calculate_1_pung(const SET &pung_set, long (&points_table)[POINT_TYPE_COUNT]) {
    calculate_kongs(&pung_set, 1, points_table);
}

// 九莲宝灯
static bool is_nine_gates(const TILE (&tiles)[14], TILE win_tile) {
    // 对立牌的种类进行打表
    int cnt_table[0x54];
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
static bool is_pure_terminal_chows(const SET (&chow_sets)[4], const SET &pair_set) {
    if (tile_rank(pair_set.mid_tile) != 5) {  // 5作将
        return false;
    }

    int _123_cnt = 0, _789_cnt = 0;
    SUIT_TYPE pair_suit = tile_suit(pair_set.mid_tile);
    for (long i = 0; i < 4; ++i) {
        SUIT_TYPE suit = tile_suit(chow_sets[i].mid_tile);
        if (suit != pair_suit) {  // 花色与将相同
            return false;
        }
        RANK_TYPE rank = tile_rank(chow_sets[i].mid_tile);
        switch (rank) {
        case 2: ++_123_cnt; break;
        case 8: ++_789_cnt; break;
        default: return false;
        }
    }
    return (_123_cnt == 2 && _789_cnt == 2);  // 123和789各2组
}

// 三色双龙会
static bool is_three_suited_terminal_chows(const SET (&chow_sets)[4], const SET &pair_set) {
    if (tile_rank(pair_set.mid_tile) != 5) {  // 5作将
        return false;
    }

    int _123_suit_table[4] = { 0 };
    int _789_suit_table[4] = { 0 };
    SUIT_TYPE pair_suit = tile_suit(pair_set.mid_tile);
    for (long i = 0; i < 4; ++i) {
        SUIT_TYPE suit = tile_suit(chow_sets[i].mid_tile);
        if (suit == pair_suit) {  // 花色与将不相同
            return false;
        }
        RANK_TYPE rank = tile_rank(chow_sets[i].mid_tile);
        switch (rank) {
        case 2: ++_123_suit_table[suit]; break;
        case 8: ++_789_suit_table[suit]; break;
        default: return false;
        }
    }

    // 与将花色不同的两色123和789各一组
    switch (pair_suit) {
    case 1: return (_123_suit_table[2] && _123_suit_table[3] && _789_suit_table[2] && _789_suit_table[3]);
    case 2: return (_123_suit_table[1] && _123_suit_table[3] && _789_suit_table[1] && _789_suit_table[3]);
    case 3: return (_123_suit_table[1] && _123_suit_table[2] && _789_suit_table[1] && _789_suit_table[2]);
    default: return false;
    }
}

// 检测不求人、全求人
static void check_melded_or_concealed_hand(const SET (&sets)[5], long fixed_cnt, bool self_drawn, long (&points_table)[POINT_TYPE_COUNT]) {
    long melded_cnt = 0;  // 明的组数
    for (long i = 0; i < fixed_cnt; ++i) {
        if (sets[i].is_melded) {
            ++melded_cnt;
        }
    }

    switch (melded_cnt) {
    case 0:  // 0组明的，自摸为不求人，点和为门前清
        points_table[self_drawn ? FULLY_CONCEALED_HAND : CONCEALED_HAND] = 1;
        break;
    case 4:
        // 4组明的，自摸还是自摸，点和为全求人
        points_table[self_drawn ? SELF_DRAWN : MELDED_HAND] = 1;
        break;
    default:
        if (self_drawn) {
            points_table[SELF_DRAWN] = 1;
        }
        break;
    }
}

// 检测将，确定平和、小三元、小四喜
static void check_pair_tile(TILE pair_tile, long chow_cnt, long (&points_table)[POINT_TYPE_COUNT]) {
    if (chow_cnt == 4) {  // 4组都是顺子
        if (is_numbered_suit_quick(pair_tile)) {  // 数牌将
            points_table[ALL_CHOWS] = 1;  // 平和
        }
        return;
    }

    // 在双箭刻的基础上，如果将是箭牌，则修正为小三元
    if (points_table[TWO_DRAGONS_PUNGS]) {
        if (is_dragons(pair_tile)) {
            points_table[LITTLE_THREE_DRAGONS] = 1;
            points_table[TWO_DRAGONS_PUNGS] = 0;
        }
        return;
    }
    // 在三风刻的基础上，如果将是风牌，则修正为小四喜
    if (points_table[BIG_THREE_WINDS]) {
        if (is_winds(pair_tile)) {
            points_table[LITTLE_FOUR_WINDS] = 1;
            points_table[BIG_THREE_WINDS] = 0;
        }
        return;
    }
}

// 检测门（五门齐的门）
static void check_tiles_suits(const TILE *tiles, long tile_cnt, long (&points_table)[POINT_TYPE_COUNT]) {
    // 打表标记有哪些花色
    uint8_t suit_flag = 0;
    for (long i = 0; i < tile_cnt; ++i) {
        suit_flag |= (1 << tile_suit(tiles[i]));
    }

    // 0011 1110
    if (suit_flag == UINT8_C(0x3E)) {
        points_table[ALL_TYPES] = 1;  // 五门齐
        return;
    }

    // 1111 0001
    if (!(suit_flag & UINT8_C(0xF1))) {
        points_table[NO_HONORS] = 1;  // 无字
    }

    // 1100 0011
    if (!(suit_flag & UINT8_C(0xC3))) {
        ++points_table[ONE_VOIDED_SUIT];  // 缺一门（万）
    }
    // 1100 0101
    if (!(suit_flag & UINT8_C(0xC5))) {
        ++points_table[ONE_VOIDED_SUIT];  // 缺一门（条）
    }
    // 1100 1001
    if (!(suit_flag & UINT8_C(0xC9))) {
        ++points_table[ONE_VOIDED_SUIT];  // 缺一门（饼）
    }

    // 当缺2门时，根据有字和无字，修正为混一色和清一色
    if (points_table[ONE_VOIDED_SUIT] == 2) {
        points_table[ONE_VOIDED_SUIT] = 0;
        points_table[suit_flag & UINT8_C(0xF1) ? HALF_FLUSH : FULL_FLUSH] = 1;
    }
}

// 检测数牌的范围（大于五、小于五、全大、全中、全小）
static void check_tiles_rank_range(const TILE *tiles, long tile_cnt, long (&points_table)[POINT_TYPE_COUNT]) {
#ifdef STRICT_98_RULE
    if (points_table[SEVEN_PAIRS]) {
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
        points_table[rank_flag & UINT16_C(0x0010) ? LOWER_FOUR : LOWER_TILES] = 1;
        return;
    }
    // 1111 1100 0011 1111
    // 检测是否只包含6789
    if (!(rank_flag & UINT16_C(0xFC3F))) {
        // 包含6为大于五，否则为全大
        points_table[rank_flag & UINT16_C(0x0040) ? UPPER_FOUR : UPPER_TILES] = 1;
        return;
    }
    // 1111 1111 1000 1111
    // 检测是否只包含456
    if (!(rank_flag & UINT16_C(0xFF8F))) {
        // 全中
        points_table[MIDDLE_TILES] = 1;
    }
}

// 判断一组牌是否包含数牌19
static bool is_set_contains_terminal_tile(const SET &set) {
    if (!is_numbered_suit_quick(set.mid_tile)) {
        return false;
    }
    RANK_TYPE rank = tile_rank(set.mid_tile);
    if (set.set_type == SET_TYPE::CHOW) {
        return (rank == 2 || rank == 8);
    }
    else {
        return (rank == 1 || rank == 9);
    }
}

// 判断一组牌是否包含数牌5
static bool is_set_contains_5(const SET &set) {
    if (!is_numbered_suit_quick(set.mid_tile)) {
        return false;
    }
    RANK_TYPE rank = tile_rank(set.mid_tile);
    if (set.set_type == SET_TYPE::CHOW) {
        return (rank >= 4 && rank <= 6);
    }
    else {
        return rank == 5;
    }
}

// 检测全带幺、全带五、全双刻
static void check_tiles_rank_by_set(const SET (&sets)[5], long (&points_table)[POINT_TYPE_COUNT]) {
    // 统计包含数牌19、字牌、5、双数牌的组数
    int terminal_cnt = 0;
    int honor_cnt = 0;
    int _5_cnt = 0;
    int even_pung_cnt = 0;
    for (long i = 0; i < 5; ++i) {
        if (is_set_contains_terminal_tile(sets[i])) {
            ++terminal_cnt;  // 数牌19
        }
        else if (sets[i].set_type != SET_TYPE::CHOW
            && is_honor(sets[i].mid_tile)) {
            ++honor_cnt;  // 字牌
        }
        else if (is_set_contains_5(sets[i])) {
            ++_5_cnt;  // 5
        }
        else if (sets[i].set_type != SET_TYPE::CHOW
            && is_numbered_suit_quick(sets[i].mid_tile) && (sets[i].mid_tile & 1) == 0) {
            ++even_pung_cnt;  // 双数牌刻子
        }
    }

    // 5组牌都包含数牌19和字牌，先暂时计为全带幺
    if (terminal_cnt + honor_cnt == 5) {
        points_table[OUTSIDE_HAND] = 1;
        return;
    }
    // 全带五
    if (_5_cnt == 5) {
        points_table[ALL_FIVE] = 1;
        return;
    }
    // 全双刻
    if (even_pung_cnt == 5) {
        points_table[ALL_EVEN_PUNGS] = 1;
    }
}

// 检测特性（断幺、推不倒、绿一色、字一色、清幺九、混幺九）
static void check_tiles_traits(const TILE *tiles, long tile_cnt, long (&points_table)[POINT_TYPE_COUNT]) {
    // 断幺
    if (!std::any_of(tiles, tiles + tile_cnt, &is_terminal_or_honor)) {
        points_table[ALL_SIMPLES] = 1;
    }

    // 推不倒
    if (std::all_of(tiles, tiles + tile_cnt, &is_reversible_tile)) {
        points_table[REVERSIBLE_TILES] = 1;
    }

#ifdef STRICT_98_RULE
    if (points_table[SEVEN_PAIRS]) {
        return;  // 严格98规则的七对不支持绿一色、清幺九、混幺九
    }
#endif

    // 绿一色
    if (std::all_of(tiles, tiles + tile_cnt, &is_green)) {
        points_table[ALL_GREEN] = 1;
    }

    // 如果断幺了就没必要检测字一色、清幺九、混幺九了
    if (points_table[ALL_SIMPLES] != 0) {
        return;
    }

    // 字一色
    if (std::all_of(tiles, tiles + tile_cnt, &is_honor)) {
        points_table[ALL_HONORS] = 1;
        return;
    }
    // 清幺九
    if (std::all_of(tiles, tiles + tile_cnt, &is_terminal)) {
        points_table[ALL_TERMINALS] = 1;
        return;
    }
    // 混幺九
    if (std::all_of(tiles, tiles + tile_cnt, &is_terminal_or_honor)) {
        points_table[ALL_TERMINALS_AND_HONORS] = 1;
    }
}

// 检测四归一
static void check_tiles_hog(const TILE *tiles, long tile_cnt, long (&points_table)[POINT_TYPE_COUNT]) {
    long kong_cnt = tile_cnt - 14;  // 标准和牌14张，多出几张就说明有几个杠
    int cnt_table[0x54] = { 0 };
    for (long i = 0; i < tile_cnt; ++i) {
        ++cnt_table[tiles[i]];
    }
    // 有多少种已经用去4张的牌减去杠的数量，即为四归一的数量
    long _4_cnt = std::count(std::begin(cnt_table), std::end(cnt_table), 4);
    points_table[TILE_HOG] = _4_cnt - kong_cnt;
}

// 移除和牌
static void remove_win_tile(TILE *standing_tiles, long tile_cnt, TILE win_tile) {
    TILE *it = std::find(standing_tiles, standing_tiles + tile_cnt, win_tile);
    long idx = it - standing_tiles;
    if (idx != tile_cnt) {
        memmove(it, it + 1, (tile_cnt - idx - 1) * sizeof(TILE));
    }
}

// 检测边坎钓
static void check_edge_closed_single_wait(const SET *concealed_sets, long set_cnt, TILE win_tile, long (&points_table)[POINT_TYPE_COUNT]) {
    // 全求人和四杠不计单钓将，也不可能有边张、坎张
    if (points_table[MELDED_HAND] || points_table[FOUR_KONGS]) {
        return;
    }

    // 恢复成一张一张的牌
    TILE standing_tiles[14];
    long tile_cnt;
    recovery_tiles_from_sets(concealed_sets, set_cnt, standing_tiles, &tile_cnt);
    sort_tiles(standing_tiles, tile_cnt);
    remove_win_tile(standing_tiles, tile_cnt, win_tile);
    --tile_cnt;

    bool waiting_table[0x54] = { 0 };
    is_basic_type_wait(standing_tiles, tile_cnt, waiting_table);

    if (set_cnt == 5) {
        // 判断是否为七对听牌
        bool temp_table[0x54] = { 0 };
        if (is_seven_pairs_wait(standing_tiles, tile_cnt, temp_table)) {
            std::transform(std::begin(temp_table), std::end(temp_table), std::begin(waiting_table),
                std::begin(waiting_table), [](bool w, bool t) { return w || t; });
        }
    }

    // 统计听牌张数
    long waiting_cnt = std::count(std::begin(waiting_table), std::end(waiting_table), true);

    // 听牌数大于1张，不计边坎钓
    if (waiting_cnt != 1) {
        return;
    }

    // 听1张的情况，看和牌张处于什么位置
    bool maybe_edge = false;
    bool maybe_closed = false;
    bool maybe_single = false;

    for (long i = 0; i < set_cnt; ++i) {
        switch (concealed_sets[i].set_type) {
        case SET_TYPE::CHOW:
            if (concealed_sets[i].mid_tile == win_tile) {
                maybe_closed = true;  // 坎张
            }
            else if (concealed_sets[i].mid_tile + 1 == win_tile
                || concealed_sets[i].mid_tile - 1 == win_tile) {
                maybe_edge = true;  // 边张
            }
            break;
        case SET_TYPE::PAIR:
            if (concealed_sets[i].mid_tile == win_tile) {
                maybe_single = true;  // 单钓
            }
            break;
        default:
            break;
        }
    }

    // 当多种可能存在时，只能计其中一种
    if (maybe_edge) {
        points_table[EDGE_WAIT] = 1;
        return;
    }
    if (maybe_closed) {
        points_table[CLOSED_WAIT] = 1;
        return;
    }
    if (maybe_single) {
        points_table[SINGLE_WAIT] = 1;
        return;
    }
}

// 检测圈风刻、门风刻
static void check_wind_pungs(const SET &sets, WIND_TYPE prevalent_wind, WIND_TYPE seat_wind, long (&points_table)[POINT_TYPE_COUNT]) {
    if (sets.set_type == SET_TYPE::PUNG || sets.set_type == SET_TYPE::KONG) {
        RANK_TYPE delta = sets.mid_tile - 0x41;
        if (delta == (int)prevalent_wind - (int)WIND_TYPE::EAST) {
            points_table[PREVALENT_WIND] = 1;
        }
        if (delta == (int)seat_wind - (int)WIND_TYPE::EAST) {
            points_table[SEAT_WIND] = 1;
        }
    }
}

// 统一校正一些不计的
static void correction_points_table(long (&points_table)[POINT_TYPE_COUNT], bool prevalent_eq_seat) {
    // 大四喜不计三风刻、碰碰和、圈风刻、门风刻、幺九刻
    if (points_table[BIG_FOUR_WINDS]) {
        points_table[BIG_THREE_WINDS] = 0;
        points_table[ALL_PUNGS] = 0;
        points_table[PREVALENT_WIND] = 0;
        points_table[SEAT_WIND] = 0;
        points_table[PUNG_OF_TERMINALS_OR_HONORS] = 0;
    }
    // 大三元不计双箭刻、箭刻（严格98规则不计缺一门）
    if (points_table[BIG_THREE_DRAGONS]) {
        points_table[TWO_DRAGONS_PUNGS] = 0;
        points_table[DRAGON_PUNG] = 0;
#ifdef STRICT_98_RULE
        points_table[ONE_VOIDED_SUIT] = 0;
#endif
    }
    // 绿一色不计混一色、缺一门
    if (points_table[ALL_GREEN]) {
        points_table[HALF_FLUSH] = 0;
        points_table[ONE_VOIDED_SUIT] = 0;
    }
    // 九莲宝灯不计清一色、门前清、缺一门、无字，减计1个幺九刻，把不求人修正为自摸
    if (points_table[NINE_GATES]) {
        points_table[FULL_FLUSH] = 0;
        points_table[CONCEALED_HAND] = 0;
        --points_table[PUNG_OF_TERMINALS_OR_HONORS];
        points_table[ONE_VOIDED_SUIT] = 0;
        points_table[NO_HONORS] = 0;
        if (points_table[FULLY_CONCEALED_HAND]) {
            points_table[FULLY_CONCEALED_HAND] = 0;
            points_table[SELF_DRAWN] = 1;
        }
    }
    // 四杠不计单钓将
    if (points_table[FOUR_KONGS]) {
        points_table[SINGLE_WAIT] = 0;
    }
    // 连七对不计七对、清一色、门前清、缺一门、无字
    if (points_table[SEVEN_SHIFTED_PAIRS]) {
        points_table[SEVEN_PAIRS] = 0;
        points_table[FULL_FLUSH] = 0;
        points_table[CONCEALED_HAND] = 0;
        points_table[ONE_VOIDED_SUIT] = 0;
        points_table[NO_HONORS] = 0;
    }
    // 十三幺不计五门齐、门前清、单钓将
    if (points_table[THIRTEEN_ORPHANS]) {
        points_table[ALL_TYPES] = 0;
        points_table[CONCEALED_HAND] = 0;
        points_table[SINGLE_WAIT] = 0;
    }

    // 清幺九不计混幺九、碰碰胡、全带幺、幺九刻、无字（严格98规则不计双同刻、不计三同刻）
    if (points_table[ALL_TERMINALS]) {
        points_table[ALL_TERMINALS_AND_HONORS] = 0;
        points_table[ALL_PUNGS] = 0;
        points_table[OUTSIDE_HAND] = 0;
        points_table[PUNG_OF_TERMINALS_OR_HONORS] = 0;
        points_table[NO_HONORS] = 0;
        points_table[DOUBLE_PUNG] = 0;  // 通行计法不计双同刻
#ifdef STRICT_98_RULE
        points_table[TRIPLE_PUNG] = 0;
        points_table[DOUBLE_PUNG] = 0;
#endif
    }

    // 小四喜不计三风刻
    if (points_table[LITTLE_FOUR_WINDS]) {
        points_table[BIG_THREE_WINDS] = 0;
        // 小四喜的第四组牌如果是19的刻子，则是混幺九；如果是箭刻则是字一色；这两种都是不计幺九刻的
        // 如果是顺子或者2-8的刻子，则不存在多余的幺九刻
        // 所以这里将幺九刻置为0
        points_table[PUNG_OF_TERMINALS_OR_HONORS] = 0;
    }

    // 小三元不计双箭刻、箭刻（严格98规则不计缺一门）
    if (points_table[LITTLE_THREE_DRAGONS]) {
        points_table[TWO_DRAGONS_PUNGS] = 0;
        points_table[DRAGON_PUNG] = 0;
#ifdef STRICT_98_RULE
        points_table[ONE_VOIDED_SUIT] = 0;
#endif
    }

    // 字一色不计混幺九、碰碰胡、全带幺、幺九刻、缺一门
    if (points_table[ALL_HONORS]) {
        points_table[ALL_TERMINALS_AND_HONORS] = 0;
        points_table[ALL_PUNGS] = 0;
        points_table[OUTSIDE_HAND] = 0;
        points_table[PUNG_OF_TERMINALS_OR_HONORS] = 0;
        points_table[ONE_VOIDED_SUIT] = 0;
    }
    // 四暗刻不计碰碰和、门前清，把不求人修正为自摸
    if (points_table[FOUR_CONCEALED_PUNGS]) {
        points_table[ALL_PUNGS] = 0;
        points_table[CONCEALED_HAND] = 0;
        if (points_table[FULLY_CONCEALED_HAND]) {
            points_table[FULLY_CONCEALED_HAND] = 0;
            points_table[SELF_DRAWN] = 1;
        }
    }
    // 一色双龙会不计七对、清一色、平和、一般高、老少副、缺一门、无字
    if (points_table[PURE_TERMINAL_CHOWS]) {
        points_table[SEVEN_PAIRS] = 0;
        points_table[FULL_FLUSH] = 0;
        points_table[ALL_CHOWS] = 0;
        points_table[PURE_DOUBLE_CHOW] = 0;
        points_table[TWO_TERMINAL_CHOWS] = 0;
        points_table[ONE_VOIDED_SUIT] = 0;
        points_table[NO_HONORS] = 0;
    }

    // 一色四同顺不计一色三同顺、一般高、四归一（严格98规则不计缺一门）
    if (points_table[QUADRUPLE_CHOW]) {
        points_table[PURE_SHIFTED_PUNGS] = 0;
        points_table[TILE_HOG] = 0;
        points_table[PURE_DOUBLE_CHOW] = 0;
#ifdef STRICT_98_RULE
        points_table[ONE_VOIDED_SUIT] = 0;
#endif
    }
    // 一色四节高不计一色三节高、碰碰和（严格98规则不计缺一门）
    if (points_table[FOUR_PURE_SHIFTED_PUNGS]) {
        points_table[PURE_TRIPLE_CHOW] = 0;
        points_table[ALL_PUNGS] = 0;
#ifdef STRICT_98_RULE
        points_table[ONE_VOIDED_SUIT] = 0;
#endif
    }

    // 一色四步高不计一色三步高、老少副、连六（严格98规则不计缺一门）
    if (points_table[FOUR_PURE_SHIFTED_CHOWS]) {
        points_table[PURE_SHIFTED_CHOWS] = 0;
        points_table[TWO_TERMINAL_CHOWS] = 0;
        points_table[SHORT_STRAIGHT] = 0;
#ifdef STRICT_98_RULE
        points_table[ONE_VOIDED_SUIT] = 0;
#endif
    }

    // 混幺九不计碰碰和、全带幺、幺九刻
    if (points_table[ALL_TERMINALS_AND_HONORS]) {
        points_table[ALL_PUNGS] = 0;
        points_table[OUTSIDE_HAND] = 0;
        points_table[PUNG_OF_TERMINALS_OR_HONORS] = 0;
    }

    // 七对不计门前清、单钓将
    if (points_table[SEVEN_PAIRS]) {
        points_table[CONCEALED_HAND] = 0;
        points_table[SINGLE_WAIT] = 0;
    }
    // 七星不靠不计五门齐、门前清
    if (points_table[GREATER_HONORS_AND_KNITTED_TILES]) {
        points_table[ALL_TYPES] = 0;
        points_table[CONCEALED_HAND] = 0;
    }
    // 全双刻不计碰碰胡、断幺、无字
    if (points_table[ALL_EVEN_PUNGS]) {
        points_table[ALL_PUNGS] = 0;
        points_table[ALL_SIMPLES] = 0;
        points_table[NO_HONORS] = 0;
    }
    // 清一色不计缺一门、无字
    if (points_table[FULL_FLUSH]) {
        points_table[ONE_VOIDED_SUIT] = 0;
        points_table[NO_HONORS] = 0;
    }
    // 一色三同顺不计一色三节高、一般高
    if (points_table[PURE_TRIPLE_CHOW]) {
        points_table[PURE_SHIFTED_PUNGS] = 0;
        points_table[PURE_DOUBLE_CHOW] = 0;
    }
    // 一色三节高不计一色三同顺
    if (points_table[PURE_SHIFTED_PUNGS]) {
        points_table[PURE_TRIPLE_CHOW] = 0;
    }
    // 全大不计大于五、无字
    if (points_table[UPPER_TILES]) {
        points_table[UPPER_FOUR] = 0;
        points_table[NO_HONORS] = 0;
    }
    // 全中不计断幺
    if (points_table[MIDDLE_TILES]) {
        points_table[ALL_SIMPLES] = 0;
        points_table[NO_HONORS] = 0;
    }
    // 全小不计小于五、无字
    if (points_table[LOWER_TILES]) {
        points_table[LOWER_FOUR] = 0;
        points_table[NO_HONORS] = 0;
    }

    // 三色双龙会不计平和、无字、喜相逢、老少副
    if (points_table[THREE_SUITED_TERMINAL_CHOWS]) {
        points_table[ALL_CHOWS] = 0;
        points_table[NO_HONORS] = 0;
        points_table[MIXED_DOUBLE_CHOW] = 0;
        points_table[TWO_TERMINAL_CHOWS] = 0;
    }
    // 全带五不计断幺、无字
    if (points_table[ALL_FIVE]) {
        points_table[ALL_SIMPLES] = 0;
        points_table[NO_HONORS] = 0;
    }

    // 七星不靠不计五门齐、门前清
    if (points_table[LESSER_HONORS_AND_KNITTED_TILES]) {
        points_table[ALL_TYPES] = 0;
        points_table[CONCEALED_HAND] = 0;
    }
    // 大于五不计无字
    if (points_table[UPPER_FOUR]) {
        points_table[NO_HONORS] = 0;
    }
    // 小于五不计无字
    if (points_table[LOWER_FOUR]) {
        points_table[NO_HONORS] = 0;
    }
    // 三风刻内部不再计幺九刻（严格98规则不计缺一门）
    if (points_table[BIG_THREE_WINDS]) {
        // 如果不是字一色或混幺九，则要减去3个幺九刻
        if (!points_table[ALL_HONORS] && !points_table[ALL_TERMINALS_AND_HONORS]) {
            assert(points_table[PUNG_OF_TERMINALS_OR_HONORS] >= 3);
            points_table[PUNG_OF_TERMINALS_OR_HONORS] -= 3;
        }
#ifdef STRICT_98_RULE
        points_table[ONE_VOIDED_SUIT] = 0;
#endif
    }

    // 推不倒不计缺一门
    if (points_table[REVERSIBLE_TILES]) {
        points_table[ONE_VOIDED_SUIT] = 0;
    }
    // 妙手回春不计自摸
    if (points_table[LAST_TILE_DRAW]) {
        points_table[SELF_DRAWN] = 0;
    }
    // 杠上开花不计自摸
    if (points_table[OUT_WITH_REPLACEMENT_TILE]) {
        points_table[SELF_DRAWN] = 0;
    }
    // 抢杠和不计和绝张
    if (points_table[ROBBING_THE_KONG]) {
        points_table[LAST_TILE] = 0;
    }
    // 双暗杠不计暗杠
    if (points_table[TWO_CONCEALED_KONGS]) {
        points_table[CONCEALED_KONG] = 0;
    }

    // 混一色不计缺一门
    if (points_table[HALF_FLUSH]) {
        points_table[ONE_VOIDED_SUIT] = 0;
    }
    // 全求人不计单钓将
    if (points_table[MELDED_HAND]) {
        points_table[SINGLE_WAIT] = 0;
    }
    // 双箭刻不计箭刻
    if (points_table[TWO_DRAGONS_PUNGS]) {
        points_table[DRAGON_PUNG] = 0;
    }

    // 不求人不计自摸
    if (points_table[FULLY_CONCEALED_HAND]) {
        points_table[SELF_DRAWN] = 0;
    }
    // 双明杠不计明杠
    if (points_table[TWO_MELDED_KONGS]) {
        points_table[MELDED_KONG] = 0;
    }

    // 圈风刻自己不计幺九刻
    if (points_table[PREVALENT_WIND]) {
        // 如果不是三风刻、小四喜、字一色、混幺九，则要减去1个幺九刻
        if (!points_table[BIG_THREE_WINDS] && !points_table[LITTLE_FOUR_WINDS]
            && !points_table[ALL_HONORS] && !points_table[ALL_TERMINALS_AND_HONORS]) {
            assert(points_table[PUNG_OF_TERMINALS_OR_HONORS] > 0);
            --points_table[PUNG_OF_TERMINALS_OR_HONORS];
        }
    }
    // 门风刻自己不计幺九刻
    if (points_table[SEAT_WIND]) {
        // 如果圈风与门风不相同，并且不是三风刻、小四喜、字一色、混幺九，则要减去1个幺九刻
        if (!prevalent_eq_seat && !points_table[BIG_THREE_WINDS] && !points_table[LITTLE_FOUR_WINDS]
            && !points_table[ALL_HONORS] && !points_table[ALL_TERMINALS_AND_HONORS]) {
            assert(points_table[PUNG_OF_TERMINALS_OR_HONORS] > 0);
            --points_table[PUNG_OF_TERMINALS_OR_HONORS];
        }
    }
    // 平和不计无字
    if (points_table[ALL_CHOWS]) {
        points_table[NO_HONORS] = 0;
    }
    // 断幺不计无字
    if (points_table[ALL_SIMPLES]) {
        points_table[NO_HONORS] = 0;
    }
}

// 检测和绝张、妙手回春、海底捞月、自摸
static void check_win_type(WIN_TYPE win_type, long (&points_table)[POINT_TYPE_COUNT]) {
    if (win_type & WIN_TYPE_4TH_TILE) {
        points_table[LAST_TILE] = 1;
    }
    if (win_type & WIN_TYPE_WALL_LAST) {
        points_table[win_type & WIN_TYPE_SELF_DRAWN ? LAST_TILE_DRAW : LAST_TILE_CLAIM] = 1;
    }
    if (win_type & WIN_TYPE_ABOUT_KONG) {
        points_table[win_type & WIN_TYPE_SELF_DRAWN ? OUT_WITH_REPLACEMENT_TILE : ROBBING_THE_KONG] = 1;
    }
    if (win_type & WIN_TYPE_SELF_DRAWN) {
        points_table[SELF_DRAWN] = 1;
    }
}

static bool is_win_tile_in_concealed_chow_sets(const SET *chow_sets, long chow_cnt, TILE win_tile) {
    return std::any_of(chow_sets, chow_sets + chow_cnt, [win_tile](const SET &chow_set) {
        return !chow_set.is_melded
            && (chow_set.mid_tile - 1 == win_tile || chow_set.mid_tile == win_tile || chow_set.mid_tile + 1 == win_tile);
    });
}

// 基本和型算番
static void calculate_basic_type_points(const SET (&sets)[5], long fixed_cnt, TILE win_tile,
    const EXTRA_CONDITION *ext_cond, long (&points_table)[POINT_TYPE_COUNT]) {
    SET pair_set;
    SET chow_sets[4];
    SET pung_sets[4];
    long chow_cnt = 0;
    long pung_cnt = 0;
    for (long i = 0; i < 5; ++i) {
        switch (sets[i].set_type) {
        case SET_TYPE::CHOW: chow_sets[chow_cnt++] = sets[i]; break;
        case SET_TYPE::PUNG:
        case SET_TYPE::KONG: pung_sets[pung_cnt++] = sets[i]; break;
        case SET_TYPE::PAIR: pair_set = sets[i]; break;
        default: assert(0); return;
        }
    }

    TILE tiles[18];
    long tile_cnt = 0;
    recovery_tiles_from_sets(sets, 5, tiles, &tile_cnt);

    // 九莲宝灯
    if (fixed_cnt == 0 && tile_cnt == 14) {
        if (is_nine_gates((const TILE (&)[14])tiles, win_tile)) {
            points_table[NINE_GATES] = 1;
        }
    }

    check_win_type(ext_cond->win_type, points_table);

    // 点和的牌张，如果不能解释为暗顺中的一张，那么将其解释为刻子，并标记这个刻子为明刻
    if ((ext_cond->win_type & WIN_TYPE_SELF_DRAWN) == 0) {
        if (!is_win_tile_in_concealed_chow_sets(chow_sets, chow_cnt, win_tile)) {
            for (long i = 0; i < pung_cnt; ++i) {
                if (pung_sets[i].mid_tile == win_tile && !pung_sets[i].is_melded) {
                    pung_sets[i].is_melded = true;
                }
            }
        }
    }

    switch (chow_cnt) {
    case 4:  // 4组顺子
        // 检测三色/一色双龙会
        if (is_three_suited_terminal_chows(chow_sets, pair_set)) {
            points_table[THREE_SUITED_TERMINAL_CHOWS] = 1;
            break;
        }
        if (is_pure_terminal_chows(chow_sets, pair_set)) {
            points_table[PURE_TERMINAL_CHOWS] = 1;
            break;
        }
        calculate_4_chows(chow_sets, points_table);
        break;
    case 3:  // 3组顺子+1组刻子
        calculate_3_chows(chow_sets, points_table);
        calculate_1_pung(pung_sets[0], points_table);
        break;
    case 2:  // 2组顺子+2组刻子
        calculate_2_chows(chow_sets, points_table);
        calculate_2_pungs(pung_sets, points_table);
        break;
    case 1:  // 1组顺子+3组刻子
        calculate_3_pungs(pung_sets, points_table);
        break;
    case 0:  // 4组刻子
        calculate_4_pungs(pung_sets, points_table);
        break;
    default:
        break;
    }

    // 检测不求人、全求人
    check_melded_or_concealed_hand(sets, fixed_cnt, ext_cond->win_type & WIN_TYPE_SELF_DRAWN, points_table);
    // 检测将，确定平和、小三元、小四喜
    check_pair_tile(pair_set.mid_tile, chow_cnt, points_table);
    // 检测全带幺、全带五、全双刻
    check_tiles_rank_by_set(sets, points_table);

    // 检测门（五门齐的门）
    check_tiles_suits(tiles, tile_cnt, points_table);
    // 检测特性（断幺、推不倒、绿一色、字一色、清幺九、混幺九）
    check_tiles_traits(tiles, tile_cnt, points_table);
    // 检测数牌的范围（大于五、小于五、全大、全中、全小）
    check_tiles_rank_range(tiles, tile_cnt, points_table);
    // 检测四归一
    check_tiles_hog(tiles, tile_cnt, points_table);
    // 检测边坎钓
    check_edge_closed_single_wait(sets + fixed_cnt, 5 - fixed_cnt, win_tile, points_table);

    // 检测圈风刻、门风刻
    for (int i = 0; i < 5; ++i) {
        check_wind_pungs(sets[i], ext_cond->prevalent_wind, ext_cond->seat_wind, points_table);
    }

    // 统一校正一些不计的
    correction_points_table(points_table, ext_cond->prevalent_wind == ext_cond->seat_wind);

    // 如果什么番都没有，则计为无番和
    if (std::all_of(std::begin(points_table), std::end(points_table), [](long p) { return p == 0; })) {
        points_table[CHICKEN_HAND] = 1;
    }
}

// “组合龙+面子+将”和型算番
static bool calculate_knitted_straight_in_basic_type_points(const HAND_TILES *hand_tiles,
    TILE win_tile, const EXTRA_CONDITION *ext_cond, long (&points_table)[POINT_TYPE_COUNT]) {
    long fixed_cnt = hand_tiles->set_count;
    if (fixed_cnt > 1) {
        return false;
    }

    const SET *fixed_sets = hand_tiles->fixed_sets;
    long standing_cnt = hand_tiles->tile_count;

    // 对立牌和和牌的种类进行打表
    int cnt_table[0x54];
    map_tiles(hand_tiles->standing_tiles, standing_cnt, cnt_table);
    ++cnt_table[win_tile];

    // 匹配组合龙
    const TILE (*matched_seq)[9] = std::find_if(&standard_knitted_straight[0], &standard_knitted_straight[6],
        [&cnt_table](const TILE (&seq)[9]) {
        return std::all_of(std::begin(seq), std::end(seq), [&cnt_table](TILE t) { return cnt_table[t] > 0; });
    });

    if (matched_seq == &standard_knitted_straight[6]) {
        return false;
    }

    // 剔除组合龙
    std::for_each(std::begin(*matched_seq), std::end(*matched_seq), [&cnt_table](TILE t) { --cnt_table[t]; });

    // 按基本和型划分
    SEPERATIONS separation;
    separation.count = 0;
    SET work_sets[5];
    memset(work_sets, 0, sizeof(work_sets));
    if (fixed_cnt == 1) {
        work_sets[3] = fixed_sets[0];  // 无关第4组是明副露，将其放在3处，这样就与门清状态的划分统一了
    }
    seperate_recursively(cnt_table, fixed_cnt + 3, 0, work_sets, &separation);
    if (separation.count != 1) {
        return false;
    }

    SET *temp_set = separation.sets[0];

    points_table[KNITTED_STRAIGHT] = 1;  // 组合龙
    if (temp_set[3].set_type == SET_TYPE::CHOW) {
        if (is_numbered_suit_quick(temp_set[4].mid_tile)) {
            points_table[ALL_CHOWS] = 1;  // 第4组是顺子，将是数牌时，为平和
        }
    }
    else {
        calculate_1_pung(temp_set[3], points_table);
    }

    check_win_type(ext_cond->win_type, points_table);
    // 门前清（暗杠不影响）
    if (fixed_cnt == 0 || (temp_set[3].set_type == SET_TYPE::KONG && !temp_set[3].is_melded)) {
        if (ext_cond->win_type & WIN_TYPE_SELF_DRAWN) {
            points_table[FULLY_CONCEALED_HAND] = 1;
        }
        else {
            points_table[CONCEALED_HAND] = 1;
        }
    }

    // 还原牌
    TILE tiles[15];  // 第四组可能为杠，所以最多为15张
    memcpy(tiles, matched_seq, 9 * sizeof(TILE));  // 组合龙的部分
    long tile_cnt;
    recovery_tiles_from_sets(&temp_set[3], 2, tiles + 9, &tile_cnt);  // 一组面子和一对将
    tile_cnt += 9;

    // 检测门（五门齐的门）
    check_tiles_suits(tiles, tile_cnt, points_table);
    // 特性和数牌的范围不用检测了，绝对不可能是有断幺、推不倒、绿一色、字一色、清幺九、混幺九
    // 检测四归一
    check_tiles_hog(tiles, tile_cnt, points_table);

    // 和牌张是组合龙范围的牌，不计边坎钓
    if (std::none_of(std::begin(*matched_seq), std::end(*matched_seq), [win_tile](TILE t) { return t == win_tile; })) {
        if (fixed_cnt == 0) {
            check_edge_closed_single_wait(temp_set + 3, 2, win_tile, points_table);
        }
        else {
            points_table[SINGLE_WAIT] = 1;
        }
    }

    // 检测圈风刻、门风刻
    check_wind_pungs(temp_set[3], ext_cond->prevalent_wind, ext_cond->seat_wind, points_table);
    // 统一校正一些不计的
    correction_points_table(points_table, ext_cond->prevalent_wind == ext_cond->seat_wind);
    return true;
}

// 十三幺
static bool is_thirteen_orphans(const TILE (&tiles)[14]) {
    return std::all_of(std::begin(tiles), std::end(tiles), &is_terminal_or_honor)
        && std::includes(std::begin(tiles), std::end(tiles),
        std::begin(standard_thirteen_orphans), std::end(standard_thirteen_orphans));
}

// 全不靠/七星不靠算番
bool caculate_honors_and_knitted_tiles(const TILE (&standing_tiles)[14], long (&points_table)[POINT_TYPE_COUNT]) {
    const TILE *it = std::find_if(std::begin(standing_tiles), std::end(standing_tiles), &is_honor);
    long numbered_cnt = it - standing_tiles;
    // 序数牌张数大于9或者小于7必然不可能是全不靠
    if (numbered_cnt > 9 || numbered_cnt < 7) {
        return false;
    }

    // 匹配组合龙
    if (std::none_of(&standard_knitted_straight[0], &standard_knitted_straight[6],
        [&standing_tiles, it](const TILE (&seq)[9]) {
        return std::includes(std::begin(seq), std::end(seq), std::begin(standing_tiles), it);
    })) {
        return false;
    }

    static const TILE seven_honors[] = { 0x41, 0x42, 0x43, 0x44, 0x51, 0x52, 0x53 };
    if (numbered_cnt == 7 && std::equal(std::begin(seven_honors), std::end(seven_honors), standing_tiles + 7)) {
        // 七种字牌齐，为七星不靠
        points_table[GREATER_HONORS_AND_KNITTED_TILES] = 1;
        return true;
    }
    else if (std::includes(std::begin(seven_honors), std::end(seven_honors), it, std::end(standing_tiles))) {
        // 全不靠
        points_table[LESSER_HONORS_AND_KNITTED_TILES] = 1;
        if (numbered_cnt == 9) {  // 有9张数牌，为带组合龙的全不靠
            points_table[KNITTED_STRAIGHT] = 1;
        }
        return true;
    }

    return false;
}

// 特殊和型算番
static bool calculate_special_type_points(const TILE (&standing_tiles)[14], WIN_TYPE win_type, long (&points_table)[POINT_TYPE_COUNT]) {
    // 七对
    if (standing_tiles[0] == standing_tiles[1]
        && standing_tiles[2] == standing_tiles[3]
        && standing_tiles[4] == standing_tiles[5]
        && standing_tiles[6] == standing_tiles[7]
        && standing_tiles[8] == standing_tiles[9]
        && standing_tiles[10] == standing_tiles[11]
        && standing_tiles[12] == standing_tiles[13]) {

        if (standing_tiles[0] + 1 == standing_tiles[2]
            && standing_tiles[2] + 1 == standing_tiles[4]
            && standing_tiles[4] + 1 == standing_tiles[6]
            && standing_tiles[6] + 1 == standing_tiles[8]
            && standing_tiles[8] + 1 == standing_tiles[10]
            && standing_tiles[10] + 1 == standing_tiles[12]) {
            // 连七对
            points_table[SEVEN_SHIFTED_PAIRS] = 1;
            check_tiles_traits(standing_tiles, 14, points_table);
        }
        else {
            // 普通七对
            points_table[SEVEN_PAIRS] = 1;

            // 检测门（五门齐的门）
            check_tiles_suits(standing_tiles, 14, points_table);
            // 检测特性（断幺、推不倒、绿一色、字一色、清幺九、混幺九）
            check_tiles_traits(standing_tiles, 14, points_table);
            // 检测数牌的范围（大于五、小于五、全大、全中、全小）
            check_tiles_rank_range(standing_tiles, 14, points_table);
            // 检测四归一
            check_tiles_hog(standing_tiles, 14, points_table);
        }
    }
    // 十三幺
    else if (is_thirteen_orphans(standing_tiles)) {
        points_table[THIRTEEN_ORPHANS] = 1;
    }
    // 全不靠/七星不靠
    else if (caculate_honors_and_knitted_tiles(standing_tiles, points_table)) {
    }
    else {
        return false;
    }

    check_win_type(win_type, points_table);
    // 圈风刻、门风刻没必要检测了，这些特殊和型都没有面子
    // 统一校正一些不计的
    correction_points_table(points_table, false);
    return true;
}

static int get_points_by_table(const long (&points_table)[POINT_TYPE_COUNT]) {
    int points = 0;
    for (int i = 1; i < FLOWER_TILES; ++i) {
        if (points_table[i] == 0) {
            continue;
        }
        points += points_value_table[i] * points_table[i];
        if (points_table[i] == 1) {
            LOG("%s %d\n", points_name[i], points_value_table[i]);
        }
        else {
            LOG("%s %d*%ld\n", points_name[i], points_value_table[i], points_table[i]);
        }
    }
    return points;
}

static long parse_tiles_impl(const char *str, TILE *tiles, long max_cnt, long *out_tile_cnt) {
    //if (strspn(str, "123456789mpsESWNCFP") != strlen(str)) {
    //    return PARSE_ERROR_ILLEGAL_CHARACTER;
    //}

    long tile_cnt = 0;

#define SET_SUIT_FOR_NUMBERED(value_)       \
    for (long i = tile_cnt; i > 0;) {       \
        if (tiles[--i] & 0xF0) break;       \
        tiles[i] |= value_;                 \
        } (void)0

    const char *p = str;
    for (; tile_cnt < max_cnt && *p != '\0'; ++p) {
        char c = *p;
        switch (c) {
        case '1': tiles[tile_cnt++] = 1; break;
        case '2': tiles[tile_cnt++] = 2; break;
        case '3': tiles[tile_cnt++] = 3; break;
        case '4': tiles[tile_cnt++] = 4; break;
        case '5': tiles[tile_cnt++] = 5; break;
        case '6': tiles[tile_cnt++] = 6; break;
        case '7': tiles[tile_cnt++] = 7; break;
        case '8': tiles[tile_cnt++] = 8; break;
        case '9': tiles[tile_cnt++] = 9; break;
        case 'm': SET_SUIT_FOR_NUMBERED(0x10); break;
        case 's': SET_SUIT_FOR_NUMBERED(0x20); break;
        case 'p': SET_SUIT_FOR_NUMBERED(0x30); break;
        case 'E': tiles[tile_cnt++] = 0x41; break;
        case 'S': tiles[tile_cnt++] = 0x42; break;
        case 'W': tiles[tile_cnt++] = 0x43; break;
        case 'N': tiles[tile_cnt++] = 0x44; break;
        case 'C': tiles[tile_cnt++] = 0x51; break;
        case 'F': tiles[tile_cnt++] = 0x52; break;
        case 'P': tiles[tile_cnt++] = 0x53; break;
        default: goto parse_finish;
        }
    }

parse_finish:
    if (tile_cnt > 0 && !(tiles[tile_cnt - 1] & 0xF0)) {
        bool hasSuffix = false;
        for (; !hasSuffix && *p != '\0'; ++p) {
            char c = *p;
            switch (c) {
            case 'm': SET_SUIT_FOR_NUMBERED(0x10); hasSuffix = true; break;
            case 's': SET_SUIT_FOR_NUMBERED(0x20); hasSuffix = true; break;
            case 'p': SET_SUIT_FOR_NUMBERED(0x30); hasSuffix = true; break;
            default: break;
            }
        }

        if (!hasSuffix) {
            return PARSE_ERROR_NO_SUFFIX_AFTER_DIGIT;
        }
    }

#undef SET_SUIT_FOR_NUMBERED

    if (out_tile_cnt != nullptr) {
        *out_tile_cnt = tile_cnt;
    }
    return (p - str);
}

long parse_tiles(const char *str, TILE *tiles, long max_cnt) {
    long tile_cnt;
    if (parse_tiles_impl(str, tiles, max_cnt, &tile_cnt) > 0) {
        return tile_cnt;
    }
    return 0;
}

static long make_fixed_set(const TILE *tiles, long tile_cnt, SET *set) {
    if (tile_cnt > 0) {
        if (tile_cnt != 3 && tile_cnt != 4) {
            return PARSE_ERROR_TOO_MANY_TILES_FOR_FIXED_SET;
        }
        if (tile_cnt == 3) {
            if (tiles[0] == tiles[1] && tiles[1] == tiles[2]) {
                set->mid_tile = tiles[1];
                set->set_type = SET_TYPE::PUNG;
                set->is_melded = true;
            }
            else {
                TILE temp[3] = { tiles[0], tiles[1], tiles[2] };
                std::sort(std::begin(temp), std::end(temp));
                if (temp[0] + 1 == temp[1] && temp[1] + 1 == temp[2]) {
                    set->mid_tile = temp[1];
                    set->set_type = SET_TYPE::CHOW;
                    set->is_melded = true;
                }
                else {
                    return PARSE_ERROR_CANNOT_MAKE_FIXED_SET;
                }
            }
        }
        else {
            if (tiles[0] != tiles[1] || tiles[1] != tiles[2] || tiles[2] != tiles[3]) {
                return PARSE_ERROR_CANNOT_MAKE_FIXED_SET;
            }
            set->mid_tile = tiles[0];
            set->set_type = SET_TYPE::KONG;
            set->is_melded = true;
        }
        set->is_melded = true;
        return 1;
    }
    return 0;
}

long string_to_tiles(const char *str, HAND_TILES *hand_tiles) {
    SET sets[4];
    long set_cnt = 0;
    bool is_concealed_kong = false;
    TILE tiles[13];
    long tile_cnt = 0;

    const char *p = str;
    while (char c = *p) {
        const char *q;
        switch (c) {
        case ' ': {
            if (set_cnt > 4) {
                return PARSE_ERROR_TOO_MANY_FIXED_SET;
            }
            long ret = make_fixed_set(tiles, tile_cnt, &sets[set_cnt]);
            if (ret < 0) {
                return ret;
            }
            set_cnt += ret;
            q = ++p;
            tile_cnt = 0;
            break;
        }
        case '[': {
            if (set_cnt > 4) {
                return PARSE_ERROR_TOO_MANY_FIXED_SET;
            }
            long ret = make_fixed_set(tiles, tile_cnt, &sets[set_cnt]);
            if (ret < 0) {
                return ret;
            }
            set_cnt += ret;
            q = ++p;
            is_concealed_kong = true;
            break;
        }
        case ']':
            if (!is_concealed_kong) {
                return PARSE_ERROR_ILLEGAL_CHARACTER;
            }
            if (tile_cnt != 4) {
                return PARSE_ERROR_TOO_MANY_TILES_FOR_FIXED_SET;
            }
            q = ++p;
            sets[set_cnt].mid_tile = tiles[0];
            sets[set_cnt].set_type = SET_TYPE::KONG;
            sets[set_cnt].is_melded = false;
            is_concealed_kong = false;
            ++set_cnt;
            tile_cnt = 0;
            break;
        default: {
                long ret = parse_tiles_impl(p, tiles, 13, &tile_cnt);
                if (ret <= 0) {
                    return ret;
                }
                q = p + ret;
            }
            break;
        }
        p = q;
    }

    //for (long i = 0; i < set_cnt; ++i) {
    //    printf("[%d %s %x] ", sets[i].is_melded, set_type_name[(int)sets[i].set_type], sets[i].mid_tile);
    //}
    //for (long i = 0; i < tile_cnt; ++i) {
    //    printf("%x ", tiles[i]);
    //}
    //puts("");
    memcpy(hand_tiles->fixed_sets, sets, set_cnt * sizeof(SET));
    hand_tiles->set_count = set_cnt;
    memcpy(hand_tiles->standing_tiles, tiles, tile_cnt * sizeof(TILE));
    hand_tiles->tile_count = tile_cnt;

    return PARSE_NO_ERROR;
}

bool is_standing_tiles_contains_win_tile(const TILE *standing_tiles, long standing_cnt, TILE win_tile) {
    return std::any_of(standing_tiles, standing_tiles + standing_cnt,
        [win_tile](TILE tile) { return tile == win_tile; });
}

size_t count_win_tile_in_fixed_sets(const SET *fixed_set, long fixed_cnt, TILE win_tile) {
    size_t cnt = 0;
    for (long i = 0; i < fixed_cnt; ++i) {
        const SET &set = fixed_set[i];
        switch (set.set_type) {
        case mahjong::SET_TYPE::CHOW:
            if (win_tile == set.mid_tile - 1
                || win_tile == set.mid_tile
                || win_tile == set.mid_tile + 1) {
                ++cnt;
            }
            break;
        case mahjong::SET_TYPE::PUNG:
            if (win_tile == set.mid_tile) {
                cnt += 3;
            }
            break;
        case mahjong::SET_TYPE::KONG:
            if (win_tile == set.mid_tile) {
                cnt += 4;
            }
            break;
        default:
            break;
        }
    }
    return cnt;
}

int check_calculator_input(const HAND_TILES *hand_tiles, TILE win_tile) {
    // 打表
    int cnt_table[0x54];
    if (!map_hand_tiles(hand_tiles, cnt_table)) {
        return ERROR_WRONG_TILES_COUNT;
    }
    ++cnt_table[win_tile];

    // 如果某张牌超过4
    if (std::any_of(std::begin(cnt_table), std::end(cnt_table), [](int cnt) { return cnt > 4; })) {
        return ERROR_TILE_COUNT_GREATER_THAN_4;
    }

    return 0;
}

int calculate_points(const HAND_TILES *hand_tiles, TILE win_tile, const EXTRA_CONDITION *ext_cond, long (&points_table)[POINT_TYPE_COUNT]) {
    if (int ret = check_calculator_input(hand_tiles, win_tile)) {
        return ret;
    }

    long fixed_cnt = hand_tiles->set_count;
    long standing_cnt = hand_tiles->tile_count;

    TILE standing_tiles[14];
    SEPERATIONS separation;

    // 合并得到14张牌
    memcpy(standing_tiles, hand_tiles->standing_tiles, standing_cnt * sizeof(TILE));
    standing_tiles[standing_cnt] = win_tile;
    sort_tiles(standing_tiles, standing_cnt + 1);

    seperate_win_hand(standing_tiles, hand_tiles->fixed_sets, fixed_cnt, &separation);

    for (long i = 0; i < separation.count; ++i) {
        std::sort(&separation.sets[i][fixed_cnt], &separation.sets[i][4], &set_cmp);
    }

    long points_tables[MAX_SEPARAION_CNT][POINT_TYPE_COUNT] = { { 0 } };
    int max_points = 0;
    long max_idx = -1;

    if (fixed_cnt == 0) {  // 门清状态，有可能是基本和型组合龙
        if (calculate_knitted_straight_in_basic_type_points(hand_tiles,
            win_tile, ext_cond, points_tables[separation.count])) {
            int current_points = get_points_by_table(points_tables[separation.count]);
            if (current_points > max_points) {
                max_points = current_points;
                max_idx = separation.count;
            }
            LOG("points = %d\n\n", current_points);
        }
        else if (calculate_special_type_points(standing_tiles, ext_cond->win_type, points_tables[separation.count])) {
            int current_points = get_points_by_table(points_tables[separation.count]);
            if (current_points > max_points) {
                max_points = current_points;
                max_idx = separation.count;
            }
            LOG("points = %d\n\n", current_points);
            if (points_tables[separation.count][SEVEN_SHIFTED_PAIRS]) {
                separation.count = 0;
            }
        }
    }
    else if (fixed_cnt == 1 && separation.count == 0) {
        // 1副露状态，有可能是基本和型组合龙
        if (calculate_knitted_straight_in_basic_type_points(hand_tiles,
            win_tile, ext_cond, points_tables[0])) {
            int current_points = get_points_by_table(points_tables[0]);
            if (current_points > max_points) {
                max_points = current_points;
                max_idx = separation.count;
            }
            LOG("points = %d\n\n", current_points);
        }
    }

    // 遍历各种划分方式，分别算番，找出最大的番的划分方式
    for (long i = 0; i < separation.count; ++i) {
#if 0  // Debug
        for (int j = 0; j < 5; ++j) {
            //printf("[%d %s %x]", _separation_sets[i][j].is_melded,
            //    set_type_name[(int)_separation_sets[i][j].set_type], _separation_sets[i][j].mid_tile);
            TILE mid_tile = separation.sets[i][j].mid_tile;
            switch (separation.sets[i][j].set_type) {
            case SET_TYPE::CHOW:
                printf(separation.sets[i][j].is_melded ? "[%s%s%s]" : "{%s%s%s}",
                    stringify_table[mid_tile - 1], stringify_table[mid_tile], stringify_table[mid_tile + 1]);
                break;
            case SET_TYPE::PUNG:
                printf(separation.sets[i][j].is_melded ? "[%s%s%s]" : "{%s%s%s}",
                    stringify_table[mid_tile], stringify_table[mid_tile], stringify_table[mid_tile]);
                break;
            case SET_TYPE::KONG:
                printf(separation.sets[i][j].is_melded ? "[%s%s%s%s]" : "{%s%s%s%s}",
                    stringify_table[mid_tile], stringify_table[mid_tile], stringify_table[mid_tile], stringify_table[mid_tile]);
                break;
            case SET_TYPE::PAIR:
                printf(separation.sets[i][j].is_melded ? "[%s%s]" : "{%s%s}",
                    stringify_table[mid_tile], stringify_table[mid_tile]);
                break;
            default:
                break;
            }
        }
        puts("");
#endif
        calculate_basic_type_points(separation.sets[i], fixed_cnt, win_tile, ext_cond, points_tables[i]);
        int current_points = get_points_by_table(points_tables[i]);
        if (current_points > max_points) {
            max_points = current_points;
            max_idx = i;
        }
        LOG("points = %d\n\n", current_points);
    }
    if (max_idx == -1) {
        return ERROR_NOT_WIN;
    }

    memcpy(points_table, points_tables[max_idx], sizeof(points_table));
    return max_points;
}

}
