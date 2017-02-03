#ifndef __STRINGIFY_H__
#define __STRINGIFY_H__

#include "tile.h"

namespace mahjong {

static const char *stringify_table[] = {
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "1m", "2m", "3m", "4m", "5m", "6m", "7m", "8m", "9m", "", "", "", "", "", "",
    "", "1s", "2s", "3s", "4s", "5s", "6s", "7s", "8s", "9s", "", "", "", "", "", "",
    "", "1p", "2p", "3p", "4p", "5p", "6p", "7p", "8p", "9p", "", "", "", "", "", "",
    "", "E" , "S" , "W" , "N" , "C" , "F" , "P" , "", "", "", "", "", "", "", "",
};

#define PARSE_NO_ERROR 0
#define PARSE_ERROR_ILLEGAL_CHARACTER -1
#define PARSE_ERROR_NO_SUFFIX_AFTER_DIGIT -2
#define PARSE_ERROR_TOO_MANY_TILES_FOR_FIXED_PACK -3
#define PARSE_ERROR_CANNOT_MAKE_FIXED_PACK -4
#define PARSE_ERROR_TOO_MANY_FIXED_PACKS -5

long parse_tiles(const char *str, tile_t *tiles, long max_cnt);
long string_to_tiles(const char *str, hand_tiles_t *hand_tiles, tile_t *serving_tile);
long tiles_to_string(const tile_t *tiles, long tile_cnt, char *str, long max_size);
long packs_to_string(const pack_t *packs, long pack_cnt, char *str, long max_size);
long hand_tiles_to_string(const hand_tiles_t *hand_tiles, char *str, long max_size);

#define INPUT_GUIDE_STRING_1 "数牌：万=m 条=s 饼=p。后缀使用小写字母，一连串同花色的数牌可合并使用用一个后缀，如123m、678s等等。"
#define INPUT_GUIDE_STRING_2 "字牌：东南西北=ESWN，中发白=CFP。使用大写字母。亦可使用后缀z，但按中国习惯顺序567z为中发白。"
#define INPUT_GUIDE_STRING_3 "每组吃、碰、明杠之间用英文空格分隔，每一组暗杠前后用英文[]。副露与立牌之间也用英文空格分隔。"
}

#endif
