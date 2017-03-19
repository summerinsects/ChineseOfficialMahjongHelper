#ifdef _MSC_VER
#pragma warning(disable: 4351)
#endif

#include "HistoryScene.h"
#include "Record.h"
#include "../widget/AlertView.h"
#include "../widget/LoadingView.h"
#include "../widget/CWTableView.h"
#include <thread>
#include <mutex>

USING_NS_CC;

static std::vector<Record> g_records;
static std::mutex g_mutex;

Scene *HistoryScene::createScene(const std::function<bool (const Record &)> &viewCallback) {
    auto scene = Scene::create();
    auto layer = HistoryScene::create();
    layer->_viewCallback = viewCallback;
    scene->addChild(layer);
    return scene;
}

static void loadRecords(std::vector<Record> &records) {
    std::string fileName = FileUtils::getInstance()->getWritablePath();
    fileName.append("history_record.json");
    std::string str = FileUtils::getInstance()->getStringFromFile(fileName);

    try {
        jw::cppJSON json;
        json.Parse(str.c_str());

        records.clear();
        records.reserve(json.size());
        std::transform(json.begin(), json.end(), std::back_inserter(records), [](const jw::cppJSON &json) {
            Record record;
            fromJson(&record, json);
            return record;
        });
    }
    catch (std::exception &e) {
        CCLOG("%s %s", __FUNCTION__, e.what());
    }
}

static void saveRecords(const std::vector<Record> &records) {
    std::string fileName = FileUtils::getInstance()->getWritablePath();
    fileName.append("history_record.json");
    FILE *file = fopen(fileName.c_str(), "wb");
    if (LIKELY(file != nullptr)) {
        try {
            jw::cppJSON json(jw::cppJSON::ValueType::Array);
            std::transform(records.begin(), records.end(), std::back_inserter(json), [](const Record &record) {
                jw::cppJSON json(jw::cppJSON::ValueType::Object);
                toJson(record, &json);
                return json;
            });
            std::string str = json.stringfiy();
            fwrite(str.c_str(), 1, str.length(), file);
        }
        catch (std::exception &e) {
            CCLOG("%s %s", __FUNCTION__, e.what());
        }
        fclose(file);
    }
}

void HistoryScene::updateRecordTexts() {
    _recordTexts.clear();
    _recordTexts.reserve(g_records.size());

    std::transform(g_records.begin(), g_records.end(), std::back_inserter(_recordTexts), [](const Record &record) {
        typedef std::pair<int, int> SeatScore;
        SeatScore seatscore[4];
        seatscore[0].first = 0; seatscore[0].second = 0;
        seatscore[1].first = 1, seatscore[1].second = 0;
        seatscore[2].first = 2, seatscore[2].second = 0;
        seatscore[3].first = 3, seatscore[3].second = 0;
        for (int i = 0; i < 16; ++i) {
            int s[4];
            translateDetailToScoreTable(record.detail[i], s);
            seatscore[0].second += s[0];
            seatscore[1].second += s[1];
            seatscore[2].second += s[2];
            seatscore[3].second += s[3];
        }

        std::stable_sort(std::begin(seatscore), std::end(seatscore),
            [](const SeatScore &left, const SeatScore &right) {
            return left.second > right.second;
        });

        RecordText rt;

        strftime(rt.startTime, sizeof(rt.startTime), "%Y-%m-%d %H:%M", localtime(&record.start_time));
        if (record.end_time != 0) {
            strftime(rt.endTime, sizeof(rt.endTime), "%Y-%m-%d %H:%M", localtime(&record.end_time));
        }
        else {
            rt.endTime[0] = '\0';
        }

        static const char *seatText[] = { "东", "南", "西", "北" };
        snprintf(rt.score, sizeof(rt.score), "%s: %s(%+d)\n%s: %s(%+d)\n%s: %s(%+d)\n%s: %s(%+d)",
            seatText[seatscore[0].first], record.name[seatscore[0].first], seatscore[0].second,
            seatText[seatscore[1].first], record.name[seatscore[1].first], seatscore[1].second,
            seatText[seatscore[2].first], record.name[seatscore[2].first], seatscore[2].second,
            seatText[seatscore[3].first], record.name[seatscore[3].first], seatscore[3].second);
        return rt;
    });
}

