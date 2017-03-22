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
    _tilePicker = TilePickWidget::create();
    const Size &widgetSize = _tilePicker->getContentSize();
    this->addChild(_tilePicker);
    _tilePicker->setFixedPacksChangedCallback(std::bind(&FanCalculatorScene::onFixedPacksChanged, this));
    _tilePicker->setWinTileChangedCallback(std::bind(&FanCalculatorScene::onWinTileChanged, this));

    // 根据情况缩放
    float y = origin.y + visibleSize.height - 10;
    if (widgetSize.width - 4 > visibleSize.width) {
        float scale = (visibleSize.width - 4) / widgetSize.width;
        _tilePicker->setScale(scale);
        y -= widgetSize.height * scale;
        _tilePicker->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, y - 25 + widgetSize.height * scale * 0.5f));
    }
    else {
        y -= widgetSize.height;
        _tilePicker->setPosition(Vec2(origin.x + widgetSize.width * 0.5f, y - 25 + widgetSize.height * 0.5f));
    }

    // 其他信息的相关控件
    _extraInfo = ExtraInfoWidget::create();
    this->addChild(_extraInfo);
    _extraInfo->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, y - 85));
    _extraInfo->setParseCallback(std::bind(&TilePickWidget::setData, _tilePicker, std::placeholders::_1, std::placeholders::_2));

    // 番种显示的Node
    _fanAreaNode = Node::create();
    _fanAreaNode->setContentSize(Size(visibleSize.width, y - 145));
    _fanAreaNode->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    this->addChild(_fanAreaNode);
    _fanAreaNode->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + y * 0.5f - 70));

    // 番算按钮
    ui::Button *button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
    button->setScale9Enabled(true);
    button->setContentSize(Size(35.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleColor(Color3B::BLACK);
    button->setTitleText("算番");
    _extraInfo->addChild(button);
    button->setPosition(Vec2(visibleSize.width - 30, 15.0f));
    button->addClickEventListener([this](Ref *) { calculate(); });

    return true;
}

void FanCalculatorScene::onFixedPacksChanged() {
    _extraInfo->refreshByKong(_tilePicker->isFixedPacksContainsKong());
}

void FanCalculatorScene::onWinTileChanged() {
    ExtraInfoWidget::RefreshByWinTile rt;
    rt.getWinTile = std::bind(&TilePickWidget::getServingTile, _tilePicker);
    rt.isStandingTilesContainsServingTile = std::bind(&TilePickWidget::isStandingTilesContainsServingTile, _tilePicker);
    rt.countServingTileInFixedPacks = std::bind(&TilePickWidget::countServingTileInFixedPacks, _tilePicker);
    _extraInfo->refreshByWinTile(rt);
}

#define FONT_SIZE 14

void FanCalculatorScene::calculate() {
    _fanAreaNode->removeAllChildren();

    Size visibleSize = Director::getInstance()->getVisibleSize();
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
        AlertView::showWithMessage("算番", "缺少和牌张", nullptr, nullptr);
        return;
    }

    mahjong::sort_tiles(hand_tiles.standing_tiles, hand_tiles.tile_count);

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
        Label *errorLabel = Label::createWithSystemFont("诈和", "Arial", FONT_SIZE);
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

    // 有n个番种，每行排2个
    long n = mahjong::FAN_TABLE_SIZE - std::count(std::begin(fan_table), std::end(fan_table), 0);
    long rows = (n >> 1) + (n & 1);  // 需要这么多行

    // 排列
    Node *innerNode = Node::create();
    float fanAreaHeight = (FONT_SIZE + 2) * (rows + 2);  // 每行间隔2像素，留空1行，另一行给“总计”用
    innerNode->setContentSize(Size(visibleSize.width, fanAreaHeight));

    for (int i = 0, j = 0; i < n; ++i) {
        while (fan_table[++j] == 0) continue;

        std::string str;
        if (fan_table[j] == 1) {
            str = StringUtils::format("%s %d番\n", mahjong::fan_name[j], mahjong::fan_value_table[j]);
        }
        else {
            str = StringUtils::format("%s %d番x%ld\n", mahjong::fan_name[j], mahjong::fan_value_table[j], fan_table[j]);
        }

        // 创建label，每行排2个
        Label *fanName = Label::createWithSystemFont(str, "Arial", FONT_SIZE);
        fanName->setColor(Color3B::BLACK);
        innerNode->addChild(fanName);
        fanName->setAnchorPoint(Vec2::ANCHOR_TOP_LEFT);
        div_t ret = div(i, 2);
        fanName->setPosition(Vec2(ret.rem == 0 ? 5.0f : visibleSize.width * 0.5f + 5.0f, (FONT_SIZE + 2) * (rows - ret.quot + 2)));
    }

    Label *fanTotal = Label::createWithSystemFont(StringUtils::format("总计：%d番", fan), "Arial", FONT_SIZE);
    fanTotal->setColor(Color3B::BLACK);
    innerNode->addChild(fanTotal);
    fanTotal->setAnchorPoint(Vec2::ANCHOR_TOP_LEFT);
    fanTotal->setPosition(Vec2(5.0f, FONT_SIZE + 2));

    // 超出高度就使用ScrollView
    if (fanAreaHeight <= fanAreaSize.height) {
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
