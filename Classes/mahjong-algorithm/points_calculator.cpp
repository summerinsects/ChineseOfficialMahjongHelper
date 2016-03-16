#include "points_calculator.h"
#include "wait_and_win_test.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

static bool seperate_2(const TILE *tiles, long tile_cnt, long fixed_set_cnt, SET (*output_sets)[5], long *separation_cnt) {
    if (tile_cnt == 2 && tiles[0] == tiles[1]) {  // if the branch reaches here, seperation is success
        // this 2 tiles is pair
        output_sets[*separation_cnt][4].is_melded = false;
        output_sets[*separation_cnt][4].mid_tile = tiles[0];
        output_sets[*separation_cnt][4].set_type = SET_TYPE::PAIR;

        // copy the sets of current seperation then sort them
        SET temp[5];
        memcpy(temp, output_sets[*separation_cnt], 5 * sizeof(SET));
        std::sort(temp + fixed_set_cnt, temp + 4, &set_cmp);

        // check if a same seperation is exists
        bool has_found = false;
        long i = *separation_cnt;
        while (i--) {
            if (std::equal(std::begin(temp), std::end(temp), output_sets[i], [](const SET &set1, const SET &set2) {
                return set1.mid_tile == set2.mid_tile && set1.set_type == set2.set_type;
            })) {
                has_found = true;
                break;
            }
        }

        if (!has_found) {
            // if not, we get a new seperation
            memcpy(output_sets[*separation_cnt + 1], output_sets[*separation_cnt], 5 * sizeof(SET));
            ++(*separation_cnt);
        }
        return true;
    }
    return false;
}

static bool seperate_N(const TILE *tiles, long tile_cnt, long fixed_set_cnt, SET (*output_sets)[5], long *separation_cnt) {
    if (tile_cnt < 3) {
        return seperate_2(tiles, tile_cnt, fixed_set_cnt, output_sets, separation_cnt);
    }

    bool ret = false;
    long i = 0;
    while (i < tile_cnt) {
        TILE tile_i = tiles[i];

        long j = i + 1;
        while (j < tile_cnt) {
            TILE tile_j = tiles[j];
            if (tile_j - tile_i >= 3) break;  // It is impossible to make a chow or pung

            long k = j + 1;
            while (k < tile_cnt) {
                TILE tile_k = tiles[k];
                if (tile_k - tile_i >= 3) break;  // It is impossible to make a chow or pung

                if (is_concealed_set_completed(tile_i, tile_j, tile_k)) {
                    long current = (14 - tile_cnt) / 3;
                    output_sets[*separation_cnt][current].is_melded = false;
                    output_sets[*separation_cnt][current].mid_tile = tile_j;
                    output_sets[*separation_cnt][current].set_type = (tile_i == tile_j) ? SET_TYPE::PUNG : SET_TYPE::CHOW;

                    // Reduced the completed pung or chow
                    TILE remains[14];
                    for (int n = 0, c = 0; n < tile_cnt; ++n) {
                        if (n == i || n == j || n == k) {
                            continue;
                        }
                        remains[c++] = tiles[n];
                    }
                    // recursive call
                    if (seperate_N(remains, tile_cnt - 3, fixed_set_cnt, output_sets, separation_cnt)) {
                        ret = true;
                    }
                }

                do ++k; while (k < tile_cnt && tiles[k] == tile_k);  // quick skip the same case
            }

            do ++j; while (j < tile_cnt && tiles[j] == tile_j);  // quick skip the same case
        }

        do ++i; while (i < tile_cnt && tiles[i] == tile_i);  // quick skip the same case
    }

    return ret;
}

static void recovery_tiles_from_sets(const SET *sets, long set_cnt, TILE *tiles, long *tile_cnt) {
    assert(tiles != nullptr && tile_cnt != nullptr);
    *tile_cnt = 0;
    for (int i = 0; i < set_cnt; ++i) {
        switch (sets[i].set_type) {
        case SET_TYPE::CHOW:
            tiles[(*tile_cnt)++] = sets[i].mid_tile - 1;
            tiles[(*tile_cnt)++] = sets[i].mid_tile;
            tiles[(*tile_cnt)++] = sets[i].mid_tile + 1;
            break;
        case SET_TYPE::PUNG:
            tiles[(*tile_cnt)++] = sets[i].mid_tile;
            tiles[(*tile_cnt)++] = sets[i].mid_tile;
            tiles[(*tile_cnt)++] = sets[i].mid_tile;
            break;
        case SET_TYPE::KONG:
            tiles[(*tile_cnt)++] = sets[i].mid_tile;
            tiles[(*tile_cnt)++] = sets[i].mid_tile;
            tiles[(*tile_cnt)++] = sets[i].mid_tile;
            tiles[(*tile_cnt)++] = sets[i].mid_tile;
            break;
        case SET_TYPE::PAIR:
            tiles[(*tile_cnt)++] = sets[i].mid_tile;
            tiles[(*tile_cnt)++] = sets[i].mid_tile;
            break;
        default:
            assert(0);
            break;
        }
    }
}

static bool is_quadruple_chow(TILE mid_tile0, TILE mid_tile1, TILE mid_tile2, TILE mid_tile3) {
    return (mid_tile0 == mid_tile1 && mid_tile0 == mid_tile2 && mid_tile0 == mid_tile3);
}

static bool is_four_pure_shifted_pungs(TILE mid_tile0, TILE mid_tile1, TILE mid_tile2, TILE mid_tile3) {
    if (is_numbered_suit_quick(mid_tile0)) {
        return (mid_tile0 + 1 == mid_tile1 && mid_tile1 + 1 == mid_tile2 && mid_tile2 + 1 == mid_tile3);
    }
    return false;
}

static bool is_four_pure_shifted_chows(TILE mid_tile0, TILE mid_tile1, TILE mid_tile2, TILE mid_tile3) {
    // shift up 2 must be: 123 345 567 789
    // shift up 1 can be min: 123 234 345 456 max: 456 567 678 789
    return ((mid_tile0 + 2 == mid_tile1 && mid_tile1 + 2 == mid_tile2 && mid_tile2 + 2 == mid_tile3)
        || (mid_tile0 + 1 == mid_tile1 && mid_tile1 + 1 == mid_tile2 && mid_tile2 + 1 == mid_tile3));
}

static bool is_pure_triple_chow(TILE mid_tile0, TILE mid_tile1, TILE mid_tile2) {
    return (mid_tile0 == mid_tile1 && mid_tile0 == mid_tile2);
}

static bool is_pure_shifted_pungs(TILE mid_tile0, TILE mid_tile1, TILE mid_tile2) {
    if (is_numbered_suit_quick(mid_tile0)) {
        return (mid_tile0 + 1 == mid_tile1 && mid_tile1 + 1 == mid_tile2);
    }
    return false;
}

static bool is_pure_straight(TILE mid_tile0, TILE mid_tile1, TILE mid_tile2) {
    if (tile_rank(mid_tile0) == 2) {
        return (mid_tile0 + 3 == mid_tile1 && mid_tile1 + 3 == mid_tile2);
    }
    return false;
}

static bool is_pure_shifted_chows(TILE mid_tile0, TILE mid_tile1, TILE mid_tile2) {
    return ((mid_tile0 + 2 == mid_tile1 && mid_tile1 + 2 == mid_tile2)
        || (mid_tile0 + 1 == mid_tile1 && mid_tile1 + 1 == mid_tile2));
}

static bool is_mixed_straight(TILE mid_tile0, TILE mid_tile1, TILE mid_tile2) {
    SUIT_TYPE suit0 = tile_suit(mid_tile0);
    SUIT_TYPE suit1 = tile_suit(mid_tile1);
    SUIT_TYPE suit2 = tile_suit(mid_tile2);
    if (suit0 != suit1 && suit0 != suit2 && suit1 != suit2) {
        RANK_TYPE rank0 = tile_rank(mid_tile0);
        RANK_TYPE rank1 = tile_rank(mid_tile1);
        RANK_TYPE rank2 = tile_rank(mid_tile2);
        return ((rank0 == 2 && rank1 == 5 && rank2 == 8)
            || (rank0 == 2 && rank1 == 8 && rank2 == 5)
            || (rank0 == 5 && rank1 == 2 && rank2 == 8)
            || (rank0 == 5 && rank1 == 8 && rank2 == 2)
            || (rank0 == 8 && rank1 == 2 && rank2 == 5)
            || (rank0 == 8 && rank1 == 5 && rank2 == 2));
    }
    return false;
}

static bool is_mixed_triple_chow_or_pung(TILE mid_tile0, TILE mid_tile1, TILE mid_tile2) {
    if (!is_numbered_suit_quick(mid_tile0) || !is_numbered_suit_quick(mid_tile1) || !is_numbered_suit_quick(mid_tile2)) {
        return false;
    }
    SUIT_TYPE suit0 = tile_suit(mid_tile0);
    SUIT_TYPE suit1 = tile_suit(mid_tile1);
    SUIT_TYPE suit2 = tile_suit(mid_tile2);
    if (suit0 != suit1 && suit0 != suit2 && suit1 != suit2) {
        RANK_TYPE rank0 = tile_rank(mid_tile0);
        RANK_TYPE rank1 = tile_rank(mid_tile1);
        RANK_TYPE rank2 = tile_rank(mid_tile2);
        return (rank0 == rank1 && rank0 == rank2);
    }
    return false;
}

static bool is_mixed_shifted_chow_or_pung(TILE mid_tile0, TILE mid_tile1, TILE mid_tile2) {
    if (!is_numbered_suit_quick(mid_tile0) || !is_numbered_suit_quick(mid_tile1) || !is_numbered_suit_quick(mid_tile2)) {
        return false;
    }
    SUIT_TYPE suit0 = tile_suit(mid_tile0);
    SUIT_TYPE suit1 = tile_suit(mid_tile1);
    SUIT_TYPE suit2 = tile_suit(mid_tile2);
    if (suit0 != suit1 && suit0 != suit2 && suit1 != suit2) {
        RANK_TYPE rank0 = tile_rank(mid_tile0);
        RANK_TYPE rank1 = tile_rank(mid_tile1);
        RANK_TYPE rank2 = tile_rank(mid_tile2);
        return ((rank1 + 1 == rank0 && rank0 + 1 == rank2)
            || (rank2 + 1 == rank0 && rank0 + 1 == rank1)
            || (rank0 + 1 == rank1 && rank1 + 1 == rank2)
            || (rank2 + 1 == rank1 && rank1 + 1 == rank0)
            || (rank0 + 1 == rank2 && rank2 + 1 == rank1)
            || (rank1 + 1 == rank2 && rank2 + 1 == rank0));
    }
    return false;
}

