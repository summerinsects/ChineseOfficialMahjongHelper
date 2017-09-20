#include "RecordScene.h"
#include <array>
#include "../mahjong-algorithm/fan_calculator.h"
#include "../widget/AlertView.h"
#include "../widget/CWTableView.h"

USING_NS_CC;

static const int fanLevel[] = { 4, 6, 8, 12, 16, 24, 32, 48, 64, 88 };
static const size_t eachLevelBeginIndex[] = { 55, 48, 39, 34, 28, 19, 16, 14, 8, 1 };
static const size_t eachLevelCounts[] = { 4, 7, 9, 5, 6, 9, 3, 2, 6, 7 };  // 各档次的番种的个数

static inline size_t computeRowsAlign4(size_t cnt) {
    return (cnt >> 2) + !!(cnt & 0x3);
}

#define ORDER(flag_, i_) (((flag_) >> ((i_) << 1)) & 0x3)

bool RecordScene::initWithIndex(size_t handIdx, const char **playerNames, const Record::Detail *detail, const OkCallback &callback) {
    if (UNLIKELY(!BaseScene::initWithTitle(handNameText[handIdx]))) {
        return false;
    }

    _okCallback = callback;

    switch (handIdx >> 2) {
    default:
        _seatFlag = 0xE4;  //3210
        _playerFlag = 0xE4;  // 3210
        break;
    case 1:
        _seatFlag = 0xB1;  // 2301
        _playerFlag = 0xB1;  // 2301
        break;
    case 2:
        _seatFlag = 0x1E;  // 0132
        _playerFlag = 0x4B;  // 1023
        break;
    case 3:
        _seatFlag = 0x4B;  // 1023
        _playerFlag = 0x1E;  // 0132
        break;
    }

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
    ui::EditBox *editBox = ui::EditBox::create(Size(35.0f, 20.0f), ui::Scale9Sprite::create("source_material/btn_square_normal.png"));
    this->addChild(editBox);
    editBox->setInputFlag(ui::EditBox::InputFlag::SENSITIVE);
    editBox->setInputMode(ui::EditBox::InputMode::NUMERIC);
    editBox->setReturnType(ui::EditBox::KeyboardReturnType::DONE);
    editBox->setFontColor(Color4B::BLACK);
    editBox->setFontSize(12);
    editBox->setText("8");
    editBox->setPosition(Vec2(origin.x + 86.0f, origin.y + visibleSize.height - 50));
    editBox->setDelegate(this);
    _editBox = editBox;

    // 输入框同位置的按钮，以实现点击后清除内容的效果
    ui::Widget *widget = ui::Widget::create();
    editBox->addChild(widget);
    widget->setTouchEnabled(true);
    widget->setContentSize(Size(35.0f, 20.0f));
    widget->setPosition(Vec2(35.0f * 0.5f, 20.0f * 0.5f));
    widget->addClickEventListener([editBox](Ref *) {
        editBox->setPlaceHolder(editBox->getText());
        editBox->setText("");
        editBox->touchDownAction(editBox, ui::Widget::TouchEventType::ENDED);
    });

    // +-按钮
    ui::Button *button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(25.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText("-5");
    button->setPosition(Vec2(origin.x + 20.0f, origin.y + visibleSize.height - 50));
    button->addClickEventListener(std::bind(&RecordScene::onMinusButton, this, std::placeholders::_1, 5));

    button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(25.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText("-1");
    button->setPosition(Vec2(origin.x + 50.0f, origin.y + visibleSize.height - 50));
    button->addClickEventListener(std::bind(&RecordScene::onMinusButton, this, std::placeholders::_1, 1));

    button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(25.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText("+1");
    button->setPosition(Vec2(origin.x + 135.0f, origin.y + visibleSize.height - 50));
    button->addClickEventListener(std::bind(&RecordScene::onPlusButton, this, std::placeholders::_1, 1));

    button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(25.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText("+5");
    button->setPosition(Vec2(origin.x + 165.0f, origin.y + visibleSize.height - 50));
    button->addClickEventListener(std::bind(&RecordScene::onPlusButton, this, std::placeholders::_1, 5));

    Label *label = Label::createWithSystemFont("番", "Arial", 12);
    label->setColor(Color3B::BLACK);
    this->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(origin.x + 106.0f, origin.y + visibleSize.height - 50));

    // 荒庄
    ui::CheckBox *drawBox = ui::CheckBox::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
    this->addChild(drawBox);
    drawBox->setZoomScale(0.0f);
    drawBox->ignoreContentAdaptWithSize(false);
    drawBox->setContentSize(Size(20.0f, 20.0f));
    drawBox->setPosition(Vec2(origin.x + visibleSize.width - 50.0f, origin.y + visibleSize.height - 50));
    drawBox->addEventListener(std::bind(&RecordScene::onDrawBox, this, std::placeholders::_1, std::placeholders::_2));
    _drawBox = drawBox;

    label = Label::createWithSystemFont("荒庄", "Arial", 12);
    label->setColor(Color3B::BLACK);
    this->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(origin.x + visibleSize.width - 35.0f, origin.y + visibleSize.height - 50));

    // 说明文本
    label = Label::createWithSystemFont("番数支持直接输入。标记主番可快速增加番数，取消标记不减少。", "Arial", 10);
    label->setColor(Color3B(0x60, 0x60, 0x60));
    this->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(origin.x + 5, origin.y + visibleSize.height - 70));
    Common::scaleLabelToFitWidth(label, visibleSize.width - 10);

    ui::RadioButtonGroup *winGroup = ui::RadioButtonGroup::create();
    winGroup->setAllowedNoSelection(true);
    this->addChild(winGroup);
    winGroup->addEventListener(std::bind(&RecordScene::onWinGroup, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    ui::RadioButtonGroup *claimGroup = ui::RadioButtonGroup::create();
    claimGroup->setAllowedNoSelection(true);
    this->addChild(claimGroup);
    claimGroup->addEventListener(std::bind(&RecordScene::onClaimGroup, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    Node *radioNode = Node::create();
    this->addChild(radioNode);

    const float gap = (visibleSize.width - 4.0f) * 0.25f;
    for (int i = 0; i < 4; ++i) {
        const float x = origin.x + gap * (i + 0.5f);

        // 名字
        label = Label::createWithSystemFont(playerNames[ORDER(_seatFlag, i)], "Arial", 12);
        label->setColor(Color3B::ORANGE);
        this->addChild(label);
        label->setPosition(Vec2(x, origin.y + visibleSize.height - 90));
        Common::scaleLabelToFitWidth(label, gap - 4);

        // 得分
        label = Label::createWithSystemFont("+0", "Arial", 12);
        label->setColor(Color3B(0x60, 0x60, 0x60));
        this->addChild(label);
        label->setPosition(Vec2(x, origin.y + visibleSize.height - 110));
        _scoreLabel[i] = label;

        // 和
        ui::RadioButton *button = ui::RadioButton::create("source_material/btn_square_normal.png", "", "source_material/btn_square_highlighted.png",
            "source_material/btn_square_disabled.png", "source_material/btn_square_disabled.png");
        radioNode->addChild(button);
        button->setZoomScale(0.0f);
        button->ignoreContentAdaptWithSize(false);
        button->setContentSize(Size(20.0f, 20.0f));
        button->setPosition(Vec2(x - 15, origin.y + visibleSize.height - 135));
        winGroup->addRadioButton(button);

        label = Label::createWithSystemFont("和", "Arial", 12);
        label->setColor(Color3B::BLACK);
        radioNode->addChild(label);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        label->setPosition(Vec2(x, origin.y + visibleSize.height - 135));

        // 点炮或自摸
        button = ui::RadioButton::create("source_material/btn_square_normal.png", "", "source_material/btn_square_highlighted.png",
            "source_material/btn_square_disabled.png", "source_material/btn_square_disabled.png");
        radioNode->addChild(button);
        button->setZoomScale(0.0f);
        button->ignoreContentAdaptWithSize(false);
        button->setContentSize(Size(20.0f, 20.0f));
        button->setPosition(Vec2(x - 15, origin.y + visibleSize.height - 165));
        claimGroup->addRadioButton(button);

        label = Label::createWithSystemFont("点炮", "Arial", 12);
        label->setColor(Color3B::BLACK);
        radioNode->addChild(label);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        label->setPosition(Vec2(x, origin.y + visibleSize.height - 165));
        _byDiscardLabel[i] = label;

        label = Label::createWithSystemFont("自摸", "Arial", 12);
        label->setColor(Color3B::BLACK);
        radioNode->addChild(label);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        label->setPosition(Vec2(x, origin.y + visibleSize.height - 165));
        label->setVisible(false);
        _selfDrawnLabel[i] = label;

        // 错和
        ui::CheckBox *checkBox= ui::CheckBox::create("source_material/btn_square_normal.png", "", "source_material/btn_square_highlighted.png",
            "source_material/btn_square_disabled.png", "source_material/btn_square_disabled.png");
        radioNode->addChild(checkBox);
        checkBox->setZoomScale(0.0f);
        checkBox->ignoreContentAdaptWithSize(false);
        checkBox->setContentSize(Size(20.0f, 20.0f));
        checkBox->setPosition(Vec2(x - 15, origin.y + visibleSize.height - 195));
        checkBox->addEventListener(std::bind(&RecordScene::onFalseWinBox, this, std::placeholders::_1, std::placeholders::_2));
        _falseWinBox[i] = checkBox;

        label = Label::createWithSystemFont("错和", "Arial", 12);
        label->setColor(Color3B::BLACK);
        radioNode->addChild(label);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        label->setPosition(Vec2(x, origin.y + visibleSize.height - 195));
    }
    _winGroup = winGroup;
    _claimGroup = claimGroup;

    // 根结点
    ui::Layout *rootLayout = ui::Layout::create();
    this->addChild(rootLayout);
    rootLayout->setPosition(Vec2(origin.x, origin.y + 30));
    rootLayout->setTouchEnabled(true);

    // 上方所有东西
    Node *topNode = Node::create();
    topNode->setContentSize(Size(visibleSize.width, 45));
    topNode->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    topNode->setIgnoreAnchorPointForPosition(false);
    rootLayout->addChild(topNode);

    // 说明
    label = Label::createWithSystemFont("标记主番（4番以上，未做排斥检测）", "Arial", 12);
    label->setColor(Color3B::BLACK);
    topNode->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(5.0f, 35.0f));

    // 展开/收起
    ui::Button *layoutButton = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
    layoutButton->setScale9Enabled(true);
    layoutButton->setContentSize(Size(55.0f, 20.0f));
    layoutButton->setTitleFontSize(12);
    topNode->addChild(layoutButton);
    layoutButton->setPosition(Vec2(visibleSize.width - 35.0f, 35.0f));

    // 常用凑番
    button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
    button->setScale9Enabled(true);
    button->setContentSize(Size(55.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText("常用凑番");
    button->addClickEventListener([this](Ref *) { showPackedFanAlert(nullptr); });
    topNode->addChild(button);
    button->setPosition(Vec2(visibleSize.width - 35.0f, 10.0f));

    cw::TableView *tableView = cw::TableView::create();
    tableView->setDirection(ui::ScrollView::Direction::VERTICAL);
    tableView->setScrollBarPositionFromCorner(Vec2(2, 2));
    tableView->setScrollBarWidth(4);
    tableView->setScrollBarOpacity(0x99);
    tableView->setDelegate(this);
    tableView->setVerticalFillOrder(cw::TableView::VerticalFillOrder::TOP_DOWN);

    tableView->setAnchorPoint(Vec2::ANCHOR_MIDDLE_BOTTOM);
    tableView->setPosition(Vec2(visibleSize.width * 0.5f, 0));
    rootLayout->addChild(tableView);
    _tableView = tableView;

    std::function<void (Ref *)> onLayoutButton = [radioNode, rootLayout, topNode, tableView](Ref *sender) {
        ui::Button *layoutButton = (ui::Button *)sender;

        Size visibleSize = Director::getInstance()->getVisibleSize();
        Size layoutSize;
        layoutSize.width = visibleSize.width;
        if (layoutButton->getUserData()) {
            layoutSize.height = visibleSize.height - 140;
            layoutButton->setUserData(reinterpret_cast<void *>(false));
            layoutButton->setTitleText("\xE2\xAC\x87\xEF\xB8\x8E收起");
            radioNode->setVisible(false);
        }
        else {
            layoutSize.height = visibleSize.height - 230;
            layoutButton->setUserData(reinterpret_cast<void *>(true));
            layoutButton->setTitleText("\xE2\xAC\x86\xEF\xB8\x8E展开");
            radioNode->setVisible(true);
        }

        rootLayout->setContentSize(layoutSize);
        topNode->setPosition(Vec2(visibleSize.width * 0.5f, layoutSize.height - 35));
        tableView->setContentSize(Size(visibleSize.width - 5, layoutSize.height - 65));
        tableView->reloadData();
    };
    onLayoutButton(layoutButton);
    layoutButton->addClickEventListener(onLayoutButton);

    // 跳到
    label = Label::createWithSystemFont("跳到", "Arial", 12);
    label->setColor(Color3B::BLACK);
    topNode->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(5.0f, 10.0f));

    // 6 8 12 16 24
    const float labelPosX = label->getContentSize().width + 5 + 2;
    const char *text[] = { "6番", "8番", "12番", "16番", "24番" };
    for (size_t i = 0; i < 5; ++i) {
        button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
        button->setScale9Enabled(true);
        button->setContentSize(Size(30.0f, 20.0f));
        button->setTitleFontSize(12);
        button->setTitleText(text[i]);
        button->addClickEventListener([tableView, i](Ref *) { tableView->jumpToCell(i + 1); });
        topNode->addChild(button);
        button->setPosition(Vec2(labelPosX + 35.0f * (0.5f + i), 10.0f));
    }

    // 确定按钮
    button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png", "source_material/btn_square_disabled.png");
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(50.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText("确定");
    button->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + 15));
    button->addClickEventListener(std::bind(&RecordScene::onOkButton, this, std::placeholders::_1));
    button->setEnabled(false);
    _okButton = button;

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
    typedef cw::TableViewCellEx<Label *, std::array<ui::Button *, 9> > CustomCell;
    CustomCell *cell = (CustomCell *)table->dequeueCell();

    Size visibleSize = Director::getInstance()->getVisibleSize();
    const float gap = (visibleSize.width - 5.0f) * 0.25f;

    if (cell == nullptr) {
        cell = CustomCell::create();

        CustomCell::ExtDataType &ext = cell->getExtData();
        Label *&label = std::get<0>(ext);
        std::array<ui::Button *, 9> &buttons = std::get<1>(ext);

        label = Label::createWithSystemFont("1番", "Arial", 12);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        cell->addChild(label);
        label->setColor(Color3B::BLACK);

        for (size_t k = 0; k < 9; ++k) {
            ui::Button *button = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
            button->setScale9Enabled(true);
            button->setContentSize(Size(gap - 4.0f, 20.0f));
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
    const std::array<ui::Button *, 9> &buttons = std::get<1>(ext);

    for (size_t k = 0; k < 9; ++k) {
        ui::Button *button = buttons[k];
        button->setVisible(false);
        button->setEnabled(false);
        button->setHighlighted(false);
        button->setTag(false);
    }

    label->setString(std::to_string(fanLevel[idx]).append("番"));
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

        Common::scaleLabelToFitWidth(button->getTitleLabel(), gap - 8.0f);
    }

    return cell;
}

void RecordScene::editBoxReturn(cocos2d::ui::EditBox *editBox) {
    const char *text = _editBox->getText();
    if (*text == '\0') {
        const char *placeholder = _editBox->getPlaceHolder();
        _editBox->setText(placeholder);
        text = placeholder;
    }

    if (atoi(text) < 8) {
        _editBox->setText("8");
    }

    updateScoreLabel();
}

void RecordScene::refresh() {
    int wc = _detail.win_claim;
    if (_detail.score >= 8) {
        char str[32];
        snprintf(str, sizeof(str), "%d", _detail.score);
        _editBox->setText(str);
    }

    // 错和
    if (_detail.false_win != 0) {
        for (int i = 0; i < 4; ++i) {
            _falseWinBox[i]->setSelected(TEST_FALSE_WIN(_detail.false_win, ORDER(_playerFlag, i)));
        }
    }

    _winIndex = WIN_INDEX(wc);
    if (_winIndex != -1) {  // 有人和牌
        int claimIndex = CLAIM_INDEX(wc);  // 点炮者
        _winGroup->setSelectedButton(ORDER(_playerFlag, _winIndex));
        if (claimIndex != -1) {
            _claimGroup->setSelectedButton(ORDER(_playerFlag, claimIndex));
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

void RecordScene::_SetScoreLabelColor(cocos2d::Label *(&scoreLabel)[4], int (&scoreTable)[4], uint8_t seatFlag, uint8_t win_claim, uint8_t false_win) {
    for (int i = 0; i < 4; ++i) {
        if (scoreTable[i] != 0) {
            if (TEST_WIN(win_claim, i)) {  // 和牌：红色
                scoreLabel[ORDER(seatFlag, i)]->setColor(Color3B(254, 87, 110));
            }
            else {
                if (UNLIKELY(TEST_FALSE_WIN(false_win, i))) {  // 错和：紫色
                    scoreLabel[ORDER(seatFlag, i)]->setColor(Color3B(89, 16, 89));
                }
                else if (UNLIKELY(TEST_CLAIM(win_claim, i))) {  // 点炮：蓝色
                    scoreLabel[ORDER(seatFlag, i)]->setColor(Color3B(44, 121, 178));
                }
                else {  // 其他：绿色
                    scoreLabel[ORDER(seatFlag, i)]->setColor(Color3B(49, 155, 28));
                }
            }
        }
        else {
            scoreLabel[ORDER(seatFlag, i)]->setColor(Color3B(0x60, 0x60, 0x60));
        }
    }
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
        SET_WIN(_detail.win_claim, ORDER(_seatFlag, _winIndex));
        if (claimIndex != -1) {
            SET_CLAIM(_detail.win_claim, ORDER(_seatFlag, claimIndex));
        }
    }
    else {  // 荒庄
        _detail.score = 0;
    }

    // 检查是否有错和
    _detail.false_win = 0;
    for (int i = 0; i < 4; ++i) {
        if (_falseWinBox[i]->isEnabled() && _falseWinBox[i]->isSelected()) {
            SET_FALSE_WIN(_detail.false_win, ORDER(_seatFlag, i));
        }
    }

    int scoreTable[4];
    TranslateDetailToScoreTable(_detail, scoreTable);

    for (int i = 0; i < 4; ++i) {
        _scoreLabel[i]->setString(Common::format("%+d", scoreTable[ORDER(_seatFlag, i)]));
    }
    // 使用不同颜色
    _SetScoreLabelColor(_scoreLabel, scoreTable, _playerFlag, _detail.win_claim, _detail.false_win);

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
        // 四位选手的总分加起来和不为0，则说明还没有选择是自摸还是点炮
        _okButton->setEnabled(false);
    }
}

void RecordScene::onMinusButton(cocos2d::Ref *sender, int delta) {
    int winScore = atoi(_editBox->getText());
    int temp = winScore - delta;
    if (temp < 8) temp = 8;
    if (winScore != temp) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", temp);
        _editBox->setText(buf);
        updateScoreLabel();
    }
}

void RecordScene::onPlusButton(cocos2d::Ref *sender, int delta) {
    int winScore = atoi(_editBox->getText());
    winScore += delta;
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", winScore);
    _editBox->setText(buf);
    updateScoreLabel();
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

void RecordScene::showPackedFanAlert(const std::function<void ()> &callback) {
    Node *rootNode = Node::create();
    rootNode->setContentSize(Size(200.0f, 100.0f));

    ui::RadioButtonGroup *radioGroup = ui::RadioButtonGroup::create();
    radioGroup->setAllowedNoSelection(true);
    rootNode->addChild(radioGroup);

    const int totalRows = 4;  // 每行2个，共4行
    for (int i = 0; i < 8; ++i) {
        ui::RadioButton *radioButton = ui::RadioButton::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
        radioButton->setZoomScale(0.0f);
        radioButton->ignoreContentAdaptWithSize(false);
        radioButton->setContentSize(Size(20.0f, 20.0f));
        radioButton->setPosition(Vec2(10.0f + 100.0f * (i & 1), (totalRows - (i >> 1) - 0.5f) * 25.0f));
        rootNode->addChild(radioButton);
        radioGroup->addRadioButton(radioButton);

        Label *label = Label::createWithSystemFont(GetPackedFanText(i + 1), "Arial", 12);
        label->setColor(Color3B::BLACK);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        radioButton->addChild(label);
        label->setPosition(Vec2(25.0f, 10.0f));

        // 这一段是实现RadioButton可以取消选中的
        radioButton->addClickEventListener([radioGroup](Ref *sender) {
            ui::RadioButton *radioButton = (ui::RadioButton *)sender;
            if (radioButton->isSelected()) {
                // 需要在下一帧调用
                radioGroup->scheduleOnce([radioGroup](float) { radioGroup->setSelectedButton(nullptr); }, 0.0f, "deselect");
            }
        });
    }

    uint8_t packedFan = _detail.packed_fan;
    if (packedFan > 0 && packedFan <= 8) {
        radioGroup->setSelectedButtonWithoutEvent(packedFan - 1);
    }

    AlertView::showWithNode("选择常用凑番", rootNode, [this, radioGroup, callback]() {
        int highlight = radioGroup->getSelectedButtonIndex();
        _detail.packed_fan = static_cast<uint8_t>(highlight + 1);
        if (callback) {
            callback();
        }
    }, nullptr);
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

    // 计算番数
    int prevWinScore = atoi(_editBox->getText());
    int currentWinScore = 0;
    for (int n = mahjong::BIG_FOUR_WINDS; n < mahjong::DRAGON_PUNG; ++n) {
        if (TEST_FAN(_detail.fan_flag, n)) {
            unsigned idx = n;
            currentWinScore += mahjong::fan_value_table[idx];
        }
    }
    currentWinScore = std::max(8, currentWinScore);

    // 增加番数
    if (currentWinScore > prevWinScore) {
        char str[16];
        snprintf(str, sizeof(str), "%d", currentWinScore);
        _editBox->setText(str);
    }
    updateScoreLabel();
}

void RecordScene::onOkButton(cocos2d::Ref *sender) {
    if (_detail.fan_flag != 0) {  // 标记了番种
        if (_drawBox->isSelected()) {  // 荒庄
            AlertView::showWithMessage("记分", "你标记了番种却选择了荒庄，是否忽略标记这些番种，记录本盘为荒庄？", 12,
                [this]() {
                _detail.fan_flag = 0;
                _detail.packed_fan = 0;
                _okCallback(_detail);
                Director::getInstance()->popScene();
            }, nullptr);
        }
        else {
            _okCallback(_detail);
            Director::getInstance()->popScene();
        }
    }
    else {  // 未标记番种
        if (_winIndex != -1) {  // 有人和牌
            showPackedFanAlert([this]() {
                _okCallback(_detail);
                Director::getInstance()->popScene();
            });
        }
        else {
            _okCallback(_detail);
            Director::getInstance()->popScene();
        }
    }
}
