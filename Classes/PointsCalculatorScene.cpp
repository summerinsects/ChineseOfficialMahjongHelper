#include "PointsCalculatorScene.h"

#pragma execution_character_set("utf-8")

#include "mahjong-algorithm/points_calculator.h"

USING_NS_CC;

Scene *PointsCalculatorScene::createScene() {
    auto scene = Scene::create();
    auto layer = PointsCalculatorScene::create();
    scene->addChild(layer);
    return scene;
}

bool PointsCalculatorScene::init() {
    if (!Layer::init()) {
        return false;
    }
    setKeyboardEnabled(true);

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    Label *tileLabel = Label::createWithSystemFont("国标麻将算番器", "Arial", 24);
    this->addChild(tileLabel);

    float x = origin.x + visibleSize.width * 0.5f;
    float y = origin.y + visibleSize.height - tileLabel->getContentSize().height * 0.5f;
    tileLabel->setPosition(Vec2(x, y));

    _editBox = ui::EditBox::create(Size(visibleSize.width - 50, 20.0f), ui::Scale9Sprite::create("source_material/tabbar_background1.png"));
    this->addChild(_editBox);
    _editBox->setInputFlag(ui::EditBox::InputFlag::SENSITIVE);
    _editBox->setInputMode(ui::EditBox::InputMode::SINGLE_LINE);
    _editBox->setFontColor(Color4B(0, 0, 0, 255));
    _editBox->setPosition(Vec2(x - 20, y - 30));

    ui::Button *button = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_selected.png", "source_material/btn_square_disabled.png");
    button->setScale9Enabled(true);
    button->setContentSize(Size(28.0f, 22.0f));
    button->setTitleText("算番");
    button->setTitleColor(Color3B::BLACK);
    this->addChild(button);
    button->setPosition(Vec2(origin.x + visibleSize.width - 20, y - 30));
    button->addClickEventListener([this](Ref *) { calculate(); });

    const char *windName[4] = {"东", "南", "西", "北"};

    Label *prevalentWindLabel = Label::createWithSystemFont("圈风", "Arial", 12);
    this->addChild(prevalentWindLabel);
    prevalentWindLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    prevalentWindLabel->setPosition(Vec2(origin.x + 10.0f, y - 60));

    for (int i = 0; i < 4; ++i) {
        _prevalentButton[i] = ui::Button::create("source_material/btn_square_normal.png", "", "source_material/btn_square_highlighted.png");
        this->addChild(_prevalentButton[i]);
        _prevalentButton[i]->setScale9Enabled(true);
        _prevalentButton[i]->setContentSize(Size(22.0f, 22.0f));
        _prevalentButton[i]->setPosition(Vec2(origin.x + 70.0f + i * 50, y - 60));
        _prevalentButton[i]->addClickEventListener([this, i](Ref *) {
            for (int n = 0; n < 4; ++n) {
                _prevalentButton[n]->setEnabled(n != i);
            }
        });

        Label *label = Label::createWithSystemFont(windName[i], "Arial", 12);
        label->setTextColor(Color4B::BLACK);
        _prevalentButton[i]->addChild(label);
        const Size &size = _prevalentButton[i]->getContentSize();
        label->setPosition(Vec2(size.width * 0.5f, size.height * 0.5f));
    }
    _prevalentButton[0]->setEnabled(false);

    Label *seatWindLabel = Label::createWithSystemFont("门风", "Arial", 12);
    this->addChild(seatWindLabel);
    seatWindLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    seatWindLabel->setPosition(Vec2(origin.x + 10.0f, y - 90));

    for (int i = 0; i < 4; ++i) {
        _seatButton[i] = ui::Button::create("source_material/btn_square_normal.png", "", "source_material/btn_square_highlighted.png");
        this->addChild(_seatButton[i]);
        _seatButton[i]->setScale9Enabled(true);
        _seatButton[i]->setContentSize(Size(22.0f, 22.0f));
        _seatButton[i]->setPosition(Vec2(origin.x + 70.0f + i * 50, y - 90));
        _seatButton[i]->addClickEventListener([this, i](Ref *) {
            for (int n = 0; n < 4; ++n) {
                _seatButton[n]->setEnabled(n != i);
            }
        });

        Label *label = Label::createWithSystemFont(windName[i], "Arial", 12);
        label->setTextColor(Color4B::BLACK);
        _seatButton[i]->addChild(label);
        const Size &size = _seatButton[i]->getContentSize();
        label->setPosition(Vec2(size.width * 0.5f, size.height * 0.5f));
    }
    _seatButton[0]->setEnabled(false);

    _byDiscardButton = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
    this->addChild(_byDiscardButton);
    _byDiscardButton->setScale9Enabled(true);
    _byDiscardButton->setContentSize(Size(22.0f, 22.0f));
    _byDiscardButton->setPosition(Vec2(origin.x + 25.0f, y - 120));
    _byDiscardButton->setHighlighted(true);
    _byDiscardButton->setTag(1);
    _byDiscardButton->addClickEventListener([this](Ref *) {
        if (_byDiscardButton->getTag() == 1) {
            _byDiscardButton->setTag(0);
            _byDiscardButton->setHighlighted(false);
        }
        else {
            _byDiscardButton->setTag(1);
            _byDiscardButton->setHighlighted(true);

            _selfDrawnButton->setTag(0);
            _selfDrawnButton->setHighlighted(false);

            _replacementButton->setTag(0);
            _replacementButton->setHighlighted(false);

            _lastTileDrawnButton->setTag(0);
            _lastTileDrawnButton->setHighlighted(false);
        }
    });

    Label *byDiscardLabel = Label::createWithSystemFont("点和", "Arial", 12);
    this->addChild(byDiscardLabel);
    byDiscardLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    byDiscardLabel->setPosition(Vec2(origin.x + 40.0f, y - 120));

    _selfDrawnButton = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
    this->addChild(_selfDrawnButton);
    _selfDrawnButton->setScale9Enabled(true);
    _selfDrawnButton->setContentSize(Size(22.0f, 22.0f));
    _selfDrawnButton->setPosition(Vec2(origin.x + 95.0f, y - 120));
    _selfDrawnButton->setHighlighted(false);
    _selfDrawnButton->setTag(0);
    _selfDrawnButton->addClickEventListener([this](Ref *) {
        if (_selfDrawnButton->getTag() == 1) {
            _selfDrawnButton->setTag(0);
            _selfDrawnButton->setHighlighted(false);
        }
        else {
            _selfDrawnButton->setTag(1);
            _selfDrawnButton->setHighlighted(true);

            _byDiscardButton->setTag(0);
            _byDiscardButton->setHighlighted(false);

            _robKongButton->setTag(0);
            _robKongButton->setHighlighted(false);

            _lastTileChaimButton->setTag(0);
            _lastTileChaimButton->setHighlighted(false);
        }
    });

    Label *selfDrawnLabel = Label::createWithSystemFont("自摸", "Arial", 12);
    this->addChild(selfDrawnLabel);
    selfDrawnLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    selfDrawnLabel->setPosition(Vec2(origin.x + 110.0f, y - 120));

    _fourthTileButton = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
    this->addChild(_fourthTileButton);
    _fourthTileButton->setScale9Enabled(true);
    _fourthTileButton->setContentSize(Size(22.0f, 22.0f));
    _fourthTileButton->setPosition(Vec2(origin.x + 165.0f, y - 120));
    _fourthTileButton->setHighlighted(false);
    _fourthTileButton->setTag(0);
    _fourthTileButton->addClickEventListener([this](Ref *) {
        if (_fourthTileButton->getTag() == 1) {
            _fourthTileButton->setTag(0);
            _fourthTileButton->setHighlighted(false);
        }
        else {
            _fourthTileButton->setTag(1);
            _fourthTileButton->setHighlighted(true);

            _robKongButton->setTag(0);
            _robKongButton->setHighlighted(false);
        }
    });

    Label *fourthTileLabel = Label::createWithSystemFont("和绝张", "Arial", 12);
    this->addChild(fourthTileLabel);
    fourthTileLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    fourthTileLabel->setPosition(Vec2(origin.x + 180.0f, y - 120));

    _replacementButton = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
    this->addChild(_replacementButton);
    _replacementButton->setScale9Enabled(true);
    _replacementButton->setContentSize(Size(22.0f, 22.0f));
    _replacementButton->setPosition(Vec2(origin.x + 25.0f, y - 150));
    _replacementButton->setHighlighted(false);
    _replacementButton->setTag(0);
    _replacementButton->addClickEventListener([this](Ref *) {
        if (_replacementButton->getTag() == 1) {
            _replacementButton->setTag(0);
            _replacementButton->setHighlighted(false);
        }
        else {
            _replacementButton->setTag(1);
            _replacementButton->setHighlighted(true);

            _robKongButton->setTag(0);
            _robKongButton->setHighlighted(false);

            _lastTileChaimButton->setTag(0);
            _lastTileChaimButton->setHighlighted(false);
        }
    });

    Label *replacementLabel = Label::createWithSystemFont("杠上开花", "Arial", 12);
    this->addChild(replacementLabel);
    replacementLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    replacementLabel->setPosition(Vec2(origin.x + 40.0f, y - 150));

    _robKongButton = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
    this->addChild(_robKongButton);
    _robKongButton->setScale9Enabled(true);
    _robKongButton->setContentSize(Size(22.0f, 22.0f));
    _robKongButton->setPosition(Vec2(origin.x + 95.0f, y - 150));
    _robKongButton->setHighlighted(false);
    _robKongButton->setTag(0);
    _robKongButton->addClickEventListener([this](Ref *) {
        if (_robKongButton->getTag() == 1) {
            _robKongButton->setTag(0);
            _robKongButton->setHighlighted(false);
        }
        else {
            _robKongButton->setTag(1);
            _robKongButton->setHighlighted(true);

            _selfDrawnButton->setTag(0);
            _selfDrawnButton->setHighlighted(false);

            _fourthTileButton->setTag(0);
            _fourthTileButton->setHighlighted(false);

            _replacementButton->setTag(0);
            _replacementButton->setHighlighted(false);

            _lastTileDrawnButton->setTag(0);
            _lastTileDrawnButton->setHighlighted(false);

            _lastTileChaimButton->setTag(0);
            _lastTileChaimButton->setHighlighted(false);
        }
    });

    Label *robKongLabel = Label::createWithSystemFont("抢杠和", "Arial", 12);
    this->addChild(robKongLabel);
    robKongLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    robKongLabel->setPosition(Vec2(origin.x + 110.0f, y - 150));

    _lastTileDrawnButton = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
    this->addChild(_lastTileDrawnButton);
    _lastTileDrawnButton->setScale9Enabled(true);
    _lastTileDrawnButton->setContentSize(Size(22.0f, 22.0f));
    _lastTileDrawnButton->setPosition(Vec2(origin.x + 165.0f, y - 150));
    _lastTileDrawnButton->setHighlighted(false);
    _lastTileDrawnButton->setTag(0);
    _lastTileDrawnButton->addClickEventListener([this](Ref *) {
        if (_lastTileDrawnButton->getTag() == 1) {
            _lastTileDrawnButton->setTag(0);
            _lastTileDrawnButton->setHighlighted(false);
        }
        else {
            _lastTileDrawnButton->setTag(1);
            _lastTileDrawnButton->setHighlighted(true);

            _byDiscardButton->setTag(0);
            _byDiscardButton->setHighlighted(false);

            _selfDrawnButton->setTag(0);
            _selfDrawnButton->setHighlighted(false);

            _robKongButton->setTag(0);
            _robKongButton->setHighlighted(false);

            _lastTileChaimButton->setTag(0);
            _lastTileChaimButton->setHighlighted(false);
        }
    });

    Label *lastTileDrawnLabel = Label::createWithSystemFont("妙手回春", "Arial", 12);
    this->addChild(lastTileDrawnLabel);
    lastTileDrawnLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    lastTileDrawnLabel->setPosition(Vec2(origin.x + 180.0f, y - 150));

    _lastTileChaimButton = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
    this->addChild(_lastTileChaimButton);
    _lastTileChaimButton->setScale9Enabled(true);
    _lastTileChaimButton->setContentSize(Size(22.0f, 22.0f));
    _lastTileChaimButton->setPosition(Vec2(origin.x + 235.0f, y - 150));
    _lastTileChaimButton->setHighlighted(false);
    _lastTileChaimButton->setTag(0);
    _lastTileChaimButton->addClickEventListener([this](Ref *) {
        if (_lastTileChaimButton->getTag() == 1) {
            _lastTileChaimButton->setTag(0);
            _lastTileChaimButton->setHighlighted(false);
        }
        else {
            _lastTileChaimButton->setTag(1);
            _lastTileChaimButton->setHighlighted(true);

            _byDiscardButton->setTag(0);
            _byDiscardButton->setHighlighted(false);

            _selfDrawnButton->setTag(0);
            _selfDrawnButton->setHighlighted(false);

            _replacementButton->setTag(0);
            _replacementButton->setHighlighted(false);

            _robKongButton->setTag(0);
            _robKongButton->setHighlighted(false);

            _lastTileDrawnButton->setTag(0);
            _lastTileDrawnButton->setHighlighted(false);
        }
    });

    Label *lastTileChaimLabel = Label::createWithSystemFont("海底捞月", "Arial", 12);
    this->addChild(lastTileChaimLabel);
    lastTileChaimLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    lastTileChaimLabel->setPosition(Vec2(origin.x + 250.0f, y - 150));

    Label *tipsLabel = Label::createWithSystemFont("使用说明：\n"
        "1.万条饼分别用小写m s p作为后缀。同花色的数牌可以合并用一个后缀。\n"
        "2.东南西北中发白分别用大写字母ESWNCFP表示\n"
        "3.每一组副露（即吃、碰、明杠）用英文[]括起来，每一组暗杠用英文{}括起来\n"
        "4.请将所有副露放在最前面，然后是立牌，和牌用英文逗号与手牌分隔开\n"
        "范例1：{EEEE}{CCCC}{FFFF}{PPPP}N,N\n"
        "范例2：1112345678999s,9s\n",
        "Arial", 10, Size(visibleSize.width, 0.0f));
    this->addChild(tipsLabel);
    tipsLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_BOTTOM);
    tipsLabel->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y));

    _pointsAreaBottom = origin.y + tipsLabel->getContentSize().height + 10;
    _pointsAreaTop = origin.y + y - 170;

    return true;
}

