#ifdef _MSC_VER
#pragma warning(disable: 4351)
#endif

#include "RecordScene.h"
#include "../common.h"
#include "../widget/AlertView.h"
#include "../widget/CWTableView.h"
#include "../widget/TilePickWidget.h"
#include "../widget/ExtraInfoWidget.h"

USING_NS_CC;

Scene *RecordScene::createScene(size_t handIdx, const char **playerNames, const Record::Detail *detail, const std::function<void (const Record::Detail &)> &okCallback) {
    auto scene = Scene::create();
    auto layer = new (std::nothrow) RecordScene();
    layer->initWithIndex(handIdx, playerNames, detail);
    layer->_okCallback = okCallback;
    layer->autorelease();

    scene->addChild(layer);
    return scene;
}

static const int fanLevel[] = { 4, 6, 8, 12, 16, 24, 32, 48, 64, 88 };
static const size_t eachLevelBeginIndex[] = { 55, 48, 39, 34, 28, 19, 16, 14, 8, 1 };
static const size_t eachLevelCounts[] = { 4, 7, 9, 5, 6, 9, 3, 2, 6, 7 };  // 各档次的番种的个数

static inline size_t computeRowsAlign4(size_t cnt) {
    return (cnt >> 2) + !!(cnt & 0x3);
}

