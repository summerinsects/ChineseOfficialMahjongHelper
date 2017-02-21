#ifdef _MSC_VER
#pragma warning(disable: 4351)
#endif

#include "MahjongTheoryScene.h"
#include "../widget/CWTableView.h"
#include "../mahjong-algorithm/stringify.h"
#include "../mahjong-algorithm/fan_calculator.h"

#include "../common.h"
#include "../compiler.h"
#include "../widget/HandTilesWidget.h"
#include "../widget/AlertView.h"
#include "../widget/LoadingView.h"

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

    _editBox = ui::EditBox::create(Size(visibleSize.width - 95, 20.0f), ui::Scale9Sprite::create("source_material/btn_square_normal.png"));
    this->addChild(_editBox);
    _editBox->setInputFlag(ui::EditBox::InputFlag::SENSITIVE);
    _editBox->setInputMode(ui::EditBox::InputMode::SINGLE_LINE);
    _editBox->setFontColor(Color4B::BLACK);
    _editBox->setFontSize(12);
    _editBox->setPlaceholderFontColor(Color4B::GRAY);
    _editBox->setPlaceHolder("在此处输入");
    _editBox->setDelegate(this);
    _editBox->setPosition(Vec2(origin.x + visibleSize.width * 0.5f - 40, origin.y + visibleSize.height - 50));

    ui::Button *button = ui::Button::create(normalImage, selectedImage);
    button->setScale9Enabled(true);
    button->setContentSize(Size(35.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText("随机");
    button->setTitleColor(titleColor);
    this->addChild(button);
    button->setPosition(Vec2(origin.x + visibleSize.width - 65, origin.y + visibleSize.height - 50));
    button->addClickEventListener([this](Ref *) { setRandomInput(); });

    button = ui::Button::create(normalImage, selectedImage);
    button->setScale9Enabled(true);
    button->setContentSize(Size(35.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText("说明");
    button->setTitleColor(titleColor);
    this->addChild(button);
    button->setPosition(Vec2(origin.x + visibleSize.width - 25, origin.y + visibleSize.height - 50));
    button->addClickEventListener(std::bind(&MahjongTheoryScene::onGuideButton, this, std::placeholders::_1));

    _handTilesWidget = HandTilesWidget::create();
    _handTilesWidget->setTileClickCallback(std::bind(&MahjongTheoryScene::onStandingTileEvent, this));
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

#if 0
    _undoButton = ui::Button::create(normalImage, selectedImage);
    _undoButton->setScale9Enabled(true);
    _undoButton->setContentSize(Size(35.0f, 20.0f));
    _undoButton->setTitleFontSize(12);
    _undoButton->setTitleText("撤销");
    _undoButton->setTitleColor(titleColor);
    this->addChild(_undoButton);
    _undoButton->setPosition(Vec2(origin.x + visibleSize.width - 65, origin.y + visibleSize.height - 75 - widgetSize.height));
    _undoButton->addClickEventListener(std::bind(&MahjongTheoryScene::onUndoButton, this, std::placeholders::_1));
    _undoButton->setEnabled(false);

    _redoButton = ui::Button::create(normalImage, selectedImage);
    _redoButton->setScale9Enabled(true);
    _redoButton->setContentSize(Size(35.0f, 20.0f));
    _redoButton->setTitleFontSize(12);
    _redoButton->setTitleText("重做");
    _redoButton->setTitleColor(titleColor);
    this->addChild(_redoButton);
    _redoButton->setPosition(Vec2(origin.x + visibleSize.width - 25, origin.y + visibleSize.height - 75 - widgetSize.height));
    _redoButton->addClickEventListener(std::bind(&MahjongTheoryScene::onRedoButton, this, std::placeholders::_1));
    _redoButton->setEnabled(false);
#endif

    Label *label = Label::createWithSystemFont("考虑特殊和型", "Arial", 12);
    label->setColor(textColor);
    this->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(origin.x + 10, origin.y + visibleSize.height - 80 - widgetSize.height));

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

    // 预先算好Cell及各label的Size
    _cellWidth = visibleSize.width - 10;
    Label *tempLabel = Label::createWithSystemFont("打「", "Arial", 12);
    _discardLabelWidth = tempLabel->getContentSize().width;
    tempLabel->setString("」摸「"); 
    _servingLabelWidth1 = tempLabel->getContentSize().width;
    tempLabel->setString("摸「");
    _servingLabelWidth2 = tempLabel->getContentSize().width;
    tempLabel->setString("」听「");
    _waitingLabelWidth1 = tempLabel->getContentSize().width;
    tempLabel->setString("听「");
    _waitingLabelWidth2 = tempLabel->getContentSize().width;
    tempLabel->setString("」共0种，00枚");
    _totalLabelWidth = tempLabel->getContentSize().width;

    _tableView = cw::TableView::create();
    _tableView->setContentSize(Size(visibleSize.width - 10, visibleSize.height - 120 - widgetSize.height));
    _tableView->setTableViewCallback([this](cw::TableView *table, cw::TableView::CallbackType type, intptr_t param1, intptr_t param2)->intptr_t {
        switch (type) {
        case cw::TableView::CallbackType::CELL_SIZE: {
            *(Size *)param2 = tableCellSizeAtIndex(table, param1);
            return 0;
        }
        case cw::TableView::CallbackType::CELL_AT_INDEX:
            return (intptr_t)tableCellAtIndex(table, param1);
        case cw::TableView::CallbackType::NUMBER_OF_CELLS:
            return (intptr_t)_orderedIndices.size();
        default:
            break;
        }
        return 0;
    });

    _tableView->setDirection(ui::ScrollView::Direction::VERTICAL);
    _tableView->setVerticalFillOrder(cw::TableView::VerticalFillOrder::TOP_DOWN);

    _tableView->setScrollBarPositionFromCorner(Vec2(5, 5));
    _tableView->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    _tableView->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + (visibleSize.height - widgetSize.height) * 0.5f - 55));
    this->addChild(_tableView);

    srand((unsigned)time(nullptr));
    setRandomInput();

    return true;
}

