#include "FanCalculatorScene.h"
#include "../mahjong-algorithm/stringify.h"
#include "../mahjong-algorithm/fan_calculator.h"

#include "../widget/TilePickWidget.h"
#include "../widget/HandTilesWidget.h"
#include "../widget/AlertView.h"

USING_NS_CC;

Scene *FanCalculatorScene::createScene() {
    auto scene = Scene::create();
    auto layer = FanCalculatorScene::create();
    scene->addChild(layer);
    return scene;
}

bool FanCalculatorScene::init() {
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
    ui::Widget *infoWidget = ui::Widget::create();
    infoWidget->setContentSize(Size(visibleSize.width, 120));
    this->addChild(infoWidget);
    infoWidget->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, y - 85));

    // 番种显示的Node
    _fanAreaNode = Node::create();
    _fanAreaNode->setContentSize(Size(visibleSize.width, y - 145));
    _fanAreaNode->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    this->addChild(_fanAreaNode);
    _fanAreaNode->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + y * 0.5f - 70));

    // 点和与自摸互斥
    _winTypeGroup = ui::RadioButtonGroup::create();
    infoWidget->addChild(_winTypeGroup);
    _winTypeGroup->addEventListener(std::bind(&FanCalculatorScene::onWinTypeGroup, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

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
    _fourthTileBox->addEventListener(std::bind(&FanCalculatorScene::onFourthTileBox, this, std::placeholders::_1, std::placeholders::_2));

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
    _robKongBox->addEventListener(std::bind(&FanCalculatorScene::onRobKongBox, this, std::placeholders::_1, std::placeholders::_2));

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
    _lastTileBox->addEventListener(std::bind(&FanCalculatorScene::onLastTileBox, this, std::placeholders::_1, std::placeholders::_2));

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
    ui::Button *button = ui::Button::create(normalImage, selectedImage);
    button->setScale9Enabled(true);
    button->setContentSize(Size(55.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText("直接输入");
    button->setTitleColor(Color3B::BLACK);
    infoWidget->addChild(button);
    button->setPosition(Vec2(visibleSize.width - 40, 75.0f));
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

void FanCalculatorScene::onWinTypeGroup(cocos2d::ui::RadioButton *radioButton, int index, cocos2d::ui::RadioButtonGroup::EventType event) {
    if (index == 0) {  // 点和
        // 绝张：可为绝张 && 抢杠没选中
        // 杠开：禁用
        // 抢杠：可为绝张 && 副露不包含和牌 && 绝张没选中 && 海底没选中
        // 海底：抢杠没选中
        _fourthTileBox->setEnabled(_maybeFourthTile && !_robKongBox->isSelected());
        _replacementBox->setEnabled(false);
        _robKongBox->setEnabled(_maybeFourthTile && _winTileCountInFixedPacks == 0
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

void FanCalculatorScene::onFourthTileBox(cocos2d::Ref *sender, cocos2d::ui::CheckBox::EventType event) {
    // 绝张与抢杠互斥
    if (event == ui::CheckBox::EventType::SELECTED) {
        // 抢杠：禁用
        _robKongBox->setEnabled(false);
    }
    else {
        // 一定是绝张，则不允许取消选中绝张
        if (_maybeFourthTile && _winTileCountInFixedPacks == 3) {
            _fourthTileBox->setSelected(true);
        }
        else {
            // 抢杠：可为绝张 && 副露不包含和牌 && 点和 && 海底没选中
            _robKongBox->setEnabled(_maybeFourthTile && _winTileCountInFixedPacks == 0
                && _winTypeGroup->getSelectedButtonIndex() == 0
                && !_lastTileBox->isSelected());
        }
    }
}

void FanCalculatorScene::onRobKongBox(cocos2d::Ref *sender, cocos2d::ui::CheckBox::EventType event) {
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

void FanCalculatorScene::onLastTileBox(cocos2d::Ref *sender, cocos2d::ui::CheckBox::EventType event) {
    // 海底与抢杠互斥
    if (event == ui::CheckBox::EventType::SELECTED) {
        // 抢杠：禁用
        _robKongBox->setEnabled(false);
    }
    else {
        // 抢杠：可为绝张 && 副露不包含和牌 && 点和 && 绝张没选中
        _robKongBox->setEnabled(_maybeFourthTile && _winTileCountInFixedPacks == 0
            && _winTypeGroup->getSelectedButtonIndex() == 0
            && !_fourthTileBox->isSelected());
    }
}

void FanCalculatorScene::onFixedPacksChanged() {
    // 当副露不包含杠的时候，杠开是禁用状态
    _hasKong = _tilePicker->getHandTilesWidget()->isFixedPacksContainsKong();
    if (_winTypeGroup->getSelectedButtonIndex() == 1) {
        // 杠开：有杠
        _replacementBox->setEnabled(_hasKong);
    }
    else {
        // 杠开：禁用
        _replacementBox->setEnabled(false);
    }
}

void FanCalculatorScene::onWinTileChanged() {
    _maybeFourthTile = false;
    _winTileCountInFixedPacks = 0;
    mahjong::tile_t winTile = _tilePicker->getHandTilesWidget()->getServingTile();
    if (winTile == 0) {  // 没有和牌张
        _fourthTileBox->setEnabled(false);
        _robKongBox->setEnabled(false);
        _lastTileBox->setEnabled(true);
        return;
    }

    // 立牌中不包含和牌张，则可能为绝张
    _maybeFourthTile = !_tilePicker->getHandTilesWidget()->isStandingTilesContainsServingTile();

    // 一定为绝张
    _winTileCountInFixedPacks = _tilePicker->getHandTilesWidget()->countServingTileInFixedPacks();
    if (_maybeFourthTile && _winTileCountInFixedPacks == 3) {
        _fourthTileBox->setEnabled(true);
        _robKongBox->setEnabled(false);
        _fourthTileBox->setSelected(true);
        return;
    }

    // 绝张：可为绝张 && 抢杠没选中
    // 抢杠：可为绝张 && 副露不包含和牌 && 点和 && 绝张没选中 && 海底没选中
    // 海底：抢杠没选中
    _fourthTileBox->setEnabled(_maybeFourthTile && !_robKongBox->isSelected());
    _robKongBox->setEnabled(_maybeFourthTile && _winTileCountInFixedPacks == 0
        && _winTypeGroup->getSelectedButtonIndex() == 0
        && !_lastTileBox->isSelected()
        && !_fourthTileBox->isSelected());
    _lastTileBox->setEnabled(!_robKongBox->isSelected());
}

void FanCalculatorScene::showInputAlert(const char *prevInput) {
    Size visibleSize = Director::getInstance()->getVisibleSize();
    const float width = visibleSize.width * 0.8f;

    ui::Widget *rootWidget = ui::Widget::create();

    Label *label = Label::createWithSystemFont("使用说明：\n"
        "1." INPUT_GUIDE_STRING_1 "\n"
        "2." INPUT_GUIDE_STRING_2 "\n"
        "3." INPUT_GUIDE_STRING_3 "\n"
        "输入范例1：[EEEE][CCCC][FFFF][PPPP]NN\n"
        "输入范例2：1112345678999s9s\n"
        "输入范例3：WWWW 444s 45m678pFF6m\n", "Arial", 10);
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

    AlertView::showWithNode("直接输入", rootWidget, [this, editBox]() {
        parseInput(editBox->getText());
    }, nullptr);
}

void FanCalculatorScene::parseInput(const char *input) {
    if (*input == '\0') {
        return;
    }

    const char *errorStr = nullptr;
    const std::string str = input;

    do {
        mahjong::hand_tiles_t hand_tiles;
        mahjong::tile_t win_tile;
        long ret = mahjong::string_to_tiles(input, &hand_tiles, &win_tile);
        if (ret != PARSE_NO_ERROR) {
            switch (ret) {
            case PARSE_ERROR_ILLEGAL_CHARACTER: errorStr = "无法解析的字符"; break;
            case PARSE_ERROR_NO_SUFFIX_AFTER_DIGIT: errorStr = "数字后面需有后缀"; break;
            case PARSE_ERROR_TOO_MANY_TILES_FOR_FIXED_PACK: errorStr = "一组副露包含了过多的牌"; break;
            case PARSE_ERROR_CANNOT_MAKE_FIXED_PACK: errorStr = "无法正确解析副露"; break;
            default: break;
            }
            break;
        }
        if (win_tile == 0) {
            errorStr = "缺少和牌张";
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
        AlertView::showWithMessage("直接输入牌", errorStr, [this, str]() {
            showInputAlert(str.c_str());
        }, nullptr);
    }
}

#define FONT_SIZE 14

void FanCalculatorScene::calculate() {
    _fanAreaNode->removeAllChildren();

    Size visibleSize = Director::getInstance()->getVisibleSize();
    const Size &fanAreaSize = _fanAreaNode->getContentSize();
    Vec2 pos(fanAreaSize.width * 0.5f, fanAreaSize.height * 0.5f);

    int flowerCnt = atoi(_editBox->getText());
    if (flowerCnt > 8) {
        AlertView::showWithMessage("算番", "花牌数的范围为0~8", nullptr, nullptr);
        return;
    }

    mahjong::hand_tiles_t hand_tiles;
    mahjong::tile_t win_tile;
    _tilePicker->getHandTilesWidget()->getData(&hand_tiles, &win_tile);
    if (win_tile == 0) {
        AlertView::showWithMessage("算番", "缺少和牌张", nullptr, nullptr);
        return;
    }

    mahjong::sort_tiles(hand_tiles.standing_tiles, hand_tiles.tile_count);

    long fan_table[mahjong::FAN_TABLE_SIZE] = { 0 };

    // 获取绝张、杠开、抢杠、海底信息
    mahjong::win_flag_t win_flag = WIN_FLAG_DISCARD;
    if (_winTypeGroup->getSelectedButtonIndex() == 1) win_flag |= WIN_FLAG_SELF_DRAWN;
    if (_fourthTileBox->isEnabled() && _fourthTileBox->isSelected()) win_flag |= WIN_FLAG_4TH_TILE;
    if (_robKongBox->isEnabled() && _robKongBox->isSelected()) win_flag |= WIN_FLAG_ABOUT_KONG;
    if (_replacementBox->isEnabled() && _replacementBox->isSelected()) win_flag |= (WIN_FLAG_ABOUT_KONG | WIN_FLAG_SELF_DRAWN);
    if (_lastTileBox->isEnabled() && _lastTileBox->isSelected()) win_flag |= WIN_FLAG_WALL_LAST;

    // 获取圈风门风
    mahjong::wind_t prevalent_wind = static_cast<mahjong::wind_t>(static_cast<int>(mahjong::wind_t::EAST) + _prevalentWindGroup->getSelectedButtonIndex());
    mahjong::wind_t seat_wind = static_cast<mahjong::wind_t>(static_cast<int>(mahjong::wind_t::EAST) + _seatWindGroup->getSelectedButtonIndex());

    // 算番
    mahjong::extra_condition_t ext_cond;
    ext_cond.win_flag = win_flag;
    ext_cond.prevalent_wind = prevalent_wind;
    ext_cond.seat_wind = seat_wind;
    int fan = calculate_fan(&hand_tiles, win_tile, &ext_cond, fan_table);

    Color3B textColor = UserDefault::getInstance()->getBoolForKey("night_mode") ? Color3B::WHITE : Color3B::BLACK;
    if (fan == ERROR_NOT_WIN) {
        Label *errorLabel = Label::createWithSystemFont("诈和", "Arial", FONT_SIZE);
        errorLabel->setColor(textColor);
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
        fanName->setColor(textColor);
        innerNode->addChild(fanName);
        fanName->setAnchorPoint(Vec2::ANCHOR_TOP_LEFT);
        div_t ret = div(i, 2);
        fanName->setPosition(Vec2(ret.rem == 0 ? 5.0f : visibleSize.width * 0.5f + 5.0f, (FONT_SIZE + 2) * (rows - ret.quot + 2)));
    }

    Label *fanTotal = Label::createWithSystemFont(StringUtils::format("总计：%d番", fan), "Arial", FONT_SIZE);
    fanTotal->setColor(textColor);
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