bool RecordScene::initWithIndex(size_t handIdx, const char **playerNames, const Record::Detail *detail) {
    if (UNLIKELY(!BaseLayer::initWithTitle(handNameText[handIdx]))) {
        return false;
    }

    _handIdx = handIdx;
    _winIndex = -1;
    if (detail != nullptr) {
        memcpy(&_detail, detail, sizeof(_detail));
    }
    else {
        memset(&_detail, 0, sizeof(_detail));
    }

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    // 番数输入框
    _editBox = ui::EditBox::create(Size(34.0f, 20.0f), ui::Scale9Sprite::create("source_material/btn_square_normal.png"));
    this->addChild(_editBox);
    _editBox->setInputFlag(ui::EditBox::InputFlag::SENSITIVE);
    _editBox->setInputMode(ui::EditBox::InputMode::NUMERIC);
    _editBox->setFontColor(Color4B::BLACK);
    _editBox->setFontSize(12);
    _editBox->setText("8");
    _editBox->setPosition(Vec2(origin.x + 65.0f, origin.y + visibleSize.height - 50));
    _editBox->setDelegate(this);

    // +-按钮
    ui::Button *minusButton = ui::Button::create("source_material/stepper_dec_n.png", "source_material/stepper_dec_h.png");
    this->addChild(minusButton);
    minusButton->setScale(20.0f / minusButton->getContentSize().height);
    minusButton->setAnchorPoint(Vec2::ANCHOR_MIDDLE_RIGHT);
    minusButton->setPosition(Vec2(origin.x + 48.0f, origin.y + visibleSize.height - 50));
    minusButton->addClickEventListener(std::bind(&RecordScene::onMinusButton, this, std::placeholders::_1));

    ui::Button *plusButton = ui::Button::create("source_material/stepper_inc_n.png", "source_material/stepper_inc_h.png");
    this->addChild(plusButton);
    plusButton->setScale(20.0f / plusButton->getContentSize().height);
    plusButton->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    plusButton->setPosition(Vec2(origin.x + 82.0f, origin.y + visibleSize.height - 50));
    plusButton->addClickEventListener(std::bind(&RecordScene::onPlusButton, this, std::placeholders::_1));

    Label *label = Label::createWithSystemFont("番", "Arial", 12);
    label->setColor(Color3B::BLACK);
    this->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(origin.x + 120.0f, origin.y + visibleSize.height - 50));

    // 荒庄
    _drawBox = ui::CheckBox::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
    this->addChild(_drawBox);
    _drawBox->setZoomScale(0.0f);
    _drawBox->ignoreContentAdaptWithSize(false);
    _drawBox->setContentSize(Size(20.0f, 20.0f));
    _drawBox->setPosition(Vec2(origin.x + visibleSize.width - 60.0f, origin.y + visibleSize.height - 50));
    _drawBox->addEventListener(std::bind(&RecordScene::onDrawBox, this, std::placeholders::_1, std::placeholders::_2));

    label = Label::createWithSystemFont("荒庄", "Arial", 12);
    label->setColor(Color3B::BLACK);
    this->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(origin.x + visibleSize.width - 45.0f, origin.y + visibleSize.height - 50));

    _winGroup = ui::RadioButtonGroup::create();
    _winGroup->setAllowedNoSelection(true);
    this->addChild(_winGroup);
    _winGroup->addEventListener(std::bind(&RecordScene::onWinGroup, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    _claimGroup = ui::RadioButtonGroup::create();
    _claimGroup->setAllowedNoSelection(true);
    this->addChild(_claimGroup);
    _claimGroup->addEventListener(std::bind(&RecordScene::onClaimGroup, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    const float gap = (visibleSize.width - 4.0f) * 0.25f;
    for (int i = 0; i < 4; ++i) {
        const float x = origin.x + gap * (i + 0.5f);

        // 名字
        Label *nameLabel = Label::createWithSystemFont(playerNames[i], "Arial", 12);
        nameLabel->setColor(Color3B::ORANGE);
        this->addChild(nameLabel);
        nameLabel->setPosition(Vec2(x, origin.y + visibleSize.height - 80));
        scaleLabelToFitWidth(nameLabel, gap - 4);

        // 得分
        _scoreLabel[i] = Label::createWithSystemFont("+0", "Arial", 12);
        _scoreLabel[i]->setColor(Color3B(0x60, 0x60, 0x60));
        this->addChild(_scoreLabel[i]);
        _scoreLabel[i]->setPosition(Vec2(x, origin.y + visibleSize.height - 100));

        // 和
        ui::RadioButton *button = ui::RadioButton::create("source_material/btn_square_normal.png", "", "source_material/btn_square_highlighted.png",
            "source_material/btn_square_disabled.png", "source_material/btn_square_disabled.png");
        this->addChild(button);
        button->setZoomScale(0.0f);
        button->ignoreContentAdaptWithSize(false);
        button->setContentSize(Size(20.0f, 20.0f));
        button->setPosition(Vec2(x - 15, origin.y + visibleSize.height - 125));
        _winGroup->addRadioButton(button);

        label = Label::createWithSystemFont("和", "Arial", 12);
        label->setColor(Color3B::BLACK);
        this->addChild(label);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        label->setPosition(Vec2(x, origin.y + visibleSize.height - 125));

        // 点炮或自摸
        button = ui::RadioButton::create("source_material/btn_square_normal.png", "", "source_material/btn_square_highlighted.png",
            "source_material/btn_square_disabled.png", "source_material/btn_square_disabled.png");
        this->addChild(button);
        button->setZoomScale(0.0f);
        button->ignoreContentAdaptWithSize(false);
        button->setContentSize(Size(20.0f, 20.0f));
        button->setPosition(Vec2(x - 15, origin.y + visibleSize.height - 155));
        _claimGroup->addRadioButton(button);

        label = Label::createWithSystemFont("点炮", "Arial", 12);
        label->setColor(Color3B::BLACK);
        this->addChild(label);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        label->setPosition(Vec2(x, origin.y + visibleSize.height - 155));
        _byDiscardLabel[i] = label;

        label = Label::createWithSystemFont("自摸", "Arial", 12);
        label->setColor(Color3B::BLACK);
        this->addChild(label);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        label->setPosition(Vec2(x, origin.y + visibleSize.height - 155));
        _selfDrawnLabel[i] = label;
        label->setVisible(false);

        // 错和
        _falseWinBox[i] = ui::CheckBox::create("source_material/btn_square_normal.png", "", "source_material/btn_square_highlighted.png",
            "source_material/btn_square_disabled.png", "source_material/btn_square_disabled.png");
        this->addChild(_falseWinBox[i]);
        _falseWinBox[i]->setZoomScale(0.0f);
        _falseWinBox[i]->ignoreContentAdaptWithSize(false);
        _falseWinBox[i]->setContentSize(Size(20.0f, 20.0f));
        _falseWinBox[i]->setPosition(Vec2(x - 15, origin.y + visibleSize.height - 185));
        _falseWinBox[i]->addEventListener(std::bind(&RecordScene::onFalseWinBox, this, std::placeholders::_1, std::placeholders::_2));

        label = Label::createWithSystemFont("错和", "Arial", 12);
        label->setColor(Color3B::BLACK);
        this->addChild(label);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        label->setPosition(Vec2(x, origin.y + visibleSize.height - 185));
    }

    // 根结点
    ui::Layout *rootLayout = ui::Layout::create();
    rootLayout->setBackGroundColorType(ui::Layout::BackGroundColorType::SOLID);
    rootLayout->setBackGroundColor(Color3B(245, 245, 245));
    this->addChild(rootLayout);
    rootLayout->setPosition(Vec2(origin.x, origin.y + 35));
    rootLayout->setTouchEnabled(true);

    // 说明
    Label *maskLabel1 = Label::createWithSystemFont("标记4番以上番种（未做排斥检测）", "Arial", 12);
    maskLabel1->setColor(Color3B::BLACK);
    rootLayout->addChild(maskLabel1);
    maskLabel1->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);

    Label *maskLabel2 = Label::createWithSystemFont("标记番种可快速增加番数，取消标记不减少。\n微调番数可按两侧的+/-，亦可直接输入", "Arial", 10);
    maskLabel2->setColor(Color3B(0x60, 0x60, 0x60));
    rootLayout->addChild(maskLabel2);
    maskLabel2->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);

    // 展开/收起
    ui::Button *spreadButton = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
    spreadButton->setScale9Enabled(true);
    spreadButton->setContentSize(Size(55.0f, 20.0f));
    spreadButton->setTitleFontSize(12);
    rootLayout->addChild(spreadButton);

    // 输入牌按钮
    ui::Button *tilesButton = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
    tilesButton->setScale9Enabled(true);
    tilesButton->setContentSize(Size(55.0f, 20.0f));
    tilesButton->setTitleFontSize(12);
    tilesButton->setTitleText("记录和牌");
    tilesButton->addClickEventListener(std::bind(&RecordScene::onTilesButton, this, std::placeholders::_1));
    rootLayout->addChild(tilesButton);

    cw::TableView *tableView = cw::TableView::create();
    tableView->setDelegate(this);
    tableView->setDirection(ui::ScrollView::Direction::VERTICAL);
    tableView->setVerticalFillOrder(cw::TableView::VerticalFillOrder::TOP_DOWN);

    tableView->setScrollBarPositionFromCorner(Vec2(5, 5));
    tableView->setAnchorPoint(Vec2::ANCHOR_MIDDLE_BOTTOM);
    tableView->setPosition(Vec2(visibleSize.width * 0.5f, 0));
    rootLayout->addChild(tableView);
    _tableView = tableView;

    std::function<void (Ref *)> layoutChildren = [rootLayout, maskLabel1, maskLabel2, tilesButton, tableView](Ref *sender) {
        ui::Button *spreadButton = (ui::Button *)sender;

        Size visibleSize = Director::getInstance()->getVisibleSize();
        Size layoutSize;
        layoutSize.width = visibleSize.width;
        if (spreadButton->getUserData()) {
            layoutSize.height = visibleSize.height - 150;
            spreadButton->setUserData(reinterpret_cast<void *>(false));
            spreadButton->setTitleText("收起列表");
        }
        else {
            layoutSize.height = visibleSize.height - 240;
            spreadButton->setUserData(reinterpret_cast<void *>(true));
            spreadButton->setTitleText("展开列表");
        }

        rootLayout->setContentSize(layoutSize);
        maskLabel1->setPosition(Vec2(5.0f, layoutSize.height - 10));
        maskLabel2->setPosition(Vec2(5.0f, layoutSize.height - 35));
        spreadButton->setPosition(Vec2(visibleSize.width - 35.0f, layoutSize.height - 10));
        tilesButton->setPosition(Vec2(visibleSize.width - 35.0f, layoutSize.height - 35));
        tableView->setContentSize(Size(visibleSize.width - 10, layoutSize.height - 55));
        tableView->reloadData();
    };
    layoutChildren(spreadButton);
    spreadButton->addClickEventListener(layoutChildren);

    // 确定按钮
    _okButton = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png", "source_material/btn_square_disabled.png");
    this->addChild(_okButton);
    _okButton->setScale9Enabled(true);
    _okButton->setContentSize(Size(50.0f, 20.0f));
    _okButton->setTitleFontSize(12);
    _okButton->setTitleColor(Color3B::BLACK);
    _okButton->setTitleText("确定");
    _okButton->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + 15));
    _okButton->addClickEventListener(std::bind(&RecordScene::onOkButton, this, std::placeholders::_1));
    _okButton->setEnabled(false);

    if (detail != nullptr) {
        refresh();
    }
    return true;
}