void MahjongTheoryScene::onGuideButton(cocos2d::Ref *sender) {
    Size visibleSize = Director::getInstance()->getVisibleSize();
    const float width = visibleSize.width * 0.8f - 10;

    Label *label = Label::createWithSystemFont(
        "1." INPUT_GUIDE_STRING_1 "\n"
        "2." INPUT_GUIDE_STRING_2 "\n"
        "3." INPUT_GUIDE_STRING_3 "\n"
        "4.基本和型暂不考虑国标番型。\n"
        "5.暂不考虑吃碰杠操作。\n"
        "6.点击表格中的有效牌，可切出该切法的弃牌，并上指定牌。\n"
        "7.点击手牌可切出对应牌，随机上牌。\n"
        "输入范例1：[EEEE]288s349pSCFF2p\n"
        "输入范例2：123p 345s 999s 6m6pEW1m\n"
        "输入范例3：356m18s1579pWNFF9p\n", "Arial", 10);
    label->setColor(Color3B::BLACK);
    label->setDimensions(width, 0);
    AlertView::showWithNode("使用说明", label, nullptr, nullptr);
}

void MahjongTheoryScene::setRandomInput() {
    // 随机生成一副牌
    mahjong::hand_tiles_t handTiles;
    handTiles.pack_count = 0;
    handTiles.tile_count = 13;

    int table[34] = {0};

    // 立牌
    int cnt = 0;
    do {
        int n = rand() % 34;
        if (table[n] < 4) {
            ++table[n];
            handTiles.standing_tiles[cnt++] = mahjong::all_tiles[n];
        }
    } while (cnt < handTiles.tile_count);
    std::sort(handTiles.standing_tiles, handTiles.standing_tiles + handTiles.tile_count);

    // 上牌
    mahjong::tile_t servingTile = 0;
    do {
        int n = rand() % 34;
        if (table[n] < 4) {
            ++table[n];
            servingTile = mahjong::all_tiles[n];
        }
    } while (servingTile == 0);

    // 设置UI
    _handTilesWidget->setData(handTiles, servingTile);

    char str[64];
    long ret = mahjong::hand_tiles_to_string(&handTiles, str, sizeof(str));
    mahjong::tiles_to_string(&servingTile, 1, str + ret, sizeof(str) - ret);
    _editBox->setText(str);

    _allResults.clear();
    _resultSources.clear();
    _orderedIndices.clear();

    _tableView->reloadData();

    calculate();
}