static bool is_pure_double_chow(TILE mid_tile0, TILE mid_tile1) {
    return mid_tile0 == mid_tile1;
}

static bool is_mixed_double_chow(TILE mid_tile0, TILE mid_tile1) {
    return (is_rank_equal_quick(mid_tile0, mid_tile1) && !is_suit_equal_quick(mid_tile0, mid_tile1));
}

static bool is_short_straight(TILE mid_tile0, TILE mid_tile1) {
    return (mid_tile0 + 3 == mid_tile1 || mid_tile1 + 3 == mid_tile0);
}

static bool is_two_terminal_chows(TILE mid_tile0, TILE mid_tile1) {
    if (is_suit_equal_quick(mid_tile0, mid_tile1)) {
        RANK_TYPE rank0 = tile_rank(mid_tile0);
        RANK_TYPE rank1 = tile_rank(mid_tile1);
        return ((rank0 == 2 && rank1 == 8) || (rank0 == 8 && rank1 == 2));
    }
    return false;
}

static POINT_TYPE get_4_chows_points(TILE mid_tile0, TILE mid_tile1, TILE mid_tile2, TILE mid_tile3) {
    if (is_quadruple_chow(mid_tile0, mid_tile1, mid_tile2, mid_tile3)) {
        return QUADRUPLE_CHOW;
    }

    if (is_four_pure_shifted_chows(mid_tile0, mid_tile1, mid_tile2, mid_tile3)) {
        return FOUR_PURE_SHIFTED_CHOWS;
    }
    return NONE;
}

static POINT_TYPE get_3_chows_points(TILE mid_tile0, TILE mid_tile1, TILE mid_tile2) {
    if (is_mixed_shifted_chow_or_pung(mid_tile0, mid_tile1, mid_tile2)) {
        return MIXED_SHIFTED_CHOWS;
    }
    if (is_mixed_straight(mid_tile0, mid_tile1, mid_tile2)) {
        return MIXED_STRAIGHT;
    }
    if (is_mixed_triple_chow_or_pung(mid_tile0, mid_tile1, mid_tile2)) {
        return MIXED_TRIPLE_CHOW;
    }
    if (is_pure_straight(mid_tile0, mid_tile1, mid_tile2)) {
        return PURE_STRAIGHT;
    }
    if (is_pure_shifted_chows(mid_tile0, mid_tile1, mid_tile2)) {
        return PURE_SHIFTED_CHOWS;
    }
    if (is_pure_triple_chow(mid_tile0, mid_tile1, mid_tile2)) {
        return PURE_TRIPLE_CHOW;
    }
    return NONE;
}

static POINT_TYPE get_2_chows_points(TILE mid_tile0, TILE mid_tile1) {
    if (is_pure_double_chow(mid_tile0, mid_tile1)) {
        return PURE_DOUBLE_CHOW;
    }
    if (is_mixed_double_chow(mid_tile0, mid_tile1)) {
        return MIXED_DOUBLE_CHOW;
    }
    if (is_short_straight(mid_tile0, mid_tile1)) {
        return SHORT_STRAIGHT;
    }
    if (is_two_terminal_chows(mid_tile0, mid_tile1)) {
        return TWO_TERMINAL_CHOWS;
    }
    return NONE;
}

static POINT_TYPE get_4_pungs_points(TILE mid_tile0, TILE mid_tile1, TILE mid_tile2, TILE mid_tile3) {
    if (is_four_pure_shifted_pungs(mid_tile0, mid_tile1, mid_tile2, mid_tile3)) {
        return FOUR_PURE_SHIFTED_PUNGS;
    }
    if (is_winds(mid_tile0) && is_winds(mid_tile1) && is_winds(mid_tile2) && is_winds(mid_tile3)) {
        return BIG_FOUR_WINDS;
    }
    return NONE;
}

static POINT_TYPE get_3_pungs_points(TILE mid_tile0, TILE mid_tile1, TILE mid_tile2) {
    if (is_mixed_shifted_chow_or_pung(mid_tile0, mid_tile1, mid_tile2)) {
        return MIXED_SHIFTED_PUNGS;
    }
    if (is_mixed_triple_chow_or_pung(mid_tile0, mid_tile1, mid_tile2)) {
        return TRIPLE_PUNG;
    }
    if (is_pure_shifted_pungs(mid_tile0, mid_tile1, mid_tile2)) {
        return PURE_SHIFTED_PUNGS;
    }
    if (is_dragons(mid_tile0) && is_dragons(mid_tile1) && is_dragons(mid_tile2)) {
        return BIG_THREE_DRAGONS;
    }
    if (is_winds(mid_tile0) && is_winds(mid_tile1) && is_winds(mid_tile2)) {
        return BIG_THREE_WINDS;
    }
    return NONE;
}

static POINT_TYPE get_2_pungs_points(TILE mid_tile0, TILE mid_tile1) {
    if (is_numbered_suit_quick(mid_tile0) && is_numbered_suit_quick(mid_tile1) && is_rank_equal_quick(mid_tile0, mid_tile1)) {
        return DOUBLE_PUNG;
    }
    if (is_dragons(mid_tile0) && is_dragons(mid_tile1)) {
        return TWO_DRAGONS_PUNGS;
    }
    return NONE;
}

static POINT_TYPE get_1_pung_points(TILE mid_tile) {
    if (is_terminal(mid_tile) || is_winds(mid_tile)) {
        return PUNG_OF_TERMINALS_OR_HONORS;
    }
    if (is_dragons(mid_tile)) {
        return DRAGON_PUNG;
    }
    return NONE;
}

template <long _Size>
static POINT_TYPE *pairwise_test_chows(const SET (&chows_sets)[_Size], POINT_TYPE *selected_points) {
    POINT_TYPE all_points[_Size][_Size] = { { NONE } };

    // init matrix
    for (int i = 0; i < _Size; ++i) {
        for (int j = 0; j < i - 1; ++j) {  // this is a symmetric matrix
            all_points[i][j] = all_points[j][i];
        }
        for (int j = i + 1; j < _Size; ++j) {
            all_points[i][j] = get_2_chows_points(chows_sets[i].mid_tile, chows_sets[j].mid_tile);
        }
    }

    bool used_flag[_Size] = { false };
    for (int i = 0; i < _Size; ++i) {
        for (int j = 0; j < _Size; ++j) {
            if (i == j) {
                continue;
            }

            // The account-once principle (Exclusionary rule)
            // When you have combined some sets to create a fan,
            // you can only combine any remaining sets once with a set that has already been used
            if (used_flag[i] && used_flag[j]) {
                continue;
            }
            if (all_points[i][j] != NONE) {
                used_flag[i] = true;
                used_flag[j] = true;
                *selected_points = all_points[i][j];
                ++selected_points;
            }
        }
    }
    return selected_points;
}

static void calculate_4_chows(const SET chow_sets[4], long (&points_table)[FLOWER_TILES]) {
    SET sets[4];
    memcpy(sets, chow_sets, sizeof(sets));
    std::sort(std::begin(sets), std::end(sets), [](const SET &set1, const SET &set2) {
        return set1.mid_tile < set2.mid_tile;
    });

    POINT_TYPE points;
    if ((points = get_4_chows_points(sets[0].mid_tile, sets[1].mid_tile, sets[2].mid_tile, sets[3].mid_tile))) {
        points_table[points] = 1;
        return;
    }

    bool _3_chows_has_points = false;
    long free_set_idx = -1;
    if ((points = get_3_chows_points(sets[0].mid_tile, sets[1].mid_tile, sets[2].mid_tile)) != NONE) {
        points_table[points] = 1;
        free_set_idx = 3;
        _3_chows_has_points = true;
    }
    else if ((points = get_3_chows_points(sets[0].mid_tile, sets[1].mid_tile, sets[3].mid_tile)) != NONE) {
        points_table[points] = 1;
        free_set_idx = 2;
        _3_chows_has_points = true;
    }
    else if ((points = get_3_chows_points(sets[0].mid_tile, sets[2].mid_tile, sets[3].mid_tile)) != NONE) {
        points_table[points] = 1;
        free_set_idx = 1;
        _3_chows_has_points = true;
    }
    else if ((points = get_3_chows_points(sets[1].mid_tile, sets[2].mid_tile, sets[3].mid_tile)) != NONE) {
        points_table[points] = 1;
        free_set_idx = 0;
        _3_chows_has_points = true;
    }

    if (_3_chows_has_points) {
        for (long i = 0; i < 4; ++i) {
            if (i == free_set_idx) {
                continue;
            }
            if ((points = get_2_chows_points(sets[i].mid_tile, sets[free_set_idx].mid_tile)) != NONE) {
                ++points_table[points];
                break;
            }
        }
        return;
    }

    POINT_TYPE selected_points[3] = { NONE };
    pairwise_test_chows(sets, selected_points);
    for (long i = 0; i < 3; ++i) {
        if (selected_points[i] != NONE) {
            ++points_table[selected_points[i]];
        }
    }
}

static void calculate_3_chows(const SET chow_sets[3], long (&points_table)[FLOWER_TILES]) {
    SET sets[3];
    memcpy(sets, chow_sets, sizeof(sets));
    std::sort(std::begin(sets), std::end(sets), [](const SET &set1, const SET &set2) {
        return set1.mid_tile < set2.mid_tile;
    });

    POINT_TYPE points;
    if ((points = get_3_chows_points(sets[0].mid_tile, sets[1].mid_tile, sets[2].mid_tile)) != NONE) {
        points_table[points] = 1;
        return;
    }

    POINT_TYPE selected_points[2] = { NONE };
    pairwise_test_chows(sets, selected_points);
    for (long i = 0; i < 2; ++i) {
        if (selected_points[i] != NONE) {
            ++points_table[selected_points[i]];
        }
    }
}

static void calculate_2_chows(const SET chow_sets[2], long (&points_table)[FLOWER_TILES]) {
    const SET *sets = chow_sets;
    POINT_TYPE points;
    if ((points = get_2_chows_points(sets[0].mid_tile, sets[1].mid_tile)) != NONE) {
        ++points_table[points];
    }
}

