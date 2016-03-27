#include "RecordScene.h"
#include "../mahjong-algorithm/points_calculator.h"

#pragma execution_character_set("utf-8")

USING_NS_CC;

static inline void setButtonChecked(ui::Button *button) {
    button->setHighlighted(true);
    button->setTag(1);
}

static inline void setButtonUnchecked(ui::Button *button) {
    button->setTag(0);
    button->setHighlighted(false);
}

static inline bool isButtonChecked(ui::Button *button) {
    return button->getTag() == 1;
}

Scene *RecordScene::createScene(int index, const char **name, const std::function<void (RecordScene *)> &okCallback) {
    auto scene = Scene::create();
    auto layer = new (std::nothrow) RecordScene();
    layer->initWithIndex(index, name);
    layer->_okCallback = okCallback;

    scene->addChild(layer);
    return scene;
}

bool RecordScene::initWithIndex(int index, const char **name) {
    if (!Layer::init()) {
        return false;
    }

    _winIndex = -1;
    memset(_scoreTable, 0, sizeof(_scoreTable));
    _pointsFlag = 0;

    auto listener = EventListenerKeyboard::create();
    listener->onKeyReleased = CC_CALLBACK_2(Layer::onKeyReleased, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    ui::Button *backBtn = ui::Button::create();
    this->addChild(backBtn);
    backBtn->setScale9Enabled(true);
    backBtn->setContentSize(Size(45, 20));
    backBtn->setTitleFontSize(16);
    backBtn->setTitleText("返回");
    backBtn->setPosition(Vec2(origin.x + 15, origin.y + visibleSize.height - 10));
    backBtn->addClickEventListener([](Ref *) {
        Director::getInstance()->popScene();
    });

    const char *handText[] = { "东风东", "东风南", "东风西", "东风北", "南风东", "南风南", "南风西", "南风北",
        "西风东", "西风南", "西风西", "西风北", "北风东", "北风南", "北风西", "北风北" };
    Label *tileLabel = Label::createWithSystemFont(handText[index], "Arial", 18);
    this->addChild(tileLabel);
    tileLabel->setPosition(Vec2(origin.x + visibleSize.width * 0.5f,
        origin.y + visibleSize.height - tileLabel->getContentSize().height * 0.5f));

    _editBox = ui::EditBox::create(Size(35.0f, 22.0f), ui::Scale9Sprite::create("source_material/tabbar_background1.png"));
    this->addChild(_editBox);
    _editBox->setInputFlag(ui::EditBox::InputFlag::SENSITIVE);
    _editBox->setInputMode(ui::EditBox::InputMode::NUMERIC);
    _editBox->setFontColor(Color4B(0, 0, 0, 255));
    _editBox->setPosition(Vec2(origin.x + 65.0f, origin.y + visibleSize.height - 50));
    _editBox->setDelegate(this);
    _editBox->setText("8");

    Label *label = Label::createWithSystemFont("番", "Arial", 12);
    this->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(origin.x + 85.0f, origin.y + visibleSize.height - 50));

    ui::Button *minusButton = ui::Button::create("source_material/stepper_dec_n.png", "source_material/stepper_dec_h.png");
    this->addChild(minusButton);
    minusButton->setScale(20.0f / minusButton->getContentSize().height);
    minusButton->setPosition(Vec2(origin.x + 25.0f, origin.y + visibleSize.height - 50));
    minusButton->addClickEventListener(std::bind(&RecordScene::minusCallback, this, std::placeholders::_1));

    ui::Button *plusButton = ui::Button::create("source_material/stepper_inc_n.png", "source_material/stepper_inc_h.png");
    this->addChild(plusButton);
    plusButton->setScale(20.0f / plusButton->getContentSize().height);
    plusButton->setPosition(Vec2(origin.x + 120.0f, origin.y + visibleSize.height - 50));
    plusButton->addClickEventListener(std::bind(&RecordScene::plusCallback, this, std::placeholders::_1));

    _drawButton = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
    this->addChild(_drawButton);
    _drawButton->setScale9Enabled(true);
    _drawButton->setContentSize(Size(22.0f, 22.0f));
    _drawButton->setPosition(Vec2(origin.x + visibleSize.width - 60.0f, origin.y + visibleSize.height - 50));
    _drawButton->addClickEventListener(std::bind(&RecordScene::drawCallback, this, std::placeholders::_1));

    label = Label::createWithSystemFont("荒庄", "Arial", 12);
    this->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(origin.x + visibleSize.width - 45.0f, origin.y + visibleSize.height - 50));

    const float gap = (visibleSize.width - 4.0f) * 0.25f;
    for (int i = 0; i < 4; ++i) {
        const float x = origin.x + gap * (i + 0.5f);
        _nameLabel[i] = Label::createWithSystemFont(name[i], "Arial", 12);
        _nameLabel[i]->setColor(Color3B::YELLOW);
        this->addChild(_nameLabel[i]);
        _nameLabel[i]->setPosition(Vec2(x, origin.y + visibleSize.height - 80));

        _scoreLabel[i] = Label::createWithSystemFont("+0", "Arial", 12);
        _scoreLabel[i]->setColor(Color3B::GRAY);
        this->addChild(_scoreLabel[i]);
        _scoreLabel[i]->setPosition(Vec2(x, origin.y + visibleSize.height - 105));

        _winButton[i] = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png", "source_material/btn_square_disabled.png");
        this->addChild(_winButton[i]);
        _winButton[i]->setScale9Enabled(true);
        _winButton[i]->setContentSize(Size(22.0f, 22.0f));
        _winButton[i]->setPosition(Vec2(x - 15, origin.y + visibleSize.height - 130));
        setButtonUnchecked(_winButton[i]);
        _winButton[i]->addClickEventListener(std::bind(&RecordScene::winCallback, this, std::placeholders::_1, i));

        label = Label::createWithSystemFont("和", "Arial", 12);
        this->addChild(label);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        label->setPosition(Vec2(x, origin.y + visibleSize.height - 130));

        _selfDrawnButton[i] = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png", "source_material/btn_square_disabled.png");
        this->addChild(_selfDrawnButton[i]);
        _selfDrawnButton[i]->setScale9Enabled(true);
        _selfDrawnButton[i]->setContentSize(Size(22.0f, 22.0f));
        _selfDrawnButton[i]->setPosition(Vec2(x - 15, origin.y + visibleSize.height - 160));
        setButtonUnchecked(_selfDrawnButton[i]);
        _selfDrawnButton[i]->addClickEventListener(std::bind(&RecordScene::selfDrawnCallback, this, std::placeholders::_1, i));

        label = Label::createWithSystemFont("自摸", "Arial", 12);
        this->addChild(label);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        label->setPosition(Vec2(x, origin.y + visibleSize.height - 160));

        _claimButton[i] = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png", "source_material/btn_square_disabled.png");
        this->addChild(_claimButton[i]);
        _claimButton[i]->setScale9Enabled(true);
        _claimButton[i]->setContentSize(Size(22.0f, 22.0f));
        _claimButton[i]->setPosition(Vec2(x - 15, origin.y + visibleSize.height - 190));
        setButtonUnchecked(_claimButton[i]);
        _claimButton[i]->addClickEventListener(std::bind(&RecordScene::claimCallback, this, std::placeholders::_1, i));

        label = Label::createWithSystemFont("点炮", "Arial", 12);
        this->addChild(label);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        label->setPosition(Vec2(x, origin.y + visibleSize.height - 190));

        _falseWinButton[i] = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png", "source_material/btn_square_disabled.png");
        this->addChild(_falseWinButton[i]);
        _falseWinButton[i]->setScale9Enabled(true);
        _falseWinButton[i]->setContentSize(Size(22.0f, 22.0f));
        _falseWinButton[i]->setPosition(Vec2(x - 15, origin.y + visibleSize.height - 220));
        setButtonUnchecked(_falseWinButton[i]);
        _falseWinButton[i]->addClickEventListener(std::bind(&RecordScene::falseWinCallback, this, std::placeholders::_1, i));

        label = Label::createWithSystemFont("错和", "Arial", 12);
        this->addChild(label);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        label->setPosition(Vec2(x, origin.y + visibleSize.height - 220));
    }

    label = Label::createWithSystemFont("标记番种（未做排斥检测）", "Arial", 12);
    this->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(origin.x + 5.0f, origin.y + visibleSize.height - 260));

    ui::Widget *innerNode = ui::Widget::create();
    innerNode->setAnchorPoint(Vec2::ANCHOR_BOTTOM_LEFT);
    static const float innerNodeHeight = 572.0f;  // 18行 * 24像素 + 10行 * 14像素
    innerNode->setContentSize(Size(visibleSize.width, innerNodeHeight));

    static const int points[] = { 4, 6, 8, 12, 16, 24, 32, 48, 64, 88 };
    static const size_t beginIndex[] = {55, 48, 39, 34, 28, 19, 16, 14, 8, 1};
    static const size_t counts[] = { 4, 7, 8, 5, 6, 9, 3, 2, 6, 7 };
    float y = innerNodeHeight;
    for (int i = 0; i < 10; ++i) {
        Label *label = Label::createWithSystemFont(StringUtils::format("%d番", points[i]), "Arial", 12);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        innerNode->addChild(label);
        label->setPosition(Vec2(5.0f, y - 7.0f));
        y -= 14.0f;

        div_t ret = div(counts[i], 4);
        int row = ret.quot + (ret.rem == 0 ? 0 : 1);
        for (size_t k = 0; k < counts[i]; ++k) {
            size_t col = k % 4;
            if (k > 0 && col == 0) {
                y -= 24.0f;
            }
            unsigned idx = beginIndex[i] + k;
            ui::Button *button = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
            innerNode->addChild(button);
            button->setScale9Enabled(true);
            button->setContentSize(Size(66.0f, 20.0f));
            button->setTitleColor(Color3B::BLACK);
            button->setTitleFontSize(12);
            button->setTitleText(points_name[idx]);
            setButtonUnchecked(button);
            button->addClickEventListener(std::bind(&RecordScene::pointsNameCallback, this, std::placeholders::_1, idx));

            button->setPosition(Vec2(gap * (col + 0.5f), y - 12.0f));
        }
        y -= 24.0f;
    }

    ui::ScrollView *scrollView = ui::ScrollView::create();
    scrollView->setDirection(ui::ScrollView::Direction::VERTICAL);
    scrollView->setBounceEnabled(true);
    scrollView->setContentSize(Size(visibleSize.width, visibleSize.height - 320));
    scrollView->setInnerContainerSize(innerNode->getContentSize());
    scrollView->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    scrollView->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f - 120.0f));
    this->addChild(scrollView);

    scrollView->addChild(innerNode);

    _okButton = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png", "source_material/btn_square_disabled.png");
    this->addChild(_okButton);
    _okButton->setScale9Enabled(true);
    _okButton->setContentSize(Size(52.0f, 22.0f));
    _okButton->setTitleText("确定");
    _okButton->setTitleColor(Color3B::BLACK);
    _okButton->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + 15));
    _okButton->addClickEventListener(std::bind(&RecordScene::okCallback, this, std::placeholders::_1));
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
        int winScore = atoi(_editBox->getText());
        if (isButtonChecked(_selfDrawnButton[_winIndex])) {
            for (int i = 0; i < 4; ++i) {
                _scoreTable[i] = (i == _winIndex) ? (winScore + 8) * 3 : (-8 - winScore);
            }
        }
        else {
            for (int i = 0; i < 4; ++i) {
                _scoreTable[i] = (i == _winIndex) ? (winScore + 24) : (isButtonChecked(_claimButton[i]) ? (-8 - winScore) : -8);
            }
        }
    }

    for (int i = 0; i < 4; ++i) {
        if (isButtonChecked(_falseWinButton[i])) {
            _scoreTable[i] -= 30;
            for (int j = 0; j < 4; ++j) {
                if (j == i) continue;
                _scoreTable[j] += 10;
            }
        }
    }

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

    if (_scoreTable[0] + _scoreTable[1] + _scoreTable[2] + _scoreTable[3] == 0) {
        if (isButtonChecked(_drawButton)) {
            _okButton->setEnabled(_pointsFlag == 0);
        }
        else {
            bool allZero = std::all_of(std::begin(_scoreTable), std::end(_scoreTable), [](int score) { return score == 0; });
            _okButton->setEnabled(!allZero);
        }
    }
    else {
        _okButton->setEnabled(false);
    }
}

