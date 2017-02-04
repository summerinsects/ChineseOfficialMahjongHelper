#ifndef _TILE_H_
#define _TILE_H_

#include <stdint.h>
#include <algorithm>

#ifdef _MSC_VER  // for MSVC
#define forceinline __forceinline
#elif defined __GNUC__  // for gcc on Linux/Apple OS X
#define forceinline __inline__ __attribute__((always_inline))
#else
#define forceinline inline
#endif

namespace mahjong {

/**
 * @brief 花色
 */
typedef uint8_t suit_t;

/**
 * @brief 点数
 */
typedef uint8_t rank_t;

#define TILE_SUIT_NONE 0
#define TILE_SUIT_CHARACTERS 1
#define TILE_SUIT_BAMBOO 2
#define TILE_SUIT_DOTS 3
#define TILE_SUIT_HONORS 4

/**
 * @brief 牌
 *
 * 合法的牌为：
 *  0x11 - 0x19 万子（CHARACTERS）
 *  0x21 - 0x29 条子（BAMBOO）
 *  0x31 - 0x39 饼子（DOTS）
 *  0x41 - 0x47 字牌（HONORS）
 */
typedef uint8_t tile_t;

/**
 * @brief 生成一张牌
 *  函数不检查输入的合法性，不保证返回值的合法性
 * @param [in] suit 花色
 * @param [in] rank 点数
 * @return tile_t 牌
 */
static forceinline tile_t make_tile(suit_t suit, rank_t rank) {
    return (((suit & 0xF) << 4) | (rank & 0xF));
}

/**
 * @brief 获取牌的花色
 *  函数不检查输入的合法性，不保证返回值的合法性
 * @param [in] tile 牌
 * @return suit_t 花色
 */
static forceinline suit_t tile_suit(tile_t tile) {
    return ((tile >> 4) & 0xF);
}

/**
 * @brief 获取牌的点数
 *  函数不检查输入的合法性，不保证返回值的合法性
 * @param [in] tile 牌
 * @return rank_t 点数
 */
static forceinline rank_t tile_rank(tile_t tile) {
    return (tile & 0xF);
}

/**
 * @brief 排序牌
 * @param [in] tiles 牌
 * @param [in] cnt 牌的数量
 */
static forceinline void sort_tiles(tile_t *tiles, long cnt) {
    std::sort(tiles, tiles + cnt);
}

/**
 * @brief 所有牌的值
 */
enum tile_value_t {
    TILE_1m = 0x11, TILE_2m, TILE_3m, TILE_4m, TILE_5m, TILE_6m, TILE_7m, TILE_8m, TILE_9m,
    TILE_1s = 0x21, TILE_2s, TILE_3s, TILE_4s, TILE_5s, TILE_6s, TILE_7s, TILE_8s, TILE_9s,
    TILE_1p = 0x31, TILE_2p, TILE_3p, TILE_4p, TILE_5p, TILE_6p, TILE_7p, TILE_8p, TILE_9p,
    TILE_E  = 0x41, TILE_S , TILE_W , TILE_N , TILE_C , TILE_F , TILE_P ,
    TILE_TABLE_SIZE
};

/**
 * @brief 所有合法的牌
 */
static const tile_t all_tiles[] = {
    TILE_1m, TILE_2m, TILE_3m, TILE_4m, TILE_5m, TILE_6m, TILE_7m, TILE_8m, TILE_9m,
    TILE_1s, TILE_2s, TILE_3s, TILE_4s, TILE_5s, TILE_6s, TILE_7s, TILE_8s, TILE_9s,
    TILE_1p, TILE_2p, TILE_3p, TILE_4p, TILE_5p, TILE_6p, TILE_7p, TILE_8p, TILE_9p,
    TILE_E , TILE_S , TILE_W , TILE_N , TILE_C , TILE_F , TILE_P
};

#define PACK_TYPE_NONE 0
#define PACK_TYPE_CHOW 1  ///< 顺子
#define PACK_TYPE_PUNG 2  ///< 刻子
#define PACK_TYPE_KONG 3  ///< 杠
#define PACK_TYPE_PAIR 4  ///< 雀头

/**
 * @brief 牌组
 * 用于表示一组面子或者雀头
 *
 * 内存结构：
 * 15---12----8----4----0
 * |meld |type|  tile   |
 * +-----+----+---------+
 *  melded 是否为明的（明顺、明刻、明杠）
 *  type 牌组类型
 *  tile 牌（对于顺子，为中间那张牌）
 */
typedef uint16_t pack_t;

/**
 * @brief 生成一个牌组
 *  函数不检查输入的合法性，不保证返回值的合法性
 * @param [in] melded 是否为明的（明顺、明刻、明杠）
 * @param [in] type 牌组类型
 * @param [in] tile 牌（对于顺子，为中间那张牌）
 */
static forceinline pack_t make_pack(bool melded, uint8_t type, tile_t tile) {
    return (melded << 12 | (type << 8) | tile);
}

/**
 * @brief 牌组是否为明的（明顺、明刻、明杠）
 *  函数不检查输入的合法性，不保证返回值的合法性
 * @param [in] pack 牌组
 * @return bool
 */
static forceinline bool is_pack_melded(pack_t pack) {
    return !!(pack >> 12);
}

/**
 * @brief 获取牌组的类型
 *  函数不检查输入的合法性，不保证返回值的合法性
 * @param [in] pack 牌组
 * @return uint8_t 牌组类型
 */
static forceinline uint8_t pack_type(pack_t pack) {
    return ((pack >> 8) & 0xF);
}

/**
 * @brief 获取牌的点数
 *  函数不检查输入的合法性，不保证返回值的合法性
 * @param [in] pack 牌组
 * @return tile_t 牌（对于顺子，为中间那张牌）
 */
static forceinline tile_t pack_tile(pack_t pack) {
    return (pack & 0xFF);
}

/**
 * @brief 手牌结构
 */
struct hand_tiles_t {
    pack_t fixed_packs[5];      ///< 副露的面子（包括暗杠）
    long pack_count;            ///< 副露的面子（包括暗杠）数
    tile_t standing_tiles[13];  ///< 立牌
    long tile_count;            ///< 立牌数
};

// 推不倒和绿一色属性
static const uint32_t traits_mask_table[256] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x03, 0x02, 0x03, 0x01, 0x03, 0x00, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

#define REVERSIBLE_BIT 0x01
#define GREEN_BIT 0x02

/**
 * @brief 判断是否为绿一色构成牌
 * @param [in] tile 牌
 * @return bool
 */
static bool forceinline is_green(tile_t tile) {
    //return (tile == 0x22 || tile == 0x23 || tile == 0x24 || tile == 0x26 || tile == 0x28 || tile == 0x52);
    return (traits_mask_table[tile] & GREEN_BIT) != 0;
}

/**
 * @brief 判断是否为推不倒构成牌
 * @param [in] tile 牌
 * @return bool
 */
static bool forceinline is_reversible(tile_t tile) {
    //return (tile == 0x22 || tile == 0x24 || tile == 0x25 || tile == 0x26 || tile == 0x28 || tile == 0x29 ||
    //    tile == 0x31 || tile == 0x32 || tile == 0x33 || tile == 0x34 || tile == 0x35 || tile == 0x38 || tile == 0x39 ||
    //    tile == 0x53);
    return (traits_mask_table[tile] & REVERSIBLE_BIT);
}

#undef GREEN_BIT
#undef REVERSIBLE_BIT

/**
 * @brief 判断是否为数牌幺九（老头牌）
 * @param [in] tile 牌
 * @return bool
 */
static forceinline bool is_terminal(tile_t tile) {
    //return (tile == 0x11 || tile == 0x19 || tile == 0x21 || tile == 0x29 || tile == 0x31 || tile == 0x39);
    // 0xC7 : 1100 0111
    return ((tile & 0xC7) == 1 && (tile >> 4));
}

/**
 * @brief 判断是否为风牌
 * @param [in] tile 牌
 * @return bool
 */
static forceinline bool is_winds(tile_t tile) {
    return (tile > 0x40 && tile < 0x45);
}

/**
 * @brief 判断是否为箭牌（三元牌）
 * @param [in] tile 牌
 * @return bool
 */
static forceinline bool is_dragons(tile_t tile) {
    return (tile > 0x44 && tile < 0x48);
}

/**
 * @brief 判断是否为字牌
 * @param [in] tile 牌
 * @return bool
 */
static forceinline bool is_honor(tile_t tile) {
    return (tile > 0x40 && tile < 0x48);
}

/**
 * @brief 判断是否为数牌
 * @param [in] tile 牌
 * @return bool
 */
static forceinline bool is_numbered_suit(tile_t tile) {
    if (tile < 0x1A) return (tile > 0x10);
    if (tile < 0x2A) return (tile > 0x20);
    if (tile < 0x3A) return (tile > 0x30);
    return false;
}

/**
 * @brief 判断是否为数牌（更快，对于非法牌可能产生误判）
 * @see is_numbered_suit
 * @param [in] tile 牌
 * @return bool
 */
static forceinline bool is_numbered_suit_quick(tile_t tile) {
    return !(tile & 0xC0);
}

/**
 * @brief 判断是否为幺九牌（包括数牌幺九和字牌）
 * @param [in] tile 牌
 * @return bool
 */
static forceinline bool is_terminal_or_honor(tile_t tile) {
    return is_terminal(tile) || is_honor(tile);
}

/**
 * @brief 判断两张牌花色是否相同（更快，对于非法牌可能产生误判）
 * @param [in] tile0 牌0
 * @param [in] tile1 牌1
 * @return bool
 */
static forceinline bool is_suit_equal_quick(tile_t tile0, tile_t tile1) {
    return ((tile0 & 0xF0) == (tile1 & 0xF0));
}

/**
 * @brief 判断两张牌点数是否相同（更快，对于非法牌可能产生误判）
 * @param [in] tile0 牌0
 * @param [in] tile1 牌1
 * @return bool
 */
static forceinline bool is_rank_equal_quick(tile_t tile0, tile_t tile1) {
    return ((tile0 & 0xCF) == (tile1 & 0xCF));
}

}

#endif
