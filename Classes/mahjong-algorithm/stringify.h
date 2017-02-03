#ifndef __STRINGIFY_H__
#define __STRINGIFY_H__

#include "tile.h"

namespace mahjong {

/**
 * @brief 解析牌的错误码
 */
#define PARSE_NO_ERROR 0                                // 无错误
#define PARSE_ERROR_ILLEGAL_CHARACTER -1                // 非法字符
#define PARSE_ERROR_NO_SUFFIX_AFTER_DIGIT -2            // 数字后面缺少后缀
#define PARSE_ERROR_TOO_MANY_TILES_FOR_FIXED_PACK -3    // 副露包含过多的牌
#define PARSE_ERROR_CANNOT_MAKE_FIXED_PACK -4           // 无法正确解析副露
#define PARSE_ERROR_TOO_MANY_FIXED_PACKS -5             // 过多组副露（一副合法手牌最多4副露）

/**
 * @brief 解析牌
 * @param [in] str 字符串
 * @param [out] tile 牌
 * @param [in] max_cnt 牌的最大数量
 * @return long 实际牌的数量。返回0表示失败
 */
long parse_tiles(const char *str, tile_t *tiles, long max_cnt);

/**
 * @brief 字符串转换为手牌结构和上牌
 * @param [in] str 字符串
 * @param [out] hand_tiles 手牌结构
 * @param [out] serving_tile 上的牌
 * @return long 返回错误码，成功时为PARSE_NO_ERROR
 */
long string_to_tiles(const char *str, hand_tiles_t *hand_tiles, tile_t *serving_tile);

/**
 * @brief 牌转换为字符串
 * @param [in] tiles 牌
 * @param [in] tile_cnt 牌的数量
 * @param [out] str 字符串
 * @param [in] max_size 字符串最大长度
 * @return long 写入的字符串数
 */
long tiles_to_string(const tile_t *tiles, long tile_cnt, char *str, long max_size);

/**
 * @brief 牌组转换为字符串
 * @param [in] packs 牌组
 * @param [in] pack_cnt 牌组的数量
 * @param [out] str 字符串
 * @param [in] max_size 字符串最大长度
 * @return long 写入的字符串数
 */
long packs_to_string(const pack_t *packs, long pack_cnt, char *str, long max_size);

/**
 * @brief 手牌结构转换为字符串
 * @param [in] hand_tiles 手牌结构
 * @param [out] str 字符串
 * @param [in] max_size 字符串最大长度
 * @return long 写入的字符串数
 */
long hand_tiles_to_string(const hand_tiles_t *hand_tiles, char *str, long max_size);

#define INPUT_GUIDE_STRING_1 "数牌：万=m 条=s 饼=p。后缀使用小写字母，一连串同花色的数牌可合并使用用一个后缀，如123m、678s等等。"
#define INPUT_GUIDE_STRING_2 "字牌：东南西北=ESWN，中发白=CFP。使用大写字母。亦可使用后缀z，但按中国习惯顺序567z为中发白。"
#define INPUT_GUIDE_STRING_3 "每组吃、碰、明杠之间用英文空格分隔，每一组暗杠前后用英文[]。副露与立牌之间也用英文空格分隔。"
}

#endif
