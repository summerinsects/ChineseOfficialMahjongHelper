#include "tile.h"
#include "shanten.h"
#include "stringify.h"
#include "fan_calculator.h"

#include <stdio.h>
#include <iostream>
#include <limits>
#include <assert.h>
#include <time.h>
#include <string.h>

using namespace mahjong;

static int count_useful_tile(const tile_table_t &used_table, const useful_table_t &useful_table) {
    int cnt = 0;
    for (int i = 0; i < 34; ++i) {
        tile_t t = all_tiles[i];
        if (useful_table[t]) {
            cnt += 4 - used_table[t];
        }
    }
    return cnt;
}

void test_wait(const char *str) {
    hand_tiles_t hand_tiles;
    tile_t serving_tile;
    string_to_tiles(str, strlen(str), &hand_tiles, &serving_tile);

    std::cout << "----------------" << std::endl;
    puts(str);
    useful_table_t useful_table;
    bool is_wait = mahjong::is_waiting(hand_tiles, &useful_table);
    if (is_wait) {
        puts(" waiting:");
        char buf[64];
        for (tile_t t = TILE_1m; t < TILE_TABLE_SIZE; ++t) {
            if (useful_table[t]) {
                tiles_to_string(&t, 1, buf, sizeof(buf));
                printf("%s ", buf);
            }
        }
    }
    else {
        puts("not wait!");
    }
    puts("");
}

void test_points(const char *str, win_flag_t win_flag, wind_t prevalent_wind, wind_t seat_wind) {
    calculate_param_t param;

    long ret = string_to_tiles(str, strlen(str), &param.hand_tiles, &param.win_tile);
    if (ret != PARSE_NO_ERROR) {
        printf("error at line %d error = %ld\n", __LINE__, ret);
        return;
    }

    fan_table_t fan_table/* = { 0 }*/;
    puts("----------------");
    puts(str);

    param.flower_count = 0;

    param.win_flag = win_flag;
    param.prevalent_wind = prevalent_wind;
    param.seat_wind = seat_wind;
    int points = calculate_fan(&param, &fan_table);

#if SUPPORT_BLESSINGS
    points -= revoke_blessings(fan_table);
#endif

    printf("max points = %d\n\n", points);
    if (points < 0) {
        return;
    }

    for (int i = 1; i < FAN_TABLE_SIZE; ++i) {
        if (fan_table[i] == 0) {
            continue;
        }
        if (fan_table[i] == 1) {
            printf("%s %d\n", fan_name[i], fan_value_table[i]);
        }
        else {
            printf("%s %d*%hd\n", fan_name[i], fan_value_table[i], fan_table[i]);
        }
    }
}

void test_shanten(const char *str) {
    hand_tiles_t hand_tiles;
    tile_t serving_tile;
    long ret = string_to_tiles(str, strlen(str), &hand_tiles, &serving_tile);
    if (ret != 0) {
        printf("error at line %d error = %ld\n", __LINE__, ret);
        return;
    }

    char buf[20];
    ret = hand_tiles_to_string(&hand_tiles, buf, sizeof(buf));
    puts(buf);

    auto display = [](const hand_tiles_t *hand_tiles, useful_table_t &useful_table) {
        char buf[64];
        for (tile_t t = TILE_1m; t < TILE_TABLE_SIZE; ++t) {
            if (useful_table[t]) {
                tiles_to_string(&t, 1, buf, sizeof(buf));
                printf("%s ", buf);
            }
        }

        tile_table_t cnt_table;
        map_hand_tiles(hand_tiles, &cnt_table);

        printf("%d枚", count_useful_tile(cnt_table, useful_table));
    };

    puts(str);
    useful_table_t useful_table/* = {false}*/;
    int ret0;
    ret0 = thirteen_orphans_shanten(hand_tiles.standing_tiles, hand_tiles.tile_count, &useful_table);
    printf("131=== %d shanten\n", ret0);
    if (ret0 != std::numeric_limits<int>::max()) display(&hand_tiles, useful_table);
    puts("\n");

    ret0 = seven_pairs_shanten(hand_tiles.standing_tiles, hand_tiles.tile_count, &useful_table);
    printf("7d=== %d shanten\n", ret0);
    if (ret0 != std::numeric_limits<int>::max()) display(&hand_tiles, useful_table);
    puts("\n");

    ret0 = honors_and_knitted_tiles_shanten(hand_tiles.standing_tiles, hand_tiles.tile_count, &useful_table);
    printf("honors and knitted tiles %d shanten\n", ret0);
    if (ret0 != std::numeric_limits<int>::max()) display(&hand_tiles, useful_table);
    puts("\n");

    ret0 = knitted_straight_shanten(hand_tiles.standing_tiles, hand_tiles.tile_count, &useful_table);
    printf("knitted straight in regular form %d shanten\n", ret0);
    if (ret0 != std::numeric_limits<int>::max()) display(&hand_tiles, useful_table);
    puts("\n");

    ret0 = regular_shanten(hand_tiles.standing_tiles, hand_tiles.tile_count, &useful_table);
    printf("regular form %d shanten\n", ret0);
    if (ret0 != std::numeric_limits<int>::max()) display(&hand_tiles, useful_table);
    puts("\n");
}