void MahjongTheoryScene::editBoxReturn(cocos2d::ui::EditBox *editBox) {
    if (parseInput(editBox->getText())) {
        _allResults.clear();
        _resultSources.clear();
        _orderedIndices.clear();

        _tableView->reloadData();

        calculate();
    }
}

bool MahjongTheoryScene::parseInput(const char *input) {
    if (*input == '\0') {
        return false;
    }

    const char *errorStr = nullptr;
    const std::string str = input;

    do {
        mahjong::hand_tiles_t hand_tiles = { 0 };
        mahjong::tile_t serving_tile = 0;
        long ret = mahjong::string_to_tiles(input, &hand_tiles, &serving_tile);
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

        if (hand_tiles.tile_count < 13) {
            switch (hand_tiles.tile_count % 3) {
            case 2:
                // 将最后一张作为上牌，不需要break
                serving_tile = hand_tiles.standing_tiles[--hand_tiles.tile_count];
            case 1:
                // 修正副露组数，以便骗过检查
                hand_tiles.pack_count = 4 - hand_tiles.tile_count / 3;
                break;
            default:
                break;
            }
        }

        ret = mahjong::check_calculator_input(&hand_tiles, serving_tile);
        if (ret != 0) {
            switch (ret) {
            case ERROR_WRONG_TILES_COUNT: errorStr = "牌张数错误"; break;
            case ERROR_TILE_COUNT_GREATER_THAN_4: errorStr = "同一种牌最多只能使用4枚"; break;
            default: break;
            }
            break;
        }

        _handTilesWidget->setData(hand_tiles, serving_tile);
        return true;
    } while (0);

    if (errorStr != nullptr) {
        AlertView::showWithMessage("提示", errorStr, [this]() {
            _editBox->touchDownAction(_editBox, ui::Widget::TouchEventType::ENDED);
        }, nullptr);
    }
    return false;
}

void MahjongTheoryScene::filterResultsByFlag(uint8_t flag) {
    flag |= FORM_FLAG_BASIC_TYPE;  // 基本和型不能被过滤掉

    _resultSources.clear();
    _orderedIndices.clear();

    // 从all里面过滤、合并
    for (auto it1 = _allResults.begin(); it1 != _allResults.end(); ++it1) {
        if (!(it1->form_flag & flag)) {
            continue;
        }

        // 相同出牌有相同上听数的两个result
        auto it2 = std::find_if(_resultSources.begin(), _resultSources.end(),
            [it1](const ResultEx &result) { return result.discard_tile == it1->discard_tile && result.shanten == it1->shanten; });

        if (it2 == _resultSources.end()) {  // 没找到，直接到resultSources
            _resultSources.push_back(ResultEx());
            memcpy(&_resultSources.back(), &*it1, sizeof(mahjong::enum_result_t));
            continue;
        }

        // 找到，则合并resultSources与allResults的和牌形式标记及有效牌
        it2->form_flag |= it1->form_flag;
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
        result.count_total = mahjong::count_useful_tile(_handTilesTable, result.useful_table);
    });

    if (_resultSources.empty()) {
        return;
    }

    // 指针数组
    std::vector<ResultEx *> temp;
    temp.resize(_resultSources.size());
    std::transform(_resultSources.begin(), _resultSources.end(), temp.begin(), [](ResultEx &r) { return &r; });

    // 排序
    std::sort(temp.begin(), temp.end(), [](ResultEx *a, ResultEx *b) {
        if (a->shanten < b->shanten) return true;
        if (a->shanten > b->shanten) return false;
        if (a->count_total > b->count_total) return true;
        if (a->count_total < b->count_total) return false;
        if (a->count_in_tiles > b->count_in_tiles) return true;
        if (a->count_in_tiles < b->count_in_tiles) return false;
        if (a->form_flag < b->form_flag) return true;
        if (a->form_flag > b->form_flag) return false;
        if (a->discard_tile < b->discard_tile) return true;
        if (a->discard_tile > b->discard_tile) return false;
        return false;
    });

    // 以第一个为标准，过滤掉上听数高的
    int minStep = temp.front()->shanten;
    temp.erase(
        std::find_if(temp.begin(), temp.end(), [minStep](ResultEx *a) { return a->shanten > minStep; }),
        temp.end());

    // 转成下标数组
    ResultEx *start = &_resultSources.front();
    _orderedIndices.resize(temp.size());
    std::transform(temp.begin(), temp.end(), _orderedIndices.begin(), [start](ResultEx *p) { return p - start; });
}

