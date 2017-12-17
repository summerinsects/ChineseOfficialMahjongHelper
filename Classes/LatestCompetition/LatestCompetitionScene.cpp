#include "LatestCompetitionScene.h"
#include <algorithm>
#include <iterator>
#include <array>
#include "network/HttpClient.h"
#include "json/document.h"
#include "json/stringbuffer.h"
#include "../widget/LoadingView.h"
#include "../widget/AlertView.h"

USING_NS_CC;

bool LatestCompetitionScene::init() {
    if (UNLIKELY(!BaseScene::initWithTitle("近期赛事"))) {
        return false;
    }

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    Label *label = Label::createWithSystemFont("宣传赛事信息，请联系逍遥宫", "Arial", 12);
    this->addChild(label);
    label->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height - 45.0f));
    label->setColor(Color3B::ORANGE);

    cw::TableView *tableView = cw::TableView::create();
    tableView->setDirection(ui::ScrollView::Direction::VERTICAL);
    tableView->setScrollBarPositionFromCorner(Vec2(2.0f, 2.0f));
    tableView->setScrollBarWidth(4.0f);
    tableView->setScrollBarOpacity(0x99);
    tableView->setBounceEnabled(true);
    tableView->setContentSize(Size(visibleSize.width - 5.0f, visibleSize.height - 65.0f));
    tableView->setDelegate(this);
    tableView->setVerticalFillOrder(cw::TableView::VerticalFillOrder::TOP_DOWN);

    tableView->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    tableView->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f - 30.0f));
    this->addChild(tableView);
    _tableView = tableView;

    label = Label::createWithSystemFont("无近期赛事信息", "Arial", 12);
    this->addChild(label);
    label->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height - 75.0f));
    label->setColor(Color3B::BLACK);
    label->setVisible(false);
    _emptyLabel = label;

    this->scheduleOnce([this](float) { requestCompetitions(); }, 0.0f, "request_competitions");

    return true;
}

void LatestCompetitionScene::requestCompetitions() {
    LoadingView *loadingView = LoadingView::create();
    this->addChild(loadingView);
    loadingView->setPosition(Director::getInstance()->getVisibleOrigin());

    network::HttpRequest *request = new (std::nothrow) network::HttpRequest();
    request->setRequestType(network::HttpRequest::Type::GET);
    request->setUrl("https://raw.githubusercontent.com/summerinsects/ChineseOfficialMahjongLatestCompetition/master/LatestCompetition.json");

    auto thiz = makeRef(this);  // 保证线程回来之前不析构
    request->setResponseCallback([thiz, loadingView](network::HttpClient *client, network::HttpResponse *response) {
        CC_UNUSED_PARAM(client);

        network::HttpClient::destroyInstance();

        if (UNLIKELY(!thiz->isRunning())) {
            return;
        }

        loadingView->removeFromParent();

        LatestCompetitionScene *pthis = thiz.get();
        if (response == nullptr) {
            AlertView::showWithMessage("提示", "获取近期赛事失败", 12, [pthis]() {
                pthis->requestCompetitions();
            }, nullptr);
            return;
        }

        log("HTTP Status Code: %ld", response->getResponseCode());

        if (!response->isSucceed()) {
            log("response failed");
            log("error buffer: %s", response->getErrorBuffer());
            AlertView::showWithMessage("提示", "获取近期赛事失败", 12, [pthis]() {
                pthis->requestCompetitions();
            }, nullptr);
            return;
        }

        std::vector<char> *buffer = response->getResponseData();
        thiz->parseResponse(buffer);
    });

    network::HttpClient::getInstance()->sendImmediate(request);
    request->release();
}

bool LatestCompetitionScene::parseResponse(const std::vector<char> *buffer) {
    if (buffer == nullptr) {
        return false;
    }

    try {
        do {
            std::string str(buffer->begin(), buffer->end());
            rapidjson::Document doc;
            doc.Parse<0>(str.c_str());
            if (doc.HasParseError() || !doc.IsArray()) {
                break;
            }

            _competitions.clear();
            _competitions.reserve(doc.Size());
            std::transform(doc.Begin(), doc.End(), std::back_inserter(_competitions), [](const rapidjson::Value &json) {
                CompetitionInfo info = { 0 };

                rapidjson::Value::ConstMemberIterator it = json.FindMember("name");
                if (it != json.MemberEnd() && it->value.IsString()) {
                    strncpy(info.name, it->value.GetString(), sizeof(info.name) - 1);
                }

                it = json.FindMember("start_time");
                if (it != json.MemberEnd() && it->value.IsInt64()) {
                    info.startTime = it->value.GetInt64();
                }

                it = json.FindMember("end_time");
                if (it != json.MemberEnd() && it->value.IsInt64()) {
                    info.endTime = it->value.GetInt64();
                }

                it = json.FindMember("url");
                if (it != json.MemberEnd() && it->value.IsString()) {
                    strncpy(info.url, it->value.GetString(), sizeof(info.url) - 1);
                }

                it = json.FindMember("time_accuracy");
                if (it != json.MemberEnd() && it->value.IsInt()) {
                    info.timeAccuracy = static_cast<TIME_ACCURACY>(it->value.GetInt());
                }
                return info;
            });

            time_t now = time(nullptr);
            std::vector<CompetitionInfo>::iterator it = std::remove_if(_competitions.begin(), _competitions.end(),
                [now](const CompetitionInfo &info) {
                return (info.timeAccuracy != TIME_ACCURACY::UNDETERMINED && info.startTime < now);
            });
            _competitions.erase(it, _competitions.end());

            _tableView->reloadDataInplacement();
            _emptyLabel->setVisible(_competitions.empty());

            return true;
        } while (0);
    }
    catch (std::exception &e) {
        CCLOG("%s %s", __FUNCTION__, e.what());
    }

    return false;
}