int main(int argc, const char *argv[]) {
#ifdef _MSC_VER
    system("chcp 65001");
#endif

    //test_shanten("19m19s22pESWCFPP");
    //test_shanten("278m3378s3779pEC");
    test_shanten("111m 5m12p1569sSWP");
    test_shanten("[111m]5m12p1569sSWP");
    //return 0;

#if 1
    // 组合龙的边嵌钓问题 2023.10.29
    test_points("[345m3]258m1488s369p7s", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("[345m3]258m1477s369p7s", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("[345m3]258m1478s369p8s", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    // 天和九莲宝灯 2023.10.27
    // 支持的算法：107 93 90 90 92
    // 不支持的算法：47 33 30 30 32
    test_points("11112345678999p", WIN_FLAG_INITIAL | WIN_FLAG_SELF_DRAWN, wind_t::EAST, wind_t::EAST);
    test_points("11122345678999p", WIN_FLAG_INITIAL | WIN_FLAG_SELF_DRAWN, wind_t::EAST, wind_t::EAST);
    test_points("11123345678999p", WIN_FLAG_INITIAL | WIN_FLAG_SELF_DRAWN, wind_t::EAST, wind_t::EAST);
    test_points("11123445678999p", WIN_FLAG_INITIAL | WIN_FLAG_SELF_DRAWN, wind_t::EAST, wind_t::EAST);
    test_points("11123455678999p", WIN_FLAG_INITIAL | WIN_FLAG_SELF_DRAWN, wind_t::EAST, wind_t::EAST);

    // 优先选择三同刻 2023.9.2
    test_points("4445677m777s777p7m", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("4456777m444s444p4m", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    // 组合龙的四归一问题 2023.5.30
    test_points("[678s3]147m5888s369p2s", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("147m258s3666789p6p", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("[666p]147m258s3779p6p", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("[666m]3556m147s258p9m", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("3569m258s122247p5m", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    // 依然是边嵌钓问题 2023.5.20
    test_points("2222444466688m3m", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    // 私货 天和、地和、人和 2023.5.13
    test_points("EEESSCCCFFFPPP", WIN_FLAG_INITIAL | WIN_FLAG_SELF_DRAWN, wind_t::EAST, wind_t::EAST);
    test_points("EEESSCCCFFFPPP", WIN_FLAG_INITIAL, wind_t::EAST, wind_t::EAST);
    test_points("EESSSCCCFFFPPP", WIN_FLAG_INITIAL | WIN_FLAG_SELF_DRAWN, wind_t::SOUTH, wind_t::SOUTH);
    test_points("EESSSCCCFFFPPP", WIN_FLAG_INITIAL, wind_t::SOUTH, wind_t::SOUTH);
    test_points("67m44678s345678p5m", WIN_FLAG_INITIAL | WIN_FLAG_SELF_DRAWN, wind_t::EAST, wind_t::NORTH);
    test_points("456789m456s4568p8p", WIN_FLAG_INITIAL, wind_t::EAST, wind_t::WEST);
    test_points("24m345567s22456p3m", WIN_FLAG_INITIAL | WIN_FLAG_SELF_DRAWN, wind_t::SOUTH, wind_t::SOUTH);
    test_points("78m123456s12399p9m", WIN_FLAG_INITIAL | WIN_FLAG_SELF_DRAWN, wind_t::WEST, wind_t::NORTH);
    test_points("234m23344599s123p", WIN_FLAG_INITIAL | WIN_FLAG_SELF_DRAWN, wind_t::WEST, wind_t::EAST);
    test_points("337788s5566pNNNN", WIN_FLAG_INITIAL, wind_t::EAST, wind_t::SOUTH);
    test_points("33469m258s147pWW2m", WIN_FLAG_INITIAL, wind_t::EAST, wind_t::SOUTH);
    test_points("23358m14447s369p4s", WIN_FLAG_INITIAL | WIN_FLAG_SELF_DRAWN, wind_t::EAST, wind_t::EAST);
    test_points("147m3669s122358p6s", WIN_FLAG_INITIAL | WIN_FLAG_SELF_DRAWN, wind_t::EAST, wind_t::SOUTH);
    test_points("369m258s147pEEPPE", WIN_FLAG_INITIAL | WIN_FLAG_SELF_DRAWN, wind_t::EAST, wind_t::EAST);

    // 组合龙的边嵌钓兼容 2023.5.13
    test_points("1233369m147s258p3m", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("2333469m147s258p3m", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("3369m147s258pEEE3m", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("1223358m147s369p3m", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("2233458m147s369p3m", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("2358m147s369pEEE3m", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    // 拆分一色三步高 2023.5.12
    test_points("12233445667pWW8p", WIN_FLAG_SELF_DRAWN, wind_t::EAST, wind_t::EAST);
    test_points("[456s2][234s3]1223678s2s", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("[789s3]33m34566778s5s", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);  //
    test_points("[234p3]1134557789p6p", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("[345m2][456m]1123468m7m", WIN_FLAG_SELF_DRAWN, wind_t::EAST, wind_t::EAST);  //
    test_points("[678p3][234p3]1145678p9p", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("[567m]2333445667m8m", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);  //
    test_points("1123445567789p3p", WIN_FLAG_SELF_DRAWN, wind_t::EAST, wind_t::EAST);
    test_points("[789p3][678p2][567p]1134p2p", WIN_FLAG_SELF_DRAWN, wind_t::EAST, wind_t::EAST);  //
    test_points("12333445678sFF2s", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);  //
    test_points("[678p]34556778pNN9p", WIN_FLAG_SELF_DRAWN, wind_t::EAST, wind_t::EAST);  //

    // 多种拆法番数一样时，优先选择问题 2022.3.7
    test_points("44556m445566s55p6m", WIN_FLAG_SELF_DRAWN, wind_t::EAST, wind_t::EAST);
    test_points("445566m5s445566p5s", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("[678p]55s222333444p", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("[CCC]11123444789p", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("[CCC]11123444456p", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("[CCC]12366678999p", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("[CCC]45666678999p", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("PPP11123444p231m", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("PPP66678999p789m", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    // 天和相关问题 2019.4.23
    test_points("1112345678999p9p", WIN_FLAG_INITIAL | WIN_FLAG_SELF_DRAWN, wind_t::EAST, wind_t::EAST);
    test_points("1112345678999p9p", WIN_FLAG_SELF_DRAWN, wind_t::EAST, wind_t::EAST);
    test_points("1112345678999p9p", WIN_FLAG_INITIAL, wind_t::EAST, wind_t::EAST);
    test_points("123456m45679p66s8p", WIN_FLAG_INITIAL | WIN_FLAG_SELF_DRAWN, wind_t::EAST, wind_t::EAST);
    test_points("123456m45679p66s8p", WIN_FLAG_SELF_DRAWN, wind_t::EAST, wind_t::EAST);
    test_points("123456m45679p66s8p", WIN_FLAG_INITIAL, wind_t::EAST, wind_t::EAST);

    // BUG测试
    test_points("[234s][234s][234s][234s]6s6s", WIN_FLAG_LAST_TILE, wind_t::EAST, wind_t::EAST);

    test_points("1122233334444s2s", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);  // 剪枝BUG 2018.4.18

    // 组合龙莫名其妙的边张Bug 2018.3.29
    test_points("33469m258s147pWW2m", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    // 套算一次原则相关bug
    test_points("234s2233445678p8p", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST); // 喜相逢、一般高、连六 2017.11.10
    test_points("[123m][789p]789s1299p3p", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);  // 漏算老少副BUG 2017.10.23

    // 清龙+同色龙顺，统一改为一般高 2017.5.22
    test_points("112233456789mEE", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("123445566789sSS", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("123456778899pWW", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    // 三杠少计双暗刻 2017.4.9
    test_points("[2222s][3333s][5555p1]67mEE8m", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    // 七星七对误判为连七对 2017.3.26
    test_points("EESSWWNNCCFFPP", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    // 小四喜的牌型中不会有幺九刻（这样是混幺九）2016.12.15
    test_points("[EEE][WWW][NNN]11sSS1s", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    // 门清一的色双龙漏计边嵌钓 2016.7.28
    test_points("1122355778899m3m", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("1123355778899s2s", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("1122335778899p5p", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    // 七对与基本和型多面听，多计了边嵌钓2016.7.23
    test_points("445566m2277779s8s", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    // 基本测试
    test_wait("19m19s199pESWNCF");  // 十三幺听白
    test_wait("19m19s19pESWNCFP");  // 十三幺13面听

    test_wait("2229999mSSWWFF");  // 七对听2m

    test_wait("369s147pESWNCFP");  // 全不靠听258m
    test_wait("58m369s17pEWNCFP");  // 全不靠听 2m 4p 南
    test_wait("258m369s147pECFP");  // 全不靠听 南 西 北

    test_wait("1112345678999s");  // 九莲宝灯
    test_wait("1112223456777m");
    test_wait("2223334445678m");
    test_wait("25558m369s46778p");  // 组合龙听龙身，1p
    test_wait("25558m369s14677p");  // 组合龙听第四组，58p
    test_wait("25568m369s14777p");  // 组合龙听第四组，47m
    test_wait("258m369s1445677p");  // 组合龙听两面钓将，47p
    test_wait("2233445566778s");
    test_wait("2458m369s147p");  // 组合龙听单钓将，4m
    test_wait("22334455p77779s");  // 基本形听8s、七对听9s

    test_points("445566m445566s5p5p", WIN_FLAG_SELF_DRAWN, wind_t::EAST, wind_t::EAST);

    test_points("[EEEE]22233344m44s4m", WIN_FLAG_SELF_DRAWN, wind_t::EAST, wind_t::EAST);
    test_points("[1111p1]23477m23457p6p", WIN_FLAG_SELF_DRAWN, wind_t::EAST, wind_t::EAST);

    test_points("[222p][123m]456s78pFF9p", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("[222p][123m]456s78pFF6p", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    test_points("1112345678999p9p", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("1122335578899s7s", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("1112223335589s7s", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("12389m123789s55p7m", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("78899m123789s55p7m", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    test_points("24m22s223344567p3m", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    test_points("1223334m445566p3m", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("1122344556677s3s", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    test_points("1112223344455p3p", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("69m258s17pEWNCFP3m", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("69m258s1pESWNCFP3m", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("69m258s147pWNCFP3m", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("2358m369s145677p3m", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("12789m123789s77p3m", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
#endif
    test_points("2223344555667m4m", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("2223344555667m4m", WIN_FLAG_SELF_DRAWN, wind_t::EAST, wind_t::EAST);
    // 理论最高番
    test_points("[EEEE][CCCC][FFFF][PPPP]NN", WIN_FLAG_SELF_DRAWN | WIN_FLAG_KONG_INVOLVED | WIN_FLAG_WALL_LAST, wind_t::EAST, wind_t::EAST);
    test_points("[1111p][2222p][3333p]111s1m1m", WIN_FLAG_SELF_DRAWN, wind_t::EAST, wind_t::EAST);
    test_points("445566m5566p556s6s", WIN_FLAG_SELF_DRAWN, wind_t::EAST, wind_t::EAST);
    test_points("1111222233334s4s", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("12378m123pCCPPP9m", WIN_FLAG_DISCARD | WIN_FLAG_LAST_TILE, wind_t::EAST, wind_t::EAST);
    //return 0;

    // 以下测试用例来自于规则书上
    puts("==== test big four winds ====");
    test_points("[EEE][WWW]SSSNNCCN", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("[EEE][WWW]99mSSSNNN", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("[EEE][WWW]33sSSSNNN", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test big three dragons ====");
    test_points("[CCC][PPP]11m99pFFF1m", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("[CCC][PPP]EEWWFFFE", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("[CCC][PPP]5556sFFF4s", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test all green ====");
    test_points("[234s]23466888sFF6s", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("[234s]22334666sFF4s", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("[222s][444s]3366688s3s", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("223344668888sFF", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test nine gates ====");
    test_points("1112345678999m9m", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test four kongs ====");
    test_points("[2222s1][5555m2][7777p3][EEEE]CC", WIN_FLAG_SELF_DRAWN, wind_t::EAST, wind_t::EAST);
    test_points("[1111m1][2222s2][3333p3][1111s1]4m4m", WIN_FLAG_SELF_DRAWN, wind_t::EAST, wind_t::EAST);
    test_points("[7777p1][NNNN2][CCCC3][3333p1]5p5p", WIN_FLAG_SELF_DRAWN, wind_t::EAST, wind_t::EAST);

    puts("==== test seven shifted pairs ====");
    test_points("1122334455667m7m", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("2233445566778p8p", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test thirteen orphans ====");
    test_points("19m19s19pESWNCFPN", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test all terminals ====");
    test_points("[111m][111s][999m]99s1p1p9s", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test little four winds ====");
    test_points("[EEE][WWW][NNN]23sSS1s", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("[WWW][SSS][NNN]EEPPP", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test little three dragons ====");
    test_points("[CCC][FFF]11199pPP9p", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("[CCC][FFF]23s111pPP1s", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("[CCC][FFF]EEENNPPN", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test all honors ====");
    test_points("[CCC][PPP]EEESSNNS", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test four concealed pungs ====");
    test_points("3444m222s222333p3m", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test pure terminal chows ====");
    test_points("1223355778899s1s", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test quadruple chow ====");
    test_points("[123m][123m]1122334m4m", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test four pure shifted pungs ====");
    test_points("[111p][222p][333p]22s44p4p", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test four pure shifted chows ====");
    test_points("[123m][234m][345m]1145m6m", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("[123s][345s][567s]78s55p9s", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test three kongs ====");
    test_points("[2222m1][3333m2][4444m3]2233s2s", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test all terminals and honors ====");
    test_points("[EEE][111m][999s]99pCC9p", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test seven pairs ====");
    test_points("33m22s77pEENCCPPN", WIN_FLAG_SELF_DRAWN, wind_t::EAST, wind_t::EAST);
    test_points("33336688m22557s7s", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("EESSWWNNCCFFPP", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("1199m1199s11999p9p", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test greater honors and knitted tiles ====");
    test_points("17m36s25pESWNCFP9s", WIN_FLAG_SELF_DRAWN, wind_t::EAST, wind_t::EAST);

    puts("==== test all even pungs ====");
    test_points("[222m][444s][666p]4488p8p", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("[222m][222s][222p]44m44s4m", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("[666m][666s][666p]88m22s8m", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test full flush ====");
    test_points("[111m]2223334449m9m", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("[123s]1112223334s4s", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("[789p]1234567899p9p", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test pure triple chow ====");
    test_points("[456m][456m][456m]4556p5p", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test pure shifted pungs ====");
    test_points("[222s][333s][444s]2233p3p", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test upper tiles ====");
    test_points("[789m][789s][789p]7899p9p", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("[777s][888m][777p]99m88s9m", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("[789m][789s][888s]88m88p8p", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test middle tiles ====");
    test_points("[456s][444s][555s]66s66p6s", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test lower tiles ====");
    test_points("[123p][123m][123s]2333s1s", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test pure straight ====");
    test_points("[123m][456m][789m]2377m1m", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("[123s][456s][789s]6688p6p", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test three suited terminal chows ====");
    test_points("[123p][789p]12378m55s9m", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test pure shifted chows ====");
    test_points("[123p][234p][345p]2234s2s", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("[123s][345s][567s]2345s2s", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("[123m]345567m77s88p8p", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test all five ====");
    test_points("[456p][456s][456m]4555m6m", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("[345m][456m][555p]55m55s5s", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test triple pung ====");
    test_points("[333p][333m]44m23333s4s", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("[111m][111p][111s]99s99p9p", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test three concealed pungs ====");
    test_points("999m11s99pEEECCC1s", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("[123s]4445777888s5s", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test lesser honors and knitted tiles ====");
    test_points("258m147s36pESWFPC", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("147m39s258pEWCFPN", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("147m258s369pSWNCF", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test knitted straight ====");
    test_points("23358m14447s369p4s", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("147m3669s122358p6s", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("369m258s147pEEPPE", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test upper four ====");
    test_points("[789s][678p][777p]78m99s9m", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("[789m][789s][789p]77s78p9p", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("[666s][666p][666m]7788p7p", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test lower four ====");
    test_points("[123s][123m][123p]2333m1m", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("[111s][222s]22m33344s4s", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test big three winds ====");
    test_points("[EEE][SSS][WWW]99m99s9m", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("[SSS][WWW][NNN]2345m5m", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("[SSS][WWW]NNNCCFFC", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test mixed straight ====");
    test_points("[123s][456p]789m23s88p1s", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("[123m][456s][789p]77m45p6p", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test reversible tiles ====");
    test_points("[123p][234p][345p]8899p8p", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("[234p][234p][234p]1123p4p", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("[345p][345p][456s]4555s6s", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("[234p][456s][888p]88sPP8s", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("[111p][222p][333p]4455p4p", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("[222s][456s]4555888s6s", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("[888p][999p][999s]88sPPP", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("1122334455889p9p", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test mixed triple chow ====");
    test_points("[345s][345p][345m]4456m4m", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("[678m][678s][678p]99s67p8p", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test mixed shifted pungs ====");
    test_points("[222p][333s][444m]22m33p3p", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("[777m][888s][999p]99m78p9p", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test chicken hand ====");
    test_points("[123p][444s][789m]34pCC2p", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test last tile draw ====");
    puts("==== test last tile claim ====");
    puts("==== test out with replacement tile ====");
    puts("==== test robbing the kong ====");

    puts("==== test two concealed kongs ====");
    test_points("[1111s][EEEE1][SSS][789m]8m8m", WIN_FLAG_SELF_DRAWN, wind_t::EAST, wind_t::EAST);

    puts("==== test all pungs ====");
    test_points("[888m][888p]888sEEPPP", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test half flush ====");
    test_points("[123m][234m]34578mCC9m", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test mixed shifted chows ====");
    test_points("[123s][234m][345p]55m45s6s", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test all types ====");
    test_points("[123m][456p]789sNNFFF", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test melded hand ====");
    test_points("[2222m1][456p][678p][888s]6m6m", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test two dragons pungs ====");
    test_points("[CCC][FFF]12378m88s9m", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test outside hand ====");
    test_points("[123m][123m][111p]11s11m1s", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("[789p][789m]7788999s9s", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("[123m][123m][789m]78mCC9m", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);
    test_points("[123m][123p]999m78pEE9p", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test fully concealed hand ====");
    test_points("234m4468s345678p7s", WIN_FLAG_SELF_DRAWN, wind_t::EAST, wind_t::EAST);

    puts("==== test two melded kongs ====");
    test_points("[4444p1][4444m1][CCC]1133m1m", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test last tile ====");
    puts("==== test dragon pung ====");
    puts("==== test prevalent wind ====");
    puts("==== test seat wind ====");
    puts("==== test concealed hand ====");
    test_points("234567m66s34567p8p", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test all chows ====");
    test_points("234m456789s3477p5p", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test tile hog ====");
    test_points("[789p][789s][789m]77m33p7m", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test double pung ====");
    test_points("[222m][555m][555s]4488p8p", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test two concealed pungs ====");
    test_points("[9999p]1255789m999s3m", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test concealed kong ====");
    puts("==== test all simples ====");
    test_points("234m456777s3444p5p", WIN_FLAG_DISCARD, wind_t::EAST, wind_t::EAST);

    puts("==== test pure double chow ====");
    puts("==== test mixed double chow ====");
    puts("==== test short straight ====");
    puts("==== test two terminal chows ====");
    puts("==== test pung of terminals or honors ====");
    puts("==== test melded kong ====");
    puts("==== test one voided suit ====");
    puts("==== test no honors ====");
    puts("==== test edge wait ====");
    puts("==== test closed wait ====");
    puts("==== test single wait ====");
    puts("==== test self drawn ====");
    test_points("[1111p1][456s]2789s456p2s", WIN_FLAG_SELF_DRAWN, wind_t::EAST, wind_t::EAST);

    return 0;
}

#include "stringify.cpp"
#include "shanten.cpp"
#include "fan_calculator.cpp"
