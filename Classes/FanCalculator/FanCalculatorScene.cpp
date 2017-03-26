#include "FanCalculatorScene.h"
#include "../mahjong-algorithm/stringify.h"
#include "../mahjong-algorithm/fan_calculator.h"
#include "../widget/TilePickWidget.h"
#include "../widget/ExtraInfoWidget.h"
#include "../widget/AlertView.h"

USING_NS_CC;

Scene *FanCalculatorScene::createScene() {
    auto scene = Scene::create();
    auto layer = FanCalculatorScene::create();
    scene->addChild(layer);
    return scene;
}

bool FanCalculatorScene::init() {
    if (UNLIKELY(!BaseLayer::initWithTitle("国标麻将算番器"))) {
        return false;
    }

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    // 选牌面板
    TilePickWidget *tilePicker = TilePickWidget::create();
    const Size &widgetSize = tilePicker->getContentSize();
    this->addChild(tilePicker);
    tilePicker->setPosition(Vec2(origin.x + visibleSize.width * 0.5f,
        origin.y + visibleSize.height - 35 - widgetSize.height * 0.5f));
    _tilePicker = tilePicker;

    // 其他信息的相关控件
    ExtraInfoWidget *extraInfo = ExtraInfoWidget::create();
    const Size &extraSize = extraInfo->getContentSize();
    this->addChild(extraInfo);
    extraInfo->setPosition(Vec2(origin.x + visibleSize.width * 0.5f,
        origin.y + visibleSize.height - 35 - widgetSize.height - 5 - extraSize.height * 0.5f));
    _extraInfo = extraInfo;

    // 番种显示的Node
    Size areaSize(visibleSize.width, visibleSize.height - 35 - widgetSize.height - 5 - extraSize.height - 10);
    _fanAreaNode = Node::create();
    _fanAreaNode->setContentSize(areaSize);
    _fanAreaNode->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    _fanAreaNode->setIgnoreAnchorPointForPosition(false);
    this->addChild(_fanAreaNode);
    _fanAreaNode->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + areaSize.height * 0.5f + 5));

    // 番算按钮
    ui::Button *button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
    button->setScale9Enabled(true);
    button->setContentSize(Size(35.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText("算番");
    _extraInfo->addChild(button);
    button->setPosition(Vec2(visibleSize.width - 30, 10.0f));
    button->addClickEventListener([this](Ref *) { calculate(); });

    tilePicker->setFixedPacksChangedCallback([tilePicker, extraInfo]() {
        extraInfo->refreshByKong(tilePicker->isFixedPacksContainsKong());
    });

    tilePicker->setWinTileChangedCallback([tilePicker, extraInfo]() {
        ExtraInfoWidget::RefreshByWinTile rt;
        rt.getWinTile = std::bind(&TilePickWidget::getServingTile, tilePicker);
        rt.isStandingTilesContainsServingTile = std::bind(&TilePickWidget::isStandingTilesContainsServingTile, tilePicker);
        rt.countServingTileInFixedPacks = std::bind(&TilePickWidget::countServingTileInFixedPacks, tilePicker);
        extraInfo->refreshByWinTile(rt);
    });
    _extraInfo->setParseCallback(std::bind(&TilePickWidget::setData, _tilePicker, std::placeholders::_1, std::placeholders::_2));

    return true;
}

cocos2d::Node *createFanResultNode(const long (&fan_table)[mahjong::FAN_TABLE_SIZE], int fontSize, float resultAreaWidth) {
    // 有n个番种，每行排2个
    long n = mahjong::FAN_TABLE_SIZE - std::count(std::begin(fan_table), std::end(fan_table), 0);
    long rows = (n >> 1) + (n & 1);  // 需要这么多行

    // 排列
    Node *node = Node::create();
    const int lineHeight = fontSize + 2;
    long resultAreaHeight = lineHeight * rows;  // 每行间隔2像素
    resultAreaHeight += (3 + lineHeight);  // 总计
    node->setContentSize(Size(resultAreaWidth, resultAreaHeight));

    long fan = 0;
    for (int i = 0, j = 0; i < n; ++i) {
        while (fan_table[++j] == 0) continue;

        int f = mahjong::fan_value_table[j];
        long n = fan_table[j];
        fan += f * n;
        std::string str = (n == 1) ? StringUtils::format("%s %d番\n", mahjong::fan_name[j], f)
            : StringUtils::format("%s %d番x%ld\n", mahjong::fan_name[j], f, fan_table[j]);

        // 创建label，每行排2个
        Label *fanName = Label::createWithSystemFont(str, "Arial", fontSize);
        fanName->setColor(Color3B(0x60, 0x60, 0x60));
        node->addChild(fanName);
        fanName->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        div_t ret = div(i, 2);
        fanName->setPosition(Vec2(ret.rem == 0 ? 10.0f : resultAreaWidth * 0.5f, resultAreaHeight - lineHeight * (ret.quot + 1)));
    }

    Label *fanTotal = Label::createWithSystemFont(StringUtils::format("总计：%ld番", fan), "Arial", fontSize);
    fanTotal->setColor(Color3B::BLACK);
    node->addChild(fanTotal);
    fanTotal->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    fanTotal->setPosition(Vec2(10.0f, lineHeight * 0.5f));

    return node;
}

void FanCalculatorScene::calculate() {
    _fanAreaNode->removeAllChildren();

    const Size &fanAreaSize = _fanAreaNode->getContentSize();
    Vec2 pos(fanAreaSize.width * 0.5f, fanAreaSize.height * 0.5f);

    int flowerCnt = _extraInfo->getFlowerCount();
    if (flowerCnt > 8) {
        AlertView::showWithMessage("算番", "花牌数的范围为0~8", nullptr, nullptr);
        return;
    }

    mahjong::hand_tiles_t hand_tiles;
    mahjong::tile_t win_tile;
    _tilePicker->getData(&hand_tiles, &win_tile);
    if (win_tile == 0) {
        AlertView::showWithMessage("算番", "牌张数错误", nullptr, nullptr);
        return;
    }

    std::sort(hand_tiles.standing_tiles, hand_tiles.standing_tiles + hand_tiles.tile_count);

    long fan_table[mahjong::FAN_TABLE_SIZE] = { 0 };

    // 获取绝张、杠开、抢杠、海底信息
    mahjong::win_flag_t win_flag = _extraInfo->getWinFlag();

    // 获取圈风门风
    mahjong::wind_t prevalent_wind = _extraInfo->getPrevalentWind();
    mahjong::wind_t seat_wind = _extraInfo->getSeatWind();

    // 算番
    mahjong::extra_condition_t ext_cond;
    ext_cond.win_flag = win_flag;
    ext_cond.prevalent_wind = prevalent_wind;
    ext_cond.seat_wind = seat_wind;
    int fan = calculate_fan(&hand_tiles, win_tile, &ext_cond, fan_table);

    if (fan == ERROR_NOT_WIN) {
        Label *errorLabel = Label::createWithSystemFont("诈和", "Arial", 14);
        errorLabel->setColor(Color3B::BLACK);
        _fanAreaNode->addChild(errorLabel);
        errorLabel->setPosition(pos);
        return;
    }
    if (fan == ERROR_WRONG_TILES_COUNT) {
        AlertView::showWithMessage("算番", "牌张数错误", nullptr, nullptr);
        return;
    }
    if (fan == ERROR_TILE_COUNT_GREATER_THAN_4) {
        AlertView::showWithMessage("算番", "同一种牌最多只能使用4枚", nullptr, nullptr);
        return;
    }

    // 加花牌
    fan += flowerCnt;
    fan_table[mahjong::FLOWER_TILES] = flowerCnt;

    Node *innerNode = createFanResultNode(fan_table, 14, fanAreaSize.width);

    // 超出高度就使用ScrollView
    if (innerNode->getContentSize().height <= fanAreaSize.height) {
        _fanAreaNode->addChild(innerNode);
        innerNode->setAnchorPoint(Vec2(0.5f, 0.5f));
        innerNode->setPosition(pos);
    }
    else {
        ui::ScrollView *scrollView = ui::ScrollView::create();
        scrollView->setDirection(ui::ScrollView::Direction::VERTICAL);
        scrollView->setScrollBarPositionFromCorner(Vec2(10, 10));
        scrollView->setContentSize(fanAreaSize);
        scrollView->setInnerContainerSize(innerNode->getContentSize());
        scrollView->addChild(innerNode);

        _fanAreaNode->addChild(scrollView);
        scrollView->setAnchorPoint(Vec2(0.5f, 0.5f));
        scrollView->setPosition(pos);
    }
}
