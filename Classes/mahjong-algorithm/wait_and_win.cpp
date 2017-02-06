#include "wait_and_win.h"

#include <assert.h>
#include <string.h>
#include <limits>

#if 0
#include <stdarg.h>
#ifdef _MSC_VER
#include <windows.h>
#undef min
#undef max
#endif
static void __log(const char *fmt, ...) {
    char buf[1024];

    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    puts(buf);
#ifdef _MSC_VER
    ::OutputDebugStringA(buf);
    ::OutputDebugStringA("\n");
#endif
}
#define MJ_LOG(fmt_, ...) __log(fmt_, ##__VA_ARGS__)
#else
#define MJ_LOG(...) do { } while (0)
#endif

namespace mahjong {

long packs_to_tiles(const pack_t *packs, long pack_cnt, tile_t *tiles, long tile_cnt) {
    if (packs == nullptr || tiles == nullptr) {
        return 0;
    }

    long cnt = 0;
    for (int i = 0; i < pack_cnt && cnt < tile_cnt; ++i) {
        tile_t tile = pack_tile(packs[i]);
        switch (pack_type(packs[i])) {
        case PACK_TYPE_CHOW:
            if (cnt < tile_cnt) tiles[cnt++] = tile - 1;
            if (cnt < tile_cnt) tiles[cnt++] = tile;
            if (cnt < tile_cnt) tiles[cnt++] = tile + 1;
            break;
        case PACK_TYPE_PUNG:
            if (cnt < tile_cnt) tiles[cnt++] = tile;
            if (cnt < tile_cnt) tiles[cnt++] = tile;
            if (cnt < tile_cnt) tiles[cnt++] = tile;
            break;
        case PACK_TYPE_KONG:
            if (cnt < tile_cnt) tiles[cnt++] = tile;
            if (cnt < tile_cnt) tiles[cnt++] = tile;
            if (cnt < tile_cnt) tiles[cnt++] = tile;
            if (cnt < tile_cnt) tiles[cnt++] = tile;
            break;
        case PACK_TYPE_PAIR:
            if (cnt < tile_cnt) tiles[cnt++] = tile;
            if (cnt < tile_cnt) tiles[cnt++] = tile;
            break;
        default:
            break;
        }
    }
    return cnt;
}

void map_tiles(const tile_t *tiles, long cnt, int (&cnt_table)[TILE_TABLE_SIZE]) {
    memset(cnt_table, 0, sizeof(cnt_table));
    for (long i = 0; i < cnt; ++i) {
        ++cnt_table[tiles[i]];
    }
}

bool map_hand_tiles(const hand_tiles_t *hand_tiles, int (&cnt_table)[TILE_TABLE_SIZE]) {
    // 将每一次副露当作3张牌来算，那么总张数=13
    if (hand_tiles->tile_count <= 0 || hand_tiles->pack_count < 0 || hand_tiles->pack_count > 4
        || hand_tiles->pack_count * 3 + hand_tiles->tile_count != 13) {
        return false;
    }

    // 将副露恢复成牌
    tile_t tiles[18];
    long tile_cnt = 0;
    if (hand_tiles->pack_count == 0) {
        memcpy(tiles, hand_tiles->standing_tiles, 13 * sizeof(tile_t));
        tile_cnt = 13;
    }
    else {
        tile_cnt = packs_to_tiles(hand_tiles->fixed_packs, hand_tiles->pack_count, tiles, 18);
        memcpy(tiles + tile_cnt, hand_tiles->standing_tiles, hand_tiles->tile_count * sizeof(tile_t));
        tile_cnt += hand_tiles->tile_count;
    }

    // 打表
    map_tiles(tiles, tile_cnt, cnt_table);
    return true;
}

int count_useful_tile(const int (&used_table)[TILE_TABLE_SIZE], const bool (&useful_table)[TILE_TABLE_SIZE]) {
    int cnt = 0;
    for (int i = 0; i < 34; ++i) {
        tile_t t = all_tiles[i];
        if (useful_table[t]) {
            cnt += 4 - used_table[t];
        }
    }
    return cnt;
}

typedef uint16_t path_unit_t;
#define UNIT_TYPE_CHOW 1
#define UNIT_TYPE_PUNG 2
#define UNIT_TYPE_PAIR 4
#define UNIT_TYPE_NEIGHBOR_BOTH 5
#define UNIT_TYPE_NEIGHBOR_MID 6
#define UNIT_TYPE_NEIGHBOR_PUNG 7

#define MAKE_UNIT(type_, tile_) (((type_) << 8) | (tile_))
#define UNIT_TYPE(unit_) (((unit_) >> 8) & 0xFF)
#define UNIT_TILE(unit_) ((unit_) & 0xFF)

#define MAX_STATE 256
#define UNIT_SIZE 7

struct work_path_t {
    path_unit_t units[UNIT_SIZE];  // 14/2=7最多7个搭子
    uint16_t depth;  // 当前路径深度
};

struct work_state_t {
    work_path_t paths[MAX_STATE];  // 所有路径
    long count;  // 路径数量
};

static bool is_basic_type_branch_exist(const long fixed_cnt, const work_path_t *work_path, const work_state_t *work_state) {
    if (work_state->count <= 0) {
        return false;
    }

    const uint16_t depth = work_path->depth + 1;

    // std::includes要求有序，但又不能破坏当前数据
    work_path_t temp;
    std::copy(&work_path->units[fixed_cnt], &work_path->units[depth], &temp.units[fixed_cnt]);
    std::sort(&temp.units[fixed_cnt], &temp.units[depth]);

    return std::any_of(&work_state->paths[0], &work_state->paths[work_state->count],
        [&temp, fixed_cnt, depth](const work_path_t &path) {
        return std::includes(&path.units[fixed_cnt], &path.units[path.depth], &temp.units[fixed_cnt], &temp.units[depth]);
    });
}

static void save_work_path(const long fixed_cnt, const work_path_t *work_path, work_state_t *work_state) {
    if (work_state->count < MAX_STATE) {
        work_path_t &path = work_state->paths[work_state->count++];
        path.depth = work_path->depth;
        std::copy(&work_path->units[fixed_cnt], &work_path->units[work_path->depth], &path.units[fixed_cnt]);

        // 检测是否重复路径时，std::includes要求有序，所以这里将它排序
        std::sort(&path.units[fixed_cnt], &path.units[path.depth]);
    }
    else {
        assert(0 && "too many state!");
    }
}

static int basic_type_wait_step_recursively(int (&cnt_table)[TILE_TABLE_SIZE], const long fixed_cnt, const bool has_pair, const uint16_t pack_cnt,
    const uint16_t neighbor_cnt, work_path_t *work_path, work_state_t *work_state) {
    if (pack_cnt == 4) {  // 已经有4组面子
        return has_pair ? -1 : 0;  // 有雀头：和了；无雀头：听牌
    }

    int max_ret;  // 当前状态能返回的最大上听数

    // 缺少的搭子数=4-完成的面子数-搭子数
    int neighbor_need = 4 - pack_cnt - neighbor_cnt;
    if (neighbor_need > 0) {  // 还需要搭子的情况
        // 有雀头时，上听数=搭子数+缺少的搭子数*2-1
        // 无雀头时，上听数=搭子数+缺少的搭子数*2
        max_ret = neighbor_cnt + neighbor_need * 2 - (has_pair ? 1 : 0);
    }
    else {  // 搭子齐了的情况
        // 有雀头时，上听数=3-完成面子数
        // 无雀头时，上听数=4-完成面子数
        max_ret = (has_pair ? 3 : 4) - pack_cnt;
    }

    const uint16_t depth = pack_cnt + neighbor_cnt + has_pair;
    work_path->depth = depth;

    int result = max_ret;

    if (pack_cnt + neighbor_cnt > 4) {  // 搭子超载
        save_work_path(fixed_cnt, work_path, work_state);
        return max_ret;
    }

    for (int i = 0; i < 34; ++i) {
        tile_t t = all_tiles[i];
        if (cnt_table[t] < 1) {
            continue;
        }

        // 雀头
        if (!has_pair && cnt_table[t] > 1) {
            work_path->units[depth] = MAKE_UNIT(UNIT_TYPE_PAIR, t);
            if (is_basic_type_branch_exist(fixed_cnt, work_path, work_state)) {
                MJ_LOG("branch exist : %x %x %x %x %x", work_units[0], work_units[1], work_units[2], work_units[3], work_units[4]);
                continue;
            }

            // 削减雀头，递归
            cnt_table[t] -= 2;
            int ret = basic_type_wait_step_recursively(cnt_table, fixed_cnt, true, pack_cnt,
                neighbor_cnt, work_path, work_state);
            result = std::min(ret, result);
            cnt_table[t] += 2;
        }

        // 刻子
        if (cnt_table[t] > 2) {
            work_path->units[depth] = MAKE_UNIT(UNIT_TYPE_PUNG, t);
            if (is_basic_type_branch_exist(fixed_cnt, work_path, work_state)) {
                continue;
            }

            // 削减这组刻子，递归
            cnt_table[t] -= 3;
            int ret = basic_type_wait_step_recursively(cnt_table, fixed_cnt, has_pair, pack_cnt + 1,
                neighbor_cnt, work_path, work_state);
            result = std::min(ret, result);
            cnt_table[t] += 3;
        }

        // 顺子（只能是数牌）
        bool is_numbered = is_numbered_suit(t);
        if (is_numbered && tile_rank(t) < 8 && cnt_table[t + 1] && cnt_table[t + 2]) {
            work_path->units[depth] = MAKE_UNIT(UNIT_TYPE_CHOW, t);
            if (is_basic_type_branch_exist(fixed_cnt, work_path, work_state)) {
                MJ_LOG("branch exist : %x %x %x %x %x", work_units[0], work_units[1], work_units[2], work_units[3], work_units[4]);
                continue;
            }

            // 削减这组顺子，递归
            --cnt_table[t];
            --cnt_table[t + 1];
            --cnt_table[t + 2];
            int ret = basic_type_wait_step_recursively(cnt_table, fixed_cnt, has_pair, pack_cnt + 1,
                neighbor_cnt, work_path, work_state);
            result = std::min(ret, result);
            ++cnt_table[t];
            ++cnt_table[t + 1];
            ++cnt_table[t + 2];
        }

        // 有雀头时N个搭子至少N-1上听
        // 无雀头时N个搭子至少N上听
        int min_ret = has_pair ? neighbor_cnt - 1 : neighbor_cnt;
        // 已经没有再削减搭子的必要了，上听数肯定会超过之前已经计算过的
        if (min_ret >= result) {
            continue;
        }

        // 刻子搭子
        if (cnt_table[t] > 1) {
            // 削减刻子搭子，递归
            work_path->units[depth] = MAKE_UNIT(UNIT_TYPE_NEIGHBOR_PUNG, t);
            if (is_basic_type_branch_exist(fixed_cnt, work_path, work_state)) {
                continue;
            }

            cnt_table[t] -= 2;
            int ret = basic_type_wait_step_recursively(cnt_table, fixed_cnt, has_pair, pack_cnt,
                neighbor_cnt + 1, work_path, work_state);
            result = std::min(ret, result);
            cnt_table[t] += 2;
        }

        // 顺子搭子（只能是数牌）
        if (is_numbered) {
            // 削减搭子，递归
            if (tile_rank(t) < 9 && cnt_table[t + 1]) {  // 两面或者边张
                work_path->units[depth] = MAKE_UNIT(UNIT_TYPE_NEIGHBOR_BOTH, t);
                if (is_basic_type_branch_exist(fixed_cnt, work_path, work_state)) {
                    MJ_LOG("branch exist : %x %x %x %x %x", work_units[0], work_units[1], work_units[2], work_units[3], work_units[4]);
                    continue;
                }

                --cnt_table[t];
                --cnt_table[t + 1];
                int ret = basic_type_wait_step_recursively(cnt_table, fixed_cnt, has_pair, pack_cnt,
                    neighbor_cnt + 1, work_path, work_state);
                result = std::min(ret, result);
                ++cnt_table[t];
                ++cnt_table[t + 1];
            }
            if (tile_rank(t) < 8 && cnt_table[t + 2]) {  // 坎张
                work_path->units[depth] = MAKE_UNIT(UNIT_TYPE_NEIGHBOR_MID, t);
                if (is_basic_type_branch_exist(fixed_cnt, work_path, work_state)) {
                    continue;
                }

                --cnt_table[t];
                --cnt_table[t + 2];
                int ret = basic_type_wait_step_recursively(cnt_table, fixed_cnt, has_pair, pack_cnt,
                    neighbor_cnt + 1, work_path, work_state);
                result = std::min(ret, result);
                ++cnt_table[t];
                ++cnt_table[t + 2];
            }
        }
    }

    if (result == max_ret) {
        save_work_path(fixed_cnt, work_path, work_state);
    }

    return result;
}

static bool numbered_tile_has_neighbor(const int (&cnt_table)[TILE_TABLE_SIZE], tile_t t) {
    rank_t r = tile_rank(t);
    if (r < 9) { if (cnt_table[t + 1]) return true; }
    if (r < 8) { if (cnt_table[t + 2]) return true; }
    if (r > 1) { if (cnt_table[t - 1]) return true; }
    if (r > 2) { if (cnt_table[t - 2]) return true; }
    return false;
}

static int basic_type_wait_step_from_table(int (&cnt_table)[TILE_TABLE_SIZE], long fixed_cnt, bool (*useful_table)[TILE_TABLE_SIZE]) {
    // 计算上听数
    work_path_t work_path;
    work_state_t work_state;
    work_state.count = 0;
    int result = basic_type_wait_step_recursively(cnt_table, fixed_cnt, false, 0, static_cast<uint16_t>(fixed_cnt), &work_path, &work_state);

    if (useful_table == nullptr) {
        return result;
    }

    // 获取能减少上听数的牌
    for (int i = 0; i < 34; ++i) {
        tile_t t = all_tiles[i];
        if (cnt_table[t] == 4) {
            continue;
        }

        if (cnt_table[t] == 0) {
            // 跳过孤张字牌和不靠张的数牌，这些牌都无法减少上听数
            if (is_honor(t) || !numbered_tile_has_neighbor(cnt_table, t)) {
                continue;
            }
        }

        ++cnt_table[t];
        work_state.count = 0;
        int temp = basic_type_wait_step_recursively(cnt_table, fixed_cnt, false, 0, static_cast<uint16_t>(fixed_cnt), &work_path, &work_state);
        if (temp < result) {
            (*useful_table)[t] = true;  // 标记为有效牌
        }
        --cnt_table[t];
    }

    return result;
}

int basic_type_wait_step(const tile_t *standing_tiles, long standing_cnt, bool (*useful_table)[TILE_TABLE_SIZE]) {
    if (standing_tiles == nullptr || (standing_cnt != 13
        && standing_cnt != 10 && standing_cnt != 7 && standing_cnt != 4 && standing_cnt != 1)) {
        return std::numeric_limits<int>::max();
    }

    // 对立牌的种类进行打表
    int cnt_table[TILE_TABLE_SIZE];
    map_tiles(standing_tiles, standing_cnt, cnt_table);

    if (useful_table != nullptr) {
        memset(*useful_table, 0, sizeof(*useful_table));
    }
    return basic_type_wait_step_from_table(cnt_table, (13 - standing_cnt) / 3, useful_table);
}

static bool is_basic_type_wait_1(int (&cnt_table)[TILE_TABLE_SIZE], bool (*waiting_table)[TILE_TABLE_SIZE]) {
    for (int i = 0; i < 34; ++i) {
        tile_t t = all_tiles[i];
        if (cnt_table[t] != 1) {
            continue;
        }

        // 单钓将
        cnt_table[t] = 0;
        if (std::all_of(std::begin(cnt_table), std::end(cnt_table), [](int n) { return n == 0; })) {
            cnt_table[t] = 1;
            if (waiting_table != nullptr) {
                (*waiting_table)[t] = true;
            }
            return true;
        }
        cnt_table[t] = 1;
    }

    return false;
}

static bool is_basic_type_wait_2(const int (&cnt_table)[TILE_TABLE_SIZE], bool (*waiting_table)[TILE_TABLE_SIZE]) {
    bool ret = false;
    for (int i = 0; i < 34; ++i) {
        tile_t t = all_tiles[i];
        if (cnt_table[t] < 1) {
            continue;
        }
        if (cnt_table[t] > 1) {
            if (waiting_table != nullptr) {
                (*waiting_table)[t] = true;  // 对倒
                ret = true;
                continue;
            }
            else {
                return true;
            }
        }
        if (is_numbered_suit_quick(t)) {  // 数牌搭子
            rank_t r = tile_rank(t);
            if (r > 1 && cnt_table[t - 1]) {  // 两面或者边张
                if (waiting_table != nullptr) {
                    if (r < 9) (*waiting_table)[t + 1] = true;
                    if (r > 2) (*waiting_table)[t - 2] = true;
                    ret = true;
                    continue;
                }
                else {
                    return true;
                }
            }
            if (r > 2 && cnt_table[t - 2]) {  // 坎张
                if (waiting_table != nullptr) {
                    (*waiting_table)[t - 1] = true;
                    ret = true;
                    continue;
                }
                else {
                    return true;
                }
            }
        }
    }
    return ret;
}

static bool is_basic_type_wait_4(int (&cnt_table)[TILE_TABLE_SIZE], bool (*waiting_table)[TILE_TABLE_SIZE]) {
    bool ret = false;
    // 削减雀头
    for (int i = 0; i < 34; ++i) {
        tile_t t = all_tiles[i];
        if (cnt_table[t] < 2) {
            continue;
        }
        // 削减雀头，递归
        cnt_table[t] -= 2;
        if (is_basic_type_wait_2(cnt_table, waiting_table)) {
            ret = true;
        }
        cnt_table[t] += 2;
        if (ret && waiting_table == nullptr) {
            return true;
        }
    }

    return ret;
}

static bool is_basic_type_wait_recursively(int (&cnt_table)[TILE_TABLE_SIZE], long left_cnt, bool (*waiting_table)[TILE_TABLE_SIZE]) {
    if (left_cnt == 1) {
        return is_basic_type_wait_1(cnt_table, waiting_table);
    }

    bool ret = false;
    if (left_cnt == 4) {
        ret = is_basic_type_wait_4(cnt_table, waiting_table);
        if (ret && waiting_table == nullptr) {
            return true;
        }
    }

    for (int i = 0; i < 34; ++i) {
        tile_t t = all_tiles[i];
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
            if (ret && waiting_table == nullptr) {
                return true;
            }
        }

        // 顺子（只能是数牌）
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
                if (ret && waiting_table == nullptr) {
                    return true;
                }
            }
        }
    }

    return ret;
}

