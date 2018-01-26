#include "CompetitionHistoryScene.h"
#include <thread>
#include <mutex>
#include <array>
#include "Competition.h"
#include "../widget/AlertDialog.h"
#include "../widget/LoadingView.h"

USING_NS_CC;

static std::vector<CompetitionData> g_competitions;
static std::mutex g_mutex;

static void loadCompetitions(std::vector<CompetitionData> &competitions) {
    std::lock_guard<std::mutex> lg(g_mutex);

    std::string fileName = FileUtils::getInstance()->getWritablePath();
    fileName.append("history_competition.json");
    LoadHistoryCompetitions(fileName.c_str(), competitions);
}

static void saveCompetitions(const std::vector<CompetitionData> &competitions) {
    std::lock_guard<std::mutex> lg(g_mutex);

    std::string fileName = FileUtils::getInstance()->getWritablePath();
    fileName.append("history_competition.json");
    SaveHistoryCompetitions(fileName.c_str(), competitions);
}

#define BUF_SIZE 511

void CompetitionHistoryScene::updateDataTexts() {
    _dataTexts.clear();
    _dataTexts.reserve(g_competitions.size());

    std::transform(g_competitions.begin(), g_competitions.end(), std::back_inserter(_dataTexts), [](const CompetitionData &data)->std::string {
        char str[BUF_SIZE + 1];
        str[BUF_SIZE] = '\0';

        struct tm ret = *localtime(&data.start_time);
        int len = snprintf(str, BUF_SIZE, __UTF8("%d年%d月%d日%.2d:%.2d"), ret.tm_year + 1900, ret.tm_mon + 1, ret.tm_mday, ret.tm_hour, ret.tm_min);
        if (data.finish_time != 0) {
            ret = *localtime(&data.finish_time);
            len += snprintf(str + len, static_cast<size_t>(BUF_SIZE - len), __UTF8("——%d年%d月%d日%.2d:%.2d"), ret.tm_year + 1900, ret.tm_mon + 1, ret.tm_mday, ret.tm_hour, ret.tm_min);
        }
        else {
            len += snprintf(str + len, static_cast<size_t>(BUF_SIZE - len), "%s", __UTF8("——(未结束)"));
        }

        len += snprintf(str + len, static_cast<size_t>(BUF_SIZE - len), "\n%s", data.name.c_str());
        snprintf(str + len, static_cast<size_t>(BUF_SIZE - len), __UTF8("\n%") __UTF8(PRIzd) __UTF8("人，%") __UTF8(PRIzd) __UTF8("轮"), data.players.size(), data.round_count);

        return str;
    });
}

bool CompetitionHistoryScene::initWithCallback(const ViewCallback &viewCallback) {
    if (UNLIKELY(!BaseScene::initWithTitle(__UTF8("历史记录")))) {
        return false;
    }

    _viewCallback = viewCallback;

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    cw::TableView *tableView = cw::TableView::create();
    tableView->setDirection(ui::ScrollView::Direction::VERTICAL);
    tableView->setScrollBarPositionFromCorner(Vec2(2.0f, 2.0f));
    tableView->setScrollBarWidth(4.0f);
    tableView->setScrollBarOpacity(0x99);
    tableView->setContentSize(Size(visibleSize.width - 5.0f, visibleSize.height - 35.0f));
    tableView->setDelegate(this);
    tableView->setVerticalFillOrder(cw::TableView::VerticalFillOrder::TOP_DOWN);

    tableView->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    tableView->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f - 15.0f));
    this->addChild(tableView);
    _tableView = tableView;

    tableView->setOnEnterCallback([this]() {
        if (LIKELY(!g_competitions.empty())) {
            updateDataTexts();
            _tableView->reloadDataInplacement();
        }
    });

#if 1
    if (UNLIKELY(g_competitions.empty())) {
        this->scheduleOnce([this](float) {
            Vec2 origin = Director::getInstance()->getVisibleOrigin();

            LoadingView *loadingView = LoadingView::create();
            this->addChild(loadingView);
            loadingView->setPosition(origin);

            auto thiz = makeRef(this);  // 保证线程回来之前不析构
            std::thread([thiz, loadingView]() {
                auto temp = std::make_shared<std::vector<CompetitionData> >();
                loadCompetitions(*temp);

                // 切换到cocos线程
                Director::getInstance()->getScheduler()->performFunctionInCocosThread([thiz, loadingView, temp]() {
                    g_competitions.swap(*temp);

                    if (LIKELY(thiz->isRunning())) {
                        thiz->updateDataTexts();
                        loadingView->removeFromParent();
                        thiz->_tableView->reloadData();
                    }
                });
            }).detach();
        }, 0.0f, "load_competitions");
    }
#else
    // 测试代码
    g_competitions.resize(50);
    for (int i = 0; i < 50; ++i) {
        g_competitions[i].name = Common::format(__UTF8("测试比赛%d"), i + 1);
        g_competitions[i].start_time = time(nullptr) - 2000;
        g_competitions[i].finish_time = time(nullptr);
    }
#endif
    return true;
}

ssize_t CompetitionHistoryScene::numberOfCellsInTableView(cw::TableView *) {
    return _dataTexts.size();
}