static void calculate_kongs(const SET *pung_sets, long pung_cnt, long (&points_table)[FLOWER_TILES]) {
    int melded_kong_cnt = 0;
    int concealed_kong_cnt = 0;
    int concealed_pung_cnt = 0;
    for (long i = 0; i < pung_cnt; ++i) {
        if (pung_sets[i].is_melded) {
            if (pung_sets[i].set_type == SET_TYPE::KONG) {
                ++melded_kong_cnt;
            }
        }
        else {
            if (pung_sets[i].set_type == SET_TYPE::KONG) {
                ++concealed_kong_cnt;
            }
            else {
                ++concealed_pung_cnt;
            }
        }
    }

    // RULE:
    // Three kongs
    // melded kong * 2 + concealed kong + concealed pung -> three kongs + concealed kong + two concealed pungs + all pungs
    // melded kong + concealed kong * 2 + melded pung    -> three kongs + two concealed kongs + all pungs
    // melded kong + concealed kong * 2 + concealed pung -> three kongs + three concealed pungs + two concealed kongs + all pungs
    // concealed kong * 3 + melded pung    -> three kongs + three concealed pungs + all pungs
    // concealed kong * 3 + concealed pung -> three kongs + four concealed pungs
    //
    // Four kongs
    // concealed kong * 2 + melded kong * 2 -> four kongs + two concealed kong
    // concealed kong * 3 + melded kong     -> four kongs + three concealed pung
    // concealed kong * 4                   -> four kongs + four concealed pung
    //

    int kong_cnt = melded_kong_cnt + concealed_kong_cnt;
    switch (kong_cnt) {
    case 0:
        switch (concealed_pung_cnt) {
        case 0: break;
        case 1: break;
        case 2: points_table[TWO_CONCEALED_PUNGS] = 1; break;
        case 3: points_table[THREE_CONCEALED_PUNGS] = 1; break;
        case 4: points_table[FOUR_CONCEALED_PUNGS] = 1; break;
        default: assert(0); break;
        }
        break;
    case 1:
        if (melded_kong_cnt == 1) {
            points_table[MELDED_KONG] = 1;
            switch (concealed_pung_cnt) {
            case 0: break;
            case 1: break;
            case 2: points_table[TWO_CONCEALED_PUNGS] = 1; break;
            case 3: points_table[THREE_CONCEALED_PUNGS] = 1; break;
            default: assert(0); break;
            }
        }
        else {
            points_table[CONCEALED_KONG] = 1;
            switch (concealed_pung_cnt) {
            case 0: break;
            case 1: points_table[TWO_CONCEALED_PUNGS] = 1; break;
            case 2: points_table[THREE_CONCEALED_PUNGS] = 1; break;
            case 3: points_table[FOUR_CONCEALED_PUNGS] = 1; break;
            default: assert(0); break;
            }
        }
        break;
    case 2:
        switch (concealed_kong_cnt) {
        case 0:
            points_table[TWO_MELDED_KONGS] = 1;
            switch (concealed_pung_cnt) {
            case 0: break;
            case 1: break;
            case 2: points_table[TWO_CONCEALED_PUNGS] = 1; break;
            default: assert(0); break;
            }
            break;
        case 1:
            points_table[MELDED_KONG] = 1;
            points_table[CONCEALED_KONG] = 1;
            switch (concealed_pung_cnt) {
            case 0: break;
            case 1: points_table[TWO_CONCEALED_PUNGS] = 1; break;
            case 2: points_table[THREE_CONCEALED_PUNGS] = 1; break;
            default: assert(0); break;
            }
            break;
        case 2:
            points_table[TWO_CONCEALED_KONGS] = 1;
            switch (concealed_pung_cnt) {
            case 0: break;
            case 1: points_table[THREE_CONCEALED_PUNGS] = 1; break;
            case 2: points_table[FOUR_CONCEALED_PUNGS] = 1; break;
            default: assert(0); break;
            }
            break;
        default:
            assert(0);
            break;
        }
        break;
    case 3:
        points_table[THREE_KONGS] = 1;
        switch (concealed_kong_cnt) {
        case 0: break;
        case 1:
            if (concealed_pung_cnt == 0) {
                points_table[CONCEALED_KONG] = 1;
            }
            else {
                points_table[CONCEALED_KONG] = 1;
                points_table[TWO_CONCEALED_PUNGS] = 1;
            }
            break;
        case 2:
            if (concealed_pung_cnt == 0) {
                points_table[TWO_CONCEALED_KONGS] = 1;
            }
            else {
                points_table[THREE_CONCEALED_PUNGS] = 1;
                points_table[TWO_CONCEALED_KONGS] = 1;
            }
            break;
        case 3:
            if (concealed_pung_cnt == 0) {
                points_table[THREE_CONCEALED_PUNGS] = 1;
            }
            else {
                points_table[FOUR_CONCEALED_PUNGS] = 1;
            }
            break;
        default:
            assert(0);
            break;
        }
        break;
    case 4:
        points_table[FOUR_KONGS] = 1;
        switch (concealed_kong_cnt) {
        case 0: break;
        case 1: points_table[CONCEALED_KONG] = 1; break;
        case 2: points_table[TWO_CONCEALED_KONGS] = 1; break;
        case 3: points_table[THREE_CONCEALED_PUNGS] = 1; break;
        case 4: points_table[FOUR_CONCEALED_PUNGS] = 1; break;
        default: assert(0); break;
        }
        break;
    default:
        assert(0);
        break;
    }

    if (pung_cnt == 4) {
        if (points_table[FOUR_KONGS] == 0 && points_table[FOUR_CONCEALED_PUNGS] == 0) {
            points_table[ALL_PUNGS] = 1;
        }
    }
}

static void calculate_4_pungs(const SET pung_sets[4], long (&points_table)[FLOWER_TILES]) {
    SET sets[4];
    memcpy(sets, pung_sets, sizeof(sets));
    std::sort(std::begin(sets), std::end(sets), [](const SET &set1, const SET &set2) {
        return set1.mid_tile < set2.mid_tile;
    });

    calculate_kongs(sets, 4, points_table);

    POINT_TYPE points;
    if ((points = get_4_pungs_points(sets[0].mid_tile, sets[1].mid_tile, sets[2].mid_tile, sets[3].mid_tile)) != NONE) {
        points_table[points] = 1;
        return;
    }

    bool _3_pungs_has_points = false;
    long free_set_idx = -1;
    if ((points = get_3_pungs_points(sets[0].mid_tile, sets[1].mid_tile, sets[2].mid_tile)) != NONE) {
        points_table[points] = 1;
        free_set_idx = 3;
        _3_pungs_has_points = true;
    }
    else if ((points = get_3_pungs_points(sets[0].mid_tile, sets[1].mid_tile, sets[3].mid_tile)) != NONE) {
        points_table[points] = 1;
        free_set_idx = 2;
        _3_pungs_has_points = true;
    }
    else if ((points = get_3_pungs_points(sets[0].mid_tile, sets[2].mid_tile, sets[3].mid_tile)) != NONE) {
        points_table[points] = 1;
        free_set_idx = 1;
        _3_pungs_has_points = true;
    }
    else if ((points = get_3_pungs_points(sets[1].mid_tile, sets[2].mid_tile, sets[3].mid_tile)) != NONE) {
        points_table[points] = 1;
        free_set_idx = 0;
        _3_pungs_has_points = true;
    }

    if (_3_pungs_has_points) {
        for (long i = 0; i < 4; ++i) {
            if (i == free_set_idx) {
                continue;
            }
            if ((points = get_2_pungs_points(sets[i].mid_tile, sets[free_set_idx].mid_tile)) != NONE) {
                ++points_table[points];
                break;
            }
        }
    }
    else {
        if ((points = get_2_pungs_points(sets[0].mid_tile, sets[1].mid_tile)) != NONE) {
            ++points_table[points];
        }
        if ((points = get_2_pungs_points(sets[0].mid_tile, sets[2].mid_tile)) != NONE) {
            ++points_table[points];
        }
        if ((points = get_2_pungs_points(sets[0].mid_tile, sets[3].mid_tile)) != NONE) {
            ++points_table[points];
        }
        if ((points = get_2_pungs_points(sets[1].mid_tile, sets[2].mid_tile)) != NONE) {
            ++points_table[points];
        }
        if ((points = get_2_pungs_points(sets[1].mid_tile, sets[3].mid_tile)) != NONE) {
            ++points_table[points];
        }
        if ((points = get_2_pungs_points(sets[2].mid_tile, sets[3].mid_tile)) != NONE) {
            ++points_table[points];
        }
    }

    if ((points = get_1_pung_points(sets[0].mid_tile)) != NONE) {
        ++points_table[points];
    }
    if ((points = get_1_pung_points(sets[1].mid_tile)) != NONE) {
        ++points_table[points];
    }
    if ((points = get_1_pung_points(sets[2].mid_tile)) != NONE) {
        ++points_table[points];
    }
    if ((points = get_1_pung_points(sets[3].mid_tile)) != NONE) {
        ++points_table[points];
    }
}

static void calculate_3_pungs(const SET pung_sets[3], long (&points_table)[FLOWER_TILES]) {
    SET sets[3];
    memcpy(sets, pung_sets, sizeof(sets));
    std::sort(std::begin(sets), std::end(sets), [](const SET &set1, const SET &set2) {
        return set1.mid_tile < set2.mid_tile;
    });

    calculate_kongs(sets, 3, points_table);

    POINT_TYPE points;
    if ((points = get_3_pungs_points(sets[0].mid_tile, sets[1].mid_tile, sets[2].mid_tile)) != NONE) {
        points_table[points] = 1;
        return;
    }

    if ((points = get_2_pungs_points(sets[0].mid_tile, sets[1].mid_tile)) != NONE) {
        ++points_table[points];
    }
    if ((points = get_2_pungs_points(sets[0].mid_tile, sets[2].mid_tile)) != NONE) {
        ++points_table[points];
    }
    if ((points = get_2_pungs_points(sets[1].mid_tile, sets[2].mid_tile)) != NONE) {
        ++points_table[points];
    }

    if ((points = get_1_pung_points(sets[0].mid_tile)) != NONE) {
        ++points_table[points];
    }
    if ((points = get_1_pung_points(sets[1].mid_tile)) != NONE) {
        ++points_table[points];
    }
    if ((points = get_1_pung_points(sets[2].mid_tile)) != NONE) {
        ++points_table[points];
    }
}

