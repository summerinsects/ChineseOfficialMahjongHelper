#ifndef _LITTLE_FAN_H_
#define _LITTLE_FAN_H_

// 只存在一个的番14个：箭刻、圈风刻、门风刻、门前清、平和、双暗刻、暗杠、断幺、明杠、无字、边张、嵌张、单钓将、自摸
static const mahjong::fan_t uniqueFanTable[] = {
    mahjong::DRAGON_PUNG, mahjong::PREVALENT_WIND, mahjong::SEAT_WIND, mahjong::CONCEALED_HAND, mahjong::ALL_CHOWS,
    mahjong::TWO_CONCEALED_PUNGS, mahjong::CONCEALED_KONG, mahjong::ALL_SIMPLES,
    mahjong::MELDED_KONG, mahjong::NO_HONORS, mahjong::EDGE_WAIT, mahjong::CLOSED_WAIT, mahjong::SINGLE_WAIT, mahjong::SELF_DRAWN
};

// 可复计的番9个：四归一、双同刻、一般高、喜相逢、连六、老少副、幺九刻、缺一门、花牌
static const mahjong::fan_t multipleFanTable[] = {
    mahjong::TILE_HOG, mahjong::DOUBLE_PUNG,
    mahjong::PURE_DOUBLE_CHOW, mahjong::MIXED_DOUBLE_CHOW, mahjong::SHORT_STRAIGHT, mahjong::TWO_TERMINAL_CHOWS,
    mahjong::PUNG_OF_TERMINALS_OR_HONORS, mahjong::ONE_VOIDED_SUIT, mahjong::FLOWER_TILES
};

#endif