ssize_t RecordScene::numberOfCellsInTableView(cw::TableView *table) {
    return 10;
}

cocos2d::Size RecordScene::tableCellSizeForIndex(cw::TableView *table, ssize_t idx) {
    size_t cnt = eachLevelCounts[idx];
    float height = computeRowsAlign4(cnt) * 25.0f;
    return Size(0, height + 15.0f);
}

cw::TableViewCell *RecordScene::tableCellAtIndex(cw::TableView *table, ssize_t idx) {
    typedef cw::TableViewCellEx<Label *, ui::Button *[9]> CustomCell;
    CustomCell *cell = (CustomCell *)table->dequeueCell();

    Size visibleSize = Director::getInstance()->getVisibleSize();
    const float gap = (visibleSize.width - 10.0f) * 0.25f;

    if (cell == nullptr) {
        cell = CustomCell::create();

        CustomCell::ExtDataType &ext = cell->getExtData();
        Label *&label = std::get<0>(ext);
        ui::Button *(&buttons)[9] = std::get<1>(ext);

        label = Label::createWithSystemFont("1番", "Arial", 12);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        cell->addChild(label);
        label->setColor(Color3B::BLACK);

        for (size_t k = 0; k < 9; ++k) {
            ui::Button *button = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
            button->setScale9Enabled(true);
            button->setContentSize(Size(gap - 5.0f, 20.0f));
            button->setTitleColor(Color3B::BLACK);
            button->setTitleFontSize(12);
            button->addClickEventListener(std::bind(&RecordScene::onPointsNameButton, this, std::placeholders::_1));

            cell->addChild(button);
            buttons[k] = button;
        }
    }

    const size_t currentLevelCount = eachLevelCounts[idx];
    size_t totalRows = computeRowsAlign4(currentLevelCount);

    const CustomCell::ExtDataType &ext = cell->getExtData();
    Label *label = std::get<0>(ext);
    ui::Button *const (&buttons)[9] = std::get<1>(ext);

    for (size_t k = 0; k < 9; ++k) {
        ui::Button *button = buttons[k];
        button->setVisible(false);
        button->setEnabled(false);
        button->setHighlighted(false);
        button->setTag(false);
    }

    label->setString(StringUtils::format("%d番", fanLevel[idx]));
    label->setPosition(Vec2(5.0f, totalRows * 25.0f + 7.0f));

    for (size_t k = 0; k < currentLevelCount; ++k) {
        ui::Button *button = buttons[k];
        size_t idx0 = eachLevelBeginIndex[idx] + k;
        button->setTitleText(mahjong::fan_name[idx0]);
        button->setUserData(reinterpret_cast<void *>(idx0));
        button->setVisible(true);
        button->setEnabled(true);

        size_t col = k & 0x3;
        size_t row = k >> 2;
        button->setPosition(Vec2(gap * (col + 0.5f), (totalRows - row - 0.5f) * 25.0f));

        if (TEST_FAN(_detail.fan_flag, idx0)) {
            button->setHighlighted(true);
            button->setTag(true);
        }

        scaleLabelToFitWidth(button->getTitleLabel(), gap - 10.0f);
    }

    return cell;
}

