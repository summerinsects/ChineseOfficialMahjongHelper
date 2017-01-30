#include "tile.h"
#include "wait_and_win.h"
#include "points_calculator.h"

#include <iostream>
#include <assert.h>

using namespace mahjong;

void test_wait(const char *str) {
    TILE tiles[13];
    long tile_cnt;
    tile_cnt = parse_tiles(str, tiles, 13);

    std::cout << "----------------" << std::endl;
    puts(str);
    bool is_wait = false;
    bool table[0x54] = { false };

    if (tile_cnt == 13) {
        if (is_thirteen_orphans_wait(tiles, tile_cnt, table)) {
            is_wait = true;
            puts("thirteen orphans wait:");
        }
        else if (is_honors_and_knitted_tiles_wait(tiles, tile_cnt, table)) {
            is_wait = true;
            puts("honors and knitted tiles wait:");
        }
        else if (is_seven_pairs_wait(tiles, tile_cnt, table)) {
            is_wait = true;
            printf("seven pairs wait:");
        }
    }

    if (is_basic_type_wait(tiles, tile_cnt, table)) {
        is_wait = true;
        puts("basic type wait:");
    }

    for (TILE t = 0x11; t < 0x54; ++t) {
        if (table[t])
            printf("%s ", stringify_table[t]);
    }
    puts("");

    if (!is_wait) {
        puts("not wait!");
    }
    puts("");
}

void test_points(const char *str, const char *win_str, WIN_TYPE win_type, WIND_TYPE prevalent_wind, WIND_TYPE seat_wind) {
    HAND_TILES hand_tiles;
    long ret = string_to_tiles(str, &hand_tiles);
    if (ret != 0) {
        printf("error at line %d error = %ld\n", __LINE__, ret);
        return;
    }

    TILE win_tile;
    ret = parse_tiles(win_str, &win_tile, 1);
    if (ret != 0) {
        printf("error at line %d error = %ld\n", __LINE__, ret);
        return;
    }

    long points_table[POINT_TYPE_COUNT] = { 0 };
    puts("----------------");
    printf("%s %s\n", str, win_str);
    mahjong::EXTRA_CONDITION ext_cond;
    ext_cond.win_type = win_type;
    ext_cond.prevalent_wind = prevalent_wind;
    ext_cond.seat_wind = seat_wind;
    int points = calculate_points(&hand_tiles, win_tile, &ext_cond, points_table);

    printf("max points = %d\n\n", points);
    //for (int i = 1; i < FLOWER_TILES; ++i) {
    //    if (points_table[i] == 0) {
    //        continue;
    //    }
    //    if (points_table[i] == 1) {
    //        printf("%s %d\n", points_name[i], points_value_table[i]);
    //    }
    //    else {
    //        printf("%s %d*%ld\n", points_name[i], points_value_table[i], points_table[i]);
    //    }
    //}
}

