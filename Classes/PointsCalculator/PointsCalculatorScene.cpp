#include "PointsCalculatorScene.h"
#include "../mahjong-algorithm/points_calculator.h"

#include "../widget/TilePickWidget.h"
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
    _tilePicker->setFixedSetsChangedCallback(std::bind(&PointsCalculatorScene::onFixedSetsChanged, this, std::placeholders::_1));
    _tilePicker->setWinTileChangedCallback(std::bind(&PointsCalculatorScene::onWinTileChanged, this, std::placeholders::_1));

    // 根据情况缩放
    float y = origin.y + visibleSize.height - 10;
    if (widgetSize.width > visibleSize.width) {
        float scale = visibleSize.width / widgetSize.width;
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
    float gapX = (visibleSize.width - 8) * 0.25f;
    ui::RadioButton *radioButton = ui::RadioButton::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
    infoWidget->addChild(radioButton);
    radioButton->setZoomScale(0.0f);
    radioButton->ignoreContentAdaptWithSize(false);
    radioButton->setContentSize(Size(20.0f, 20.0f));
    radioButton->setPosition(Vec2(20.0f, 105.0f));
    _winTypeGroup->addRadioButton(radioButton);

    Label *byDiscardLabel = Label::createWithSystemFont("点和", "Arial", 12);
    byDiscardLabel->setColor(textColor);
    infoWidget->addChild(byDiscardLabel);
    byDiscardLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    byDiscardLabel->setPosition(Vec2(35.0f, 105.0f));

    // 自摸
    radioButton = ui::RadioButton::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
    infoWidget->addChild(radioButton);
    radioButton->setZoomScale(0.0f);
    radioButton->ignoreContentAdaptWithSize(false);
    radioButton->setContentSize(Size(20.0f, 20.0f));
    radioButton->setPosition(Vec2(20.0f + gapX, 105.0f));
    _winTypeGroup->addRadioButton(radioButton);

    Label *selfDrawnLabel = Label::createWithSystemFont("自摸", "Arial", 12);
    selfDrawnLabel->setColor(textColor);
    infoWidget->addChild(selfDrawnLabel);
    selfDrawnLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    selfDrawnLabel->setPosition(Vec2(35.0f + gapX, 105.0f));

    // 绝张
    _fourthTileBox = ui::CheckBox::create("source_material/btn_square_normal.png", "", "source_material/btn_square_highlighted.png", "source_material/btn_square_disabled.png", "source_material/btn_square_disabled.png");
    infoWidget->addChild(_fourthTileBox);
    _fourthTileBox->setZoomScale(0.0f);
    _fourthTileBox->ignoreContentAdaptWithSize(false);
    _fourthTileBox->setContentSize(Size(20.0f, 20.0f));
    _fourthTileBox->setPosition(Vec2(20.0f + gapX * 2, 105.0f));
    _fourthTileBox->setEnabled(false);
    _fourthTileBox->addEventListener(std::bind(&PointsCalculatorScene::onFourthTileBox, this, std::placeholders::_1, std::placeholders::_2));

    Label *fourthTileLabel = Label::createWithSystemFont("绝张", "Arial", 12);
    fourthTileLabel->setColor(textColor);
    infoWidget->addChild(fourthTileLabel);
    fourthTileLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    fourthTileLabel->setPosition(Vec2(35.0f + gapX * 2, 105.0f));

    // 杠开
    _replacementBox = ui::CheckBox::create("source_material/btn_square_normal.png", "", "source_material/btn_square_highlighted.png", "source_material/btn_square_disabled.png", "source_material/btn_square_disabled.png");
    infoWidget->addChild(_replacementBox);
    _replacementBox->setZoomScale(0.0f);
    _replacementBox->ignoreContentAdaptWithSize(false);
    _replacementBox->setContentSize(Size(20.0f, 20.0f));
    _replacementBox->setPosition(Vec2(20.0f, 75.0f));
    _replacementBox->setEnabled(false);
    _replacementBox->addEventListener(std::bind(&PointsCalculatorScene::onReplacementBox, this, std::placeholders::_1, std::placeholders::_2));

    Label *replacementLabel = Label::createWithSystemFont("杠开", "Arial", 12);
    replacementLabel->setColor(textColor);
    infoWidget->addChild(replacementLabel);
    replacementLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    replacementLabel->setPosition(Vec2(35.0f, 75.0f));

    // 抢杠
    _robKongBox = ui::CheckBox::create("source_material/btn_square_normal.png", "", "source_material/btn_square_highlighted.png", "source_material/btn_square_disabled.png", "source_material/btn_square_disabled.png");
    infoWidget->addChild(_robKongBox);
    _robKongBox->setZoomScale(0.0f);
    _robKongBox->ignoreContentAdaptWithSize(false);
    _robKongBox->setContentSize(Size(20.0f, 20.0f));
    _robKongBox->setPosition(Vec2(20.0f + gapX, 75.0f));
    _robKongBox->setEnabled(false);
    _robKongBox->addEventListener(std::bind(&PointsCalculatorScene::onRobKongBox, this, std::placeholders::_1, std::placeholders::_2));

    Label *robKongLabel = Label::createWithSystemFont("抢杠", "Arial", 12);
    robKongLabel->setColor(textColor);
    infoWidget->addChild(robKongLabel);
    robKongLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    robKongLabel->setPosition(Vec2(35.0f + gapX, 75.0f));

    // 海底
    _lastTileBox = ui::CheckBox::create("source_material/btn_square_normal.png", "", "source_material/btn_square_highlighted.png", "source_material/btn_square_disabled.png", "source_material/btn_square_disabled.png");
    infoWidget->addChild(_lastTileBox);
    _lastTileBox->setZoomScale(0.0f);
    _lastTileBox->ignoreContentAdaptWithSize(false);
    _lastTileBox->setContentSize(Size(20.0f, 20.0f));
    _lastTileBox->setPosition(Vec2(20.0f + gapX * 2, 75.0f));
    _lastTileBox->addEventListener(std::bind(&PointsCalculatorScene::onLastTileBox, this, std::placeholders::_1, std::placeholders::_2));

    Label *lastTileDrawnLabel = Label::createWithSystemFont("海底", "Arial", 12);
    lastTileDrawnLabel->setColor(textColor);
    infoWidget->addChild(lastTileDrawnLabel);
    lastTileDrawnLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    lastTileDrawnLabel->setPosition(Vec2(35.0f + gapX * 2, 75.0f));

    const char *windName[4] = { "东", "南", "西", "北" };

    // 圈风
    Label *prevalentWindLabel = Label::createWithSystemFont("圈风", "Arial", 12);
    prevalentWindLabel->setColor(textColor);
    infoWidget->addChild(prevalentWindLabel);
    prevalentWindLabel->setPosition(Vec2(20.0f, 45.0f));

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

        Label *label = Label::createWithSystemFont(windName[i], "Arial", 12);
        label->setColor(Color3B::BLACK);
        button->addChild(label);
        label->setPosition(Vec2(10.0f, 10.0f));
    }

    // 门风
    Label *seatWindLabel = Label::createWithSystemFont("门风", "Arial", 12);
    seatWindLabel->setColor(textColor);
    infoWidget->addChild(seatWindLabel);
    seatWindLabel->setPosition(Vec2(20.0f, 15.0f));

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

        Label *label = Label::createWithSystemFont(windName[i], "Arial", 12);
        label->setColor(Color3B::BLACK);
        button->addChild(label);
        label->setPosition(Vec2(10.0f, 10.0f));
    }

    // 直接输入
    ui::Button *button = ui::Button::create(normalImage, selectedImage);
    button->setScale9Enabled(true);
    button->setContentSize(Size(55.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText("直接输入");
    button->setTitleColor(Color3B::BLACK);
    infoWidget->addChild(button);
    button->setPosition(Vec2(visibleSize.width - 40, 90.0f));
    button->addClickEventListener([this](Ref *) { showInputAlert(nullptr); });

    // 花牌数
    Label *flowerLabel = Label::createWithSystemFont("花牌数", "Arial", 12);
    flowerLabel->setColor(textColor);
    infoWidget->addChild(flowerLabel);
    flowerLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_RIGHT);
    flowerLabel->setPosition(Vec2(visibleSize.width - 50, 45.0f));

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
        _fourthTileBox->setEnabled(_maybeFourthTile && !_robKongBox->isSelected());
        _replacementBox->setEnabled(false);
        _robKongBox->setEnabled(_maybeRobKong && !_fourthTileBox->isSelected() && !_lastTileBox->isSelected());
        _lastTileBox->setEnabled(!_robKongBox->isSelected());
    }
    else {  // 自摸
        _fourthTileBox->setEnabled(_maybeFourthTile);
        _replacementBox->setEnabled(_hasKong);
        _robKongBox->setEnabled(false);
        _lastTileBox->setEnabled(true);
    }
}

