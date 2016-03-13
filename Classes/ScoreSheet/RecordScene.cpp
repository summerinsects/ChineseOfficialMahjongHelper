#include "RecordScene.h"

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

Scene *RecordScene::createScene(int index, const char **name, const std::function<void (const int (&scores)[4])> &okCallback) {
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

    auto listener = EventListenerKeyboard::create();
    listener->onKeyReleased = CC_CALLBACK_2(Layer::onKeyReleased, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    ui::Button *backBtn = ui::Button::create();
    this->addChild(backBtn);
    backBtn->setScale9Enabled(true);
    backBtn->setContentSize(Size(45, 20));
    backBtn->setTitleFontSize(12);
    backBtn->setTitleText("返回");
    backBtn->setPosition(Vec2(origin.x + 15, origin.y + visibleSize.height - 10));
    backBtn->addClickEventListener(std::bind(&RecordScene::backCallback, this, std::placeholders::_1));

    const char *nameText[] = { "东风东", "东风南", "东风西", "东风北", "南风东", "南风南", "南风西", "南风北",
        "西风东", "西风南", "西风西", "西风北", "北风东", "北风南", "北风西", "北风北" };
    Label *tileLabel = Label::createWithSystemFont(nameText[index], "Arial", 18);
    this->addChild(tileLabel);
    tileLabel->setPosition(Vec2(origin.x + visibleSize.width * 0.5f,
        origin.y + visibleSize.height - tileLabel->getContentSize().height * 0.5f));

    _editBox = ui::EditBox::create(Size(35.0f, 22.0f), ui::Scale9Sprite::create("source_material/tabbar_background1.png"));
    this->addChild(_editBox);
    _editBox->setInputFlag(ui::EditBox::InputFlag::SENSITIVE);
    _editBox->setInputMode(/*ui::EditBox::InputMode::SINGLE_LINE |*/ ui::EditBox::InputMode::NUMERIC);
    _editBox->setFontColor(Color4B(0, 0, 0, 255));
    _editBox->setPosition(Vec2(origin.x + visibleSize.width * 0.5f - 95, origin.y + visibleSize.height - 50));
    _editBox->setDelegate(this);
    _editBox->setText("8");

    Label *label = Label::createWithSystemFont("番", "Arial", 12);
    this->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(origin.x + visibleSize.width * 0.5f - 75, origin.y + visibleSize.height - 50));

    _drawButton = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
    this->addChild(_drawButton);
    _drawButton->setScale9Enabled(true);
    _drawButton->setContentSize(Size(22.0f, 22.0f));
    _drawButton->setPosition(Vec2(origin.x + visibleSize.width * 0.5f + 80, origin.y + visibleSize.height - 50));
    _drawButton->addClickEventListener(std::bind(&RecordScene::drawCallback, this, std::placeholders::_1));

    label = Label::createWithSystemFont("荒庄", "Arial", 12);
    this->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(origin.x + visibleSize.width * 0.5f + 95, origin.y + visibleSize.height - 50));

    const float gap = visibleSize.width * 0.25f;
    for (int i = 0; i < 4; ++i) {
        const float x = origin.x + gap * (i + 0.5f);
        _nameLabel[i] = Label::createWithSystemFont(name[i], "Arial", 12);
        this->addChild(_nameLabel[i]);
        _nameLabel[i]->setPosition(Vec2(x, origin.y + visibleSize.height - 80));

        _scoreLabel[i] = Label::createWithSystemFont("+0", "Arial", 12);
        this->addChild(_scoreLabel[i]);
        _scoreLabel[i]->setPosition(Vec2(x, origin.y + visibleSize.height - 110));

        _winButton[i] = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
        this->addChild(_winButton[i]);
        _winButton[i]->setScale9Enabled(true);
        _winButton[i]->setContentSize(Size(22.0f, 22.0f));
        _winButton[i]->setPosition(Vec2(x - 15, origin.y + visibleSize.height - 140));
        setButtonUnchecked(_winButton[i]);
        _winButton[i]->addClickEventListener(std::bind(&RecordScene::winCallback, this, std::placeholders::_1, i));

        label = Label::createWithSystemFont("和", "Arial", 12);
        this->addChild(label);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        label->setPosition(Vec2(x, origin.y + visibleSize.height - 140));

        _selfDrawnButton[i] = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
        this->addChild(_selfDrawnButton[i]);
        _selfDrawnButton[i]->setScale9Enabled(true);
        _selfDrawnButton[i]->setContentSize(Size(22.0f, 22.0f));
        _selfDrawnButton[i]->setPosition(Vec2(x - 15, origin.y + visibleSize.height - 170));
        setButtonUnchecked(_selfDrawnButton[i]);
        _selfDrawnButton[i]->addClickEventListener(std::bind(&RecordScene::selfDrawnCallback, this, std::placeholders::_1, i));

        label = Label::createWithSystemFont("自摸", "Arial", 12);
        this->addChild(label);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        label->setPosition(Vec2(x, origin.y + visibleSize.height - 170));

        _claimButton[i] = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
        this->addChild(_claimButton[i]);
        _claimButton[i]->setScale9Enabled(true);
        _claimButton[i]->setContentSize(Size(22.0f, 22.0f));
        _claimButton[i]->setPosition(Vec2(x - 15, origin.y + visibleSize.height - 200));
        setButtonUnchecked(_claimButton[i]);
        _claimButton[i]->addClickEventListener(std::bind(&RecordScene::claimCallback, this, std::placeholders::_1, i));

        label = Label::createWithSystemFont("点炮", "Arial", 12);
        this->addChild(label);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        label->setPosition(Vec2(x, origin.y + visibleSize.height - 200));

        _falseWinButton[i] = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
        this->addChild(_falseWinButton[i]);
        _falseWinButton[i]->setScale9Enabled(true);
        _falseWinButton[i]->setContentSize(Size(22.0f, 22.0f));
        _falseWinButton[i]->setPosition(Vec2(x - 15, origin.y + visibleSize.height - 230));
        setButtonUnchecked(_falseWinButton[i]);
        _falseWinButton[i]->addClickEventListener(std::bind(&RecordScene::falseWinCallback, this, std::placeholders::_1, i));

        label = Label::createWithSystemFont("诈和", "Arial", 12);
        this->addChild(label);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        label->setPosition(Vec2(x, origin.y + visibleSize.height - 230));
    }

    ui::Button *okButton = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
    this->addChild(okButton);
    okButton->setScale9Enabled(true);
    okButton->setContentSize(Size(52.0f, 22.0f));
    okButton->setTitleText("确定");
    okButton->setTitleColor(Color3B::BLACK);
    okButton->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + 15));
    okButton->addClickEventListener(std::bind(&RecordScene::okCallback, this, std::placeholders::_1));

    _winIndex = -1;
    return true;
}

void RecordScene::editBoxReturn(cocos2d::ui::EditBox *editBox) {
    updateScoreLabel();
}

void RecordScene::updateScoreLabel() {
    memset(_scoreTable, 0, sizeof(_scoreTable));
    if (_winIndex != -1) {
        const char *str = _editBox->getText();
        int winScore = atoi(str);
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
    }
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

void RecordScene::backCallback(cocos2d::Ref *sender) {
    Director::getInstance()->popScene();
}

void RecordScene::okCallback(cocos2d::Ref *sender) {
    if (_scoreTable[0] + _scoreTable[1] + _scoreTable[2] + _scoreTable[3] != 0) {
        return;
    }
    _okCallback(_scoreTable);
    Director::getInstance()->popScene();
}

void RecordScene::onKeyReleased(EventKeyboard::KeyCode keyCode, Event *unusedEvent) {
    if (keyCode == EventKeyboard::KeyCode::KEY_BACK) {
        Director::getInstance()->popScene();
    }
}
