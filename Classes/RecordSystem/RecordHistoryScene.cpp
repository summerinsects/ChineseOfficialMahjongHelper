#include "RecordHistoryScene.h"
#include <array>
#include "Record.h"
#include "../widget/AlertView.h"
#include "../widget/LoadingView.h"

USING_NS_CC;

static std::vector<Record> g_records;
static std::mutex g_mutex;

static void loadRecords(std::vector<Record> &records) {
    std::lock_guard<std::mutex> lg(g_mutex);

    std::string fileName = FileUtils::getInstance()->getWritablePath();
    fileName.append("history_record.json");
    LoadHistoryRecords(fileName.c_str(), records);
}

static void saveRecords(const std::vector<Record> &records) {
    std::string fileName = FileUtils::getInstance()->getWritablePath();
    fileName.append("history_record.json");
    SaveHistoryRecords(fileName.c_str(), records);
}

#define BUF_SIZE 511

void RecordHistoryScene::updateRecordTexts() {
    _recordTexts.clear();
    _recordTexts.reserve(g_records.size());

    std::transform(g_records.begin(), g_records.end(), std::back_inserter(_recordTexts), [](const Record &record)->std::string {
        char str[BUF_SIZE + 1];
        str[BUF_SIZE] = '\0';

        struct tm ret = *localtime(&record.start_time);
        int len = snprintf(str, BUF_SIZE, "%d年%d月%d日%.2d:%.2d", ret.tm_year + 1900, ret.tm_mon + 1, ret.tm_mday, ret.tm_hour, ret.tm_min);
        if (record.end_time != 0) {
            ret = *localtime(&record.end_time);
            len += snprintf(str + len, BUF_SIZE - len, "——%d年%d月%d日%.2d:%.2d", ret.tm_year + 1900, ret.tm_mon + 1, ret.tm_mday, ret.tm_hour, ret.tm_min);
        }
        else {
            len += snprintf(str + len, BUF_SIZE - len, "——(未结束)");
        }

        typedef std::pair<int, int> SeatScore;
        SeatScore seatscore[4];
        seatscore[0].first = 0; seatscore[0].second = 0;
        seatscore[1].first = 1, seatscore[1].second = 0;
        seatscore[2].first = 2, seatscore[2].second = 0;
        seatscore[3].first = 3, seatscore[3].second = 0;
        for (int i = 0; i < 16; ++i) {
            int s[4];
            TranslateDetailToScoreTable(record.detail[i], s);
            seatscore[0].second += s[0];
            seatscore[1].second += s[1];
            seatscore[2].second += s[2];
            seatscore[3].second += s[3];
        }

        std::stable_sort(std::begin(seatscore), std::end(seatscore),
            [](const SeatScore &left, const SeatScore &right) {
            return left.second > right.second;
        });

        static const char *seatText[] = { "东", "南", "西", "北" };
        len += snprintf(str + len, BUF_SIZE - len, "\n%s: %s (%+d)\n%s: %s (%+d)\n%s: %s (%+d)\n%s: %s (%+d)",
            seatText[seatscore[0].first], record.name[seatscore[0].first], seatscore[0].second,
            seatText[seatscore[1].first], record.name[seatscore[1].first], seatscore[1].second,
            seatText[seatscore[2].first], record.name[seatscore[2].first], seatscore[2].second,
            seatText[seatscore[3].first], record.name[seatscore[3].first], seatscore[3].second);
        return str;
    });
}

bool RecordHistoryScene::initWithCallback(const ViewCallback &viewCallback) {
    if (UNLIKELY(!BaseScene::initWithTitle("历史记录"))) {
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
        if (LIKELY(!g_records.empty())) {
            updateRecordTexts();
            _tableView->reloadDataInplacement();
        }
    });

    if (UNLIKELY(g_records.empty())) {
        this->scheduleOnce([this](float) {
            Vec2 origin = Director::getInstance()->getVisibleOrigin();

            LoadingView *loadingView = LoadingView::create();
            this->addChild(loadingView);
            loadingView->setPosition(origin);

            auto thiz = makeRef(this);  // 保证线程回来之前不析构
            std::thread([thiz, loadingView]() {
                auto temp = std::make_shared<std::vector<Record> >();
                loadRecords(*temp);

                // 切换到cocos线程
                Director::getInstance()->getScheduler()->performFunctionInCocosThread([thiz, loadingView, temp]() {
                    g_records.swap(*temp);

                    if (LIKELY(thiz->isRunning())) {
                        thiz->updateRecordTexts();
                        loadingView->removeFromParent();
                        thiz->_tableView->reloadData();
                    }
                });
            }).detach();
        }, 0.0f, "load_records");
    }

    return true;
}

ssize_t RecordHistoryScene::numberOfCellsInTableView(cw::TableView *) {
    return _recordTexts.size();
}