bool is_basic_type_wait(const tile_t *standing_tiles, long standing_cnt, bool (*waiting_table)[TILE_TABLE_SIZE]) {
    // 对立牌的种类进行打表
    int cnt_table[TILE_TABLE_SIZE];
    map_tiles(standing_tiles, standing_cnt, cnt_table);

    if (waiting_table != nullptr) {
        memset(*waiting_table, 0, sizeof(*waiting_table));
    }
    return is_basic_type_wait_recursively(cnt_table, standing_cnt, waiting_table);
}

bool is_basic_type_win(const tile_t *standing_tiles, long standing_cnt, tile_t test_tile) {
    bool waiting_table[TILE_TABLE_SIZE];
    return (is_basic_type_wait(standing_tiles, standing_cnt, &waiting_table)
        && waiting_table[test_tile]);
}

// 七对

int seven_pairs_wait_step(const tile_t *standing_tiles, long standing_cnt, bool (*useful_table)[TILE_TABLE_SIZE]) {
    if (standing_tiles == nullptr || standing_cnt != 13) {
        return std::numeric_limits<int>::max();
    }

    // 对牌的种类进行打表，并统计对子数
    int pair_cnt = 0;
    int cnt_table[TILE_TABLE_SIZE] = { 0 };
    for (long i = 0; i < standing_cnt; ++i) {
        tile_t tile = standing_tiles[i];
        ++cnt_table[tile];
        if (cnt_table[tile] == 2) {
            ++pair_cnt;
            cnt_table[tile] = 0;
        }
    }

    // 有效牌
    if (useful_table != nullptr) {
        std::transform(std::begin(cnt_table), std::end(cnt_table), std::begin(*useful_table), [](int n) { return !!n; });
    }
    return 6 - pair_cnt;
}

