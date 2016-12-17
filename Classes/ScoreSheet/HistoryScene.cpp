#include "HistoryScene.h"
#include "Record.h"
#include "../widget/AlertLayer.h"

#pragma execution_character_set("utf-8")

USING_NS_CC;

static std::vector<Record> g_records;

Scene *HistoryScene::createScene(const std::function<void (const Record &)> &viewCallback) {
    auto scene = Scene::create();
    auto layer = HistoryScene::create();
    layer->_viewCallback = viewCallback;
    scene->addChild(layer);
    return scene;
}

static void lazyLoadRecords() {
    if (!g_records.empty()) {
        return;
    }

    std::string fileName = FileUtils::getInstance()->getWritablePath();
    fileName.append("history_record.json");
    std::string str = FileUtils::getInstance()->getStringFromFile(fileName);

    try {
        jw::cppJSON json;
        json.Parse(str.c_str());

        g_records.clear();
        g_records.reserve(json.size());
        std::transform(json.begin(), json.end(), std::back_inserter(g_records), [](const jw::cppJSON &json) {
            Record record;
            fromJson(&record, json);
            return record;
        });
    }
    catch (std::exception &e) {
        CCLOG("%s %s", __FUNCTION__, e.what());
    }
}

static void saveRecords() {
    std::string fileName = FileUtils::getInstance()->getWritablePath();
    fileName.append("history_record.json");
    FILE *file = fopen(fileName.c_str(), "wb");
    if (file != nullptr) {
        try {
            jw::cppJSON json(jw::cppJSON::ValueType::Array);
            std::transform(g_records.begin(), g_records.end(), std::back_inserter(json), [](const Record &record) {
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

ui::Widget *HistoryScene::createRecordWidget(size_t idx, float width) {
    ui::Widget *widget = ui::Widget::create();
    widget->setContentSize(Size(width, 36.0f));

    ui::Button *button = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
    button->setScale9Enabled(true);
    button->setContentSize(Size(40.0f, 20.0f));
    button->setTitleColor(Color3B::BLACK);
    button->setTitleFontSize(12);
    button->setTitleText("删除");
    button->addClickEventListener([this, idx](Ref *) {
        AlertLayer::showWithMessage("删除记录", "删除后无法找回，确认删除？", [this, idx]() {
            g_records.erase(g_records.begin() + idx);
            saveRecords();
            std::function<void (const Record &)> viewCallback = std::move(_viewCallback);
            Director::getInstance()->popScene();
            Director::getInstance()->pushScene(HistoryScene::createScene(viewCallback));
        }, nullptr);
    });
    widget->addChild(button);
    button->setPosition(Vec2(width - 22.0f, 18.0f));

    button = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
    button->setScale9Enabled(true);
    button->setContentSize(Size(40.0f, 20.0f));
    button->setTitleColor(Color3B::BLACK);
    button->setTitleFontSize(12);
    button->setTitleText("查看");
    button->addClickEventListener([this, idx](Ref *) {
        _viewCallback(g_records[idx]);
        Director::getInstance()->popScene();
    });
    widget->addChild(button);
    button->setPosition(Vec2(width - 66.0f, 18.0f));

    const Record &record = g_records[idx];
    int scores[4] = { 0 };
    for (int i = 0; i < 16; ++i) {
        scores[0] += record.scores[i][0];
        scores[1] += record.scores[i][1];
        scores[2] += record.scores[i][2];
        scores[3] += record.scores[i][3];
    }

    char str[255];
    size_t len = 0;
    len += strftime(str, sizeof(str), "%Y-%m-%d %H:%M", localtime(&record.startTime));
    strcpy(str + len, " -- ");
    len += 4;
    len += strftime(str + len, sizeof(str) - len, "%Y-%m-%d %H:%M", localtime(&record.endTime));

    snprintf(str + len, sizeof(str) - len, "\n[%s(%+d) %s(%+d) %s(%+d) %s(%+d)]",
        record.name[0], scores[0], record.name[1], scores[1], record.name[2], scores[2], record.name[3], scores[3]);
    Label *label = Label::createWithSystemFont(str, "Arail", 10);
    widget->addChild(label);
    label->setPosition(Vec2((width - 88.0f) * 0.5f, 18.0f));

    return widget;
}

bool HistoryScene::init() {
    if (!BaseLayer::initWithTitle("历史记录")) {
        return false;
    }

    lazyLoadRecords();

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    ui::Widget *innerNode = ui::Widget::create();
    innerNode->setAnchorPoint(Vec2::ANCHOR_BOTTOM_LEFT);
    const float scrollHeight = visibleSize.height - 35;
    const float innerNodeHeight = std::max<float>(g_records.size() * 40, scrollHeight);
    innerNode->setContentSize(Size(visibleSize.width, innerNodeHeight));

    float y = innerNodeHeight - 20;
    for (std::vector<Record>::size_type i = 0; i < g_records.size(); ++i) {
        ui::Widget *widget = createRecordWidget(i, visibleSize.width);
        innerNode->addChild(widget);
        widget->setPosition(Vec2(visibleSize.width * 0.5f, y));
        y -= 40;
    }

    ui::ScrollView *scrollView = ui::ScrollView::create();
    scrollView->setDirection(ui::ScrollView::Direction::VERTICAL);
    scrollView->setContentSize(Size(visibleSize.width, scrollHeight));
    scrollView->setScrollBarPositionFromCorner(Vec2(10, 10));
    scrollView->setInnerContainerSize(innerNode->getContentSize());
    scrollView->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    scrollView->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f - 15.0f));
    this->addChild(scrollView);

    scrollView->addChild(innerNode);

    return true;
}

void HistoryScene::addRecord(const Record &record) {
    lazyLoadRecords();
    if (g_records.end() == std::find(g_records.begin(), g_records.end(), record)) {
        g_records.push_back(record);
        saveRecords();
    }
}