void RecordScene::editBoxReturn(cocos2d::ui::EditBox *editBox) {
    updateScoreLabel();
}

void RecordScene::refresh() {
    int wc = _detail.win_claim;
    if (_detail.score >= 8) {
        char str[32];
        snprintf(str, sizeof(str), "%d", _detail.score);
        _editBox->setText(str);
    }

    _winIndex = WIN_INDEX(wc);
    if (_winIndex != -1) {  // 有人和牌
        int claimIndex = CLAIM_INDEX(wc);  // 点炮者
        _winGroup->setSelectedButton(_winIndex);
        if (claimIndex != -1) {
            _claimGroup->setSelectedButton(claimIndex);
        }
    }

    // 错和
    if (_detail.false_win != 0) {
        for (int i = 0; i < 4; ++i) {
            _falseWinBox[i]->setSelected(TEST_FALSE_WIN(_detail.false_win, i));
        }
    }

    if (_detail.score == 0) {
        _drawBox->setSelected(true);
        onDrawBox(_drawBox, ui::CheckBox::EventType::SELECTED);
    }
    else {
        updateScoreLabel();
    }

    _tableView->reloadDataInplacement();
}

void RecordScene::updateScoreLabel() {
    _detail.win_claim = 0;
    int claimIndex = -1;
    if (_winIndex != -1) {  // 有人和牌
        int winScore = atoi(_editBox->getText());  // 获取输入框里所填番数
        _detail.score = std::max(8, winScore);
        claimIndex = _claimGroup->getSelectedButtonIndex();

        // 记录和牌和点炮
        _detail.win_claim = 0;
        SET_WIN(_detail.win_claim, _winIndex);
        if (claimIndex != -1) {
            SET_CLAIM(_detail.win_claim, claimIndex);
        }
    }
    else {  // 荒庄
        _detail.score = 0;
    }

    // 检查是否有错和
    _detail.false_win = 0;
    for (int i = 0; i < 4; ++i) {
        if (_falseWinBox[i]->isEnabled() && _falseWinBox[i]->isSelected()) {
            SET_FALSE_WIN(_detail.false_win, i);
        }
    }

    int scoreTable[4];
    translateDetailToScoreTable(_detail, scoreTable);

    // 正负0分使用不同颜色
    for (int i = 0; i < 4; ++i) {
        _scoreLabel[i]->setString(StringUtils::format("%+d", scoreTable[i]));
        if (scoreTable[i] > 0) {
            _scoreLabel[i]->setColor(Color3B(224, 45, 45));
        }
        else if (scoreTable[i] < 0) {
            _scoreLabel[i]->setColor(Color3B(37, 153, 14));
        }
        else {
            _scoreLabel[i]->setColor(Color3B(0x60, 0x60, 0x60));
        }
    }

    // 四位选手的总分加起来和为0
    if (scoreTable[0] + scoreTable[1] + scoreTable[2] + scoreTable[3] == 0) {
        if (_drawBox->isSelected()) {  // 荒庄
            _okButton->setEnabled(true);
        }
        else {
            _okButton->setEnabled(_winGroup->getSelectedButtonIndex() != -1);
        }
    }
    else {
        // 四位选手的总分加起来和不为0说明还没有选择是自摸还是点炮
        _okButton->setEnabled(false);
    }
}