void PointsCalculatorScene::onFourthTileBox(cocos2d::Ref *sender, cocos2d::ui::CheckBox::EventType event) {
    // 绝张与抢杠互斥
    if (event == ui::CheckBox::EventType::SELECTED) {
        _robKongBox->setEnabled(false);
    }
    else {
        _robKongBox->setEnabled(_maybeRobKong
            && _winTypeGroup->getSelectedButtonIndex() == 0
            && !_lastTileBox->isSelected());
    }
}

void PointsCalculatorScene::onRobKongBox(cocos2d::Ref *sender, cocos2d::ui::CheckBox::EventType event) {
    // 抢杠与绝张、海底互斥
    if (event == ui::CheckBox::EventType::SELECTED) {
        _fourthTileBox->setEnabled(false);
        _lastTileBox->setEnabled(false);
    }
    else {
        _fourthTileBox->setEnabled(_maybeFourthTile);
        _lastTileBox->setEnabled(true);
    }
}

void PointsCalculatorScene::onReplacementBox(cocos2d::Ref *sender, cocos2d::ui::CheckBox::EventType event) {
    if (event == ui::CheckBox::EventType::UNSELECTED) {

    }
}

void PointsCalculatorScene::onLastTileBox(cocos2d::Ref *sender, cocos2d::ui::CheckBox::EventType event) {
    // 海底与抢杠互斥
    if (event == ui::CheckBox::EventType::SELECTED) {
        _robKongBox->setEnabled(false);
    }
    else {
        _robKongBox->setEnabled(_maybeRobKong && _winTypeGroup->getSelectedButtonIndex() == 0
            && !_fourthTileBox->isSelected());
    }
}

