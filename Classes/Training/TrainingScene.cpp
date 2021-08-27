#include "TrainingScene.h"
#include <fstream>
#include "network/HttpClient.h"
#include "../UICommon.h"
#include "../UIColors.h"
#include "../TilesImage.h"
#include "../widget/LoadingView.h"
#include "../widget/HandTilesWidget.h"
#include "../widget/PopupMenu.h"
#include "../widget/AlertDialog.h"
#include "../widget/Toast.h"

USING_NS_CC;

struct PuzzleTemplate {
    std::string question[3];
    std::string answer[3];
    std::string meldable[3];
};

static std::vector<PuzzleTemplate> s_puzzles;

static void parseField(const char *str, ptrdiff_t size, std::string (&arr)[3]) {
    while (size > 0 && Common::__isspace(*str)) {
        ++str;
        --size;
    }
    while (size > 0 && Common::__isspace(str[size - 1])) {
        --size;
    }

    size_t cnt = 0;
    for (auto n = std::find(str, str + size, ',') - str; n != size; n = std::find(str, str + size, ',') - str) {
        if (n != 1 || str[0] != '0') {
            arr[cnt] = std::string(str, n);
        }
        if (++cnt == 3) {
            return;
        }
        ++n;
        str += n;
        size -= n;
    }

    if (cnt < 3 && size > 0 && (size != 1 || str[0] != '0')) {
        arr[cnt] = std::string(str, size);
    }
}

static bool parseLine(const char *str, ptrdiff_t size, PuzzleTemplate &tmp) {
    while (size > 0 && Common::__isspace(*str)) {
        ++str;
        --size;
    }
    while (size > 0 && Common::__isspace(str[size - 1])) {
        --size;
    }

    auto n = std::find(str, str + size, '|') - str;
    if (n < size) {
        parseField(str, n, tmp.question);
        ++n;
        str += n;
        size -= n;

        n = std::find(str, str + size, '|') - str;
        if (n < size) {
            parseField(str, n, tmp.answer);
            ++n;
            str += n;
            size -= n;

            n = std::find(str, str + size, '|') - str;
            parseField(str, std::min(n, size), tmp.meldable);

            return true;
        }
    }

    return false;
}

static bool loadTemplates(const char *str, ptrdiff_t size, std::vector<PuzzleTemplate> &puzzles) {
    puzzles.reserve(800);

    PuzzleTemplate tmp;

    while (size > 0) {
        auto n = std::find(str, str + size, '\n') - str;
        if (parseLine(str, n, tmp)) {
            puzzles.push_back(std::move(tmp));
        }

        if (n == size) {
            break;
        }

        ++n;
        str += n;
        size -= n;
    }

    return true;
}

static const uint8_t OrderTable[][3] = {
    { 1, 2, 3 }, { 1, 3, 2 }, { 2, 1, 3 }, { 2, 3, 1 }, { 3, 1, 2 }, { 3, 2, 1 }
};