static void calculate_2_pungs(const SET pung_sets[2], long (&points_table)[FLOWER_TILES]) {
    const SET *sets = pung_sets;

    calculate_kongs(sets, 2, points_table);

    POINT_TYPE points;
    if ((points = get_2_pungs_points(sets[0].mid_tile, sets[1].mid_tile)) != NONE) {
        ++points_table[points];
    }

    if ((points = get_1_pung_points(sets[0].mid_tile)) != NONE) {
        ++points_table[points];
    }
    if ((points = get_1_pung_points(sets[1].mid_tile)) != NONE) {
        ++points_table[points];
    }
}

static void calculate_1_pung(const SET &pung_set, long (&points_table)[FLOWER_TILES]) {
    calculate_kongs(&pung_set, 1, points_table);
    POINT_TYPE points = get_1_pung_points(pung_set.mid_tile);
    if (points != NONE) {
        ++points_table[points];
    }
}

static bool is_nine_gates(const TILE tiles[14], TILE win_tile) {
    TILE hand_tiles[13];
    copy_exclude(tiles, tiles + 14, &win_tile, (&win_tile) + 1, hand_tiles);
    sort_tiles(hand_tiles, 13);

    // nine gates is fixed format of tiles in hand
    switch (hand_tiles[0]) {
    case 0x11: return (memcmp(hand_tiles, standard_nine_gates[0], sizeof(hand_tiles)) == 0);
    case 0x21: return (memcmp(hand_tiles, standard_nine_gates[1], sizeof(hand_tiles)) == 0);
    case 0x31: return (memcmp(hand_tiles, standard_nine_gates[2], sizeof(hand_tiles)) == 0);
    default: return false;
    }
}

static bool is_pure_terminal_chows(const SET (&chow_sets)[4], const SET &pair_set) {
    if (tile_rank(pair_set.mid_tile) != 5) {  // 5 as pair
        return false;
    }

    // two 123 and two 789 in same suit
    // eg: 123s 123s 789s 789s 55s
    int _123_cnt = 0, _789_cnt = 0;
    SUIT_TYPE pair_suit = tile_suit(pair_set.mid_tile);
    for (long i = 0; i < 4; ++i) {
        SUIT_TYPE suit = tile_suit(chow_sets[i].mid_tile);
        if (suit != pair_suit) {
            return false;
        }
        RANK_TYPE rank = tile_rank(chow_sets[i].mid_tile);
        switch (rank) {
        case 2: ++_123_cnt; break;
        case 8: ++_789_cnt; break;
        default: return false;
        }
    }
    return (_123_cnt == 2 && _789_cnt == 2);
}

static bool is_three_suited_terminal_chows(const SET (&chow_sets)[4], const SET &pair_set) {
    if (tile_rank(pair_set.mid_tile) != 5) {  // 5 as pair in one suit
        return false;
    }

    // two 123 in remaining suits and 789 in remaining suits
    // eg: 123s 789s 123p 789p 55m
    int _123_suit_table[4] = { 0 };
    int _789_suit_table[4] = { 0 };
    SUIT_TYPE pair_suit = tile_suit(pair_set.mid_tile);
    for (long i = 0; i < 4; ++i) {
        SUIT_TYPE suit = tile_suit(chow_sets[i].mid_tile);
        if (suit == pair_suit) {
            return false;
        }
        RANK_TYPE rank = tile_rank(chow_sets[i].mid_tile);
        switch (rank) {
        case 2: ++_123_suit_table[suit]; break;
        case 8: ++_789_suit_table[suit]; break;
        default: return false;
        }
    }
    switch (pair_suit) {
    case 1: return (_123_suit_table[2] && _123_suit_table[3] && _789_suit_table[2] && _789_suit_table[3]);
    case 2: return (_123_suit_table[1] && _123_suit_table[3] && _789_suit_table[1] && _789_suit_table[3]);
    case 3: return (_123_suit_table[1] && _123_suit_table[2] && _789_suit_table[1] && _789_suit_table[2]);
    default: assert(0); return false;
    }
}

static void check_melded_or_concealed_hand(const SET (&sets)[5], long fixed_cnt, bool self_drawn, long (&points_table)[FLOWER_TILES]) {
    long melded_cnt = 0;
    for (long i = 0; i < fixed_cnt; ++i) {
        if (sets[i].is_melded) {
            ++melded_cnt;
        }
    }

    switch (melded_cnt) {
    case 0: points_table[self_drawn ? FULLY_CONCEALED_HAND : CONCEALED_HAND] = 1; break;
    case 4: points_table[self_drawn ? SELF_DRAWN : MELDED_HAND] = 1; break;
    default:
        if (self_drawn) {
            points_table[SELF_DRAWN] = 1;
        }
        break;
    }
}

static void check_pair_tile(TILE pair_tile, long chow_cnt, long (&points_table)[FLOWER_TILES]) {
    if (chow_cnt == 4) {
        if (is_numbered_suit_quick(pair_tile)) {
            points_table[ALL_CHOWS] = 1;
        }
    }
    else {
        if (points_table[TWO_DRAGONS_PUNGS]) {
            if (is_dragons(pair_tile)) {
                points_table[LITTLE_THREE_DRAGONS] = 1;
                points_table[TWO_DRAGONS_PUNGS] = 0;
            }
        }
        else if (points_table[BIG_THREE_WINDS]) {
            if (is_winds(pair_tile)) {
                points_table[LITTLE_FOUR_WINDS] = 1;
                points_table[BIG_THREE_WINDS] = 0;
            }
        }
    }
}

static void check_tiles_suits(const TILE *tiles, long tile_cnt, long (&points_table)[FLOWER_TILES]) {
    int has_characters = 0;
    int has_bamboo = 0;
    int has_dots = 0;
    int has_winds = 0;
    int has_dragons = 0;
    for (long i = 0; i < tile_cnt; ++i) {
        SUIT_TYPE suit = tile_suit(tiles[i]);
        switch (suit) {
        case TILE_SUIT_CHARACTERS: has_characters = 1; break;
        case TILE_SUIT_BAMBOO: has_bamboo = 1; break;
        case TILE_SUIT_DOTS: has_dots = 1; break;
        case TILE_SUIT_WINDS: has_winds = 1; break;
        case TILE_SUIT_DRAGONS: has_dragons = 1; break;
        }
    }

    int total_suit = has_characters + has_bamboo + has_dots + has_winds + has_dragons;
    switch (total_suit) {
    case 5:
        points_table[ALL_TYPES] = 1;
        break;
    case 4:
        if (!has_characters || !has_bamboo || !has_dots) {
            if (!points_table[REVERSIBLE_TILES]) {
                points_table[ONE_VOIDED_SUIT] = 1;
            }
        }
        break;
    case 3:
        if (!has_winds && !has_dragons) {
            points_table[NO_HONORS] = 1;
        }
        else if (has_winds && has_dragons) {
            points_table[HALF_FLUSH] = 1;
        }
        else {
            points_table[ONE_VOIDED_SUIT] = 1;
        }
        break;
    case 2:
        if (!has_winds && !has_dragons) {
            points_table[ONE_VOIDED_SUIT] = 1;
            points_table[NO_HONORS] = 1;
        }
        else if (has_winds && has_dragons) {
            points_table[ALL_HONORS] = 1;
        }
        else {
            points_table[HALF_FLUSH] = 1;
        }
        break;
    case 1:
        if (has_characters || has_bamboo || has_dots) {
            points_table[FULL_FLUSH] = 1;
        }
        else {
            assert(0);
        }
        break;
    default:
        assert(0);
        break;
    }
}

static void check_tiles_rank_by_tiles(const TILE *tiles, long tile_cnt, long (&points_table)[FLOWER_TILES]) {
    // Mapped rank to a table
    uint16_t rank_flag = 0;
    for (long i = 0; i < tile_cnt; ++i) {
        SUIT_TYPE suit = tile_suit(tiles[i]);
        if (suit == TILE_SUIT_WINDS || suit == TILE_SUIT_DRAGONS) {
            return;  // honor tiles are not allowed
        }
        rank_flag |= (1 << tile_rank(tiles[i]));
    }

    // 1111 1111 1110 0001
    // no 56789 ?
    if (!(rank_flag & 0xFFE1)) {
        // no 4 ?
        points_table[rank_flag & 0x0010 ? LOWER_FOUR : LOWER_TILES] = 1;
    }
    // 1111 1100 0011 1111
    // no 12345 ?
    else if (!(rank_flag & 0xFC3F)) {
        // no 6 ?
        points_table[rank_flag & 0x0040 ? UPPER_FOUR : UPPER_TILES] = 1;
    }

    // 1111 1110 0000 0011
    // no 1 and no 9
    if (!(rank_flag & 0xFE03)) {
        // 1111 1111 1000 1111
        // no 2478 ?
        if (!(rank_flag & 0xFF8F)) {
            points_table[MIDDLE_TILES] = 1;
        }
        else {
            points_table[ALL_SIMPLES] = 1;
        }
    }
}

static void check_tiles_rank_by_set(const SET (&sets)[5], long (&points_table)[FLOWER_TILES]) {
    int terminal_cnt = 0;
    int honor_cnt = 0;
    int _5_cnt = 0;
    int even_pung_cnt = 0;
    for (long i = 0; i < 5; ++i) {
        if (is_set_contains_tile(sets[i], 0x11) ||
            is_set_contains_tile(sets[i], 0x19) ||
            is_set_contains_tile(sets[i], 0x21) ||
            is_set_contains_tile(sets[i], 0x29) ||
            is_set_contains_tile(sets[i], 0x31) ||
            is_set_contains_tile(sets[i], 0x39)) {
            ++terminal_cnt;
        }
        else if (is_set_contains_tile(sets[i], 0x41) ||
            is_set_contains_tile(sets[i], 0x42) ||
            is_set_contains_tile(sets[i], 0x43) ||
            is_set_contains_tile(sets[i], 0x44) ||
            is_set_contains_tile(sets[i], 0x51) ||
            is_set_contains_tile(sets[i], 0x52) ||
            is_set_contains_tile(sets[i], 0x53)) {
            ++honor_cnt;
        }
        else if (is_set_contains_tile(sets[i], 0x15) ||
            is_set_contains_tile(sets[i], 0x25) ||
            is_set_contains_tile(sets[i], 0x35)) {
            ++_5_cnt;
        }
        else if (sets[i].set_type == SET_TYPE::PUNG ||
            sets[i].set_type == SET_TYPE::KONG ||
            sets[i].set_type == SET_TYPE::PAIR) {
            if (is_numbered_suit_quick(sets[i].mid_tile) && sets[i].mid_tile % 2 == 0) {
                ++even_pung_cnt;
            }
        }
    }

    if (terminal_cnt + honor_cnt == 5) {
        points_table[OUTSIDE_HAND] = 1;
    }
    else if (_5_cnt == 5) {
        points_table[ALL_FIVE] = 1;
    }
    else if (even_pung_cnt == 5) {
        points_table[ALL_EVEN_PUNGS] = 1;
    }
}

