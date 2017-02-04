#ifndef _WAIT_AND_WIN_TEST_H_
#define _WAIT_AND_WIN_TEST_H_

#include "tile.h"

namespace mahjong {

/**
 * @brief 牌组转换成牌
 *
 * @param [in] packs 牌组
 * @param [in] pack_cnt 牌组的数量
 * @param [out] tiles 牌
 * @param [in] tile_cnt 牌的最大数量
 * @return long 牌的实际数量
 */
long packs_to_tiles(const pack_t *packs, long pack_cnt, tile_t *tiles, long tile_cnt);

/**
 * @brief 将牌打表
 *
 * @param [in] tiles 牌
 * @param [in] cnt 牌的数量
 * @param [out] cnt_table 牌的数量表
 */
void map_tiles(const tile_t *tiles, long cnt, int (&cnt_table)[TILE_TABLE_SIZE]);

/**
 * @brief 将手牌打表
 *
 * @param [in] hand_tiles 手牌
 * @param [out] cnt_table 牌的数量表
 * @return bool 手牌结构是否正确。即是否符合：副露组数*3+立牌数=13
 */
bool map_hand_tiles(const hand_tiles_t *hand_tiles, int (&cnt_table)[TILE_TABLE_SIZE]);

/**
 * @brief 将表转换成牌
 *
 * @param [in] cnt_table 牌的数量表
 * @param [out] tiles 牌
 * @param [in] max_cnt 牌的最大数量
 * @return long 牌的实际数量
 */
long table_to_tiles(const int (&cnt_table)[TILE_TABLE_SIZE], tile_t *tiles, long max_cnt);

/**
 * @brief 计数有效牌枚数
 *
 * @param [in] used_table 已经的使用牌的数量表
 * @param [in] useful_table 有效牌标记表
 * @return int 有效牌枚数
 */
int count_useful_tile(const int (&used_table)[TILE_TABLE_SIZE], const bool (&useful_table)[TILE_TABLE_SIZE]);

/**
 * @addtogroup shanten
 * @{
 */

/**
 * @brief 基本和型上听数
 *
 * @param [in] standing_tiles 立牌
 * @param [in] standing_cnt 立牌数
 * @param [out] useful_table 有效牌标记表（可为null)
 * @return int 上听数
 */
int basic_type_wait_step(const tile_t *standing_tiles, long standing_cnt, bool (*useful_table)[TILE_TABLE_SIZE]);

/**
 * @brief 基本和型是否听牌
 *
 * @param [in] standing_tiles 立牌
 * @param [in] standing_cnt 立牌数
 * @param [out] waiting_table 听牌标记表（可为null)
 * @return bool 是否听牌
 */
bool is_basic_type_wait(const tile_t *standing_tiles, long standing_cnt, bool (*waiting_table)[TILE_TABLE_SIZE]);

/**
 * @brief 基本和型是否和牌
 *
 * @param [in] standing_tiles 立牌
 * @param [in] standing_cnt 立牌数
 * @param [in] test_tile 测试的牌
 * @return bool 是否和牌
 */
bool is_basic_type_win(const tile_t *standing_tiles, long standing_cnt, tile_t test_tile);



/**
 * @brief 七对上听数
 *
 * @param [in] standing_tiles 立牌
 * @param [in] standing_cnt 立牌数
 * @param [out] useful_table 有效牌标记表（可为null)
 * @return int 上听数
 */
int seven_pairs_wait_step(const tile_t *standing_tiles, long standing_cnt, bool (*useful_table)[TILE_TABLE_SIZE]);

/**
 * @brief 七对是否听牌
 *
 * @param [in] standing_tiles 立牌
 * @param [in] standing_cnt 立牌数
 * @param [out] waiting_table 听牌标记表（可为null)
 * @return bool 是否听牌
 */
bool is_seven_pairs_wait(const tile_t *standing_tiles, long standing_cnt, bool (*waiting_table)[TILE_TABLE_SIZE]);

/**
 * @brief 七对是否和牌
 *
 * @param [in] standing_tiles 立牌
 * @param [in] standing_cnt 立牌数
 * @param [in] test_tile 测试的牌
 * @return bool 是否和牌
 */
bool is_seven_pairs_win(const tile_t *standing_tiles, long standing_cnt, tile_t test_tile);



/**
 * @brief 十三幺上听数
 *
 * @param [in] standing_tiles 立牌
 * @param [in] standing_cnt 立牌数
 * @param [out] useful_table 有效牌标记表（可为null)
 * @return int 上听数
 */
int thirteen_orphans_wait_step(const tile_t *standing_tiles, long standing_cnt, bool (*useful_table)[TILE_TABLE_SIZE]);

/**
 * @brief 十三幺是否听牌
 *
 * @param [in] standing_tiles 立牌
 * @param [in] standing_cnt 立牌数
 * @param [out] waiting_table 听牌标记表（可为null)
 * @return bool 是否听牌
 */
bool is_thirteen_orphans_wait(const tile_t *standing_tiles, long standing_cnt, bool (*waiting_table)[TILE_TABLE_SIZE]);

/**
 * @brief 十三幺是否和牌
 *
 * @param [in] standing_tiles 立牌
 * @param [in] standing_cnt 立牌数
 * @param [in] test_tile 测试的牌
 * @return bool 是否和牌
 */
bool is_thirteen_orphans_win(const tile_t *standing_tiles, long standing_cnt, tile_t test_tile);



/**
 * @brief 组合龙上听数
 *
 * @param [in] standing_tiles 立牌
 * @param [in] standing_cnt 立牌数
 * @param [out] useful_table 有效牌标记表（可为null)
 * @return int 上听数
 */
int knitted_straight_in_basic_type_wait_step(const tile_t *standing_tiles, long standing_cnt, bool (*useful_table)[TILE_TABLE_SIZE]);

/**
 * @brief 组合龙是否听牌
 *
 * @param [in] standing_tiles 立牌
 * @param [in] standing_cnt 立牌数
 * @param [out] waiting_table 听牌标记表（可为null)
 * @return bool 是否听牌
 */
bool is_knitted_straight_in_basic_type_wait(const tile_t *standing_tiles, long standing_cnt, bool (*waiting_table)[TILE_TABLE_SIZE]);

/**
 * @brief 组合龙是否和牌
 *
 * @param [in] standing_tiles 立牌
 * @param [in] standing_cnt 立牌数
 * @param [in] test_tile 测试的牌
 * @return bool 是否和牌
 */
bool is_knitted_straight_in_basic_type_win(const tile_t *standing_tiles, long standing_cnt, tile_t test_tile);



/**
 * @brief 全不靠上听数
 *
 * @param [in] standing_tiles 立牌
 * @param [in] standing_cnt 立牌数
 * @param [out] useful_table 有效牌标记表（可为null)
 * @return int 上听数
 */
int honors_and_knitted_tiles_wait_step(const tile_t *standing_tiles, long standing_cnt, bool (*useful_table)[TILE_TABLE_SIZE]);

/**
 * @brief 全不靠是否听牌
 *
 * @param [in] standing_tiles 立牌
 * @param [in] standing_cnt 立牌数
 * @param [out] waiting_table 听牌标记表（可为null)
 * @return bool 是否听牌
 */
bool is_honors_and_knitted_tiles_wait(const tile_t *standing_tiles, long standing_cnt, bool (*waiting_table)[TILE_TABLE_SIZE]);

/**
 * @brief 全不靠是否和牌
 *
 * @param [in] standing_tiles 立牌
 * @param [in] standing_cnt 立牌数
 * @param [in] test_tile 测试的牌
 * @return bool 是否和牌
 */
bool is_honors_and_knitted_tiles_win(const tile_t *standing_tiles, long standing_cnt, tile_t test_tile);

/**
 * @brief 组合龙
 *  只有如下6种
 *    147m 258s 369p
 *    147m 369s 258p
 *    258m 147s 369p
 *    258m 369s 147p
 *    369m 147s 258p
 *    369m 258s 147p
 */
static const tile_t standard_knitted_straight[6][9] = {
    { TILE_1m, TILE_4m, TILE_7m, TILE_2s, TILE_5s, TILE_8s, TILE_3p, TILE_6p, TILE_9p },
    { TILE_1m, TILE_4m, TILE_7m, TILE_3s, TILE_6s, TILE_9s, TILE_2p, TILE_5p, TILE_8p },
    { TILE_2m, TILE_5m, TILE_8m, TILE_1s, TILE_4s, TILE_7s, TILE_3p, TILE_6p, TILE_9p },
    { TILE_2m, TILE_5m, TILE_8m, TILE_3s, TILE_6s, TILE_9s, TILE_1p, TILE_4p, TILE_7p },
    { TILE_3m, TILE_6m, TILE_9m, TILE_1s, TILE_4s, TILE_7s, TILE_2p, TILE_5p, TILE_8p },
    { TILE_3m, TILE_6m, TILE_9m, TILE_2s, TILE_5s, TILE_8s, TILE_1p, TILE_4p, TILE_7p },
};

/**
 * @brief 十三幺13面听
 */
static const tile_t standard_thirteen_orphans[13] = {
    TILE_1m, TILE_9m, TILE_1s, TILE_9s, TILE_1p, TILE_9p, TILE_E, TILE_S, TILE_W, TILE_N, TILE_C, TILE_F, TILE_P
};

/**
 * @name form flags
 * @{
 *  解析牌的错误码
 */
#define FORM_FLAG_BASIC_TYPE                0x01  ///< 基本和型
#define FORM_FLAG_SEVEN_PAIRS               0x02  ///< 七对
#define FORM_FLAG_THIRTEEN_ORPHANS          0x04  ///< 十三幺
#define FORM_FLAG_HONORS_AND_KNITTED_TILES  0x08  ///< 全不靠
#define FORM_FLAG_KNITTED_STRAIGHT          0x10  ///< 组合龙
#define FORM_FLAG_ALL                       0xFF  ///< 全部和型
/**
 * @}
 */

/**
 * @brief 枚举打哪张牌的计算结果信息
 */
struct enum_result_t {
    tile_t discard_tile;                    ///< 打这张牌
    uint8_t form_flag;                      ///< 和牌形式
    int wait_step;                          ///< 上听数
    bool useful_table[TILE_TABLE_SIZE];     ///< 有效牌标记表
};

/**
 * @brief 枚举打哪张牌的计算回调函数
 *
 * @param [in] context 从enum_discard_tile传过来的context原样传回
 * @param [in] result 计算结果
 * @retval true 继续枚举
 * @retval false 结束枚举
 */
typedef bool (*enum_callback_t)(void *context, const enum_result_t *result);

/**
 * @brief 枚举打哪张牌
 *
 * @param [in] hand_tiles 手牌结构
 * @param [in] serving_tile 上牌（可为0，此时仅计算手牌的信息）
 * @param [in] form_flag 计算哪些和型
 * @param [in] context 用户自定义参数，将原样从回调函数传回
 * @param [in] enum_callback 回调函数
 */
void enum_discard_tile(const hand_tiles_t *hand_tiles, tile_t serving_tile, uint8_t form_flag,
    void *context, enum_callback_t enum_callback);

}

/**
 * end group
 * @}
 */

#endif
