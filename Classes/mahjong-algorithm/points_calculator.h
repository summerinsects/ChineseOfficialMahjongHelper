#ifndef _POINTS_CALCULATOR_H_
#define _POINTS_CALCULATOR_H_

#include "tile.h"

enum class SET_TYPE : uint8_t {
    NONE = 0,
    CHOW = 1,
    PUNG = 2,
    KONG = 3,
    PAIR = 4
};

static const char *set_type_name[] = { "NONE", "CHOW", "PUNG", "KONG", "PAIR" };

struct SET {
    bool is_melded;
    SET_TYPE set_type;
    TILE mid_tile;
};

static bool is_set_contains_tile(const SET &set, TILE tile) {
    //assert(set.set_type != SET_TYPE::NONE);
    if (set.set_type == SET_TYPE::CHOW) {
        return (set.mid_tile - 1 == tile
            || set.mid_tile == tile
            || set.mid_tile + 1 == tile);
    }
    else {
        return set.mid_tile == tile;
    }
}

static forceinline bool set_cmp(const SET &set1, const SET &set2) {
    return ((set1.mid_tile < set2.mid_tile)
        || (set1.mid_tile == set2.mid_tile && set1.set_type < set2.set_type));
}

static const char *stringify_table[] = {
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "1m", "2m", "3m", "4m", "5m", "6m", "7m", "8m", "9m", "", "", "", "", "", "",
    "", "1s", "2s", "3s", "4s", "5s", "6s", "7s", "8s", "9s", "", "", "", "", "", "",
    "", "1p", "2p", "3p", "4p", "5p", "6p", "7p", "8p", "9p", "", "", "", "", "", "",
    "", "E", "S", "W", "N", "", "", "", "", "", "", "", "", "", "", "",
    "", "C", "F", "P", "", "", "", "", "", "", "", "", "", "", "", "",
};

const char *parse_tiles(const char *str, TILE *tiles, long *out_tile_cnt);
bool string_to_tiles(const char *str, SET *fixed_sets, long *fixed_set_cnt, TILE *concealed_tiles, long *concealed_cnt);

enum POINT_TYPE {
    NONE = 0,
    BIG_FOUR_WINDS = 1, BIG_THREE_DRAGONS, ALL_GREEN, NINE_GATES, FOUR_KONGS, SEVEN_SHIFTED_PAIRS, THIRTEEN_ORPHANS,
    ALL_TERMINALS, LITTLE_FOUR_WINDS, LITTLE_THREE_DRAGONS, ALL_HONORS, FOUR_CONCEALED_PUNGS, PURE_TERMINAL_CHOWS,
    QUADRUPLE_CHOW, FOUR_PURE_SHIFTED_PUNGS,
    FOUR_PURE_SHIFTED_CHOWS, THREE_KONGS, ALL_TERMINALS_AND_HONORS,
    SEVEN_PAIRS, GREATER_HONORS_AND_KNITTED_TILES, ALL_EVEN_PUNGS, FULL_FLUSH, PURE_TRIPLE_CHOW, PURE_SHIFTED_PUNGS, UPPER_TILES, MIDDLE_TILES, LOWER_TILES,
    PURE_STRAIGHT, THREE_SUITED_TERMINAL_CHOWS, PURE_SHIFTED_CHOWS, ALL_FIVE, TRIPLE_PUNG, THREE_CONCEALED_PUNGS,
    LESSER_HONORS_AND_KNITTED_TILES, KNITTED_STRAIGHT, UPPER_FOUR, LOWER_FOUR, BIG_THREE_WINDS,
    MIXED_STRAIGHT, REVERSIBLE_TILES, MIXED_TRIPLE_CHOW, MIXED_SHIFTED_PUNGS, CHICKEN_HAND, LAST_TILE_DRAW, LAST_TILE_CLAIM, OUT_WITH_REPLACEMENT_TILE, ROBBING_THE_KONG, TWO_CONCEALED_KONGS,
    ALL_PUNGS, HALF_FLUSH, MIXED_SHIFTED_CHOWS, ALL_TYPES, MELDED_HAND, TWO_DRAGONS_PUNGS,
    OUTSIDE_HAND, FULLY_CONCEALED_HAND, TWO_MELDED_KONGS, LAST_TILE,
    DRAGON_PUNG, PREVALENT_WIND, SEAT_WIND, CONCEALED_HAND, ALL_CHOWS, TILE_HOG, DOUBLE_PUNG, TWO_CONCEALED_PUNGS, CONCEALED_KONG, ALL_SIMPLES,
    PURE_DOUBLE_CHOW, MIXED_DOUBLE_CHOW, SHORT_STRAIGHT, TWO_TERMINAL_CHOWS, PUNG_OF_TERMINALS_OR_HONORS, MELDED_KONG, ONE_VOIDED_SUIT, NO_HONORS, EDGE_WAIT, CLOSED_WAIT, SINGLE_WAIT, SELF_DRAWN,
    FLOWER_TILES
};

