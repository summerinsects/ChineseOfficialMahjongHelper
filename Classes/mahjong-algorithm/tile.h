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

typedef uint8_t SUIT_TYPE;
typedef uint8_t RANK_TYPE;

//struct TILE {
//   SUIT_TYPE suit_type : 4;
//   RANK_TYPE rank_type : 4;
//};

#define TILE_SUIT_NONE 0
#define TILE_SUIT_CHARACTERS 1
#define TILE_SUIT_BAMBOO 2
#define TILE_SUIT_DOTS 3
#define TILE_SUIT_WINDS 4
#define TILE_SUIT_DRAGONS 5
#define TILE_SUIT_FLOWERS 6

// 0x11 - 0x19 CHARACTERS
// 0x21 - 0x29 BAMBOO
// 0x31 - 0x39 DOTS
// 0x41 - 0x44 WINDS
// 0x51 - 0x53 DRAGONS
// 0x61 - 0x68 FLOWERS
typedef uint8_t TILE;

static forceinline TILE make_tile(SUIT_TYPE suit, RANK_TYPE rank) {
    return (((suit & 0xF) << 4) | (rank & 0xF));
}

static forceinline SUIT_TYPE tile_suit(TILE tile) {
    return ((tile >> 4) & 0xF);
}

static forceinline RANK_TYPE tile_rank(TILE tile) {
    return (tile & 0xF);
}

static forceinline void sort_tiles(TILE *tiles, long cnt) {
    std::sort(tiles, tiles + cnt);
}

static const uint32_t traits_mask_table[256] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x03, 0x02, 0x03, 0x01, 0x03, 0x00, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

#define REVERSIBLE_BIT 0x01
#define GREEN_BIT 0x02

bool forceinline is_green(TILE tile) {
    //return (tile == 0x22 || tile == 0x23 || tile == 0x24 || tile == 0x26 || tile == 0x28 || tile == 0x52);
    return (traits_mask_table[tile] & GREEN_BIT) != 0;
}

bool forceinline is_reversible_tile(TILE tile) {
    //return (tile == 0x22 || tile == 0x24 || tile == 0x25 || tile == 0x26 || tile == 0x28 || tile == 0x29 ||
    //    tile == 0x31 || tile == 0x32 || tile == 0x33 || tile == 0x34 || tile == 0x35 || tile == 0x38 || tile == 0x39 ||
    //    tile == 0x53);
    return (traits_mask_table[tile] & REVERSIBLE_BIT);
}

static forceinline bool is_terminal(TILE tile) {
    //return (tile == 0x11 || tile == 0x19 || tile == 0x21 || tile == 0x29 || tile == 0x31 || tile == 0x39);
    // 0xC7 : 1100 0111
    return ((tile & 0xC7) == 1 && (tile >> 4));
}

static forceinline bool is_winds(TILE tile) {
    return (tile > 0x40 && tile < 0x45);
}

static forceinline bool is_dragons(TILE tile) {
    return (tile > 0x50 && tile < 0x54);
}

static forceinline bool is_honor(TILE tile) {
    if (tile < 0x45) return (tile > 0x40);
    if (tile < 0x54) return (tile > 0x50);
    return false;
}

static forceinline bool is_numbered_suit(TILE tile) {
    if (tile < 0x1A) return (tile > 0x10);
    if (tile < 0x2A) return (tile > 0x20);
    if (tile < 0x3A) return (tile > 0x30);
    return false;
}

static forceinline bool is_numbered_suit_quick(TILE tile) {
    return !(tile & 0xC0);
}

static forceinline bool is_terminal_or_honor(TILE tile) {
    return is_terminal(tile) || is_honor(tile);
}

static forceinline bool is_suit_equal_quick(TILE tile0, TILE tile1) {
    return ((tile0 & 0xF0) == (tile1 & 0xF0));
}

static forceinline bool is_rank_equal_quick(TILE tile0, TILE tile1) {
    return ((tile0 & 0xCF) == (tile1 & 0xCF));
}

static forceinline bool is_pung(TILE tile0, TILE tile1, TILE tile2) {
    return (tile0 == tile1 && tile1 == tile2);
}

// Notice that tiles in different suit is discontinuous,
// it is very easy to determine whether a chow can be made up
static forceinline bool is_chow(TILE tile0, TILE tile1, TILE tile2) {
    return (is_numbered_suit_quick(tile0) && tile0 + 1 == tile1 && tile1 + 1 == tile2);
}

bool forceinline is_concealed_set_completed(TILE tile0, TILE tile1, TILE tile2) {
    return is_chow(tile0, tile1, tile2) || is_pung(tile0, tile1, tile2);
}

#endif
