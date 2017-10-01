#include "CompetitionHistoryScene.h"
#include <thread>
#include <mutex>
#include <array>
#include "json/stringbuffer.h"
#ifdef COCOS2D_DEBUG
#include "json/prettywriter.h"
#else
#include "json/writer.h"
#endif
#include "Competition.h"
#include "../widget/AlertView.h"
#include "../widget/LoadingView.h"

USING_NS_CC;

static std::vector<CompetitionData> g_competitions;
static std::mutex g_mutex;

static void loadCompetitions(std::vector<CompetitionData> &competitions) {
    std::lock_guard<std::mutex> lg(g_mutex);

    std::string fileName = FileUtils::getInstance()->getWritablePath();
    fileName.append("history_competition.json");
    std::string str = FileUtils::getInstance()->getStringFromFile(fileName);

    try {
        rapidjson::Document doc;
        doc.Parse<0>(str.c_str());
        if (doc.HasParseError() || !doc.IsArray()) {
            return;
        }

        competitions.clear();
        competitions.reserve(doc.Size());
        std::transform(doc.Begin(), doc.End(), std::back_inserter(competitions), [](const rapidjson::Value &json) {
            CompetitionData data;
            CompetitionData::fromJson(json, data);
            return data;
        });

        std::sort(competitions.begin(), competitions.end(), [](const CompetitionData &c1, const CompetitionData &c2) { return c1.start_time > c2.start_time; });
    }
    catch (std::exception &e) {
        CCLOG("%s %s", __FUNCTION__, e.what());
    }
}

static void saveCompetitions(const std::vector<CompetitionData> &competitions) {
    std::lock_guard<std::mutex> lg(g_mutex);

    std::string fileName = FileUtils::getInstance()->getWritablePath();
    fileName.append("history_competition.json");
    FILE *file = fopen(fileName.c_str(), "wb");
    if (LIKELY(file != nullptr)) {
        try {
            rapidjson::Document doc(rapidjson::Type::kArrayType);
            doc.Reserve(static_cast<rapidjson::SizeType>(competitions.size()), doc.GetAllocator());
            std::for_each(competitions.begin(), competitions.end(), [&doc](const CompetitionData &data) {
                rapidjson::Value json(rapidjson::Type::kObjectType);
                CompetitionData::toJson(data, json, doc.GetAllocator());
                doc.PushBack(std::move(json), doc.GetAllocator());
            });

            rapidjson::StringBuffer buf;
#ifdef COCOS2D_DEBUG
            rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buf);
#else
            rapidjson::Writer<rapidjson::StringBuffer> writer(buf);
#endif
            doc.Accept(writer);

            fwrite(buf.GetString(), 1, buf.GetSize(), file);
        }
        catch (std::exception &e) {
            CCLOG("%s %s", __FUNCTION__, e.what());
        }
        fclose(file);
    }
}

#define BUF_SIZE 511

void CompetitionHistoryScene::updateDataTexts() {
    _dataTexts.clear();
    _dataTexts.reserve(g_competitions.size());

    std::transform(g_competitions.begin(), g_competitions.end(), std::back_inserter(_dataTexts), [](const CompetitionData &data)->std::string {
        char str[BUF_SIZE + 1];
        str[BUF_SIZE] = '\0';

        struct tm ret = *localtime(&data.start_time);
        int len = snprintf(str, BUF_SIZE, "%d年%d月%d日%.2d:%.2d", ret.tm_year + 1900, ret.tm_mon + 1, ret.tm_mday, ret.tm_hour, ret.tm_min);
        if (data.finish_time != 0) {
            ret = *localtime(&data.finish_time);
            len += snprintf(str + len, BUF_SIZE - len, "——%d年%d月%d日%.2d:%.2d", ret.tm_year + 1900, ret.tm_mon + 1, ret.tm_mday, ret.tm_hour, ret.tm_min);
        }
        else {
            len += snprintf(str + len, BUF_SIZE - len, "%s", "——(未结束)");
        }

        len += snprintf(str + len, BUF_SIZE - len, "\n%s", data.name.c_str());
        len += snprintf(str + len, BUF_SIZE - len, "\n%" PRIzd "人，%" PRIzd "轮", data.players.size(), data.round_count);

        return str;
    });
}

bool CompetitionHistoryScene::initWithCallback(const ViewCallback &viewCallback) {
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
        if (LIKELY(!g_competitions.empty())) {
            updateDataTexts();
            _tableView->reloadDataInplacement();
        }
    });

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
                Director::getInstance()->getScheduler()->performFunctionInCocosThread([thiz, loadingView, temp]() mutable {
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

    return true;
}

ssize_t CompetitionHistoryScene::numberOfCellsInTableView(cw::TableView *table) {
    return _dataTexts.size();
}

float CompetitionHistoryScene::tableCellSizeForIndex(cw::TableView *table, ssize_t idx) {
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
        std::array<LayerColor *, 2> &layerColors = std::get<0>(ext);
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
    const std::array<LayerColor *, 2> &layerColors = std::get<0>(ext);
    Label *label = std::get<1>(ext);
    ui::Button *delBtn = std::get<2>(ext);

    layerColors[0]->setVisible(!(idx & 1));
    layerColors[1]->setVisible(!!(idx & 1));

    delBtn->setUserData(reinterpret_cast<void *>(idx));

    label->setString(_dataTexts[idx]);

    return cell;
}

void CompetitionHistoryScene::onDeleteButton(cocos2d::Ref *sender) {
    ui::Button *button = (ui::Button *)sender;
    size_t idx = reinterpret_cast<size_t>(button->getUserData());
    AlertView::showWithMessage("删除记录", "删除后无法找回，确认删除？", 12, [this, idx]() {
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
    }, nullptr);
}

void CompetitionHistoryScene::onCellClicked(cocos2d::Ref *sender) {
    cw::TableViewCell *cell = (cw::TableViewCell *)sender;
    _viewCallback(&g_competitions[cell->getIdx()]);
}

static void __modifyData(const CompetitionData *data) {
    auto it = std::find_if(g_competitions.begin(), g_competitions.end(), [data](const CompetitionData &c) {
        return (c.start_time == data->start_time);  // 我们认为开始时间相同的为同一个记录
    });

    if (it == g_competitions.end()) {
        g_competitions.push_back(*data);
    }

    std::sort(g_competitions.begin(), g_competitions.end(), [](const CompetitionData &c1, const CompetitionData &c2) { return c1.start_time > c2.start_time; });

    auto temp = std::make_shared<std::vector<CompetitionData> >(g_competitions);
    std::thread([temp]() {
        saveCompetitions(*temp);
    }).detach();
}

void CompetitionHistoryScene::modifyData(const CompetitionData *data) {
    if (UNLIKELY(g_competitions.empty())) {
        CompetitionData dataCopy = *data;
        std::thread([dataCopy]() {
            auto temp = std::make_shared<std::vector<CompetitionData> >();
            loadCompetitions(*temp);

            Director::getInstance()->getScheduler()->performFunctionInCocosThread([dataCopy, temp]() mutable {
                g_competitions.swap(*temp);
                __modifyData(&dataCopy);
            });
        }).detach();
    }
    else {
        __modifyData(data);
    }
}