ssize_t LatestCompetitionScene::numberOfCellsInTableView(cw::TableView *) {
    return _competitions.size();
}

float LatestCompetitionScene::tableCellSizeForIndex(cw::TableView *, ssize_t) {
    return 50.0f;
}

cw::TableViewCell *LatestCompetitionScene::tableCellAtIndex(cw::TableView *table, ssize_t idx) {
    typedef cw::TableViewCellEx<std::array<LayerColor *, 2>, std::array<Label *, 2>, ui::Button *> CustomCell;
    CustomCell *cell = (CustomCell *)table->dequeueCell();

    if (cell == nullptr) {
        cell = CustomCell::create();

        Size visibleSize = Director::getInstance()->getVisibleSize();
        const float width = visibleSize.width - 5.0f;

        CustomCell::ExtDataType &ext = cell->getExtData();
        std::array<LayerColor *, 2> &layerColors = std::get<0>(ext);
        std::array<Label *, 2> &label = std::get<1>(ext);
        ui::Button *&detailBtn = std::get<2>(ext);

        layerColors[0] = LayerColor::create(Color4B(0x10, 0x10, 0x10, 0x10), width, 50.0f);
        cell->addChild(layerColors[0]);

        layerColors[1] = LayerColor::create(Color4B(0xC0, 0xC0, 0xC0, 0x10), width, 50.0f);
        cell->addChild(layerColors[1]);

        label[0] = Label::createWithSystemFont("", "Arail", 10);
        label[0]->setColor(Color3B::BLACK);
        cell->addChild(label[0]);
        label[0]->setPosition(Vec2(5.0f, 35.0f));
        label[0]->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);

        label[1] = Label::createWithSystemFont("", "Arail", 10);
        label[1]->setColor(Color3B(0x60, 0x60, 0x60));
        cell->addChild(label[1]);
        label[1]->setPosition(Vec2(5.0f, 15.0f));
        label[1]->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);

        detailBtn = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
        detailBtn->setScale9Enabled(true);
        detailBtn->setContentSize(Size(40.0f, 20.0f));
        detailBtn->setTitleFontSize(12);
        detailBtn->setTitleText("详情");
        detailBtn->addClickEventListener(std::bind(&LatestCompetitionScene::onDetailButton, this, std::placeholders::_1));
        cell->addChild(detailBtn);
        detailBtn->setPosition(Vec2(width - 25.0f, 25.0f));
    }

    const CustomCell::ExtDataType &ext = cell->getExtData();
    const std::array<LayerColor *, 2> &layerColors = std::get<0>(ext);
    const std::array<Label *, 2> &label = std::get<1>(ext);
    ui::Button *detailBtn = std::get<2>(ext);

    layerColors[0]->setVisible(!(idx & 1));
    layerColors[1]->setVisible(!!(idx & 1));

    detailBtn->setUserData(reinterpret_cast<void *>(idx));

    const CompetitionInfo &info = _competitions[idx];
    label[0]->setString(info.name);

    struct tm ret = *localtime(&info.startTime);
    std::string date;

    switch (info.timeAccuracy) {
    default:
        date = "具体时间待定";
        break;
    case TIME_ACCURACY::MONTHS:
        date = Common::format("%d年%d月", ret.tm_year + 1900, ret.tm_mon + 1);
        if (info.endTime != 0) {
            date.append("——");
            ret = *localtime(&info.endTime);
            date.append(Common::format("%d年%d月", ret.tm_year + 1900, ret.tm_mon + 1));
        }
        break;
    case TIME_ACCURACY::DAYS:
        date = Common::format("%d年%d月%d日", ret.tm_year + 1900, ret.tm_mon + 1, ret.tm_mday);
        if (info.endTime != 0) {
            date.append("——");
            ret = *localtime(&info.endTime);
            date.append(Common::format("%d年%d月%d日", ret.tm_year + 1900, ret.tm_mon + 1, ret.tm_mday));
        }
        break;
    case TIME_ACCURACY::HONRS:
        date = Common::format("%d年%d月%d日%.2d点", ret.tm_year + 1900, ret.tm_mon + 1, ret.tm_mday, ret.tm_hour);
        if (info.endTime != 0) {
            date.append("——");
            ret = *localtime(&info.endTime);
            date.append(Common::format("%d年%d月%d日%.2d点", ret.tm_year + 1900, ret.tm_mon + 1, ret.tm_mday, ret.tm_hour));
        }
        break;
    case TIME_ACCURACY::MINUTES:
        date = Common::format("%d年%d月%d日%.2d:%.2d", ret.tm_year + 1900, ret.tm_mon + 1, ret.tm_mday, ret.tm_hour, ret.tm_min);
        if (info.endTime != 0) {
            date.append("——");
            ret = *localtime(&info.endTime);
            date.append(Common::format("%d年%d月%d日%.2d:%.2d", ret.tm_year + 1900, ret.tm_mon + 1, ret.tm_mday, ret.tm_hour, ret.tm_min));
        }
        break;
    }
    label[1]->setString(date);

    detailBtn->setVisible(info.url[0] != '\0');

    return cell;
}

void LatestCompetitionScene::onDetailButton(cocos2d::Ref *sender) {
    ui::Button *button = (ui::Button *)sender;
    size_t idx = reinterpret_cast<size_t>(button->getUserData());
    Application::getInstance()->openURL(_competitions[idx].url);
}
