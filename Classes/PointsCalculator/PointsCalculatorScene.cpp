#include "PointsCalculatorScene.h"
#include "../mahjong-algorithm/points_calculator.h"

#include "../widget/TilePickWidget.h"
#include "../widget/HandTilesWidget.h"
#include "../widget/AlertLayer.h"

USING_NS_CC;

Scene *PointsCalculatorScene::createScene() {
    auto scene = Scene::create();
    auto layer = PointsCalculatorScene::create();
    scene->addChild(layer);
    return scene;
}

bool PointsCalculatorScene::init() {
    if (!BaseLayer::initWithTitle("国标麻将算番器")) {
        return false;
    }

    Color3B textColor;
    const char *normalImage, *selectedImage;
    if (UserDefault::getInstance()->getBoolForKey("night_mode")) {
        textColor = Color3B::WHITE;
        normalImage = "source_material/btn_square_normal.png";
        selectedImage = "source_material/btn_square_highlighted.png";
    }
    else {
        textColor = Color3B::BLACK;
        normalImage = "source_material/btn_square_highlighted.png";
        selectedImage = "source_material/btn_square_selected.png";
    }

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    // 选牌面板
    _tilePicker = TilePickWidget::create();
    const Size &widgetSize = _tilePicker->getContentSize();
    this->addChild(_tilePicker);
    _tilePicker->setFixedSetsChangedCallback(std::bind(&PointsCalculatorScene::onFixedSetsChanged, this));
    _tilePicker->setWinTileChangedCallback(std::bind(&PointsCalculatorScene::onWinTileChanged, this));

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
    ui::Widget *infoWidget = ui::Widget::create();
    infoWidget->setContentSize(Size(visibleSize.width, 120));
    this->addChild(infoWidget);
    infoWidget->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, y - 85));

    // 番种显示的Node
    _pointsAreaNode = Node::create();
    _pointsAreaNode->setContentSize(Size(visibleSize.width, y - 145));
    _pointsAreaNode->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    this->addChild(_pointsAreaNode);
    _pointsAreaNode->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + y * 0.5f - 70));

    // 点和与自摸互斥
    _winTypeGroup = ui::RadioButtonGroup::create();
    infoWidget->addChild(_winTypeGroup);
    _winTypeGroup->addEventListener(std::bind(&PointsCalculatorScene::onWinTypeGroup, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    // 点和
    const float gapX = 65;
    ui::RadioButton *radioButton = ui::RadioButton::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
    infoWidget->addChild(radioButton);
    radioButton->setZoomScale(0.0f);
    radioButton->ignoreContentAdaptWithSize(false);
    radioButton->setContentSize(Size(20.0f, 20.0f));
    radioButton->setPosition(Vec2(20.0f, 105.0f));
    _winTypeGroup->addRadioButton(radioButton);

    Label *label = Label::createWithSystemFont("点和", "Arial", 12);
    label->setColor(textColor);
    infoWidget->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(35.0f, 105.0f));

    // 自摸
    radioButton = ui::RadioButton::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
    infoWidget->addChild(radioButton);
    radioButton->setZoomScale(0.0f);
    radioButton->ignoreContentAdaptWithSize(false);
    radioButton->setContentSize(Size(20.0f, 20.0f));
    radioButton->setPosition(Vec2(20.0f + gapX, 105.0f));
    _winTypeGroup->addRadioButton(radioButton);

    label = Label::createWithSystemFont("自摸", "Arial", 12);
    label->setColor(textColor);
    infoWidget->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(35.0f + gapX, 105.0f));

    // 绝张
    _fourthTileBox = ui::CheckBox::create("source_material/btn_square_normal.png", "", "source_material/btn_square_highlighted.png", "source_material/btn_square_disabled.png", "source_material/btn_square_disabled.png");
    infoWidget->addChild(_fourthTileBox);
    _fourthTileBox->setZoomScale(0.0f);
    _fourthTileBox->ignoreContentAdaptWithSize(false);
    _fourthTileBox->setContentSize(Size(20.0f, 20.0f));
    _fourthTileBox->setPosition(Vec2(20.0f + gapX * 2, 105.0f));
    _fourthTileBox->setEnabled(false);
    _fourthTileBox->addEventListener(std::bind(&PointsCalculatorScene::onFourthTileBox, this, std::placeholders::_1, std::placeholders::_2));

    label = Label::createWithSystemFont("绝张", "Arial", 12);
    label->setColor(textColor);
    infoWidget->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(35.0f + gapX * 2, 105.0f));

    // 杠开
    _replacementBox = ui::CheckBox::create("source_material/btn_square_normal.png", "", "source_material/btn_square_highlighted.png", "source_material/btn_square_disabled.png", "source_material/btn_square_disabled.png");
    infoWidget->addChild(_replacementBox);
    _replacementBox->setZoomScale(0.0f);
    _replacementBox->ignoreContentAdaptWithSize(false);
    _replacementBox->setContentSize(Size(20.0f, 20.0f));
    _replacementBox->setPosition(Vec2(20.0f, 75.0f));
    _replacementBox->setEnabled(false);

    label = Label::createWithSystemFont("杠开", "Arial", 12);
    label->setColor(textColor);
    infoWidget->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(35.0f, 75.0f));

    // 抢杠
    _robKongBox = ui::CheckBox::create("source_material/btn_square_normal.png", "", "source_material/btn_square_highlighted.png", "source_material/btn_square_disabled.png", "source_material/btn_square_disabled.png");
    infoWidget->addChild(_robKongBox);
    _robKongBox->setZoomScale(0.0f);
    _robKongBox->ignoreContentAdaptWithSize(false);
    _robKongBox->setContentSize(Size(20.0f, 20.0f));
    _robKongBox->setPosition(Vec2(20.0f + gapX, 75.0f));
    _robKongBox->setEnabled(false);
    _robKongBox->addEventListener(std::bind(&PointsCalculatorScene::onRobKongBox, this, std::placeholders::_1, std::placeholders::_2));

    label = Label::createWithSystemFont("抢杠", "Arial", 12);
    label->setColor(textColor);
    infoWidget->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(35.0f + gapX, 75.0f));

    // 海底
    _lastTileBox = ui::CheckBox::create("source_material/btn_square_normal.png", "", "source_material/btn_square_highlighted.png", "source_material/btn_square_disabled.png", "source_material/btn_square_disabled.png");
    infoWidget->addChild(_lastTileBox);
    _lastTileBox->setZoomScale(0.0f);
    _lastTileBox->ignoreContentAdaptWithSize(false);
    _lastTileBox->setContentSize(Size(20.0f, 20.0f));
    _lastTileBox->setPosition(Vec2(20.0f + gapX * 2, 75.0f));
    _lastTileBox->addEventListener(std::bind(&PointsCalculatorScene::onLastTileBox, this, std::placeholders::_1, std::placeholders::_2));

    label = Label::createWithSystemFont("海底", "Arial", 12);
    label->setColor(textColor);
    infoWidget->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(35.0f + gapX * 2, 75.0f));

    const char *windName[4] = { "东", "南", "西", "北" };

    // 圈风
    label = Label::createWithSystemFont("圈风", "Arial", 12);
    label->setColor(textColor);
    infoWidget->addChild(label);
    label->setPosition(Vec2(20.0f, 45.0f));

    _prevalentWindGroup = ui::RadioButtonGroup::create();
    infoWidget->addChild(_prevalentWindGroup);

    for (int i = 0; i < 4; ++i) {
        ui::RadioButton *button = ui::RadioButton::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
        button->setZoomScale(0.0f);
        button->ignoreContentAdaptWithSize(false);
        button->setContentSize(Size(20.0f, 20.0f));
        button->setPosition(Vec2(50.0f + i * 30, 45.0f));
        infoWidget->addChild(button);
        _prevalentWindGroup->addRadioButton(button);

        label = Label::createWithSystemFont(windName[i], "Arial", 12);
        label->setColor(Color3B::BLACK);
        button->addChild(label);
        label->setPosition(Vec2(10.0f, 10.0f));
    }

    // 门风
    label = Label::createWithSystemFont("门风", "Arial", 12);
    label->setColor(textColor);
    infoWidget->addChild(label);
    label->setPosition(Vec2(20.0f, 15.0f));

    _seatWindGroup = ui::RadioButtonGroup::create();
    infoWidget->addChild(_seatWindGroup);

    for (int i = 0; i < 4; ++i) {
        ui::RadioButton *button = ui::RadioButton::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
        button->setZoomScale(0.0f);
        button->ignoreContentAdaptWithSize(false);
        button->setContentSize(Size(20.0f, 20.0f));
        button->setPosition(Vec2(50.0f + i * 30, 15.0f));
        infoWidget->addChild(button);
        _seatWindGroup->addRadioButton(button);

        label = Label::createWithSystemFont(windName[i], "Arial", 12);
        label->setColor(Color3B::BLACK);
        button->addChild(label);
        label->setPosition(Vec2(10.0f, 10.0f));
    }

    // 直接输入
    label = Label::createWithSystemFont("选牌太麻烦？试试", "Arial", 12);
    label->setColor(textColor);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_RIGHT);
    infoWidget->addChild(label);
    label->setPosition(Vec2(visibleSize.width - 15, 100.0f));

    ui::Button *button = ui::Button::create(normalImage, selectedImage);
    button->setScale9Enabled(true);
    button->setContentSize(Size(55.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText("直接输入");
    button->setTitleColor(Color3B::BLACK);
    infoWidget->addChild(button);
    button->setPosition(Vec2(visibleSize.width - 40, 80.0f));
    button->addClickEventListener([this](Ref *) { showInputAlert(nullptr); });

    // 花牌数
    label = Label::createWithSystemFont("花牌数", "Arial", 12);
    label->setColor(textColor);
    infoWidget->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_RIGHT);
    label->setPosition(Vec2(visibleSize.width - 50, 45.0f));

    _editBox = ui::EditBox::create(Size(35.0f, 20.0f), ui::Scale9Sprite::create("source_material/btn_square_normal.png"));
    infoWidget->addChild(_editBox);
    _editBox->setInputFlag(ui::EditBox::InputFlag::SENSITIVE);
    _editBox->setInputMode(ui::EditBox::InputMode::NUMERIC);
    _editBox->setFontColor(Color4B::BLACK);
    _editBox->setFontSize(12);
    _editBox->setText("0");
    _editBox->setPosition(Vec2(visibleSize.width - 30, 45.0f));

    // 番算按钮
    button = ui::Button::create(normalImage, selectedImage);
    button->setScale9Enabled(true);
    button->setContentSize(Size(35.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText("算番");
    button->setTitleColor(Color3B::BLACK);
    infoWidget->addChild(button);
    button->setPosition(Vec2(visibleSize.width - 30, 15.0f));
    button->addClickEventListener([this](Ref *) { calculate(); });

    return true;
}

void PointsCalculatorScene::onWinTypeGroup(cocos2d::ui::RadioButton *radioButton, int index, cocos2d::ui::RadioButtonGroup::EventType event) {
    if (index == 0) {  // 点和
        // 绝张：可为绝张 && 抢杠没选中
        // 杠开：禁用
        // 抢杠：可为绝张 && 副露不包含和牌 && 绝张没选中 && 海底没选中
        // 海底：抢杠没选中
        _fourthTileBox->setEnabled(_maybeFourthTile && !_robKongBox->isSelected());
        _replacementBox->setEnabled(false);
        _robKongBox->setEnabled(_maybeFourthTile && _winTileCountInFixedSets == 0
            && !_fourthTileBox->isSelected() && !_lastTileBox->isSelected());
        _lastTileBox->setEnabled(!_robKongBox->isSelected());
    }
    else {  // 自摸
        // 绝张：可为绝张
        // 杠开：有杠
        // 抢杠：禁用
        // 海底：可用
        _fourthTileBox->setEnabled(_maybeFourthTile);
        _replacementBox->setEnabled(_hasKong);
        _robKongBox->setEnabled(false);
        _lastTileBox->setEnabled(true);
    }
}

void PointsCalculatorScene::onFourthTileBox(cocos2d::Ref *sender, cocos2d::ui::CheckBox::EventType event) {
    // 绝张与抢杠互斥
    if (event == ui::CheckBox::EventType::SELECTED) {
        // 抢杠：禁用
        _robKongBox->setEnabled(false);
    }
    else {
        // 一定是绝张，则不允许取消选中绝张
        if (_maybeFourthTile && _winTileCountInFixedSets == 3) {
            _fourthTileBox->setSelected(true);
        }
        else {
            // 抢杠：可为绝张 && 副露不包含和牌 && 点和 && 海底没选中
            _robKongBox->setEnabled(_maybeFourthTile && _winTileCountInFixedSets == 0
                && _winTypeGroup->getSelectedButtonIndex() == 0
                && !_lastTileBox->isSelected());
        }
    }
}

void PointsCalculatorScene::onRobKongBox(cocos2d::Ref *sender, cocos2d::ui::CheckBox::EventType event) {
    // 抢杠与绝张、海底互斥
    if (event == ui::CheckBox::EventType::SELECTED) {
        // 绝张：禁用
        // 海底：禁用
        _fourthTileBox->setEnabled(false);
        _lastTileBox->setEnabled(false);
    }
    else {
        // 绝张：可为绝张
        // 海底：可用
        _fourthTileBox->setEnabled(_maybeFourthTile);
        _lastTileBox->setEnabled(true);
    }
}

void PointsCalculatorScene::onLastTileBox(cocos2d::Ref *sender, cocos2d::ui::CheckBox::EventType event) {
    // 海底与抢杠互斥
    if (event == ui::CheckBox::EventType::SELECTED) {
        // 抢杠：禁用
        _robKongBox->setEnabled(false);
    }
    else {
        // 抢杠：可为绝张 && 副露不包含和牌 && 点和 && 绝张没选中
        _robKongBox->setEnabled(_maybeFourthTile && _winTileCountInFixedSets == 0
            && _winTypeGroup->getSelectedButtonIndex() == 0
            && !_fourthTileBox->isSelected());
    }
}

void PointsCalculatorScene::onFixedSetsChanged() {
    // 当副露不包含杠的时候，杠开是禁用状态
    _hasKong = _tilePicker->getHandTilesWidget()->isFixedSetsContainsKong();
    if (_winTypeGroup->getSelectedButtonIndex() == 1) {
        // 杠开：有杠
        _replacementBox->setEnabled(_hasKong);
    }
    else {
        // 杠开：禁用
        _replacementBox->setEnabled(false);
    }
}

void PointsCalculatorScene::onWinTileChanged() {
    _maybeFourthTile = false;
    _winTileCountInFixedSets = 0;
    mahjong::TILE winTile = _tilePicker->getHandTilesWidget()->getWinTile();
    if (winTile == 0) {  // 没有和牌张
        _fourthTileBox->setEnabled(false);
        _robKongBox->setEnabled(false);
        _lastTileBox->setEnabled(true);
        return;
    }

    // 立牌中不包含和牌张，则可能为绝张
    _maybeFourthTile = !_tilePicker->getHandTilesWidget()->isStandingTilesContainsWinTile();

    // 一定为绝张
    _winTileCountInFixedSets = _tilePicker->getHandTilesWidget()->countWinTileInFixedSets();
    if (_maybeFourthTile && _winTileCountInFixedSets == 3) {
        _fourthTileBox->setEnabled(true);
        _robKongBox->setEnabled(false);
        _fourthTileBox->setSelected(true);
        return;
    }

    // 绝张：可为绝张 && 抢杠没选中
    // 抢杠：可为绝张 && 副露不包含和牌 && 点和 && 绝张没选中 && 海底没选中
    // 海底：抢杠没选中
    _fourthTileBox->setEnabled(_maybeFourthTile && !_robKongBox->isSelected());
    _robKongBox->setEnabled(_maybeFourthTile && _winTileCountInFixedSets == 0
        && _winTypeGroup->getSelectedButtonIndex() == 0
        && !_lastTileBox->isSelected()
        && !_fourthTileBox->isSelected());
    _lastTileBox->setEnabled(!_robKongBox->isSelected());
}

void PointsCalculatorScene::showInputAlert(const char *prevInput) {
    Size visibleSize = Director::getInstance()->getVisibleSize();
    const float width = visibleSize.width * 0.8f;

    ui::Widget *rootWidget = ui::Widget::create();

    Label *label = Label::createWithSystemFont("使用说明：\n"
        "1.数牌：万=m 条=s 饼=p。后缀使用小写字母，同花色的数牌可合并用一个后缀。\n"
        "2.字牌：东南西北=ESWN，中发白=CFP。使用大写字母。\n"
        "3.每一组吃、碰、明杠之间用英文空格分隔，每一组暗杠用英文[]。\n"
        "4.和牌用英文逗号,与手牌分隔开。\n"
        "范例1：[EEEE][CCCC][FFFF][PPPP]N,N\n"
        "范例2：1112345678999s,9s\n"
        "范例3：WWWW 444s 45m678pFF,6m\n", "Arial", 10);
    label->setColor(Color3B::BLACK);
    label->setDimensions(width, 0);
    rootWidget->addChild(label);

    // 输入手牌
    ui::EditBox *editBox = ui::EditBox::create(Size(width - 10, 20.0f), ui::Scale9Sprite::create("source_material/btn_square_normal.png"));
    editBox->setInputFlag(ui::EditBox::InputFlag::SENSITIVE);
    editBox->setInputMode(ui::EditBox::InputMode::SINGLE_LINE);
    editBox->setFontColor(Color4B::BLACK);
    editBox->setFontSize(12);
    editBox->setPlaceholderFontColor(Color4B::GRAY);
    editBox->setPlaceHolder("输入手牌");
    if (prevInput != nullptr) {
        editBox->setText(prevInput);
    }

    rootWidget->addChild(editBox);

    const Size &labelSize = label->getContentSize();
    rootWidget->setContentSize(Size(width, labelSize.height + 30));
    editBox->setPosition(Vec2(width * 0.5f, 10));
    label->setPosition(Vec2(width * 0.5f, labelSize.height * 0.5f + 30));

    AlertLayer::showWithNode("直接输入", rootWidget, [this, editBox]() {
        parseInput(editBox->getText());
    }, nullptr);
}

void PointsCalculatorScene::parseInput(const char *input) {
    const char *errorStr = nullptr;
    const std::string str = input;

    do {
        if (str.empty()) {
            break;
        }

        if (strspn(input, "123456789mpsESWNCFP [],") != str.length()) {
            errorStr = "无法解析输入的文本";
            break;
        }

        std::string::const_iterator it = std::find(str.begin(), str.end(), ',');
        if (it == str.end()) {
            errorStr = "缺少和牌张";
            break;
        }

        if (std::any_of(it + 1, str.end(), [](char ch) { return ch == ','; })) {
            errorStr = "过多逗号";
            break;
        }

        std::string tilesString(str.begin(), it);
        std::string winString(it + 1, str.end());

        mahjong::HAND_TILES hand_tiles;
        mahjong::TILE win_tile;

        long ret = mahjong::string_to_tiles(tilesString.c_str(), &hand_tiles);
        if (ret != PARSE_NO_ERROR) {
            switch (ret) {
            case PARSE_ERROR_ILLEGAL_CHARACTER: errorStr = "无法解析的字符"; break;
            case PARSE_ERROR_NO_SUFFIX_AFTER_DIGIT: errorStr = "数字后面需有后缀"; break;
            case PARSE_ERROR_TOO_MANY_TILES_FOR_FIXED_SET: errorStr = "一组副露包含了过多的牌"; break;
            case PARSE_ERROR_CANNOT_MAKE_FIXED_SET: errorStr = "无法正确解析副露"; break;
            default: break;
            }
            break;
        }
        mahjong::sort_tiles(hand_tiles.standing_tiles, hand_tiles.tile_count);

        ret = mahjong::parse_tiles(winString.c_str(), &win_tile, 1);
        if (ret != 1) {
            switch (ret) {
            case PARSE_ERROR_ILLEGAL_CHARACTER: errorStr = "无法解析的字符"; break;
            case PARSE_ERROR_NO_SUFFIX_AFTER_DIGIT: errorStr = "数字后面需有后缀"; break;
            case PARSE_ERROR_TOO_MANY_TILES_FOR_FIXED_SET: errorStr = "一组副露包含了过多的牌"; break;
            case PARSE_ERROR_CANNOT_MAKE_FIXED_SET: errorStr = "无法正确解析副露"; break;
            default: break;
            }
            break;
        }

        ret = mahjong::check_calculator_input(&hand_tiles, win_tile);
        if (ret != 0) {
            switch (ret) {
            case ERROR_WRONG_TILES_COUNT: errorStr = "牌张数错误"; break;
            case ERROR_TILE_COUNT_GREATER_THAN_4: errorStr = "同一种牌最多只能使用4枚"; break;
            default: break;
            }
            break;
        }
        _tilePicker->setData(hand_tiles, win_tile);
    } while (0);

    if (errorStr != nullptr) {
        AlertLayer::showWithMessage("直接输入牌", errorStr, [this, str]() {
            showInputAlert(str.c_str());
        }, nullptr);
    }
}

#define FONT_SIZE 14

void PointsCalculatorScene::calculate() {
    _pointsAreaNode->removeAllChildren();

    Size visibleSize = Director::getInstance()->getVisibleSize();
    const Size &pointsAreaSize = _pointsAreaNode->getContentSize();
    Vec2 pos(pointsAreaSize.width * 0.5f, pointsAreaSize.height * 0.5f);

    int flowerCnt = atoi(_editBox->getText());
    if (flowerCnt > 8) {
        AlertLayer::showWithMessage("算番", "花牌数的范围为0~8", nullptr, nullptr);
        return;
    }

    // 获取和牌张
    mahjong::TILE win_tile = _tilePicker->getHandTilesWidget()->getWinTile();
    if (win_tile == 0) {
        AlertLayer::showWithMessage("算番", "缺少和牌张", nullptr, nullptr);
        return;
    }

    mahjong::HAND_TILES hand_tiles;

    // 获取副露
    const std::vector<mahjong::SET> &fixedSets = _tilePicker->getHandTilesWidget()->getFixedSets();
    hand_tiles.set_count = std::copy(fixedSets.begin(), fixedSets.end(), std::begin(hand_tiles.fixed_sets))
        - std::begin(hand_tiles.fixed_sets);

    // 获取立牌
    const std::vector<mahjong::TILE> &standingTiles = _tilePicker->getHandTilesWidget()->getStandingTiles();
    hand_tiles.tile_count = std::copy(standingTiles.begin(), standingTiles.end() - 1, std::begin(hand_tiles.standing_tiles))
        - std::begin(hand_tiles.standing_tiles);
    mahjong::sort_tiles(hand_tiles.standing_tiles, hand_tiles.tile_count);

    long points_table[mahjong::POINT_TYPE_COUNT] = { 0 };

    // 获取绝张、杠开、抢杠、海底信息
    mahjong::WIN_TYPE win_type = WIN_TYPE_DISCARD;
    if (_winTypeGroup->getSelectedButtonIndex() == 1) win_type |= WIN_TYPE_SELF_DRAWN;
    if (_fourthTileBox->isEnabled() && _fourthTileBox->isSelected()) win_type |= WIN_TYPE_4TH_TILE;
    if (_robKongBox->isEnabled() && _robKongBox->isSelected()) win_type |= WIN_TYPE_ABOUT_KONG;
    if (_replacementBox->isEnabled() && _replacementBox->isSelected()) win_type |= (WIN_TYPE_ABOUT_KONG | WIN_TYPE_SELF_DRAWN);
    if (_lastTileBox->isEnabled() && _lastTileBox->isSelected()) win_type |= WIN_TYPE_WALL_LAST;

    // 获取圈风门风
    mahjong::WIND_TYPE prevalent_wind = static_cast<mahjong::WIND_TYPE>(static_cast<int>(mahjong::WIND_TYPE::EAST) + _prevalentWindGroup->getSelectedButtonIndex());
    mahjong::WIND_TYPE seat_wind = static_cast<mahjong::WIND_TYPE>(static_cast<int>(mahjong::WIND_TYPE::EAST) + _seatWindGroup->getSelectedButtonIndex());

    // 算番
    mahjong::EXTRA_CONDITION ext_cond;
    ext_cond.win_type = win_type;
    ext_cond.prevalent_wind = prevalent_wind;
    ext_cond.seat_wind = seat_wind;
    int points = calculate_points(&hand_tiles, win_tile, &ext_cond, points_table);

    Color3B textColor = UserDefault::getInstance()->getBoolForKey("night_mode") ? Color3B::WHITE : Color3B::BLACK;
    if (points == ERROR_NOT_WIN) {
        Label *errorLabel = Label::createWithSystemFont("诈和", "Arial", FONT_SIZE);
        errorLabel->setColor(textColor);
        _pointsAreaNode->addChild(errorLabel);
        errorLabel->setPosition(pos);
        return;
    }
    if (points == ERROR_WRONG_TILES_COUNT) {
        AlertLayer::showWithMessage("算番", "牌张数错误", nullptr, nullptr);
        return;
    }
    if (points == ERROR_TILE_COUNT_GREATER_THAN_4) {
        AlertLayer::showWithMessage("算番", "同一种牌最多只能使用4枚", nullptr, nullptr);
        return;
    }

    // 加花牌
    points += flowerCnt;
    points_table[mahjong::FLOWER_TILES] = flowerCnt;

    // 有n个番种，每行排2个
    long n = mahjong::POINT_TYPE_COUNT - std::count(std::begin(points_table), std::end(points_table), 0);
    long rows = (n >> 1) + (n & 1);  // 需要这么多行

    // 排列
    Node *innerNode = Node::create();
    float pointsAreaHeight = (FONT_SIZE + 2) * (rows + 2);  // 每行间隔2像素，留空1行，另一行给“总计”用
    innerNode->setContentSize(Size(visibleSize.width, pointsAreaHeight));

    for (int i = 0, j = 0; i < n; ++i) {
        while (points_table[++j] == 0) continue;

        std::string str;
        if (points_table[j] == 1) {
            str = StringUtils::format("%s %d番\n", mahjong::points_name[j], mahjong::points_value_table[j]);
        }
        else {
            str = StringUtils::format("%s %d番x%ld\n", mahjong::points_name[j], mahjong::points_value_table[j], points_table[j]);
        }

        // 创建label，每行排2个
        Label *pointName = Label::createWithSystemFont(str, "Arial", FONT_SIZE);
        pointName->setColor(textColor);
        innerNode->addChild(pointName);
        pointName->setAnchorPoint(Vec2::ANCHOR_TOP_LEFT);
        div_t ret = div(i, 2);
        pointName->setPosition(Vec2(ret.rem == 0 ? 5.0f : visibleSize.width * 0.5f + 5.0f, (FONT_SIZE + 2) * (rows - ret.quot + 2)));
    }

    Label *pointTotal = Label::createWithSystemFont(StringUtils::format("总计：%d番", points), "Arial", FONT_SIZE);
    pointTotal->setColor(textColor);
    innerNode->addChild(pointTotal);
    pointTotal->setAnchorPoint(Vec2::ANCHOR_TOP_LEFT);
    pointTotal->setPosition(Vec2(5.0f, FONT_SIZE + 2));

    // 超出高度就使用ScrollView
    if (pointsAreaHeight <= pointsAreaSize.height) {
        _pointsAreaNode->addChild(innerNode);
        innerNode->setAnchorPoint(Vec2(0.5f, 0.5f));
        innerNode->setPosition(pos);
    }
    else {
        ui::ScrollView *scrollView = ui::ScrollView::create();
        scrollView->setDirection(ui::ScrollView::Direction::VERTICAL);
        scrollView->setScrollBarPositionFromCorner(Vec2(10, 10));
        scrollView->setContentSize(pointsAreaSize);
        scrollView->setInnerContainerSize(innerNode->getContentSize());
        scrollView->addChild(innerNode);

        _pointsAreaNode->addChild(scrollView);
        scrollView->setAnchorPoint(Vec2(0.5f, 0.5f));
        scrollView->setPosition(pos);
    }
}
