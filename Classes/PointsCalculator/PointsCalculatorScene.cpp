#include "PointsCalculatorScene.h"
#include "../mahjong-algorithm/points_calculator.h"

#include "../widget/TilePickWidget.h"

USING_NS_CC;

Scene *PointsCalculatorScene::createScene() {
    auto scene = Scene::create();
    auto layer = PointsCalculatorScene::create();
    scene->addChild(layer);
    return scene;
}

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

bool PointsCalculatorScene::init() {
    if (!Layer::init()) {
        return false;
    }

    auto listener = EventListenerKeyboard::create();
    listener->onKeyReleased = CC_CALLBACK_2(Layer::onKeyReleased, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    Label *tileLabel = Label::createWithSystemFont("国标麻将算番器", "Arial", 24);
    this->addChild(tileLabel);
    tileLabel->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height - 12));

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

    _tilePicker = TilePickWidget::create();
    const Size &widgetSize = _tilePicker->getContentSize();
    this->addChild(_tilePicker);

    float y = origin.y + visibleSize.height - 5;
    if (widgetSize.width > visibleSize.width) {
        float scale = visibleSize.width / widgetSize.width;
        _tilePicker->setScale(scale);
        y -= widgetSize.height * scale;
        _tilePicker->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, y - 20 + widgetSize.height * scale * 0.5f));
    }
    else {
        y -= widgetSize.height;
        _tilePicker->setPosition(Vec2(origin.x + widgetSize.width * 0.5f, y - 20 + widgetSize.height * 0.5f));
    }

    ui::Widget *infoWidget = ui::Widget::create();
    infoWidget->setContentSize(Size(visibleSize.width, 120));
    this->addChild(infoWidget);
    infoWidget->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, y - 80));

    _pointsAreaNode = Node::create();
    _pointsAreaNode->setContentSize(Size(visibleSize.width, y - 140));
    _pointsAreaNode->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    this->addChild(_pointsAreaNode);
    _pointsAreaNode->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + y * 0.5f - 70));

    Label *flowerLabel = Label::createWithSystemFont("花牌数", "Arial", 12);
    infoWidget->addChild(flowerLabel);
    flowerLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_RIGHT);
    flowerLabel->setPosition(Vec2(visibleSize.width - 50, 105.0f));

    _editBox = ui::EditBox::create(Size(35.0f, 22.0f), ui::Scale9Sprite::create("source_material/tabbar_background1.png"));
    infoWidget->addChild(_editBox);
    _editBox->setInputFlag(ui::EditBox::InputFlag::SENSITIVE);
    _editBox->setInputMode(ui::EditBox::InputMode::NUMERIC);
    _editBox->setFontColor(Color4B(0, 0, 0, 255));
    _editBox->setText("0");
    _editBox->setPosition(Vec2(visibleSize.width - 30, 105.0f));

    ui::Button *button = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_selected.png", "source_material/btn_square_disabled.png");
    button->setScale9Enabled(true);
    button->setContentSize(Size(34.0f, 22.0f));
    button->setTitleFontSize(12);
    button->setTitleText("算番");
    button->setTitleColor(Color3B::BLACK);
    infoWidget->addChild(button);
    button->setPosition(Vec2(visibleSize.width - 30, 75.0f));
    button->addClickEventListener([this](Ref *) { calculate(); });

    const char *windName[4] = {"东", "南", "西", "北"};

    Label *prevalentWindLabel = Label::createWithSystemFont("圈风", "Arial", 12);
    infoWidget->addChild(prevalentWindLabel);
    prevalentWindLabel->setPosition(Vec2(20.0f, 105.0f));

    for (int i = 0; i < 4; ++i) {
        _prevalentButton[i] = ui::Button::create("source_material/btn_square_normal.png", "", "source_material/btn_square_highlighted.png");
        infoWidget->addChild(_prevalentButton[i]);
        _prevalentButton[i]->setScale9Enabled(true);
        _prevalentButton[i]->setContentSize(Size(22.0f, 22.0f));
        _prevalentButton[i]->setTitleColor(Color3B::BLACK);
        _prevalentButton[i]->setTitleFontSize(12);
        _prevalentButton[i]->setTitleText(windName[i]);
        _prevalentButton[i]->setPosition(Vec2(50.0f + i * 30, 105.0f));
        _prevalentButton[i]->addClickEventListener([this, i](Ref *) {
            for (int n = 0; n < 4; ++n) {
                _prevalentButton[n]->setEnabled(n != i);
            }
        });
    }
    _prevalentButton[0]->setEnabled(false);

    Label *seatWindLabel = Label::createWithSystemFont("门风", "Arial", 12);
    infoWidget->addChild(seatWindLabel);
    seatWindLabel->setPosition(Vec2(20.0f, 75.0f));

    for (int i = 0; i < 4; ++i) {
        _seatButton[i] = ui::Button::create("source_material/btn_square_normal.png", "", "source_material/btn_square_highlighted.png");
        infoWidget->addChild(_seatButton[i]);
        _seatButton[i]->setScale9Enabled(true);
        _seatButton[i]->setContentSize(Size(22.0f, 22.0f));
        _seatButton[i]->setTitleColor(Color3B::BLACK);
        _seatButton[i]->setTitleFontSize(12);
        _seatButton[i]->setTitleText(windName[i]);
        _seatButton[i]->setPosition(Vec2(50.0f + i * 30, 75.0f));
        _seatButton[i]->addClickEventListener([this, i](Ref *) {
            for (int n = 0; n < 4; ++n) {
                _seatButton[n]->setEnabled(n != i);
            }
        });
    }
    _seatButton[0]->setEnabled(false);

    float gapX = (visibleSize.width - 8) * 0.25f;
    _byDiscardButton = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
    infoWidget->addChild(_byDiscardButton);
    _byDiscardButton->setScale9Enabled(true);
    _byDiscardButton->setContentSize(Size(22.0f, 22.0f));
    _byDiscardButton->setPosition(Vec2(20.0f, 45.0f));
    setButtonChecked(_byDiscardButton);
    _byDiscardButton->addClickEventListener([this](Ref *) {
        if (isButtonChecked(_byDiscardButton)) {
            setButtonUnchecked(_byDiscardButton);
        }
        else {
            setButtonChecked(_byDiscardButton);
            setButtonUnchecked(_selfDrawnButton);
            setButtonUnchecked(_replacementButton);
            setButtonUnchecked(_lastTileDrawnButton);
        }
    });

    Label *byDiscardLabel = Label::createWithSystemFont("点和", "Arial", 12);
    infoWidget->addChild(byDiscardLabel);
    byDiscardLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    byDiscardLabel->setPosition(Vec2(35.0f, 45.0f));

    _selfDrawnButton = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
    infoWidget->addChild(_selfDrawnButton);
    _selfDrawnButton->setScale9Enabled(true);
    _selfDrawnButton->setContentSize(Size(22.0f, 22.0f));
    _selfDrawnButton->setPosition(Vec2(20.0f + gapX, 45.0f));
    setButtonUnchecked(_selfDrawnButton);
    _selfDrawnButton->addClickEventListener([this](Ref *) {
        if (isButtonChecked(_selfDrawnButton)) {
            setButtonUnchecked(_selfDrawnButton);
        }
        else {
            setButtonChecked(_selfDrawnButton);
            setButtonUnchecked(_byDiscardButton);
            setButtonUnchecked(_robKongButton);
            setButtonUnchecked(_lastTileClaimButton);
        }
    });

    Label *selfDrawnLabel = Label::createWithSystemFont("自摸", "Arial", 12);
    infoWidget->addChild(selfDrawnLabel);
    selfDrawnLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    selfDrawnLabel->setPosition(Vec2(35.0f + gapX, 45.0f));

    _fourthTileButton = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
    infoWidget->addChild(_fourthTileButton);
    _fourthTileButton->setScale9Enabled(true);
    _fourthTileButton->setContentSize(Size(22.0f, 22.0f));
    _fourthTileButton->setPosition(Vec2(20.0f + gapX * 2, 45.0f));
    setButtonUnchecked(_fourthTileButton);
    _fourthTileButton->addClickEventListener([this](Ref *) {
        if (isButtonChecked(_fourthTileButton)) {
            setButtonUnchecked(_fourthTileButton);
        }
        else {
            setButtonChecked(_fourthTileButton);
            setButtonUnchecked(_robKongButton);
        }
    });

    Label *fourthTileLabel = Label::createWithSystemFont("绝张", "Arial", 12);
    infoWidget->addChild(fourthTileLabel);
    fourthTileLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    fourthTileLabel->setPosition(Vec2(35.0f + gapX * 2, 45.0f));

    _replacementButton = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
    infoWidget->addChild(_replacementButton);
    _replacementButton->setScale9Enabled(true);
    _replacementButton->setContentSize(Size(22.0f, 22.0f));
    _replacementButton->setPosition(Vec2(20.0f, 15.0f));
    setButtonUnchecked(_replacementButton);
    _replacementButton->addClickEventListener([this](Ref *) {
        if (isButtonChecked(_replacementButton)) {
            setButtonUnchecked(_replacementButton);
        }
        else {
            setButtonChecked(_replacementButton);
            setButtonUnchecked(_robKongButton);
            setButtonUnchecked(_lastTileClaimButton);
        }
    });

    Label *replacementLabel = Label::createWithSystemFont("杠开", "Arial", 12);
    infoWidget->addChild(replacementLabel);
    replacementLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    replacementLabel->setPosition(Vec2(35.0f, 15.0f));

    _robKongButton = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
    infoWidget->addChild(_robKongButton);
    _robKongButton->setScale9Enabled(true);
    _robKongButton->setContentSize(Size(22.0f, 22.0f));
    _robKongButton->setPosition(Vec2(20.0f + gapX, 15.0f));
    setButtonUnchecked(_robKongButton);
    _robKongButton->addClickEventListener([this](Ref *) {
        if (isButtonChecked(_robKongButton)) {
            setButtonUnchecked(_robKongButton);
        }
        else {
            setButtonChecked(_robKongButton);
            setButtonUnchecked(_selfDrawnButton);
            setButtonUnchecked(_fourthTileButton);
            setButtonUnchecked(_replacementButton);
            setButtonUnchecked(_lastTileDrawnButton);
        }
    });

    Label *robKongLabel = Label::createWithSystemFont("抢杠", "Arial", 12);
    infoWidget->addChild(robKongLabel);
    robKongLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    robKongLabel->setPosition(Vec2(35.0f + gapX, 15.0f));

    _lastTileDrawnButton = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
    infoWidget->addChild(_lastTileDrawnButton);
    _lastTileDrawnButton->setScale9Enabled(true);
    _lastTileDrawnButton->setContentSize(Size(22.0f, 22.0f));
    _lastTileDrawnButton->setPosition(Vec2(20.0f + gapX * 2, 15.0f));
    setButtonUnchecked(_lastTileDrawnButton);
    _lastTileDrawnButton->addClickEventListener([this](Ref *) {
        if (isButtonChecked(_lastTileDrawnButton)) {
            setButtonUnchecked(_lastTileDrawnButton);
        }
        else {
            setButtonChecked(_lastTileDrawnButton);
            setButtonUnchecked(_byDiscardButton);
            setButtonUnchecked(_selfDrawnButton);
            setButtonUnchecked(_robKongButton);
            setButtonUnchecked(_lastTileClaimButton);
        }
    });

    Label *lastTileDrawnLabel = Label::createWithSystemFont("妙手", "Arial", 12);
    infoWidget->addChild(lastTileDrawnLabel);
    lastTileDrawnLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    lastTileDrawnLabel->setPosition(Vec2(35.0f + gapX * 2, 15.0f));

    _lastTileClaimButton = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
    infoWidget->addChild(_lastTileClaimButton);
    _lastTileClaimButton->setScale9Enabled(true);
    _lastTileClaimButton->setContentSize(Size(22.0f, 22.0f));
    _lastTileClaimButton->setPosition(Vec2(20.0f + gapX * 3, 15.0f));
    setButtonUnchecked(_lastTileClaimButton);
    _lastTileClaimButton->addClickEventListener([this](Ref *) {
        if (isButtonChecked(_lastTileClaimButton)) {
            setButtonUnchecked(_lastTileClaimButton);
        }
        else {
            setButtonChecked(_lastTileClaimButton);
            setButtonUnchecked(_byDiscardButton);
            setButtonUnchecked(_selfDrawnButton);
            setButtonUnchecked(_replacementButton);
            setButtonUnchecked(_lastTileDrawnButton);
        }
    });

    Label *lastTileClaimLabel = Label::createWithSystemFont("海底", "Arial", 12);
    infoWidget->addChild(lastTileClaimLabel);
    lastTileClaimLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    lastTileClaimLabel->setPosition(Vec2(35.0f + gapX * 3, 15.0f));

    return true;
}

