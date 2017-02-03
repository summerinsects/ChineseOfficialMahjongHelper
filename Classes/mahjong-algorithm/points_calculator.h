#ifndef _POINTS_CALCULATOR_H_
#define _POINTS_CALCULATOR_H_

#include "tile.h"

#define HAS_CONCEALED_KONG_AND_MELDED_KONG  1  // 支持明暗杠

namespace mahjong {

/**
 * @brief 番种
 */
enum fan_t {
    NONE = 0,
    BIG_FOUR_WINDS = 1, BIG_THREE_DRAGONS, ALL_GREEN, NINE_GATES, FOUR_KONGS, SEVEN_SHIFTED_PAIRS, THIRTEEN_ORPHANS,
    ALL_TERMINALS, LITTLE_FOUR_WINDS, LITTLE_THREE_DRAGONS, ALL_HONORS, FOUR_CONCEALED_PUNGS, PURE_TERMINAL_CHOWS,
    QUADRUPLE_CHOW, FOUR_PURE_SHIFTED_PUNGS,
    FOUR_PURE_SHIFTED_CHOWS, THREE_KONGS, ALL_TERMINALS_AND_HONORS,
    SEVEN_PAIRS, GREATER_HONORS_AND_KNITTED_TILES, ALL_EVEN_PUNGS, FULL_FLUSH, PURE_TRIPLE_CHOW, PURE_SHIFTED_PUNGS, UPPER_TILES, MIDDLE_TILES, LOWER_TILES,
    PURE_STRAIGHT, THREE_SUITED_TERMINAL_CHOWS, PURE_SHIFTED_CHOWS, ALL_FIVE, TRIPLE_PUNG, THREE_CONCEALED_PUNGS,
    LESSER_HONORS_AND_KNITTED_TILES, KNITTED_STRAIGHT, UPPER_FOUR, LOWER_FOUR, BIG_THREE_WINDS,
    MIXED_STRAIGHT, REVERSIBLE_TILES, MIXED_TRIPLE_CHOW, MIXED_SHIFTED_PUNGS, CHICKEN_HAND, LAST_TILE_DRAW, LAST_TILE_CLAIM, OUT_WITH_REPLACEMENT_TILE, ROBBING_THE_KONG,
    ALL_PUNGS, HALF_FLUSH, MIXED_SHIFTED_CHOWS, ALL_TYPES, MELDED_HAND, TWO_CONCEALED_KONGS, TWO_DRAGONS_PUNGS,
#if HAS_CONCEALED_KONG_AND_MELDED_KONG
    CONCEALED_KONG_AND_MELDED_KONG,
#endif
    OUTSIDE_HAND, FULLY_CONCEALED_HAND, TWO_MELDED_KONGS, LAST_TILE,
    DRAGON_PUNG, PREVALENT_WIND, SEAT_WIND, CONCEALED_HAND, ALL_CHOWS, TILE_HOG, DOUBLE_PUNG, TWO_CONCEALED_PUNGS, CONCEALED_KONG, ALL_SIMPLES,
    PURE_DOUBLE_CHOW, MIXED_DOUBLE_CHOW, SHORT_STRAIGHT, TWO_TERMINAL_CHOWS, PUNG_OF_TERMINALS_OR_HONORS, MELDED_KONG, ONE_VOIDED_SUIT, NO_HONORS, EDGE_WAIT, CLOSED_WAIT, SINGLE_WAIT, SELF_DRAWN,
    FLOWER_TILES,
    FAN_COUNT
};

/**
 * @brief 风（用来表示圈风门风）
 */
enum class wind_t {
    EAST, SOUTH, WEST, NORTH
};

/**
 * @brief 和牌类型
 */
typedef uint8_t win_type_t;

#define WIN_TYPE_DISCARD 0  // 点和
#define WIN_TYPE_SELF_DRAWN 1  // 自摸
#define WIN_TYPE_4TH_TILE 2  // 绝张
#define WIN_TYPE_ABOUT_KONG 4  // 关于杠，复合点和时为枪杠和，复合自摸则为杠上开花
#define WIN_TYPE_WALL_LAST 8  // 牌墙最后一张，复合点和时为海底捞月，复合自摸则为妙手回春

#define ERROR_WRONG_TILES_COUNT -1
#define ERROR_TILE_COUNT_GREATER_THAN_4 -2
#define ERROR_NOT_WIN -3

/**
 * @brief 检查算番的输入是否合法
 *
 *
 * @param [in] hand_tiles 手牌信息
 * @param [in] win_tile 和牌张
 * @return 返回0表示成功，否则返回上述错误码
 */
int check_calculator_input(const hand_tiles_t *hand_tiles, tile_t win_tile);

/**
 * @brief 附加信息
 */
struct extra_condition_t {
    win_type_t win_type;    ///< 和牌类型
    wind_t prevalent_wind;  ///< 圈风
    wind_t seat_wind;       ///< 门风
};

/**
 * @brief 算番
 *
 * @param [in] hand_tiles 手牌信息
 * @param [in] win_tile 和牌张
 * @param [in] ext_cond 附加信息
 * @param [out] fan_table 番表，当有某种番时，相应的会设置为这种番出现的次数
 * @return 番值
 */
int calculate_points(const hand_tiles_t *hand_tiles, tile_t win_tile, const extra_condition_t *ext_cond, long (&fan_table)[FAN_COUNT]);

#if 0

/**
 * @brief 番名（英文）
 */
static const char *fan_name[] = {
    "None",
    "Big Four Winds", "Big Three Dragons", "All Green", "Nine Gates", "Four Kongs", "Seven Shifted Pairs", "Thirteen Orphans",
    "All Terminals", "Little Four Winds", "Little Three Dragons", "All Honors", "Four Concealed Pungs", "Pure Terminal Chows",
    "Quadruple Chow", "Four Pure Shifted Pungs",
    "Four Pure Shifted Chows", "Three Kongs", "All Terminals and Honors",
    "Seven Pairs", "Greater Honors and Knitted Tiles", "All Even Pungs", "Full Flush", "Pure Triple Chow", "Pure Shifted Pungs", "Upper Tiles", "Middle Tiles", "Lower Tiles",
    "Pure Straight", "Three-Suited Terminal Chows", "Pure Shifted Chows", "All Five", "Triple Pung", "Three Concealed Pungs",
    "Lesser Honors and Knitted Tiles", "Knitted Straight", "Upper Four", "Lower Four", "Big Three Winds",
    "Mixed Straight", "Reversible Tiles", "Mixed Triple Chow", "Mixed Shifted Pungs", "Chicken Hand", "Last Tile Draw", "Last Tile Claim", "Out with Replacement Tile", "Robbing The Kong",
    "All Pungs", "Half Flush", "Mixed Shifted Chows", "All Types", "Melded Hand", "Two Concealed Kongs", "Two Dragons Pungs",
    "Outside Hand", "Fully Concealed Hand", "Two Melded Kongs", "Last Tile",
    "Dragon Pung", "Prevalent Wind", "Seat Wind", "Concealed Hand", "All Chows", "Tile Hog", "Double Pung",
#if HAS_CONCEALED_KONG_AND_MELDED_KONG
    "Concealed Kong and Melded Kong",
#endif
    "Two Concealed Pungs", "Concealed Kong", "All Simples",
    "Pure Double Chow", "Mixed Double Chow", "Short Straight", "Two Terminal Chows", "Pung of Terminals or Honors", "Melded Kong", "One Voided Suit", "No Honors", "Edge Wait", "Closed Wait", "Single Wait", "Self-Drawn",
    "Flower Tiles"
};

#else

#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif

/**
 * @brief 番名（简体中文）
 */
static const char *fan_name[] = {
    "无",
    "大四喜", "大三元", "绿一色", "九莲宝灯", "四杠", "连七对", "十三幺",
    "清幺九", "小四喜", "小三元", "字一色", "四暗刻", "一色双龙会",
    "一色四同顺", "一色四节高",
    "一色四步高", "三杠", "混幺九",
    "七对", "七星不靠", "全双刻", "清一色", "一色三同顺", "一色三节高", "全大", "全中", "全小",
    "清龙", "三色双龙会", "一色三步高", "全带五", "三同刻", "三暗刻",
    "全不靠", "组合龙", "大于五", "小于五", "三风刻",
    "花龙", "推不倒", "三色三同顺", "三色三节高", "无番和", "妙手回春", "海底捞月", "杠上开花", "抢杠和",
    "碰碰和", "混一色", "三色三步高", "五门齐", "全求人", "双暗杠", "双箭刻",
#if HAS_CONCEALED_KONG_AND_MELDED_KONG
    "明暗杠",
#endif
    "全带幺", "不求人", "双明杠", "和绝张",
    "箭刻", "圈风刻", "门风刻", "门前清", "平和", "四归一", "双同刻", "双暗刻", "暗杠", "断幺",
    "一般高", "喜相逢", "连六", "老少副", "幺九刻", "明杠", "缺一门", "无字", "边张", "坎张", "单钓将", "自摸",
    "花牌"
};

#endif

/**
 * @brief 番值
 */
static const int fan_value_table[FAN_COUNT] = {
    0,
    88, 88, 88, 88, 88, 88, 88,
    64, 64, 64, 64, 64, 64,
    48, 48,
    32, 32, 32,
    24, 24, 24, 24, 24, 24, 24, 24, 24,
    16, 16, 16, 16, 16, 16,
    12, 12, 12, 12, 12,
    8, 8, 8, 8, 8, 8, 8, 8, 8,
    6, 6, 6, 6, 6, 6, 6,
#if HAS_CONCEALED_KONG_AND_MELDED_KONG
    5,
#endif
    4, 4, 4, 4,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1
};

/**
 * @brief 判断立牌是否包含和牌
 * 如果是，则必然不是和绝张
 *
 * @param [in] standing_tiles 立牌
 * @param [in] standing_cnt 立牌数
 * @param [in] win_tile 和牌张
 * @return bool
 */
bool is_standing_tiles_contains_win_tile(const tile_t *standing_tiles, long standing_cnt, tile_t win_tile);

/**
 * @brief 统计和牌在副露中出现的张数
 * 如果出现3张，则必然和绝张
 *
 * @param [in] fixed_pack 副露牌组
 * @param [in] standing_cnt 副露牌组数
 * @param [in] win_tile 和牌张
 * @return size_t
 */
size_t count_win_tile_in_fixed_packs(const pack_t *fixed_pack, long fixed_cnt, tile_t win_tile);

}

#endif