bool is_seven_pairs_wait(const tile_t *standing_tiles, long standing_cnt, bool (*waiting_table)[TILE_TABLE_SIZE]) {
    if (0 == seven_pairs_wait_step(standing_tiles, standing_cnt, waiting_table)) {
        return true;
    }
    return false;
}

bool is_seven_pairs_win(const tile_t *standing_tiles, long standing_cnt, tile_t test_tile) {
    bool useful_table[TILE_TABLE_SIZE];
    return (0 == seven_pairs_wait_step(standing_tiles, standing_cnt, &useful_table)
        && useful_table[test_tile]);
}

// 十三幺

int thirteen_orphans_wait_step(const tile_t *standing_tiles, long standing_cnt, bool (*useful_table)[TILE_TABLE_SIZE]) {
    if (standing_tiles == nullptr || standing_cnt != 13) {
        return std::numeric_limits<int>::max();
    }

    // 对牌的种类进行打表
    int cnt_table[TILE_TABLE_SIZE];
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

    if (useful_table != nullptr) {
    // 先标记所有的幺九牌为有效牌
        memset(*useful_table, 0, sizeof(*useful_table));
        std::for_each(std::begin(standard_thirteen_orphans), std::end(standard_thirteen_orphans),
            [useful_table](tile_t t) {
            (*useful_table)[t] = true;
        });
    }

    if (has_pair) {  // 当有对子时，上听数为：12-幺九牌的种类
        if (useful_table != nullptr) {
            // 当有对子时，已有的幺九牌都不需要了
            for (int i = 0; i < 13; ++i) {
                tile_t t = standard_thirteen_orphans[i];
                int n = cnt_table[t];
                if (n > 0) {
                    (*useful_table)[t] = false;
                }
            }
        }
        return 12 - cnt;
    }
    else {  // 当没有对子时，上听数为：13-幺九牌的种类
        return 13 - cnt;
    }
}


