#ifndef _WAIT_AND_WIN_TEST_H_
#define _WAIT_AND_WIN_TEST_H_

#include "tile.h"

namespace mahjong {

void map_tiles(const tile_t *tiles, long cnt, int (&cnt_table)[TILE_TABLE_COUNT]);
long table_to_tiles(const int (&cnt_table)[TILE_TABLE_COUNT], tile_t *tiles, long max_cnt);
int count_contributing_tile(const int (&used_table)[TILE_TABLE_COUNT], const bool (&useful_table)[TILE_TABLE_COUNT]);

int basic_type_wait_step(const tile_t *standing_tiles, long standing_cnt, bool (&useful_table)[TILE_TABLE_COUNT]);
bool is_basic_type_wait(const tile_t *standing_tiles, long standing_cnt, bool (&waiting_table)[TILE_TABLE_COUNT]);
bool is_basic_type_win(const tile_t *standing_tiles, long standing_cnt, tile_t test_tile);

int seven_pairs_wait_step(const tile_t *standing_tiles, long standing_cnt, bool (&useful_table)[TILE_TABLE_COUNT]);
bool is_seven_pairs_wait(const tile_t *standing_tiles, long standing_cnt, bool (&waiting_table)[TILE_TABLE_COUNT]);
bool is_seven_pairs_win(const tile_t *standing_tiles, long standing_cnt, tile_t test_tile);

int thirteen_orphans_wait_step(const tile_t *standing_tiles, long standing_cnt, bool (&useful_table)[TILE_TABLE_COUNT]);
bool is_thirteen_orphans_wait(const tile_t *standing_tiles, long standing_cnt, bool (&waiting_table)[TILE_TABLE_COUNT]);
bool is_thirteen_orphans_win(const tile_t *standing_tiles, long standing_cnt, tile_t test_tile);

int knitted_straight_in_basic_type_wait_step(const tile_t *standing_tiles, long standing_cnt, bool (&useful_table)[TILE_TABLE_COUNT]);
bool is_knitted_straight_in_basic_type_wait(const tile_t *standing_tiles, long standing_cnt, bool (&waiting_table)[TILE_TABLE_COUNT]);
bool is_knitted_straight_in_basic_type_win(const tile_t *standing_tiles, long standing_cnt, tile_t test_tile);

int honors_and_knitted_tiles_wait_step(const tile_t *standing_tiles, long standing_cnt, bool (&useful_table)[TILE_TABLE_COUNT]);
bool is_honors_and_knitted_tiles_wait(const tile_t *standing_tiles, long standing_cnt, bool (&waiting_table)[TILE_TABLE_COUNT]);
bool is_honors_and_knitted_tiles_win(const tile_t *standing_tiles, long standing_cnt, tile_t test_tile);

// 组合龙只有6种
// 147m 258s 369p
// 147m 369s 258p
// 258m 147s 369p
// 258m 369s 147p
// 369m 147s 258p
// 369m 258s 147p
static const tile_t standard_knitted_straight[6][9] = {
    { TILE_1m, TILE_4m, TILE_7m, TILE_2s, TILE_5s, TILE_8s, TILE_3p, TILE_6p, TILE_9p },
    { TILE_1m, TILE_4m, TILE_7m, TILE_3s, TILE_6s, TILE_9s, TILE_2p, TILE_5p, TILE_8p },
    { TILE_2m, TILE_5m, TILE_8m, TILE_1s, TILE_4s, TILE_7s, TILE_3p, TILE_6p, TILE_9p },
    { TILE_2m, TILE_5m, TILE_8m, TILE_3s, TILE_6s, TILE_9s, TILE_1p, TILE_4p, TILE_7p },
    { TILE_3m, TILE_6m, TILE_9m, TILE_1s, TILE_4s, TILE_7s, TILE_2p, TILE_5p, TILE_8p },
    { TILE_3m, TILE_6m, TILE_9m, TILE_2s, TILE_5s, TILE_8s, TILE_1p, TILE_4p, TILE_7p },
};

// 十三幺13面听
static const tile_t standard_thirteen_orphans[13] = {
    TILE_1m, TILE_9m, TILE_1s, TILE_9s, TILE_1p, TILE_9p, TILE_E, TILE_S, TILE_W, TILE_N, TILE_C, TILE_F, TILE_P
};

#define FORM_FLAG_BASIC_TYPE                0x01
#define FORM_FLAG_SEVEN_PAIRS               0x02
#define FORM_FLAG_THIRTEEN_ORPHANS          0x04
#define FORM_FLAG_HONORS_AND_KNITTED_TILES  0x08
#define FORM_FLAG_KNITTED_STRAIGHT          0x10
#define FORM_FLAG_ALL                       0xFF

struct enum_result_t {
    tile_t discard_tile;  // 打这张牌
    uint8_t form_flag;  // 和牌形式
    int wait_step;  // 上听数
    bool useful_table[TILE_TABLE_COUNT];  // 有效牌标记
};

// 返回false可提前结束枚举
typedef bool (*enum_callback_t)(void *context, const enum_result_t *result);

// 枚举打哪张牌
void enum_discard_tile(const hand_tiles_t *hand_tiles, tile_t serving_tile, uint8_t form_flag,
    void *context, enum_callback_t enum_callback);

}

#endif