void RecordScene::minusCallback(cocos2d::Ref *sender) {
    int winScore = atoi(_editBox->getText());
    if (winScore > 8) {
        --winScore;
        _editBox->setText(StringUtils::format("%d", winScore).c_str());
        updateScoreLabel();
    }
}

void RecordScene::plusCallback(cocos2d::Ref *sender) {
    int winScore = atoi(_editBox->getText());
    ++winScore;
    _editBox->setText(StringUtils::format("%d", winScore).c_str());
    updateScoreLabel();
}

void RecordScene::drawCallback(cocos2d::Ref *sender) {
    _winIndex = -1;
    if (isButtonChecked(_drawButton)) {
        setButtonUnchecked(_drawButton);
        for (int i = 0; i < 4; ++i) {
            _winButton[i]->setEnabled(true);
            _selfDrawnButton[i]->setEnabled(true);
            _claimButton[i]->setEnabled(true);
            setButtonUnchecked(_falseWinButton[i]);
            _falseWinButton[i]->setEnabled(true);
            _scoreLabel[i]->setString("+0");
        }
    }
    else {
        setButtonChecked(_drawButton);
        for (int i = 0; i < 4; ++i) {
            setButtonUnchecked(_winButton[i]);
            _winButton[i]->setEnabled(false);
            setButtonUnchecked(_selfDrawnButton[i]);
            _selfDrawnButton[i]->setEnabled(false);
            setButtonUnchecked(_claimButton[i]);
            _claimButton[i]->setEnabled(false);
            setButtonUnchecked(_falseWinButton[i]);
            _falseWinButton[i]->setEnabled(true);
        }
    }
    updateScoreLabel();
}