bool is_thirteen_orphans_wait(const tile_t *standing_tiles, long standing_cnt, bool (*waiting_table)[TILE_TABLE_SIZE]) {
    if (0 == thirteen_orphans_wait_step(standing_tiles, standing_cnt, waiting_table)) {
        return true;
    }
    return false;
}

bool is_thirteen_orphans_win(const tile_t *standing_tiles, long standing_cnt, tile_t test_tile) {
    bool useful_table[TILE_TABLE_SIZE];
    return (0 == thirteen_orphans_wait_step(standing_tiles, standing_cnt, &useful_table)
        && useful_table[test_tile]);
}

static bool is_basic_type_match_1(const int (&cnt_table)[TILE_TABLE_SIZE]) {
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

static bool is_basic_type_match_2(int (&cnt_table)[TILE_TABLE_SIZE]) {
    bool ret = false;

    // 顺子（只能是数牌）
    for (int s = 1; s <= 3; ++s) {
        tile_t t1 = make_tile(s, 1);
        tile_t t9 = make_tile(s, 9);
        for (tile_t t = t1; t <= t9 - 2; ++t) {
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
    for (int i = 0; i < 34; ++i) {
        tile_t t = all_tiles[i];
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

// “组合龙+面子+雀头”和型
static bool is_knitted_straight_in_basic_type_wait_impl(const int (&cnt_table)[TILE_TABLE_SIZE], long left_cnt, bool (*waiting_table)[TILE_TABLE_SIZE]) {
    // 匹配组合龙
    const tile_t (*matched_seq)[9] = nullptr;
    tile_t missing_tiles[9];
    int missing_cnt = 0;
    for (int i = 0; i < 6; ++i) {
        missing_cnt = 0;
        for (int k = 0; k < 9; ++k) {
            tile_t t = standard_knitted_straight[i][k];
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
    int temp_table[TILE_TABLE_SIZE];
    memcpy(temp_table, cnt_table, sizeof(temp_table));
    for (int i = 0; i < 9; ++i) {
        tile_t t = (*matched_seq)[i];
        if (temp_table[t]) {
            --temp_table[t];
        }
    }

    if (missing_cnt == 1) {
        if (left_cnt == 10) {
            if (is_basic_type_match_1(temp_table)) {
                if (waiting_table != nullptr) {
                    (*waiting_table)[missing_tiles[0]] = true;
                }
                return true;
            }
        }
        else {
            if (is_basic_type_match_2(temp_table)) {
                if (waiting_table != nullptr) {
                    (*waiting_table)[missing_tiles[0]] = true;
                }
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

static int knitted_straight_in_basic_type_wait_step_1(const tile_t *standing_tiles, long standing_cnt, int which_seq, bool (*useful_table)[TILE_TABLE_SIZE]) {
    if (useful_table != nullptr) {
        memset(*useful_table, 0, sizeof(*useful_table));
    }

    // 打表
    int cnt_table[TILE_TABLE_SIZE];
    map_tiles(standing_tiles, standing_cnt, cnt_table);

    int cnt = 0;

    // 统计组合龙部分的牌
    for (int i = 0; i < 9; ++i) {
        tile_t t = standard_knitted_straight[which_seq][i];
        int n = cnt_table[t];
        if (n > 0) {  // 有，削减之
            ++cnt;
            --cnt_table[t];
        }
        else if (useful_table != nullptr) {  // 没有， 记录有效牌
            (*useful_table)[t] = true;
        }
    }

    // 余下“1组面子+雀头”的上听数
    int result = basic_type_wait_step_from_table(cnt_table, 3, useful_table);

    // 上听数=组合龙缺少的张数+余下“1组面子+雀头”的上听数
    return (9 - cnt) + result;
}

int knitted_straight_in_basic_type_wait_step(const tile_t *standing_tiles, long standing_cnt, bool (*useful_table)[TILE_TABLE_SIZE]) {
    if (standing_tiles == nullptr || (standing_cnt != 13 && standing_cnt != 10)) {
        return std::numeric_limits<int>::max();
    }

    int ret = std::numeric_limits<int>::max();
    bool temp_table[TILE_TABLE_SIZE];

    if (useful_table != nullptr) {
        memset(*useful_table, 0, sizeof(*useful_table));
    }

    // 6种组合龙分别计算
    for (int i = 0; i < 6; ++i) {
        int st = knitted_straight_in_basic_type_wait_step_1(standing_tiles, standing_cnt, i, useful_table != nullptr ? &temp_table : nullptr);
        if (st < ret) {
            ret = st;
            if (useful_table != nullptr) {
                memcpy(*useful_table, temp_table, sizeof(*useful_table));
            }
        }
        else if (st == ret && useful_table != nullptr) {  // 两种不同组合龙上听数如果相等的话，直接增加有效牌
            std::transform(std::begin(*useful_table), std::end(*useful_table), std::begin(temp_table),
                std::begin(*useful_table), [](bool u, bool t) { return u || t; });
        }
    }

    return ret;
}

bool is_knitted_straight_in_basic_type_wait(const tile_t *standing_tiles, long standing_cnt, bool (*waiting_table)[TILE_TABLE_SIZE]) {
    if (standing_tiles == nullptr || (standing_cnt != 13 && standing_cnt != 10)) {
        return false;
    }

    // 对立牌的种类进行打表
    int cnt_table[TILE_TABLE_SIZE];
    map_tiles(standing_tiles, standing_cnt, cnt_table);

    return is_knitted_straight_in_basic_type_wait_impl(cnt_table, standing_cnt, waiting_table);
}

bool is_knitted_straight_in_basic_type_win(const tile_t *standing_tiles, long standing_cnt, tile_t test_tile) {
    bool waiting_table[TILE_TABLE_SIZE];
    return (is_knitted_straight_in_basic_type_wait(standing_tiles, standing_cnt, &waiting_table)
        && waiting_table[test_tile]);
}

// 全不靠/七星不靠

static int honors_and_knitted_tiles_wait_step_1(const tile_t *standing_tiles, long standing_cnt, int which_seq, bool (*useful_table)[TILE_TABLE_SIZE]) {
    if (standing_tiles == nullptr || standing_cnt != 13) {
        return std::numeric_limits<int>::max();
    }

    if (useful_table != nullptr) {
        memset(*useful_table, 0, sizeof(*useful_table));
    }

    // 对牌的种类进行打表
    int cnt_table[TILE_TABLE_SIZE];
    map_tiles(standing_tiles, standing_cnt, cnt_table);

    int cnt = 0;

    // 统计组合龙部分的数牌
    for (int i = 0; i < 9; ++i) {
        tile_t t = standard_knitted_straight[which_seq][i];
        int n = cnt_table[t];
        if (n > 0) {  // 有，增加计数
            ++cnt;
        }
        else if (useful_table != nullptr) {  // 没有， 记录有效牌
            (*useful_table)[t] = true;
        }
    }

    // 统计字牌
    for (int i = 6; i < 13; ++i) {
        tile_t t = standard_thirteen_orphans[i];
        int n = cnt_table[t];
        if (n > 0) {  // 有，增加计数
            ++cnt;
        }
        else if (useful_table != nullptr) {  // 没有， 记录有效牌
            (*useful_table)[t] = true;
        }
    }

    // 上听数=13-符合牌型的计数
    return 13 - cnt;
}

int honors_and_knitted_tiles_wait_step(const tile_t *standing_tiles, long standing_cnt, bool (*useful_table)[TILE_TABLE_SIZE]) {
    int ret = std::numeric_limits<int>::max();
    bool temp_table[TILE_TABLE_SIZE];

    if (useful_table != nullptr) {
        memset(*useful_table, 0, sizeof(*useful_table));
    }

    // 6种组合龙分别计算
    for (int i = 0; i < 6; ++i) {
        int st = honors_and_knitted_tiles_wait_step_1(standing_tiles, standing_cnt, i, useful_table != nullptr ? &temp_table : nullptr);
        if (st < ret) {
            ret = st;
            if (useful_table != nullptr) {
                memcpy(*useful_table, temp_table, sizeof(*useful_table));
            }
        }
        else if (st == ret && useful_table != nullptr) {  // 两种不同组合龙上听数如果相等的话，直接增加有效牌
            std::transform(std::begin(*useful_table), std::end(*useful_table), std::begin(temp_table),
                std::begin(*useful_table), [](bool u, bool t) { return u || t; });
        }
    }
    return ret;
}

bool is_honors_and_knitted_tiles_wait(const tile_t *standing_tiles, long standing_cnt, bool (*waiting_table)[TILE_TABLE_SIZE]) {
    if (0 == honors_and_knitted_tiles_wait_step(standing_tiles, standing_cnt, waiting_table)) {
        return true;
    }
    return false;
}

bool is_honors_and_knitted_tiles_win(const tile_t *standing_tiles, long standing_cnt, tile_t test_tile) {
    bool useful_table[TILE_TABLE_SIZE];
    if (0 == honors_and_knitted_tiles_wait_step(standing_tiles, standing_cnt, &useful_table)) {
        return useful_table[test_tile];
    }
    return false;
}

static bool enum_discard_tile_1(const hand_tiles_t *hand_tiles, tile_t discard_tile, uint8_t form_flag,
    void *context, enum_callback_t enum_callback) {
    enum_result_t result;
    result.discard_tile = discard_tile;
    result.form_flag = FORM_FLAG_BASIC_TYPE;
    result.wait_step = basic_type_wait_step(hand_tiles->standing_tiles, hand_tiles->tile_count, &result.useful_table);
    if (result.wait_step == 0 && result.useful_table[discard_tile]) {
        result.wait_step = -1;
    }
    if (!enum_callback(context, &result)) {
        return false;
    }

    if (hand_tiles->tile_count == 13) {
        if (form_flag | FORM_FLAG_SEVEN_PAIRS) {
            result.form_flag = FORM_FLAG_SEVEN_PAIRS;
            result.wait_step = seven_pairs_wait_step(hand_tiles->standing_tiles, hand_tiles->tile_count, &result.useful_table);
            if (result.wait_step == 0 && result.useful_table[discard_tile]) {
                result.wait_step = -1;
            }
            if (!enum_callback(context, &result)) {
                return false;
            }
        }

        if (form_flag | FORM_FLAG_THIRTEEN_ORPHANS) {
            result.form_flag = FORM_FLAG_THIRTEEN_ORPHANS;
            result.wait_step = thirteen_orphans_wait_step(hand_tiles->standing_tiles, hand_tiles->tile_count, &result.useful_table);
            if (result.wait_step == 0 && result.useful_table[discard_tile]) {
                result.wait_step = -1;
            }
            if (!enum_callback(context, &result)) {
                return false;
            }
        }

        if (form_flag | FORM_FLAG_HONORS_AND_KNITTED_TILES) {
            result.form_flag = FORM_FLAG_HONORS_AND_KNITTED_TILES;
            result.wait_step = honors_and_knitted_tiles_wait_step(hand_tiles->standing_tiles, hand_tiles->tile_count, &result.useful_table);
            if (result.wait_step == 0 && result.useful_table[discard_tile]) {
                result.wait_step = -1;
            }
            if (!enum_callback(context, &result)) {
                return false;
            }
        }
    }

    if (hand_tiles->tile_count == 13 || hand_tiles->tile_count == 10) {
        if (form_flag | FORM_FLAG_KNITTED_STRAIGHT) {
            result.form_flag = FORM_FLAG_KNITTED_STRAIGHT;
            result.wait_step = knitted_straight_in_basic_type_wait_step(hand_tiles->standing_tiles, hand_tiles->tile_count, &result.useful_table);
            if (result.wait_step == 0 && result.useful_table[discard_tile]) {
                result.wait_step = -1;
            }
            if (!enum_callback(context, &result)) {
                return false;
            }
        }
    }

    return true;
}

long table_to_tiles(const int (&cnt_table)[TILE_TABLE_SIZE], tile_t *tiles, long max_cnt) {
    long cnt = 0;
    for (int i = 0; i < 34; ++i) {
        tile_t t = all_tiles[i];
        for (int n = 0; n < cnt_table[t]; ++n) {
            *tiles++ = t;
            ++cnt;
            if (cnt == max_cnt) {
                return max_cnt;
            }
        }
    }
    return cnt;
}

void enum_discard_tile(const hand_tiles_t *hand_tiles, tile_t serving_tile, uint8_t form_flag,
    void *context, enum_callback_t enum_callback) {
    if (!enum_discard_tile_1(hand_tiles, serving_tile, form_flag, context, enum_callback)) {
        return;
    }

    if (serving_tile == 0) {
        return;
    }

    int cnt_table[TILE_TABLE_SIZE];
    map_tiles(hand_tiles->standing_tiles, hand_tiles->tile_count, cnt_table);

    hand_tiles_t temp;
    memcpy(&temp, hand_tiles, sizeof(temp));

    for (int i = 0; i < 34; ++i) {
        tile_t t = all_tiles[i];
        if (cnt_table[t] && t != serving_tile && cnt_table[serving_tile] < 4) {
            --cnt_table[t];
            ++cnt_table[serving_tile];

            table_to_tiles(cnt_table, temp.standing_tiles, temp.tile_count);
            if (!enum_discard_tile_1(&temp, t, form_flag, context, enum_callback)) {
                return;
            }

            --cnt_table[serving_tile];
            ++cnt_table[t];
        }
    }
}

}