void PointsCalculatorScene::onFixedSetsChanged(TilePickWidget *sender) {
    // 当副露不包含杠的时候，杠开是禁用状态
    const std::vector<mahjong::SET> &fixedSets = sender->getFixedSets();
    _hasKong = std::any_of(fixedSets.begin(), fixedSets.end(), [](const mahjong::SET &s) { return s.set_type == mahjong::SET_TYPE::KONG; });
    if (_winTypeGroup->getSelectedButtonIndex() == 1) {
        _replacementBox->setEnabled(_hasKong);
    }
    else {
        _replacementBox->setEnabled(false);
    }
}

void PointsCalculatorScene::onWinTileChanged(TilePickWidget *sender) {
    // 当立牌中有和牌张时，绝张是禁用状态
    _maybeFourthTile = false;
    _maybeRobKong = false;
    mahjong::TILE winTile = sender->getWinTile();
    if (winTile == 0) {
        _fourthTileBox->setEnabled(false);
        _robKongBox->setEnabled(false);
        _lastTileBox->setEnabled(true);
    }
    else {
        // 立牌中有和牌张时，不可能是绝张和抢杠
        const std::vector<mahjong::TILE> &standingTiles = sender->getStandingTiles();
        auto it = std::find(standingTiles.begin(), standingTiles.end(), winTile);
        _maybeFourthTile = (it == standingTiles.end());

        if (_maybeFourthTile) {
            // 当副露中有和牌张是，不可能是抢杠
            const std::vector<mahjong::SET> &fixedSets = sender->getFixedSets();
            _maybeRobKong = std::none_of(fixedSets.begin(), fixedSets.end(),
                std::bind(&mahjong::is_set_contains_tile, std::placeholders::_1, winTile));
        }
        else {
            _maybeRobKong = false;
        }

        _fourthTileBox->setEnabled(_maybeFourthTile && !_robKongBox->isSelected());
        _robKongBox->setEnabled(_maybeRobKong
            && _winTypeGroup->getSelectedButtonIndex() == 0
            && !_lastTileBox->isSelected()
            && !_fourthTileBox->isSelected());
        _lastTileBox->setEnabled(!_robKongBox->isSelected());
    }
}

