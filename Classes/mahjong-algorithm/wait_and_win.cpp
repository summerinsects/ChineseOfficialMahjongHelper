#include "wait_and_win.h"

#include <assert.h>
#include <string.h>
#include <limits>

namespace mahjong {

void map_tiles(const TILE *tiles, long cnt, int (&cnt_table)[0x54]) {
    memset(cnt_table, 0, sizeof(cnt_table));
    for (long i = 0; i < cnt; ++i) {
        ++cnt_table[tiles[i]];
    }
}

int count_contributing_tile(int (&used_table)[0x54], bool (&useful_table)[0x54]) {
    int cnt = 0;
    for (TILE t = 0x11; t <= 0x53; ++t) {
        if (useful_table[t])
            cnt += 4 - used_table[t];
    }
    return cnt;
}

static int basic_type_wait_step_recursively(int (&cnt_table)[0x54], int set_cnt, bool has_pair, int neighbour_cnt,
    bool (&useful_table)[0x54]) {
    if (set_cnt + neighbour_cnt >= 4) {  // 搭子超载
        // 有将的情况，听牌时完成面子数为3，上听数=3-完成面子数
        // 无将的情况，听牌时完成面子数为4，上听数=4-完成面子数
        return has_pair ? 3 - set_cnt : 4 - set_cnt;
    }

    int result = std::numeric_limits<int>::max();

    for (TILE t = 0x11; t <= 0x53; ++t) {
        if (cnt_table[t] < 1) {
            continue;
        }

        // 刻子
        if (cnt_table[t] > 2) {
            // 削减这组刻子，递归
            cnt_table[t] -= 3;
            int ret = basic_type_wait_step_recursively(cnt_table, set_cnt + 1, has_pair, neighbour_cnt, useful_table);
            result = std::min(ret, result);
            cnt_table[t] += 3;
        }

        // 顺子（只能是序数牌）
        bool is_numbered = is_numbered_suit(t);
        if (is_numbered) {
            if (tile_rank(t) < 8 && cnt_table[t + 1] && cnt_table[t + 2]) {
                // 削减这组顺子，递归
                --cnt_table[t];
                --cnt_table[t + 1];
                --cnt_table[t + 2];
                int ret = basic_type_wait_step_recursively(cnt_table, set_cnt + 1, has_pair, neighbour_cnt, useful_table);
                result = std::min(ret, result);
                ++cnt_table[t];
                ++cnt_table[t + 1];
                ++cnt_table[t + 2];
            }
        }

        // 对子可看作将或者刻子搭子
        if (cnt_table[t] > 1) {
            // 削减对子
            cnt_table[t] -= 2;

            // 作为将，递归
            if (!has_pair) {
                int ret = basic_type_wait_step_recursively(cnt_table, set_cnt, true, neighbour_cnt, useful_table);
                result = std::min(ret, result);
            }

            // 作为刻子搭子，递归
            int ret = basic_type_wait_step_recursively(cnt_table, set_cnt, has_pair, neighbour_cnt + 1, useful_table);
            result = std::min(ret, result);
            useful_table[t] = true;  // 记录有效牌
            cnt_table[t] += 2;
        }

        // 顺子搭子（只能是序数牌）
        if (is_numbered) {
            // 削减搭子，递归
            if (tile_rank(t) < 9 && cnt_table[t + 1]) {  // 两面或者边张
                --cnt_table[t];
                --cnt_table[t + 1];
                int ret = basic_type_wait_step_recursively(cnt_table, set_cnt, has_pair, neighbour_cnt + 1, useful_table);
                result = std::min(ret, result);
                if (tile_rank(t) > 1) useful_table[t - 1] = true;  // 记录有效牌
                if (tile_rank(t) < 8) useful_table[t + 2] = true;  // 记录有效牌
                ++cnt_table[t];
                ++cnt_table[t + 1];
            }
            if (tile_rank(t) < 8 && cnt_table[t + 2]) {  // 坎张
                --cnt_table[t];
                --cnt_table[t + 2];
                int ret = basic_type_wait_step_recursively(cnt_table, set_cnt, has_pair, neighbour_cnt + 1, useful_table);
                result = std::min(ret, result);
                useful_table[t + 1] = true;  // 记录有效牌
                ++cnt_table[t];
                ++cnt_table[t + 2];
            }
        }
    }

    if (result == std::numeric_limits<int>::max()) {
        // 缺少的搭子数=4-完成的面子数-搭子数
        int neighbour_need = 4 - set_cnt - neighbour_cnt;

        // 有将的情况，上听数=搭子数+缺少的搭子数*2-1
        // 无将的情况，上听数=搭子数+缺少的搭子数*2
        result = neighbour_cnt + neighbour_need * 2;
        if (has_pair) {
            --result;
        }

        if (neighbour_need > 0) {
            // 面子搭子
            for (TILE t = 0x11; t <= 0x53; ++t) {
                if (cnt_table[t] == 0) {
                    continue;
                }

                // 刻子搭子
                useful_table[t] = true;

                // 顺子搭子（只能是序数牌）
                if (is_numbered_suit(t)) {
                    RANK_TYPE r = tile_rank(t);
                    if (r > 1) useful_table[t - 1] = true;
                    if (r > 2) useful_table[t - 2] = true;
                    if (r < 9) useful_table[t + 1] = true;
                    if (r < 8) useful_table[t + 2] = true;
                }
            }
        }

        // 缺将
        if (!has_pair) {
            for (TILE t = 0x11; t <= 0x53; ++t) {
                if (cnt_table[t] == 0) {
                    continue;
                }
                useful_table[t] = true;
            }
        }
    }

    return result;
}

int basic_type_wait_step(const TILE *standing_tiles, long standing_cnt, bool (&useful_table)[0x54]) {
    if (standing_tiles == nullptr || (standing_cnt != 13
        && standing_cnt != 10 && standing_cnt != 7 && standing_cnt != 4 && standing_cnt != 1)) {
        return std::numeric_limits<int>::max();
    }

    // 对立牌的种类进行打表
    int cnt_table[0x54];
    map_tiles(standing_tiles, standing_cnt, cnt_table);

    memset(useful_table, 0, sizeof(useful_table));
    return basic_type_wait_step_recursively(cnt_table, (13 - standing_cnt) / 3, false, 0, useful_table);
}

static bool is_basic_type_wait_1(int (&cnt_table)[0x54], bool (&waiting_table)[0x54]) {
    for (TILE t = 0x11; t < 0x54; ++t) {
        if (cnt_table[t] != 1) {
            continue;
        }

        // 单钓将
        cnt_table[t] = 0;
        if (std::all_of(std::begin(cnt_table), std::end(cnt_table), [](int n) { return n == 0; })) {
            cnt_table[t] = 1;
            waiting_table[t] = true;
            return true;
        }
        cnt_table[t] = 1;
    }

    return false;
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

    for (TILE t = 0x11; t <= 0x53; ++t) {
        if (cnt_table[t] < 1) {
            continue;
        }

        // 刻子
        if (cnt_table[t] > 2) {
            // 削减这组刻子，递归
            cnt_table[t] -= 3;
            if (is_basic_type_wait_recursively(cnt_table, left_cnt - 3, waiting_table)) {
                ret = true;
            }
            cnt_table[t] += 3;
        }

        // 顺子（只能是序数牌）
        bool is_numbered = is_numbered_suit(t);
        if (is_numbered) {
            if (tile_rank(t) < 8 && cnt_table[t + 1] && cnt_table[t + 2]) {
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
    }

    if (!ret) {
        if (left_cnt == 4) {  // 4张无法提取面子的牌
            ret = is_basic_type_wait_4(cnt_table, waiting_table);
        }
    }
    return ret;
}

bool is_basic_type_wait(const TILE *standing_tiles, long standing_cnt, bool (&waiting_table)[0x54]) {
    // 对立牌的种类进行打表
    int cnt_table[0x54];
    map_tiles(standing_tiles, standing_cnt, cnt_table);

    return is_basic_type_wait_recursively(cnt_table, standing_cnt, waiting_table);
}

bool is_basic_type_win(const TILE *standing_tiles, long standing_cnt, TILE test_tile) {
    bool waiting_table[0x54];
    return (is_basic_type_wait(standing_tiles, standing_cnt, waiting_table)
        && waiting_table[test_tile]);
}

// 七对

int seven_pairs_wait_step(const TILE *standing_tiles, long standing_cnt, bool (&useful_table)[0x54]) {
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
    memcpy(useful_table, cnt_table, sizeof(useful_table));
    return 6 - pair_cnt;
}

bool is_seven_pairs_wait(const TILE *standing_tiles, long standing_cnt, bool (&waiting_table)[0x54]) {
    bool useful_table[0x54];
    if (0 == seven_pairs_wait_step(standing_tiles, standing_cnt, useful_table)) {
        memcpy(waiting_table, useful_table, sizeof(waiting_table));
        return true;
    }
    return false;
}

bool is_seven_pairs_win(const TILE *standing_tiles, long standing_cnt, TILE test_tile) {
    bool useful_table[0x54];
    return (0 == seven_pairs_wait_step(standing_tiles, standing_cnt, useful_table)
        && useful_table[test_tile]);
}

// 十三幺

int thirteen_orphans_wait_step(const TILE *standing_tiles, long standing_cnt, bool (&useful_table)[0x54]) {
    if (standing_tiles == nullptr || standing_cnt != 13) {
        return std::numeric_limits<int>::max();
    }

    // 对牌的种类进行打表
    int cnt_table[0x54];
    map_tiles(standing_tiles, standing_cnt, cnt_table);

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
    memset(useful_table, 0, sizeof(useful_table));
    std::for_each(std::begin(standard_thirteen_orphans), std::end(standard_thirteen_orphans),
        [&useful_table](TILE t) {
        useful_table[t] = true;
    });

    if (has_pair) {  // 当有对子时，上听数为：12-幺九牌的种类
        // 当有对子时，已有的幺九牌都不需要了
        for (int i = 0; i < 13; ++i) {
            TILE t = standard_thirteen_orphans[i];
            int n = cnt_table[t];
            if (n > 0) {
                useful_table[t] = false;
            }
        }
        return 12 - cnt;
    }
    else {  // 当没有对子时，上听数为：13-幺九牌的种类
        return 13 - cnt;
    }
}


bool is_thirteen_orphans_wait(const TILE *standing_tiles, long standing_cnt, bool (&waiting_table)[0x54]) {
    bool useful_table[0x54];
    if (0 == thirteen_orphans_wait_step(standing_tiles, standing_cnt, useful_table)) {
        memcpy(waiting_table, useful_table, sizeof(waiting_table));
        return true;
    }
    return false;
}

bool is_thirteen_orphans_win(const TILE *standing_tiles, long standing_cnt, TILE test_tile) {
    bool useful_table[0x54];
    return (0 == thirteen_orphans_wait_step(standing_tiles, standing_cnt, useful_table)
        && useful_table[test_tile]);
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
static bool is_knitted_straight_in_basic_type_wait_impl(const int (&cnt_table)[0x54], long left_cnt, bool (&waiting_table)[0x54]) {
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

static int knitted_straight_in_basic_type_wait_step_1(const TILE *standing_tiles, long standing_cnt, int which_seq, bool (&useful_table)[0x54]) {
    memset(useful_table, 0, sizeof(useful_table));

    // 打表
    int cnt_table[0x54];
    map_tiles(standing_tiles, standing_cnt, cnt_table);

    int cnt = 0;

    // 统计组合龙部分的牌
    for (int i = 0; i < 9; ++i) {
        TILE t = standard_knitted_straight[which_seq][i];
        int n = cnt_table[t];
        if (n > 0) {  // 有，削减之
            ++cnt;
            --cnt_table[t];
        }
        else {  // 没有， 记录有效牌
            useful_table[t] = true;
        }
    }

    // 余下“1组面子+将”的上听数
    int result = basic_type_wait_step_recursively(cnt_table, 3, false, 0, useful_table);
    // 上听数=组合龙缺少的张数+余下“1组面子+将”的上听数
    return (9 - cnt) + result;
}

int knitted_straight_in_basic_type_wait_step(const TILE *standing_tiles, long standing_cnt, bool (&useful_table)[0x54]) {
    if (standing_tiles == nullptr || (standing_cnt != 13 && standing_cnt != 10)) {
        return std::numeric_limits<int>::max();
    }

    int ret = std::numeric_limits<int>::max();
    bool temp_table[0x54];
    memset(useful_table, 0, sizeof(useful_table));

    // 6种组合龙分别计算
    for (int i = 0; i < 6; ++i) {
        int st = knitted_straight_in_basic_type_wait_step_1(standing_tiles, standing_cnt, i, temp_table);
        if (st < ret) {
            ret = st;
            memcpy(useful_table, temp_table, sizeof(useful_table));
        }
        else if (st == ret) {  // 两种不同组合龙上听数如果相等的话，直接增加有效牌
            for (TILE t = 0x11; t < 0x54; ++t) {
                if (temp_table[t] && !useful_table[t]) {
                    useful_table[t] = true;
                }
            }
        }
    }

    return ret;
}

bool is_knitted_straight_in_basic_type_wait(const TILE *standing_tiles, long standing_cnt, bool (&waiting_table)[0x54]) {
    if (standing_tiles == nullptr || (standing_cnt != 13 && standing_cnt != 10)) {
        return false;
    }

    // 对立牌的种类进行打表
    int cnt_table[0x54];
    map_tiles(standing_tiles, standing_cnt, cnt_table);

    return is_knitted_straight_in_basic_type_wait_impl(cnt_table, standing_cnt, waiting_table);
}

bool is_knitted_straight_in_basic_type_win(const TILE *standing_tiles, long standing_cnt, TILE test_tile) {
    bool waiting_table[0x54];
    return (is_knitted_straight_in_basic_type_wait(standing_tiles, standing_cnt, waiting_table)
        && waiting_table[test_tile]);
}

// 全不靠/七星不靠

static int honors_and_knitted_tiles_wait_step_1(const TILE *standing_tiles, long standing_cnt, int which_seq, bool (&useful_table)[0x54]) {
    if (standing_tiles == nullptr || standing_cnt != 13) {
        return std::numeric_limits<int>::max();
    }

    memset(useful_table, 0, sizeof(useful_table));

    // 对牌的种类进行打表
    int cnt_table[0x54];
    map_tiles(standing_tiles, standing_cnt, cnt_table);

    int cnt = 0;

    // 统计组合龙部分的数牌
    for (int i = 0; i < 9; ++i) {
        TILE t = standard_knitted_straight[which_seq][i];
        int n = cnt_table[t];
        if (n > 0) {  // 有，增加记数
            ++cnt;
        }
        else {  // 没有， 记录有效牌
            useful_table[t] = true;
        }
    }

    // 统计字牌
    for (int i = 6; i < 13; ++i) {
        TILE t = standard_thirteen_orphans[i];
        int n = cnt_table[t];
        if (n > 0) {  // 有，增加记数
            ++cnt;
        }
        else {  // 没有， 记录有效牌
            useful_table[t] = true;
        }
    }

    // 上听数=13-符合牌型的记数
    return 13 - cnt;
}

int honors_and_knitted_tiles_wait_step(const TILE *standing_tiles, long standing_cnt, bool (&useful_table)[0x54]) {
    int ret = std::numeric_limits<int>::max();
    bool temp_table[0x54];
    memset(useful_table, 0, sizeof(useful_table));

    // 6种组合龙分别计算
    for (int i = 0; i < 6; ++i) {
        int st = honors_and_knitted_tiles_wait_step_1(standing_tiles, standing_cnt, i, temp_table);
        if (st < ret) {
            ret = st;
            memcpy(useful_table, temp_table, sizeof(useful_table));
        }
        else if (st == ret) {  // 两种不同组合龙上听数如果相等的话，直接增加有效牌
            for (TILE t = 0x11; t < 0x54; ++t) {
                if (temp_table[t] && !useful_table[t]) {
                    useful_table[t] = true;
                }
            }
        }
    }
    return ret;
}

bool is_honors_and_knitted_tiles_wait(const TILE *standing_tiles, long standing_cnt, bool (&waiting_table)[0x54]) {
    bool useful_table[0x54];
    if (0 == honors_and_knitted_tiles_wait_step(standing_tiles, standing_cnt, useful_table)) {
        memcpy(waiting_table, useful_table, sizeof(waiting_table));
        return true;
    }
    return false;
}

bool is_honors_and_knitted_tiles_win(const TILE *standing_tiles, long standing_cnt, TILE test_tile) {
    bool useful_table[0x54];
    if (0 == honors_and_knitted_tiles_wait_step(standing_tiles, standing_cnt, useful_table)) {
        return useful_table[test_tile];
    }
    return false;
}

}
