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

static std::vector<CompetitionData> g_datas;
static std::mutex g_mutex;

static void loadDatas(std::vector<CompetitionData> &datas) {
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

        datas.clear();
        datas.reserve(doc.Size());
        std::transform(doc.Begin(), doc.End(), std::back_inserter(datas), [](const rapidjson::Value &json) {
            CompetitionData data;
            CompetitionData::fromJson(json, data);
            return data;
        });

        std::sort(datas.begin(), datas.end(), [](const CompetitionData &r1, const CompetitionData &r2) { return r1.start_time > r2.start_time; });
    }
    catch (std::exception &e) {
        CCLOG("%s %s", __FUNCTION__, e.what());
    }
}

static void savedatas(const std::vector<CompetitionData> &datas) {
    std::lock_guard<std::mutex> lg(g_mutex);

    std::string fileName = FileUtils::getInstance()->getWritablePath();
    fileName.append("history_competition.json");
    FILE *file = fopen(fileName.c_str(), "wb");
    if (LIKELY(file != nullptr)) {
        try {
            rapidjson::Document doc(rapidjson::Type::kArrayType);
            doc.Reserve(static_cast<rapidjson::SizeType>(datas.size()), doc.GetAllocator());
            std::for_each(datas.begin(), datas.end(), [&doc](const CompetitionData &data) {
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

void CompetitionHistoryScene::updateDataTexts() {
    _dataTexts.clear();
    _dataTexts.reserve(g_datas.size());

    std::transform(g_datas.begin(), g_datas.end(), std::back_inserter(_dataTexts), [](const CompetitionData &data) {
        struct tm ret = *localtime(&data.start_time);
        std::string str = Common::format("%d年%d月%d日%.2d:%.2d", ret.tm_year + 1900, ret.tm_mon + 1, ret.tm_mday, ret.tm_hour, ret.tm_min);
        if (data.finish_time != 0) {
            ret = *localtime(&data.finish_time);
            str.append(Common::format("——%d年%d月%d日%.2d:%.2d", ret.tm_year + 1900, ret.tm_mon + 1, ret.tm_mday, ret.tm_hour, ret.tm_min));
        }
        else {
            str.append("——(未结束)");
        }
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
    tableView->setScrollBarPositionFromCorner(Vec2(2, 2));
    tableView->setScrollBarWidth(4);
    tableView->setScrollBarOpacity(0x99);
    tableView->setContentSize(Size(visibleSize.width - 5.0f, visibleSize.height - 35));
    tableView->setDelegate(this);
    tableView->setVerticalFillOrder(cw::TableView::VerticalFillOrder::TOP_DOWN);

    tableView->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    tableView->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f - 15.0f));
    this->addChild(tableView);
    _tableView = tableView;

    tableView->setOnEnterCallback([this]() {
        if (LIKELY(!g_datas.empty())) {
            updateDataTexts();
            _tableView->reloadDataInplacement();
        }
    });

    if (UNLIKELY(g_datas.empty())) {
        this->scheduleOnce([this](float) {
            Vec2 origin = Director::getInstance()->getVisibleOrigin();

            LoadingView *loadingView = LoadingView::create();
            this->addChild(loadingView);
            loadingView->setPosition(origin);

            auto thiz = makeRef(this);  // 保证线程回来之前不析构
            std::thread([thiz, loadingView]() {
                std::vector<CompetitionData> temp;
                loadDatas(temp);

                // 切换到cocos线程
                Director::getInstance()->getScheduler()->performFunctionInCocosThread([thiz, loadingView, temp]() mutable {
                    g_datas.swap(temp);

                    if (LIKELY(thiz->isRunning())) {
                        thiz->updateDataTexts();
                        loadingView->removeFromParent();
                        thiz->_tableView->reloadData();
                    }
                });
            }).detach();
        }, 0.0f, "load_datas");
    }

    return true;
}

ssize_t CompetitionHistoryScene::numberOfCellsInTableView(cw::TableView *table) {
    return _dataTexts.size();
}

float CompetitionHistoryScene::tableCellSizeForIndex(cw::TableView *table, ssize_t idx) {
    return 70.0f;
}

cw::TableViewCell *CompetitionHistoryScene::tableCellAtIndex(cw::TableView *table, ssize_t idx) {
    typedef cw::TableViewCellEx<std::array<LayerColor *, 2>, Label *, ui::Button *> CustomCell;
    CustomCell *cell = (CustomCell *)table->dequeueCell();

    if (cell == nullptr) {
        cell = CustomCell::create();

        Size visibleSize = Director::getInstance()->getVisibleSize();
        const float width = visibleSize.width - 5.0f;

        CustomCell::ExtDataType &ext = cell->getExtData();
        std::array<LayerColor *, 2> &layerColor = std::get<0>(ext);
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
        delBtn->addClickEventListener(std::bind(&CompetitionHistoryScene::onDeleteButton, this, std::placeholders::_1));
        cell->addChild(delBtn);
        delBtn->setPosition(Vec2(width - 20.0f, 35.0f));

        cell->setContentSize(Size(width, 70));
        cell->setTouchEnabled(true);
        cell->addClickEventListener(std::bind(&CompetitionHistoryScene::onCellClicked, this, std::placeholders::_1));
    }

    const CustomCell::ExtDataType &ext = cell->getExtData();
    const std::array<LayerColor *, 2> &layerColor = std::get<0>(ext);
    Label *label = std::get<1>(ext);
    ui::Button *delBtn = std::get<2>(ext);

    layerColor[0]->setVisible(!(idx & 1));
    layerColor[1]->setVisible(!!(idx & 1));

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

        g_datas.erase(g_datas.begin() + idx);

        auto thiz = makeRef(this);  // 保证线程回来之前不析构
        std::vector<CompetitionData> temp = g_datas;
        std::thread([thiz, temp, loadingView](){
            savedatas(temp);

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
    _viewCallback(&g_datas[cell->getIdx()]);
}

static void __modifyData(const CompetitionData *data) {
    auto it = std::find_if(g_datas.begin(), g_datas.end(), [data](const CompetitionData &r) {
        return (r.start_time == data->start_time);  // 我们认为开始时间相同的为同一个记录
    });

    if (it == g_datas.end()) {
        g_datas.push_back(*data);
    }

    std::sort(g_datas.begin(), g_datas.end(), [](const CompetitionData &r1, const CompetitionData &r2) { return r1.start_time > r2.start_time; });

    std::vector<CompetitionData> temp = g_datas;
    std::thread(&savedatas, temp).detach();
}

void CompetitionHistoryScene::modifyData(const CompetitionData *data) {
    if (UNLIKELY(g_datas.empty())) {
        CompetitionData dataCopy = *data;
        std::thread([dataCopy]() {
            std::vector<CompetitionData> temp;
            loadDatas(temp);

            Director::getInstance()->getScheduler()->performFunctionInCocosThread([dataCopy, temp]() mutable {
                g_datas.swap(temp);
                __modifyData(&dataCopy);
            });
        }).detach();
    }
    else {
        __modifyData(data);
    }
}