void PointsCalculatorScene::showInputAlert(const char *prevInput) {
    Size visibleSize = Director::getInstance()->getVisibleSize();
    const float width = visibleSize.width * 0.8f;

    ui::Widget *rootWidget = ui::Widget::create();

    Label *label = Label::createWithSystemFont("使用说明：\n"
        "1.数牌：万=m 条=s 饼=p。后缀使用小写字母，同花色的数牌可合并用一个后缀。\n"
        "2.字牌：东南西北=ESWN，中发白=CFP。使用大写字母。\n"
        "3.每一组副露（即吃、碰、明杠）用英文[]，每一组暗杠用英文{}。\n"
        "4.请将所有副露放在最前面，然后是立牌，和牌用英文逗号,与手牌分隔开。\n"
        "范例1：{EEEE}{CCCC}{FFFF}{PPPP}N,N\n"
        "范例2：1112345678999s,9s\n"
        "范例3：[WWWW][444s]45m678pFF,6m\n", "Arial", 10);
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

    AlertLayer::showWithNode("直接输入牌", rootWidget, [this, editBox]() {
        parseInput(editBox->getText());
    }, nullptr);
}

void PointsCalculatorScene::parseInput(const char *input) {
    const char *errorStr = nullptr;
    const std::string str = input;

    do {
        if (strspn(input, "123456789mpsESWNCFP[]{},") != str.length()) {
            errorStr = "无法解析输入的文本";
            break;
        }

        std::string::const_iterator it = std::find(str.begin(), str.end(), ',');
        if (it == str.end()) {
            errorStr = "缺少和牌张";
            break;
        }

        std::string tilesString(str.begin(), it);
        std::string winString(it + 1, str.end());

        mahjong::SET fixed_sets[5];
        long set_cnt;
        mahjong::TILE standing_tiles[13];
        long tile_cnt;
        mahjong::TILE win_tile;

        if (!mahjong::string_to_tiles(tilesString.c_str(), fixed_sets, &set_cnt, standing_tiles, &tile_cnt)) {
            errorStr = "无法正确解析输入的文本";
            break;
        }
        mahjong::sort_tiles(standing_tiles, tile_cnt);

        const char *p = mahjong::parse_tiles(winString.c_str(), &win_tile, nullptr);
        if (*p != '\0') {
            errorStr = "无法正确解析输入的文本";
            break;
        }

        if (int ret = mahjong::check_calculator_input(fixed_sets, set_cnt, standing_tiles, tile_cnt, win_tile)) {
            switch (ret) {
            case ERROR_WRONG_TILES_COUNT: errorStr = "牌张数错误"; break;
            case ERROR_TILE_COUNT_GREATER_THAN_4: errorStr = "同一种牌最多只能使用4枚"; break;
            default: break;
            }
            break;
        }
        _tilePicker->setData(fixed_sets, set_cnt, standing_tiles, tile_cnt, win_tile);
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

    // 获取副露
    mahjong::SET fixed_sets[5];
    long set_cnt = std::copy(_tilePicker->getFixedSets().begin(), _tilePicker->getFixedSets().end(), std::begin(fixed_sets))
        - std::begin(fixed_sets);

    // 获取立牌
    mahjong::TILE standing_tiles[13];
    long tile_cnt = std::copy(_tilePicker->getStandingTiles().begin(), _tilePicker->getStandingTiles().end(), std::begin(standing_tiles))
        - std::begin(standing_tiles);
    mahjong::sort_tiles(standing_tiles, tile_cnt);

    // 获取和牌张
    mahjong::TILE win_tile = _tilePicker->getWinTile();

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
    int points = calculate_points(fixed_sets, set_cnt, standing_tiles, tile_cnt, win_tile, win_type, prevalent_wind, seat_wind, points_table);
    if (points == ERROR_NOT_WIN) {
        Label *errorLabel = Label::createWithSystemFont("诈和", "Arial", FONT_SIZE);
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

    Color3B textColor = UserDefault::getInstance()->getBoolForKey("night_mode") ? Color3B::WHITE : Color3B::BLACK;

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