int main(int argc, const char *argv[]) {
    system("chcp 65001");
#if 0
    test_wait("19m19s199pESWNCF");
    test_wait("19m19s19pESWNCFP");

    test_wait("2229999mSSWWFF");

    test_wait("369s147pESWNCFP");
    test_wait("58m369s17pEWNCFP");
    test_wait("258m369s147pECFP");

    test_wait("1112345678999s");
    test_wait("1112223456777m");
    test_wait("2223334445678m");
    test_wait("25558m369s46778p");
    test_wait("25558m369s14677p");
    test_wait("25568m369s14777p");
    test_wait("258m369s1445677p");
    test_wait("2233445566778s");
    test_wait("2458m369s147p");


    test_points("[EEEE]22233344m44s", "4m", WIN_TYPE_SELF_DRAWN, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("1111p 23477m23457p", "6p", WIN_TYPE_SELF_DRAWN, WIND_TYPE::EAST, WIND_TYPE::EAST);

    test_points("222p 123m 456s78pFF", "9p", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("222p 123m 456s78pFF", "6p", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    test_points("1112345678999p", "9p", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("1122335578899s", "7s", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("1112223335589s", "7s", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("12389m123789s55p", "7m", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("78899m123789s55p", "7m", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    test_points("24m22s223344567p", "3m", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    test_points("1223334m445566p", "3m", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("1122344556677s", "3s", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    test_points("1112223344455p", "3p", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("69m258s17pEWNCFP", "3m", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("69m258s1pESWNCFP", "3m", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("69m258s147pWNCFP", "3m", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("2358m369s145677p", "3m", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("12789m123789s77p", "3m", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
#endif
    //test_points("2223344555667m", "4m", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    //test_points("2223344555667m", "4m", WIN_TYPE_SELF_DRAWN, WIND_TYPE::EAST, WIND_TYPE::EAST);
    //test_points("{EEEE}{CCCC}{FFFF}{PPPP}N", "N", true, WIN_TYPE_OUT_WITH_REPLACEMENT_TILE | WIN_TYPE_LAST_TILE_DRAW, WIND_TYPE::EAST, WIND_TYPE::EAST);
    //test_points("{1111p}{2222p}{3333p}111s1m", "1m", WIN_TYPE_SELF_DRAWN, WIND_TYPE::EAST, WIND_TYPE::EAST);
    //test_points("445566m5566p556s", "6s", WIN_TYPE_SELF_DRAWN, WIND_TYPE::EAST, WIND_TYPE::EAST);
    //test_points("1111222233334s", "4s", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    //test_points("12378m123pCCPPP", "9m", WIN_TYPE_DISCARD | WIN_TYPE_4TH_TILE, WIND_TYPE::EAST, WIND_TYPE::EAST);
    //return 0;

    puts("==== test big four winds ====");
    test_points("EEE WWW SSSNNCC", "N", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("EEE WWW 99mSSSNN", "N", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("EEE WWW 33sSSSNN", "N", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test big three dragons ====");
    test_points("CCC PPP 11m99pFFF", "1m", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("CCC PPP EEWWFFF", "E", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("CCC PPP 5556sFFF", "4s", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test all green ====");
    test_points("234s 23466888sFF", "6s", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("234s 22334666sFF", "4s", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("222s 444s 3366688s", "3s", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("223344668888sF", "F", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test nine gates ====");
    test_points("1112345678999m", "9m", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test four kongs ====");
    test_points("2222s 5555m 7777p EEEE C", "C", WIN_TYPE_SELF_DRAWN, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("1111m 2222s 3333p 1111s 4m", "4m", WIN_TYPE_SELF_DRAWN, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("7777p NNNN CCCC 3333p 5p", "5p", WIN_TYPE_SELF_DRAWN, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("CCCC NNNN CCCC NNNN E", "E", WIN_TYPE_SELF_DRAWN, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test seven shifted pairs ====");
    test_points("1122334455667m", "7m", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("2233445566778p", "8p", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test thirteen orphans ====");
    test_points("19m19s19pESWNCFP", "N", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test all terminals ====");
    test_points("111m 111s 999m 99s1p1p", "9s", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test little four winds ====");
    test_points("EEE WWW NNN 23sSS", "1s", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("WWW SSS NNN EEPP", "P", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test little three dragons ====");
    test_points("CCC FFF 11199pPP", "9p", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("CCC FFF 23s111pPP", "1s", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("CCC FFF EEENNPP", "N", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test all honors ====");
    test_points("CCC PPP EEESSNN", "S", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test four concealed pungs ====");
    test_points("3444m222s222333p", "3m", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test pure terminal chows ====");
    test_points("1223355778899s", "1s", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test quadruple chow ====");
    test_points("123m 123m 1122334m", "4m", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test four pure shifted pungs ====");
    test_points("111p 222p 333p 22s44p", "4p", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test four pure shifted chows ====");
    test_points("123m 234m 345m 1145m", "6m", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("123s 345s 567s 78s55p", "9s", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test three kongs ====");
    test_points("2222m 3333m 4444m 2233s", "2s", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test all terminals and honors ====");
    test_points("EEE 111m 999s 99pCC", "9p", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test seven pairs ====");
    test_points("33m22s77pEENCCPP", "N", WIN_TYPE_SELF_DRAWN, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("33336688m22557s", "7s", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("EESSWWNNCCFFP", "P", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("1199m1199s11999p", "9p", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test greater honors and knitted tiles ====");
    test_points("17m36s25pESWNCFP", "9s", WIN_TYPE_SELF_DRAWN, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test all even pungs ====");
    test_points("222m 444s 666p 4488p", "8p", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("222m 222s 222p 44m44s", "4m", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("666m 666s 666s 88m22s", "8m", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test full flush ====");
    test_points("111m 2223334449m", "9m", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("123s 1112223334s", "4s", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("789p 1234567899p", "9p", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test pure triple chow ====");
    test_points("456m 456m 456m 4556p", "5p", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test pure shifted pungs ====");
    test_points("222s 333s 444s 2233p", "3p", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test upper tiles ====");
    test_points("789m 789s 789p 7899p", "9p", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("777s 888m 777p 99m88s", "9m", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("789m 789s 888s 88m88p", "8p", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test middle tiles ====");
    test_points("456s 444s 555s 66s66p", "6s", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test lower tiles ====");
    test_points("123p 123m 123s 2333s", "1s", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test pure straight ====");
    test_points("123m 456m 789m 2377m", "1m", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("123s 456s 789s 6688p", "6p", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test three suited terminal chows ====");
    test_points("123p 789p 12378m55s", "9m", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test pure shifted chows ====");
    test_points("123p 234p 345p 2234s", "2s", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("123s 345s 567s 2345s", "2s", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("123m 345567m77s88p", "8p", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test all five ====");
    test_points("456p 456s 456m 4555m", "6m", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("345m 456m 555p 55m55s", "5s", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test triple pung ====");
    test_points("333p 333m 44m23333s", "4s", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("111m 111p 111s 99s99p", "9p", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test three concealed pungs ====");
    test_points("999m11s99pEEECCC", "1s", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("123s 4445777888s", "5s", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test lesser honors and knitted tiles ====");
    test_points("258m147s36pESWFP", "C", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("147m39s258pEWCFP", "N", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("147m258s369pSWNC", "F", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test knitted straight ====");
    test_points("23358m14447s369p", "4s", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("147m3669s122358p", "6s", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("369m258s147pEEPP", "E", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test upper four ====");
    test_points("789s 678p 777p 78m99s", "9m", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("789m 789s 789p 77s78p", "9p", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("666s 666p 666m 7788p", "7p", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test lower four ====");
    test_points("123s 123m 123p 2333m", "1m", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("111s 222s 22m33344s", "4s", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test big three winds ====");
    test_points("EEE SSS WWW 99m99s", "9m", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("SSS WWW NNN 2345m", "5m", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("SSS WWW NNNCCFF", "C", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test mixed straight ====");
    test_points("123s 456p 789m23s88p", "1s", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("123m 456s 789p 77m45p", "6p", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test reversible tiles ====");
    test_points("123p 234p 345p 8899p", "8p", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("234p 234p 234p 1123p", "4p", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("345p 345p 456s 4555s", "6s", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("234p 456s 888p 88sPP", "8s", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("111p 222p 333p 4455p", "4p", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("222s 456s 4555888s", "6s", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("888p 999p 999s 88sPP", "P", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("1122334455889p", "9p", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test mixed triple chow ====");
    test_points("345s 345p 345m 4456m", "4m", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("678m 678s 678p 99s67p", "8p", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test mixed shifted pungs ====");
    test_points("222p 333s 444m 22m33p", "3p", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("777m 888s 999p 99m78p", "9p", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test chicken hand ====");
    test_points("123p 444s 789m 34pCC", "2p", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test last tile draw ====");
    puts("==== test last tile claim ====");
    puts("==== test out with replacement tile ====");
    puts("==== test robbing the kong ====");

    puts("==== test two concealed kongs ====");
    test_points("[1111s]EEEE SSS 789m 8m", "8m", WIN_TYPE_SELF_DRAWN, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test all pungs ====");
    test_points("888m 888p 888sEEPP", "P", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test half flush ====");
    test_points("123m 234m 34578mCC", "9m", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test mixed shifted chows ====");
    test_points("123s 234m 345p 55m45s", "6s", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test all types ====");
    test_points("123m 456p 789sNNFF", "F", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test melded hand ====");
    test_points("2222m 456p 678p 888s 6m", "6m", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test two dragons pungs ====");
    test_points("CCC FFF 12378m88s", "9m", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test outside hand ====");
    test_points("123m 123m 111p 11s11m", "1s", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("789p 789m 7788999s", "9s", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("123m 123m 789m 78mCC", "9m", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);
    test_points("123m 123p 999m78pEE", "9p", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test fully concealed hand ====");
    test_points("234m4468s345678p", "7s", WIN_TYPE_SELF_DRAWN, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test two melded kongs ====");
    test_points("4444p 4444m CCC 1133m", "1m", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test last tile ====");
    puts("==== test dragon pung ====");
    puts("==== test prevalent wind ====");
    puts("==== test seat wind ====");
    puts("==== test concealed hand ====");
    test_points("234567m66s34567p", "8p", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test all chows ====");
    test_points("234m456789s3477p", "5p", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test tile hog ====");
    test_points("789p 789s 789m 77m33p", "7m", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test double pung ====");
    test_points("222m 555m 555s 4488p", "8p", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test two concealed pungs ====");
    test_points("[9999p]1255789m999s", "3m", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

    puts("==== test concealed kong ====");
    puts("==== test all simples ====");
    test_points("234m456777s3444p", "5p", WIN_TYPE_DISCARD, WIND_TYPE::EAST, WIND_TYPE::EAST);

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
    test_points("1111p 456s 2789s456p", "2s", WIN_TYPE_SELF_DRAWN, WIND_TYPE::EAST, WIND_TYPE::EAST);

    return 0;
}

#include "wait_and_win.cpp"
#include "points_calculator.cpp"