void RecordScene::onMinusButton(cocos2d::Ref *sender) {
    int winScore = atoi(_editBox->getText());
    if (winScore > 8) {
        --winScore;
        _editBox->setText(StringUtils::format("%d", winScore).c_str());
        updateScoreLabel();
    }
}

void RecordScene::onPlusButton(cocos2d::Ref *sender) {
    int winScore = atoi(_editBox->getText());
    ++winScore;
    _editBox->setText(StringUtils::format("%d", winScore).c_str());
    updateScoreLabel();
}

void RecordScene::_WinHandToCalculateParam(const Record::Detail::WinHand &winHand, RecordScene::CalculateParam &param) {
    memset(&param, 0, sizeof(CalculateParam));
    std::copy(winHand.fixed_packs, winHand.fixed_packs + winHand.pack_count,
        param.hand_tiles.fixed_packs);
    param.hand_tiles.pack_count = winHand.pack_count;
    std::copy(winHand.standing_tiles, winHand.standing_tiles + winHand.tile_count,
        param.hand_tiles.standing_tiles);
    param.hand_tiles.tile_count = winHand.tile_count;
    param.win_tile = winHand.win_tile;
    param.flower_count = winHand.flower_count;
    param.ext_cond.win_flag = winHand.win_flag;
}

void RecordScene::_CalculateParamToWinHand(const RecordScene::CalculateParam &param, Record::Detail::WinHand &winHand) {
    memset(&winHand, 0, sizeof(Record::Detail::WinHand));
    std::copy(param.hand_tiles.fixed_packs, param.hand_tiles.fixed_packs + param.hand_tiles.pack_count,
        winHand.fixed_packs);
    winHand.pack_count = param.hand_tiles.pack_count;
    std::copy(param.hand_tiles.standing_tiles, param.hand_tiles.standing_tiles + param.hand_tiles.tile_count,
        winHand.standing_tiles);
    winHand.tile_count = param.hand_tiles.tile_count;
    winHand.win_tile = param.win_tile;
    winHand.flower_count = param.flower_count;
    winHand.win_flag = param.ext_cond.win_flag;
}

void RecordScene::onTilesButton(cocos2d::Ref *sender) {
    CalculateParam param;
    _WinHandToCalculateParam(_detail.win_hand, param);
    showCalculator(param);
}