bool HistoryScene::init() {
    if (UNLIKELY(!BaseLayer::initWithTitle("历史记录"))) {
        return false;
    }

    updateRecordTexts();

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    _tableView = cw::TableView::create();
    _tableView->setContentSize(Size(visibleSize.width - 10.0f, visibleSize.height - 35));
    _tableView->setDelegate(this);
    _tableView->setDirection(ui::ScrollView::Direction::VERTICAL);
    _tableView->setVerticalFillOrder(cw::TableView::VerticalFillOrder::TOP_DOWN);

    _tableView->setScrollBarPositionFromCorner(Vec2(5, 5));
    _tableView->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    _tableView->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f - 15.0f));
    _tableView->reloadData();
    this->addChild(_tableView);

    if (UNLIKELY(g_records.empty())) {
        LoadingView *loadingView = LoadingView::create();
        this->addChild(loadingView);
        loadingView->setPosition(origin);

        auto thiz = RefPtr<HistoryScene>(this);

        std::thread([thiz, loadingView]() {
            std::vector<Record> temp;
            std::lock_guard<std::mutex> lg(g_mutex);
            loadRecords(temp);

            Director::getInstance()->getScheduler()->performFunctionInCocosThread([thiz, loadingView, temp]() mutable {
                g_records.swap(temp);

                if (LIKELY(thiz->getParent() != nullptr)) {
                    thiz->updateRecordTexts();
                    loadingView->removeFromParent();
                    thiz->_tableView->reloadData();
                }
            });
        }).detach();
    }

    return true;
}

ssize_t HistoryScene::numberOfCellsInTableView(cw::TableView *table) {
    return _recordTexts.size();
}

cocos2d::Size HistoryScene::tableCellSizeForIndex(cw::TableView *table, ssize_t idx) {
    return Size(0, 70);
}

cw::TableViewCell *HistoryScene::tableCellAtIndex(cw::TableView *table, ssize_t idx) {
    typedef cw::TableViewCellEx<LayerColor *[2], Label *, ui::Button *, ui::Button *> CustomCell;
    CustomCell *cell = (CustomCell *)table->dequeueCell();

    if (cell == nullptr) {
        cell = CustomCell::create();

        Size visibleSize = Director::getInstance()->getVisibleSize();
        const float width = visibleSize.width - 10.0f;

        CustomCell::ExtDataType &ext = cell->getExtData();
        LayerColor *(&layerColor)[2] = std::get<0>(ext);
        Label *&label = std::get<1>(ext);
        ui::Button *&delBtn = std::get<2>(ext);
        ui::Button *&viewBtn = std::get<3>(ext);

        layerColor[0] = LayerColor::create(Color4B::WHITE, width, 68);
        cell->addChild(layerColor[0]);
        layerColor[0]->setPosition(Vec2(0, 1));

        layerColor[1] = LayerColor::create(Color4B(239, 243, 247, 255), width, 68);
        cell->addChild(layerColor[1]);
        layerColor[1]->setPosition(Vec2(0, 1));

        label = Label::createWithSystemFont("", "Arail", 10);
        label->setColor(Color3B::BLACK);
        cell->addChild(label);
        label->setPosition(Vec2(2.0f, 35.0f));
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);

        delBtn = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
        delBtn->setScale9Enabled(true);
        delBtn->setContentSize(Size(40.0f, 20.0f));
        delBtn->setTitleFontSize(12);
        delBtn->setTitleText("删除");
        delBtn->addClickEventListener(std::bind(&HistoryScene::onDeleteButton, this, std::placeholders::_1));
        cell->addChild(delBtn);
        delBtn->setPosition(Vec2(width - 25.0f, 50.0f));

        viewBtn = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
        viewBtn->setScale9Enabled(true);
        viewBtn->setContentSize(Size(40.0f, 20.0f));
        viewBtn->setTitleFontSize(12);
        viewBtn->setTitleText("查看");
        viewBtn->addClickEventListener(std::bind(&HistoryScene::onViewButton, this, std::placeholders::_1));
        cell->addChild(viewBtn);
        viewBtn->setPosition(Vec2(width - 25.0f, 20.0f));
    }

    const CustomCell::ExtDataType &ext = cell->getExtData();
    LayerColor *const (&layerColor)[2] = std::get<0>(ext);
    Label *label = std::get<1>(ext);
    ui::Button *delBtn = std::get<2>(ext);
    ui::Button *viewBtn = std::get<3>(ext);

    layerColor[0]->setVisible(!(idx & 1));
    layerColor[1]->setVisible(!!(idx & 1));

    delBtn->setUserData(reinterpret_cast<void *>(idx));
    viewBtn->setUserData(reinterpret_cast<void *>(idx));

    const RecordText &rt = _recordTexts[idx];

    std::string str = rt.startTime;
    if (rt.endTime[0] != '\0') {
        str.append(" -- ");
        str.append(rt.endTime);
    }
    else {
        str.append(" (未结束)");
    }
    str.append("\n");
    str.append(rt.score);

    label->setString(str);

    return cell;
}

