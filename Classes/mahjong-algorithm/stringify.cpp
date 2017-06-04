#include "stringify.h"
#include <string.h>

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

#define SET_SUIT_FOR_HONOR() \
    for (long i = tile_cnt; i > 0;) {       \
        if (tiles[--i] & 0xF0) break;       \
        if (tiles[i] > 7) return PARSE_ERROR_ILLEGAL_CHARACTER; \
        tiles[i] |= 0x40;                   \
        } (void)0

#define NO_SUFFIX_AFTER_DIGIT() (tile_cnt > 0 && !(tiles[tile_cnt - 1] & 0xF0))
#define CHECK_SUFFIX() if (NO_SUFFIX_AFTER_DIGIT()) return PARSE_ERROR_NO_SUFFIX_AFTER_DIGIT

    const char *p = str;
    for (; tile_cnt < max_cnt && *p != '\0'; ++p) {
        char c = *p;
        switch (c) {
        case '0': tiles[tile_cnt++] = 5; break;
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
        case 'z': SET_SUIT_FOR_HONOR(); break;
        case 'E': CHECK_SUFFIX(); tiles[tile_cnt++] = TILE_E; break;
        case 'S': CHECK_SUFFIX(); tiles[tile_cnt++] = TILE_S; break;
        case 'W': CHECK_SUFFIX(); tiles[tile_cnt++] = TILE_W; break;
        case 'N': CHECK_SUFFIX(); tiles[tile_cnt++] = TILE_N; break;
        case 'C': CHECK_SUFFIX(); tiles[tile_cnt++] = TILE_C; break;
        case 'F': CHECK_SUFFIX(); tiles[tile_cnt++] = TILE_F; break;
        case 'P': CHECK_SUFFIX(); tiles[tile_cnt++] = TILE_P; break;
        default: goto parse_finish;
        }
    }

parse_finish:
    // 一连串数字+后缀，但已经超过容量，那么放弃中间一部分数字，直接解析最近的后缀
    if (NO_SUFFIX_AFTER_DIGIT()) {
        const char *p1 = strpbrk(p, "mspz");
        if (p1 == nullptr) {
            return PARSE_ERROR_NO_SUFFIX_AFTER_DIGIT;
        }

        switch (*p1) {
        case 'm': SET_SUIT_FOR_NUMBERED(0x10); break;
        case 's': SET_SUIT_FOR_NUMBERED(0x20); break;
        case 'p': SET_SUIT_FOR_NUMBERED(0x30); break;
        case 'z': SET_SUIT_FOR_HONOR(); break;
        default: return PARSE_ERROR_NO_SUFFIX_AFTER_DIGIT;
        }
        p = p1 + 1;
    }

#undef SET_SUIT_FOR_NUMBERED
#undef SET_SUIT_FOR_HONOR
#undef NO_SUFFIX_AFTER_DIGIT
#undef CHECK_SUFFIX

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
            return PARSE_ERROR_WRONG_TILES_COUNT_FOR_FIXED_PACK;
        }
        if (tile_cnt == 3) {
            if (tiles[0] == tiles[1] && tiles[1] == tiles[2]) {
                *pack = make_pack(1, PACK_TYPE_PUNG, tiles[0]);  // TODO: 增加供牌信息
            }
            else {
                if (tiles[0] + 1 == tiles[1] && tiles[1] + 1 == tiles[2]) {
                    *pack = make_pack(1, PACK_TYPE_CHOW, tiles[1]);
                }
                else if (tiles[0] + 1 == tiles[2] && tiles[2] + 1 == tiles[1]) {
                    *pack = make_pack(1, PACK_TYPE_CHOW, tiles[2]);
                }
                else if (tiles[1] + 1 == tiles[0] && tiles[0] + 1 == tiles[2]) {
                    *pack = make_pack(2, PACK_TYPE_CHOW, tiles[0]);
                }
                else if (tiles[1] + 1 == tiles[2] && tiles[2] + 1 == tiles[0]) {
                    *pack = make_pack(3, PACK_TYPE_CHOW, tiles[2]);
                }
                else if (tiles[2] + 1 == tiles[0] && tiles[0] + 1 == tiles[1]) {
                    *pack = make_pack(2, PACK_TYPE_CHOW, tiles[0]);
                }
                else if (tiles[2] + 1 == tiles[1] && tiles[1] + 1 == tiles[0]) {
                    *pack = make_pack(3, PACK_TYPE_CHOW, tiles[1]);
                }
                else {
                    return PARSE_ERROR_CANNOT_MAKE_FIXED_PACK;
                }
            }
        }
        else {
            if (tiles[0] != tiles[1] || tiles[1] != tiles[2] || tiles[2] != tiles[3]) {
                return PARSE_ERROR_CANNOT_MAKE_FIXED_PACK;
            }
            *pack = make_pack(1, PACK_TYPE_KONG, tiles[0]);  // TODO: 增加供牌信息
        }
        return 1;
    }
    return 0;
}