static void check_tiles_traits(const TILE *tiles, long tile_cnt, long (&points_table)[FLOWER_TILES]) {
    if (std::all_of(tiles, tiles + tile_cnt, &is_reversible_tile)) {
        points_table[REVERSIBLE_TILES] = 1;
    }

    if (std::all_of(tiles, tiles + tile_cnt, &is_green)) {
        points_table[ALL_GREEN] = 1;
    }

    if (std::all_of(tiles, tiles + tile_cnt, &is_terminal)) {
        points_table[ALL_TERMINALS] = 1;
    }

    if (!points_table[ALL_TERMINALS] && std::all_of(tiles, tiles + tile_cnt, &is_terminal_or_honor)) {
        points_table[ALL_TERMINALS_AND_HONORS] = 1;
    }
}

static void check_tiles_hog(const TILE *tiles, long tile_cnt, long (&points_table)[FLOWER_TILES]) {
    long kong_cnt = tile_cnt - 14;
    int cnt_table[0x54] = { 0 };
    for (long i = 0; i < tile_cnt; ++i) {
        ++cnt_table[tiles[i]];
    }
    long _4_cnt = std::count(std::begin(cnt_table), std::end(cnt_table), 4);
    points_table[TILE_HOG] = _4_cnt - kong_cnt;
}

static void check_edge_closed_single_wait(const SET *concealed_sets, long set_cnt, TILE win_tile, long (&points_table)[FLOWER_TILES]) {
    TILE concealed_tiles[14];
    long concealed_tile_cnt;
    recovery_tiles_from_sets(concealed_sets, set_cnt, concealed_tiles, &concealed_tile_cnt);
    sort_tiles(concealed_tiles, concealed_tile_cnt);
    copy_exclude(concealed_tiles, concealed_tiles + concealed_tile_cnt, &win_tile, (&win_tile) + 1, concealed_tiles);

    bool waiting_table[6][10] = { { 0 } };
    long waiting_cnt = 0;

    switch (set_cnt) {
    case 5:
        is_basic_type_13_wait(concealed_tiles, waiting_table);
        break;
    case 4:
        is_basic_type_10_wait(concealed_tiles, waiting_table);
        break;
    case 3:
        is_basic_type_7_wait(concealed_tiles, waiting_table);
        break;
    case 2:
        is_basic_type_4_wait(concealed_tiles, waiting_table);
        break;
    case 1:
        waiting_table[tile_suit(win_tile)][tile_rank(win_tile)] = true;
        break;
    default:
        break;
    }
    for (int i = 1; i < 6; ++i) {
        for (int j = 1; j < 10; ++j) {
            if (waiting_table[i][j]) {
                ++waiting_cnt;
            }
        }
    }

    if (waiting_cnt != 1 || points_table[MELDED_HAND]) {
        return;
    }

    bool maybe_edge = false;
    bool maybe_closed = false;
    bool maybe_single = false;

    for (long i = 0; i < set_cnt; ++i) {
        switch (concealed_sets[i].set_type) {
        case SET_TYPE::CHOW:
            if (concealed_sets[i].mid_tile == win_tile) {
                maybe_closed = true;
            }
            else if (concealed_sets[i].mid_tile + 1 == win_tile
                || concealed_sets[i].mid_tile - 1 == win_tile) {
                maybe_edge = true;
            }
            break;
        case SET_TYPE::PAIR:
            if (concealed_sets[i].mid_tile == win_tile) {
                maybe_single = true;
            }
            break;
        default:
            break;
        }
    }

    if (maybe_edge) {
        points_table[EDGE_WAIT] = 1;
        return;
    }
    if (maybe_closed) {
        points_table[CLOSED_WAIT] = 1;
        return;
    }
    if (maybe_single) {
        points_table[SINGLE_WAIT] = 1;
        return;
    }
}

static void check_wind_pungs(const SET &sets, WIND_TYPE prevalent_wind, WIND_TYPE seat_wind, long (&points_table)[FLOWER_TILES]) {
    if (sets.set_type == SET_TYPE::PUNG || sets.set_type == SET_TYPE::KONG) {
        RANK_TYPE delta = sets.mid_tile - 0x41;
        if (delta == (int)prevalent_wind - (int)WIND_TYPE::EAST) {
            points_table[PREVALENT_WIND] = 1;
        }
        if (delta == (int)seat_wind - (int)WIND_TYPE::EAST) {
            points_table[SEAT_WIND] = 1;
        }
    }
}

