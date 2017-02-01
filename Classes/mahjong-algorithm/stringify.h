#ifndef __STRINGIFY_H__
#define __STRINGIFY_H__

#include "tile.h"

namespace mahjong {

static const char *stringify_table[] = {
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "1m", "2m", "3m", "4m", "5m", "6m", "7m", "8m", "9m", "", "", "", "", "", "",
    "", "1s", "2s", "3s", "4s", "5s", "6s", "7s", "8s", "9s", "", "", "", "", "", "",
    "", "1p", "2p", "3p", "4p", "5p", "6p", "7p", "8p", "9p", "", "", "", "", "", "",
    "", "E", "S", "W", "N", "", "", "", "", "", "", "", "", "", "", "",
    "", "C", "F", "P", "", "", "", "", "", "", "", "", "", "", "", "",
};

#define PARSE_NO_ERROR 0
#define PARSE_ERROR_ILLEGAL_CHARACTER -1
#define PARSE_ERROR_NO_SUFFIX_AFTER_DIGIT -2
#define PARSE_ERROR_TOO_MANY_TILES_FOR_FIXED_SET -3
#define PARSE_ERROR_CANNOT_MAKE_FIXED_SET -4
#define PARSE_ERROR_TOO_MANY_FIXED_SET -5

long parse_tiles(const char *str, tile_t *tiles, long max_cnt);
long string_to_tiles(const char *str, hand_tiles_t *hand_tiles);

}

#endif
