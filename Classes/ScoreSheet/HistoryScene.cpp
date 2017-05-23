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

#include "json/stringbuffer.h"
#include "json/prettywriter.h"

USING_NS_CC;

static std::vector<Record> g_records;
static std::mutex g_mutex;

Scene *HistoryScene::createScene(const std::function<void (Record *)> &viewCallback) {
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
        rapidjson::Document doc;
        doc.Parse<0>(str.c_str());
        if (doc.HasParseError() || !doc.IsArray()) {
            return;
        }

        records.clear();
        records.reserve(doc.Size());
        std::transform(doc.Begin(), doc.End(), std::back_inserter(records), [](const rapidjson::Value &json) {
            Record record;
            JsonToRecord(json, record);
            return record;
        });

        std::sort(records.begin(), records.end(), [](const Record &r1, const Record &r2) { return r1.start_time > r2.start_time; });
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
            rapidjson::Document doc(rapidjson::Type::kArrayType);
            std::for_each(records.begin(), records.end(), [&doc](const Record &record) {
                rapidjson::Value json(rapidjson::Type::kObjectType);
                RecordToJson(record, json, doc.GetAllocator());
                doc.PushBack(std::move(json), doc.GetAllocator());
            });

            rapidjson::StringBuffer buf;
            rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buf);
            doc.Accept(writer);

            fwrite(buf.GetString(), 1, buf.GetSize(), file);
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

void HistoryScene::onEnter() {
    Layer::onEnter();

    if (LIKELY(!g_records.empty())) {
        updateRecordTexts();
        _tableView->reloadData();
    }
}

ssize_t HistoryScene::numberOfCellsInTableView(cw::TableView *table) {
    return _recordTexts.size();
}

cocos2d::Size HistoryScene::tableCellSizeForIndex(cw::TableView *table, ssize_t idx) {
    return Size(0, 70);
}

cw::TableViewCell *HistoryScene::tableCellAtIndex(cw::TableView *table, ssize_t idx) {
    typedef cw::TableViewCellEx<LayerColor *[2], Label *, ui::Button *> CustomCell;
    CustomCell *cell = (CustomCell *)table->dequeueCell();

    if (cell == nullptr) {
        cell = CustomCell::create();

        Size visibleSize = Director::getInstance()->getVisibleSize();
        const float width = visibleSize.width - 10.0f;

        CustomCell::ExtDataType &ext = cell->getExtData();
        LayerColor *(&layerColor)[2] = std::get<0>(ext);
        Label *&label = std::get<1>(ext);
        ui::Button *&delBtn = std::get<2>(ext);

        layerColor[0] = LayerColor::create(Color4B(0xC0, 0xC0, 0xC0, 0x10), width, 68);
        cell->addChild(layerColor[0]);
        layerColor[0]->setPosition(Vec2(0, 1));

        layerColor[1] = LayerColor::create(Color4B(0x80, 0x80, 0x80, 0x10), width, 68);
        cell->addChild(layerColor[1]);
        layerColor[1]->setPosition(Vec2(0, 1));

        label = Label::createWithSystemFont("", "Arail", 10);
        label->setColor(Color3B::BLACK);
        cell->addChild(label);
        label->setPosition(Vec2(2.0f, 35.0f));
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);

        delBtn = ui::Button::create("drawable/btn_trash_bin.png");
        delBtn->setScale(Director::getInstance()->getContentScaleFactor() * 0.5f);
        delBtn->addClickEventListener(std::bind(&HistoryScene::onDeleteButton, this, std::placeholders::_1));
        cell->addChild(delBtn);
        delBtn->setPosition(Vec2(width - 20.0f, 35.0f));

        cell->setContentSize(Size(width, 70));
        cell->setTouchEnabled(true);
        cell->addTouchEventListener(std::bind(&HistoryScene::onCellEvent, this, std::placeholders::_1, std::placeholders::_2));
    }

    const CustomCell::ExtDataType &ext = cell->getExtData();
    LayerColor *const (&layerColor)[2] = std::get<0>(ext);
    Label *label = std::get<1>(ext);
    ui::Button *delBtn = std::get<2>(ext);

    layerColor[0]->setVisible(!(idx & 1));
    layerColor[1]->setVisible(!!(idx & 1));

    delBtn->setUserData(reinterpret_cast<void *>(idx));

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
    AlertView::showWithMessage("删除记录", "删除后无法找回，确认删除？", 12, [this, idx]() {
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

void HistoryScene::onCellEvent(cocos2d::Ref *sender, cocos2d::ui::Widget::TouchEventType event) {
    if (event != ui::Widget::TouchEventType::ENDED) {
        return;
    }

    cw::TableViewCell *cell = (cw::TableViewCell *)sender;
    _viewCallback(&g_records[cell->getIdx()]);
}

static void __modifyRecord(const Record *record) {
    auto it = std::find_if(g_records.begin(), g_records.end(), [&record](const Record &r) {
        return (&r == record);  // 同一个记录
    });

    if (it == g_records.end()) {
        g_records.push_back(*record);
    }

    std::sort(g_records.begin(), g_records.end(), [](const Record &r1, const Record &r2) { return r1.start_time > r2.start_time; });

    std::vector<Record> temp = g_records;
    std::thread([temp]() {
        std::lock_guard<std::mutex> lg(g_mutex);
        saveRecords(temp);
    }).detach();
}

void HistoryScene::modifyRecord(const Record *record) {
    if (UNLIKELY(g_records.empty())) {
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
