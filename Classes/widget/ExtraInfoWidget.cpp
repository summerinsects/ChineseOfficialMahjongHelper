#include "ExtraInfoWidget.h"
#include "../compiler.h"
#include "AlertView.h"

USING_NS_CC;

bool ExtraInfoWidget::init() {
    if (UNLIKELY(!Node::init())) {
        return false;
    }

    this->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    this->setIgnoreAnchorPointForPosition(false);

    Size visibleSize = Director::getInstance()->getVisibleSize();

    this->setContentSize(Size(visibleSize.width, 110));

    // 点和与自摸互斥
    _winTypeGroup = ui::RadioButtonGroup::create();
    this->addChild(_winTypeGroup);
    _winTypeGroup->addEventListener(std::bind(&ExtraInfoWidget::onWinTypeGroup, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    // 点和
    const float gapX = 65;
    ui::RadioButton *radioButton = ui::RadioButton::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
    this->addChild(radioButton);
    radioButton->setZoomScale(0.0f);
    radioButton->ignoreContentAdaptWithSize(false);
    radioButton->setContentSize(Size(20.0f, 20.0f));
    radioButton->setPosition(Vec2(20.0f, 100.0f));
    _winTypeGroup->addRadioButton(radioButton);

    Label *label = Label::createWithSystemFont("点和", "Arial", 12);
    label->setColor(Color3B::BLACK);
    this->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(35.0f, 100.0f));

    // 自摸
    radioButton = ui::RadioButton::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
    this->addChild(radioButton);
    radioButton->setZoomScale(0.0f);
    radioButton->ignoreContentAdaptWithSize(false);
    radioButton->setContentSize(Size(20.0f, 20.0f));
    radioButton->setPosition(Vec2(20.0f + gapX, 100.0f));
    _winTypeGroup->addRadioButton(radioButton);

    label = Label::createWithSystemFont("自摸", "Arial", 12);
    label->setColor(Color3B::BLACK);
    this->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(35.0f + gapX, 100.0f));

    // 绝张
    _fourthTileBox = ui::CheckBox::create("source_material/btn_square_normal.png", "", "source_material/btn_square_highlighted.png", "source_material/btn_square_disabled.png", "source_material/btn_square_disabled.png");
    this->addChild(_fourthTileBox);
    _fourthTileBox->setZoomScale(0.0f);
    _fourthTileBox->ignoreContentAdaptWithSize(false);
    _fourthTileBox->setContentSize(Size(20.0f, 20.0f));
    _fourthTileBox->setPosition(Vec2(20.0f + gapX * 2, 100.0f));
    _fourthTileBox->setEnabled(false);
    _fourthTileBox->addEventListener(std::bind(&ExtraInfoWidget::onFourthTileBox, this, std::placeholders::_1, std::placeholders::_2));

    label = Label::createWithSystemFont("绝张", "Arial", 12);
    label->setColor(Color3B::BLACK);
    this->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(35.0f + gapX * 2, 100.0f));

    // 杠开
    _replacementBox = ui::CheckBox::create("source_material/btn_square_normal.png", "", "source_material/btn_square_highlighted.png", "source_material/btn_square_disabled.png", "source_material/btn_square_disabled.png");
    this->addChild(_replacementBox);
    _replacementBox->setZoomScale(0.0f);
    _replacementBox->ignoreContentAdaptWithSize(false);
    _replacementBox->setContentSize(Size(20.0f, 20.0f));
    _replacementBox->setPosition(Vec2(20.0f, 70.0f));
    _replacementBox->setEnabled(false);

    label = Label::createWithSystemFont("杠开", "Arial", 12);
    label->setColor(Color3B::BLACK);
    this->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(35.0f, 70.0f));

    // 抢杠
    _robKongBox = ui::CheckBox::create("source_material/btn_square_normal.png", "", "source_material/btn_square_highlighted.png", "source_material/btn_square_disabled.png", "source_material/btn_square_disabled.png");
    this->addChild(_robKongBox);
    _robKongBox->setZoomScale(0.0f);
    _robKongBox->ignoreContentAdaptWithSize(false);
    _robKongBox->setContentSize(Size(20.0f, 20.0f));
    _robKongBox->setPosition(Vec2(20.0f + gapX, 70.0f));
    _robKongBox->setEnabled(false);
    _robKongBox->addEventListener(std::bind(&ExtraInfoWidget::onRobKongBox, this, std::placeholders::_1, std::placeholders::_2));

    label = Label::createWithSystemFont("抢杠", "Arial", 12);
    label->setColor(Color3B::BLACK);
    this->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(35.0f + gapX, 70.0f));

    // 海底
    _lastTileBox = ui::CheckBox::create("source_material/btn_square_normal.png", "", "source_material/btn_square_highlighted.png", "source_material/btn_square_disabled.png", "source_material/btn_square_disabled.png");
    this->addChild(_lastTileBox);
    _lastTileBox->setZoomScale(0.0f);
    _lastTileBox->ignoreContentAdaptWithSize(false);
    _lastTileBox->setContentSize(Size(20.0f, 20.0f));
    _lastTileBox->setPosition(Vec2(20.0f + gapX * 2, 70.0f));
    _lastTileBox->addEventListener(std::bind(&ExtraInfoWidget::onLastTileBox, this, std::placeholders::_1, std::placeholders::_2));

    label = Label::createWithSystemFont("海底", "Arial", 12);
    label->setColor(Color3B::BLACK);
    this->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(35.0f + gapX * 2, 70.0f));

    const char *windName[4] = { "东", "南", "西", "北" };

    // 圈风
    label = Label::createWithSystemFont("圈风", "Arial", 12);
    label->setColor(Color3B::BLACK);
    this->addChild(label);
    label->setPosition(Vec2(20.0f, 40.0f));

    _prevalentWindGroup = ui::RadioButtonGroup::create();
    this->addChild(_prevalentWindGroup);

    for (int i = 0; i < 4; ++i) {
        ui::RadioButton *button = ui::RadioButton::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
        button->setZoomScale(0.0f);
        button->ignoreContentAdaptWithSize(false);
        button->setContentSize(Size(20.0f, 20.0f));
        button->setPosition(Vec2(50.0f + i * 30, 40.0f));
        this->addChild(button);
        _prevalentWindGroup->addRadioButton(button);

        label = Label::createWithSystemFont(windName[i], "Arial", 12);
        label->setColor(Color3B::BLACK);
        button->addChild(label);
        label->setPosition(Vec2(10.0f, 10.0f));
    }

    // 门风
    label = Label::createWithSystemFont("门风", "Arial", 12);
    label->setColor(Color3B::BLACK);
    this->addChild(label);
    label->setPosition(Vec2(20.0f, 10.0f));

    _seatWindGroup = ui::RadioButtonGroup::create();
    this->addChild(_seatWindGroup);

    for (int i = 0; i < 4; ++i) {
        ui::RadioButton *button = ui::RadioButton::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
        button->setZoomScale(0.0f);
        button->ignoreContentAdaptWithSize(false);
        button->setContentSize(Size(20.0f, 20.0f));
        button->setPosition(Vec2(50.0f + i * 30, 10.0f));
        this->addChild(button);
        _seatWindGroup->addRadioButton(button);

        label = Label::createWithSystemFont(windName[i], "Arial", 12);
        label->setColor(Color3B::BLACK);
        button->addChild(label);
        label->setPosition(Vec2(10.0f, 10.0f));
    }

    // 使用说明
    ui::Button *button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
    button->setScale9Enabled(true);
    button->setContentSize(Size(55.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText("使用说明");
    this->addChild(button);
    button->setPosition(Vec2(visibleSize.width - 40, 70.0f));
    button->addClickEventListener(std::bind(&ExtraInfoWidget::onInstructionButton, this, std::placeholders::_1));

    // 花牌数
    label = Label::createWithSystemFont("花", "Arial", 12);
    label->setColor(Color3B::BLACK);
    this->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_RIGHT);
    label->setPosition(Vec2(visibleSize.width - 90, 40.0f));

    Sprite *sprite = Sprite::create("source_material/btn_square_normal.png");
    sprite->setScale(20 / sprite->getContentSize().width);
    this->addChild(sprite);
    sprite->setPosition(Vec2(visibleSize.width - 50, 40.0f));

    label = Label::createWithSystemFont("0", "Arial", 12);
    label->setColor(Color3B::BLACK);
    this->addChild(label);
    label->setPosition(Vec2(visibleSize.width - 50, 40.0f));
    _flowerLabel = label;

    button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
    button->setScale9Enabled(true);
    button->setContentSize(Size(25.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText("\xE2\x97\x80\xEF\xB8\x8E");
    this->addChild(button);
    button->setPosition(Vec2(visibleSize.width - 75, 40.0f));
    button->addClickEventListener([label](Ref *) {
        int n = atoi(label->getString().c_str());
        if (n > 0) {
            label->setString(StringUtils::format("%d", n - 1));
        }
    });

    button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
    button->setScale9Enabled(true);
    button->setContentSize(Size(25.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText("\xE2\x96\xB6\xEF\xB8\x8E");
    this->addChild(button);
    button->setPosition(Vec2(visibleSize.width - 25, 40.0f));
    button->addClickEventListener([label](Ref *) {
        int n = atoi(label->getString().c_str());
        if (n < 8) {
            label->setString(StringUtils::format("%d", n + 1));
        }
    });

    return true;
}

int ExtraInfoWidget::getFlowerCount() const {
    return atoi(_flowerLabel->getString().c_str());
}

void ExtraInfoWidget::setFlowerCount(int cnt) {
    _flowerLabel->setString(StringUtils::format("%d", cnt));
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
    if (flag & WIN_FLAG_SELF_DRAWN) _winTypeGroup->setSelectedButton(1);
    if (flag & WIN_FLAG_WALL_LAST) _lastTileBox->setSelected(true);
}

mahjong::wind_t ExtraInfoWidget::getPrevalentWind() const {
    return static_cast<mahjong::wind_t>(static_cast<int>(mahjong::wind_t::EAST) + _prevalentWindGroup->getSelectedButtonIndex());
}

void ExtraInfoWidget::setPrevalentWind(mahjong::wind_t wind) {
    switch (wind) {
    case mahjong::wind_t::EAST: _prevalentWindGroup->setSelectedButton(0); break;
    case mahjong::wind_t::SOUTH: _prevalentWindGroup->setSelectedButton(1); break;
    case mahjong::wind_t::WEST: _prevalentWindGroup->setSelectedButton(2); break;
    case mahjong::wind_t::NORTH: _prevalentWindGroup->setSelectedButton(3); break;
    }
}

mahjong::wind_t ExtraInfoWidget::getSeatWind() const {
    return static_cast<mahjong::wind_t>(static_cast<int>(mahjong::wind_t::EAST) + _seatWindGroup->getSelectedButtonIndex());
}

void ExtraInfoWidget::setSeatWind(mahjong::wind_t wind) {
    switch (wind) {
    case mahjong::wind_t::EAST: _seatWindGroup->setSelectedButton(0); break;
    case mahjong::wind_t::SOUTH: _seatWindGroup->setSelectedButton(1); break;
    case mahjong::wind_t::WEST: _seatWindGroup->setSelectedButton(2); break;
    case mahjong::wind_t::NORTH: _seatWindGroup->setSelectedButton(3); break;
    }
}

void ExtraInfoWidget::onWinTypeGroup(cocos2d::ui::RadioButton *radioButton, int index, cocos2d::ui::RadioButtonGroup::EventType event) {
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

void ExtraInfoWidget::onFourthTileBox(cocos2d::Ref *sender, cocos2d::ui::CheckBox::EventType event) {
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

void ExtraInfoWidget::onRobKongBox(cocos2d::Ref *sender, cocos2d::ui::CheckBox::EventType event) {
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

void ExtraInfoWidget::onLastTileBox(cocos2d::Ref *sender, cocos2d::ui::CheckBox::EventType event) {
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

void ExtraInfoWidget::refreshByWinTile(const RefreshByWinTile &rt) {
    _maybeFourthTile = false;
    _winTileCountInFixedPacks = 0;
    mahjong::tile_t winTile = rt.getWinTile();
    if (winTile == 0) {  // 没有和牌张
        _fourthTileBox->setEnabled(false);
        _robKongBox->setEnabled(false);
        _lastTileBox->setEnabled(true);
        return;
    }

    // 立牌中不包含和牌张，则可能为绝张
    _maybeFourthTile = !rt.isStandingTilesContainsServingTile();

    // 一定为绝张
    _winTileCountInFixedPacks = rt.countServingTileInFixedPacks();
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
    refreshByKong(rt.isFixedPacksContainsKong());
}

void ExtraInfoWidget::onInstructionButton(cocos2d::Ref *sender) {
    AlertView::showWithMessage("使用说明",
        "1. 本算番器遵循中国国家体育总局于1998年7月审定的《中国麻将竞赛规则（试行）》，一些争议之处采取大众普遍接受的通行计番方式，请以您所参加的比赛细则中之规定为准。\n"
        "2. 边张、嵌张、单钓将必须在严格独听时才计；对于可解释为组合龙龙身部分的牌，一律不计边张、嵌张、单钓将。\n"
        "3. 必然门前清的番种自摸和牌时只计自摸，不计不求人。如七对、全不靠、七星不靠、十三幺、四暗刻、连七对、九莲宝灯等。\n"
        "4. 绿一色、字一色、清幺九、混幺九、全大、全中、全小、大于五、小于五均可复合七对。\n"
        "5. 全双刻不可复合七对，对于由仅偶数牌组成的七对，只计七对+断幺。\n"
        "6. 大三元、小三元、三风刻、一色四同顺、一色四节高、一色四步高等至少缺少一门花色序数牌的番种，可计缺一门。\n"
        "7. 有发的绿一色不计混一色。\n"
        "8. 大四喜、小四喜可计混一色。\n"
        "9. 清幺九不计双同刻，可计三同刻。\n"
        "10. 不重复原则特指单个番种与其他番种的必然包含关系，不适用某几个番种同时出现时与其他番种的包含关系。例如，绿一色+清一色，必然断幺，但要计断幺。\n"
        "11. 1明杠1暗杠计5番。暗杠的加计遵循国际麻将联盟（MIL）的规则，杠系列和暗刻系列最多各取一个番种计分。",
        10, nullptr, nullptr);
}
