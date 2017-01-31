#ifndef _WAIT_AND_WIN_TEST_H_
#define _WAIT_AND_WIN_TEST_H_

#include "tile.h"

namespace mahjong {

void map_tiles(const TILE *tiles, long cnt, int (&cnt_table)[0x54]);
int count_contributing_tile(int (&used_table)[0x54], bool (&useful_table)[0x54]);

int basic_type_wait_step(const TILE *standing_tiles, long standing_cnt, bool (&useful_table)[0x54]);
bool is_basic_type_wait(const TILE *standing_tiles, long standing_cnt, bool (&waiting_table)[0x54]);
bool is_basic_type_win(const TILE *standing_tiles, long standing_cnt, TILE test_tile);

int seven_pairs_wait_step(const TILE *standing_tiles, long standing_cnt, bool (&useful_table)[0x54]);
bool is_seven_pairs_wait(const TILE *standing_tiles, long standing_cnt, bool (&waiting_table)[0x54]);
bool is_seven_pairs_win(const TILE *standing_tiles, long standing_cnt, TILE test_tile);

int thirteen_orphans_wait_step(const TILE *standing_tiles, long standing_cnt, bool (&useful_table)[0x54]);
bool is_thirteen_orphans_wait(const TILE *standing_tiles, long standing_cnt, bool (&waiting_table)[0x54]);
bool is_thirteen_orphans_win(const TILE *standing_tiles, long standing_cnt, TILE test_tile);

int knitted_straight_in_basic_type_wait_step(const TILE *standing_tiles, long standing_cnt, bool (&useful_table)[0x54]);
bool is_knitted_straight_in_basic_type_wait(const TILE *standing_tiles, long standing_cnt, bool (&waiting_table)[0x54]);
bool is_knitted_straight_in_basic_type_win(const TILE *standing_tiles, long standing_cnt, TILE test_tile);

int honors_and_knitted_tiles_wait_step(const TILE *standing_tiles, long standing_cnt, bool (&useful_table)[0x54]);
bool is_honors_and_knitted_tiles_wait(const TILE *standing_tiles, long standing_cnt, bool (&waiting_table)[0x54]);
bool is_honors_and_knitted_tiles_win(const TILE *standing_tiles, long standing_cnt, TILE test_tile);

// 组合龙只有6种
// 147m 258s 369p
// 147m 369s 258p
// 258m 147s 369p
// 258m 369s 147p
// 369m 147s 258p
// 369m 258s 147p
static const TILE standard_knitted_straight[6][9] = {
    { 0x11, 0x14, 0x17, 0x22, 0x25, 0x28, 0x33, 0x36, 0x39 },
    { 0x11, 0x14, 0x17, 0x23, 0x26, 0x29, 0x32, 0x35, 0x38 },
    { 0x12, 0x15, 0x18, 0x21, 0x24, 0x27, 0x33, 0x36, 0x39 },
    { 0x12, 0x15, 0x18, 0x23, 0x26, 0x29, 0x31, 0x34, 0x37 },
    { 0x13, 0x16, 0x19, 0x21, 0x24, 0x27, 0x32, 0x35, 0x38 },
    { 0x13, 0x16, 0x19, 0x22, 0x25, 0x28, 0x31, 0x34, 0x37 },
};

// 十三幺13面听
static const TILE standard_thirteen_orphans[13] = {
    0x11, 0x19, 0x21, 0x29, 0x31, 0x39, 0x41, 0x42, 0x43, 0x44, 0x51, 0x52, 0x53
};

// 九莲宝灯
static const TILE standard_nine_gates[3][13] = {
    { 0x11, 0x11, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x19, 0x19 },
    { 0x21, 0x21, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x29, 0x29 },
    { 0x31, 0x31, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x39, 0x39 }
};

}

#endif