#define FONT_SIZE 14

void PointsCalculatorScene::calculate() {
    _pointsAreaNode->removeAllChildren();

    Size visibleSize = Director::getInstance()->getVisibleSize();
    const Size &pointsAreaSize = _pointsAreaNode->getContentSize();
    Vec2 pos(pointsAreaSize.width * 0.5f, pointsAreaSize.height * 0.5f);

    std::string errorStr;
    do {
        int flowerCnt = atoi(_editBox->getText());
        if (flowerCnt > 8) {
            errorStr = "花牌数不得超过8";
            break;
        }

        SET sets[5];
        long set_cnt = std::copy(_tilePicker->getFixedSets().begin(), _tilePicker->getFixedSets().end(), std::begin(sets))
            - std::begin(sets);

        TILE tiles[13];
        long tile_cnt = std::copy(_tilePicker->getHandTiles().begin(), _tilePicker->getHandTiles().end(), std::begin(tiles))
            - std::begin(tiles);
        sort_tiles(tiles, tile_cnt);

        TILE win_tile = _tilePicker->getWinTile();

        long points_table[FLOWER_TILES + 1] = { 0 };
        WIN_TYPE win_type = WIN_TYPE_DISCARD;
        if (_selfDrawnButton->isHighlighted()) win_type |= WIN_TYPE_SELF_DRAWN;
        if (_fourthTileButton->isHighlighted()) win_type |= WIN_TYPE_4TH_TILE;
        if (_robKongButton->isHighlighted()) win_type |= WIN_TYPE_ABOUT_KONG;
        if (_replacementButton->isHighlighted()) win_type |= (WIN_TYPE_ABOUT_KONG | WIN_TYPE_SELF_DRAWN);
        if (_lastTileDrawnButton->isHighlighted()) win_type |= (WIN_TYPE_WALL_LAST | WIN_TYPE_SELF_DRAWN);
        if (_lastTileClaimButton->isHighlighted()) win_type |= WIN_TYPE_WALL_LAST;

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
        if (points == ERROR_NOT_WIN) {
            errorStr = "诈和";
            break;
        }
        if (points == ERROR_WRONG_TILES_COUNT) {
            errorStr = "相公";
            break;
        }

        points += flowerCnt;
        points_table[FLOWER_TILES] = flowerCnt;
        int n = FLOWER_TILES + 1 - std::count(std::begin(points_table), std::end(points_table), 0);
        long rows = n / 2 + n % 2;
        Node *innerNode = Node::create();
        float pointsAreaHeight = (FONT_SIZE + 2) * (rows + 2);
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
            div_t ret = div(i, 2);
            pointName->setPosition(Vec2(ret.rem == 0 ? 5.0f : visibleSize.width * 0.5f + 5.0f, (FONT_SIZE + 2) * (rows - ret.quot + 2)));
        }
        Label *pointTotal = Label::createWithSystemFont(StringUtils::format("总计：%d番", points), "Arial", FONT_SIZE);
        innerNode->addChild(pointTotal);
        pointTotal->setAnchorPoint(Vec2::ANCHOR_TOP_LEFT);
        pointTotal->setPosition(Vec2(5.0f, FONT_SIZE + 2));

        if (pointsAreaHeight <= pointsAreaSize.height) {
            _pointsAreaNode->addChild(innerNode);
            innerNode->setAnchorPoint(Vec2(0.5f, 0.5f));
            innerNode->setPosition(pos);
        }
        else {
            ui::ScrollView *scrollView = ui::ScrollView::create();
            scrollView->setDirection(ui::ScrollView::Direction::VERTICAL);
            scrollView->setBounceEnabled(true);
            scrollView->setContentSize(pointsAreaSize);
            scrollView->setInnerContainerSize(innerNode->getContentSize());
            scrollView->addChild(innerNode);

            _pointsAreaNode->addChild(scrollView);
            scrollView->setAnchorPoint(Vec2(0.5f, 0.5f));
            scrollView->setPosition(pos);
        }

        return;

    } while (0);

    Label *errorLabel = Label::createWithSystemFont(errorStr, "Arial", FONT_SIZE);
    _pointsAreaNode->addChild(errorLabel);
    errorLabel->setPosition(pos);
}

void PointsCalculatorScene::onKeyReleased(EventKeyboard::KeyCode keyCode, Event *unusedEvent) {
    if (keyCode == EventKeyboard::KeyCode::KEY_BACK) {
        Director::getInstance()->popScene();
    }
}
