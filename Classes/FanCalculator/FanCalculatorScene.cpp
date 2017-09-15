#include "FanCalculatorScene.h"
#include "../mahjong-algorithm/stringify.h"
#include "../mahjong-algorithm/fan_calculator.h"
#include "../widget/TilePickWidget.h"
#include "../widget/ExtraInfoWidget.h"
#include "../widget/AlertView.h"
#include "../widget/TilesKeyboard.h"
#include "../FanTable/FanDefinition.h"

USING_NS_CC;

bool FanCalculatorScene::init() {
    if (UNLIKELY(!BaseScene::initWithTitle("国标麻将算番器"))) {
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

    // 直接输入
    ui::Button *button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
    button->setScale9Enabled(true);
    button->setContentSize(Size(55.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText("直接输入");
    extraInfo->addChild(button);
    button->setPosition(Vec2(visibleSize.width - 40, 100.0f));
    button->addClickEventListener([this](Ref *) { showInputAlert(nullptr); });

    // 番算按钮
    button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
    button->setScale9Enabled(true);
    button->setContentSize(Size(55.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText("算  番");
    extraInfo->addChild(button);
    button->setPosition(Vec2(visibleSize.width - 40, 10.0f));
    button->addClickEventListener([this](Ref *) { calculate(); });

    // 番种显示的Node
    Size areaSize(visibleSize.width, visibleSize.height - 35 - widgetSize.height - 5 - extraSize.height - 10);
    _fanAreaNode = Node::create();
    _fanAreaNode->setContentSize(areaSize);
    _fanAreaNode->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    _fanAreaNode->setIgnoreAnchorPointForPosition(false);
    this->addChild(_fanAreaNode);
    _fanAreaNode->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + areaSize.height * 0.5f + 5));

    tilePicker->setFixedPacksChangedCallback([tilePicker, extraInfo]() {
        extraInfo->refreshByKong(tilePicker->isFixedPacksContainsKong());
    });

    tilePicker->setWinTileChangedCallback([tilePicker, extraInfo]() {
        ExtraInfoWidget::RefreshByWinTile rt;
        rt.getWinTile = std::bind(&TilePickWidget::getServingTile, tilePicker);
        rt.isStandingTilesContainsServingTile = std::bind(&TilePickWidget::isStandingTilesContainsServingTile, tilePicker);
        rt.countServingTileInFixedPacks = std::bind(&TilePickWidget::countServingTileInFixedPacks, tilePicker);
        rt.isFixedPacksContainsKong = std::bind(&TilePickWidget::isFixedPacksContainsKong, tilePicker);
        extraInfo->refreshByWinTile(rt);
    });

    return true;
}

static cocos2d::Node *createFanResultNode(const mahjong::fan_table_t &fan_table, int fontSize, float resultAreaWidth) {
    // 有n个番种，每行排2个
    ptrdiff_t n = mahjong::FAN_TABLE_SIZE - std::count(std::begin(fan_table), std::end(fan_table), 0);
    ptrdiff_t rows = (n >> 1) + (n & 1);  // 需要这么多行

    // 排列
    Node *node = Node::create();
    const int lineHeight = fontSize + 2;
    ptrdiff_t resultAreaHeight = lineHeight * rows;  // 每行间隔2像素
    resultAreaHeight += (5 + lineHeight) + 20;  // 总计+提示
    node->setContentSize(Size(resultAreaWidth, static_cast<float>(resultAreaHeight)));

    uint16_t fan = 0;
    for (int i = 0, j = 0; i < n; ++i) {
        while (fan_table[++j] == 0) continue;

        uint16_t f = mahjong::fan_value_table[j];
        uint16_t n = fan_table[j];
        fan += f * n;
        std::string str = (n == 1) ? Common::format<128>("%s %hu番\n", mahjong::fan_name[j], f)
            : Common::format<128>("%s %hu番x%hu\n", mahjong::fan_name[j], f, fan_table[j]);

        // 创建label，每行排2个
        Label *label = Label::createWithSystemFont(str, "Arial", static_cast<float>(fontSize));
        label->setColor(Color3B(0x60, 0x60, 0x60));
        node->addChild(label);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        div_t ret = div(i, 2);
        label->setPosition(Vec2(ret.rem == 0 ? 0.0f : resultAreaWidth * 0.5f, static_cast<float>(resultAreaHeight - lineHeight * (ret.quot + 1))));

        // 创建与label同位置的button
        ui::Button *button = ui::Button::create();
        button->setScale9Enabled(true);
        button->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        button->setPosition(label->getPosition());
        button->setContentSize(label->getContentSize());
        node->addChild(button);
        button->addClickEventListener([j](Ref *) {
            Director::getInstance()->pushScene(FanDefinitionScene::create(j));
        });
    }

    Label *label = Label::createWithSystemFont(Common::format<64>("总计：%hu番", fan), "Arial", static_cast<float>(fontSize));
    label->setColor(Color3B::BLACK);
    node->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(0.0f, lineHeight * 0.5f + 20));

    label = Label::createWithSystemFont("点击番种名可查看番种介绍。", "Arial", 10);
    label->setColor(Color3B(51, 204, 255));
    node->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(0.0f, 5));

    return node;
}

void FanCalculatorScene::showInputAlert(const char *prevInput) {
    Size visibleSize = Director::getInstance()->getVisibleSize();
    const float width = visibleSize.width * 0.8f - 10;

    Node *rootNode = Node::create();

    Label *label = Label::createWithSystemFont("使用说明：\n"
        "1." INPUT_GUIDE_STRING_1 "\n"
        "2." INPUT_GUIDE_STRING_2 "\n"
        "3." INPUT_GUIDE_STRING_3 "\n"
        "输入范例1：[EEEE][CCCC][FFFF][PPPP]NN\n"
        "输入范例2：1112345678999s9s\n"
        "输入范例3：WWWW 444s 45m678pFF6m\n", "Arial", 10);
    label->setColor(Color3B::BLACK);
    label->setDimensions(width, 0);
    rootNode->addChild(label);

    // 输入手牌
    ui::EditBox *editBox = ui::EditBox::create(Size(width - 10, 20.0f), ui::Scale9Sprite::create("source_material/btn_square_normal.png"));
    editBox->setInputFlag(ui::EditBox::InputFlag::SENSITIVE);
    editBox->setInputMode(ui::EditBox::InputMode::SINGLE_LINE);
    editBox->setReturnType(ui::EditBox::KeyboardReturnType::DONE);
    editBox->setFontColor(Color4B::BLACK);
    editBox->setFontSize(12);
    editBox->setPlaceholderFontColor(Color4B::GRAY);
    editBox->setPlaceHolder("输入手牌");
    if (prevInput != nullptr) {
        editBox->setText(prevInput);
    }
    TilesKeyboard::hookEditBox(editBox);
    rootNode->addChild(editBox);

    const Size &labelSize = label->getContentSize();
    rootNode->setContentSize(Size(width, labelSize.height + 30));
    editBox->setPosition(Vec2(width * 0.5f, 10));
    label->setPosition(Vec2(width * 0.5f, labelSize.height * 0.5f + 30));

    AlertView::showWithNode("直接输入", rootNode, [this, editBox]() {
        const char *input = editBox->getText();
        const char *errorStr = TilesKeyboard::parseInput(input,
            std::bind(&TilePickWidget::setData, _tilePicker, std::placeholders::_1, std::placeholders::_2));
        if (errorStr != nullptr) {
            const std::string str = input;
            AlertView::showWithMessage("直接输入牌", errorStr, 12, [this, str]() {
                showInputAlert(str.c_str());
            }, nullptr);
        }
    }, nullptr);
}

void FanCalculatorScene::calculate() {
    _fanAreaNode->removeAllChildren();

    const Size &fanAreaSize = _fanAreaNode->getContentSize();
    Vec2 pos(fanAreaSize.width * 0.5f, fanAreaSize.height * 0.5f);

    int flowerCnt = _extraInfo->getFlowerCount();
    if (flowerCnt > 8) {
        AlertView::showWithMessage("算番", "花牌数的范围为0~8", 12, nullptr, nullptr);
        return;
    }

    mahjong::calculate_param_t param;
    _tilePicker->getData(&param.hand_tiles, &param.win_tile);
    if (param.win_tile == 0) {
        AlertView::showWithMessage("算番", "牌张数错误", 12, nullptr, nullptr);
        return;
    }

    std::sort(param.hand_tiles.standing_tiles, param.hand_tiles.standing_tiles + param.hand_tiles.tile_count);

    param.flower_count = flowerCnt;
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
    int fan = calculate_fan(&param, fan_table);

    if (fan == ERROR_NOT_WIN) {
        Label *errorLabel = Label::createWithSystemFont("诈和", "Arial", 14);
        errorLabel->setColor(Color3B::BLACK);
        _fanAreaNode->addChild(errorLabel);
        errorLabel->setPosition(pos);
        return;
    }
    if (fan == ERROR_WRONG_TILES_COUNT) {
        AlertView::showWithMessage("算番", "牌张数错误", 12, nullptr, nullptr);
        return;
    }
    if (fan == ERROR_TILE_COUNT_GREATER_THAN_4) {
        AlertView::showWithMessage("算番", "同一种牌最多只能使用4枚", 12, nullptr, nullptr);
        return;
    }

    Node *innerNode = createFanResultNode(fan_table, 14, fanAreaSize.width - 10);

    // 超出高度就使用ScrollView
    if (innerNode->getContentSize().height <= fanAreaSize.height) {
        _fanAreaNode->addChild(innerNode);
        innerNode->setAnchorPoint(Vec2(0.5f, 0.5f));
        innerNode->setPosition(pos);
    }
    else {
        ui::ScrollView *scrollView = ui::ScrollView::create();
        scrollView->setDirection(ui::ScrollView::Direction::VERTICAL);
        scrollView->setScrollBarPositionFromCorner(Vec2(2, 2));
        scrollView->setScrollBarWidth(4);
        scrollView->setScrollBarOpacity(0x99);
        scrollView->setContentSize(Size(fanAreaSize.width - 10, fanAreaSize.height));
        scrollView->setInnerContainerSize(innerNode->getContentSize());
        scrollView->addChild(innerNode);

        _fanAreaNode->addChild(scrollView);
        scrollView->setAnchorPoint(Vec2(0.5f, 0.5f));
        scrollView->setPosition(pos);
    }
}
