#include "RecordScene.h"
#include "../mahjong-algorithm/fan_calculator.h"
#include "../UICommon.h"
#include "../widget/AlertDialog.h"
#include "../widget/Toast.h"
#include "../widget/TilePickWidget.h"
#include "../widget/ExtraInfoWidget.h"
#include "../mahjong-algorithm/stringify.h"
#include "LittleFan.h"

USING_NS_CC;

static const Color3B C3B_RED = Color3B(254, 87, 110);
static const Color3B C3B_BLUE = Color3B(44, 121, 178);
static const Color3B C3B_GREEN = Color3B(49, 155, 28);
static const Color3B C3B_GRAY = Color3B(96, 96, 96);
static const Color3B C3B_PURPLE = Color3B(89, 16, 89);

// 8个常用番作为初始的「最近使用」
static mahjong::fan_t recentFans[8] = {
    mahjong::MIXED_SHIFTED_CHOWS,
    mahjong::MIXED_TRIPLE_CHOW,
    mahjong::MIXED_STRAIGHT,
    mahjong::ALL_TYPES,
    mahjong::PURE_STRAIGHT,
    mahjong::HALF_FLUSH,
    mahjong::ALL_PUNGS,
    mahjong::PURE_SHIFTED_CHOWS
};

static const mahjong::fan_t standardFans[mahjong::FAN_TABLE_SIZE] = {
    mahjong::FAN_NONE,

    mahjong::BIG_FOUR_WINDS,
    mahjong::BIG_THREE_DRAGONS,
    mahjong::ALL_GREEN,
    mahjong::NINE_GATES,
    mahjong::FOUR_KONGS,
    mahjong::SEVEN_SHIFTED_PAIRS,
    mahjong::THIRTEEN_ORPHANS,

    mahjong::ALL_TERMINALS,
    mahjong::LITTLE_FOUR_WINDS,
    mahjong::LITTLE_THREE_DRAGONS,
    mahjong::ALL_HONORS,
    mahjong::FOUR_CONCEALED_PUNGS,
    mahjong::PURE_TERMINAL_CHOWS,

    mahjong::QUADRUPLE_CHOW,
    mahjong::FOUR_PURE_SHIFTED_PUNGS,

    mahjong::FOUR_PURE_SHIFTED_CHOWS,
    mahjong::THREE_KONGS,
    mahjong::ALL_TERMINALS_AND_HONORS,

    mahjong::SEVEN_PAIRS,
    mahjong::GREATER_HONORS_AND_KNITTED_TILES,
    mahjong::ALL_EVEN_PUNGS,
    mahjong::FULL_FLUSH,
    mahjong::PURE_TRIPLE_CHOW,
    mahjong::PURE_SHIFTED_PUNGS,
    mahjong::UPPER_TILES,
    mahjong::MIDDLE_TILES,
    mahjong::LOWER_TILES,

    mahjong::PURE_STRAIGHT,
    mahjong::THREE_SUITED_TERMINAL_CHOWS,
    mahjong::PURE_SHIFTED_CHOWS,
    mahjong::ALL_FIVE,
    mahjong::TRIPLE_PUNG,
    mahjong::THREE_CONCEALED_PUNGS,

    mahjong::LESSER_HONORS_AND_KNITTED_TILES,
    mahjong::KNITTED_STRAIGHT,
    mahjong::UPPER_FOUR,
    mahjong::LOWER_FOUR,
    mahjong::BIG_THREE_WINDS,

    mahjong::MIXED_STRAIGHT,
    mahjong::REVERSIBLE_TILES,
    mahjong::MIXED_TRIPLE_CHOW,
    mahjong::MIXED_SHIFTED_PUNGS,
    mahjong::CHICKEN_HAND,
    mahjong::LAST_TILE_DRAW,
    mahjong::LAST_TILE_CLAIM,
    mahjong::OUT_WITH_REPLACEMENT_TILE,
    mahjong::ROBBING_THE_KONG,

    mahjong::ALL_PUNGS,
    mahjong::HALF_FLUSH,
    mahjong::MIXED_SHIFTED_CHOWS,
    mahjong::ALL_TYPES,
    mahjong::MELDED_HAND,
    mahjong::TWO_CONCEALED_KONGS,
    mahjong::TWO_DRAGONS_PUNGS,

    mahjong::OUTSIDE_HAND,
    mahjong::FULLY_CONCEALED_HAND,
    mahjong::TWO_MELDED_KONGS,
    mahjong::LAST_TILE
};

namespace {
    typedef struct {
        const char *const title;
        const mahjong::fan_t *fans;
        size_t count;
        const mahjong::fan_t first_fan;
    } CellDetail;
}

static CellDetail cellDetails[11] = {
    { "最近使用", recentFans, 8, mahjong::FAN_NONE },
    { "4番", &standardFans[mahjong::OUTSIDE_HAND], 4, mahjong::OUTSIDE_HAND },
    { "6番", &standardFans[mahjong::ALL_PUNGS], 7, mahjong::ALL_PUNGS },
    { "8番", &standardFans[mahjong::MIXED_STRAIGHT], 9, mahjong::MIXED_STRAIGHT },
    { "12番", &standardFans[mahjong::LESSER_HONORS_AND_KNITTED_TILES], 5, mahjong::LESSER_HONORS_AND_KNITTED_TILES },
    { "16番", &standardFans[mahjong::PURE_STRAIGHT], 6, mahjong::PURE_STRAIGHT },
    { "24番", &standardFans[mahjong::SEVEN_PAIRS], 9, mahjong::SEVEN_PAIRS },
    { "32番", &standardFans[mahjong::FOUR_PURE_SHIFTED_CHOWS], 3, mahjong::FOUR_PURE_SHIFTED_CHOWS },
    { "48番", &standardFans[mahjong::QUADRUPLE_CHOW], 2, mahjong::QUADRUPLE_CHOW },
    { "64番", &standardFans[mahjong::ALL_TERMINALS], 6, mahjong::ALL_TERMINALS },
    { "88番", &standardFans[mahjong::BIG_FOUR_WINDS], 7, mahjong::BIG_FOUR_WINDS }
};

static FORCE_INLINE size_t computeRowsAlign4(size_t cnt) {
    return (cnt >> 2) + !!(cnt & 0x3);
}

#define ORDER(flag_, i_) (((flag_) >> ((i_) << 1)) & 0x3)

#define PLAYER_TO_UI(p_) ORDER(_seatFlag, (p_))
#define UI_TO_PLAYER(u_) ORDER(_playerFlag, (u_))

