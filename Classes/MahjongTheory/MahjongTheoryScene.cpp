#include "MahjongTheoryScene.h"
#include <array>
#include "../mahjong-algorithm/stringify.h"
#include "../mahjong-algorithm/fan_calculator.h"
#include "../TilesImage.h"
#include "../widget/TilePickWidget.h"
#include "../widget/AlertView.h"
#include "../widget/LoadingView.h"

USING_NS_CC;

static mahjong::tile_t serveRandomTile(const mahjong::tile_table_t &usedTable, mahjong::tile_t discardTile);

bool MahjongTheoryScene::init() {
    if (UNLIKELY(!BaseScene::initWithTitle("牌理"))) {
        return false;
    }

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    // 输入框
    ui::EditBox *editBox= ui::EditBox::create(Size(visibleSize.width - 95.0f, 20.0f), ui::Scale9Sprite::create("source_material/btn_square_normal.png"));
    this->addChild(editBox);
    editBox->setInputFlag(ui::EditBox::InputFlag::SENSITIVE);
    editBox->setInputMode(ui::EditBox::InputMode::SINGLE_LINE);
    editBox->setReturnType(ui::EditBox::KeyboardReturnType::DONE);
    editBox->setFontColor(Color4B::BLACK);
    editBox->setFontSize(12);
    editBox->setPlaceholderFontColor(Color4B::GRAY);
    editBox->setPlaceHolder("在此处输入");
    editBox->setMaxLength(50);
    editBox->setDelegate(this);
    editBox->setPosition(Vec2(origin.x + visibleSize.width * 0.5f - 40.0f, origin.y + visibleSize.height - 50.0f));
    _editBox = editBox;

    // 与输入框同位置的空白button
    const Size &editSize = editBox->getContentSize();
    ui::Widget *widget = ui::Widget::create();
    widget->setTouchEnabled(true);
    widget->setContentSize(editSize);
    widget->addClickEventListener([this](Ref *) { showInputAlert(); });
    editBox->addChild(widget);
    widget->setPosition(Vec2(editSize.width * 0.5f, editSize.height * 0.5f));

    // 随机按钮
    ui::Button *button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
    button->setScale9Enabled(true);
    button->setContentSize(Size(35.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText("随机");
    this->addChild(button);
    button->setPosition(Vec2(origin.x + visibleSize.width - 65.0f, origin.y + visibleSize.height - 50.0f));
    button->addClickEventListener([this](Ref *) { setRandomInput(); });

    // 说明按钮
    button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
    button->setScale9Enabled(true);
    button->setContentSize(Size(35.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText("说明");
    this->addChild(button);
    button->setPosition(Vec2(origin.x + visibleSize.width - 25.0f, origin.y + visibleSize.height - 50.0f));
    button->addClickEventListener(std::bind(&MahjongTheoryScene::onGuideButton, this, std::placeholders::_1));

    // 手牌
    HandTilesWidget *handTilesWidget = HandTilesWidget::create();
    handTilesWidget->setTileClickCallback(std::bind(&MahjongTheoryScene::onStandingTileEvent, this));
    this->addChild(handTilesWidget);
    Size widgetSize = handTilesWidget->getContentSize();

    // 根据情况缩放
    if (widgetSize.width - 4 > visibleSize.width) {
        float scale = (visibleSize.width - 4.0f) / widgetSize.width;
        handTilesWidget->setScale(scale);
        widgetSize.width = visibleSize.width - 4.0f;
        widgetSize.height *= scale;
    }
    handTilesWidget->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height - 65.0f - widgetSize.height * 0.5f));
    _handTilesWidget = handTilesWidget;

    // 撤销与重做按钮
    button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
    button->setScale9Enabled(true);
    button->setContentSize(Size(35.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText("撤销");
    this->addChild(button);
    button->setPosition(Vec2(origin.x + visibleSize.width - 65.0f, origin.y + visibleSize.height - 80.0f - widgetSize.height));
    button->addClickEventListener(std::bind(&MahjongTheoryScene::onUndoButton, this, std::placeholders::_1));
    button->setEnabled(false);
    _undoButton = button;

    button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
    button->setScale9Enabled(true);
    button->setContentSize(Size(35.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText("重做");
    this->addChild(button);
    button->setPosition(Vec2(origin.x + visibleSize.width - 25.0f, origin.y + visibleSize.height - 80.0f - widgetSize.height));
    button->addClickEventListener(std::bind(&MahjongTheoryScene::onRedoButton, this, std::placeholders::_1));
    button->setEnabled(false);
    _redoButton = button;

    // 特殊和型选项
    Label *label = Label::createWithSystemFont("考虑特殊和型", "Arial", 12);
    label->setColor(Color3B::BLACK);
    this->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(origin.x + 10, origin.y + visibleSize.height - 80.0f - widgetSize.height));

    static const char *title[] = { "七对", "十三幺", "全不靠", "组合龙" };
    const float yPos = origin.y + visibleSize.height - 105.0f - widgetSize.height;
    const float gap = (visibleSize.width - 4.0f) * 0.25f;
    for (int i = 0; i < 4; ++i) {
        const float xPos = origin.x + gap * (i + 0.5f);
        ui::CheckBox *checkBox = ui::CheckBox::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
        this->addChild(checkBox);
        checkBox->setZoomScale(0.0f);
        checkBox->ignoreContentAdaptWithSize(false);
        checkBox->setContentSize(Size(20.0f, 20.0f));
        checkBox->setPosition(Vec2(xPos - 20.0f, yPos));
        checkBox->setSelected(true);
        checkBox->addEventListener([this](Ref *, ui::CheckBox::EventType) {
            filterResultsByFlag(getFilterFlag());
            _tableView->reloadData();
        });
        _checkBoxes[i] = checkBox;

        label = Label::createWithSystemFont(title[i], "Arial", 12);
        label->setColor(Color3B::BLACK);
        this->addChild(label);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        label->setPosition(Vec2(xPos - 5.0f, yPos));
    }

    // 预先算好Cell及各label的Size
    _cellWidth = visibleSize.width - 5;
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

    cw::TableView *tableView = cw::TableView::create();
    tableView->setDirection(ui::ScrollView::Direction::VERTICAL);
    tableView->setScrollBarPositionFromCorner(Vec2(2.0f, 2.0f));
    tableView->setScrollBarWidth(4.0f);
    tableView->setScrollBarOpacity(0x99);
    tableView->setBounceEnabled(true);
    tableView->setContentSize(Size(visibleSize.width - 5.0f, visibleSize.height - 130.0f - widgetSize.height));
    tableView->setDelegate(this);
    tableView->setVerticalFillOrder(cw::TableView::VerticalFillOrder::TOP_DOWN);

    tableView->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    tableView->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + (visibleSize.height - widgetSize.height) * 0.5f - 60.0f));
    this->addChild(tableView);
    _tableView = tableView;

    srand(static_cast<unsigned>(time(nullptr)));

    // 下一帧设置随机输入
    this->scheduleOnce([this](float) { setRandomInput(); }, 0.0f, "set_random_input");

    return true;
}

void MahjongTheoryScene::onGuideButton(cocos2d::Ref *) {
    AlertView::showWithMessage("使用说明",
        "牌理功能未经严格测试，可能存在bug。\n\n"
        "1.数牌：万=m 条=s 饼=p。后缀使用小写字母，一连串同花色的数牌可合并使用用一个后缀，如123m、678s等等。\n"
        "2.字牌：东南西北=ESWN，中发白=CFP。使用大写字母。亦兼容天凤风格的后缀z，但按中国习惯顺序567z为中发白。\n"
        "3.吃、碰、杠用英文[]，可选用逗号+数字表示供牌来源。数字的具体规则如下：\n"
        "  (1) 吃：表示第几张牌是由上家打出，如[567m,2]表示57万吃6万（第2张）。对于不指定数字的，默认为吃第1张。\n"
        "  (2) 碰：表示由哪家打出，1为上家，2为对家，3为下家，如[999s,3]表示碰下家的9条。对于不指定数字的，默认为碰上家。\n"
        "  (3) 杠：与碰类似，但对于不指定数字的，则认为是暗杠。例如：[SSSS]表示暗杠南；[8888p,1]表示明杠上家的8饼。\n"
        "4.输入牌的总数不能超过14张。\n"
        "5.当输入牌的数量为(n*3+2)时，最后一张牌作为摸上来的牌。\n"
        "6.当输入牌的数量为(n*3+1)时，系统会随机补一张摸上来的牌。\n"
        "7.基本和型暂不考虑国标番型。\n"
        "8.暂不考虑吃碰杠操作。\n"
        "9.点击表格中的有效牌，可切出该切法的弃牌，并上指定牌。\n"
        "10.点击手牌可切出对应牌，随机上牌。\n"
        "输入范例1：[EEEE]288s349pSCFF2p\n"
        "输入范例2：[123p,1][345s,2][999s,3]6m6pEW1m\n"
        "输入范例3：356m18s1579pWNFF9p",
        10, nullptr, nullptr);
}

void MahjongTheoryScene::showInputAlert() {
    mahjong::hand_tiles_t handTiles;
    mahjong::tile_t servingTile;
    mahjong::string_to_tiles(_editBox->getText(), &handTiles, &servingTile);
    if (0 != mahjong::check_calculator_input(&handTiles, servingTile)) {
        _editBox->touchDownAction(_editBox, ui::Widget::TouchEventType::ENDED);
        return;
    }

    Size visibleSize = Director::getInstance()->getVisibleSize();
    const float maxWidth = visibleSize.width - 20;

    // 选牌面板和其他信息的相关控件
    TilePickWidget *tilePicker = TilePickWidget::create();

    // 缩放
    Size pickerSize = tilePicker->getContentSize();
    const float pickerScale = maxWidth / pickerSize.width;
    tilePicker->setScale(pickerScale);
    pickerSize = Size(maxWidth, pickerSize.height * pickerScale);

    // 布局在rootNode上
    Node *rootNode = Node::create();
    rootNode->setContentSize(Size(maxWidth, pickerSize.height));
    rootNode->addChild(tilePicker);
    tilePicker->setPosition(Vec2(maxWidth * 0.5f, pickerSize.height * 0.5f));

    if (handTiles.tile_count != 0 && servingTile != 0) {
        tilePicker->setData(handTiles, servingTile);
    }

    // 通过AlertView显示出来
    AlertView::showWithNode("输入手牌", rootNode, maxWidth, [this, tilePicker]() {
        mahjong::hand_tiles_t handTiles;
        mahjong::tile_t servingTile;
        tilePicker->getData(&handTiles, &servingTile);
        char str[64];
        intptr_t ret = mahjong::hand_tiles_to_string(&handTiles, str, 64);
        if (ret > 0) {
            mahjong::tiles_to_string(&servingTile, 1, &str[ret], 64 - ret);
            _editBox->setText(str);
            editBoxReturn(_editBox);
        }
    }, std::bind(&ui::EditBox::touchDownAction, _editBox, _editBox, ui::Widget::TouchEventType::ENDED));
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

    // 转化为字符串，显示在输入框内
    char str[64];
    intptr_t ret = mahjong::hand_tiles_to_string(&handTiles, str, sizeof(str));
    mahjong::tiles_to_string(&servingTile, 1, str + ret, sizeof(str) - ret);
    _editBox->setText(str);

    _allResults.clear();
    _resultSources.clear();
    _orderedIndices.clear();

    _tableView->reloadData();

    _undoCache.clear();
    _redoCache.clear();
    _undoButton->setEnabled(false);
    _redoButton->setEnabled(false);

    calculate();
}

void MahjongTheoryScene::editBoxReturn(cocos2d::ui::EditBox *editBox) {
    if (parseInput(editBox->getText())) {
        _allResults.clear();
        _resultSources.clear();
        _orderedIndices.clear();

        _tableView->reloadData();

        _undoCache.clear();
        _redoCache.clear();
        _undoButton->setEnabled(false);
        _redoButton->setEnabled(false);

        calculate();
    }
}

bool MahjongTheoryScene::parseInput(const char *input) {
    if (*input == '\0') {
        return false;
    }

    const char *errorStr = nullptr;
    std::string str = input;

    do {
        mahjong::hand_tiles_t hand_tiles = { 0 };
        mahjong::tile_t serving_tile = 0;
        intptr_t ret = mahjong::string_to_tiles(input, &hand_tiles, &serving_tile);
        if (ret != PARSE_NO_ERROR) {
            switch (ret) {
            case PARSE_ERROR_ILLEGAL_CHARACTER: errorStr = "无法解析的字符"; break;
            case PARSE_ERROR_NO_SUFFIX_AFTER_DIGIT: errorStr = "数字后面需有后缀"; break;
            case PARSE_ERROR_WRONG_TILES_COUNT_FOR_FIXED_PACK: errorStr = "一组副露包含了错误的牌数目"; break;
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

        // 随机上牌
        if (serving_tile == 0) {
            mahjong::tile_table_t cnt_table;
            mahjong::map_tiles(hand_tiles.standing_tiles, hand_tiles.tile_count, cnt_table);
            serving_tile = serveRandomTile(cnt_table, 0);

            char temp[64];
            mahjong::tiles_to_string(&serving_tile, 1, temp, sizeof(temp));
            str.append(temp);
            _editBox->setText(str.c_str());
        }

        // 检查输入的合法性
        ret = mahjong::check_calculator_input(&hand_tiles, serving_tile);
        if (ret != 0) {
            switch (ret) {
            case ERROR_WRONG_TILES_COUNT: errorStr = "牌张数错误"; break;
            case ERROR_TILE_COUNT_GREATER_THAN_4: errorStr = "同一种牌最多只能使用4枚"; break;
            default: break;
            }
            break;
        }

        // 设置UI
        _handTilesWidget->setData(hand_tiles, serving_tile);
        return true;
    } while (0);

    if (errorStr != nullptr) {
        AlertView::showWithMessage("提示", errorStr, 12, std::bind(&MahjongTheoryScene::showInputAlert, this), nullptr);
    }
    return false;
}

void MahjongTheoryScene::filterResultsByFlag(uint8_t flag) {
    flag |= FORM_FLAG_BASIC_FORM;  // 基本和型不能被过滤掉

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
            _resultSources.emplace_back();
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
    uint8_t flag = FORM_FLAG_BASIC_FORM;
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

    auto thiz = makeRef(this);  // 保证线程回来之前不析构
    std::thread([thiz, hand_tiles, serving_tile, loadingView]() {
        mahjong::enum_discard_tile(&hand_tiles, serving_tile, FORM_FLAG_ALL, thiz.get(),
            [](void *context, const mahjong::enum_result_t *result) {
            MahjongTheoryScene *thiz = (MahjongTheoryScene *)context;
            if (result->shanten != std::numeric_limits<int>::max()) {
                thiz->_allResults.push_back(*result);
            }
            return (thiz->isRunning());
        });

        // 调回cocos线程
        Director::getInstance()->getScheduler()->performFunctionInCocosThread([thiz, loadingView]() {
            if (thiz->isRunning()) {
                thiz->filterResultsByFlag(thiz->getFilterFlag());
                thiz->_tableView->reloadData();
                loadingView->removeFromParent();
            }
        });
    }).detach();
}

mahjong::tile_t serveRandomTile(const mahjong::tile_table_t &usedTable, mahjong::tile_t discardTile) {
    // 没用到的牌表
    mahjong::tile_table_t remainTable;
    std::transform(std::begin(usedTable), std::end(usedTable), std::begin(remainTable),
        [](int n) { return 4 - n; });
    //--remainTable[discardTile];  // 有必要吗？

    // 没用到的牌
    mahjong::tile_t remainTiles[136];
    intptr_t remainCnt = mahjong::table_to_tiles(remainTable, remainTiles, 136);

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

    // 打一张牌，上随机牌或指定牌
    int tag = button->getTag();
    if (tag != INVALID_TAG) {
        servingTile = mahjong::all_tiles[tag];
    }
    else {
        servingTile = serveRandomTile(_handTilesTable, discardTile);
    }

    _undoCache.emplace_back();
    _redoCache.clear();
    _undoButton->setEnabled(true);
    _redoButton->setEnabled(false);
    StateData &state = _undoCache.back();
    _handTilesWidget->getData(&state.handTiles, &state.servingTile);
    state.allResults = _allResults;

    deduce(discardTile, servingTile);
}

void MahjongTheoryScene::onStandingTileEvent() {
    mahjong::tile_t discardTile = _handTilesWidget->getCurrentTile();
    if (discardTile == 0 || _handTilesWidget->getServingTile() == 0) {
        return;
    }

    // 打一张牌，上随机牌
    mahjong::tile_t servingTile = serveRandomTile(_handTilesTable, discardTile);

    _undoCache.emplace_back();
    _redoCache.clear();
    _undoButton->setEnabled(true);
    _redoButton->setEnabled(false);
    StateData &state = _undoCache.back();
    _handTilesWidget->getData(&state.handTiles, &state.servingTile);
    state.allResults.swap(_allResults);

    deduce(discardTile, servingTile);
}

void MahjongTheoryScene::recoverFromState(StateData &state) {
    // 设置UI
    _handTilesWidget->setData(state.handTiles, state.servingTile);

    char str[64];
    intptr_t ret = mahjong::hand_tiles_to_string(&state.handTiles, str, sizeof(str));
    mahjong::tiles_to_string(&state.servingTile, 1, str + ret, sizeof(str) - ret);
    _editBox->setText(str);

    // 打表
    mahjong::map_hand_tiles(&state.handTiles, _handTilesTable);
    if (state.servingTile != 0) {
        ++_handTilesTable[state.servingTile];
    }

    // 设置数据
    _allResults.swap(state.allResults);
    filterResultsByFlag(getFilterFlag());
    _tableView->reloadData();
}

void MahjongTheoryScene::onUndoButton(cocos2d::Ref *) {
    if (LIKELY(!_undoCache.empty())) {
        _redoCache.emplace_back();
        StateData &state = _redoCache.back();
        _handTilesWidget->getData(&state.handTiles, &state.servingTile);
        state.allResults.swap(_allResults);

        recoverFromState(_undoCache.back());

        _undoCache.pop_back();
        _undoButton->setEnabled(!_undoCache.empty());
        _redoButton->setEnabled(true);
    }
}

void MahjongTheoryScene::onRedoButton(cocos2d::Ref *) {
    if (LIKELY(!_redoCache.empty())) {
        _undoCache.emplace_back();
        StateData &state = _undoCache.back();
        _handTilesWidget->getData(&state.handTiles, &state.servingTile);
        state.allResults.swap(_allResults);

        recoverFromState(_redoCache.back());

        _redoCache.pop_back();
        _undoButton->setEnabled(true);
        _redoButton->setEnabled(!_redoCache.empty());
    }
}

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
    mahjong::tile_table_t cntTable;
    mahjong::map_tiles(handTiles.standing_tiles, handTiles.tile_count, cntTable);
    ++cntTable[st];  // 当前上牌

    // 打出牌
    if (discardTile != 0 && cntTable[discardTile] > 0) {
        --cntTable[discardTile];
    }

    // 从表恢复成立牌
    const intptr_t prevCnt = handTiles.tile_count;
    handTiles.tile_count = mahjong::table_to_tiles(cntTable, handTiles.standing_tiles, 13);
    if (prevCnt != handTiles.tile_count) {
        return;
    }

    // 设置UI
    _handTilesWidget->setData(handTiles, servingTile);

    char str[64];
    intptr_t ret = mahjong::hand_tiles_to_string(&handTiles, str, sizeof(str));
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
    default: str = Common::format("%d上听(", step); break;
    }

    bool needCaesuraSign = false;
#define APPEND_CAESURA_SIGN_IF_NECESSARY() \
    if (LIKELY(needCaesuraSign)) { str.append("、"); } needCaesuraSign = true

    if (flag & FORM_FLAG_BASIC_FORM) {
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

// 分割string到两个label
static void spiltStringToLabel(const std::string &str, float width, Label *label1, Label *label2) {
    label2->setVisible(false);
    label1->setString(str);
    const Size &size = label1->getContentSize();
    if (size.width <= width) {  // 宽度允许
        return;
    }

    StringUtils::StringUTF8 utf8(str);
    long utf8Len = utf8.length();
    long pos = static_cast<long>(width / size.width * utf8Len);  // 切这么多

    // 切没了，全部放在第2个label上
    if (pos <= 0) {
        label1->setString("");
        label2->setVisible(true);
        label2->setString(str);
        return;
    }

    std::string str1 = utf8.getAsCharSequence(0, pos);
    std::string str2 = utf8.getAsCharSequence(pos, utf8Len - pos);

    // 保证不从数字中间切断
    if (!str2.empty() && Common::__isdigit(str2.front())) {  // 第2个字符串以数字开头
        // 将第1个字符串尾部的数字都转移到第2个字符串头部
        while (pos > 0 && !str1.empty() && Common::__isdigit(str1.back())) {
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
#define TILE_WIDTH_SMALL 15

ssize_t MahjongTheoryScene::numberOfCellsInTableView(cw::TableView *) {
    return _orderedIndices.size();
}

float MahjongTheoryScene::tableCellSizeForIndex(cw::TableView *, ssize_t idx) {
    size_t realIdx = _orderedIndices[idx];
    const ResultEx *result = &_resultSources[realIdx];  // 当前cell的数据

    // 用有效牌种数作为key，缓存曾经计算过的高度
    const uint16_t key = static_cast<uint16_t>(result->count_in_tiles);
    std::unordered_map<uint16_t, int>::iterator it = _cellHeightMap.find(key);
    if (it != _cellHeightMap.end()) {
        return static_cast<float>(it->second);
    }

    const float cellWidth = _cellWidth - SPACE * 2;  // 前后各留2像素
    float remainWidth;  // 第一行除了前面一些label之后剩下宽度
    remainWidth = cellWidth - _discardLabelWidth - TILE_WIDTH_SMALL -
        (result->shanten > 0 ? _servingLabelWidth1 : _waitingLabelWidth1);

    int lineCnt = 1;
    int remainCnt = result->count_in_tiles;
    do {
        int limitedCnt = static_cast<int>(remainWidth / TILE_WIDTH_SMALL);  // 第N行可以排这么多
        if (limitedCnt >= remainCnt) {  // 排得下
            // 包括后面的字是否排得下
            bool inOneLine = remainCnt * TILE_WIDTH_SMALL + _totalLabelWidth <= remainWidth;
            int height = 25 + 25 * (inOneLine ? lineCnt : lineCnt + 1);
            _cellHeightMap.insert(std::make_pair(key, height));
            return static_cast<float>(height);
        }
        remainCnt -= limitedCnt;
        ++lineCnt;
        remainWidth = cellWidth;
    } while (remainCnt > 0);

    return 50.0f;
}

cw::TableViewCell *MahjongTheoryScene::tableCellAtIndex(cw::TableView *table, ssize_t idx) {
    typedef cw::TableViewCellEx<std::array<LayerColor *, 2>, Label *, Label *, ui::Button *, std::array<Label *, 2>, std::array<ui::Button *, 34>, std::array<Label *, 2> > CustomCell;
    CustomCell *cell = (CustomCell *)table->dequeueCell();
    if (cell == nullptr) {
        cell = CustomCell::create();

        CustomCell::ExtDataType &ext = cell->getExtData();
        std::array<LayerColor *, 2> &layerColors = std::get<0>(ext);
        Label *&typeLabel = std::get<1>(ext);
        Label *&discardLabel = std::get<2>(ext);
        ui::Button *&discardButton = std::get<3>(ext);
        std::array<Label *, 2> &usefulLabel = std::get<4>(ext);
        std::array<ui::Button *, 34> &usefulButtons = std::get<5>(ext);
        std::array<Label *, 2> &cntLabel = std::get<6>(ext);

        // 背景色
        layerColors[0] = LayerColor::create(Color4B(0x10, 0x10, 0x10, 0x10), _cellWidth, 0.0f);
        cell->addChild(layerColors[0]);

        layerColors[1] = LayerColor::create(Color4B(0xC0, 0xC0, 0xC0, 0x10), _cellWidth, 0.0f);
        cell->addChild(layerColors[1]);

        // 和牌型label
        Label *label = Label::createWithSystemFont("", "Arial", 12);
        label->setColor(Color3B(0x60, 0x60, 0x60));
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        cell->addChild(label);
        typeLabel = label;

        // 打label
        label = Label::createWithSystemFont("打「", "Arial", 12);
        label->setColor(Color3B(0x60, 0x60, 0x60));
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        cell->addChild(label);
        discardLabel = label;

        // 打的那张牌button
        const float tileScale = CC_CONTENT_SCALE_FACTOR() / TILE_WIDTH * TILE_WIDTH_SMALL;
        ui::Button *button = ui::Button::create("tiles/bg.png");
        button->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        button->setScale(tileScale);
        cell->addChild(button);
        button->addClickEventListener(std::bind(&MahjongTheoryScene::onTileButton, this, std::placeholders::_1));
        discardButton = button;

        // 摸label
        label = Label::createWithSystemFont("」摸「", "Arial", 12);
        label->setColor(Color3B(0x60, 0x60, 0x60));
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        cell->addChild(label);
        usefulLabel[0] = label;

        // 听label
        label = Label::createWithSystemFont("」听「", "Arial", 12);
        label->setColor(Color3B(0x60, 0x60, 0x60));
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        cell->addChild(label);
        usefulLabel[1] = label;

        // 34张牌button
        for (int i = 0; i < 34; ++i) {
            button = ui::Button::create(tilesImageName[mahjong::all_tiles[i]]);
            button->setScale(tileScale);
            button->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
            cell->addChild(button);
            button->setTag(i);
            button->addClickEventListener(std::bind(&MahjongTheoryScene::onTileButton, this, std::placeholders::_1));
            usefulButtons[i] = button;
        }

        // 共几种几枚分在两个label上
        label = Label::createWithSystemFont("", "Arial", 12);
        label->setColor(Color3B(0x60, 0x60, 0x60));
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        cell->addChild(label);
        cntLabel[0] = label;

        label = Label::createWithSystemFont("", "Arial", 12);
        label->setColor(Color3B(0x60, 0x60, 0x60));
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        cell->addChild(label);
        label->setPosition(Vec2(SPACE, TILE_WIDTH_SMALL));
        cntLabel[1] = label;
    }

    const CustomCell::ExtDataType &ext = cell->getExtData();
    const std::array<LayerColor *, 2> &layerColors = std::get<0>(ext);
    Label *typeLabel = std::get<1>(ext);
    Label *discardLabel = std::get<2>(ext);
    ui::Button *discardButton = std::get<3>(ext);
    const std::array<Label *, 2> &usefulLabel = std::get<4>(ext);
    const std::array<ui::Button *, 34> &usefulButtons = std::get<5>(ext);
    const std::array<Label *, 2> &cntLabel = std::get<6>(ext);

    const float cellHeight = tableCellSizeForIndex(table, idx);
    size_t realIdx = _orderedIndices[idx];
    const ResultEx *result = &_resultSources[realIdx];

    // 背景色
    if (idx & 1) {
        layerColors[0]->setVisible(false);
        layerColors[1]->setVisible(true);
        layerColors[1]->setContentSize(Size(_cellWidth, cellHeight));
    }
    else {
        layerColors[0]->setVisible(true);
        layerColors[1]->setVisible(false);
        layerColors[0]->setContentSize(Size(_cellWidth, cellHeight));
    }

    // 和牌型label
    typeLabel->setString(getResultTypeString(result->form_flag, result->shanten));
    typeLabel->setPosition(Vec2(SPACE, cellHeight - 10.0f));
    typeLabel->setColor(result->shanten != -1 ? Color3B(0x60, 0x60, 0x60) : Color3B::ORANGE);

    float xPos = SPACE;
    float yPos = cellHeight - 35.0f;

    // 打label
    discardLabel->setPosition(Vec2(xPos, yPos));
    xPos += discardLabel->getContentSize().width;

    // 打的那张牌button
    discardButton->loadTextureNormal(tilesImageName[result->discard_tile]);
    discardButton->setUserData(reinterpret_cast<void *>(realIdx));
    discardButton->setPosition(Vec2(xPos, yPos));
    xPos += TILE_WIDTH_SMALL;

    // 摸或者听label
    if (result->shanten > 0) {
        usefulLabel[0]->setVisible(true);
        usefulLabel[1]->setVisible(false);
        usefulLabel[0]->setPosition(Vec2(xPos, yPos));
        xPos += usefulLabel[0]->getContentSize().width;
    }
    else {
        usefulLabel[0]->setVisible(false);
        usefulLabel[1]->setVisible(true);
        usefulLabel[1]->setPosition(Vec2(xPos, yPos));
        xPos += usefulLabel[1]->getContentSize().width;
    }

    // 34张牌button
    for (int i = 0; i < 34; ++i) {
        if (!result->useful_table[mahjong::all_tiles[i]]) {
            usefulButtons[i]->setVisible(false);
            continue;
        }

        usefulButtons[i]->setUserData(reinterpret_cast<void *>(realIdx));
        usefulButtons[i]->setVisible(true);

        if (xPos + TILE_WIDTH_SMALL > _cellWidth - SPACE * 2) {
            xPos = SPACE;
            yPos -= 25;
        }
        usefulButtons[i]->setPosition(Vec2(xPos, yPos));
        xPos += TILE_WIDTH_SMALL;
    }

    // 共几种几枚分在两个label上
    std::string str = Common::format("」共%d种，%d枚", result->count_in_tiles, result->count_total);
    if (yPos > 15) {
        spiltStringToLabel(str, _cellWidth - SPACE * 2 - xPos, cntLabel[0], cntLabel[1]);
    }
    else {
        cntLabel[0]->setString(str);
        cntLabel[1]->setVisible(false);
    }
    cntLabel[0]->setPosition(Vec2(xPos, yPos));

    return cell;
}
