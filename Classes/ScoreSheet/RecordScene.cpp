#include "RecordScene.h"
#include "../common.h"
#include "../mahjong-algorithm/points_calculator.h"

USING_NS_CC;

Scene *RecordScene::createScene(size_t handIdx, const char **playerNames, const std::function<void (RecordScene *)> &okCallback) {
    auto scene = Scene::create();
    auto layer = new (std::nothrow) RecordScene();
    layer->initWithIndex(handIdx, playerNames);
    layer->_okCallback = okCallback;
    layer->autorelease();

    scene->addChild(layer);
    return scene;
}

bool RecordScene::initWithIndex(size_t handIdx, const char **playerNames) {
    if (!BaseLayer::initWithTitle(handNameText[handIdx])) {
        return false;
    }

    _winIndex = -1;
    memset(_scoreTable, 0, sizeof(_scoreTable));
    _pointsFlag = 0;

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    // 番数输入框
    _editBox = ui::EditBox::create(Size(35.0f, 20.0f), ui::Scale9Sprite::create("source_material/tabbar_background1.png"));
    this->addChild(_editBox);
    _editBox->setInputFlag(ui::EditBox::InputFlag::SENSITIVE);
    _editBox->setInputMode(ui::EditBox::InputMode::NUMERIC);
    _editBox->setFontColor(Color4B::BLACK);
    _editBox->setFontSize(12);
    _editBox->setText("8");
    _editBox->setPosition(Vec2(origin.x + 65.0f, origin.y + visibleSize.height - 50));
    _editBox->setDelegate(this);

    Label *label = Label::createWithSystemFont("番", "Arial", 12);
    this->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(origin.x + 85.0f, origin.y + visibleSize.height - 50));

    // +-按钮
    ui::Button *minusButton = ui::Button::create("source_material/stepper_dec_n.png", "source_material/stepper_dec_h.png");
    this->addChild(minusButton);
    minusButton->setScale(20.0f / minusButton->getContentSize().height);
    minusButton->setPosition(Vec2(origin.x + 25.0f, origin.y + visibleSize.height - 50));
    minusButton->addClickEventListener(std::bind(&RecordScene::onMinusButton, this, std::placeholders::_1));

    ui::Button *plusButton = ui::Button::create("source_material/stepper_inc_n.png", "source_material/stepper_inc_h.png");
    this->addChild(plusButton);
    plusButton->setScale(20.0f / plusButton->getContentSize().height);
    plusButton->setPosition(Vec2(origin.x + 120.0f, origin.y + visibleSize.height - 50));
    plusButton->addClickEventListener(std::bind(&RecordScene::onPlusButton, this, std::placeholders::_1));

    // 荒庄
    _drawButton = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
    this->addChild(_drawButton);
    _drawButton->setScale9Enabled(true);
    _drawButton->setContentSize(Size(20.0f, 20.0f));
    _drawButton->setPosition(Vec2(origin.x + visibleSize.width - 60.0f, origin.y + visibleSize.height - 50));
    _drawButton->addClickEventListener(std::bind(&RecordScene::onDrawButton, this, std::placeholders::_1));

    label = Label::createWithSystemFont("荒庄", "Arial", 12);
    this->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(origin.x + visibleSize.width - 45.0f, origin.y + visibleSize.height - 50));

    const float gap = (visibleSize.width - 4.0f) * 0.25f;
    for (int i = 0; i < 4; ++i) {
        const float x = origin.x + gap * (i + 0.5f);

        // 名字
        _nameLabel[i] = Label::createWithSystemFont(playerNames[i], "Arial", 12);
        _nameLabel[i]->setColor(Color3B::YELLOW);
        this->addChild(_nameLabel[i]);
        _nameLabel[i]->setPosition(Vec2(x, origin.y + visibleSize.height - 80));

        // 得分
        _scoreLabel[i] = Label::createWithSystemFont("+0", "Arial", 12);
        _scoreLabel[i]->setColor(Color3B::GRAY);
        this->addChild(_scoreLabel[i]);
        _scoreLabel[i]->setPosition(Vec2(x, origin.y + visibleSize.height - 105));

        // 和
        _winButton[i] = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png", "source_material/btn_square_disabled.png");
        this->addChild(_winButton[i]);
        _winButton[i]->setScale9Enabled(true);
        _winButton[i]->setContentSize(Size(20.0f, 20.0f));
        _winButton[i]->setPosition(Vec2(x - 15, origin.y + visibleSize.height - 130));
        setButtonUnchecked(_winButton[i]);
        _winButton[i]->addClickEventListener(std::bind(&RecordScene::onWinButton, this, std::placeholders::_1, i));

        label = Label::createWithSystemFont("和", "Arial", 12);
        this->addChild(label);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        label->setPosition(Vec2(x, origin.y + visibleSize.height - 130));

        // 自摸
        _selfDrawnButton[i] = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png", "source_material/btn_square_disabled.png");
        this->addChild(_selfDrawnButton[i]);
        _selfDrawnButton[i]->setScale9Enabled(true);
        _selfDrawnButton[i]->setContentSize(Size(20.0f, 20.0f));
        _selfDrawnButton[i]->setPosition(Vec2(x - 15, origin.y + visibleSize.height - 160));
        setButtonUnchecked(_selfDrawnButton[i]);
        _selfDrawnButton[i]->addClickEventListener(std::bind(&RecordScene::onSelfDrawnButton, this, std::placeholders::_1, i));

        label = Label::createWithSystemFont("自摸", "Arial", 12);
        this->addChild(label);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        label->setPosition(Vec2(x, origin.y + visibleSize.height - 160));

        // 点炮
        _claimButton[i] = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png", "source_material/btn_square_disabled.png");
        this->addChild(_claimButton[i]);
        _claimButton[i]->setScale9Enabled(true);
        _claimButton[i]->setContentSize(Size(20.0f, 20.0f));
        _claimButton[i]->setPosition(Vec2(x - 15, origin.y + visibleSize.height - 190));
        setButtonUnchecked(_claimButton[i]);
        _claimButton[i]->addClickEventListener(std::bind(&RecordScene::onClaimButton, this, std::placeholders::_1, i));

        label = Label::createWithSystemFont("点炮", "Arial", 12);
        this->addChild(label);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        label->setPosition(Vec2(x, origin.y + visibleSize.height - 190));

        // 错和
        _falseWinButton[i] = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png", "source_material/btn_square_disabled.png");
        this->addChild(_falseWinButton[i]);
        _falseWinButton[i]->setScale9Enabled(true);
        _falseWinButton[i]->setContentSize(Size(20.0f, 20.0f));
        _falseWinButton[i]->setPosition(Vec2(x - 15, origin.y + visibleSize.height - 220));
        setButtonUnchecked(_falseWinButton[i]);
        _falseWinButton[i]->addClickEventListener(std::bind(&RecordScene::onFalseWinButton, this, std::placeholders::_1, i));

        label = Label::createWithSystemFont("错和", "Arial", 12);
        this->addChild(label);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        label->setPosition(Vec2(x, origin.y + visibleSize.height - 220));
    }

    // 说明
    label = Label::createWithSystemFont("标记番种（未做排斥检测）", "Arial", 12);
    this->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(origin.x + 5.0f, origin.y + visibleSize.height - 250));

    label = Label::createWithSystemFont("标记番种可快速增加番数，取消标记不减少。", "Arial", 10);
    this->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(origin.x + 5.0f, origin.y + visibleSize.height - 270));

    // ScrollView内部结点
    ui::Widget *innerNode = ui::Widget::create();
    innerNode->setAnchorPoint(Vec2::ANCHOR_BOTTOM_LEFT);
    static const float innerNodeHeight = 572.0f;  // 18行 * 24像素 + 10行 * 14像素
    innerNode->setContentSize(Size(visibleSize.width, innerNodeHeight));

    static const int points[] = { 4, 6, 8, 12, 16, 24, 32, 48, 64, 88 };
    static const size_t beginIndex[] =
