#include "wait_and_win.h"

#include <assert.h>
#include <string.h>
#include <limits>

namespace mahjong {

static bool is_knitted_straight_in_basic_type_wait_(const int (&cnt_table)[0x54], long left_cnt, bool (&waiting_table)[0x54]);

static int basic_type_wait_step_recursively(int (&cnt_table)[0x54], long left_cnt, long step, int (&contributing_table)[0x54]) {
    if (left_cnt == 1) {
        // 找到未使用的牌
        int *it = std::find_if(std::begin(cnt_table), std::end(cnt_table), [](int n) { return n > 0; });
        // 存在且张数等于1
        if (it == std::end(cnt_table) || *it != 1) {
            return std::numeric_limits<int>::max();
        }
        // 还有其他未使用的牌
        if (std::any_of(it + 1, std::end(cnt_table), [](int n) { return n > 0; })) {
            return std::numeric_limits<int>::max();
        }

        TILE t = static_cast<TILE>(it - std::begin(cnt_table));
        contributing_table[t] = 1;
        return 0;
    }

    // TODO:
    if (left_cnt == 4) {

    }
}

int basic_type_wait_step(const TILE *standing_tiles, long standing_cnt, int (&contributing_table)[0x54]) {
    // TODO:
    if (standing_tiles == nullptr || standing_cnt != 13
        || standing_cnt != 10 || standing_cnt != 7 || standing_cnt != 4 || standing_cnt != 1) {
        return std::numeric_limits<int>::max();
    }

    // 对立牌的种类进行打表
    int cnt_table[0x54] = { 0 };
    for (long i = 0; i < standing_cnt; ++i) {
        ++cnt_table[standing_tiles[i]];
    }

    return std::numeric_limits<int>::max();
}

static bool is_basic_type_wait_1(const int (&cnt_table)[0x54], bool (&waiting_table)[0x54]) {
    // 找到未使用的牌
    const int *it = std::find_if(std::begin(cnt_table), std::end(cnt_table), [](int n) { return n > 0; });
    // 存在且张数等于1
    if (it == std::end(cnt_table) || *it != 1) {
        return false;
    }
    // 还有其他未使用的牌
    if (std::any_of(it + 1, std::end(cnt_table), [](int n) { return n > 0; })) {
        return false;
    }

    TILE t = static_cast<TILE>(it - std::begin(cnt_table));
    waiting_table[t] = true;
    return true;
}

static bool is_basic_type_wait_2(const int (&cnt_table)[0x54], bool (&waiting_table)[0x54]) {
    bool ret = false;
    for (TILE t = 0x11; t < 0x54; ++t) {
        if (cnt_table[t] < 1) {
            continue;
        }
        if (cnt_table[t] > 1) {
            waiting_table[t] = true;  // 对倒
            ret = true;
            continue;
        }
        if (is_numbered_suit_quick(t)) {  // 数牌搭子
            RANK_TYPE r = tile_rank(t);
            if (r > 1 && cnt_table[t - 1]) {  // 两面或者边张
                if (r < 9) waiting_table[t + 1] = true;
                if (r > 2) waiting_table[t - 2] = true;
                ret = true;
                continue;
            }
            if (r > 2 && cnt_table[t - 2]) {  // 坎张
                waiting_table[t - 1] = true;
                ret = true;
                continue;
            }
        }
    }
    return ret;
}

static bool is_basic_type_wait_4(int (&cnt_table)[0x54], bool (&waiting_table)[0x54]) {
    bool ret = false;
    // 削减将
    for (TILE t = 0x11; t < 0x54; ++t) {
        if (cnt_table[t] < 2) {
            continue;
        }
        // 削减将，递归
        cnt_table[t] -= 2;
        if (is_basic_type_wait_2(cnt_table, waiting_table)) {
            ret = true;
        }
        cnt_table[t] += 2;
    }

    return ret;
}

static bool is_basic_type_wait_recursively(int (&cnt_table)[0x54], long left_cnt, bool (&waiting_table)[0x54]) {
    if (left_cnt == 1) {
        return is_basic_type_wait_1(cnt_table, waiting_table);
    }

    bool ret = false;

    // 顺子（只能是序数牌）
    for (int s = 1; s <= 3; ++s) {
        TILE t1 = make_tile(s, 1);
        TILE t9 = make_tile(s, 9);
        for (TILE t = t1; t <= t9 - 2; ++t) {
            if (!cnt_table[t] || !cnt_table[t + 1] || !cnt_table[t + 2]) {
                continue;
            }

            // 削减这组顺子，递归
            --cnt_table[t];
            --cnt_table[t + 1];
            --cnt_table[t + 2];
            if (is_basic_type_wait_recursively(cnt_table, left_cnt - 3, waiting_table)) {
                ret = true;
            }
            ++cnt_table[t];
            ++cnt_table[t + 1];
            ++cnt_table[t + 2];
        }
    }

    // 刻子
    for (TILE t = 0x11; t <= 0x53; ++t) {
        if (cnt_table[t] < 3) {
            continue;
        }

        // 削减这组刻子，递归
        cnt_table[t] -= 3;
        if (is_basic_type_wait_recursively(cnt_table, left_cnt - 3, waiting_table)) {
            ret = true;
        }
        cnt_table[t] += 3;
    }

    if (!ret) {
        if (left_cnt == 4) {  // 4张无法提取面子的牌
            ret = is_basic_type_wait_4(cnt_table, waiting_table);
        }
        else if (left_cnt == 10 || left_cnt == 13) {
            ret = is_knitted_straight_in_basic_type_wait_(cnt_table, left_cnt, waiting_table);
        }
    }
    return ret;
}

bool is_basic_type_wait(const TILE *standing_tiles, long standing_cnt, bool (&waiting_table)[0x54]) {
    // 对立牌的种类进行打表
    int cnt_table[0x54] = { 0 };
    for (long i = 0; i < standing_cnt; ++i) {
        ++cnt_table[standing_tiles[i]];
    }

    return is_basic_type_wait_recursively(cnt_table, standing_cnt, waiting_table);
}

bool is_basic_type_win(const TILE *standing_tiles, long standing_cnt, TILE test_tile) {
    bool waiting_table[0x54];
    return (is_basic_type_wait(standing_tiles, standing_cnt, waiting_table)
        && waiting_table[test_tile]);
}

// 七对

int seven_pairs_wait_step(const TILE *standing_tiles, long standing_cnt, int (&contributing_table)[0x54]) {
    if (standing_tiles == nullptr || standing_cnt != 13) {
        return std::numeric_limits<int>::max();
    }

    // 对牌的种类进行打表，并统计对子数
    int pair_cnt = 0;
    int cnt_table[0x54] = { 0 };
    for (long i = 0; i < standing_cnt; ++i) {
        TILE tile = standing_tiles[i];
        ++cnt_table[tile];
        if (cnt_table[tile] == 2) {
            ++pair_cnt;
            cnt_table[tile] = 0;
        }
    }

    // 有效牌
    std::transform(std::begin(cnt_table), std::end(cnt_table), std::begin(contributing_table), [](int cnt) { return cnt; });
    return 6 - pair_cnt;
}

bool is_seven_pairs_wait(const TILE *standing_tiles, long standing_cnt, bool (&waiting_table)[0x54]) {
    int contributing_table[0x54];
    if (0 == seven_pairs_wait_step(standing_tiles, standing_cnt, contributing_table)) {
        std::transform(std::begin(contributing_table), std::end(contributing_table), std::begin(waiting_table),
            [](int c) { return !!c; });
        return true;
    }
    return false;
}

bool is_seven_pairs_win(const TILE *standing_tiles, long standing_cnt, TILE test_tile) {
    int contributing_table[0x54];
    return (0 == seven_pairs_wait_step(standing_tiles, standing_cnt, contributing_table)
        && !!contributing_table[test_tile]);
}

// 十三幺

int thirteen_orphans_wait_step(const TILE *standing_tiles, long standing_cnt, int (&contributing_table)[0x54]) {
    if (standing_tiles == nullptr || standing_cnt != 13) {
        return std::numeric_limits<int>::max();
    }

    // 对牌的种类进行打表
    int cnt_table[0x54] = { 0 };
    for (long i = 0; i < standing_cnt; ++i) {
        ++cnt_table[standing_tiles[i]];
    }

    bool has_pair = false;
    int cnt = 0;
    for (int i = 0; i < 13; ++i) {
        int n = cnt_table[standard_thirteen_orphans[i]];
        if (n > 0) {
            ++cnt;  // 幺九牌的种类
            if (n > 1) {
                has_pair = true;  // 幺九牌对子
            }
        }
    }

    // 先标记所有的幺九牌为有效牌
    std::fill(std::begin(contributing_table), std::end(contributing_table), 0);
    std::for_each(std::begin(standard_thirteen_orphans), std::end(standard_thirteen_orphans),
        [&contributing_table](TILE t) {
        contributing_table[t] = 1;
    });

    if (has_pair) {    // 当有对子时，上听数为：12-幺九牌的种类
        // 当有对子时，已有的幺九牌都不需要了
        for (int i = 0; i < 13; ++i) {
            TILE t = standard_thirteen_orphans[i];
            int n = cnt_table[t];
            if (n > 0) {
                contributing_table[t] = 0;
            }
        }
        return 12 - cnt;
    }
    else {  // 当没有对子时，上听数为：13-幺九牌的种类
        return 13 - cnt;
    }
}


bool is_thirteen_orphans_wait(const TILE *standing_tiles, long standing_cnt, bool (&waiting_table)[0x54]) {
    int contributing_table[0x54];
    if (0 == thirteen_orphans_wait_step(standing_tiles, standing_cnt, contributing_table)) {
        std::transform(std::begin(contributing_table), std::end(contributing_table), std::begin(waiting_table),
            [](int c) { return !!c; });
        return true;
    }
    return false;
}

bool is_thirteen_orphans_win(const TILE *standing_tiles, long standing_cnt, TILE test_tile) {
    int contributing_table[0x54];
    return (0 == thirteen_orphans_wait_step(standing_tiles, standing_cnt, contributing_table)
        && !!contributing_table[test_tile]);
}

static bool is_basic_type_match_1(const int (&cnt_table)[0x54]) {
    // 找到未使用的牌
    const int *it = std::find_if(std::begin(cnt_table), std::end(cnt_table), [](int n) { return n > 0; });
    // 存在且张数等于2
    if (it == std::end(cnt_table) || *it != 2) {
        return false;
    }
    // 还有其他未使用的牌
    if (std::any_of(it + 1, std::end(cnt_table), [](int n) { return n > 0; })) {
        return false;
    }
    return true;
}

static bool is_basic_type_match_2(int (&cnt_table)[0x54]) {
    bool ret = false;

    // 顺子（只能是序数牌）
    for (int s = 1; s <= 3; ++s) {
        TILE t1 = make_tile(s, 1);
        TILE t9 = make_tile(s, 9);
        for (TILE t = t1; t <= t9 - 2; ++t) {
            if (!cnt_table[t] || !cnt_table[t + 1] || !cnt_table[t + 2]) {
                continue;
            }

            // 削减这组顺子，递归
            --cnt_table[t];
            --cnt_table[t + 1];
            --cnt_table[t + 2];
            if (is_basic_type_match_1(cnt_table)) {
                ret = true;
            }
            ++cnt_table[t];
            ++cnt_table[t + 1];
            ++cnt_table[t + 2];
        }
    }

    // 刻子
    for (TILE t = 0x11; t <= 0x53; ++t) {
        if (cnt_table[t] < 3) {
            continue;
        }

        // 削减这组刻子，递归
        cnt_table[t] -= 3;
        if (is_basic_type_match_1(cnt_table)) {
            ret = true;
        }
        cnt_table[t] += 3;
    }

    return ret;
}

// “组合龙+面子+将”和型
static bool is_knitted_straight_in_basic_type_wait_(const int (&cnt_table)[0x54], long left_cnt, bool (&waiting_table)[0x54]) {
    // 匹配组合龙
    const TILE (*matched_seq)[9] = nullptr;
    TILE missing_tiles[9];
    int missing_cnt = 0;
    for (int i = 0; i < 6; ++i) {
        missing_cnt = 0;
        for (int k = 0; k < 9; ++k) {
            TILE t = standard_knitted_straight[i][k];
            if (cnt_table[t] == 0) {
                missing_tiles[missing_cnt++] = t;
            }
        }
        if (missing_cnt < 2) {
            matched_seq = &standard_knitted_straight[i];
            break;
        }
    }

    if (matched_seq == nullptr || missing_cnt > 2) {
        return false;
    }

    // 剔除组合龙
    int temp_table[0x54];
    memcpy(temp_table, cnt_table, sizeof(temp_table));
    for (int i = 0; i < 9; ++i) {
        TILE t = (*matched_seq)[i];
        if (temp_table[t]) {
            --temp_table[t];
        }
    }

    if (missing_cnt == 1) {
        if (left_cnt == 10) {
            if (is_basic_type_match_1(temp_table)) {
                waiting_table[missing_tiles[0]] = true;
                return true;
            }
        }
        else {
            if (is_basic_type_match_2(temp_table)) {
                waiting_table[missing_tiles[0]] = true;
                return true;
            }
        }
    }
    else {
        if (left_cnt == 10) {
            return is_basic_type_wait_1(temp_table, waiting_table);
        }
        else {
            return is_basic_type_wait_recursively(temp_table, 4, waiting_table);
        }
    }

    return false;
}

int knitted_straight_in_basic_type_wait_step(const TILE *standing_tiles, long standing_cnt, int (&contributing_table)[0x54]) {
    if (standing_tiles == nullptr || standing_cnt != 13 || standing_cnt != 10) {
        return std::numeric_limits<int>::max();
    }

    // TODO:
    return std::numeric_limits<int>::max();
}

bool is_knitted_straight_in_basic_type_wait(const TILE *standing_tiles, long standing_cnt, bool (&waiting_table)[0x54]) {
    if (standing_tiles == nullptr || standing_cnt != 13 || standing_cnt != 10) {
        return false;
    }

    return false;
}

bool is_knitted_straight_in_basic_type_win(const TILE *standing_tiles, long standing_cnt, TILE test_tile) {
    bool waiting_table[0x54];
    return (is_knitted_straight_in_basic_type_wait(standing_tiles, standing_cnt, waiting_table)
        && waiting_table[test_tile]);
}

// 全不靠/七星不靠

static int honors_and_knitted_tiles_wait_step_(const TILE *standing_tiles, long standing_cnt, int which_seq, int (&contributing_table)[0x54]) {
    if (standing_tiles == nullptr || standing_cnt != 13) {
        return std::numeric_limits<int>::max();
    }

    std::fill(std::begin(contributing_table), std::end(contributing_table), 0);

    // 对牌的种类进行打表
    int cnt_table[0x54] = { 0 };
    for (long i = 0; i < standing_cnt; ++i) {
        ++cnt_table[standing_tiles[i]];
    }

    int cnt = 0;

    // 统计数牌
    for (int i = 0; i < 9; ++i) {
        TILE t = standard_knitted_straight[which_seq][i];
        int n = cnt_table[standard_knitted_straight[which_seq][i]];
        if (n > 0) {
            ++cnt;
        }
        else {
            contributing_table[t] = 1;
        }
    }

    // 统计字牌
    for (int i = 6; i < 13; ++i) {
        TILE t = standard_thirteen_orphans[i];
        int n = cnt_table[t];
        if (n > 0) {
            ++cnt;
        }
        else {
            contributing_table[t] = 1;
        }
    }

    return 13 - cnt;
}

int honors_and_knitted_tiles_wait_step(const TILE *standing_tiles, long standing_cnt, int (&contributing_table)[0x54]) {
    int ret = std::numeric_limits<int>::max();
    int temp_table[0x54];
    std::fill(std::begin(contributing_table), std::end(contributing_table), 0);

    // 6种组合龙分别计算
    for (int i = 0; i < 6; ++i) {
        int st = honors_and_knitted_tiles_wait_step_(standing_tiles, standing_cnt, i, temp_table);
        if (st < ret) {
            ret = st;
            std::copy(std::begin(temp_table), std::end(temp_table), std::begin(contributing_table));
        }
        else if (st == ret) {  // 两种不同组合龙上听数如果相等的话，直接增加有效牌
            for (TILE t = 0x11; t < 0x54; ++t) {
                if (temp_table[t] && !contributing_table[t]) {
                    contributing_table[t] = 1;
                }
            }
        }
    }
    return ret;
}

bool is_honors_and_knitted_tiles_wait(const TILE *standing_tiles, long standing_cnt, bool (&waiting_table)[0x54]) {
    int contributing_table[0x54];
    if (0 == honors_and_knitted_tiles_wait_step(standing_tiles, standing_cnt, contributing_table)) {
        std::transform(std::begin(contributing_table), std::end(contributing_table), std::begin(waiting_table),
            [](int c) { return !!c; });
        return true;
    }
    return false;
}

bool is_honors_and_knitted_tiles_win(const TILE *standing_tiles, long standing_cnt, TILE test_tile) {
    int contributing_table[0x54];
    if (0 == honors_and_knitted_tiles_wait_step(standing_tiles, standing_cnt, contributing_table)) {
        return !!contributing_table[test_tile];
    }
    return false;
}

}