void RecordScene::onDrawBox(cocos2d::Ref *sender, cocos2d::ui::CheckBox::EventType event) {
    if (event == ui::CheckBox::EventType::SELECTED) {
        _winIndex = -1;
        // 禁用所有人的和、自摸/点炮，启用错和
        for (int i = 0; i < 4; ++i) {
            ui::RadioButton *button = _winGroup->getRadioButtonByIndex(i);
            button->setEnabled(false);
            button = _claimGroup->getRadioButtonByIndex(i);
            button->setEnabled(false);
            _falseWinBox[i]->setEnabled(true);
        }
    }
    else {
        _winIndex = _winGroup->getSelectedButtonIndex();
        // 启用所有人的和、自摸/点炮、错和
        for (int i = 0; i < 4; ++i) {
            ui::RadioButton *button = _winGroup->getRadioButtonByIndex(i);
            button->setEnabled(true);
            button = _claimGroup->getRadioButtonByIndex(i);
            button->setEnabled(true);
            _falseWinBox[i]->setEnabled(i != _winIndex);
        }
    }
    updateScoreLabel();
}

void RecordScene::onWinGroup(cocos2d::ui::RadioButton *radioButton, int index, cocos2d::ui::RadioButtonGroup::EventType event) {
    _winIndex = index;
    for (int i = 0; i < 4; ++i) {
        if (i == index) {
            // 和的选手，显示自摸，禁用错和
            _byDiscardLabel[i]->setVisible(false);
            _selfDrawnLabel[i]->setVisible(true);
            _falseWinBox[i]->setEnabled(false);
        }
        else {
            // 未和的选手，显示点炮，启用错和
            _byDiscardLabel[i]->setVisible(true);
            _selfDrawnLabel[i]->setVisible(false);
            _falseWinBox[i]->setEnabled(true);
        }
    }
    updateScoreLabel();
}

void RecordScene::onClaimGroup(cocos2d::ui::RadioButton *radioButton, int index, cocos2d::ui::RadioButtonGroup::EventType event) {
    if (_winIndex == -1) return;
    updateScoreLabel();
}

void RecordScene::onFalseWinBox(cocos2d::Ref *sender, cocos2d::ui::CheckBox::EventType event) {
    updateScoreLabel();
}

void RecordScene::onPointsNameButton(cocos2d::Ref *sender) {
    ui::Button *button = (ui::Button *)sender;
    size_t index = reinterpret_cast<size_t>(button->getUserData());

    // 标记/取消标记番种
    bool selected = !!button->getTag();
    if (selected) {
        button->setHighlighted(false);
        button->setTag(false);
        RESET_FAN(_detail.fan_flag, index);
    }
    else {
        button->setHighlighted(true);
        button->setTag(true);
        SET_FAN(_detail.fan_flag, index);
    }

    // 增加番数
    int prevWinScore = atoi(_editBox->getText());
    int currentWinScore = 0;
    for (int n = mahjong::BIG_FOUR_WINDS; n < mahjong::DRAGON_PUNG; ++n) {
        if (TEST_FAN(_detail.fan_flag, n)) {
            unsigned idx = n;
            currentWinScore += mahjong::fan_value_table[idx];
        }
    }
    currentWinScore = std::max(8, currentWinScore);
    if (currentWinScore > prevWinScore) {
        char str[16];
        snprintf(str, sizeof(str), "%d", currentWinScore);
        _editBox->setText(str);
    }
    updateScoreLabel();
}

void RecordScene::onOkButton(cocos2d::Ref *sender) {
    if (_drawBox->isSelected() && _detail.fan_flag != 0) {
        AlertView::showWithMessage("记分", "你标记了番种却选择了荒庄，是否忽略标记这些番种，记录本盘为荒庄？", [this]() {
            _detail.fan_flag = 0;
            _okCallback(_detail);
            Director::getInstance()->popScene();
        }, nullptr);
        return;
    }

    _okCallback(_detail);
    Director::getInstance()->popScene();
}

