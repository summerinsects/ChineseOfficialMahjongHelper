#include "stringify.h"

namespace mahjong {

static long parse_tiles_impl(const char *str, tile_t *tiles, long max_cnt, long *out_tile_cnt) {
    //if (strspn(str, "123456789mpsESWNCFP") != strlen(str)) {
    //    return PARSE_ERROR_ILLEGAL_CHARACTER;
    //}

    long tile_cnt = 0;

#define SET_SUIT_FOR_NUMBERED(value_)       \
    for (long i = tile_cnt; i > 0;) {       \
        if (tiles[--i] & 0xF0) break;       \
        tiles[i] |= value_;                 \
        } (void)0

    const char *p = str;
    for (; tile_cnt < max_cnt && *p != '\0'; ++p) {
        char c = *p;
        switch (c) {
        case '1': tiles[tile_cnt++] = 1; break;
        case '2': tiles[tile_cnt++] = 2; break;
        case '3': tiles[tile_cnt++] = 3; break;
        case '4': tiles[tile_cnt++] = 4; break;
        case '5': tiles[tile_cnt++] = 5; break;
        case '6': tiles[tile_cnt++] = 6; break;
        case '7': tiles[tile_cnt++] = 7; break;
        case '8': tiles[tile_cnt++] = 8; break;
        case '9': tiles[tile_cnt++] = 9; break;
        case 'm': SET_SUIT_FOR_NUMBERED(0x10); break;
        case 's': SET_SUIT_FOR_NUMBERED(0x20); break;
        case 'p': SET_SUIT_FOR_NUMBERED(0x30); break;
        case 'E': tiles[tile_cnt++] = TILE_E; break;
        case 'S': tiles[tile_cnt++] = TILE_S; break;
        case 'W': tiles[tile_cnt++] = TILE_W; break;
        case 'N': tiles[tile_cnt++] = TILE_N; break;
        case 'C': tiles[tile_cnt++] = TILE_C; break;
        case 'F': tiles[tile_cnt++] = TILE_F; break;
        case 'P': tiles[tile_cnt++] = TILE_P; break;
        default: goto parse_finish;
        }
    }

parse_finish:
    if (tile_cnt > 0 && !(tiles[tile_cnt - 1] & 0xF0)) {
        bool hasSuffix = false;
        for (; !hasSuffix && *p != '\0'; ++p) {
            char c = *p;
            switch (c) {
            case 'm': SET_SUIT_FOR_NUMBERED(0x10); hasSuffix = true; break;
            case 's': SET_SUIT_FOR_NUMBERED(0x20); hasSuffix = true; break;
            case 'p': SET_SUIT_FOR_NUMBERED(0x30); hasSuffix = true; break;
            default: break;
            }
        }

        if (!hasSuffix) {
            return PARSE_ERROR_NO_SUFFIX_AFTER_DIGIT;
        }
    }

#undef SET_SUIT_FOR_NUMBERED

    if (out_tile_cnt != nullptr) {
        *out_tile_cnt = tile_cnt;
    }
    return (p - str);
}

long parse_tiles(const char *str, tile_t *tiles, long max_cnt) {
    long tile_cnt;
    if (parse_tiles_impl(str, tiles, max_cnt, &tile_cnt) > 0) {
        return tile_cnt;
    }
    return 0;
}

static long make_fixed_pack(const tile_t *tiles, long tile_cnt, pack_t *pack) {
    if (tile_cnt > 0) {
        if (tile_cnt != 3 && tile_cnt != 4) {
            return PARSE_ERROR_TOO_MANY_TILES_FOR_FIXED_SET;
        }
        if (tile_cnt == 3) {
            if (tiles[0] == tiles[1] && tiles[1] == tiles[2]) {
                *pack = make_pack(true, PACK_TYPE_PUNG, tiles[0]);
            }
            else {
                tile_t temp[3] = { tiles[0], tiles[1], tiles[2] };
                std::sort(std::begin(temp), std::end(temp));
                if (temp[0] + 1 == temp[1] && temp[1] + 1 == temp[2]) {
                    *pack = make_pack(true, PACK_TYPE_CHOW, temp[1]);
                }
                else {
                    return PARSE_ERROR_CANNOT_MAKE_FIXED_SET;
                }
            }
        }
        else {
            if (tiles[0] != tiles[1] || tiles[1] != tiles[2] || tiles[2] != tiles[3]) {
                return PARSE_ERROR_CANNOT_MAKE_FIXED_SET;
            }
            *pack = make_pack(true, PACK_TYPE_KONG, tiles[0]);
        }
        return 1;
    }
    return 0;
}

long string_to_tiles(const char *str, hand_tiles_t *hand_tiles) {
    pack_t packs[4];
    long pack_cnt = 0;
    bool is_concealed_kong = false;
    tile_t tiles[13];
    long tile_cnt = 0;

    const char *p = str;
    while (char c = *p) {
        const char *q;
        switch (c) {
        case ' ': {
            if (pack_cnt > 4) {
                return PARSE_ERROR_TOO_MANY_FIXED_SET;
            }
            long ret = make_fixed_pack(tiles, tile_cnt, &packs[pack_cnt]);
            if (ret < 0) {
                return ret;
            }
            pack_cnt += ret;
            q = ++p;
            tile_cnt = 0;
            break;
        }
        case '[': {
            if (pack_cnt > 4) {
                return PARSE_ERROR_TOO_MANY_FIXED_SET;
            }
            long ret = make_fixed_pack(tiles, tile_cnt, &packs[pack_cnt]);
            if (ret < 0) {
                return ret;
            }
            pack_cnt += ret;
            q = ++p;
            is_concealed_kong = true;
            break;
        }
        case ']':
            if (!is_concealed_kong) {
                return PARSE_ERROR_ILLEGAL_CHARACTER;
            }
            if (tile_cnt != 4) {
                return PARSE_ERROR_TOO_MANY_TILES_FOR_FIXED_SET;
            }
            q = ++p;
            packs[pack_cnt] = make_pack(false, PACK_TYPE_KONG, tiles[0]);
            is_concealed_kong = false;
            ++pack_cnt;
            tile_cnt = 0;
            break;
        default: {
                long ret = parse_tiles_impl(p, tiles, 13, &tile_cnt);
                if (ret < 0) {
                    return ret;
                }
                if (ret == 0) {
                    return PARSE_ERROR_ILLEGAL_CHARACTER;
                }
                q = p + ret;
            }
            break;
        }
        p = q;
    }

    memcpy(hand_tiles->fixed_packs, packs, pack_cnt * sizeof(pack_t));
    hand_tiles->pack_count = pack_cnt;
    memcpy(hand_tiles->standing_tiles, tiles, tile_cnt * sizeof(tile_t));
    hand_tiles->tile_count = tile_cnt;

    return PARSE_NO_ERROR;
}

}

