#ifndef _WAIT_AND_WIN_TEST_H_
#define _WAIT_AND_WIN_TEST_H_

#include "tile.h"

namespace mahjong {

bool is_basic_type_1_wait(const TILE *standing_tiles, bool (&waiting_table)[6][10]);
bool is_basic_type_1_win(const TILE *standing_tiles, TILE test_tile);

bool is_basic_type_4_wait(const TILE *standing_tiles, bool (&waiting_table)[6][10]);
bool is_basic_type_4_win(const TILE *standing_tiles, TILE test_tile);

bool is_basic_type_7_wait(const TILE *standing_tiles, bool (&waiting_table)[6][10]);
bool is_basic_type_7_win(const TILE *standing_tiles, TILE test_tile);

bool is_basic_type_10_wait(const TILE *standing_tiles, bool (&waiting_table)[6][10]);
bool is_basic_type_10_win(const TILE *standing_tiles, TILE test_tile);

bool is_basic_type_13_wait(const TILE *standing_tiles, bool (&waiting_table)[6][10]);
bool is_basic_type_13_win(const TILE *standing_tiles, TILE test_tile);

bool is_seven_pairs_wait(const TILE (&standing_tiles)[13], TILE *waiting);
bool is_seven_pairs_win(const TILE (&standing_tiles)[13], TILE test_tile);

bool is_thirteen_orphans_wait(const TILE (&standing_tiles)[13], TILE *waiting, unsigned *waiting_cnt);
bool is_thirteen_orphans_win(const TILE (&standing_tiles)[13], TILE test_tile);

bool is_honors_and_knitted_tiles_wait(const TILE (&standing_tiles)[13], TILE *waiting);
bool is_honors_and_knitted_tiles_win(const TILE (&standing_tiles)[13], TILE test_tile);



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

int basic_type_wait_step(const TILE *standing_tiles, long standing_cnt);
int seven_pairs_wait_step(const TILE *standing_tiles, long standing_cnt);
int thirteen_orphans_wait_step(const TILE *standing_tiles, long standing_cnt);
int honors_and_knitted_tiles_wait_step(const TILE *standing_tiles, long standing_cnt);

template <class _InputIterator1, class _InputIterator2, class _OutputIterator>
_OutputIterator copy_exclude(_InputIterator1 src_first, _InputIterator1 src_last,
    _InputIterator2 fliter_first, _InputIterator2 fliter_last, _OutputIterator dest) {
    while (src_first != src_last) {
        if (fliter_first != fliter_last && *src_first == *fliter_first) {
            ++fliter_first;
        }
        else {
            *dest = *src_first;
            ++dest;
        }
        ++src_first;
    }
    return dest;
}

}

#endif