static void correction_points_table(long (&points_table)[FLOWER_TILES]) {
    if (points_table[BIG_FOUR_WINDS]) {
        points_table[BIG_THREE_WINDS] = 0;
        points_table[ALL_PUNGS] = 0;
        points_table[PREVALENT_WIND] = 0;
        points_table[SEAT_WIND] = 0;
        points_table[PUNG_OF_TERMINALS_OR_HONORS] = 0;
    }
    if (points_table[BIG_THREE_DRAGONS]) {
        points_table[TWO_DRAGONS_PUNGS] = 0;
        points_table[DRAGON_PUNG] = 0;
        points_table[ONE_VOIDED_SUIT] = 0;
    }
    if (points_table[ALL_GREEN]) {
        points_table[HALF_FLUSH] = 0;
        points_table[ONE_VOIDED_SUIT] = 0;
    }
    if (points_table[NINE_GATES]) {
        points_table[FULL_FLUSH] = 0;
        points_table[CONCEALED_HAND] = 0;
        points_table[ONE_VOIDED_SUIT] = 0;
        points_table[NO_HONORS] = 0;
    }
    if (points_table[FOUR_KONGS]) {
        points_table[SINGLE_WAIT] = 0;
    }
    if (points_table[SEVEN_SHIFTED_PAIRS]) {
        points_table[SEVEN_PAIRS] = 0;
        points_table[FULL_FLUSH] = 0;
        points_table[CONCEALED_HAND] = 0;
        points_table[ONE_VOIDED_SUIT] = 0;
        points_table[NO_HONORS] = 0;
    }
    if (points_table[THIRTEEN_ORPHANS]) {
        points_table[ALL_TYPES] = 0;
        points_table[CONCEALED_HAND] = 0;
        points_table[SINGLE_WAIT] = 0;
    }

    if (points_table[ALL_TERMINALS]) {
        points_table[ALL_TERMINALS_AND_HONORS] = 0;
        points_table[ALL_PUNGS] = 0;
        points_table[OUTSIDE_HAND] = 0;
        points_table[PUNG_OF_TERMINALS_OR_HONORS] = 0;
        points_table[NO_HONORS] = 0;
        points_table[TRIPLE_PUNG] = 0;
        points_table[DOUBLE_PUNG] = 0;
    }
    if (points_table[LITTLE_FOUR_WINDS]) {
        points_table[BIG_THREE_WINDS] = 0;
    }
    if (points_table[LITTLE_THREE_DRAGONS]) {
        points_table[TWO_DRAGONS_PUNGS] = 0;
        points_table[DRAGON_PUNG] = 0;
        points_table[ONE_VOIDED_SUIT] = 0;
    }
    if (points_table[ALL_HONORS]) {
        points_table[ALL_TERMINALS_AND_HONORS] = 0;
        points_table[ALL_PUNGS] = 0;
        points_table[OUTSIDE_HAND] = 0;
        points_table[PUNG_OF_TERMINALS_OR_HONORS] = 0;
    }
    if (points_table[FOUR_CONCEALED_PUNGS]) {
        points_table[ALL_PUNGS] = 0;
        points_table[CONCEALED_HAND] = 0;
        if (points_table[FULLY_CONCEALED_HAND]) {
            points_table[FULLY_CONCEALED_HAND] = 0;
            points_table[SELF_DRAWN] = 1;
        }
    }
    if (points_table[PURE_TERMINAL_CHOWS]) {
        points_table[SEVEN_PAIRS] = 0;
        points_table[FULL_FLUSH] = 0;
        points_table[ALL_CHOWS] = 0;
        points_table[PURE_DOUBLE_CHOW] = 0;
        points_table[TWO_TERMINAL_CHOWS] = 0;
        points_table[ONE_VOIDED_SUIT] = 0;
        points_table[NO_HONORS] = 0;
    }

    if (points_table[QUADRUPLE_CHOW]) {
        points_table[PURE_SHIFTED_PUNGS] = 0;
        points_table[TILE_HOG] = 0;
        points_table[PURE_DOUBLE_CHOW] = 0;
        points_table[ONE_VOIDED_SUIT] = 0;
    }
    if (points_table[FOUR_PURE_SHIFTED_PUNGS]) {
        points_table[PURE_TRIPLE_CHOW] = 0;
        points_table[ALL_PUNGS] = 0;
        points_table[ONE_VOIDED_SUIT] = 0;
    }

    if (points_table[FOUR_PURE_SHIFTED_CHOWS]) {
        points_table[TWO_TERMINAL_CHOWS] = 0;
        points_table[SHORT_STRAIGHT] = 0;
        points_table[ONE_VOIDED_SUIT] = 0;
    }
    if (points_table[THREE_KONGS]) {

    }
    if (points_table[ALL_TERMINALS_AND_HONORS]) {
        points_table[ALL_PUNGS] = 0;
        points_table[OUTSIDE_HAND] = 0;
        points_table[PUNG_OF_TERMINALS_OR_HONORS] = 0;
    }

    if (points_table[SEVEN_PAIRS]) {
        points_table[CONCEALED_HAND] = 0;
        points_table[SINGLE_WAIT] = 0;
    }
    if (points_table[GREATER_HONORS_AND_KNITTED_TILES]) {
        points_table[CONCEALED_HAND] = 0;
        points_table[ALL_TYPES] = 0;
    }
    if (points_table[ALL_EVEN_PUNGS]) {
        points_table[ALL_PUNGS] = 0;
        points_table[ALL_SIMPLES] = 0;
        points_table[NO_HONORS] = 0;
    }
    if (points_table[FULL_FLUSH]) {
        points_table[ONE_VOIDED_SUIT] = 0;
        points_table[NO_HONORS] = 0;
    }
    if (points_table[PURE_TRIPLE_CHOW]) {
        points_table[PURE_SHIFTED_PUNGS] = 0;
        points_table[PURE_DOUBLE_CHOW] = 0;
    }
    if (points_table[PURE_SHIFTED_PUNGS]) {
        points_table[PURE_TRIPLE_CHOW] = 0;
    }
    if (points_table[UPPER_TILES]) {
        points_table[UPPER_FOUR] = 0;
        points_table[NO_HONORS] = 0;
    }
    if (points_table[MIDDLE_TILES]) {
        points_table[ALL_SIMPLES] = 0;
        points_table[NO_HONORS] = 0;
    }
    if (points_table[LOWER_TILES]) {
        points_table[LOWER_FOUR] = 0;
        points_table[NO_HONORS] = 0;
    }

    if (points_table[PURE_STRAIGHT]) {

    }
    if (points_table[THREE_SUITED_TERMINAL_CHOWS]) {
        points_table[ALL_CHOWS] = 0;
        points_table[NO_HONORS] = 0;
        points_table[MIXED_DOUBLE_CHOW] = 0;
        points_table[TWO_TERMINAL_CHOWS] = 0;
    }
    if (points_table[PURE_SHIFTED_CHOWS]) {

    }
    if (points_table[ALL_FIVE]) {
        points_table[ALL_SIMPLES] = 0;
        points_table[NO_HONORS] = 0;
    }
    if (points_table[TRIPLE_PUNG]) {

    }
    if (points_table[THREE_CONCEALED_PUNGS]) {

    }

    if (points_table[LESSER_HONORS_AND_KNITTED_TILES]) {
        points_table[CONCEALED_HAND] = 0;
        points_table[ALL_TYPES] = 0;
    }
    if (points_table[KNITTED_STRAIGHT]) {

    }
    if (points_table[UPPER_FOUR]) {
        points_table[NO_HONORS] = 0;
    }
    if (points_table[LOWER_FOUR]) {
        points_table[NO_HONORS] = 0;
    }
    if (points_table[BIG_THREE_WINDS]) {
        points_table[ONE_VOIDED_SUIT] = 0;
    }

    if (points_table[MIXED_STRAIGHT]) {

    }
    if (points_table[REVERSIBLE_TILES]) {
        points_table[ONE_VOIDED_SUIT] = 0;
    }
    if (points_table[MIXED_TRIPLE_CHOW]) {

    }
    if (points_table[MIXED_SHIFTED_PUNGS]) {

    }
    if (points_table[CHICKEN_HAND]) {

    }
    if (points_table[LAST_TILE_DRAW]) {
        points_table[SELF_DRAWN] = 0;
    }
    if (points_table[LAST_TILE_CLAIM]) {

    }
    if (points_table[OUT_WITH_REPLACEMENT_TILE]) {
        points_table[SELF_DRAWN] = 0;
    }
    if (points_table[ROBBING_THE_KONG]) {
        points_table[LAST_TILE] = 0;
    }
    if (points_table[TWO_CONCEALED_KONGS]) {
        points_table[CONCEALED_KONG] = 0;
    }

    if (points_table[ALL_PUNGS]) {

    }
    if (points_table[HALF_FLUSH]) {
        points_table[ONE_VOIDED_SUIT] = 0;
    }
    if (points_table[MIXED_SHIFTED_CHOWS]) {

    }
    if (points_table[ALL_TYPES]) {

    }
    if (points_table[MELDED_HAND]) {
        points_table[SINGLE_WAIT] = 0;
    }
    if (points_table[TWO_DRAGONS_PUNGS]) {
        points_table[DRAGON_PUNG] = 0;
    }
    if (points_table[OUTSIDE_HAND]) {

    }

    if (points_table[FULLY_CONCEALED_HAND]) {
        points_table[SELF_DRAWN] = 0;
    }
    if (points_table[TWO_MELDED_KONGS]) {
        points_table[MELDED_KONG] = 0;
    }
    if (points_table[LAST_TILE]) {

    }

    if (points_table[DRAGON_PUNG]) {

    }
    if (points_table[PREVALENT_WIND] || points_table[SEAT_WIND]) {
        if (!points_table[BIG_THREE_WINDS] && !points_table[LITTLE_FOUR_WINDS]
            && !points_table[ALL_HONORS] && !points_table[ALL_TERMINALS_AND_HONORS]) {
            assert(points_table[PUNG_OF_TERMINALS_OR_HONORS] > 0);
            --points_table[PUNG_OF_TERMINALS_OR_HONORS];
        }
    }
    if (points_table[CONCEALED_HAND]) {

    }
    if (points_table[ALL_CHOWS]) {
        points_table[NO_HONORS] = 0;
    }
    if (points_table[TILE_HOG]) {

    }
    if (points_table[DOUBLE_PUNG]) {

    }
    if (points_table[TWO_CONCEALED_PUNGS]) {

    }
    if (points_table[CONCEALED_KONG]) {

    }
    if (points_table[ALL_SIMPLES]) {
        points_table[NO_HONORS] = 0;
    }

    if (points_table[PURE_DOUBLE_CHOW]) {

    }
    if (points_table[MIXED_DOUBLE_CHOW]) {

    }
    if (points_table[SHORT_STRAIGHT]) {

    }
    if (points_table[TWO_TERMINAL_CHOWS]) {

    }
    if (points_table[PUNG_OF_TERMINALS_OR_HONORS]) {

    }
    if (points_table[MELDED_KONG]) {

    }
    if (points_table[ONE_VOIDED_SUIT]) {

    }
    if (points_table[NO_HONORS]) {

    }
    if (points_table[EDGE_WAIT]) {

    }
    if (points_table[CLOSED_WAIT]) {

    }
    if (points_table[SINGLE_WAIT]) {

    }
    if (points_table[SELF_DRAWN]) {

    }
}

static void check_win_type(WIN_TYPE win_type, long (&points_table)[FLOWER_TILES]) {
    if (win_type & WIN_TYPE_4TH_TILE) {
        points_table[LAST_TILE] = 1;
    }
    if (win_type & WIN_TYPE_WALL_LAST) {
        points_table[win_type & WIN_TYPE_SELF_DRAWN ? LAST_TILE_DRAW : LAST_TILE_CLAIM] = 1;
    }
    if (win_type & WIN_TYPE_ABOUT_KONG) {
        points_table[win_type & WIN_TYPE_SELF_DRAWN ? OUT_WITH_REPLACEMENT_TILE : ROBBING_THE_KONG] = 1;
    }
    if (win_type & WIN_TYPE_SELF_DRAWN) {
        points_table[SELF_DRAWN] = 1;
    }
}

static void calculate_basic_type_points(const SET (&sets)[5], long fixed_cnt, TILE win_tile, WIN_TYPE win_type,
    WIND_TYPE prevalent_wind, WIND_TYPE seat_wind, long (&points_table)[FLOWER_TILES]) {
    SET pair_set;
    SET chow_sets[4];
    SET pung_sets[4];
    long chow_cnt = 0;
    long pung_cnt = 0;
    for (long i = 0; i < 5; ++i) {
        switch (sets[i].set_type) {
        case SET_TYPE::CHOW: chow_sets[chow_cnt++] = sets[i]; break;
        case SET_TYPE::PUNG:
        case SET_TYPE::KONG: pung_sets[pung_cnt++] = sets[i]; break;
        case SET_TYPE::PAIR: pair_set = sets[i]; break;
        default: assert(0); break;
        }
    }

    TILE tiles[18];
    long tile_cnt = 0;
    recovery_tiles_from_sets(sets, 5, tiles, &tile_cnt);

    if (fixed_cnt == 0 && tile_cnt == 14) {
        if (is_nine_gates(tiles, win_tile)) {
            points_table[NINE_GATES] = 1;
        }
    }

    check_win_type(win_type, points_table);

    // make up a concealed pung by self-drawn
    if ((win_type & WIN_TYPE_SELF_DRAWN) == 0) {
        for (long i = 0; i < pung_cnt; ++i) {
            if (pung_sets[i].mid_tile == win_tile && !pung_sets[i].is_melded) {
                pung_sets[i].is_melded = true;  // default is concealed
            }
        }
    }

    switch (chow_cnt) {
    case 4:
        if (is_pure_terminal_chows(chow_sets, pair_set)) {
            points_table[PURE_TERMINAL_CHOWS] = 1;
            break;
        }
        if (is_three_suited_terminal_chows(chow_sets, pair_set)) {
            points_table[THREE_SUITED_TERMINAL_CHOWS] = 1;
            break;
        }
        calculate_4_chows(chow_sets, points_table);
        break;
    case 3:
        calculate_3_chows(chow_sets, points_table);
        calculate_1_pung(pung_sets[0], points_table);
        break;
    case 2:
        calculate_2_chows(chow_sets, points_table);
        calculate_2_pungs(pung_sets, points_table);
        break;
    case 1:
        calculate_3_pungs(pung_sets, points_table);
        break;
    case 0:
        calculate_4_pungs(pung_sets, points_table);
        break;
    default:
        break;
    }

    check_melded_or_concealed_hand(sets, fixed_cnt, win_type & WIN_TYPE_SELF_DRAWN, points_table);

    check_pair_tile(pair_set.mid_tile, chow_cnt, points_table);

    check_tiles_rank_by_set(sets, points_table);

    check_tiles_traits(tiles, tile_cnt, points_table);
    check_tiles_rank_by_tiles(tiles, tile_cnt, points_table);
    check_tiles_suits(tiles, tile_cnt, points_table);
    check_tiles_hog(tiles, tile_cnt, points_table);
    check_edge_closed_single_wait(sets + fixed_cnt, 5 - fixed_cnt, win_tile, points_table);

    for (int i = 0; i < 5; ++i) {
        check_wind_pungs(sets[i], prevalent_wind, seat_wind, points_table);
    }

    correction_points_table(points_table);

    if (std::all_of(std::begin(points_table), std::end(points_table), [](int p) { return p == 0; })) {
        points_table[CHICKEN_HAND] = 1;
    }
}

