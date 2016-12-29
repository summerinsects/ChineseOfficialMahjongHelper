#include "HistoryScene.h"
#include "Record.h"
#include "../widget/AlertLayer.h"
#include "../widget/CWTableView.h"
#include "../compiler.h"
#include <thread>

#pragma execution_character_set("utf-8")

USING_NS_CC;

static std::vector<Record> g_records;

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
    if (file != nullptr) {
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

static void loadRecordsAsync(const std::function<void ()> &callback) {
    std::thread thread([callback]() {
        std::vector<Record> temp;
        loadRecords(temp);
        Director::getInstance()->getScheduler()->performFunctionInCocosThread([callback, temp]() mutable {
            g_records.swap(temp);
            callback();
        });
    });
    thread.detach();
}

static void saveRecordsAsync(const std::function<void ()> &callback) {
    std::vector<Record> temp = g_records;
    std::thread thread([callback, temp](){
        saveRecords(temp);
        Director::getInstance()->getScheduler()->performFunctionInCocosThread(callback);
    });
    thread.detach();
}

bool HistoryScene::init() {
    if (!BaseLayer::initWithTitle("历史记录")) {
        return false;
    }

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    _tableView = cw::TableView::create();
    _tableView->setContentSize(Size(visibleSize.width - 10.0f, visibleSize.height - 35));
    _tableView->setTableViewCallback([this](cw::TableView *table, cw::TableView::CallbackType type, intptr_t param1, intptr_t param2)->intptr_t {
        switch (type) {
        case cw::TableView::CallbackType::CELL_SIZE: {
            *(Size *)param2 = Size(0, 70);
            return 0;
        }
        case cw::TableView::CallbackType::CELL_AT_INDEX:
            return (intptr_t)tableCellAtIndex(table, param1);
        case cw::TableView::CallbackType::NUMBER_OF_CELLS:
            return (intptr_t)g_records.size();
        }
        return 0;
    });

    _tableView->setDirection(ui::ScrollView::Direction::VERTICAL);
    _tableView->setVerticalFillOrder(cw::TableView::VerticalFillOrder::TOP_DOWN);

    _tableView->setScrollBarPositionFromCorner(Vec2(5, 5));
    _tableView->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    _tableView->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f - 15.0f));
    _tableView->reloadData();
    this->addChild(_tableView);

    if (UNLIKELY(g_records.empty())) {
        _tableView->setEnabled(false);

        Sprite *sprite = Sprite::create("source_material/loading_black.png");
        this->addChild(sprite);
        sprite->setScale(40 / sprite->getContentSize().width);
        sprite->setPosition(_tableView->getPosition());
        sprite->runAction(RepeatForever::create(RotateBy::create(0.5f, 180.0f)));

        auto thiz = RefPtr<HistoryScene>(this);
        loadRecordsAsync([thiz, sprite]() {
            if (LIKELY(thiz->getParent() != nullptr)) {
                sprite->removeFromParent();
                thiz->_tableView->setEnabled(true);
                thiz->_tableView->reloadData();
            }
        });
    }

    return true;
}

