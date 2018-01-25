#include "FanCalculatorScene.h"
#include <algorithm>
#include <iterator>
#include "../mahjong-algorithm/stringify.h"
#include "../mahjong-algorithm/fan_calculator.h"
#include "../UICommon.h"
#include "../widget/TilePickWidget.h"
#include "../widget/ExtraInfoWidget.h"
#include "../widget/Toast.h"
#include "../FanTable/FanDefinitionScene.h"

USING_NS_CC;

static const Color3B C3B_GRAY = Color3B(96, 96, 96);
static const Color3B C3B_BLUE_THEME = Color3B(51, 204, 255);

bool FanCalculatorScene::init() {
    if (UNLIKELY(!BaseScene::initWithTitle(__UTF8("国标麻将算番器")))) {
        return false;
    }

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    // 选牌面板
    TilePickWidget *tilePicker = TilePickWidget::create(visibleSize.width - 10.0f);
    const Size &widgetSize = tilePicker->getContentSize();
    this->addChild(tilePicker);
    tilePicker->setPosition(Vec2(origin.x + visibleSize.width * 0.5f,
        origin.y + visibleSize.height - 35.0f - widgetSize.height * 0.5f));
    _tilePicker = tilePicker;

    // 其他信息的相关控件
    ExtraInfoWidget *extraInfo = ExtraInfoWidget::create(visibleSize.width - 10.0f, [this](Ref *) { calculate(); });
    const Size &extraSize = extraInfo->getContentSize();
    this->addChild(extraInfo);
    extraInfo->setPosition(Vec2(origin.x + visibleSize.width * 0.5f,
        origin.y + visibleSize.height - 35.0f - widgetSize.height - 5.0f - extraSize.height * 0.5f));
    _extraInfo = extraInfo;

    extraInfo->setInputCallback(std::bind(&TilePickWidget::setData, _tilePicker, std::placeholders::_1, std::placeholders::_2));

    // 番种显示的Node
    Size areaSize(visibleSize.width, visibleSize.height - 35.0f - widgetSize.height - 5.0f - extraSize.height - 10.0f);
    Node *node = Node::create();
    node->setContentSize(areaSize);
    node->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    node->setIgnoreAnchorPointForPosition(false);
    this->addChild(node);
    node->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + areaSize.height * 0.5f + 5.0f));
    _fanAreaNode = node;

    tilePicker->setFixedPacksChangedCallback([tilePicker, extraInfo]() {
        extraInfo->refreshByKong(tilePicker->isFixedPacksContainsKong());
    });

    tilePicker->setWinTileChangedCallback([tilePicker, extraInfo]() {
        extraInfo->refreshByWinTile(tilePicker->getServingTile(), !tilePicker->isStandingTilesContainsServingTile(),
            tilePicker->countServingTileInFixedPacks(), tilePicker->isFixedPacksContainsKong());
    });

    return true;
}

cocos2d::Node *createFanResultNode(const mahjong::fan_table_t &fan_table, int fontSize, float resultAreaWidth) {
    // 有n个番种，每行排2个
    ptrdiff_t n = mahjong::FAN_TABLE_SIZE - std::count(std::begin(fan_table), std::end(fan_table), 0);
    ptrdiff_t rows = (n >> 1) + (n & 1);  // 需要这么多行

    // 排列
    Node *node = Node::create();
    const int lineHeight = fontSize + 2;  // 每行间隔2像素
    ptrdiff_t resultAreaHeight = lineHeight * rows;
    resultAreaHeight += (5 + lineHeight) + 20;  // 总计+提示
    node->setContentSize(Size(resultAreaWidth, static_cast<float>(resultAreaHeight)));

    uint16_t fan = 0;
    for (int i = 0, j = 0; i < n; ++i) {
        while (fan_table[++j] == 0) continue;

        uint16_t f = mahjong::fan_value_table[j];
        uint16_t n = fan_table[j];
        fan += f * n;
        std::string str = (n == 1) ? Common::format(__UTF8("%s %hu番\n"), mahjong::fan_name[j], f)
            : Common::format(__UTF8("%s %hu番x%hu\n"), mahjong::fan_name[j], f, n);

        // 创建label，每行排2个
        Label *label = Label::createWithSystemFont(str, "Arial", static_cast<float>(fontSize));
        label->setColor(C3B_GRAY);
        node->addChild(label);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        div_t ret = div(i, 2);
        label->setPosition(Vec2(ret.rem == 0 ? 0.0f : resultAreaWidth * 0.5f, static_cast<float>(resultAreaHeight - lineHeight * (ret.quot + 1))));

        // 创建与label同位置的widget
        ui::Widget *widget = ui::Widget::create();
        widget->setTouchEnabled(true);
        widget->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        widget->setPosition(label->getPosition());
        widget->setContentSize(label->getContentSize());
        node->addChild(widget);
        widget->addClickEventListener([j](Ref *) {
            Director::getInstance()->pushScene(FanDefinitionScene::create(j));
        });
    }

    Label *label = Label::createWithSystemFont(Common::format(__UTF8("总计：%hu番"), fan), "Arial", static_cast<float>(fontSize));
    label->setColor(Color3B::BLACK);
    node->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(0.0f, lineHeight * 0.5f + 20.0f));

    label = Label::createWithSystemFont(__UTF8("点击番种名可查看番种介绍。"), "Arial", 10);
    label->setColor(C3B_BLUE_THEME);
    node->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(0.0f, 5.0f));

    return node;
}