void HistoryScene::onDeleteButton(cocos2d::Ref *sender) {
    ui::Button *button = (ui::Button *)sender;
    size_t idx = reinterpret_cast<size_t>(button->getUserData());
    AlertView::showWithMessage("删除记录", "删除后无法找回，确认删除？", [this, idx]() {
        LoadingView *loadingView = LoadingView::create();
        this->addChild(loadingView);
        loadingView->setPosition(Director::getInstance()->getVisibleSize());

        g_records.erase(g_records.begin() + idx);
        auto thiz = RefPtr<HistoryScene>(this);

        std::vector<Record> temp = g_records;
        std::thread([thiz, temp, loadingView](){
            std::lock_guard<std::mutex> lg(g_mutex);
            saveRecords(temp);
            Director::getInstance()->getScheduler()->performFunctionInCocosThread([thiz, loadingView]() {
                if (LIKELY(thiz->getParent() != nullptr)) {
                    thiz->updateRecordTexts();
                    loadingView->removeFromParent();
                    thiz->_tableView->reloadDataInplacement();
                }
            });
        }).detach();
    }, nullptr);
}

void HistoryScene::onViewButton(cocos2d::Ref *sender) {
    ui::Button *button = (ui::Button *)sender;
    size_t idx = reinterpret_cast<size_t>(button->getUserData());
    if (_viewCallback(g_records[idx])) {
        Director::getInstance()->popScene();
    }
}

static void __modifyRecord(const Record &record) {
    auto it = std::find_if(g_records.begin(), g_records.end(), [&record](const Record &r) {
        return (r.start_time == record.start_time);  // 我们认为开始时间相同的为同一个记录
    });

    if (it == g_records.end()) {
        g_records.push_back(record);
    }
    else {
        memcpy(&*it, &record, sizeof(Record));
    }

    std::vector<Record> temp = g_records;
    std::thread([temp]() {
        std::lock_guard<std::mutex> lg(g_mutex);
        saveRecords(temp);
    }).detach();
}

void HistoryScene::modifyRecord(const Record &record) {
    if (g_records.empty()) {
        std::thread([record]() {
            std::vector<Record> temp;
            std::lock_guard<std::mutex> lg(g_mutex);
            loadRecords(temp);

            Director::getInstance()->getScheduler()->performFunctionInCocosThread([record, temp]() mutable {
                g_records.swap(temp);
                __modifyRecord(record);
            });
        }).detach();
    }
    else {
        __modifyRecord(record);
    }
}