// 获取过滤标记
uint8_t MahjongTheoryScene::getFilterFlag() const {
    uint8_t flag = FORM_FLAG_BASIC_TYPE;
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
    mahjong::tile_t serving_tile;
    _handTilesWidget->getData(&hand_tiles, &serving_tile);

    if (hand_tiles.tile_count == 0) {
        return;
    }

    // 打表
    mahjong::map_hand_tiles(&hand_tiles, _handTilesTable);
    if (serving_tile != 0) {
        ++_handTilesTable[serving_tile];
    }

    // 异步计算
    _allResults.clear();

    LoadingView *loadingView = LoadingView::create();
    this->addChild(loadingView);
    loadingView->setPosition(Director::getInstance()->getVisibleOrigin());

    auto thiz = RefPtr<MahjongTheoryScene>(this);  // 保证线程回来之前不析构
    std::thread([thiz, hand_tiles, serving_tile, loadingView]() {
        mahjong::enum_discard_tile(&hand_tiles, serving_tile, FORM_FLAG_ALL, thiz.get(),
            [](void *context, const mahjong::enum_result_t *result) {
            MahjongTheoryScene *thiz = (MahjongTheoryScene *)context;
            if (result->shanten != std::numeric_limits<int>::max()) {
                thiz->_allResults.push_back(*result);
            }
            return (thiz->getParent() != nullptr);
        });

        // 调回cocos线程
        Director::getInstance()->getScheduler()->performFunctionInCocosThread([thiz, loadingView]() {
            if (thiz->getParent() != nullptr) {
                thiz->filterResultsByFlag(thiz->getFilterFlag());
                thiz->_tableView->reloadData();
                loadingView->removeFromParent();
            }
        });
    }).detach();
}

mahjong::tile_t MahjongTheoryScene::serveRandomTile(mahjong::tile_t discardTile) const {
    // 没用到的牌表
    int remainTable[mahjong::TILE_TABLE_SIZE];
    std::transform(std::begin(_handTilesTable), std::end(_handTilesTable), std::begin(remainTable),
        [](int n) { return 4 - n; });
    //--remainTable[discardTile];  // 有必要吗？

    // 没用到的牌
    mahjong::tile_t remainTiles[136];
    long remainCnt = mahjong::table_to_tiles(remainTable, remainTiles, 136);

    // 随机给一张牌
    mahjong::tile_t servingTile;
    do {
        servingTile = remainTiles[rand() % remainCnt];
    } while (servingTile == discardTile);

    return servingTile;
}

void MahjongTheoryScene::onTileButton(cocos2d::Ref *sender) {
    ui::Button *button = (ui::Button *)sender;
    size_t realIdx = reinterpret_cast<size_t>(button->getUserData());
    mahjong::tile_t discardTile = _resultSources[realIdx].discard_tile;
    mahjong::tile_t servingTile;

    int tag = button->getTag();
    if (tag != INVALID_TAG) {
        servingTile = mahjong::all_tiles[tag];
    }
    else {
        servingTile = serveRandomTile(discardTile);
    }

    deduce(discardTile, servingTile);
#if 0
    _undoCache.push_back(std::make_pair(discardTile, servingTile));
    _redoCache.clear();
    _undoButton->setEnabled(true);
    _redoButton->setEnabled(false);
#endif
}