#if HAS_CONCEALED_KONG_AND_MELDED_KONG
    { 56, 48, 39, 34, 28, 19, 16, 14, 8, 1 };
#else
    { 55, 48, 39, 34, 28, 19, 16, 14, 8, 1 };
#endif
    static const size_t counts[] = { 4, 7, 8, 5, 6, 9, 3, 2, 6, 7 };  // 各档次的番种的个数
    float y = innerNodeHeight;
    for (int i = 0; i < 10; ++i) {
        // x番label
        Label *label = Label::createWithSystemFont(StringUtils::format("%d番", points[i]), "Arial", 12);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        innerNode->addChild(label);
        label->setPosition(Vec2(5.0f, y - 7.0f));
        y -= 14.0f;

        // 每行排4个番种
        for (size_t k = 0; k < counts[i]; ++k) {
            size_t col = k % 4;
            if (k > 0 && col == 0) {
                y -= 24.0f;
            }

            size_t idx = beginIndex[i] + k;
            ui::Button *button = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
            innerNode->addChild(button);
            button->setScale9Enabled(true);
            button->setContentSize(Size(66.0f, 20.0f));
            button->setTitleColor(Color3B::BLACK);
            button->setTitleFontSize(12);
            button->setTitleText(mahjong::points_name[idx]);
            setButtonUnchecked(button);
            button->addClickEventListener(std::bind(&RecordScene::onPointsNameButton, this, std::placeholders::_1, idx));

            button->setPosition(Vec2(gap * (col + 0.5f), y - 12.0f));
        }
        y -= 24.0f;
    }

    // ScrollView
    ui::ScrollView *scrollView = ui::ScrollView::create();
    scrollView->setDirection(ui::ScrollView::Direction::VERTICAL);
    scrollView->setScrollBarPositionFromCorner(Vec2(10, 10));
    scrollView->setContentSize(Size(visibleSize.width, visibleSize.height - 330));
    scrollView->setInnerContainerSize(innerNode->getContentSize());
    scrollView->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    scrollView->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f - 125.0f));
    this->addChild(scrollView);

    scrollView->addChild(innerNode);

    // 确定按钮
    _okButton = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png", "source_material/btn_square_disabled.png");
    this->addChild(_okButton);
    _okButton->setScale9Enabled(true);
    _okButton->setContentSize(Size(52.0f, 22.0f));
    _okButton->setTitleText("确定");
    _okButton->setTitleColor(Color3B::BLACK);
    _okButton->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + 15));
    _okButton->addClickEventListener(std::bind(&RecordScene::onOkButton, this, std::placeholders::_1));
    _okButton->setEnabled(false);

    _winIndex = -1;
    return true;
}