bool RecordScene::initWithIndex(size_t handIdx, const PlayerNames &names, const Record::Detail *detail, const SubmitCallback &callback) {
    if (UNLIKELY(!BaseScene::initWithTitle(handNameText[handIdx]))) {
        return false;
    }

    _handIdx = handIdx;
    _submitCallback = callback;

    switch (handIdx >> 2) {
    default: _seatFlag = 0xE4; _playerFlag = 0xE4; break;  // 3210 3210
    case 1: _seatFlag = 0xB1; _playerFlag = 0xB1; break;  // 2301 2301
    case 2: _seatFlag = 0x1E; _playerFlag = 0x4B; break;  // 0132 1023
    case 3: _seatFlag = 0x4B; _playerFlag = 0x1E; break;  // 1023 0132
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

    float yPos = origin.y + visibleSize.height - 45.0f;
    // 番数输入框
    ui::EditBox *editBox = UICommon::createEditBox(Size(35.0f, 20.0f));
    this->addChild(editBox);
    editBox->setInputFlag(ui::EditBox::InputFlag::SENSITIVE);
    editBox->setInputMode(ui::EditBox::InputMode::NUMERIC);
    editBox->setReturnType(ui::EditBox::KeyboardReturnType::DONE);
    editBox->setFontColor(Color4B::BLACK);
    editBox->setFontSize(12);
    editBox->setText("8");
    editBox->setMaxLength(3);  // 理论最高番332番，所以最大为3位
    editBox->setPosition(Vec2(origin.x + 84.0f, yPos));
    editBox->setDelegate(this);
    _editBox = editBox;

    Label *label = Label::createWithSystemFont("番", "Arial", 12);
    label->setColor(Color3B::BLACK);
    this->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(origin.x + 104.0f, yPos));

    // +-按钮
    static const float xPos[4] = { 18.0f, 48.0f, 133.0f, 163.0f };
    static const char *titleText[4] = { "-5", "-1", "+1", "+5" };
    static const int delta[4] = { -5, -1, 1, 5 };
    for (int i = 0; i < 4; ++i) {
        ui::Button *button = UICommon::createButton();
        this->addChild(button);
        button->setScale9Enabled(true);
        button->setContentSize(Size(25.0f, 20.0f));
        button->setTitleFontSize(12);
        button->setTitleText(titleText[i]);
        button->setPosition(Vec2(origin.x + xPos[i], yPos));
        button->addClickEventListener(std::bind(&RecordScene::onPlusButton, this, std::placeholders::_1, delta[i]));
    }

    // 荒庄
    ui::CheckBox *checkBox = UICommon::createCheckBox();
    this->addChild(checkBox);
    checkBox->setZoomScale(0.0f);
    checkBox->ignoreContentAdaptWithSize(false);
    checkBox->setContentSize(Size(20.0f, 20.0f));
    checkBox->setPosition(Vec2(origin.x + visibleSize.width - 50.0f, yPos));
    checkBox->addEventListener(std::bind(&RecordScene::onDrawBox, this, std::placeholders::_1, std::placeholders::_2));
    _drawBox = checkBox;

    label = Label::createWithSystemFont("荒庄", "Arial", 12);
    label->setColor(Color3B::BLACK);
    this->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(origin.x + visibleSize.width - 35.0f, yPos));

    // 罚分调整
    ui::Button *button = UICommon::createButton();
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(55.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText("罚分调整");
    button->setPosition(Vec2(origin.x + visibleSize.width - 35.0f, origin.y + visibleSize.height - 70.0f));
    button->addClickEventListener(std::bind(&RecordScene::onPenaltyButton, this, std::placeholders::_1, names));

    // 说明文本
    label = Label::createWithSystemFont("番数支持直接输入，强烈建议先「标记主番」直接增加番数，再用+-按钮调整。",
        "Arial", 10, Size(visibleSize.width - 75.0f, 0.0f));
    label->setColor(C3B_GRAY);
    this->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_TOP_LEFT);
    label->setPosition(Vec2(origin.x + 5.0f, origin.y + visibleSize.height - 60.0f));

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
        label = Label::createWithSystemFont(names[PLAYER_TO_UI(i)], "Arial", 12.0f);
        label->setColor(Color3B::ORANGE);
        this->addChild(label);
        label->setPosition(Vec2(x, origin.y + visibleSize.height - 95.0f));
        cw::scaleLabelToFitWidth(label, gap - 4.0f);

        // 得分
        label = Label::createWithSystemFont("+0", "Arial", 12);
        label->setColor(C3B_GRAY);
        this->addChild(label);
        label->setPosition(Vec2(x, origin.y + visibleSize.height - 115.0f));
        _scoreLabel[i] = label;

        // 和牌
        float y = origin.y + visibleSize.height - 140.0f;
        ui::RadioButton *button = UICommon::createRadioButton();
        radioNode->addChild(button);
        button->setZoomScale(0.0f);
        button->ignoreContentAdaptWithSize(false);
        button->setContentSize(Size(20.0f, 20.0f));
        button->setPosition(Vec2(x - 15.0f, y));
        winGroup->addRadioButton(button);

        label = Label::createWithSystemFont("和牌", "Arial", 12);
        label->setColor(Color3B::BLACK);
        radioNode->addChild(label);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        label->setPosition(Vec2(x, y));

        // 点炮或自摸
        y = origin.y + visibleSize.height - 170.0f;
        button = UICommon::createRadioButton();
        radioNode->addChild(button);
        button->setZoomScale(0.0f);
        button->ignoreContentAdaptWithSize(false);
        button->setContentSize(Size(20.0f, 20.0f));
        button->setPosition(Vec2(x - 15.0f, y));
        claimGroup->addRadioButton(button);

        label = Label::createWithSystemFont("点炮", "Arial", 12);
        label->setColor(Color3B::BLACK);
        radioNode->addChild(label);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        label->setPosition(Vec2(x, y));
        _byDiscardLabel[i] = label;

        label = Label::createWithSystemFont("自摸", "Arial", 12);
        label->setColor(Color3B::BLACK);
        radioNode->addChild(label);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        label->setPosition(Vec2(x, y));
        label->setVisible(false);
        _selfDrawnLabel[i] = label;

        // 罚分
        y = origin.y + visibleSize.height - 195.0f;
        label = Label::createWithSystemFont("调整 ", "Arial", 12);
        label->setColor(Color3B::BLACK);
        radioNode->addChild(label);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_RIGHT);
        label->setPosition(Vec2(x, y));

        label = Label::createWithSystemFont("+0", "Arial", 12);
        label->setColor(C3B_GRAY);
        radioNode->addChild(label);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        label->setPosition(Vec2(x, y));
        _penaltyLabel[i] = label;
    }
    _winGroup = winGroup;
    _claimGroup = claimGroup;

    // 根结点
    ui::Layout *rootLayout = ui::Layout::create();
    this->addChild(rootLayout);
    rootLayout->setPosition(Vec2(origin.x, origin.y + 30.0f));
    rootLayout->setTouchEnabled(true);

    // 上方所有东西
    DrawNode *topNode = DrawNode::create();
    topNode->setContentSize(Size(visibleSize.width, 50.0f));
    topNode->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    topNode->setIgnoreAnchorPointForPosition(false);
    rootLayout->addChild(topNode);
    topNode->drawLine(Vec2(0.0f, 50.0f), Vec2(visibleSize.width, 50.0f), Color4F::BLACK);

    // 说明
    label = Label::createWithSystemFont("标记主番（4番以上）", "Arial", 12);
    label->setColor(Color3B::BLACK);
    topNode->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(5.0f, 35.0f));

    // 展开/收起
    ui::Button *layoutButton = UICommon::createButton();
    layoutButton->setScale9Enabled(true);
    layoutButton->setContentSize(Size(55.0f, 20.0f));
    layoutButton->setTitleFontSize(12);
    topNode->addChild(layoutButton);
    layoutButton->setPosition(Vec2(visibleSize.width - 35.0f, 35.0f));

    button = UICommon::createButton();
    button->setScale9Enabled(true);
    button->setContentSize(Size(55.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText("记录和牌");
    button->addClickEventListener([this](Ref *) { onRecordTilesButton(nullptr); });
    topNode->addChild(button);
    button->setPosition(Vec2(visibleSize.width - 100.0f, 35.0f));
    _recordTilesButton = button;

    // 小番
    button = UICommon::createButton();
    button->setScale9Enabled(true);
    button->setContentSize(Size(55.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText("小番");
    button->addClickEventListener([this](Ref *) { showLittleFanAlert(false); });
    topNode->addChild(button);
    button->setPosition(Vec2(visibleSize.width - 35.0f, 10.0f));
    _littleFanButton = button;

    cw::TableView *tableView = cw::TableView::create();
    tableView->setDirection(ui::ScrollView::Direction::VERTICAL);
    tableView->setScrollBarPositionFromCorner(Vec2(2.0f, 2.0f));
    tableView->setScrollBarWidth(4.0f);
    tableView->setScrollBarOpacity(0x99);
    tableView->setDelegate(this);
    tableView->setVerticalFillOrder(cw::TableView::VerticalFillOrder::TOP_DOWN);

    tableView->setAnchorPoint(Vec2::ANCHOR_MIDDLE_BOTTOM);
    tableView->setPosition(Vec2(visibleSize.width * 0.5f, 0.0f));
    rootLayout->addChild(tableView);
    _tableView = tableView;

    std::function<void (Ref *)> onLayoutButton = [radioNode, rootLayout, topNode, tableView](Ref *sender) {
        ui::Button *layoutButton = (ui::Button *)sender;

        Size visibleSize = Director::getInstance()->getVisibleSize();
        Size layoutSize;
        layoutSize.width = visibleSize.width;
        if (layoutButton->getUserData()) {
            layoutSize.height = visibleSize.height - 145.0f;
            layoutButton->setUserData(reinterpret_cast<void *>(false));
            layoutButton->setTitleText("\xE2\xAC\x87\xEF\xB8\x8E收起");
            radioNode->setVisible(false);
        }
        else {
            layoutSize.height = visibleSize.height - 225.0f;
            layoutButton->setUserData(reinterpret_cast<void *>(true));
            layoutButton->setTitleText("\xE2\xAC\x86\xEF\xB8\x8E展开");
            radioNode->setVisible(true);
        }

        rootLayout->setContentSize(layoutSize);
        topNode->setPosition(Vec2(visibleSize.width * 0.5f, layoutSize.height - 35.0f));
        tableView->setContentSize(Size(visibleSize.width - 5.0f, layoutSize.height - 65.0f));
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
    const float labelPosX = label->getContentSize().width + 5.0f + 2.0f;
    static const char *text[] = { "6番", "8番", "12番", "16番", "24番" };
    for (size_t i = 0; i < 5; ++i) {
        button = UICommon::createButton();
        button->setScale9Enabled(true);
        button->setContentSize(Size(30.0f, 20.0f));
        button->setTitleFontSize(12);
        button->setTitleText(text[i]);
        button->addClickEventListener([tableView, i](Ref *) { tableView->jumpToCell(i + 2); });
        topNode->addChild(button);
        button->setPosition(Vec2(labelPosX + 35.0f * (0.5f + i), 10.0f));
    }

    // 提交按钮
    button = UICommon::createButton();
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(50.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText("提交");
    button->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + 15.0f));
    button->addClickEventListener(std::bind(&RecordScene::onSubmitButton, this, std::placeholders::_1));
    button->setEnabled(false);
    _submitButton = button;

    if (detail != nullptr) {
        refresh();
    }
    return true;
}

ssize_t RecordScene::numberOfCellsInTableView(cw::TableView *) {
    return 11;
}

float RecordScene::tableCellSizeForIndex(cw::TableView *, ssize_t idx) {
    size_t cnt = cellDetails[idx].count;
    float height = computeRowsAlign4(cnt) * 25.0f;
    return (height + 15.0f);
}

namespace {
    struct FakeCheckBox {
        ui::Widget *root_widget = nullptr;
        ui::Scale9Sprite *normal_sprite = nullptr;
        ui::Scale9Sprite *selected_sprite = nullptr;
        Label *title_label = nullptr;
        bool is_selected = false;
        ssize_t cell_idx = 0;
        mahjong::fan_t fan = mahjong::FAN_NONE;

        inline void setSelectd(bool selected) {
            if (selected) {
                is_selected = true;
                normal_sprite->setVisible(false);
                selected_sprite->setVisible(true);
            }
            else {
                is_selected = false;
                normal_sprite->setVisible(true);
                selected_sprite->setVisible(false);
            }
        }
    };

    typedef cw::TableViewCellEx<Label *, std::array<FakeCheckBox, 9> > CustomCell;
}

cw::TableViewCell *RecordScene::tableCellAtIndex(cw::TableView *table, ssize_t idx) {
    CustomCell *cell = (CustomCell *)table->dequeueCell();

    Size visibleSize = Director::getInstance()->getVisibleSize();
    const float gap = (visibleSize.width - 5.0f) * 0.25f;

    if (cell == nullptr) {
        cell = CustomCell::create();

        CustomCell::ExtDataType &ext = cell->getExtData();
        Label *&label = std::get<0>(ext);
        std::array<FakeCheckBox, 9> &checkBoxes = std::get<1>(ext);

        label = Label::createWithSystemFont("1番", "Arial", 12);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        cell->addChild(label);
        label->setColor(Color3B::BLACK);

        for (size_t k = 0; k < 9; ++k) {
            Size size(gap - 4.0f, 20.0f);
            Vec2 pos(size.width * 0.5f, size.height * 0.5f);

            ui::Widget *rootWidget = ui::Widget::create();
            rootWidget->setTouchEnabled(true);
            rootWidget->setContentSize(size);
            rootWidget->addClickEventListener(std::bind(&RecordScene::onFanNameButton, this, std::placeholders::_1));

            ui::Scale9Sprite *normalSprite = ui::Scale9Sprite::create("source_material/btn_square_normal.png");
            rootWidget->addChild(normalSprite);
            normalSprite->setContentSize(size);
            normalSprite->setPosition(pos);

            ui::Scale9Sprite *selectedSprite = ui::Scale9Sprite::create("source_material/btn_square_highlighted.png");
            rootWidget->addChild(selectedSprite);
            selectedSprite->setContentSize(size);
            selectedSprite->setPosition(pos);

            Label *titleLabel = Label::createWithSystemFont("", "Arail", 12);
            titleLabel->setColor(C3B_GRAY);
            rootWidget->addChild(titleLabel);
            titleLabel->setPosition(pos);

            cell->addChild(rootWidget);

            FakeCheckBox &checkBox = checkBoxes[k];
            checkBox.root_widget = rootWidget;
            checkBox.normal_sprite = normalSprite;
            checkBox.selected_sprite = selectedSprite;
            checkBox.title_label = titleLabel;
            checkBox.is_selected = false;
            rootWidget->setUserData(&checkBox);
        }
    }

    const CellDetail &detail = cellDetails[idx];
    const size_t currentLevelCount = detail.count;
    size_t totalRows = computeRowsAlign4(currentLevelCount);

    CustomCell::ExtDataType &ext = cell->getExtData();
    Label *label = std::get<0>(ext);
    std::array<FakeCheckBox, 9> &checkBoxes = std::get<1>(ext);

    label->setString(detail.title);
    label->setPosition(Vec2(5.0f, totalRows * 25.0f + 7.0f));

    const mahjong::fan_t *fans = detail.fans;
    for (size_t k = 0; k < currentLevelCount; ++k) {
        mahjong::fan_t fan = fans[k];

        FakeCheckBox &checkBox = checkBoxes[k];
        checkBox.cell_idx = idx;
        checkBox.fan = fan;

        checkBox.root_widget->setVisible(true);
        checkBox.title_label->setString(mahjong::fan_name[fan]);
        cw::scaleLabelToFitWidth(checkBox.title_label, gap - 8.0f);

        size_t col = k & 0x3;
        size_t row = k >> 2;
        checkBox.root_widget->setPosition(Vec2(gap * (col + 0.5f), (totalRows - row - 0.5f) * 25.0f));

        checkBox.setSelectd(TEST_FAN(_detail.fan_flag, fan));
    }

    for (size_t k = currentLevelCount; k < 9; ++k) {
        FakeCheckBox &checkBox = checkBoxes[k];
        checkBox.root_widget->setVisible(false);
        checkBox.is_selected = false;
    }

    return cell;
}

void RecordScene::editBoxEditingDidBegin(cocos2d::ui::EditBox *editBox) {
    // 点击后清除内容的效果
    editBox->setPlaceHolder(editBox->getText());
    editBox->setText("");
}

void RecordScene::editBoxReturn(cocos2d::ui::EditBox *editBox) {
    const char *text = editBox->getText();
    if (*text == '\0') {
        const char *placeholder = editBox->getPlaceHolder();
        editBox->setText(placeholder);
        text = placeholder;
    }

    if (atoi(text) < 8) {
        editBox->setText("8");
    }

    updateScoreLabel();
}

static inline void updatePenaltyLabel(Label *label, int16_t ps) {
    label->setString(Common::format("%+hd", ps));

    if (ps < 0) label->setColor(C3B_GREEN);
    else if (ps > 0) label->setColor(C3B_RED);
    else label->setColor(C3B_GRAY);
}

void RecordScene::refresh() {
    uint8_t wf = _detail.win_flag;
    uint8_t cf = _detail.claim_flag;
    if (_detail.fan >= 8) {
        char str[32];
        snprintf(str, sizeof(str), "%hu", _detail.fan);
        _editBox->setText(str);
    }

    // 罚分
    for (int i = 0; i < 4; ++i) {
        updatePenaltyLabel(_penaltyLabel[i], _detail.penalty_scores[PLAYER_TO_UI(i)]);
    }

    _winIndex = WIN_CLAIM_INDEX(wf);
    if (_winIndex != -1) {  // 有人和牌
        int claimIndex = WIN_CLAIM_INDEX(cf);  // 点炮者
        _winGroup->setSelectedButton(UI_TO_PLAYER(_winIndex));
        if (claimIndex != -1) {
            _claimGroup->setSelectedButton(UI_TO_PLAYER(claimIndex));
        }
    }

    if (_detail.fan == 0) {
        _drawBox->setSelected(true);
        onDrawBox(_drawBox, ui::CheckBox::EventType::SELECTED);
    }
    else {
        updateScoreLabel();
    }

    _tableView->reloadDataInplacement();
}

void RecordScene::SetScoreLabelColor(cocos2d::Label *(&scoreLabel)[4], int (&scoreTable)[4], uint8_t win_flag, uint8_t claim_flag, const int16_t (&penalty_scores)[4]) {
    for (int i = 0; i < 4; ++i) {
        if (scoreTable[i] != 0) {
            if (TEST_WIN_CLAIM(win_flag, i)) {  // 和牌：红色
                scoreLabel[i]->setColor(C3B_RED);
            }
            else {
                if (UNLIKELY(penalty_scores[i] < 0)) {  // 罚分：紫色
                    scoreLabel[i]->setColor(C3B_PURPLE);
                }
                else if (UNLIKELY(TEST_WIN_CLAIM(claim_flag, i))) {  // 点炮：蓝色
                    scoreLabel[i]->setColor(C3B_BLUE);
                }
                else {  // 其他：绿色
                    scoreLabel[i]->setColor(C3B_GREEN);
                }
            }
        }
        else {
            scoreLabel[i]->setColor(C3B_GRAY);
        }
    }
}

void RecordScene::updateScoreLabel() {
    _detail.win_flag = 0;
    _detail.claim_flag = 0;
    int claimIndex = -1;
    if (_winIndex != -1) {  // 有人和牌
        int fan = atoi(_editBox->getText());  // 获取输入框里所填番数
        _detail.fan = std::max<uint16_t>(8, fan);
        claimIndex = _claimGroup->getSelectedButtonIndex();

        // 记录和牌和点炮
        SET_WIN_CLAIM(_detail.win_flag, PLAYER_TO_UI(_winIndex));
        if (claimIndex != -1) {
            SET_WIN_CLAIM(_detail.claim_flag, PLAYER_TO_UI(claimIndex));
        }
    }
    else {  // 荒庄
        _detail.fan = 0;
    }

    int scoreTable[4];
    TranslateDetailToScoreTable(_detail, scoreTable);

    for (int i = 0; i < 4; ++i) {
        _scoreLabel[i]->setString(Common::format("%+d", scoreTable[PLAYER_TO_UI(i)]));
    }

    // 使用不同颜色
    Label *tempLabel[4] = { _scoreLabel[UI_TO_PLAYER(0)], _scoreLabel[UI_TO_PLAYER(1)], _scoreLabel[UI_TO_PLAYER(2)], _scoreLabel[UI_TO_PLAYER(3)] };
    SetScoreLabelColor(tempLabel, scoreTable, _detail.win_flag, _detail.claim_flag, _detail.penalty_scores);

    // 未选择和牌
    if (_winIndex == -1) {
        // 荒庄时允许确定
        _submitButton->setEnabled(_drawBox->isSelected());
    }
    else {
        // 未选择是点炮还是自摸时，不允许确定
        _submitButton->setEnabled(claimIndex != -1);
    }
}

void RecordScene::onPlusButton(cocos2d::Ref *, int delta) {
    int winScore = atoi(_editBox->getText());
    int temp = winScore + delta;
    if (temp < 8) temp = 8;
    if (winScore != temp) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", temp);
        _editBox->setText(buf);
        updateScoreLabel();
    }
}

void RecordScene::onRecordTilesButton(cocos2d::Ref *) {
    if (_drawBox->isSelected()) {
        Toast::makeText(this, "荒庄时不能记录和牌", Toast::LENGTH_LONG)->show();
        return;
    }

    mahjong::calculate_param_t param;

    mahjong::string_to_tiles(_detail.win_hand.tiles, &param.hand_tiles, &param.win_tile);
    param.win_flag = _detail.win_hand.win_flag;
    param.flower_count = _detail.win_hand.flower_count;

    int idx = _winGroup->getSelectedButtonIndex();
    if (idx >= 0 && idx < 4) {
        param.seat_wind = static_cast<mahjong::wind_t>(idx);
    }

    showCalculator(param);
}

void RecordScene::onDrawBox(cocos2d::Ref *, cocos2d::ui::CheckBox::EventType event) {
    if (event == ui::CheckBox::EventType::SELECTED) {
        _recordTilesButton->setEnabled(false);
        _littleFanButton->setEnabled(false);
        _winIndex = -1;
        // 禁用所有人的和、自摸/点炮
        for (int i = 0; i < 4; ++i) {
            ui::RadioButton *button = _winGroup->getRadioButtonByIndex(i);
            button->setEnabled(false);
            button = _claimGroup->getRadioButtonByIndex(i);
            button->setEnabled(false);
        }
    }
    else {
        _recordTilesButton->setEnabled(true);
        _littleFanButton->setEnabled(true);
        _winIndex = _winGroup->getSelectedButtonIndex();
        // 启用所有人的和、自摸/点炮
        for (int i = 0; i < 4; ++i) {
            ui::RadioButton *button = _winGroup->getRadioButtonByIndex(i);
            button->setEnabled(true);
            button = _claimGroup->getRadioButtonByIndex(i);
            button->setEnabled(true);
        }
    }
    updateScoreLabel();
}

void RecordScene::onWinGroup(cocos2d::ui::RadioButton *, int index, cocos2d::ui::RadioButtonGroup::EventType) {
    _winIndex = index;
    for (int i = 0; i < 4; ++i) {
        if (i == index) {
            // 和的选手，显示自摸
            _byDiscardLabel[i]->setVisible(false);
            _selfDrawnLabel[i]->setVisible(true);
        }
        else {
            // 未和的选手，显示点炮
            _byDiscardLabel[i]->setVisible(true);
            _selfDrawnLabel[i]->setVisible(false);
        }
    }
    updateScoreLabel();
}

void RecordScene::onClaimGroup(cocos2d::ui::RadioButton *, int, cocos2d::ui::RadioButtonGroup::EventType) {
    if (_winIndex == -1) return;
    updateScoreLabel();
}

void RecordScene::onPenaltyButton(cocos2d::Ref *, const PlayerNames &names) {
    float maxWidth = AlertDialog::maxWidth();

    Node *rootNode = Node::create();
    rootNode->setContentSize(Size(maxWidth, 145.0f));

    const float gap = (maxWidth - 4.0f) * 0.25f;

    static const int16_t value[4] = { -10, -5, +5, +10 };
    static const char *text[4] = { "-10", "-5", "+5", "+10" };
    static const float buttonY[4] = { 110.0f, 85.0f, 35.0f, 10.0f };

    std::shared_ptr<std::array<int16_t, 4> > penaltyScores = std::make_shared<std::array<int16_t, 4> >();
    memcpy(penaltyScores->data(), &_detail.penalty_scores, sizeof(_detail.penalty_scores));

    for (int i = 0; i < 4; ++i) {
        const float x = gap * (i + 0.5f);

        // 名字
        Label *label = Label::createWithSystemFont(names[PLAYER_TO_UI(i)], "Arial", 12.0f);
        label->setColor(Color3B::ORANGE);
        rootNode->addChild(label);
        label->setPosition(Vec2(x, 135.0f));
        cw::scaleLabelToFitWidth(label, gap - 2.0f);

        ui::Scale9Sprite *sprite = ui::Scale9Sprite::create("source_material/btn_square_normal.png");
        rootNode->addChild(sprite);
        sprite->setContentSize(Size(30.0f, 20.0f));
        sprite->setPosition(Vec2(x, 60.0f));

        // 罚分
        label = Label::createWithSystemFont("", "Arial", 12);
        rootNode->addChild(label);
        label->setPosition(Vec2(x, 60.0f));
        updatePenaltyLabel(label, penaltyScores->at(PLAYER_TO_UI(i)));

        for (int n = 0; n < 4; ++n) {
            ui::Button *button = UICommon::createButton();
            button->setScale9Enabled(true);
            button->setContentSize(Size(30.0f, 20.0f));
            button->setTitleFontSize(12);
            button->setTitleText(text[n]);
            rootNode->addChild(button);
            button->setPosition(Vec2(x, buttonY[n]));
            int v = value[n];
            button->addClickEventListener([this, penaltyScores, label, i, v](Ref *) {
                int16_t &ps = penaltyScores->at(PLAYER_TO_UI(i));
                ps += v;
                updatePenaltyLabel(label, ps);
            });
        }
    }

    AlertDialog::Builder(this)
        .setTitle("罚分调整")
        .setContentNode(rootNode)
        .setCloseOnTouchOutside(false)
        .setNegativeButton("取消", nullptr)
        .setPositiveButton("确定", [this, penaltyScores](AlertDialog *, int) {
        memcpy(&_detail.penalty_scores, penaltyScores->data(), sizeof(_detail.penalty_scores));
        for (int i = 0; i < 4; ++i) {
            updatePenaltyLabel(_penaltyLabel[i], _detail.penalty_scores[PLAYER_TO_UI(i)]);
        }
        updateScoreLabel();
        return true;
    }).create()->show();
}

void RecordScene::showLittleFanAlert(bool callFromSubmiting) {
    const float maxWidth = AlertDialog::maxWidth();

    Node *rootNode = Node::create();
    rootNode->setContentSize(Size(maxWidth, 255.0f));

    uint16_t uniqueFan = _detail.unique_fan;
    uint64_t multipleFan = _detail.multiple_fan;

    std::array<ui::CheckBox *, 14> checkBoxes;
    std::array<Label *, 9> labels;

    // 只存在一个的番用CheckBox，每行3个，共5行
    const float width3 = maxWidth / 3.0f;
    for (int i = 0, totalRows = 5; i < 14; ++i) {
        div_t ret = div(i, 3);
        const float xPos = width3 * ret.rem;
        const float yPos = 130.0f + (totalRows - ret.quot - 0.5f) * 25.0f;

        ui::CheckBox *checkBox = UICommon::createCheckBox();
        checkBox->setZoomScale(0.0f);
        checkBox->ignoreContentAdaptWithSize(false);
        checkBox->setContentSize(Size(20.0f, 20.0f));
        checkBox->setPosition(Vec2(xPos + 12.0f, yPos));
        rootNode->addChild(checkBox);
        checkBox->setSelected(TEST_UNIQUE_FAN(uniqueFan, i));
        checkBoxes[i] = checkBox;

        Label *label = Label::createWithSystemFont(mahjong::fan_name[uniqueFanTable[i]], "Arial", 12);
        label->setColor(Color3B::BLACK);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        rootNode->addChild(label);
        label->setPosition(Vec2(xPos + 27.0f, yPos));
        cw::scaleLabelToFitWidth(label, width3 - 29.0f);
    }

    // 复计的上限
    static const int limitCounts[] = { 3, 2, 2, 2, 2, 2, 4, 3, 8 };

    // 可复计的番有+-按钮，每行2个，共5行
    const float width2 = maxWidth * 0.5f;
    for (int i = 0, totalRows = 5; i < 9; ++i) {
        const float xPos = width2 * (i & 1);
        const float yPos = (totalRows - (i >> 1) - 0.5f) * 25.0f;

        // 方框背景
        ui::Scale9Sprite *sprite = ui::Scale9Sprite::create("source_material/btn_square_normal.png");
        rootNode->addChild(sprite);
        sprite->setContentSize(Size(20.0f, 20.0f));
        sprite->setPosition(Vec2(xPos + 35.0f, yPos));

        int cnt = static_cast<int>(MULTIPLE_FAN_COUNT(multipleFan, i));

        // 显示数量的Label
        Label *label = Label::createWithSystemFont(std::to_string(cnt), "Arial", 12);
        label->setColor(Color3B::BLACK);
        rootNode->addChild(label);
        label->setPosition(Vec2(xPos + 35.0f, yPos));
        label->setTag(cnt);
        labels[i] = label;

        // -按钮
        ui::Button *button = UICommon::createButton();
        button->setScale9Enabled(true);
        button->setContentSize(Size(20.0f, 20.0f));
        button->setTitleFontSize(12);
        button->setTitleText("-1");
        rootNode->addChild(button);
        button->setPosition(Vec2(xPos + 12.0f, yPos));
        button->addClickEventListener([label](Ref *) {
            int n = label->getTag();
            if (n > 0) {
                label->setTag(--n);
                label->setString(std::to_string(n));
            }
        });

        // +按钮
        int limitCount = limitCounts[i];
        button = UICommon::createButton();
        button->setScale9Enabled(true);
        button->setContentSize(Size(20.0f, 20.0f));
        button->setTitleFontSize(12);
        button->setTitleText("+1");
        rootNode->addChild(button);
        button->setPosition(Vec2(xPos + 58.0f, yPos));
        button->addClickEventListener([label, limitCount](Ref *) {
            int n = label->getTag();
            if (n < limitCount) {
                label->setTag(++n);
                label->setString(std::to_string(n));
            }
        });

        // 番名Label
        label = Label::createWithSystemFont(mahjong::fan_name[multipleFanTable[i]], "Arial", 12);
        label->setColor(Color3B::BLACK);
        rootNode->addChild(label);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        label->setPosition(Vec2(xPos + 70.0f, yPos));
        cw::scaleLabelToFitWidth(label, width2 - 72.0f);
    }

    AlertDialog::Builder(this)
        .setTitle("标记小番")
        .setContentNode(rootNode)
        .setCloseOnTouchOutside(false)
        .setNegativeButton("取消", nullptr)
        .setPositiveButton("确定", [this, checkBoxes, labels, callFromSubmiting](AlertDialog *, int) {
        uint16_t uniqueFan = 0;
        uint64_t multipleFan = 0;
        for (int i = 0; i < 14; ++i) {
            if (checkBoxes[i]->isSelected()) {
                SET_UNIQUE_FAN(uniqueFan, i);
            }
        }

        for (int i = 0; i < 9; ++i) {
            int cnt = labels[i]->getTag();
            if (cnt > 0) {
                SET_MULTIPLE_FAN(multipleFan, i, cnt);
            }
        }

        _detail.unique_fan = uniqueFan;
        _detail.multiple_fan = multipleFan;
        if (callFromSubmiting) {
            finish();
        }
        return true;
    }).create()->show();
}

static const ssize_t standardFanToCellIdx[mahjong::FAN_TABLE_SIZE] {
    0,
    10, 10, 10, 10, 10, 10, 10,
    9, 9, 9, 9, 9, 9,
    8, 8,
    7, 7, 7,
    6, 6, 6, 6, 6, 6, 6, 6, 6,
    5, 5, 5, 5, 5, 5,
    4, 4, 4, 4, 4,
    3, 3, 3, 3, 3, 3, 3, 3, 3,
    2, 2, 2, 2, 2, 2, 2,
    1, 1, 1, 1,
};

void RecordScene::onFanNameButton(cocos2d::Ref *sender) {
    ui::Button *button = (ui::Button *)sender;
    FakeCheckBox &checkBox = *(FakeCheckBox *)button->getUserData();
    const mahjong::fan_t fan = checkBox.fan;

    // 标记/取消标记番种
    if (checkBox.is_selected) {
        checkBox.setSelectd(false);
        RESET_FAN(_detail.fan_flag, fan);
    }
    else {
        checkBox.setSelectd(true);
        SET_FAN(_detail.fan_flag, fan);
    }

    // 计算番数
    int prevWinScore = atoi(_editBox->getText());
    int currentWinScore = 0;
    for (int n = mahjong::BIG_FOUR_WINDS; n < mahjong::DRAGON_PUNG; ++n) {
        if (TEST_FAN(_detail.fan_flag, n)) {
            currentWinScore += mahjong::fan_value_table[n];
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

    if (checkBox.cell_idx != 0) {
        // 如果在「最近使用」里，则更新
        const auto it = std::find(std::begin(recentFans), std::end(recentFans), fan);
        if (it != std::end(recentFans)) {
            CustomCell *cell = (CustomCell *)_tableView->cellAtIndex(0);
            if (cell != nullptr) {
                // 相应CheckBox的下标
                const ptrdiff_t idx = it - std::begin(recentFans);

                // 刷新CheckBox
                std::array<FakeCheckBox, 9> &checkBoxes = std::get<1>(cell->getExtData());
                if (fan == cellDetails[0].fans[idx]) {
                    checkBoxes[idx].setSelectd(TEST_FAN(_detail.fan_flag, fan));
                }
                else {
                    assert(0);
                }
            }
        }
    }
    else {
        // 点击「最近使用」，更新对应格子的番
        ssize_t cellIdx = standardFanToCellIdx[fan];
        if (cellIdx != 0) {
            CustomCell *cell = (CustomCell *)_tableView->cellAtIndex(cellIdx);
            if (cell != nullptr) {
                // 相应CheckBox的下标
                const CellDetail &detail = cellDetails[cellIdx];
                const ssize_t idx = static_cast<ssize_t>(fan) - static_cast<ssize_t>(detail.first_fan);

                // 刷新CheckBox
                std::array<FakeCheckBox, 9> &checkBoxes = std::get<1>(cell->getExtData());
                if (fan == detail.fans[idx]) {
                    checkBoxes[idx].setSelectd(TEST_FAN(_detail.fan_flag, fan));
                }
                else {
                    assert(0);
                }
            }
        }
    }
}

void RecordScene::adjustRecentFans() {
    uint64_t fanFlag = _detail.fan_flag;
    if (fanFlag == 0) {
        return;
    }

    mahjong::fan_t temp[8];
    uint8_t cnt = 0;

    // 1. 将所有标记的番写到temp
    for (int fan = mahjong::BIG_FOUR_WINDS; fan < mahjong::DRAGON_PUNG; ++fan) {
        if (TEST_FAN(fanFlag, fan)) {
            temp[cnt++] = static_cast<mahjong::fan_t>(fan);
            if (cnt >= 8) {
                break;
            }
        }
    }

    // 2. 对于不满8个的，补充原来recentFans的数据，注意去重
    if (cnt < 8) {
        for (int i = 0; i < 8; ++i) {
            mahjong::fan_t fan = recentFans[i];
            if (TEST_FAN(fanFlag, fan)) {
                continue;
            }
            temp[cnt++] = fan;
            if (cnt >= 8) {
                break;
            }
        }
    }

    cellDetails[0].count = cnt;
    std::copy(std::begin(temp), std::begin(temp) + cnt, std::begin(recentFans));
}

void RecordScene::onSubmitButton(cocos2d::Ref *) {
    if (_detail.fan_flag != 0 || _detail.unique_fan != 0 || _detail.multiple_fan != 0) {  // 标记了番种
        if (_drawBox->isSelected()) {  // 荒庄
            AlertDialog::Builder(this)
                .setTitle("记分")
                .setMessage("你标记了番种却选择了荒庄，是否忽略标记这些番种，记录本盘为荒庄？")
                .setNegativeButton("取消", nullptr)
                .setPositiveButton("确定", [this](AlertDialog *, int) {
                _detail.fan_flag = 0;
                _detail.unique_fan = 0;
                _detail.multiple_fan = 0;
                memset(&_detail.win_hand, 0, sizeof(_detail.win_hand));
                finish();
                return true;
            }).create()->show();
        }
        else {
            finish();
        }
    }
    else {  // 未标记番种
        if (_winIndex != -1 && Common::isCStringEmpty(_detail.win_hand.tiles) && _detail.unique_fan == 0 && _detail.multiple_fan == 0) {
            showLittleFanAlert(true);
        }
        else {
            finish();
        }
    }
}

void RecordScene::finish() {
    _submitCallback(_detail);
    adjustRecentFans();
    Director::getInstance()->popScene();
}

// in FanCalculatorScene.cpp
cocos2d::Node *createFanResultNode(const mahjong::fan_table_t &fan_table, int fontSize, float resultAreaWidth);

void RecordScene::showCalculator(const mahjong::calculate_param_t &param) {
    Size visibleSize = Director::getInstance()->getVisibleSize();
    const float maxWidth = visibleSize.width - 20;

    // 选牌面板和其他信息的相关控件
    TilePickWidget *tilePicker = TilePickWidget::create();
    ExtraInfoWidget *extraInfo = ExtraInfoWidget::create();

    extraInfo->setFlowerCount(param.flower_count);
    extraInfo->setPrevalentWind(static_cast<mahjong::wind_t>(_handIdx / 4));
    extraInfo->setSeatWind(param.seat_wind);

    // 缩放
    Size pickerSize = tilePicker->getContentSize();
    const float pickerScale = maxWidth / pickerSize.width;
    tilePicker->setScale(pickerScale);
    pickerSize = Size(maxWidth, pickerSize.height * pickerScale);

    Size extraInfoSize = extraInfo->getContentSize();
    const float extraInfoScale = maxWidth / extraInfoSize.width;
    extraInfo->setScale(extraInfoScale);
    extraInfoSize = Size(maxWidth, extraInfoSize.height * extraInfoScale);

    // 布局在rootNode上
    Node *rootNode = Node::create();
    rootNode->setContentSize(Size(maxWidth, pickerSize.height + extraInfoSize.height + 5));
    rootNode->addChild(tilePicker);
    tilePicker->setPosition(Vec2(maxWidth * 0.5f, pickerSize.height * 0.5f + extraInfoSize.height + 5));
    rootNode->addChild(extraInfo);
    extraInfo->setPosition(Vec2(maxWidth * 0.5f, extraInfoSize.height * 0.5f));

    tilePicker->setFixedPacksChangedCallback([tilePicker, extraInfo]() {
        extraInfo->refreshByKong(tilePicker->isFixedPacksContainsKong());
    });

    tilePicker->setWinTileChangedCallback([tilePicker, extraInfo]() {
        extraInfo->refreshByWinTile(tilePicker->getServingTile(), !tilePicker->isStandingTilesContainsServingTile(),
            tilePicker->countServingTileInFixedPacks(), tilePicker->isFixedPacksContainsKong());
    });

    if (param.hand_tiles.tile_count != 0 && param.win_tile != 0) {
        tilePicker->setData(param.hand_tiles, param.win_tile);
        extraInfo->setWinFlag(param.win_flag);
    }

    // 通过AlertDialog显示出来
    AlertDialog::Builder(this)
        .setTitle("记录和牌")
        .setContentNode(rootNode)
        .setCloseOnTouchOutside(false)
        .setNegativeButton("取消", nullptr)
        .setPositiveButton("确定", [this, tilePicker, extraInfo, param](AlertDialog *dlg, int) {
        mahjong::calculate_param_t temp = { 0 };
        tilePicker->getData(&temp.hand_tiles, &temp.win_tile);
        if (temp.win_tile == 0 && temp.hand_tiles.tile_count == 0 && temp.hand_tiles.pack_count == 0) {
            AlertDialog::Builder(this)
                .setTitle("记录和牌")
                .setMessage("确定不记录和牌吗？")
                .setPositiveButton("确定", [this, dlg](AlertDialog *, int) { dlg->dismiss(); return true; })
                .setNegativeButton("取消", nullptr)
                .create()->show();
            return false;
        }

        temp.flower_count = static_cast<uint8_t>(extraInfo->getFlowerCount());
        if (temp.flower_count > 8) {
            Toast::makeText(this, "花牌数的范围为0~8", Toast::LENGTH_LONG)->show();
            return false;
        }

        if (temp.win_tile == 0) {
            Toast::makeText(this, "牌张数错误", Toast::LENGTH_LONG)->show();
            return false;
        }

        std::sort(temp.hand_tiles.standing_tiles, temp.hand_tiles.standing_tiles + temp.hand_tiles.tile_count);

        mahjong::fan_table_t fan_table = { 0 };

        // 获取绝张、杠开、抢杠、海底信息
        temp.win_flag = extraInfo->getWinFlag();

        // 获取圈风门风
        temp.prevalent_wind = extraInfo->getPrevalentWind();
        temp.seat_wind = extraInfo->getSeatWind();

        // 算番
        int fan = mahjong::calculate_fan(&temp, &fan_table);

        if (fan == ERROR_NOT_WIN) {
            Toast::makeText(this, "诈和", Toast::LENGTH_LONG)->show();
            return false;
        }
        if (fan == ERROR_WRONG_TILES_COUNT) {
            Toast::makeText(this, "牌张数错误", Toast::LENGTH_LONG)->show();
            return false;
        }
        if (fan == ERROR_TILE_COUNT_GREATER_THAN_4) {
            Toast::makeText(this, "同一种牌最多只能使用4枚", Toast::LENGTH_LONG)->show();
            return false;
        }

        const float maxWidth = AlertDialog::maxWidth();

        Node *innerNode = createFanResultNode(fan_table, 12, maxWidth);
        const Size &fanResultSize = innerNode->getContentSize();

        // 花（使用emoji代码）
        Label *flowerLabel = nullptr;
        if (temp.flower_count > 0) {
            flowerLabel = Label::createWithSystemFont(std::string(EMOJI_FLOWER_8, temp.flower_count * (sizeof(EMOJI_FLOWER) - 1)), "Arial", 12);
            flowerLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
#if CC_TARGET_PLATFORM == CC_PLATFORM_WIN32
            flowerLabel->setColor(Color3B(224, 45, 45));
#endif
        }

        // 手牌
        Node *tilesNode = HandTilesWidget::createStaticNode(temp.hand_tiles, temp.win_tile);
        Size tilesNodeSize = tilesNode->getContentSize();
        if (tilesNodeSize.width > maxWidth) {
            const float scale = maxWidth / tilesNodeSize.width;
            tilesNode->setScale(scale);
            tilesNodeSize.width = maxWidth;
            tilesNodeSize.height *= scale;
        }
        innerNode->addChild(tilesNode);
        tilesNode->setPosition(Vec2(maxWidth * 0.5f, fanResultSize.height + 5 + tilesNodeSize.height * 0.5f));

        if (temp.flower_count > 0) {
            innerNode->addChild(flowerLabel);
            const Size &flowerSize = flowerLabel->getContentSize();
            flowerLabel->setPosition(Vec2(0, fanResultSize.height + 5 + tilesNodeSize.height + 5 + flowerSize.height * 0.5f));

            innerNode->setContentSize(Size(maxWidth, fanResultSize.height + 5 + tilesNodeSize.height + 5 + flowerSize.height));
        }
        else {
            innerNode->setContentSize(Size(maxWidth, fanResultSize.height + 5 + tilesNodeSize.height));
        }

        uint64_t fanFlag = 0;
        for (int n = mahjong::BIG_FOUR_WINDS; n < mahjong::DRAGON_PUNG; ++n) {
            if (fan_table[n]) {
                SET_FAN(fanFlag, n);
            }
        }

        uint16_t uniqueFan = 0;
        uint64_t multipleFan = 0;
        for (unsigned n = 0; n < 14; ++n) {
            if (fan_table[uniqueFanTable[n]] > 0) {
                SET_UNIQUE_FAN(uniqueFan, n);
            }
        }
        for (unsigned n = 0; n < 9; ++n) {
            uint16_t cnt = fan_table[multipleFanTable[n]];
            if (cnt > 0) {
                SET_MULTIPLE_FAN(multipleFan, n, cnt);
            }
        }

        AlertDialog::Builder(this)
            .setTitle("记录和牌")
            .setContentNode(innerNode)
            .setCloseOnTouchOutside(false)
            .setPositiveButton("确定", [this, temp, fan, fanFlag, uniqueFan, multipleFan, dlg](AlertDialog *, int) {
            _detail.fan = std::max<uint16_t>(fan, 8);
            _detail.fan_flag = fanFlag;
            _detail.unique_fan = uniqueFan;
            _detail.multiple_fan = multipleFan;

            Record::Detail::WinHand &winHand = _detail.win_hand;
            memset(&winHand, 0, sizeof(winHand));
            intptr_t n = mahjong::hand_tiles_to_string(&temp.hand_tiles, winHand.tiles, 64);
            mahjong::tiles_to_string(&temp.win_tile, 1, &winHand.tiles[n], sizeof(winHand.tiles) - n);

            winHand.win_flag = temp.win_flag;
            winHand.flower_count = temp.flower_count;

            // 更改输入框中的番数
            char buf[32];
            snprintf(buf, sizeof(buf), "%d", fan);
            _editBox->setText(buf);

            // 根据记录和牌的门风确定和牌的是哪一家
            int winIndex = static_cast<int>(temp.seat_wind);
            _winGroup->setSelectedButton(winIndex);

            if (temp.win_flag & WIN_FLAG_SELF_DRAWN) {  // 自摸
                _claimGroup->setSelectedButton(winIndex);
            }

            refresh();
            dlg->dismiss();
            return true;
        }).setNegativeButton("取消", nullptr).create()->show();
        return false;
    }).create()->show();
}