enum class WIND_TYPE {
    EAST, SOUTH, WEST, NORTH
};

typedef uint8_t WIN_TYPE;

#define WIN_TYPE_DISCARD 0
#define WIN_TYPE_SELF_DRAWN 1
#define WIN_TYPE_4TH_TILE 2
#define WIN_TYPE_ABOUT_KONG 4
#define WIN_TYPE_WALL_LAST 8

#define MAX_SEPARAION_CNT 10

int calculate_points(const SET *fixed_set, long fixed_cnt, const TILE *concealed_tiles, long concealed_cnt, TILE win_tile, WIN_TYPE win_type, WIND_TYPE prevalent_wind, WIND_TYPE seat_wind, long *points_table);


#if 0

static const char *points_name[] = {
    "None",
    "Big Four Winds", "Big Three Dragons", "All Green", "Nine Gates", "Four Kongs", "Seven Shifted Pairs", "Thirteen Orphans",
    "All Terminals", "Little Four Winds", "Little Three Dragons", "All Honors", "Four Concealed Pungs", "Pure Terminal Chows",
    "Quadruple Chow", "Four Pure Shifted Pungs",
    "Four Pure Shifted Chows", "Three Kongs", "All Terminals and Honors",
    "Seven Pairs", "Greater Honors and Knitted Tiles", "All Even Pungs", "Full Flush", "Pure Triple Chow", "Pure Shifted Pungs", "Upper Tiles", "Middle Tiles", "Lower Tiles",
    "Pure Straight", "Three-Suited Terminal Chows", "Pure Shifted Chows", "All Five", "Triple Pung", "Three Concealed Pungs",
    "Lesser Honors and Knitted Tiles", "Knitted Straight", "Upper Four", "Lower Four", "Big Three Winds",
    "Mixed Straight", "Reversible Tiles", "Mixed Triple Chow", "Mixed Shifted Pungs", "Chicken Hand", "Last Tile Draw", "Last Tile Claim", "Out with Replacement Tile", "Robbing The Kong", "Two Concealed Kongs",
    "All Pungs", "Half Flush", "Mixed Shifted Chows", "All Types", "Melded Hand", "Two Dragons Pungs",
    "Outside Hand", "Fully Concealed Hand", "Two Melded Kongs", "Last Tile",
    "Dragon Pung", "Prevalent Wind", "Seat Wind", "Concealed Hand", "All Chows", "Tile Hog", "Double Pung",
    "Two Concealed Pungs", "Concealed Kong", "All Simples",
    "Pure Double Chow", "Mixed Double Chow", "Short Straight", "Two Terminal Chows", "Pung of Terminals or Honors", "Melded Kong", "One Voided Suit", "No Honors", "Edge Wait", "Closed Wait", "Single Wait", "Self-Drawn",
    "Flower Tiles"
};

#else

static const char *points_name[] = {
    "无",
    "大四喜", "大三元", "绿一色", "九莲宝灯", "四杠", "连七对", "十三幺",
    "清幺九", "小四喜", "小三元", "字一色", "四暗刻", "一色双龙会",
    "一色四同顺", "一色四节高",
    "一色四步高", "三杠", "混幺九",
    "七对", "七星不靠", "全双刻", "清一色", "一色三同顺", "一色三节高", "全大", "全中", "全小",
    "清龙", "三色双龙会", "一色三步高", "全带五", "三同刻", "三暗刻",
    "全不靠", "组合龙", "大于五", "小于五", "三风刻",
    "花龙", "推不倒", "三色三同顺", "三色三节高", "无番和", "妙手回春", "海底捞月", "杠上开花", "抢杠和", "双暗杠",
    "碰碰和", "混一色", "三色三步高", "五门齐", "全求人", "双箭刻",
    "全带幺", "不求人", "双明杠", "和绝张",
    "箭刻", "圈风刻", "门风刻", "门前清", "平和", "四归一", "双同刻", "双暗刻", "暗杠", "断幺",
    "一般高", "喜相逢", "连六", "老少副", "幺九刻", "明杠", "缺一门", "无字", "边张", "坎张", "单钓将", "自摸",
    "花牌"
};

#endif

static const int points_value_table[FLOWER_TILES + 1] = {
    0,
    88, 88, 88, 88, 88, 88, 88,
    64, 64, 64, 64, 64, 64,
    48, 48,
    32, 32, 32,
    24, 24, 24, 24, 24, 24, 24, 24, 24,
    16, 16, 16, 16, 16, 16,
    12, 12, 12, 12, 12,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    6, 6, 6, 6, 6, 6,
    4, 4, 4, 4,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1
};

#endif