static bool calculate_special_type_points(const TILE (&concealed_tiles)[14], WIN_TYPE win_type, long (&points_table)[FLOWER_TILES]) {
    if (concealed_tiles[0] == concealed_tiles[1]
        && concealed_tiles[2] == concealed_tiles[3]
        && concealed_tiles[4] == concealed_tiles[5]
        && concealed_tiles[6] == concealed_tiles[7]
        && concealed_tiles[8] == concealed_tiles[9]
        && concealed_tiles[10] == concealed_tiles[11]
        && concealed_tiles[12] == concealed_tiles[13]) {

        if (concealed_tiles[0] + 1 == concealed_tiles[2]
            && concealed_tiles[2] + 1 == concealed_tiles[4]
            && concealed_tiles[4] + 1 == concealed_tiles[6]
            && concealed_tiles[6] + 1 == concealed_tiles[8]
            && concealed_tiles[8] + 1 == concealed_tiles[10]
            && concealed_tiles[10] + 1 == concealed_tiles[12]) {
            points_table[SEVEN_SHIFTED_PAIRS] = 1;

            check_tiles_rank_by_tiles(concealed_tiles, 14, points_table);
        }
        else {
            points_table[SEVEN_PAIRS] = 1;

            check_tiles_traits(concealed_tiles, 14, points_table);
            check_tiles_rank_by_tiles(concealed_tiles, 14, points_table);
            check_tiles_suits(concealed_tiles, 14, points_table);
            check_tiles_hog(concealed_tiles, 14, points_table);
        }
    }
    else if (std::includes(std::begin(concealed_tiles), std::end(concealed_tiles),
        std::begin(standard_thirteen_orphans), std::end(standard_thirteen_orphans))) {
        points_table[THIRTEEN_ORPHANS] = 1;
    }
    else {
        const TILE *it = std::find_if(std::begin(concealed_tiles), std::end(concealed_tiles), &is_honor);
        long numbered_cnt = it - concealed_tiles;
        // quick return if numbered tiles more than 9 or honor tiles more than 7 (i.e. numbered_cnt less than 6)
        if (numbered_cnt > 9 || numbered_cnt < 7) {
            return false;
        }

        const TILE *matched_seq = nullptr;
        for (int i = 0; i < 6; ++i) {
            if (std::includes(std::begin(standard_knitted_straight[i]), std::end(standard_knitted_straight[i]),
                std::begin(concealed_tiles), it)) {
                matched_seq = standard_knitted_straight[i];  // match a knitted straight
                break;
            }
        }

        if (matched_seq == nullptr) {
            return false;
        }

        // standard_thirteen_orphans + 6 is honors i.e. ESWNCFP
        if (numbered_cnt == 7 && memcmp(standard_thirteen_orphans + 6, concealed_tiles + 7, 7 * sizeof(TILE)) == 0) {
            points_table[GREATER_HONORS_AND_KNITTED_TILES] = 1;
        }
        else if (std::includes(standard_thirteen_orphans + 6, standard_thirteen_orphans + 13, it, std::end(concealed_tiles))) {
            points_table[LESSER_HONORS_AND_KNITTED_TILES] = 1;
            if (numbered_cnt == 9) {
                points_table[KNITTED_STRAIGHT] = 1;
            }
        }
    }

    check_win_type(win_type, points_table);
    correction_points_table(points_table);
    return true;
}

static bool calculate_knitted_straight_in_basic_type_points(SET &fourth_set, const TILE *concealed_tiles, long concealed_cnt,
    TILE win_tile, WIN_TYPE win_type, WIND_TYPE prevalent_wind, WIND_TYPE seat_wind, long (&points_table)[FLOWER_TILES]) {
    assert(concealed_cnt == 14 || concealed_cnt == 11);
    const TILE *matched_seq = nullptr;
    for (int i = 0; i < 6; ++i) {
        if (std::includes(concealed_tiles, concealed_tiles + concealed_cnt,
            std::begin(standard_knitted_straight[i]), std::end(standard_knitted_straight[i]))) {
            matched_seq = standard_knitted_straight[i];  // match a knitted straight
            break;
        }
    }

    if (matched_seq == nullptr) {
        return false;
    }

    TILE pair_tile = 0;

    TILE remains[5];
    TILE *end = copy_exclude(concealed_tiles, concealed_tiles + concealed_cnt, matched_seq, matched_seq + 9, remains);
    long remain_cnt = end - remains;
    if (remain_cnt == 5) {  // 4th set is concealed, remain 14-9=5, contains a pair and a chow/pung
        TILE *it = std::adjacent_find(std::begin(remains), std::end(remains));  // find the pair
        while (it != std::end(remains)) {
            pair_tile = *it;  // restore the pair tile
            TILE pair[2] = { pair_tile, pair_tile };
            TILE temp[3];  // tiles except the pair
            copy_exclude(std::begin(remains), std::end(remains), std::begin(pair), std::end(pair), temp);

            if (is_chow(temp[0], temp[1], temp[2])) {
                fourth_set.is_melded = false;
                fourth_set.mid_tile = temp[1];
                fourth_set.set_type = SET_TYPE::CHOW;
                break;
            }
            else if (is_pung(temp[0], temp[1], temp[2])) {
                fourth_set.is_melded = false;
                fourth_set.mid_tile = temp[1];
                fourth_set.set_type = SET_TYPE::PUNG;
                break;
            }

            // if meets a pung first, test above will fail
            do ++it; while (it != std::end(remains) && *it == pair_tile);
            it = std::adjacent_find(it, std::end(remains));
            pair_tile = 0;
        }
        if (pair_tile == 0) {
            return false;
        }
    }
    else {
        // 4th set is melded, remain 11-9=2, it must be the pair
        assert(remain_cnt == 2);
        if (remains[0] != remains[1]) {
            return false;
        }
        pair_tile = remains[0];
        if (fourth_set.set_type != SET_TYPE::CHOW
            && fourth_set.set_type != SET_TYPE::PUNG
            && fourth_set.set_type != SET_TYPE::KONG) {
            return false;
        }
    }

    points_table[KNITTED_STRAIGHT] = 1;
    if (fourth_set.set_type == SET_TYPE::CHOW) {
        if (is_numbered_suit_quick(pair_tile)) {
            points_table[ALL_CHOWS] = 1;  // it is a special all chows
        }
    }
    else {
        POINT_TYPE points = get_1_pung_points(fourth_set.mid_tile);
        if (points != NONE) {
            ++points_table[points];
        }
    }

    check_win_type(win_type, points_table);
    if (win_type & WIN_TYPE_SELF_DRAWN) {
        points_table[FULLY_CONCEALED_HAND] = 1;
    }
    else {
        points_table[CONCEALED_HAND] = 1;
    }

    check_tiles_traits(concealed_tiles, 14, points_table);
    check_tiles_rank_by_tiles(concealed_tiles, 14, points_table);
    check_tiles_suits(concealed_tiles, 14, points_table);
    check_tiles_hog(concealed_tiles, 14, points_table);

    // if the win tile is not in knitted straight, we should check whether edge/closed/single wait
    if (std::find(matched_seq, matched_seq + 9, win_tile) == matched_seq + 9) {
        if (remain_cnt == 5) {
            SET sets[2];
            sets[0].is_melded = fourth_set.is_melded;
            sets[0].set_type = fourth_set.set_type;
            sets[0].mid_tile = fourth_set.mid_tile;
            sets[1].is_melded = false;
            sets[1].set_type = SET_TYPE::PAIR;
            sets[1].mid_tile = pair_tile;

            check_edge_closed_single_wait(sets, 2, win_tile, points_table);
        }
        else {
            assert(remain_cnt == 2);
            points_table[SINGLE_WAIT] = 1;
        }
    }

    check_wind_pungs(fourth_set, prevalent_wind, seat_wind, points_table);

    correction_points_table(points_table);
    return true;
}

static int get_points_by_table(const long (&points_table)[FLOWER_TILES]) {
    int points = 0;
    for (int i = 1; i < FLOWER_TILES; ++i) {
        if (points_table[i] == 0) {
            continue;
        }
        points += points_value_table[i] * points_table[i];
        if (points_table[i] == 1) {
            printf("%s %d\n", points_name[i], points_value_table[i]);
        }
        else {
            printf("%s %d*%ld\n", points_name[i], points_value_table[i], points_table[i]);
        }
    }
    return points;
}

const char *parse_tiles(const char *str, TILE *tiles, long *out_tile_cnt) {
    //std::regex reg("[1-9]+[mps]|[ESWNCFP]");
    //if (std::regex_match(str, reg)) {
    //    std::cout << "cannot parse the string" << std::endl;
    //    return;
    //}

    long tile_cnt = 0;
    TILE temp[14];
    long temp_cnt = 0;
    const char *p = str;
    for (; temp_cnt < 14 && tile_cnt < 14 && *p != '\0'; ++p) {
        char c = *p;
        switch (c) {
        case '1': temp[temp_cnt++] = 1; break;
        case '2': temp[temp_cnt++] = 2; break;
        case '3': temp[temp_cnt++] = 3; break;
        case '4': temp[temp_cnt++] = 4; break;
        case '5': temp[temp_cnt++] = 5; break;
        case '6': temp[temp_cnt++] = 6; break;
        case '7': temp[temp_cnt++] = 7; break;
        case '8': temp[temp_cnt++] = 8; break;
        case '9': temp[temp_cnt++] = 9; break;
        case 'm':
            for (long i = 0; i < temp_cnt && tile_cnt < 14; ++i) {
                tiles[tile_cnt++] = temp[i] | 0x10;
            }
            temp_cnt = 0;
            break;
        case 's':
            for (long i = 0; i < temp_cnt && tile_cnt < 14; ++i) {
                tiles[tile_cnt++] = temp[i] | 0x20;
            }
            temp_cnt = 0;
            break;
        case 'p':
            for (long i = 0; i < temp_cnt && tile_cnt < 14; ++i) {
                tiles[tile_cnt++] = temp[i] | 0x30;
            }
            temp_cnt = 0;
            break;
        case 'E': tiles[tile_cnt++] = 0x41; break;
        case 'S': tiles[tile_cnt++] = 0x42; break;
        case 'W': tiles[tile_cnt++] = 0x43; break;
        case 'N': tiles[tile_cnt++] = 0x44; break;
        case 'C': tiles[tile_cnt++] = 0x51; break;
        case 'F': tiles[tile_cnt++] = 0x52; break;
        case 'P': tiles[tile_cnt++] = 0x53; break;
        default: goto end;
        }
    }

end:
    if (temp_cnt != 0) {
        puts("Expect m/s/p to finish a series of numbers");
        return nullptr;
    }
    if (out_tile_cnt != nullptr) {
        *out_tile_cnt = tile_cnt;
    }
    return p;
}