void MahjongTheoryScene::onStandingTileEvent() {
    mahjong::tile_t discardTile = _handTilesWidget->getCurrentTile();
    if (discardTile == 0 || _handTilesWidget->getServingTile() == 0) {
        return;
    }

    mahjong::tile_t servingTile = serveRandomTile(discardTile);

    // 推演
    deduce(discardTile, servingTile);
#if 0
    _undoCache.push_back(std::make_pair(discardTile, servingTile));
    _redoCache.clear();
    _undoButton->setEnabled(true);
    _redoButton->setEnabled(false);
#endif
}

#if 0
void MahjongTheoryScene::onUndoButton(cocos2d::Ref *sender) {
    if (!_undoCache.empty()) {
        deduce(_undoCache.back().second, _undoCache.back().first);
        _redoCache.push_back(_undoCache.back());
        _undoCache.pop_back();
        _undoButton->setEnabled(!_undoCache.empty());
        _redoButton->setEnabled(true);
    }
}

void MahjongTheoryScene::onRedoButton(cocos2d::Ref *sender) {
    if (!_redoCache.empty()) {
        deduce(_redoCache.back().first, _redoCache.back().second);
        _undoCache.push_back(_redoCache.back());
        _redoCache.pop_back();
        _undoButton->setEnabled(true);
        _redoButton->setEnabled(!_redoCache.empty());
    }
}
#endif