long string_to_tiles(const char *str, hand_tiles_t *hand_tiles, tile_t *serving_tile) {
    size_t len = strlen(str);
    if (strspn(str, "0123456789mpszESWNCFP []") != len) {
        return PARSE_ERROR_ILLEGAL_CHARACTER;
    }

    pack_t packs[4];
    long pack_cnt = 0;
    bool is_concealed_kong = false;
    tile_t tiles[14];
    long tile_cnt = 0;

    const char *p = str;
    while (char c = *p) {
        const char *q;
        switch (c) {
        case ' ': {
            if (pack_cnt > 4) {
                return PARSE_ERROR_TOO_MANY_FIXED_PACKS;
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
                return PARSE_ERROR_TOO_MANY_FIXED_PACKS;
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
                return PARSE_ERROR_WRONG_TILES_COUNT_FOR_FIXED_PACK;
            }
            q = ++p;
            packs[pack_cnt] = make_pack(0, PACK_TYPE_KONG, tiles[0]);
            is_concealed_kong = false;
            ++pack_cnt;
            tile_cnt = 0;
            break;
        default: {
                long ret = parse_tiles_impl(p, tiles, 14, &tile_cnt);
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
    long max_cnt = 13 - pack_cnt * 3;
    if (tile_cnt > max_cnt) {
        memcpy(hand_tiles->standing_tiles, tiles, max_cnt * sizeof(tile_t));
        hand_tiles->tile_count = max_cnt;
        if (serving_tile != nullptr) {
            *serving_tile = tiles[max_cnt];
        }
    }
    else {
        memcpy(hand_tiles->standing_tiles, tiles, tile_cnt * sizeof(tile_t));
        hand_tiles->tile_count = tile_cnt;
        if (serving_tile != nullptr) {
            *serving_tile = 0;
        }
    }

    return PARSE_NO_ERROR;
}

long tiles_to_string(const tile_t *tiles, long tile_cnt, char *str, long max_size) {
    bool tenhon = false;
    char *p = str, *end = str + max_size;

    static const char suffix[] = "mspz";
    static const char honor_text[] = "ESWNCFP";
    suit_t last_suit = 0;
    for (long i = 0; i < tile_cnt && p < end; ++i) {
        tile_t t = tiles[i];
        suit_t s = tile_suit(t);
        rank_t r = tile_rank(t);
        if (s == 1 || s == 2 || s == 3) {  // 数牌
            if (r >= 1 && r <= 9) {  // 有效范围1-9
                if (last_suit != s && last_suit != 0) {  // 花色变了，加后缀
                    if (last_suit != 4 || tenhon) {
                        *p++ = suffix[last_suit - 1];
                    }
                }
                if (p < end) {
                    *p++ = '0' + r;  // 写入一个数字字符
                }
                last_suit = s;  // 记录花色
            }
        }
        else if (s == 4) {  // 字牌
            if (r >= 1 && r <= 7) {  // 有效范围1-7
                if (last_suit != s && last_suit != 0) {  // 花色变了，加后缀
                    if (last_suit != 4) {
                        *p++ = suffix[last_suit - 1];
                    }
                }
                if (p < end) {
                    if (tenhon) {  // 天凤式后缀
                        *p++ = '0' + r;  // 写入一个数字字符
                    }
                    else {
                        *p++ = honor_text[r - 1];  // 直接写入字牌相应字母
                    }
                    last_suit = s;
                }
            }
        }
    }

    // 写入过且还有空间，补充后缀
    if (p != str && p < end && (last_suit != 4 || tenhon)) {
        *p++ = suffix[last_suit - 1];
    }

    if (p < end) {
        *p = '\0';
    }
    return p - str;
}

long packs_to_string(const pack_t *packs, long pack_cnt, char *str, long max_size) {
    char *p = str, *end = str + max_size;
    tile_t temp[4];
    for (long i = 0; i < pack_cnt && p < end; ++i) {
        pack_t pack = packs[i];
        tile_t t = pack_tile(pack);
        uint8_t pt = pack_type(pack);
        switch (pt) {
        case PACK_TYPE_CHOW:
            temp[0] = t - 1; temp[1] = t; temp[2] = t + 1;
            p += tiles_to_string(temp, 3, p, end - p);
            if (p < end) *p++ = ' ';
            break;
        case PACK_TYPE_PUNG:
            temp[0] = t; temp[1] = t; temp[2] = t;
            p += tiles_to_string(temp, 3, p, end - p);
            if (p < end) *p++ = ' ';
            break;
        case PACK_TYPE_KONG:
            if (!is_pack_melded(pack) && p < end) *p++ = '[';
            temp[0] = t; temp[1] = t; temp[2] = t; temp[3] = t;
            p += tiles_to_string(temp, 4, p, end - p);
            if (!is_pack_melded(pack) && p < end) *p++ = ']';
            if (p < end) *p++ = ' ';
            break;
        case PACK_TYPE_PAIR:
            temp[0] = t; temp[1] = t;
            p += tiles_to_string(temp, 2, p, end - p);
            if (p < end) *p++ = ' ';
            break;
        default: break;
        }
    }

    if (p < end) {
        *p = '\0';
    }
    return p - str;
}

long hand_tiles_to_string(const hand_tiles_t *hand_tiles, char *str, long max_size) {
    char *p = str, *end = str + max_size;
    p += packs_to_string(hand_tiles->fixed_packs, hand_tiles->pack_count, str, max_size);
    if (p < end) p += tiles_to_string(hand_tiles->standing_tiles, hand_tiles->tile_count, p, end - p);
    return p - str;
}

}