bool string_to_tiles(const char *str, SET *fixed_sets, long *fixed_set_cnt, TILE *concealed_tiles, long *concealed_cnt) {
    SET sets[5];
    long set_cnt = 0;
    bool has_fixed_set = false, is_concealed_kong = false;
    TILE tiles[14];
    long tile_cnt = 0;

    const char *p = str;
    while (char c = *p) {
        const char *q;
        switch (c) {
        case '[':
            q = ++p;
            is_concealed_kong = false;
            has_fixed_set = true;
            break;
        case ']':
            if (!has_fixed_set) {
                puts("Closing bracket ] does not match");
                return false;
            }
            if (tile_cnt != 3 && tile_cnt != 4) {
                puts("Only 3 or 4 tiles allowed in a fixed set between [ ]");
                return false;
            }
            if (tile_cnt == 3) {
                if (is_concealed_kong) {
                    puts("Concealed kong need 4 same tiles");
                    return false;
                }
                if (tiles[0] + 1 == tiles[1] && tiles[1] + 1 == tiles[2]) {
                    sets[set_cnt].mid_tile = tiles[1];
                    sets[set_cnt].set_type = SET_TYPE::CHOW;
                    sets[set_cnt].is_melded = true;
                }
                else if (tiles[0] == tiles[1] && tiles[1] == tiles[2]) {
                    sets[set_cnt].mid_tile = tiles[1];
                    sets[set_cnt].set_type = SET_TYPE::PUNG;
                    sets[set_cnt].is_melded = true;
                }
                else {
                    puts("Cannot make a chow or pung");
                    return false;
                }
            }
            else {
                if (tiles[0] != tiles[1] || tiles[1] != tiles[2] || tiles[2] != tiles[3]) {
                    puts("Kong need 4 same tiles");
                }
                sets[set_cnt].mid_tile = tiles[0];
                sets[set_cnt].set_type = SET_TYPE::KONG;
                sets[set_cnt].is_melded = true;
            }
            q = ++p;
            has_fixed_set = false;
            ++set_cnt;
            tile_cnt = 0;
            break;
        case '{':
            q = ++p;
            is_concealed_kong = true;
            has_fixed_set = true;
            break;
        case '}':
            if (!has_fixed_set) {
                puts("Closing bracket } does not match");
                return false;
            }
            if (!is_concealed_kong) {
                puts("{} is only for concealed kong");
                return false;
            }
            if (tile_cnt != 4) {
                puts("Concealed kong need 4 same tiles");
                return false;
            }
            q = ++p;
            sets[set_cnt].mid_tile = tiles[0];
            sets[set_cnt].set_type = SET_TYPE::KONG;
            sets[set_cnt].is_melded = false;
            has_fixed_set = false;
            ++set_cnt;
            tile_cnt = 0;
            break;
        default:
            q = parse_tiles(p, tiles, &tile_cnt);
            if (q == p || q == nullptr) {
                puts("Unexpect character");
                return false;
            }
            break;
        }
        p = q;
    }

    if (has_fixed_set) {
        puts("Expect closing bracket!");
        return false;
    }

    //for (long i = 0; i < set_cnt; ++i) {
    //    printf("[%d %s %x] ", sets[i].is_melded, set_type_name[(int)sets[i].set_type], sets[i].mid_tile);
    //}
    //for (long i = 0; i < tile_cnt; ++i) {
    //    printf("%x ", tiles[i]);
    //}
    //puts("");
    if (fixed_sets != nullptr) {
        memcpy(fixed_sets, sets, set_cnt * sizeof(SET));
    }
    if (fixed_set_cnt != nullptr) {
        *fixed_set_cnt = set_cnt;
    }
    memcpy(concealed_tiles, tiles, tile_cnt * sizeof(TILE));
    if (concealed_cnt != nullptr) {
        *concealed_cnt = tile_cnt;
    }

    return true;
}

int calculate_points(const SET *fixed_set, long fixed_cnt, const TILE *concealed_tiles, long concealed_cnt, TILE win_tile, WIN_TYPE win_type, WIND_TYPE prevalent_wind, WIND_TYPE seat_wind, long *points_table) {

    assert(concealed_tiles != nullptr);
    assert(concealed_cnt > 0);

    if (fixed_set == nullptr) {
        fixed_cnt = 0;
    }

    //switch (fixed_cnt) {
    //case 0: if (concealed_cnt != 13) return false; break;
    //case 1: if (concealed_cnt != 10) return false; break;
    //case 2: if (concealed_cnt != 7) return false; break;
    //case 3: if (concealed_cnt != 4) return false; break;
    //case 4: if (concealed_cnt != 1) return false; break;
    //default: return false;
    //}
    if (fixed_cnt < 0 || fixed_cnt > 4 || fixed_cnt * 3 + concealed_cnt != 13) {
        return ERROR_WRONG_TILES_COUNT;
    }

    TILE _concealed_tiles[14];
    SET _separation_sets[MAX_SEPARAION_CNT][5];
    long _separation_cnt;

    // combine win tile with concealed tiles
    memcpy(_concealed_tiles, concealed_tiles, sizeof(TILE) * concealed_cnt);
    _concealed_tiles[concealed_cnt] = win_tile;
    sort_tiles(_concealed_tiles, concealed_cnt + 1);

    memcpy(_separation_sets[0], fixed_set, fixed_cnt * sizeof(SET));
    for (long i = 0; i < fixed_cnt; ++i) {
        if (fixed_set[i].set_type != SET_TYPE::KONG) {
            _separation_sets[0][i].is_melded = true;
        }
    }

    _separation_cnt = 0;
    seperate_N(_concealed_tiles, concealed_cnt + 1, fixed_cnt, _separation_sets, &_separation_cnt);

    for (long i = 0; i < _separation_cnt; ++i) {
        std::sort(&_separation_sets[i][fixed_cnt], &_separation_sets[i][4], &set_cmp);
    }

    long points_tables[MAX_SEPARAION_CNT][FLOWER_TILES] = { 0 };
    int max_points = 0;
    long max_idx = -1;

    if (fixed_cnt == 0) {  // concealed hand, it may be special type or knitted straight in basic type
        if (calculate_knitted_straight_in_basic_type_points(_separation_sets[_separation_cnt][0], _concealed_tiles, 14,
            win_tile, win_type, prevalent_wind, seat_wind, points_tables[_separation_cnt])) {
            int current_points = get_points_by_table(points_tables[_separation_cnt]);
            if (current_points > max_points) {
                max_points = current_points;
                max_idx = _separation_cnt;
            }
            printf("points = %d\n\n", current_points);
        }
        else if (calculate_special_type_points(_concealed_tiles, win_type, points_tables[_separation_cnt])) {
            int current_points = get_points_by_table(points_tables[_separation_cnt]);
            if (current_points > max_points) {
                max_points = current_points;
                max_idx = _separation_cnt;
            }
            printf("points = %d\n\n", current_points);
            if (points_tables[_separation_cnt][SEVEN_SHIFTED_PAIRS]) {
                _separation_cnt = 0;
            }
        }
    }
    else if (fixed_cnt == 1 && _separation_cnt == 0) {
        // 1 completed chow, pung or kong and cannot be separated, it must be knitted straight in basic type
        if (calculate_knitted_straight_in_basic_type_points(_separation_sets[0][0], _concealed_tiles, 14,
            win_tile, win_type, prevalent_wind, seat_wind, points_tables[0])) {
            int current_points = get_points_by_table(points_tables[0]);
            if (current_points > max_points) {
                max_points = current_points;
                max_idx = _separation_cnt;
            }
            printf("points = %d\n\n", current_points);
        }
    }

    // traverses each separation to find the maximum points
    for (long i = 0; i < _separation_cnt; ++i) {
        for (int j = 0; j < 5; ++j) {
            //printf("[%d %s %x]", _separation_sets[i][j].is_melded,
            //    set_type_name[(int)_separation_sets[i][j].set_type], _separation_sets[i][j].mid_tile);
            TILE mid_tile = _separation_sets[i][j].mid_tile;
            switch (_separation_sets[i][j].set_type) {
            case SET_TYPE::CHOW:
                printf(_separation_sets[i][j].is_melded ? "[%s%s%s]" : "{%s%s%s}",
                    stringify_table[mid_tile - 1], stringify_table[mid_tile], stringify_table[mid_tile + 1]);
                break;
            case SET_TYPE::PUNG:
                printf(_separation_sets[i][j].is_melded ? "[%s%s%s]" : "{%s%s%s}",
                    stringify_table[mid_tile], stringify_table[mid_tile], stringify_table[mid_tile]);
                break;
            case SET_TYPE::KONG:
                printf(_separation_sets[i][j].is_melded ? "[%s%s%s%s]" : "{%s%s%s%s}",
                    stringify_table[mid_tile], stringify_table[mid_tile], stringify_table[mid_tile], stringify_table[mid_tile]);
                break;
            case SET_TYPE::PAIR:
                printf(_separation_sets[i][j].is_melded ? "[%s%s]" : "{%s%s}",
                    stringify_table[mid_tile], stringify_table[mid_tile]);
                break;
            default:
                break;
            }
        }
        puts("");

        calculate_basic_type_points(_separation_sets[i], fixed_cnt, win_tile, win_type, prevalent_wind, seat_wind, points_tables[i]);
        int current_points = get_points_by_table(points_tables[i]);
        if (current_points > max_points) {
            max_points = current_points;
            max_idx = i;
        }
        printf("points = %d\n\n", current_points);
    }
    if (max_idx == -1) {
        return ERROR_NOT_WIN;
    }

    if (points_table != nullptr) {
        memcpy(points_table, points_tables[max_idx], sizeof(points_tables[max_idx]));
    }
    return max_points;
}