static uint64_t generatePuzzle(mahjong::hand_tiles_t *handTiles, mahjong::tile_t *servingTile) {
    std::mt19937_64 engine(std::chrono::system_clock::now().time_since_epoch().count());

    // 选择一个题目模板
    const PuzzleTemplate &tpl = s_puzzles.at(std::uniform_int_distribution<size_t>(0, s_puzzles.size() - 1)(engine));

    memset(handTiles, 0, sizeof(*handTiles));
    *servingTile = 0;
    uint64_t answer = 0;

    auto &order = OrderTable[std::uniform_int_distribution<>(0, 5)(engine)];  // 分配花色

    // 牌表
    mahjong::tile_table_t table = { 0 };
    uint8_t cnt = 0, i;
    for (i = 0; i < 3; ++i) {
        uint8_t s = order[i % 3];
        for (char c : tpl.question[i]) {
            ++table[mahjong::make_tile(s, c - '0')];
            ++cnt;
        }
        for (char c : tpl.answer[i]) {
            answer |= (1ULL << mahjong::make_tile(s, c - '0'));
        }
    }

    // 添加雀头
    auto fill_pair = [&engine, &table, handTiles](bool terminal){
        // 每门花色的19与其他牌的最小距离
        uint8_t dist[6] = { 0 };
        for (uint8_t s = 0; s < 3; ++s) {
            auto &d1 = dist[s << 1];
            for (uint8_t i = 0; i < 9; ++i, ++d1) {
                if (table[mahjong::make_tile(s + 1, i + 1)]) break;
            }
            auto &d9 = dist[(s << 1) + 1];
            for (uint8_t i = 0; i < 9; ++i, ++d9) {
                if (table[mahjong::make_tile(s + 1, 9 - i)]) break;
            }
        }

        // 选择最大距离的牌作为雀头
        uint8_t k = 0, max_dist = dist[0];
        for (uint8_t i = 1; i < 6; ++i) {
            // 如果相同，则一半概率选择后面的
            if (dist[i] > max_dist || (dist[i] == max_dist && static_cast<uint8_t>(std::uniform_int_distribution<>(0, 99)(engine)) < 50)) {
                k = i;
                max_dist = dist[i];
            }
        }

        uint8_t suit = (k >> 1) + 1, rank;
        if (terminal) {
            rank = (k & 1) * 8 + 1;
        }
        else {
            int a, b;
            if (k & 1) {
                a = 12 - max_dist;
                b = max_dist < 6 ? 9 : 8;  // 避免全带幺
            }
            else {
                a = max_dist < 6 ? 1 : 2;  // 避免全带幺
                b = max_dist - 2;
            }
            if (a < b) {
                rank = static_cast<uint8_t>(std::uniform_int_distribution<>(a, b)(engine));
            }
            else {
                rank = (k & 1) * 8 + 1;
            }
        }
        table[mahjong::make_tile(suit, rank)] += 2;
    };

    if (cnt < 12) {
        // 用明箭刻补齐无关第四组
        mahjong::tile_t t = static_cast<mahjong::tile_t>(std::uniform_int_distribution<>(mahjong::TILE_C, mahjong::TILE_P)(engine));
        handTiles->fixed_packs[0] = mahjong::make_pack(static_cast<uint8_t>(std::uniform_int_distribution<>(0, 2)(engine)), PACK_TYPE_PUNG, t);
        ++handTiles->pack_count;

        if (cnt == 9) {
            fill_pair(false);
        }
    }
    else if (cnt == 12) {
        fill_pair(true);
    }

    // 添加副露
    auto make_fixed_pack = [&engine, &table](const std::string &fixed, uint8_t suit)->uint16_t {
        char c;
        if (fixed.size() < 2) {
            c = fixed[0];
        }
        else {
            // 50% 概率
            if (std::uniform_int_distribution<>(0, 1)(engine)) {
                c = fixed[0];
            } else {
                c = fixed[1];
            }
        }
        mahjong::tile_t t = mahjong::make_tile(suit, c - '0');
        --table[t - 1];
        --table[t];
        --table[t + 1];

        return mahjong::make_pack(static_cast<uint8_t>(std::uniform_int_distribution<>(0, 2)(engine)), PACK_TYPE_CHOW, t);
    };

    // 副露几组
    // 断幺或独幺的14张牌强制副露一组，避免门断平听牌型
    i = 0;
    if (cnt == 14
        && table[mahjong::TILE_1m] + table[mahjong::TILE_9m] + table[mahjong::TILE_1s] + table[mahjong::TILE_9s] + table[mahjong::TILE_1p] + table[mahjong::TILE_9p] < 2) {
        for (; i < 3; ++i) {
            if (tpl.meldable[i].size()) {
                handTiles->fixed_packs[handTiles->pack_count++] = make_fixed_pack(tpl.meldable[i], order[i % 3]);
            }
        }
    }

    for (; i < 3; ++i) {
        // 40%概率
        if (tpl.meldable[i].size() && std::uniform_int_distribution<>(0, 9)(engine) < 4) {
            handTiles->fixed_packs[handTiles->pack_count++] = make_fixed_pack(tpl.meldable[i], order[i % 3]);
        }
    }

    // 收集立牌
    mahjong::tile_t tmp[14];
    cnt = 0;
    for (i = 0; i < mahjong::TILE_TABLE_SIZE; ++i) {
        for (auto k = table[i]; k > 0; --k) {
            tmp[cnt++] = i;
        }
    }

    // 随机选一张作为摸上来的牌
    uint8_t k = static_cast<uint8_t>(std::uniform_int_distribution<>(0, cnt - 1)(engine));
    for (i = 0; i < k; ++i) {
        handTiles->standing_tiles[i] = tmp[i];
    }
    *servingTile = tmp[k];
    for (i = k + 1; i < cnt; ++i) {
        handTiles->standing_tiles[i - 1] = tmp[i];
    }
    handTiles->tile_count = cnt - 1;

    return answer;
}