static const mahjong::wind_t seatWindTable[16][4] = {
    { mahjong::wind_t::EAST, mahjong::wind_t::SOUTH, mahjong::wind_t::WEST, mahjong::wind_t::NORTH },
    { mahjong::wind_t::NORTH, mahjong::wind_t::EAST, mahjong::wind_t::SOUTH, mahjong::wind_t::WEST },
    { mahjong::wind_t::WEST, mahjong::wind_t::NORTH, mahjong::wind_t::EAST, mahjong::wind_t::SOUTH },
    { mahjong::wind_t::SOUTH, mahjong::wind_t::WEST, mahjong::wind_t::NORTH, mahjong::wind_t::EAST },

    { mahjong::wind_t::SOUTH, mahjong::wind_t::EAST, mahjong::wind_t::NORTH, mahjong::wind_t::WEST },
    { mahjong::wind_t::EAST, mahjong::wind_t::NORTH, mahjong::wind_t::WEST, mahjong::wind_t::SOUTH },
    { mahjong::wind_t::NORTH, mahjong::wind_t::WEST, mahjong::wind_t::SOUTH, mahjong::wind_t::EAST },
    { mahjong::wind_t::WEST, mahjong::wind_t::SOUTH, mahjong::wind_t::EAST, mahjong::wind_t::NORTH },

    { mahjong::wind_t::NORTH, mahjong::wind_t::WEST, mahjong::wind_t::EAST, mahjong::wind_t::SOUTH },
    { mahjong::wind_t::WEST, mahjong::wind_t::SOUTH, mahjong::wind_t::NORTH, mahjong::wind_t::EAST },
    { mahjong::wind_t::SOUTH, mahjong::wind_t::EAST, mahjong::wind_t::WEST, mahjong::wind_t::NORTH },
    { mahjong::wind_t::EAST, mahjong::wind_t::NORTH, mahjong::wind_t::SOUTH, mahjong::wind_t::WEST },

    { mahjong::wind_t::WEST, mahjong::wind_t::NORTH, mahjong::wind_t::SOUTH, mahjong::wind_t::EAST },
    { mahjong::wind_t::SOUTH, mahjong::wind_t::WEST, mahjong::wind_t::EAST, mahjong::wind_t::NORTH },
    { mahjong::wind_t::EAST, mahjong::wind_t::SOUTH, mahjong::wind_t::NORTH, mahjong::wind_t::WEST },
    { mahjong::wind_t::NORTH, mahjong::wind_t::EAST, mahjong::wind_t::WEST, mahjong::wind_t::SOUTH }
};

void RecordScene::showCalculator(const CalculateParam &param) {
    Size visibleSize = Director::getInstance()->getVisibleSize();
    const float maxWidth = visibleSize.width * 0.8f - 10;

    // 选牌面板和其他信息的相关控件
    TilePickWidget *tilePicker = TilePickWidget::create();
    ExtraInfoWidget *extraInfo = ExtraInfoWidget::create();

    extraInfo->setFlowerCount(param.flower_count);
    extraInfo->setWinFlag(param.ext_cond.win_flag);

    // 设置圈风
    extraInfo->setPrevalentWind(static_cast<mahjong::wind_t>(_handIdx / 4));

    int winIndex = _winGroup->getSelectedButtonIndex();
    int claimIndex = _claimGroup->getSelectedButtonIndex();
    if (winIndex != -1) {  // 有人和牌
        // 设置门风
        extraInfo->setSeatWind(seatWindTable[_handIdx][winIndex]);

        // 自摸
        extraInfo->setWinFlag(claimIndex != winIndex ? WIN_FLAG_DISCARD : WIN_FLAG_SELF_DRAWN);
    }

    // 设置回调函数
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

    extraInfo->setParseCallback(std::bind(&TilePickWidget::setData, tilePicker, std::placeholders::_1, std::placeholders::_2));

    // 缩放
    Size tilePickerSize = tilePicker->getContentSize();
    const float tilePickerScale = maxWidth / tilePickerSize.width;
    tilePicker->setScale(tilePickerScale);
    tilePickerSize = Size(maxWidth, tilePickerSize.height * tilePickerScale);

    Size extraInfoSize = extraInfo->getContentSize();
    const float extraInfoScale = maxWidth / extraInfoSize.width;
    extraInfo->setScale(extraInfoScale);
    extraInfoSize = Size(maxWidth, extraInfoSize.height * extraInfoScale);

    // 布局在rootWidget上
    ui::Widget *rootWidget = ui::Widget::create();
    rootWidget->setContentSize(Size(maxWidth, tilePickerSize.height + extraInfoSize.height + 5));
    rootWidget->addChild(tilePicker);
    tilePicker->setPosition(Vec2(maxWidth * 0.5f, tilePickerSize.height * 0.5f + extraInfoSize.height + 5));
    rootWidget->addChild(extraInfo);
    extraInfo->setPosition(Vec2(maxWidth * 0.5f, extraInfoSize.height * 0.5f));

    if (param.hand_tiles.tile_count != 0 && param.win_tile != 0) {
        tilePicker->setData(param.hand_tiles, param.win_tile);
    }

    // 通过AlertView显示出来
    AlertView::showWithNode("记录和牌", rootWidget,
        std::bind(&RecordScene::calculate, this, tilePicker, extraInfo, param), nullptr);
}