void RecordScene::winCallback(cocos2d::Ref *sender, int index) {
    setButtonChecked(_winButton[index]);
    if (_winIndex == index) return;

    _winIndex = index;
    for (int i = 0; i < 4; ++i) {
        if (i == index) {
            setButtonChecked(_winButton[i]);
            setButtonUnchecked(_selfDrawnButton[i]);
            _selfDrawnButton[i]->setEnabled(true);
            setButtonUnchecked(_claimButton[i]);
            _claimButton[i]->setEnabled(false);
            setButtonUnchecked(_falseWinButton[i]);
            _falseWinButton[i]->setEnabled(false);
        }
        else {
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

void RecordScene::selfDrawnCallback(cocos2d::Ref *sender, int index) {
    if (_winIndex == -1) return;

    if (isButtonChecked(_selfDrawnButton[index])) {
        setButtonUnchecked(_selfDrawnButton[index]);
        for (int i = 0; i < 4; ++i) {
            _claimButton[i]->setEnabled(index != i);
        }
    }
    else {
        setButtonChecked(_selfDrawnButton[index]);
        for (int i = 0; i < 4; ++i) {
            setButtonUnchecked(_claimButton[i]);
            _claimButton[i]->setEnabled(false);
        }
    }
    updateScoreLabel();
}

void RecordScene::claimCallback(cocos2d::Ref *sender, int index) {
    if (_winIndex == -1) return;

    if (isButtonChecked(_claimButton[index])) {
        for (int i = 0; i < 4; ++i) {
            _claimButton[i]->setEnabled(_winIndex != i);
        }
        setButtonUnchecked(_claimButton[index]);
    }
    else {
        for (int i = 0; i < 4; ++i) {
            _claimButton[i]->setEnabled(index == i);
        }
        setButtonChecked(_claimButton[index]);
    }
    updateScoreLabel();
}

void RecordScene::falseWinCallback(cocos2d::Ref *sender, int index) {
    if (isButtonChecked(_falseWinButton[index])) {
        setButtonUnchecked(_falseWinButton[index]);
    }
    else {
        setButtonChecked(_falseWinButton[index]);
    }
    updateScoreLabel();
}

void RecordScene::pointsNameCallback(cocos2d::Ref *sender, int index) {
    ui::Button *button = (ui::Button *)sender;
    if (isButtonChecked(button)) {
        setButtonUnchecked(button);
        _pointsFlag &= ~(1ULL << index);
    }
    else {
        setButtonChecked(button);
        _pointsFlag |= 1ULL << index;
    }

    int prevWinScore = atoi(_editBox->getText());
    int currentWinScore = 0;
    for (int n = 0; n < 64; ++n) {
        if (_pointsFlag & (1ULL << n)) {
            currentWinScore += points_value_table[n];
        }
    }
    currentWinScore = std::max(8, currentWinScore);
    if (currentWinScore != prevWinScore) {
        _editBox->setText(StringUtils::format("%d", currentWinScore).c_str());
    }
    updateScoreLabel();
}

void RecordScene::okCallback(cocos2d::Ref *sender) {
    _okCallback(this);
    Director::getInstance()->popScene();
}

void RecordScene::onKeyReleased(EventKeyboard::KeyCode keyCode, Event *unusedEvent) {
    if (keyCode == EventKeyboard::KeyCode::KEY_BACK) {
        Director::getInstance()->popScene();
    }
}