float RecordHistoryScene::tableCellSizeForIndex(cw::TableView *, ssize_t) {
    return 70.0f;
}

cw::TableViewCell *RecordHistoryScene::tableCellAtIndex(cw::TableView *table, ssize_t idx) {
    typedef cw::TableViewCellEx<std::array<LayerColor *, 2>, Label *, ui::Button *> CustomCell;
    CustomCell *cell = (CustomCell *)table->dequeueCell();

    if (cell == nullptr) {
        cell = CustomCell::create();

        Size visibleSize = Director::getInstance()->getVisibleSize();
        const float width = visibleSize.width - 5.0f;

        CustomCell::ExtDataType &ext = cell->getExtData();
        std::array<LayerColor *, 2> &layerColors = std::get<0>(ext);
        Label *&label = std::get<1>(ext);
        ui::Button *&delBtn = std::get<2>(ext);

        layerColors[0] = LayerColor::create(Color4B(0x10, 0x10, 0x10, 0x10), width, 70.0f);
        cell->addChild(layerColors[0]);

        layerColors[1] = LayerColor::create(Color4B(0xC0, 0xC0, 0xC0, 0x10), width, 70.0f);
        cell->addChild(layerColors[1]);

        label = Label::createWithSystemFont("", "Arail", 10);
        label->setColor(Color3B::BLACK);
        cell->addChild(label);
        label->setPosition(Vec2(2.0f, 35.0f));
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);

        delBtn = ui::Button::create("drawable/btn_trash_bin.png");
        delBtn->setScale(Director::getInstance()->getContentScaleFactor() * 0.5f);
        delBtn->addClickEventListener(std::bind(&RecordHistoryScene::onDeleteButton, this, std::placeholders::_1));
        cell->addChild(delBtn);
        delBtn->setPosition(Vec2(width - 20.0f, 35.0f));

        cell->setContentSize(Size(width, 70.0f));
        cell->setTouchEnabled(true);
        cell->addClickEventListener(std::bind(&RecordHistoryScene::onCellClicked, this, std::placeholders::_1));
    }

    const CustomCell::ExtDataType &ext = cell->getExtData();
    const std::array<LayerColor *, 2> &layerColors = std::get<0>(ext);
    Label *label = std::get<1>(ext);
    ui::Button *delBtn = std::get<2>(ext);

    layerColors[0]->setVisible(!(idx & 1));
    layerColors[1]->setVisible(!!(idx & 1));

    delBtn->setUserData(reinterpret_cast<void *>(idx));

    label->setString(_recordTexts[idx]);

    return cell;
}

void RecordHistoryScene::onDeleteButton(cocos2d::Ref *sender) {
    ui::Button *button = (ui::Button *)sender;
    size_t idx = reinterpret_cast<size_t>(button->getUserData());
    AlertView::showWithMessage("删除记录", "删除后无法找回，确认删除？", 12, [this, idx]() {
        LoadingView *loadingView = LoadingView::create();
        this->addChild(loadingView);
        loadingView->setPosition(Director::getInstance()->getVisibleSize());

        g_records.erase(g_records.begin() + idx);

        auto thiz = makeRef(this);  // 保证线程回来之前不析构
        auto temp = std::make_shared<std::vector<Record> >(g_records);
        std::thread([thiz, temp, loadingView](){
            saveRecords(*temp);

            // 切换到cocos线程
            Director::getInstance()->getScheduler()->performFunctionInCocosThread([thiz, loadingView]() {
                if (LIKELY(thiz->isRunning())) {
                    thiz->updateRecordTexts();
                    loadingView->removeFromParent();
                    thiz->_tableView->reloadDataInplacement();
                }
            });
        }).detach();
    }, nullptr);
}

void RecordHistoryScene::onCellClicked(cocos2d::Ref *sender) {
    cw::TableViewCell *cell = (cw::TableViewCell *)sender;
    _viewCallback(&g_records[cell->getIdx()]);
}

void RecordHistoryScene::modifyRecord(const Record *record) {
    if (UNLIKELY(g_records.empty())) {  // 如果当前没有加载过历史记录
        auto r = std::make_shared<Record>(*record);  // 复制当前记录
        // 子线程中加载、修改、保存
        std::thread([r]() {
            auto records = std::make_shared<std::vector<Record> >();
            loadRecords(*records);
            ModifyRecordInHistory(*records, r.get());
            saveRecords(*records);

            // 切换到主线程，覆盖整个历史记录
            Director::getInstance()->getScheduler()->performFunctionInCocosThread([records]() {
                g_records.swap(*records);
            });
        }).detach();
    }
    else {  // 如果当前加载过历史记录
        // 直接修改
        ModifyRecordInHistory(g_records, record);

        // 子线程中保存
        auto temp = std::make_shared<std::vector<Record> >(g_records);
        std::thread([temp]() {
            saveRecords(*temp);
        }).detach();
    }
}