class ShaderLayer : public Layer {
public:
    CREATE_FUNC(ShaderLayer);

    virtual bool init() override {
        if (UNLIKELY(!Layer::init())) {
            return false;
        }

        Size visibleSize = Director::getInstance()->getVisibleSize();
        const float height = visibleSize.height - 30;

        this->setContentSize(Size(visibleSize.width, height));
        this->setAnchorPoint(Vec2::ANCHOR_BOTTOM_LEFT);

        // 触摸监听
        auto touchListener = EventListenerTouchOneByOne::create();
        touchListener->setSwallowTouches(true);
        touchListener->onTouchBegan = [this, height](Touch *touch, Event *event) {
            Vec2 pos = this->convertTouchToNodeSpace(touch);
            if (pos.y >= height) {
                return false;
            }
            event->stopPropagation();
            return true;
        };
        _eventDispatcher->addEventListenerWithSceneGraphPriority(touchListener, this);
        _touchListener = touchListener;

        return true;
    }
};

static inline float percentage_of(uint32_t num, uint32_t deno) {
    if (deno == 0) return 0.0f;
    else return num * 100.0f / deno;
}

bool TrainingScene::init() {
    if (UNLIKELY(!BaseScene::initWithTitle(__UTF8("训练")))) {
        return false;
    }

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    _viewRect = Rect(origin, visibleSize);

    ui::Button *button = ui::Button::create("icon/menu.png");
    this->addChild(button);
    button->setScale(20.0f / button->getContentSize().width);
    button->setPosition(Vec2(origin.x + visibleSize.width - 15.0f, origin.y + visibleSize.height - 15.0f));
    button->addClickEventListener(std::bind(&TrainingScene::onMoreButton, this, std::placeholders::_1));

    // 手牌
    HandTilesWidget *handTilesWidget = HandTilesWidget::create();
    handTilesWidget->setTileClickCallback(std::bind(&TrainingScene::onStandingTileEvent, this));
    this->addChild(handTilesWidget);
    Size widgetSize = handTilesWidget->getContentSize();

    // 根据情况缩放
    if (widgetSize.width - 4 > visibleSize.width) {
        float scale = (visibleSize.width - 4.0f) / widgetSize.width;
        handTilesWidget->setScale(scale);
        widgetSize.width = visibleSize.width - 4.0f;
        widgetSize.height *= scale;
    }

    // 牌和控件的高度(widgetSize.height + 50.0f)
    // 剩余高度(visibleSize.height - widgetSize.height - 50.0f)
    // 4/6分配
    const float y0 = (visibleSize.height - widgetSize.height - 50.0f) * 0.6f;

    handTilesWidget->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + y0 + 50.0f + widgetSize.height * 0.5f));
    _handTilesWidget = handTilesWidget;

    ui::Widget *widget = ui::Widget::create();
    widget->setContentSize(widgetSize);
    widget->setTouchEnabled(true);
    this->addChild(widget);
    widget->setPosition(handTilesWidget->getPosition());
    //widget->addChild(LayerColor::create(Color4B::RED, widgetSize.width, widgetSize.height));
    widget->setVisible(false);
    _shaderWidget = widget;

    Label *label = Label::createWithSystemFont(__UTF8("0/0 正确率：0.00%"), "Arial", 10);
    label->setTextColor(C4B_GRAY);
    this->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_RIGHT);
    label->setPosition(Vec2(origin.x + visibleSize.width - 5.0f, origin.y + y0 + 35.0f));
    _countLabel = label;

    label = Label::createWithSystemFont(__UTF8("解题正确后自动跳转"), "Arial", 12);
    label->setTextColor(C4B_BLACK);
    this->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_RIGHT);
    label->setPosition(Vec2(origin.x + visibleSize.width - 5.0f, origin.y + y0 + 10.0f));

    ui::CheckBox *checkBox = UICommon::createCheckBox();
    this->addChild(checkBox);
    checkBox->setZoomScale(0.0f);
    checkBox->ignoreContentAdaptWithSize(false);
    checkBox->setContentSize(Size(20.0f, 20.0f));
    checkBox->setPosition(Vec2(origin.x + visibleSize.width - 20.0f - label->getContentSize().width, origin.y + y0 + 10.0f));
    checkBox->setSelected(true);
    checkBox->addEventListener([this](Ref *, ui::CheckBox::EventType type) {
        _autoJump = (type == ui::CheckBox::EventType::SELECTED);
    });
    _jumpCheck = checkBox;

    button = UICommon::createButton();
    button->setScale9Enabled(true);
    button->setContentSize(Size(55.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText(__UTF8("跳过此题"));
    this->addChild(button);
    button->setPosition(Vec2(origin.x + 32.5f, origin.y + y0 + 35.0f));
    button->addClickEventListener(std::bind(&TrainingScene::onSkipButton, this, std::placeholders::_1));
    _skipButton = button;

    button = UICommon::createButton();
    button->setScale9Enabled(true);
    button->setContentSize(Size(55.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText(__UTF8("查看答案"));
    this->addChild(button);
    button->setPosition(Vec2(origin.x + 32.5f, origin.y + y0 + 10.0f));
    button->addClickEventListener(std::bind(&TrainingScene::onAnswerButton, this, std::placeholders::_1));
    _answerButton = button;

    const float tileScale = CC_CONTENT_SCALE_FACTOR() * 20.0f / TILE_WIDTH;
    for (uint8_t i = 0; i < 2; ++i) {
        Sprite *sprite = Sprite::create(tilesImageName[mahjong::TILE_1m]);
        this->addChild(sprite);
        sprite->setScale(tileScale);
        sprite->setPosition(Vec2(origin.x + 80.0f + 20.0f * i, origin.y + y0 + 15.0f));
        sprite->setVisible(false);
        _answerTiles[i] = sprite;
    }

    // 每行排n个间隔5
    // w-20n-5(n+1)>=0
    // w-5>=25n
    // n<=(w-5)/25
    _countsPerRow = static_cast<unsigned>(floorf((visibleSize.width - 5.0f) / 25.0f));
    _resultPos.x = (visibleSize.width - _countsPerRow * 25.0f + 5.0f) * 0.5f;

    // 最多排n行
    // h-20n-5(n+1)>=0
    // h-5>=25n
    // n<=(h-5)/25
    float rows = floorf((y0 - 5.0f) / 25.0f);
    //_resultPos.y = y0 - (y0 - rows * 25.0f + 5.0f) * 0.5f - 20.0f;
    // 化简得下式
    _resultPos.y = y0 * 0.5f + rows * 12.5f - 22.5f;

    LayerColor *layer = LayerColor::create(Color4B(44, 121, 178, 255), 20.0f, 20.0f);
    this->addChild(layer);
    layer->setPosition(Vec2(origin.x + _resultPos.x, origin.y + _resultPos.y));
    _currentNode = layer;

    if (LIKELY(!s_puzzles.empty())) {
        setPuzzle();
    }
    else {
        scheduleOnce([this](float) {
            loadPuzzle();
        }, 0.0f, "load_puzzle");
    }

    return true;
}

void TrainingScene::loadPuzzle() {
    LoadingView *loadingView = LoadingView::create();
    loadingView->showInScene(this);

    auto thiz = makeRef(this);  // 保证线程回来之前不析构

    auto puzzles = std::make_shared<std::vector<PuzzleTemplate> >();
    AsyncTaskPool::getInstance()->enqueue(AsyncTaskPool::TaskType::TASK_IO, [thiz, loadingView, puzzles](void *) {
        s_puzzles.swap(*puzzles);
        if (LIKELY(thiz->isRunning())) {
            loadingView->dismiss();
            thiz->setPuzzle();
        }
    }, nullptr, [puzzles] {
        std::string path = FileUtils::getInstance()->getWritablePath();
        path.append("puzzles.txt");
        if (!FileUtils::getInstance()->isFileExist(path)) {
            path = "text/puzzles.txt";
            CCLOG("use default puzzles");
        }
        std::string content = FileUtils::getInstance()->getStringFromFile(path);
        loadTemplates(content.data(), content.size(), *puzzles);
    });
}

void TrainingScene::setPuzzle() {
    mahjong::hand_tiles_t handTiles;
    mahjong::tile_t servingTile;
    _answer = generatePuzzle(&handTiles, &servingTile);
    _handTilesWidget->setData(handTiles, servingTile);
    _newPuzzle = true;
    _right = false;
    _answerTiles[0]->setVisible(false);
    _answerTiles[1]->setVisible(false);
    addResult();
}

void TrainingScene::onAnswerButton(Ref *sender) {
    // 新题，记一个错误
    if (_newPuzzle) {
        _newPuzzle = false;

        ++_totalCount;
        refreshRate();
        setResult(false);
    }

    for (uint8_t i = 0, c = 0; i < 64; ++i) {
        if (_answer & (1ULL << i)) {
            Sprite *sprite = _answerTiles[c++];
            sprite->setTexture(Director::getInstance()->getTextureCache()->addImage(tilesImageName[i]));
            sprite->setVisible(true);
        }
    }
}

void TrainingScene::onSkipButton(Ref *sender) {
    // 新题，记一个错误
    if (_newPuzzle) {
        _newPuzzle = false;

        ++_totalCount;
        refreshRate();
        setResult(false);
    }

    scheduleOnce([this](float) { setPuzzle(); }, 0.0f, "new_puzzle");
}

void TrainingScene::onStandingTileEvent() {
    // 正确过，就不再可点击了
    if (_right) return;

    mahjong::tile_t discardTile = _handTilesWidget->getCurrentTile();
    if (discardTile == 0 || _handTilesWidget->getServingTile() == 0) {
        return;
    }

    if (_answer & (1ULL << discardTile)) {
        // 答对了
        _right = true;
        if (_newPuzzle) {
            ++_rightCount;
        }

        _handTilesWidget->discardCurrentTile();

        // 自动跳转
        if (_autoJump) {
            scheduleOnce([this](float) { setPuzzle(); }, 0.0f, "new_puzzle");
        }
    }

    if (_newPuzzle) {
        ++_totalCount;
        refreshRate();
        setResult(_right);
    }

    // 答过一次就不是新题了
    _newPuzzle = false;
}

void TrainingScene::refreshRate() {
    char str[128];
    snprintf(str, sizeof(str), __UTF8("%u/%u 正确率：%.2f%%"), _rightCount, _totalCount, percentage_of(_rightCount, _totalCount));
    _countLabel->setString(str);
}

void TrainingScene::addResult() {
    auto size = _resultNode.size();
    auto col = size % _countsPerRow;

    // 一行已经满了
    if (UNLIKELY(col == 0)) {
        _currentNode->setPositionX(_viewRect.origin.x + _resultPos.x);

        // 所有的向下移动
        for (auto it = _resultNode.begin(); it != _resultNode.end(); ) {
            float y = (*it)->getPositionY();
            y -= 25.0f;
            if (LIKELY(y >= _viewRect.origin.y + 5.0f)) {
                (*it)->setPositionY(y);
                ++it;
            }
            else {
                // 超出屏幕的删除
                (*it)->removeFromParentAndCleanup(true);
                it = _resultNode.erase(it);
            }
        }
    }
    else {
        _currentNode->setPositionX(_viewRect.origin.x + _resultPos.x + 25.0f * col);
    }
    _currentNode->setVisible(true);
}

void TrainingScene::setResult(bool right) {
    static const Color4B Red(254, 87, 110, 255);
    static const Color4B Green(45, 175, 90, 255);

    // 根据已有的个数计算所在列
    auto size = _resultNode.size();
    auto col = size % _countsPerRow;

    LayerColor *layer = LayerColor::create(right ? Green : Red, 20.0f, 20.0f);
    this->addChild(layer);
    layer->setPosition(Vec2(_viewRect.origin.x + _resultPos.x + 25.0f * col, _viewRect.origin.y + _resultPos.y));
    _resultNode.push_back(layer);

    Label *label = Label::createWithSystemFont(right ? "\xE2\x9C\x93" : "\xE2\x9C\x95", "Arial", 16);
    //Label *label = Label::createWithSystemFont(right ? "\xE2\x9C\x94" : "\xE2\x9C\x96", "Arial", 16);
    layer->addChild(label);
    label->setPosition(Vec2(10.0f, 10.0f));

    _currentNode->setVisible(false);
}

void TrainingScene::removeAllResults() {
    std::for_each(_resultNode.begin(), _resultNode.end(), [](Node *n) { n->removeFromParentAndCleanup(true); });
    _resultNode.clear();
}

void TrainingScene::onMoreButton(Ref *sender) {
    Vec2 pos = ((ui::Button *)sender)->getPosition();
    pos.y -= 15.0f;
    PopupMenu *menu = PopupMenu::create(this, { __UTF8("更新题库") }, pos, Vec2::ANCHOR_TOP_RIGHT);
    menu->setMenuItemCallback([this](PopupMenu *, size_t idx) {
        switch (idx) {
        case 0: requestLatestPuzzles(); break;
        default: UNREACHABLE(); break;
        }
        //Director::getInstance()->replaceScene(TrainingScene::create());
    });
    menu->show();
}

void TrainingScene::startNormal() {
    _jumpCheck->setEnabled(true);
    _skipButton->setEnabled(true);
    _answerButton->setEnabled(true);
    _shaderWidget->setVisible(false);

    _totalCount = 0;
    _rightCount = 0;
    refreshRate();
    removeAllResults();

    setPuzzle();
}

void TrainingScene::requestLatestPuzzles() {
    LoadingView *loadingView = LoadingView::create();
    loadingView->showInScene(this);

    network::HttpRequest *request = new (std::nothrow) network::HttpRequest();
    request->setRequestType(network::HttpRequest::Type::GET);
    request->setUrl("https://gitee.com/summerinsects/ChineseOfficialMahjongHelperDataSource/raw/master/training/puzzles.txt");

    auto thiz = makeRef(this);  // 保证线程回来之前不析构
    request->setResponseCallback([thiz, loadingView](network::HttpClient *client, network::HttpResponse *response) {
        CC_UNUSED_PARAM(client);

        network::HttpClient::destroyInstance();

        if (UNLIKELY(!thiz->isRunning())) {
            return;
        }

        if (response == nullptr) {
            loadingView->dismiss();
            Toast::makeText(thiz.get(), __UTF8("更新题库失败！"), Toast::LENGTH_LONG)->show();
            return;
        }

        log("HTTP Status Code: %ld", response->getResponseCode());

        if (!response->isSucceed()) {
            log("response failed");
            log("error buffer: %s", response->getErrorBuffer());
            loadingView->dismiss();
            Toast::makeText(thiz.get(), __UTF8("更新题库失败！"), Toast::LENGTH_LONG)->show();
            return;
        }

        std::vector<char> *buffer = response->getResponseData();
        if (buffer == nullptr) {
            loadingView->dismiss();
            return;
        }

        auto buf = std::make_shared<std::vector<char> >(std::move(*buffer));
        auto puzzles = std::make_shared<std::vector<PuzzleTemplate> >();
        AsyncTaskPool::getInstance()->enqueue(AsyncTaskPool::TaskType::TASK_IO, [thiz, loadingView, puzzles](void *) {
            if (!puzzles->empty()) {
                s_puzzles.swap(*puzzles);
                if (LIKELY(thiz->isRunning())) {
                    loadingView->dismiss();
                    Toast::makeText(thiz.get(), __UTF8("更新题库成功！"), Toast::LENGTH_LONG)->show();
                    thiz->startNormal();
                }
            }
            else {
                if (LIKELY(thiz->isRunning())) {
                    loadingView->dismiss();
                    Toast::makeText(thiz.get(), __UTF8("更新题库失败！"), Toast::LENGTH_LONG)->show();
                }
            }
        }, nullptr, [buf, puzzles] {
            loadTemplates(buf->data(), buf->size(), *puzzles);
            if (puzzles->size() > 0) {
                std::string path = FileUtils::getInstance()->getWritablePath();
                path.append("puzzles.txt");
                std::ofstream os(path, std::ios::out);
                if (os.good()) {
                    os.write(buf->data(), buf->size());
                }
            }
        });
    });

    network::HttpClient::getInstance()->send(request);
    request->release();
}