// in FanCalculatorScene.cpp
cocos2d::Node *createFanResultNode(const long (&fan_table)[mahjong::FAN_TABLE_SIZE], int fontSize, float resultAreaWidth);

#define FONT_SIZE 12

void RecordScene::calculate(TilePickWidget *tilePicker, ExtraInfoWidget *extraInfo, const CalculateParam &param) {
    CalculateParam temp = { 0 };
    temp.flower_count = extraInfo->getFlowerCount();
    if (temp.flower_count > 8) {
        AlertView::showWithMessage("算番", "花牌数的范围为0~8", std::bind(&RecordScene::showCalculator, this, param), nullptr);
        return;
    }

    tilePicker->getData(&temp.hand_tiles, &temp.win_tile);
    if (temp.win_tile == 0) {
        AlertView::showWithMessage("算番", "缺少和牌张", std::bind(&RecordScene::showCalculator, this, temp), nullptr);
        return;
    }

    std::sort(temp.hand_tiles.standing_tiles, temp.hand_tiles.standing_tiles + temp.hand_tiles.tile_count);

    long fan_table[mahjong::FAN_TABLE_SIZE] = { 0 };

    // 获取绝张、杠开、抢杠、海底信息
    temp.ext_cond.win_flag = extraInfo->getWinFlag();

    // 获取圈风门风
    temp.ext_cond.prevalent_wind = extraInfo->getPrevalentWind();
    temp.ext_cond.seat_wind = extraInfo->getSeatWind();

    // 算番
    int fan = calculate_fan(&temp.hand_tiles, temp.win_tile, &temp.ext_cond, fan_table);

    if (fan == ERROR_NOT_WIN) {
        AlertView::showWithMessage("算番", "诈和", std::bind(&RecordScene::showCalculator, this, temp), nullptr);
        return;
    }
    if (fan == ERROR_WRONG_TILES_COUNT) {
        AlertView::showWithMessage("算番", "牌张数错误", std::bind(&RecordScene::showCalculator, this, temp), nullptr);
        return;
    }
    if (fan == ERROR_TILE_COUNT_GREATER_THAN_4) {
        AlertView::showWithMessage("算番", "同一种牌最多只能使用4枚", std::bind(&RecordScene::showCalculator, this, temp), nullptr);
        return;
    }

    // 加花牌
    fan += temp.flower_count;
    fan_table[mahjong::FLOWER_TILES] = temp.flower_count;

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Node *innerNode = createFanResultNode(fan_table, 12, visibleSize.width);
    const Size &fanResultSize = innerNode->getContentSize();

    // 手牌
    Node *tilesNode = HandTilesWidget::createStaticNode(temp.hand_tiles, temp.win_tile);
    Size tilesNodeSize = tilesNode->getContentSize();
    if (tilesNodeSize.width > visibleSize.width) {
        const float scale = visibleSize.width / tilesNodeSize.width;
        tilesNode->setScale(scale);
        tilesNodeSize.width = visibleSize.width;
        tilesNodeSize.height *= scale;
    }
    innerNode->addChild(tilesNode);
    tilesNode->setPosition(Vec2(visibleSize.width * 0.5f, fanResultSize.height + 5 + tilesNodeSize.height * 0.5f));
    innerNode->setContentSize(Size(visibleSize.width, fanResultSize.height + 5 + tilesNodeSize.height));

    uint64_t fanFlag = 0;
    for (int n = mahjong::BIG_FOUR_WINDS; n < mahjong::DRAGON_PUNG; ++n) {
        if (fan_table[n]) {
            SET_FAN(fanFlag, n);
        }
    }

    AlertView::showWithNode("算番", innerNode, [this, temp, fan, fanFlag]() {
        _detail.score = std::max(fan, 8);
        _detail.fan_flag = fanFlag;

        int winIndex = _winGroup->getSelectedButtonIndex();
        int claimIndex = _claimGroup->getSelectedButtonIndex();
        if (winIndex != -1) {  // 有人和牌
            if (claimIndex != winIndex) {
                if (temp.ext_cond.win_flag & WIN_FLAG_SELF_DRAWN) {  // 自摸
                    _claimGroup->setSelectedButton(winIndex);
                }
            }
            else {
                //if (ext_cond.win_flag & WIN_FLAG_SELF_DRAWN) {  // 点和
                //    _claimGroup->setSelectedButton(-1);
                //}
            }
        }

        _CalculateParamToWinHand(temp, _detail.win_hand);
        refresh();
    }, std::bind(&RecordScene::showCalculator, this, temp));
}