// 推演
void MahjongTheoryScene::deduce(mahjong::tile_t discardTile, mahjong::tile_t servingTile) {
    if (discardTile == servingTile) {
        return;
    }

    // 获取牌
    mahjong::hand_tiles_t handTiles;
    mahjong::tile_t st;
    _handTilesWidget->getData(&handTiles, &st);

    // 对立牌打表
    int cntTable[mahjong::TILE_TABLE_SIZE];
    mahjong::map_tiles(handTiles.standing_tiles, handTiles.tile_count, cntTable);
    ++cntTable[st];  // 当前上牌

    // 打出牌
    if (discardTile != 0 && cntTable[discardTile] > 0) {
        --cntTable[discardTile];
    }

    // 从表恢复成立牌
    const long prevCnt = handTiles.tile_count;
    handTiles.tile_count = mahjong::table_to_tiles(cntTable, handTiles.standing_tiles, 13);
    if (prevCnt != handTiles.tile_count) {
        return;
    }

    // 设置UI
    _handTilesWidget->setData(handTiles, servingTile);

    char str[64];
    long ret = mahjong::hand_tiles_to_string(&handTiles, str, sizeof(str));
    mahjong::tiles_to_string(&servingTile, 1, str + ret, sizeof(str) - ret);
    _editBox->setText(str);

    // 计算
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

    if (flag & FORM_FLAG_BASIC_TYPE) {
        needCaesuraSign = true;
        str.append("基本和型");
    }
    if (flag & FORM_FLAG_SEVEN_PAIRS) {
        APPEND_CAESURA_SIGN_IF_NECESSARY();
        str.append("七对");
    }
    if (flag & FORM_FLAG_THIRTEEN_ORPHANS) {
        APPEND_CAESURA_SIGN_IF_NECESSARY();
        str.append("十三幺");
    }
    if (flag & FORM_FLAG_HONORS_AND_KNITTED_TILES) {
        APPEND_CAESURA_SIGN_IF_NECESSARY();
        str.append("全不靠");
    }
    if (flag & FORM_FLAG_KNITTED_STRAIGHT) {
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

#define SPACE 2
#define TILE_WIDTH 15

cocos2d::Size MahjongTheoryScene::tableCellSizeAtIndex(cw::TableView *table, ssize_t idx) {
    size_t realIdx = _orderedIndices[idx];
    const ResultEx *result = &_resultSources[realIdx];  // 当前cell的数据

    const uint16_t key = ((!!result->discard_tile) << 9) | ((!!result->shanten) << 8) | result->count_in_tiles;
    std::unordered_map<uint16_t, int>::iterator it = _cellHeightMap.find(key);
    if (it != _cellHeightMap.end()) {
        return Size(0, it->second);
    }

    const float cellWidth = _cellWidth - SPACE * 2;  // 前后各留2像素
    float remainWidth;  // 第一行除了前面一些label之后剩下宽度
    if (result->discard_tile != 0) {
        remainWidth = cellWidth - _discardLabelWidth - TILE_WIDTH -
            (result->shanten > 0 ? _servingLabelWidth1 : _waitingLabelWidth1);
    }
    else {
        remainWidth = cellWidth -
            (result->shanten > 0 ? _servingLabelWidth2 : _waitingLabelWidth2);
    }

    int lineCnt = 1;
    int remainCnt = result->count_in_tiles;
    do {
        int limitedCnt = (int)remainWidth / TILE_WIDTH;  // 第N行可以排这么多
        if (limitedCnt >= remainCnt) {  // 排得下
            // 包括后面的字是否排得下
            bool inOneLine = remainCnt * TILE_WIDTH + _totalLabelWidth <= remainWidth;
            int height = 25 + 25 * (inOneLine ? lineCnt : lineCnt + 1);
            _cellHeightMap.insert(std::make_pair(key, height));
            return Size(0, height);
        }
        remainCnt -= limitedCnt;
        ++lineCnt;
        remainWidth = cellWidth;
    } while (remainCnt > 0);

    return Size(0, 50);
}

cw::TableViewCell *MahjongTheoryScene::tableCellAtIndex(cw::TableView *table, ssize_t idx) {
    typedef cw::TableViewCellEx<LayerColor *[2], Label *, Label *, ui::Button *, Label *, ui::Button *[34], Label *, Label *> CustomCell;
    CustomCell *cell = (CustomCell *)table->dequeueCell();
    if (cell == nullptr) {
        cell = CustomCell::create();

        Color4B bgColor0, bgColor1;
        Color3B textColor;
        if (UserDefault::getInstance()->getBoolForKey("night_mode")) {
            bgColor0 = Color4B(22, 22, 22, 255);
            bgColor1 = Color4B(32, 32, 32, 255);
            textColor = Color3B(208, 208, 208);
        }
        else {
            bgColor0 = Color4B::WHITE;
            bgColor1 = Color4B(239, 243, 247, 255);
            textColor = Color3B(80, 80, 80);
        }

        CustomCell::ExtDataType &ext = cell->getExtData();
        LayerColor *(&layerColor)[2] = std::get<0>(ext);
        Label *&typeLabel = std::get<1>(ext);
        Label *&discardLabel = std::get<2>(ext);
        ui::Button *&discardButton = std::get<3>(ext);
        Label *&usefulLabel = std::get<4>(ext);
        ui::Button *(&usefulButton)[34] = std::get<5>(ext);
        Label *&cntLabel1 = std::get<6>(ext);
        Label *&cntLabel2 = std::get<7>(ext);

        layerColor[0] = LayerColor::create(bgColor0, _cellWidth, 0);
        cell->addChild(layerColor[0]);
        layerColor[0]->setPosition(Vec2(0, 1));

        layerColor[1] = LayerColor::create(bgColor1, _cellWidth, 0);
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

        discardButton = ui::Button::create("tiles/bg.png");
        discardButton->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        discardButton->setScale(TILE_WIDTH / discardButton->getContentSize().width);
        cell->addChild(discardButton);
        discardButton->addClickEventListener(std::bind(&MahjongTheoryScene::onTileButton, this, std::placeholders::_1));

        usefulLabel = Label::createWithSystemFont("", "Arial", 12);
        usefulLabel->setColor(textColor);
        usefulLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        cell->addChild(usefulLabel);

        for (int i = 0; i < 34; ++i) {
            ui::Button *button = ui::Button::create(tilesImageName[mahjong::all_tiles[i]]);
            button->setScale(TILE_WIDTH / button->getContentSize().width);
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
        cntLabel2->setPosition(Vec2(SPACE, TILE_WIDTH));
    }

    const CustomCell::ExtDataType &ext = cell->getExtData();
    LayerColor *const (&layerColor)[2] = std::get<0>(ext);
    Label *typeLabel = std::get<1>(ext);
    Label *discardLabel = std::get<2>(ext);
    ui::Button *discardButton = std::get<3>(ext);
    Label *usefulLabel = std::get<4>(ext);
    ui::Button *const (&usefulButton)[34] = std::get<5>(ext);
    Label *cntLabel1 = std::get<6>(ext);
    Label *cntLabel2 = std::get<7>(ext);

    const Size cellSize = tableCellSizeAtIndex(table, idx);
    size_t realIdx = _orderedIndices[idx];
    const ResultEx *result = &_resultSources[realIdx];

    if (idx & 1) {
        layerColor[0]->setVisible(false);
        layerColor[1]->setVisible(true);
        layerColor[1]->setContentSize(Size(_cellWidth, cellSize.height - 2));
    }
    else {
        layerColor[0]->setVisible(true);
        layerColor[1]->setVisible(false);
        layerColor[0]->setContentSize(Size(_cellWidth, cellSize.height - 2));
    }

    typeLabel->setString(getResultTypeString(result->form_flag, result->shanten));
    typeLabel->setPosition(Vec2(SPACE, cellSize.height - 10));
    if (UserDefault::getInstance()->getBoolForKey("night_mode")) {
        typeLabel->setColor(result->shanten != -1 ? Color3B(208, 208, 208) : Color3B::YELLOW);
    }
    else {
        typeLabel->setColor(result->shanten != -1 ? Color3B(80, 80, 80) : Color3B::ORANGE);
    }

    float xPos = SPACE;
    float yPos = cellSize.height - 35;

    if (result->discard_tile != 0) {
        discardLabel->setVisible(true);
        discardButton->setVisible(true);

        discardLabel->setString("打「");
        discardLabel->setPosition(Vec2(xPos, yPos));
        xPos += discardLabel->getContentSize().width;

        discardButton->loadTextureNormal(tilesImageName[result->discard_tile]);
        discardButton->setUserData(reinterpret_cast<void *>(realIdx));
        discardButton->setPosition(Vec2(xPos, yPos));
        xPos += TILE_WIDTH;

        if (result->shanten > 0) {
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
        discardButton->setVisible(false);

        if (result->shanten != 0) {
            usefulLabel->setString("摸「");
        }
        else {
            usefulLabel->setString("听「");
        }
        usefulLabel->setPosition(Vec2(xPos, yPos));

        xPos += usefulLabel->getContentSize().width;
    }

    for (int i = 0; i < 34; ++i) {
        if (!result->useful_table[mahjong::all_tiles[i]]) {
            usefulButton[i]->setVisible(false);
            continue;
        }

        usefulButton[i]->setUserData(reinterpret_cast<void *>(realIdx));
        usefulButton[i]->setVisible(true);

        if (xPos + TILE_WIDTH > _cellWidth - SPACE * 2) {
            xPos = SPACE;
            yPos -= 25;
        }
        usefulButton[i]->setPosition(Vec2(xPos, yPos));
        xPos += TILE_WIDTH;
    }

    std::string str = StringUtils::format("」共%d种，%d枚", result->count_in_tiles, result->count_total);
    if (yPos > 15) {
        spiltStringToLabel(str, _cellWidth - SPACE * 2 - xPos, cntLabel1, cntLabel2);
    }
    else {
        cntLabel1->setString(str);
        cntLabel2->setVisible(false);
    }
    cntLabel1->setPosition(Vec2(xPos, yPos));

    return cell;
}