#define FONT_SIZE 18

void PointsCalculatorScene::calculate() {
    if (_pointsAreaNode != nullptr) {
        _pointsAreaNode->removeFromParent();
        _pointsAreaNode = nullptr;
    }

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();
    Size pointsAreaSize(visibleSize.width, _pointsAreaTop - _pointsAreaBottom);

    Vec2 pos(origin.x + visibleSize.width * 0.5f, origin.y + _pointsAreaBottom + pointsAreaSize.height * 0.5f);

    do {
        std::string str = _editBox->getText();
        std::string::iterator it = std::find(str.begin(), str.end(), ',');
        if (it == str.end()) {
            break;
        }
        std::string tilesString(str.begin(), it);
        std::string winString(it + 1, str.end());

        SET sets[5];
        long set_cnt;
        TILE tiles[13];
        long tile_cnt;
        if (!string_to_tiles(tilesString.c_str(), sets, &set_cnt, tiles, &tile_cnt)) {
            break;
        }

        TILE win_tile;
        const char *p = parse_tiles(winString.c_str(), &win_tile, nullptr);
        if (*p != '\0') {
            break;
        }

        long points_table[FLOWER_TILES] = { 0 };
        WIN_TYPE win_type = WIN_TYPE_DISCARD;
        if (_selfDrawnButton->isHighlighted()) win_type |= WIN_TYPE_SELF_DRAWN;
        if (_fourthTileButton->isHighlighted()) win_type |= WIN_TYPE_4TH_TILE;
        if (_robKongButton->isHighlighted()) win_type |= WIN_TYPE_ABOUT_KONG;
        if (_replacementButton->isHighlighted()) win_type |= (WIN_TYPE_ABOUT_KONG | WIN_TYPE_SELF_DRAWN);
        if (_lastTileDrawnButton->isHighlighted()) win_type |= (WIN_TYPE_WALL_LAST | WIN_TYPE_SELF_DRAWN);
        if (_lastTileChaimButton->isHighlighted()) win_type |= WIN_TYPE_WALL_LAST;

        WIND_TYPE prevalent_wind = WIND_TYPE::EAST, seat_wind = WIND_TYPE::EAST;
        for (int i = 0; i < 4; ++i) {
            if (!_prevalentButton[i]->isEnabled()) {
                prevalent_wind = static_cast<WIND_TYPE>(static_cast<int>(WIND_TYPE::EAST) + i);
                break;
            }
        }
        for (int i = 0; i < 4; ++i) {
            if (!_seatButton[i]->isEnabled()) {
                seat_wind = static_cast<WIND_TYPE>(static_cast<int>(WIND_TYPE::EAST) + i);
                break;
            }
        }

        int points = calculate_points(sets, set_cnt, tiles, tile_cnt, win_tile, win_type, prevalent_wind, seat_wind, points_table);

        int n = FLOWER_TILES - std::count(std::begin(points_table), std::end(points_table), 0);
        Node *innerNode = Node::create();
        float pointsAreaHeight = (FONT_SIZE + 2)* (n + 2);
        innerNode->setContentSize(Size(visibleSize.width, pointsAreaHeight));

        for (int i = 0, j = 0; i < n; ++i) {
            while (points_table[++j] == 0) continue;

            std::string str;
            if (points_table[j] == 1) {
                str = StringUtils::format("%s %d\n", points_name[j], points_value_table[j]);
            }
            else {
                str = StringUtils::format("%s %d*%ld\n", points_name[j], points_value_table[j], points_table[j]);
            }

            Label *pointName = Label::createWithSystemFont(str, "Arial", FONT_SIZE);
            innerNode->addChild(pointName);
            pointName->setAnchorPoint(Vec2::ANCHOR_TOP_LEFT);
            pointName->setPosition(Vec2(5.0f, (FONT_SIZE + 2) * (n - i + 2)));
        }
        str = StringUtils::format("总计：%d番", points);
        Label *pointTotal = Label::createWithSystemFont(str, "Arial", FONT_SIZE);
        innerNode->addChild(pointTotal);
        pointTotal->setAnchorPoint(Vec2::ANCHOR_TOP_LEFT);
        pointTotal->setPosition(Vec2(5.0f, FONT_SIZE + 2));

        if (pointsAreaHeight <= pointsAreaSize.height) {
            innerNode->setAnchorPoint(Vec2(0.5f, 0.5f));
            innerNode->setPosition(pos);
            this->addChild(innerNode);
            _pointsAreaNode = innerNode;
        }
        else {
            ui::ScrollView *scrollView = ui::ScrollView::create();
            scrollView->setDirection(ui::ScrollView::Direction::VERTICAL);
            scrollView->setBounceEnabled(true);
            scrollView->setContentSize(pointsAreaSize);
            scrollView->setInnerContainerSize(innerNode->getContentSize());
            scrollView->setAnchorPoint(Vec2(0.5f, 0.5f));
            scrollView->setPosition(pos);
            this->addChild(scrollView);

            scrollView->addChild(innerNode);
            _pointsAreaNode = innerNode;
        }

        return;

    } while (0);

    Label *errorLabel = Label::createWithSystemFont("无法正确解析牌", "Arial", FONT_SIZE);
    errorLabel->setPosition(pos);
    this->addChild(errorLabel);
    _pointsAreaNode = errorLabel;
}

void PointsCalculatorScene::onKeyReleased(EventKeyboard::KeyCode keyCode, Event* unused_event) {
    if (keyCode == EventKeyboard::KeyCode::KEY_BACK) {
        Director::getInstance()->end();

#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
        exit(0);
#endif
    }
}
