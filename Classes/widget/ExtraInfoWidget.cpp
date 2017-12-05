#include "ExtraInfoWidget.h"
#include "../utils/common.h"
#include "AlertView.h"
#include "../mahjong-algorithm/stringify.h"

USING_NS_CC;

bool ExtraInfoWidget::init() {
    if (UNLIKELY(!Node::init())) {
        return false;
    }

    this->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    this->setIgnoreAnchorPointForPosition(false);

    Size visibleSize = Director::getInstance()->getVisibleSize();

    this->setContentSize(Size(visibleSize.width, 110.0f));

    const float gapX = 65.0f;

    // 点和与自摸互斥
    ui::RadioButtonGroup *radioGroup = ui::RadioButtonGroup::create();
    this->addChild(radioGroup);
    radioGroup->addEventListener(std::bind(&ExtraInfoWidget::onWinTypeGroup, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    static const char *winTypeTexts[] = { "点和", "自摸" };
    for (int i = 0; i < 2; ++i) {
        ui::RadioButton *radioButton = ui::RadioButton::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
        this->addChild(radioButton);
        radioButton->setZoomScale(0.0f);
        radioButton->ignoreContentAdaptWithSize(false);
        radioButton->setContentSize(Size(20.0f, 20.0f));
        radioButton->setPosition(Vec2(20.0f + gapX * i, 100.0f));
        radioGroup->addRadioButton(radioButton);

        Label *label = Label::createWithSystemFont(winTypeTexts[i], "Arial", 12);
        label->setColor(Color3B::BLACK);
        this->addChild(label);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        label->setPosition(Vec2(35.0f + gapX * i, 100.0f));
    }
    _winTypeGroup = radioGroup;

    // 绝张
    ui::CheckBox *checkBox = ui::CheckBox::create("source_material/btn_square_normal.png", "", "source_material/btn_square_highlighted.png", "source_material/btn_square_disabled.png", "source_material/btn_square_disabled.png");
    this->addChild(checkBox);
    checkBox->setZoomScale(0.0f);
    checkBox->ignoreContentAdaptWithSize(false);
    checkBox->setContentSize(Size(20.0f, 20.0f));
    checkBox->setPosition(Vec2(20.0f + gapX * 2, 100.0f));
    checkBox->setEnabled(false);
    checkBox->addEventListener(std::bind(&ExtraInfoWidget::onFourthTileBox, this, std::placeholders::_1, std::placeholders::_2));
    _fourthTileBox = checkBox;

    Label *label = Label::createWithSystemFont("绝张", "Arial", 12);
    label->setColor(Color3B::BLACK);
    this->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(35.0f + gapX * 2, 100.0f));

    static const char *extTexts[] = { "杠开", "抢杠", "海底" };
    ui::CheckBox *checkBoxes[3];
    for (int i = 0; i < 3; ++i) {
        ui::CheckBox *checkBox = ui::CheckBox::create("source_material/btn_square_normal.png", "", "source_material/btn_square_highlighted.png", "source_material/btn_square_disabled.png", "source_material/btn_square_disabled.png");
        this->addChild(checkBox);
        checkBox->setZoomScale(0.0f);
        checkBox->ignoreContentAdaptWithSize(false);
        checkBox->setContentSize(Size(20.0f, 20.0f));
        checkBox->setPosition(Vec2(20.0f + gapX * i, 70.0f));
        checkBoxes[i] = checkBox;

        label = Label::createWithSystemFont(extTexts[i], "Arial", 12);
        label->setColor(Color3B::BLACK);
        this->addChild(label);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        label->setPosition(Vec2(35.0f + gapX * i, 70.0f));
    }

    checkBoxes[0]->setEnabled(false);
    _replacementBox = checkBoxes[0];

    checkBoxes[1]->setEnabled(false);
    checkBoxes[1]->addEventListener(std::bind(&ExtraInfoWidget::onRobKongBox, this, std::placeholders::_1, std::placeholders::_2));
    _robKongBox = checkBoxes[1];

    checkBoxes[2]->addEventListener(std::bind(&ExtraInfoWidget::onLastTileBox, this, std::placeholders::_1, std::placeholders::_2));
    _lastTileBox = checkBoxes[2];

    // 圈风和门风
    static const char *windName[4] = { "东", "南", "西", "北" };
    static const char *windType[2] = { "圈风", "门风" };
    for (int k = 0; k < 2; ++k) {
        const float posY = 40.0f - k * 30.0f;

        label = Label::createWithSystemFont(windType[k], "Arial", 12);
        label->setColor(Color3B::BLACK);
        this->addChild(label);
        label->setPosition(Vec2(20.0f, posY));

        radioGroup = ui::RadioButtonGroup::create();
        this->addChild(radioGroup);
        for (int i = 0; i < 4; ++i) {
            ui::RadioButton *radioButton = ui::RadioButton::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
            radioButton->setZoomScale(0.0f);
            radioButton->ignoreContentAdaptWithSize(false);
            radioButton->setContentSize(Size(20.0f, 20.0f));
            radioButton->setPosition(Vec2(50.0f + i * 30, posY));
            this->addChild(radioButton);

            label = Label::createWithSystemFont(windName[i], "Arial", 12);
            label->setColor(Color3B::BLACK);
            radioButton->addChild(label);
            label->setPosition(Vec2(10.0f, 10.0f));

            radioGroup->addRadioButton(radioButton);
        }
        _windGroups[k] = radioGroup;
    }

    // 直接输入
    ui::Button *button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
    button->setScale9Enabled(true);
    button->setContentSize(Size(55.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText("直接输入");
    this->addChild(button);
    button->setPosition(Vec2(visibleSize.width - 40.0f, 100.0f));
    button->addClickEventListener([this](Ref *) { showInputAlert(nullptr); });

    // 使用说明
    button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
    button->setScale9Enabled(true);
    button->setContentSize(Size(55.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText("使用说明");
    this->addChild(button);
    button->setPosition(Vec2(visibleSize.width - 40.0f, 70.0f));
    button->addClickEventListener(std::bind(&ExtraInfoWidget::onInstructionButton, this, std::placeholders::_1));

    // 花牌数
    label = Label::createWithSystemFont("\xF0\x9F\x8C\xB8", "Arial", 12);
#if CC_TARGET_PLATFORM == CC_PLATFORM_WIN32
    label->setColor(Color3B(224, 45, 45));
#endif
    this->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_RIGHT);
    label->setPosition(Vec2(visibleSize.width - 85.0f, 40.0f));

    label = Label::createWithSystemFont("x0", "Arial", 12);
    label->setColor(Color3B::BLACK);
    this->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(visibleSize.width - 85.0f, 40.0f));
    label->setTag(0);
    _flowerLabel = label;

    button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
    button->setScale9Enabled(true);
    button->setContentSize(Size(25.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText("-1");
    this->addChild(button);
    button->setPosition(Vec2(visibleSize.width - 55.0f, 40.0f));
    button->addClickEventListener([label](Ref *) {
        int n = label->getTag();
        if (n > 0) {
            label->setTag(--n);
            label->setString(Common::format("x%d", n));
        }
    });

    button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
    button->setScale9Enabled(true);
    button->setContentSize(Size(25.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText("+1");
    this->addChild(button);
    button->setPosition(Vec2(visibleSize.width - 25.0f, 40.0f));
    button->addClickEventListener([label](Ref *) {
        int n = label->getTag();
        if (n < 8) {
            label->setTag(++n);
            label->setString(Common::format("x%d", n));
        }
    });

    return true;
}

int ExtraInfoWidget::getFlowerCount() const {
    return _flowerLabel->getTag();
}

void ExtraInfoWidget::setFlowerCount(int cnt) {
    if (cnt > 8) cnt = 8;
    if (cnt < 0) cnt = 0;
    _flowerLabel->setTag(cnt);
    _flowerLabel->setString(Common::format("x%d", cnt));
}

mahjong::win_flag_t ExtraInfoWidget::getWinFlag() const {
    mahjong::win_flag_t ret = WIN_FLAG_DISCARD;
    if (_winTypeGroup->getSelectedButtonIndex() == 1) ret |= WIN_FLAG_SELF_DRAWN;
    if (_fourthTileBox->isEnabled() && _fourthTileBox->isSelected()) ret |= WIN_FLAG_4TH_TILE;
    if (_robKongBox->isEnabled() && _robKongBox->isSelected()) ret |= WIN_FLAG_ABOUT_KONG;
    if (_replacementBox->isEnabled() && _replacementBox->isSelected()) ret |= (WIN_FLAG_ABOUT_KONG | WIN_FLAG_SELF_DRAWN);
    if (_lastTileBox->isEnabled() && _lastTileBox->isSelected()) ret |= WIN_FLAG_WALL_LAST;
    return ret;
}

void ExtraInfoWidget::setWinFlag(mahjong::win_flag_t flag) {
    if (flag & WIN_FLAG_SELF_DRAWN) {
        _winTypeGroup->setSelectedButton(1);
    }

    if (flag & WIN_FLAG_WALL_LAST) {
        _lastTileBox->setSelected(true);
        onLastTileBox(_lastTileBox, ui::CheckBox::EventType::SELECTED);
    }

    if (flag & WIN_FLAG_ABOUT_KONG) {
        if (flag & WIN_FLAG_SELF_DRAWN) {
            _replacementBox->setSelected(_hasKong);
        }
        else {
            _robKongBox->setSelected(_maybeFourthTile);
            onRobKongBox(_robKongBox, _maybeFourthTile ? ui::CheckBox::EventType::SELECTED : ui::CheckBox::EventType::UNSELECTED);
        }
    }
}

mahjong::wind_t ExtraInfoWidget::getPrevalentWind() const {
    return static_cast<mahjong::wind_t>(static_cast<int>(mahjong::wind_t::EAST) + _windGroups[0]->getSelectedButtonIndex());
}

void ExtraInfoWidget::setPrevalentWind(mahjong::wind_t wind) {
    switch (wind) {
    case mahjong::wind_t::EAST: _windGroups[0]->setSelectedButton(0); break;
    case mahjong::wind_t::SOUTH: _windGroups[0]->setSelectedButton(1); break;
    case mahjong::wind_t::WEST: _windGroups[0]->setSelectedButton(2); break;
    case mahjong::wind_t::NORTH: _windGroups[0]->setSelectedButton(3); break;
    }
}

mahjong::wind_t ExtraInfoWidget::getSeatWind() const {
    return static_cast<mahjong::wind_t>(static_cast<int>(mahjong::wind_t::EAST) + _windGroups[1]->getSelectedButtonIndex());
}

void ExtraInfoWidget::setSeatWind(mahjong::wind_t wind) {
    switch (wind) {
    case mahjong::wind_t::EAST: _windGroups[1]->setSelectedButton(0); break;
    case mahjong::wind_t::SOUTH: _windGroups[1]->setSelectedButton(1); break;
    case mahjong::wind_t::WEST: _windGroups[1]->setSelectedButton(2); break;
    case mahjong::wind_t::NORTH: _windGroups[1]->setSelectedButton(3); break;
    }
}

void ExtraInfoWidget::onWinTypeGroup(cocos2d::ui::RadioButton *, int index, cocos2d::ui::RadioButtonGroup::EventType) {
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

void ExtraInfoWidget::onFourthTileBox(cocos2d::Ref *, cocos2d::ui::CheckBox::EventType event) {
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

void ExtraInfoWidget::onRobKongBox(cocos2d::Ref *, cocos2d::ui::CheckBox::EventType event) {
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

void ExtraInfoWidget::onLastTileBox(cocos2d::Ref *, cocos2d::ui::CheckBox::EventType event) {
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
    
void ExtraInfoWidget::refreshByKong(bool hasKong) {
    // 当副露不包含杠的时候，杠开是禁用状态
    _hasKong = hasKong;
    if (_winTypeGroup->getSelectedButtonIndex() == 1) {
        // 杠开：有杠
        _replacementBox->setEnabled(_hasKong);
    }
    else {
        // 杠开：禁用
        _replacementBox->setEnabled(false);
    }
}

void ExtraInfoWidget::refreshByWinTile(mahjong::tile_t winTile, bool maybeFourthTile, size_t winTileCountInFixedPacks, bool hasKong) {
    _maybeFourthTile = false;
    _winTileCountInFixedPacks = 0;
    if (winTile == 0) {  // 没有和牌张
        _fourthTileBox->setEnabled(false);
        _robKongBox->setEnabled(false);
        _lastTileBox->setEnabled(true);
        return;
    }

    _maybeFourthTile = maybeFourthTile;

    // 一定为绝张
    _winTileCountInFixedPacks = winTileCountInFixedPacks;
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

    // 杠开
    refreshByKong(hasKong);
}

void ExtraInfoWidget::onInstructionButton(cocos2d::Ref *) {
    Size visibleSize = Director::getInstance()->getVisibleSize();
    const float maxWidth = visibleSize.width * 0.8f - 10;
    Label *label = Label::createWithSystemFont(
        "1. 本程序不对和牌张位置的牌进行吃、碰、杠检测，如果要对某张牌进行吃、碰、杠操作，请将这张牌放在手牌范围内。点击算番结果处的番种名，可跳转到相应番种的定义。\n"
        "2. 本程序遵循中国国家体育总局于1998年7月审定的《中国麻将竞赛规则（试行）》，一些争议之处采取大众普遍接受的通行计番方式，请以您所参加的比赛细则中之规定为准。\n"
        "3. 手牌在形式上听多种牌（包括听第5张不存在的牌）时，不计边张、嵌张、单钓将。边张、嵌张、单钓将最多计其中一个。\n"
        "4. 组合龙普通型和牌中，可加计平和。对于可解释为组合龙龙身部分的听牌，一律不计边张、嵌张、单钓将。\n"
        "5. 必然门前清的番种（七对、全不靠、七星不靠、十三幺、四暗刻、连七对、九莲宝灯）自摸和牌时只计自摸，不计不求人。\n"
        "6. 绿一色、字一色、清幺九、混幺九、全大、全中、全小、大于五、小于五均可复合七对。全双刻不可复合七对，对于由仅偶数牌组成的七对，只计七对+断幺。\n"
        "7. 字一色、混幺九、清幺九不计碰碰和。\n"
        "8. 至少缺一门花色的番种（包括大三元、小三元、三风刻、一色四同顺、一色四节高、一色四步高、推不倒），除推不倒不计缺一门以外，其他番种均可加计缺一门。\n"
        "9. 有发的绿一色不计混一色。大四喜、小四喜可计混一色。\n"
        "10. 清幺九不计双同刻，可计三同刻。\n"
        "11. 九莲宝灯和258时计1个幺九刻，和其他牌不计幺九刻。\n"
        "12. 不重复原则特指单个番种与其他番种的必然包含关系，不适用某几个番种同时出现时与其他番种的包含关系。例如，绿一色+清一色，必然断幺，但要计断幺。\n"
        "13. 双暗杠6番，一明杠一暗杠5番，双明杠4番。暗杠的加计遵循国际麻将联盟（MIL）的规则，即杠系列和暗刻系列最多各计一个。",
        "Arail", 10);
    label->setColor(Color3B::BLACK);
    label->setDimensions(maxWidth, 0.0f);

    Node *node = nullptr;

    // 超出高度就使用ScrollView
    const Size &labelSize = label->getContentSize();
    const float maxHeight = visibleSize.height * 0.8f - 80.0f;
    if (labelSize.height <= maxHeight) {
        node = label;
    }
    else {
        ui::ScrollView *scrollView = ui::ScrollView::create();
        scrollView->setDirection(ui::ScrollView::Direction::VERTICAL);
        scrollView->setScrollBarPositionFromCorner(Vec2(2.0f, 2.0f));
        scrollView->setScrollBarWidth(4.0f);
        scrollView->setScrollBarOpacity(0x99);
        scrollView->setBounceEnabled(true);
        scrollView->setContentSize(Size(maxWidth, maxHeight));
        scrollView->setInnerContainerSize(labelSize);
        scrollView->addChild(label);
        label->setPosition(Vec2(labelSize.width * 0.5f, labelSize.height * 0.5f));

        node = scrollView;
    }

    AlertView::showWithNode("使用说明", node, nullptr, nullptr);
}

void ExtraInfoWidget::showInputAlert(const char *prevInput) {
    Size visibleSize = Director::getInstance()->getVisibleSize();
    const float width = visibleSize.width * 0.8f - 10.0f;

    Node *rootNode = Node::create();

    Label *label = Label::createWithSystemFont("使用说明：\n"
        "1.数牌：万=m 条=s 饼=p。后缀使用小写字母，一连串同花色的数牌可合并使用用一个后缀，如123m、678s等等。\n"
        "2.字牌：东南西北=ESWN，中发白=CFP。使用大写字母。亦兼容天凤风格的后缀z，但按中国习惯顺序567z为中发白。\n"
        "3.吃、碰、杠用英文[]，可选用逗号+数字表示供牌来源。数字的具体规则如下：\n"
        "  (1) 吃：表示第几张牌是由上家打出，如[567m,2]表示57万吃6万（第2张）。对于不指定数字的，默认为吃第1张。\n"
        "  (2) 碰：表示由哪家打出，1为上家，2为对家，3为下家，如[999s,3]表示碰下家的9条。对于不指定数字的，默认为碰上家。\n"
        "  (3) 杠：与碰类似，但对于不指定数字的，则认为是暗杠。例如：[SSSS]表示暗杠南；[8888p,1]表示明杠上家的8饼。\n"
        "输入范例1：[EEEE][CCCC][FFFF][PPPP]NN\n"
        "输入范例2：1112345678999s9s\n"
        "输入范例3：[WWWW,1][444s]45m678pFF6m\n", "Arial", 10);
    label->setColor(Color3B::BLACK);
    label->setDimensions(width, 0.0f);
    rootNode->addChild(label);

    // 输入手牌
    ui::EditBox *editBox = ui::EditBox::create(Size(width - 10.0f, 20.0f), ui::Scale9Sprite::create("source_material/btn_square_normal.png"));
    editBox->setInputFlag(ui::EditBox::InputFlag::SENSITIVE);
    editBox->setInputMode(ui::EditBox::InputMode::SINGLE_LINE);
    editBox->setReturnType(ui::EditBox::KeyboardReturnType::DONE);
    editBox->setFontColor(Color4B::BLACK);
    editBox->setFontSize(12);
    editBox->setPlaceholderFontColor(Color4B::GRAY);
    editBox->setPlaceHolder("输入手牌");
    editBox->setMaxLength(50);
    if (prevInput != nullptr) {
        editBox->setText(prevInput);
    }

    rootNode->addChild(editBox);

    const Size &labelSize = label->getContentSize();
    rootNode->setContentSize(Size(width, labelSize.height + 30.0f));
    editBox->setPosition(Vec2(width * 0.5f, 10.0f));
    label->setPosition(Vec2(width * 0.5f, labelSize.height * 0.5f + 30.0f));

    AlertView::showWithNode("直接输入", rootNode, [this, editBox]() {
        const char *input = editBox->getText();
        const char *errorStr = parseInput(input);
        if (errorStr != nullptr) {
            const std::string str = input;
            AlertView::showWithMessage("直接输入牌", errorStr, 12, [this, str]() {
                showInputAlert(str.c_str());
            }, nullptr);
        }
    }, nullptr);
}

const char *ExtraInfoWidget::parseInput(const char *input) {
    if (*input == '\0') {
        return nullptr;
    }

    mahjong::hand_tiles_t hand_tiles;
    mahjong::tile_t win_tile;
    intptr_t ret = mahjong::string_to_tiles(input, &hand_tiles, &win_tile);
    if (ret != PARSE_NO_ERROR) {
        switch (ret) {
            case PARSE_ERROR_ILLEGAL_CHARACTER: return "无法解析的字符";
            case PARSE_ERROR_NO_SUFFIX_AFTER_DIGIT: return "数字后面需有后缀";
            case PARSE_ERROR_WRONG_TILES_COUNT_FOR_FIXED_PACK: return "一组副露包含了错误的牌数目";
            case PARSE_ERROR_CANNOT_MAKE_FIXED_PACK: return "无法正确解析副露";
            default: break;
        }
        return nullptr;
    }
    if (win_tile == 0) {
        return "缺少和牌张";
    }

    ret = mahjong::check_calculator_input(&hand_tiles, win_tile);
    if (ret != 0) {
        switch (ret) {
            case ERROR_WRONG_TILES_COUNT: return "牌张数错误";
            case ERROR_TILE_COUNT_GREATER_THAN_4: return "同一种牌最多只能使用4枚";
            default: break;
        }
        return nullptr;
    }

    _inputCallback(hand_tiles, win_tile);
    return nullptr;
}
