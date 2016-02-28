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

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    Label *tileLabel = Label::createWithSystemFont("国标麻将算番器", "Arial", 26);
    this->addChild(tileLabel);

    float x = origin.x + visibleSize.width * 0.5f;
    float y = origin.y + visibleSize.height - tileLabel->getContentSize().height * 0.5f;
    tileLabel->setPosition(Vec2(x, y));

    _editBox = ui::EditBox::create(Size(visibleSize.width - 50, 30.0f), ui::Scale9Sprite::create("source_material/tabbar_background1.png"));
    this->addChild(_editBox);
    _editBox->setInputFlag(ui::EditBox::InputFlag::SENSITIVE);
    _editBox->setFontColor(Color4B(0, 0, 0, 255));
    _editBox->setPosition(Vec2(x - 20, y - 40));

    ui::Button *button = ui::Button::create("source_material/btn_rounded_normal.png");
    button->setTitleText("Go");
    button->setTitleColor(Color3B::BLACK);
    this->addChild(button);
    button->setPosition(Vec2(origin.x + visibleSize.width - 20, y - 40));
    button->addClickEventListener([this](Ref *) { calculate(); });

    const char *windName[4] = {"东", "南", "西", "北"};
    Label *prevalentWindLabel = Label::createWithSystemFont("圈风", "Arial", 16);
    this->addChild(prevalentWindLabel);
    prevalentWindLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    prevalentWindLabel->setPosition(Vec2(10.0f, y - 80));

    for (int i = 0; i < 4; ++i) {
        _prevalentBox[i] = ui::CheckBox::create("source_material/btn_rounded_normal.png",
            "source_material/btn_rounded_selected.png");
        this->addChild(_prevalentBox[i]);
        _prevalentBox[i]->setPosition(Vec2(70.0f + i * 50, y - 80));
        _prevalentBox[i]->addEventListener([this, i](Ref *, ui::CheckBox::EventType event) {
            switch (event) {
            case cocos2d::ui::CheckBox::EventType::SELECTED:
                for (int n = 0; n < 4; ++n) {
                    if (n != i) _prevalentBox[n]->setSelected(false);
                }
                break;
            case cocos2d::ui::CheckBox::EventType::UNSELECTED:
                _prevalentBox[i]->setSelected(true);
                break;
            default:
                break;
            }
        });

        Label *label = Label::createWithSystemFont(windName[i], "Arial", 16);
        label->setTextColor(Color4B::BLACK);
        _prevalentBox[i]->addChild(label);
        const Size &size = _prevalentBox[i]->getContentSize();
        label->setPosition(Vec2(size.width * 0.5f, size.height * 0.5f));
    }
    _prevalentBox[0]->setSelected(true);

    Label *seatWindLabel = Label::createWithSystemFont("门风", "Arial", 16);
    this->addChild(seatWindLabel);
    seatWindLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    seatWindLabel->setPosition(Vec2(10.0f, y - 120));

    for (int i = 0; i < 4; ++i) {
        _seatBox[i] = ui::CheckBox::create("source_material/btn_rounded_normal.png", "source_material/btn_rounded_selected.png");
        this->addChild(_seatBox[i]);
        _seatBox[i]->setPosition(Vec2(70.0f + i * 50, y - 120));
        _seatBox[i]->addEventListener([this, i](Ref *, ui::CheckBox::EventType event) {
            switch (event) {
            case cocos2d::ui::CheckBox::EventType::SELECTED:
                for (int n = 0; n < 4; ++n) {
                    if (n != i) _seatBox[n]->setSelected(false);
                }
                break;
            case cocos2d::ui::CheckBox::EventType::UNSELECTED:
                _seatBox[i]->setSelected(true);
                break;
            default:
                break;
            }
        });

        Label *label = Label::createWithSystemFont(windName[i], "Arial", 16);
        label->setTextColor(Color4B::BLACK);
        _seatBox[i]->addChild(label);
        const Size &size = _seatBox[i]->getContentSize();
        label->setPosition(Vec2(size.width * 0.5f, size.height * 0.5f));
    }
    _seatBox[0]->setSelected(true);

    _byDiscardBox = ui::CheckBox::create("source_material/btn_rounded_normal.png", "source_material/btn_rounded_selected.png");
    this->addChild(_byDiscardBox);
    _byDiscardBox->setPosition(Vec2(25.0f, y - 160));
    _byDiscardBox->setSelected(true);

    Label *byDiscardLabel = Label::createWithSystemFont("点和", "Arial", 16);
    this->addChild(byDiscardLabel);
    byDiscardLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    byDiscardLabel->setPosition(Vec2(45.0f, y - 160));

    _selfDrawnBox = ui::CheckBox::create("source_material/btn_rounded_normal.png", "source_material/btn_rounded_selected.png");
    this->addChild(_selfDrawnBox);
    _selfDrawnBox->setPosition(Vec2(95.0f, y - 160));

    Label *selfDrawnLabel = Label::createWithSystemFont("自摸", "Arial", 16);
    this->addChild(selfDrawnLabel);
    selfDrawnLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    selfDrawnLabel->setPosition(Vec2(115.0f, y - 160));

    _fourthTileBox = ui::CheckBox::create("source_material/btn_rounded_normal.png", "source_material/btn_rounded_selected.png");
    this->addChild(_fourthTileBox);
    _fourthTileBox->setPosition(Vec2(165.0f, y - 160));

    Label *fourthTileLabel = Label::createWithSystemFont("和绝张", "Arial", 16);
    this->addChild(fourthTileLabel);
    fourthTileLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    fourthTileLabel->setPosition(Vec2(185.0f, y - 160));

    _replacementBox = ui::CheckBox::create("source_material/btn_rounded_normal.png", "source_material/btn_rounded_selected.png");
    this->addChild(_replacementBox);
    _replacementBox->setPosition(Vec2(25.0f, y - 200));

    Label *replacementLabel = Label::createWithSystemFont("杠上开花", "Arial", 16);
    this->addChild(replacementLabel);
    replacementLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    replacementLabel->setPosition(Vec2(45.0f, y - 200));

    _robKongBox = ui::CheckBox::create("source_material/btn_rounded_normal.png", "source_material/btn_rounded_selected.png");
    this->addChild(_robKongBox);
    _robKongBox->setPosition(Vec2(95.0f, y - 200));

    Label *robKongLabel = Label::createWithSystemFont("抢杠和", "Arial", 16);
    this->addChild(robKongLabel);
    robKongLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    robKongLabel->setPosition(Vec2(115.0f, y - 200));

    _lastTileDrawnBox = ui::CheckBox::create("source_material/btn_rounded_normal.png", "source_material/btn_rounded_selected.png");
    this->addChild(_lastTileDrawnBox);
    _lastTileDrawnBox->setPosition(Vec2(165.0f, y - 200));

    Label *lastTileDrawnLabel = Label::createWithSystemFont("妙手回春", "Arial", 16);
    this->addChild(lastTileDrawnLabel);
    lastTileDrawnLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    lastTileDrawnLabel->setPosition(Vec2(185.0f, y - 200));

    _lastTileChaimBox = ui::CheckBox::create("source_material/btn_rounded_normal.png", "source_material/btn_rounded_selected.png");
    this->addChild(_lastTileChaimBox);
    _lastTileChaimBox->setPosition(Vec2(235.0f, y - 200));

    Label *lastTileChaimLabel = Label::createWithSystemFont("海底捞月", "Arial", 16);
    this->addChild(lastTileChaimLabel);
    lastTileChaimLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    lastTileChaimLabel->setPosition(Vec2(255.0f, y - 200));

    Label *tipsLabel = Label::createWithSystemFont("使用说明：\n"
        "1.万条饼分别用m s p作为后缀。同花色的数牌可以合并用一个后缀。\n"
        "2.东南西北中发白分别用大写字母ESWNCFP表示\n"
        "3.每一组副露（即吃、碰、明杠）用英文[]括起来，每一组暗杠用英文{}括起来\n"
        "4.请将所有副露放在最前面，然后是立牌，和牌用英文逗号与手牌分隔开\n"
        "范例1：{EEEE}{CCCC}{FFFF}{PPPP}N,N\n"
        "范例2：1112345678999s,9s\n",
        "Arial", 10, Size(visibleSize.width, 0.0f));
    this->addChild(tipsLabel);
    tipsLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_TOP);
    tipsLabel->setPosition(Vec2(origin.x + visibleSize.width * 0.5f,
        origin.y + visibleSize.height - 240));

    _pointsAreaSize = Size(visibleSize.width,
        visibleSize.height - 240 - tipsLabel->getContentSize().height);

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

    float x = origin.x + visibleSize.width * 0.5f;

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
        if (_selfDrawnBox->isSelected()) win_type |= WIN_TYPE_SELF_DRAWN;
        if (_fourthTileBox->isSelected()) win_type |= WIN_TYPE_4TH_TILE;
        if (_robKongBox->isSelected()) win_type |= WIN_TYPE_ABOUT_KONG;
        if (_replacementBox->isSelected()) win_type |= (WIN_TYPE_ABOUT_KONG | WIN_TYPE_SELF_DRAWN);
        if (_lastTileDrawnBox->isSelected()) win_type |= (WIN_TYPE_WALL_LAST | WIN_TYPE_SELF_DRAWN);
        if (_lastTileChaimBox->isSelected()) win_type |= WIN_TYPE_WALL_LAST;

        WIND_TYPE prevalent_wind = WIND_TYPE::EAST, seat_wind = WIND_TYPE::EAST;
        for (int i = 0; i < 4; ++i) {
            if (_prevalentBox[i]->isSelected()) {
                prevalent_wind = static_cast<WIND_TYPE>(static_cast<int>(WIND_TYPE::EAST) + i);
            }
        }
        for (int i = 0; i < 4; ++i) {
            if (_seatBox[i]->isSelected()) {
                seat_wind = static_cast<WIND_TYPE>(static_cast<int>(WIND_TYPE::EAST) + i);
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

        if (pointsAreaHeight <= _pointsAreaSize.height) {
            innerNode->setAnchorPoint(Vec2(0.5f, 0.5f));
            innerNode->setPosition(Vec2(x, origin.y + _pointsAreaSize.height * 0.5f));
            this->addChild(innerNode);
            _pointsAreaNode = innerNode;
        }
        else {
            ui::ScrollView *scrollView = ui::ScrollView::create();
            scrollView->setDirection(ui::ScrollView::Direction::VERTICAL);
            scrollView->setBounceEnabled(true);
            scrollView->setContentSize(_pointsAreaSize);
            scrollView->setInnerContainerSize(innerNode->getContentSize());
            scrollView->setAnchorPoint(Vec2(0.5f, 0.5f));
            scrollView->setPosition(Vec2(x, origin.y + _pointsAreaSize.height * 0.5f));
            this->addChild(scrollView);

            scrollView->addChild(innerNode);
            _pointsAreaNode = innerNode;
        }

        return;

    } while (0);

    Label *errorLabel = Label::createWithSystemFont("无法正确解析牌", "Arial", FONT_SIZE);
    errorLabel->setPosition(Vec2(x, origin.y + _pointsAreaSize.height * 0.5f));
    this->addChild(errorLabel);
    _pointsAreaNode = errorLabel;
}

void PointsCalculatorScene::onKeyReleased(EventKeyboard::KeyCode keyCode, Event* unused_event) {
    MessageBox("123", "456");
}