cw::TableViewCell *HistoryScene::tableCellAtIndex(cw::TableView *table, ssize_t idx) {
    typedef cw::TableViewCellEx<Label *, ui::Button *, ui::Button *> CustomCell;
    CustomCell *cell = (CustomCell *)table->dequeueCell();

    if (cell == nullptr) {
        cell = CustomCell::create();

        Size visibleSize = Director::getInstance()->getVisibleSize();
        const float width = visibleSize.width - 10.0f;

        CustomCell::ExtDataType &ext = cell->getExtData();
        Label *&label = std::get<0>(ext);
        ui::Button *&delBtn = std::get<1>(ext);
        ui::Button *&viewBtn = std::get<2>(ext);

        Sprite *sprite = Sprite::create("source_material/btn_square_disabled.png");
        cell->addChild(sprite);
        sprite->setPosition(Vec2(width * 0.5f, 35.0f));
        sprite->setScaleX(width / sprite->getContentSize().width);
        sprite->setScaleY(65 / sprite->getContentSize().height);

        label = Label::createWithSystemFont("", "Arail", 10);
        cell->addChild(label);
        label->setPosition(Vec2(2.0f, 35.0f));
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);

        delBtn = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
        delBtn->setScale9Enabled(true);
        delBtn->setContentSize(Size(40.0f, 20.0f));
        delBtn->setTitleColor(Color3B::BLACK);
        delBtn->setTitleFontSize(12);
        delBtn->setTitleText("删除");
        delBtn->addClickEventListener(std::bind(&HistoryScene::onDeleteButton, this, std::placeholders::_1));
        cell->addChild(delBtn);
        delBtn->setPosition(Vec2(width - 22.0f, 35.0f));

        viewBtn = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
        viewBtn->setScale9Enabled(true);
        viewBtn->setContentSize(Size(40.0f, 20.0f));
        viewBtn->setTitleColor(Color3B::BLACK);
        viewBtn->setTitleFontSize(12);
        viewBtn->setTitleText("查看");
        viewBtn->addClickEventListener(std::bind(&HistoryScene::onViewButton, this, std::placeholders::_1));
        cell->addChild(viewBtn);
        viewBtn->setPosition(Vec2(width - 66.0f, 35.0f));
    }

    const CustomCell::ExtDataType &ext = cell->getExtData();
    Label *label = std::get<0>(ext);
    ui::Button *delBtn = std::get<1>(ext);
    ui::Button *viewBtn = std::get<2>(ext);

    delBtn->setTag(idx);
    viewBtn->setTag(idx);

    const Record &record = g_records[idx];
    int scores[4] = { 0 };
    for (int i = 0; i < 16; ++i) {
        int s[4];
        translateDetailToScoreTable(record.detail[i], s);
        scores[0] += s[0];
        scores[1] += s[1];
        scores[2] += s[2];
        scores[3] += s[3];
    }

    char str[255];
    size_t len = 0;
    len += strftime(str, sizeof(str), "%Y-%m-%d %H:%M", localtime(&record.start_time));
    if (record.end_time != 0) {
        strcpy(str + len, " -- ");
        len += 4;
        len += strftime(str + len, sizeof(str) - len, "%Y-%m-%d %H:%M", localtime(&record.end_time));
    }

    snprintf(str + len, sizeof(str) - len, "\n%s(%+d)\n%s(%+d)\n%s(%+d)\n%s(%+d)",
        record.name[0], scores[0], record.name[1], scores[1], record.name[2], scores[2], record.name[3], scores[3]);
    label->setString(str);

    return cell;
}

void HistoryScene::onDeleteButton(cocos2d::Ref *sender) {
    ui::Button *button = (ui::Button *)sender;
    size_t idx = button->getTag();
    AlertLayer::showWithMessage("删除记录", "删除后无法找回，确认删除？", [this, idx]() {
        g_records.erase(g_records.begin() + idx);
        auto thiz = RefPtr<HistoryScene>(this);
        saveRecordsAsync([thiz]() {
            if (LIKELY(thiz->getParent() != nullptr)) {
                thiz->_tableView->reloadData();
            }
        });
    }, nullptr);
}

void HistoryScene::onViewButton(cocos2d::Ref *sender) {
    ui::Button *button = (ui::Button *)sender;
    size_t idx = button->getTag();
    if (_viewCallback(g_records[idx])) {
        Director::getInstance()->popScene();
    }
}

static void addRecord(const Record &record) {
    if (g_records.end() == std::find(g_records.begin(), g_records.end(), record)) {
        g_records.push_back(record);
        saveRecordsAsync([]{});
    }
}

void HistoryScene::addRecord(const Record &record) {
    if (g_records.empty()) {
        loadRecordsAsync([record]{
            ::addRecord(record);
        });
    }
    else {
        ::addRecord(record);
    }
}

static void modifyRecord(const Record &record) {
    auto it = std::find_if(g_records.begin(), g_records.end(), [&record](const Record &r) {
        return (r.start_time == record.start_time
            && r.end_time == record.end_time);
    });

    if (it == g_records.end()) {
        g_records.push_back(record);
    }
    else {
        memcpy(&*it, &record, sizeof(Record));
    }
    saveRecordsAsync([]{});
}

void HistoryScene::modifyRecord(const Record &record) {
    if (g_records.empty()) {
        loadRecordsAsync([record]{
            ::modifyRecord(record);
        });
    }
    else {
        ::modifyRecord(record);
    }
}