void FanCalculatorScene::calculate() {
    _fanAreaNode->removeAllChildren();

    const Size &fanAreaSize = _fanAreaNode->getContentSize();
    Vec2 pos(fanAreaSize.width * 0.5f, fanAreaSize.height * 0.5f);

    int flowerCnt = _extraInfo->getFlowerCount();
    if (flowerCnt > 8) {
        Toast::makeText(this, __UTF8("花牌数的范围为0~8"), Toast::LENGTH_LONG)->show();
        return;
    }

    mahjong::calculate_param_t param;
    _tilePicker->getData(&param.hand_tiles, &param.win_tile);
    if (param.win_tile == 0) {
        Toast::makeText(this, __UTF8("牌张数错误"), Toast::LENGTH_LONG)->show();
        return;
    }

    std::sort(param.hand_tiles.standing_tiles, param.hand_tiles.standing_tiles + param.hand_tiles.tile_count);

    param.flower_count = static_cast<uint8_t>(flowerCnt);
    mahjong::fan_table_t fan_table = { 0 };

    // 获取绝张、杠开、抢杠、海底信息
    mahjong::win_flag_t win_flag = _extraInfo->getWinFlag();

    // 获取圈风门风
    mahjong::wind_t prevalent_wind = _extraInfo->getPrevalentWind();
    mahjong::wind_t seat_wind = _extraInfo->getSeatWind();

    // 算番
    param.win_flag = win_flag;
    param.prevalent_wind = prevalent_wind;
    param.seat_wind = seat_wind;
    int fan = calculate_fan(&param, &fan_table);

    if (fan == ERROR_NOT_WIN) {
        Toast::makeText(this, __UTF8("诈和"), Toast::LENGTH_LONG)->show();
        return;
    }
    if (fan == ERROR_WRONG_TILES_COUNT) {
        Toast::makeText(this, __UTF8("牌张数错误"), Toast::LENGTH_LONG)->show();
        return;
    }
    if (fan == ERROR_TILE_COUNT_GREATER_THAN_4) {
        Toast::makeText(this, __UTF8("同一种牌最多只能使用4枚"), Toast::LENGTH_LONG)->show();
        return;
    }

    Node *innerNode = createFanResultNode(fan_table, 14, fanAreaSize.width - 10.0f);

    // 超出高度就使用ScrollView
    if (innerNode->getContentSize().height <= fanAreaSize.height) {
        _fanAreaNode->addChild(innerNode);
        innerNode->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
        innerNode->setPosition(pos);
    }
    else {
        ui::ScrollView *scrollView = ui::ScrollView::create();
        scrollView->setDirection(ui::ScrollView::Direction::VERTICAL);
        scrollView->setScrollBarPositionFromCorner(Vec2(2.0f, 2.0f));
        scrollView->setScrollBarWidth(4.0f);
        scrollView->setScrollBarOpacity(0x99);
        scrollView->setContentSize(Size(fanAreaSize.width - 10.0f, fanAreaSize.height));
        scrollView->setInnerContainerSize(innerNode->getContentSize());
        scrollView->addChild(innerNode);

        _fanAreaNode->addChild(scrollView);
        scrollView->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
        scrollView->setPosition(pos);
    }
}