float CompetitionHistoryScene::tableCellSizeForIndex(cw::TableView *, ssize_t) {
    return 40.0f;
}

cw::TableViewCell *CompetitionHistoryScene::tableCellAtIndex(cw::TableView *table, ssize_t idx) {
    typedef cw::TableViewCellEx<std::array<LayerColor *, 2>, Label *, ui::Button *> CustomCell;
    CustomCell *cell = (CustomCell *)table->dequeueCell();

    if (cell == nullptr) {
        cell = CustomCell::create();

        Size visibleSize = Director::getInstance()->getVisibleSize();
        const float width = visibleSize.width - 5.0f;

        CustomCell::ExtDataType &ext = cell->getExtData();
        LayerColor **layerColors = std::get<0>(ext).data();
        Label *&label = std::get<1>(ext);
        ui::Button *&delBtn = std::get<2>(ext);

        layerColors[0] = LayerColor::create(Color4B(0x10, 0x10, 0x10, 0x10), width, 40.0f);
        cell->addChild(layerColors[0]);

        layerColors[1] = LayerColor::create(Color4B(0xC0, 0xC0, 0xC0, 0x10), width, 40.0f);
        cell->addChild(layerColors[1]);

        label = Label::createWithSystemFont("", "Arail", 10);
        label->setColor(Color3B::BLACK);
        cell->addChild(label);
        label->setPosition(Vec2(2.0f, 20.0f));
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);

        delBtn = ui::Button::create("drawable/btn_trash_bin.png");
        delBtn->setScale(Director::getInstance()->getContentScaleFactor() * 0.5f);
        delBtn->addClickEventListener(std::bind(&CompetitionHistoryScene::onDeleteButton, this, std::placeholders::_1));
        cell->addChild(delBtn);
        delBtn->setPosition(Vec2(width - 20.0f, 20.0f));

        cell->setContentSize(Size(width, 40.0f));
        cell->setTouchEnabled(true);
        cell->addClickEventListener(std::bind(&CompetitionHistoryScene::onCellClicked, this, std::placeholders::_1));
    }

    const CustomCell::ExtDataType &ext = cell->getExtData();
    LayerColor *const *layerColors = std::get<0>(ext).data();
    Label *label = std::get<1>(ext);
    ui::Button *delBtn = std::get<2>(ext);

    layerColors[0]->setVisible((idx & 1) == 0);
    layerColors[1]->setVisible((idx & 1) != 0);

    delBtn->setUserData(reinterpret_cast<void *>(idx));

    label->setString(_dataTexts[idx]);

    return cell;
}

void CompetitionHistoryScene::onDeleteButton(cocos2d::Ref *sender) {
    ui::Button *button = (ui::Button *)sender;
    size_t idx = reinterpret_cast<size_t>(button->getUserData());

    AlertDialog::Builder(this)
        .setTitle(__UTF8("删除记录"))
        .setMessage(__UTF8("删除后无法找回，确认删除？"))
        .setNegativeButton(__UTF8("取消"), nullptr)
        .setPositiveButton(__UTF8("确定"), [this, idx](AlertDialog *, int) {
        LoadingView *loadingView = LoadingView::create();
        this->addChild(loadingView);
        loadingView->setPosition(Director::getInstance()->getVisibleSize());

        g_competitions.erase(g_competitions.begin() + idx);

        auto thiz = makeRef(this);  // 保证线程回来之前不析构
        auto temp = std::make_shared<std::vector<CompetitionData> >(g_competitions);
        std::thread([thiz, temp, loadingView](){
            saveCompetitions(*temp);

            // 切换到cocos线程
            Director::getInstance()->getScheduler()->performFunctionInCocosThread([thiz, loadingView]() {
                if (LIKELY(thiz->isRunning())) {
                    thiz->updateDataTexts();
                    loadingView->removeFromParent();
                    thiz->_tableView->reloadDataInplacement();
                }
            });
        }).detach();
        return true;
    }).create()->show();
}

void CompetitionHistoryScene::onCellClicked(cocos2d::Ref *sender) {
    cw::TableViewCell *cell = (cw::TableViewCell *)sender;
    _viewCallback(&g_competitions[cell->getIdx()]);
}

void CompetitionHistoryScene::modifyData(const CompetitionData *data) {
    if (UNLIKELY(g_competitions.empty())) {  // 如果当前没有加载过历史记录
        auto d = std::make_shared<CompetitionData>(*data);  // 复制当前记录
        // 子线程中加载、修改、保存
        std::thread([d]() {
            auto competitions = std::make_shared<std::vector<CompetitionData> >();
            loadCompetitions(*competitions);
            ModifyCompetitionInHistory(*competitions, d.get());
            saveCompetitions(*competitions);

            // 切换到主线程，覆盖整个历史记录
            Director::getInstance()->getScheduler()->performFunctionInCocosThread([competitions]() {
                g_competitions.swap(*competitions);
            });
        }).detach();
    }
    else {  // 如果当前加载过历史记录
        // 直接修改
        ModifyCompetitionInHistory(g_competitions, data);

        // 子线程中保存
        auto temp = std::make_shared<std::vector<CompetitionData> >(g_competitions);
        std::thread([temp]() {
            saveCompetitions(*temp);
        }).detach();
    }
}
