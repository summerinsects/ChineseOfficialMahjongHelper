#include "MahjongTheoryScene.h"
#include "../widget/CWTableView.h"
#include "../mahjong-algorithm/stringify.h"
#include "../mahjong-algorithm/points_calculator.h"

#include "../common.h"
#include "../compiler.h"
#include "../widget/HandTilesWidget.h"
#include "../widget/AlertLayer.h"

#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif

USING_NS_CC;

Scene *MahjongTheoryScene::createScene() {
    auto scene = Scene::create();
    auto layer = MahjongTheoryScene::create();
    scene->addChild(layer);
    return scene;
}

bool MahjongTheoryScene::init() {
    if (!BaseLayer::initWithTitle("牌理（测试版）")) {
        return false;
    }

    Color3B titleColor, textColor;
    const char *normalImage, *selectedImage;
    if (UserDefault::getInstance()->getBoolForKey("night_mode")) {
        titleColor = Color3B::BLACK;
        textColor = Color3B::WHITE;
        normalImage = "source_material/btn_square_normal.png";
        selectedImage = "source_material/btn_square_highlighted.png";
    }
    else {
        titleColor = Color3B::WHITE;
        textColor = Color3B::BLACK;
        normalImage = "source_material/btn_square_highlighted.png";
        selectedImage = "source_material/btn_square_selected.png";
    }

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    ui::Button *button = ui::Button::create(normalImage, selectedImage);
    button->setScale9Enabled(true);
    button->setContentSize(Size(visibleSize.width - 50, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText("在此输入");
    button->setTitleColor(titleColor);
    this->addChild(button);
    button->setPosition(Vec2(origin.x + visibleSize.width * 0.5f - 20, origin.y + visibleSize.height - 50));
    button->addClickEventListener(std::bind(&MahjongTheoryScene::showInputAlert, this, std::placeholders::_1, nullptr));

    button = ui::Button::create(normalImage, selectedImage);
    button->setScale9Enabled(true);
    button->setContentSize(Size(35.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText("计算");
    button->setTitleColor(titleColor);
    this->addChild(button);
    button->setPosition(Vec2(origin.x + visibleSize.width - 25, origin.y + visibleSize.height - 50));
    button->addClickEventListener([this](Ref *) { calculate(); });

    _handTilesWidget = HandTilesWidget::create();
    this->addChild(_handTilesWidget);
    Size widgetSize = _handTilesWidget->getContentSize();

    // 根据情况缩放
    if (widgetSize.width - 4 > visibleSize.width) {
        float scale = (visibleSize.width - 4) / widgetSize.width;
        _handTilesWidget->setScale(scale);
        widgetSize.width *= scale;
        widgetSize.height *= scale;
    }
    _handTilesWidget->setPosition(Vec2(origin.x + widgetSize.width * 0.5f, origin.y + visibleSize.height - 65 - widgetSize.height * 0.5f));

    Label *label = Label::createWithSystemFont("考虑特殊和型", "Arial", 12);
    label->setColor(textColor);
    this->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(origin.x + 10, origin.y + visibleSize.height - 75 - widgetSize.height));

    static const char *title[] = { "七对", "十三幺", "全不靠", "组合龙" };
    const float yPos = origin.y + visibleSize.height - 100 - widgetSize.height;
    const float gap = (visibleSize.width - 4.0f) * 0.25f;
    for (int i = 0; i < 4; ++i) {
        const float xPos = origin.x + gap * (i + 0.5f);
        _checkBoxes[i] = ui::CheckBox::create("source_material/btn_square_normal.png", "", "source_material/btn_square_highlighted.png", "source_material/btn_square_disabled.png", "source_material/btn_square_disabled.png");
        this->addChild(_checkBoxes[i]);
        _checkBoxes[i]->setZoomScale(0.0f);
        _checkBoxes[i]->ignoreContentAdaptWithSize(false);
        _checkBoxes[i]->setContentSize(Size(20.0f, 20.0f));
        _checkBoxes[i]->setPosition(Vec2(xPos - 15, yPos));
        _checkBoxes[i]->setSelected(true);
        _checkBoxes[i]->addEventListener([this](Ref *sender, ui::CheckBox::EventType event) {
            filterResultsByFlag(getFilterFlag());
            _tableView->reloadData();
        });

        label = Label::createWithSystemFont(title[i], "Arial", 12);
        label->setColor(textColor);
        this->addChild(label);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        label->setPosition(Vec2(xPos, yPos));
    }

    _newLineFlag = 15;
    _tableView = cw::TableView::create();
    _tableView->setContentSize(Size(visibleSize.width - 10, visibleSize.height - 120 - widgetSize.height));
    _tableView->setTableViewCallback([this](cw::TableView *table, cw::TableView::CallbackType type, intptr_t param1, intptr_t param2)->intptr_t {
        switch (type) {
        case cw::TableView::CallbackType::CELL_SIZE: {
            if (_resultSources[_orderedIndices[param1]].count_in_tiles < _newLineFlag) {
                *(Size *)param2 = Size(0, 50);
            }
            else {
                *(Size *)param2 = Size(0, 80);
            }
            return 0;
        }
        case cw::TableView::CallbackType::CELL_AT_INDEX:
            return (intptr_t)tableCellAtIndex(table, param1);
        case cw::TableView::CallbackType::NUMBER_OF_CELLS:
            return (intptr_t)_orderedIndices.size();
        }
        return 0;
    });

    _tableView->setDirection(ui::ScrollView::Direction::VERTICAL);
    _tableView->setVerticalFillOrder(cw::TableView::VerticalFillOrder::TOP_DOWN);

    _tableView->setScrollBarPositionFromCorner(Vec2(5, 5));
    _tableView->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    _tableView->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + (visibleSize.height - widgetSize.height) * 0.5f - 55));
    _tableView->reloadData();
    this->addChild(_tableView);

    return true;
}

void MahjongTheoryScene::showInputAlert(cocos2d::Ref *sender, const char *prevInput) {
    Size visibleSize = Director::getInstance()->getVisibleSize();
    const float width = visibleSize.width * 0.8f;

    ui::Widget *rootWidget = ui::Widget::create();

    Label *label = Label::createWithSystemFont("使用说明：\n"
        "1.数牌：万=m 条=s 饼=p。后缀使用小写字母，同花色的数牌可合并用一个后缀。\n"
        "2.字牌：东南西北=ESWN，中发白=CFP。使用大写字母。\n"
        "3.每一组吃、碰、明杠之间用英文空格分隔，每一组暗杠用英文[]。\n"
        "范例1：[EEEE][CCCC][FFFF][PPPP]NN\n"
        "范例2：1112345678999s9s\n"
        "范例3：WWWW 444s 45m678pFF6m\n", "Arial", 10);
    label->setColor(Color3B::BLACK);
    label->setDimensions(width, 0);
    rootWidget->addChild(label);

    // 输入手牌
    ui::EditBox *editBox = ui::EditBox::create(Size(width - 10, 20.0f), ui::Scale9Sprite::create("source_material/btn_square_normal.png"));
    editBox->setInputFlag(ui::EditBox::InputFlag::SENSITIVE);
    editBox->setInputMode(ui::EditBox::InputMode::SINGLE_LINE);
    editBox->setFontColor(Color4B::BLACK);
    editBox->setFontSize(12);
    editBox->setPlaceholderFontColor(Color4B::GRAY);
    editBox->setPlaceHolder("输入手牌");
    if (prevInput != nullptr) {
        editBox->setText(prevInput);
    }

    rootWidget->addChild(editBox);

    const Size &labelSize = label->getContentSize();
    rootWidget->setContentSize(Size(width, labelSize.height + 30));
    editBox->setPosition(Vec2(width * 0.5f, 10));
    label->setPosition(Vec2(width * 0.5f, labelSize.height * 0.5f + 30));

    ui::Button *button = (ui::Button *)sender;
    AlertLayer::showWithNode("输入手牌", rootWidget, [this, editBox, button]() {
        parseInput(button, editBox->getText());
    }, nullptr);
}

bool MahjongTheoryScene::parseInput(cocos2d::ui::Button *button, const char *input) {
    if (*input == '\0') {
        return false;
    }

    const char *errorStr = nullptr;
    const std::string str = input;

    do {
        mahjong::hand_tiles_t hand_tiles;
        mahjong::tile_t win_tile;
        long ret = mahjong::string_to_tiles(input, &hand_tiles, &win_tile);
        if (ret != PARSE_NO_ERROR) {
            switch (ret) {
            case PARSE_ERROR_ILLEGAL_CHARACTER: errorStr = "无法解析的字符"; break;
            case PARSE_ERROR_NO_SUFFIX_AFTER_DIGIT: errorStr = "数字后面需有后缀"; break;
            case PARSE_ERROR_TOO_MANY_TILES_FOR_FIXED_PACK: errorStr = "一组副露包含了过多的牌"; break;
            case PARSE_ERROR_CANNOT_MAKE_FIXED_PACK: errorStr = "无法正确解析副露"; break;
            default: break;
            }
            break;
        }

        ret = mahjong::check_calculator_input(&hand_tiles, win_tile);
        if (ret != 0) {
            switch (ret) {
            case ERROR_WRONG_TILES_COUNT: errorStr = "牌张数错误"; break;
            case ERROR_TILE_COUNT_GREATER_THAN_4: errorStr = "同一种牌最多只能使用4枚"; break;
            default: break;
            }
            break;
        }

        if (button != nullptr) {
            button->setTitleText(str);
        }
        _handTilesWidget->setData(hand_tiles, win_tile);
        return true;
    } while (0);

    if (errorStr != nullptr) {
        AlertLayer::showWithMessage("输入牌", errorStr, [this, str, button]() {
            showInputAlert(button, str.c_str());
        }, nullptr);
    }
    return false;
}

void MahjongTheoryScene::filterResultsByFlag(uint8_t flag) {
    flag |= CONSIDERATION_FLAG_BASIC_TYPE;  // 基本和型不能被过滤掉

    // 从all里面过滤、合并
    for (auto it1 = _allResults.begin(); it1 != _allResults.end(); ++it1) {
        if (!(it1->consideration_flag & flag)) {
            continue;
        }

        // 相同出牌有相同上听数的两个result
        auto it2 = std::find_if(_resultSources.begin(), _resultSources.end(),
            [it1](const ResultEx &result) { return result.discard_tile == it1->discard_tile && result.wait_step == it1->wait_step; });

        if (it2 == _resultSources.end()) {  // 没找到，直接添加it1
            _resultSources.push_back(ResultEx());
            memcpy(&_resultSources.back(), &*it1, sizeof(mahjong::enum_result_t));
            continue;
        }

        // 合并it1 it2
        it2->consideration_flag |= it1->consideration_flag;
        for (int i = 0; i < 34; ++i) {
            mahjong::tile_t t = mahjong::all_tiles[i];
            if (it1->useful_table[t]) {
                it2->useful_table[t] = true;
            }
        }
    }

    // 更新count
    std::for_each(_resultSources.begin(), _resultSources.end(), [this](ResultEx &result) {
        result.count_in_tiles = 0;
        for (int i = 0; i < 34; ++i) {
            mahjong::tile_t t = mahjong::all_tiles[i];
            if (result.useful_table[t]) {
                ++result.count_in_tiles;
            }
        }
        result.count_total = mahjong::count_contributing_tile(_handTilesTable, result.useful_table);
    });

    _orderedIndices.clear();
    if (_resultSources.empty()) {
        return;
    }

    // 指针数组
    std::vector<ResultEx *> temp;
    temp.resize(_resultSources.size());
    std::transform(_resultSources.begin(), _resultSources.end(), temp.begin(), [](ResultEx &r) { return &r; });

    // 排序
    std::sort(temp.begin(), temp.end(), [](ResultEx *a, ResultEx *b) {
        if (a->wait_step < b->wait_step) return true;
        if (a->wait_step > b->wait_step) return false;
        if (a->count_total > b->count_total) return true;
        if (a->count_total < b->count_total) return false;
        if (a->count_in_tiles > b->count_in_tiles) return true;
        if (a->count_in_tiles < b->count_in_tiles) return false;
        if (a->consideration_flag < b->consideration_flag) return true;
        if (a->consideration_flag > b->consideration_flag) return false;
        if (a->discard_tile < b->discard_tile) return true;
        if (a->discard_tile > b->discard_tile) return false;
        return false;
    });

    // 以第一个为标准，过滤掉上听数高的
    int minStep = temp.front()->wait_step;
    temp.erase(
        std::find_if(temp.begin(), temp.end(), [minStep](ResultEx *a) { return a->wait_step > minStep; }),
        temp.end());

    // 转成下标数组
    ResultEx *start = &_resultSources.front();
    _orderedIndices.resize(_resultSources.size());
    std::transform(temp.begin(), temp.end(), _orderedIndices.begin(), [start](ResultEx *p) { return p - start; });
}

uint8_t MahjongTheoryScene::getFilterFlag() const {
    uint8_t flag = CONSIDERATION_FLAG_BASIC_TYPE;
    for (int i = 0; i < 4; ++i) {
        if (_checkBoxes[i]->isSelected()) {
            flag |= 1 << (i + 1);
        }
    }
    return flag;
}

void MahjongTheoryScene::calculate() {
    // 获取牌
    mahjong::hand_tiles_t hand_tiles;
    mahjong::tile_t win_tile;
    _handTilesWidget->getData(&hand_tiles, &win_tile);

    if (hand_tiles.tile_count == 0) {
        return;
    }

    // 打表
    mahjong::map_hand_tiles(&hand_tiles, _handTilesTable);
    if (win_tile != 0) {
        ++_handTilesTable[win_tile];
    }

    _newLineFlag = (win_tile == 0) ? 15 : 10;

    // 计算
    _resultSources.clear();
    mahjong::enum_discard_tile(&hand_tiles, win_tile, CONSIDERATION_FLAG_ALL, this,
        [](void *context, const mahjong::enum_result_t *result) {
        if (result->wait_step != std::numeric_limits<int>::max()) {
            MahjongTheoryScene *thiz = (MahjongTheoryScene *)context;
            thiz->_allResults.push_back(*result);
        }
    });

    filterResultsByFlag(getFilterFlag());
    _tableView->reloadData();
}

void MahjongTheoryScene::onTileButton(cocos2d::Ref *sender) {
    ui::Button *button = (ui::Button *)sender;
    mahjong::tile_t tile = mahjong::all_tiles[button->getTag()];
    size_t realIdx = reinterpret_cast<size_t>(button->getUserData());

    ResultEx *result = &_resultSources[realIdx];
    deduce(result->discard_tile, tile);
}

// 推演
void MahjongTheoryScene::deduce(mahjong::tile_t discardTile, mahjong::tile_t drawnTile) {
    if (discardTile == drawnTile) {
        return;
    }

    // 获取牌
    mahjong::hand_tiles_t handTiles;
    mahjong::tile_t winTile;
    _handTilesWidget->getData(&handTiles, &winTile);

    int cntTable[mahjong::TILE_TABLE_COUNT];
    memcpy(cntTable, _handTilesTable, sizeof(cntTable));

    // 打出牌
    if (discardTile != 0 && cntTable[discardTile] > 0) {
        --cntTable[discardTile];
    }

    const long prevCnt = handTiles.tile_count;
    handTiles.tile_count = mahjong::table_to_tiles(cntTable, handTiles.standing_tiles, 13);
    if (prevCnt != handTiles.tile_count) {
        return;
    }

    _handTilesWidget->setData(handTiles, drawnTile);
    calculate();
}

static std::string getResultTypeString(uint8_t flag, int step) {
    std::string str;
    switch (step) {
    case 0: str = "听牌("; break;
    case -1: str = "和了("; break;
    default: str = StringUtils::format("%d上听(", step); break;
    }

    bool needCaesuraSign = false;
#define APPEND_CAESURA_SIGN_IF_NECESSARY() \
    if (LIKELY(needCaesuraSign)) { str.append("、"); } needCaesuraSign = true

    if (flag & CONSIDERATION_FLAG_BASIC_TYPE) {
        str.append("基本和型");
    }
    if (flag & CONSIDERATION_FLAG_SEVEN_PAIRS) {
        APPEND_CAESURA_SIGN_IF_NECESSARY();
        str.append("七对");
    }
    if (flag & CONSIDERATION_FLAG_THIRTEEN_ORPHANS) {
        APPEND_CAESURA_SIGN_IF_NECESSARY();
        str.append("十三幺");
    }
    if (flag & CONSIDERATION_FLAG_HONORS_AND_KNITTED_TILES) {
        APPEND_CAESURA_SIGN_IF_NECESSARY();
        str.append("全不靠");
    }
    if (flag & CONSIDERATION_FLAG_KNITTED_STRAIGHT) {
        APPEND_CAESURA_SIGN_IF_NECESSARY();
        str.append("组合龙");
    }
#undef APPEND_CAESURA_SIGN_IF_NECESSARY

    str.append(")");
    return str;
}

static int forceinline __isdigit(int c) {
    return (c >= -1 && c <= 255) ? isdigit(c) : 0;
}

// 分割string到两个label
static void spiltStringToLabel(const std::string &str, float width, Label *label1, Label *label2) {
    label2->setVisible(false);
    label1->setString(str);
    const Size &size = label1->getContentSize();
    if (size.width <= width) {  // 宽度允许
        return;
    }

    long utf8Len = StringUtils::getCharacterCountInUTF8String(str);
    long pos = width / size.width * utf8Len;  // 切这么多

    // 切没了，全部放在第2个label上
    if (pos <= 0) {
        label1->setString("");
        label2->setVisible(true);
        label2->setString(str);
    }

    std::string str1 = ui::Helper::getSubStringOfUTF8String(str, 0, pos);
    std::string str2 = ui::Helper::getSubStringOfUTF8String(str, pos, utf8Len);

    // 保证不从数字中间切断
    if (!str2.empty() && __isdigit(str2.front())) {  // 第2个字符串以数字开头
        // 将第1个字符串尾部的数字都转移到第2个字符串头部
        while (pos > 0 && !str1.empty() && __isdigit(str1.back())) {
            --pos;
            str2.insert(0, 1, str1.back());
            str1.pop_back();
        }
    }

    if (pos > 0) {
        label1->setString(str1);
        label2->setVisible(true);
        label2->setString(str2);
    }
    else {
        label1->setString("");
        label2->setVisible(true);
        label2->setString(str);
    }
}

cw::TableViewCell *MahjongTheoryScene::tableCellAtIndex(cw::TableView *table, ssize_t idx) {
    typedef cw::TableViewCellEx<LayerColor *[2], Label *, Label *, Sprite *, Label *, ui::Button *[34], Label *, Label *> CustomCell;
    Size visibleSize = Director::getInstance()->getVisibleSize();

    CustomCell *cell = (CustomCell *)table->dequeueCell();
    if (cell == nullptr) {
        cell = CustomCell::create();

        Color4B bgColor0, bgColor1;
        Color3B textColor;
        if (UserDefault::getInstance()->getBoolForKey("night_mode")) {
            bgColor0 = Color4B(22, 22, 22, 255);
            bgColor1 = Color4B(32, 32, 32, 255);
            textColor = Color3B::WHITE;
        }
        else {
            bgColor0 = Color4B::WHITE;
            bgColor1 = Color4B(239, 243, 247, 255);
            textColor = Color3B::BLACK;
        }

        CustomCell::ExtDataType &ext = cell->getExtData();
        LayerColor *(&layerColor)[2] = std::get<0>(ext);
        Label *&typeLabel = std::get<1>(ext);
        Label *&discardLabel = std::get<2>(ext);
        Sprite *&discardSprite = std::get<3>(ext);
        Label *&usefulLabel = std::get<4>(ext);
        ui::Button *(&usefulButton)[34] = std::get<5>(ext);
        Label *&cntLabel1 = std::get<6>(ext);
        Label *&cntLabel2 = std::get<7>(ext);

        layerColor[0] = LayerColor::create(bgColor0, visibleSize.width - 10, 0);
        cell->addChild(layerColor[0]);
        layerColor[0]->setPosition(Vec2(0, 1));

        layerColor[1] = LayerColor::create(bgColor1, visibleSize.width - 10, 0);
        cell->addChild(layerColor[1]);
        layerColor[1]->setPosition(Vec2(0, 1));

        typeLabel = Label::createWithSystemFont("", "Arial", 12);
        typeLabel->setColor(textColor);
        typeLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        cell->addChild(typeLabel);

        discardLabel = Label::createWithSystemFont("", "Arial", 12);
        discardLabel->setColor(textColor);
        discardLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        cell->addChild(discardLabel);

        discardSprite = Sprite::create("tiles/bg.png");
        discardSprite->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        discardSprite->setScale(15 / discardSprite->getContentSize().width);
        cell->addChild(discardSprite);

        usefulLabel = Label::createWithSystemFont("", "Arial", 12);
        usefulLabel->setColor(textColor);
        usefulLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        cell->addChild(usefulLabel);

        for (int i = 0; i < 34; ++i) {
            ui::Button *button = ui::Button::create(tilesImageName[mahjong::all_tiles[i]]);
            button->setScale(15 / button->getContentSize().width);
            button->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
            cell->addChild(button);
            button->setTag(i);
            button->addClickEventListener(std::bind(&MahjongTheoryScene::onTileButton, this, std::placeholders::_1));
            usefulButton[i] = button;
        }

        cntLabel1 = Label::createWithSystemFont("", "Arial", 12);
        cntLabel1->setColor(textColor);
        cntLabel1->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        cell->addChild(cntLabel1);

        cntLabel2 = Label::createWithSystemFont("", "Arial", 12);
        cntLabel2->setColor(textColor);
        cntLabel2->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        cell->addChild(cntLabel2);
        cntLabel2->setPosition(Vec2(5, 15));
    }

    const CustomCell::ExtDataType &ext = cell->getExtData();
    LayerColor *const (&layerColor)[2] = std::get<0>(ext);
    Label *typeLabel = std::get<1>(ext);
    Label *discardLabel = std::get<2>(ext);
    Sprite *discardSprite = std::get<3>(ext);
    Label *usefulLabel = std::get<4>(ext);
    ui::Button *const (&usefulButton)[34] = std::get<5>(ext);
    Label *cntLabel1 = std::get<6>(ext);
    Label *cntLabel2 = std::get<7>(ext);

    size_t realIdx = _orderedIndices[idx];
    ResultEx *result = &_resultSources[realIdx];

    if (idx & 1) {
        layerColor[0]->setVisible(false);
        layerColor[1]->setVisible(true);
        layerColor[1]->setContentSize(Size(visibleSize.width - 10, result->count_in_tiles < _newLineFlag ? 48 : 78));
    }
    else {
        layerColor[0]->setVisible(true);
        layerColor[1]->setVisible(false);
        layerColor[0]->setContentSize(Size(visibleSize.width - 10, result->count_in_tiles < _newLineFlag ? 48 : 78));
    }

    typeLabel->setString(getResultTypeString(result->consideration_flag, result->wait_step));
    typeLabel->setPosition(Vec2(5, result->count_in_tiles < _newLineFlag ? 40 : 70));

    float xPos = 5;
    float yPos = result->count_in_tiles < _newLineFlag ? 15 : 40;

    if (result->discard_tile != 0) {
        discardLabel->setVisible(true);
        discardSprite->setVisible(true);

        discardLabel->setString("打「");
        discardLabel->setPosition(Vec2(xPos, yPos));
        xPos += discardLabel->getContentSize().width;

        discardSprite->setTexture(Director::getInstance()->getTextureCache()->addImage(tilesImageName[result->discard_tile]));
        discardSprite->setPosition(Vec2(xPos, yPos));
        xPos += 15;

        if (result->wait_step > 0) {
            usefulLabel->setString("」摸「");
        }
        else {
            usefulLabel->setString("」听「");
        }

        usefulLabel->setPosition(Vec2(xPos, yPos));
        xPos += usefulLabel->getContentSize().width;
    }
    else {
        discardLabel->setVisible(false);
        discardSprite->setVisible(false);

        if (result->wait_step != 0) {
            usefulLabel->setString("摸「");
        }
        else {
            usefulLabel->setString("听「");
        }
        usefulLabel->setPosition(Vec2(xPos, yPos));

        xPos += usefulLabel->getContentSize().width;
    }

    int btn = 0;
    for (int i = 0; i < 34; ++i) {
        if (!result->useful_table[mahjong::all_tiles[i]]) {
            usefulButton[i]->setVisible(false);
            continue;
        }

        usefulButton[i]->setUserData(reinterpret_cast<void *>(realIdx));
        usefulButton[i]->setVisible(true);

        if (xPos + 15 > visibleSize.width - 20) {
            xPos = 5;
            yPos -= 25;
        }
        usefulButton[i]->setPosition(Vec2(xPos, yPos));
        xPos += 15;
    }

    std::string str = StringUtils::format("」共%d种，%d枚", result->count_in_tiles, result->count_total);
    if (yPos > 20) {
        spiltStringToLabel(str, visibleSize.width - 20 - xPos, cntLabel1, cntLabel2);
    }
    else {
        cntLabel1->setString(str);
        cntLabel2->setVisible(false);
    }
    cntLabel1->setPosition(Vec2(xPos, yPos));

    return cell;
}