void RecordScene::editBoxReturn(cocos2d::ui::EditBox *editBox) {
    updateScoreLabel();
}

void RecordScene::updateScoreLabel() {
    memset(_scoreTable, 0, sizeof(_scoreTable));
    if (_winIndex != -1) {
        int winScore = atoi(_editBox->getText());  // 获取输入框里所填番数
        if (isButtonChecked(_selfDrawnButton[_winIndex])) {  // 自摸的情况算各家得分
            for (int i = 0; i < 4; ++i) {
                _scoreTable[i] = (i == _winIndex) ? (winScore + 8) * 3 : (-8 - winScore);
            }
        }
        else {  // 点和的情况算各家得分
            for (int i = 0; i < 4; ++i) {
                _scoreTable[i] = (i == _winIndex) ? (winScore + 24) : (isButtonChecked(_claimButton[i]) ? (-8 - winScore) : -8);
            }
        }
    }

    // 检查是否有错和
    for (int i = 0; i < 4; ++i) {
        if (isButtonChecked(_falseWinButton[i])) {
            _scoreTable[i] -= 30;
            for (int j = 0; j < 4; ++j) {
                if (j == i) continue;
                _scoreTable[j] += 10;
            }
        }
    }

    // 正负0分使用不同颜色
    for (int i = 0; i < 4; ++i) {
        _scoreLabel[i]->setString(StringUtils::format("%+d", _scoreTable[i]));
        if (_scoreTable[i] > 0) {
            _scoreLabel[i]->setColor(Color3B::RED);
        }
        else if (_scoreTable[i] < 0) {
            _scoreLabel[i]->setColor(Color3B::GREEN);
        }
        else {
            _scoreLabel[i]->setColor(Color3B::GRAY);
        }
    }

    // 四位选手的总分加起来和为0
    if (_scoreTable[0] + _scoreTable[1] + _scoreTable[2] + _scoreTable[3] == 0) {
        if (isButtonChecked(_drawButton)) {  // 荒庄
            _okButton->setEnabled(_pointsFlag == 0);
        }
        else {  // 不荒庄，那么四位选手的得分一定没有为0的
            _okButton->setEnabled(std::any_of(std::begin(_scoreTable), std::end(_scoreTable), [](int score) { return score != 0; }));
        }
    }
    else {
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

void RecordScene::onDrawButton(cocos2d::Ref *sender) {
    // 荒庄
    _winIndex = -1;
    if (isButtonChecked(_drawButton)) {
        setButtonUnchecked(_drawButton);
        // 启用所有人的和、自摸、点炮
        for (int i = 0; i < 4; ++i) {
            _winButton[i]->setEnabled(true);
            _selfDrawnButton[i]->setEnabled(true);
            _claimButton[i]->setEnabled(true);
        }
    }
    else {
        setButtonChecked(_drawButton);
        // 禁用所有人的和、自摸、点炮
        for (int i = 0; i < 4; ++i) {
            setButtonUnchecked(_winButton[i]);
            _winButton[i]->setEnabled(false);
            setButtonUnchecked(_selfDrawnButton[i]);
            _selfDrawnButton[i]->setEnabled(false);
            setButtonUnchecked(_claimButton[i]);
            _claimButton[i]->setEnabled(false);
        }
    }
    updateScoreLabel();
}

void RecordScene::onWinButton(cocos2d::Ref *sender, int index) {
    setButtonChecked(_winButton[index]);
    if (_winIndex == index) return;

    _winIndex = index;
    for (int i = 0; i < 4; ++i) {
        if (i == index) {
            // 和的选手，启用自摸，禁用点炮、错和
            setButtonChecked(_winButton[i]);
            setButtonUnchecked(_selfDrawnButton[i]);
            _selfDrawnButton[i]->setEnabled(true);
            setButtonUnchecked(_claimButton[i]);
            _claimButton[i]->setEnabled(false);
            setButtonUnchecked(_falseWinButton[i]);
            _falseWinButton[i]->setEnabled(false);
        }
        else {
            // 没和的选手，禁用和、自摸，启用点炮、错和
            setButtonUnchecked(_winButton[i]);
            setButtonUnchecked(_selfDrawnButton[i]);
            _selfDrawnButton[i]->setEnabled(false);
            setButtonUnchecked(_claimButton[i]);
            _claimButton[i]->setEnabled(true);
            setButtonUnchecked(_falseWinButton[i]);
            _falseWinButton[i]->setEnabled(true);
        }
    }
    updateScoreLabel();
}

void RecordScene::onSelfDrawnButton(cocos2d::Ref *sender, int index) {
    if (_winIndex == -1) return;

    if (isButtonChecked(_selfDrawnButton[index])) {  // 取消自摸
        setButtonUnchecked(_selfDrawnButton[index]);
        // 启用其他三位选手的点炮
        for (int i = 0; i < 4; ++i) {
            _claimButton[i]->setEnabled(index != i);
        }
    }
    else {  // 选中自摸
        setButtonChecked(_selfDrawnButton[index]);
        // 禁用所有人的点炮
        for (int i = 0; i < 4; ++i) {
            setButtonUnchecked(_claimButton[i]);
            _claimButton[i]->setEnabled(false);
        }
    }
    updateScoreLabel();
}

void RecordScene::onClaimButton(cocos2d::Ref *sender, int index) {
    if (_winIndex == -1) return;

    if (isButtonChecked(_claimButton[index])) {  // 取消点炮
        // 启用其他三位选手的点炮
        for (int i = 0; i < 4; ++i) {
            _claimButton[i]->setEnabled(_winIndex != i);
        }
        setButtonUnchecked(_claimButton[index]);
    }
    else {  // 点炮
        for (int i = 0; i < 4; ++i) {
            _claimButton[i]->setEnabled(index == i);
        }
        setButtonChecked(_claimButton[index]);
    }
    updateScoreLabel();
}

void RecordScene::onFalseWinButton(cocos2d::Ref *sender, int index) {
    if (isButtonChecked(_falseWinButton[index])) {
        setButtonUnchecked(_falseWinButton[index]);
    }
    else {
        setButtonChecked(_falseWinButton[index]);
    }
    updateScoreLabel();
}

void RecordScene::onPointsNameButton(cocos2d::Ref *sender, int index) {
#if HAS_CONCEALED_KONG_AND_MELDED_KONG
    if (index > mahjong::POINT_TYPE::CONCEALED_KONG_AND_MELDED_KONG) {
        --index;
    }
#endif
    // 标记/取消标记番种
    ui::Button *button = (ui::Button *)sender;
    if (isButtonChecked(button)) {
        setButtonUnchecked(button);
        _pointsFlag &= ~(1ULL << index);
    }
    else {
        setButtonChecked(button);
        _pointsFlag |= 1ULL << index;
    }

    // 增加番数
    int prevWinScore = atoi(_editBox->getText());
    int currentWinScore = 0;
    for (int n = 0; n < 64; ++n) {
        if (_pointsFlag & (1ULL << n)) {
            unsigned idx = n;
#if HAS_CONCEALED_KONG_AND_MELDED_KONG
            if (idx >= mahjong::POINT_TYPE::CONCEALED_KONG_AND_MELDED_KONG) {
                ++idx;
            }
#endif
            currentWinScore += mahjong::points_value_table[idx];
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
    _okCallback(this);
    Director::getInstance()->popScene();
}
