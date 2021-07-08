#include "LatestCompetitionScene.h"
#include <algorithm>
#include <iterator>
#include <array>
#include "network/HttpClient.h"
#include "json/document.h"
#include "json/stringbuffer.h"
#include "../UICommon.h"
#include "../UIColors.h"
#include "../widget/LoadingView.h"
#include "../widget/AlertDialog.h"
#include "../widget/CommonWebViewScene.h"

USING_NS_CC;

bool LatestCompetitionScene::init() {
    if (UNLIKELY(!BaseScene::initWithTitle(__UTF8("近期赛事")))) {
        return false;
    }

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

    Label *label = Label::createWithSystemFont(__UTF8("无近期赛事信息"), "Arial", 12);
    this->addChild(label);
    label->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height - 75.0f));
    label->setTextColor(C4B_BLACK);
    label->setVisible(false);
    _emptyLabel = label;

    this->scheduleOnce([this](float) { requestCompetitions(); }, 0.0f, "request_competitions");

    return true;
}

void LatestCompetitionScene::requestCompetitions() {
    LoadingView *loadingView = LoadingView::create();
    loadingView->showInScene(this);

    network::HttpRequest *request = new (std::nothrow) network::HttpRequest();
    request->setRequestType(network::HttpRequest::Type::GET);
    request->setUrl("https://gitee.com/summerinsects/ChineseOfficialMahjongHelperDataSource/raw/master/competition/latest.json");

    auto thiz = makeRef(this);  // 保证线程回来之前不析构
    request->setResponseCallback([thiz, loadingView](network::HttpClient *client, network::HttpResponse *response) {
        CC_UNUSED_PARAM(client);

        network::HttpClient::destroyInstance();

        if (UNLIKELY(!thiz->isRunning())) {
            return;
        }

        loadingView->dismiss();

        LatestCompetitionScene *pthis = thiz.get();
        if (response == nullptr) {
            AlertDialog::Builder(pthis)
                .setTitle(__UTF8("提示"))
                .setMessage(__UTF8("获取近期赛事失败"))
                .setNegativeButton(__UTF8("取消"), nullptr)
                .setPositiveButton(__UTF8("重试"), [pthis](AlertDialog *, int) {
                pthis->requestCompetitions();
                return true;
            }).create()->show();
            return;
        }

        log("HTTP Status Code: %ld", response->getResponseCode());

        if (!response->isSucceed()) {
            log("response failed");
            log("error buffer: %s", response->getErrorBuffer());
            AlertDialog::Builder(pthis)
                .setTitle(__UTF8("提示"))
                .setMessage(__UTF8("获取近期赛事失败"))
                .setNegativeButton(__UTF8("取消"), nullptr)
                .setPositiveButton(__UTF8("重试"), [pthis](AlertDialog *, int) {
                pthis->requestCompetitions();
                return true;
            }).create()->show();
            return;
        }

        std::vector<char> *buffer = response->getResponseData();
        if (buffer != nullptr) {
            thiz->parseResponse(buffer->data(), buffer->size());
        }
    });

    network::HttpClient::getInstance()->send(request);
    request->release();
}

bool LatestCompetitionScene::parseResponse(const char *buffer, size_t size) {
    try {
        rapidjson::Document doc;
        doc.Parse<0>(buffer, size);
        if (doc.HasParseError() || !doc.IsArray()) {
            return false;
        }

        _competitions.clear();
        _competitions.reserve(doc.Size());

        int64_t now = std::chrono::system_clock::now().time_since_epoch().count();
        for (auto jt = doc.Begin(); jt != doc.End(); ++jt) {
            CompetitionInfo info{};

            auto &json = *jt;
            rapidjson::Value::ConstMemberIterator it = json.FindMember("start_time");
            if (it != json.MemberEnd() && it->value.IsInt64()) {
                info.startTime = static_cast<time_t>(it->value.GetInt64());
            }

            it = json.FindMember("end_time");
            if (it != json.MemberEnd() && it->value.IsInt64()) {
                info.endTime = static_cast<time_t>(it->value.GetInt64());
            }

            it = json.FindMember("time_accuracy");
            if (it != json.MemberEnd() && it->value.IsInt()) {
                info.timeAccuracy = static_cast<TIME_ACCURACY>(it->value.GetInt());
            }

            if (info.timeAccuracy != TIME_ACCURACY::UNDETERMINED && info.startTime < now) {
                continue;
            }

            it = json.FindMember("name");
            if (it != json.MemberEnd() && it->value.IsString()) {
                info.name.assign(it->value.GetString(), it->value.GetStringLength());
            }

            it = json.FindMember("url");
            if (it != json.MemberEnd() && it->value.IsString()) {
                info.url.assign(it->value.GetString(), it->value.GetStringLength());
            }

            _competitions.emplace_back(std::move(info));
        }

        _tableView->reloadDataInplacement();
        _emptyLabel->setVisible(_competitions.empty());

        return true;
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

        const float cellWidth = table->getContentSize().width;

        CustomCell::ExtDataType &ext = cell->getExtData();
        LayerColor **layerColors = std::get<0>(ext).data();
        Label **label = std::get<1>(ext).data();
        ui::Button *&detailBtn = std::get<2>(ext);

        layerColors[0] = LayerColor::create(Color4B(0x10, 0x10, 0x10, 0x10), cellWidth, 50.0f);
        cell->addChild(layerColors[0]);

        layerColors[1] = LayerColor::create(Color4B(0xC0, 0xC0, 0xC0, 0x10), cellWidth, 50.0f);
        cell->addChild(layerColors[1]);

        label[0] = Label::createWithSystemFont("", "Arail", 10);
        label[0]->setTextColor(C4B_BLACK);
        cell->addChild(label[0]);
        label[0]->setPosition(Vec2(5.0f, 35.0f));
        label[0]->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);

        label[1] = Label::createWithSystemFont("", "Arail", 10);
        label[1]->setTextColor(C4B_GRAY);
        cell->addChild(label[1]);
        label[1]->setPosition(Vec2(5.0f, 15.0f));
        label[1]->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);

        detailBtn = UICommon::createButton();
        detailBtn->setScale9Enabled(true);
        detailBtn->setContentSize(Size(40.0f, 20.0f));
        detailBtn->setTitleFontSize(12);
        detailBtn->setTitleText(__UTF8("详情"));
        detailBtn->addClickEventListener(std::bind(&LatestCompetitionScene::onDetailButton, this, std::placeholders::_1));
        cell->addChild(detailBtn);
        detailBtn->setPosition(Vec2(cellWidth - 25.0f, 25.0f));
    }

    const CustomCell::ExtDataType &ext = cell->getExtData();
    LayerColor *const *layerColors = std::get<0>(ext).data();
    Label *const *label = std::get<1>(ext).data();
    ui::Button *detailBtn = std::get<2>(ext);

    layerColors[0]->setVisible((idx & 1) == 0);
    layerColors[1]->setVisible((idx & 1) != 0);

    detailBtn->setUserData(reinterpret_cast<void *>(idx));

    const CompetitionInfo &info = _competitions[idx];
    label[0]->setString(info.name);

    struct tm ret = *localtime(&info.startTime);
    std::string date;

    switch (info.timeAccuracy) {
    default:
        date = __UTF8("具体时间待定");
        break;
    case TIME_ACCURACY::MONTHS:
        date = Common::format(__UTF8("%d年%d月"), ret.tm_year + 1900, ret.tm_mon + 1);
        if (info.endTime != 0) {
            date.append(__UTF8("——"));
            ret = *localtime(&info.endTime);
            date.append(Common::format(__UTF8("%d年%d月"), ret.tm_year + 1900, ret.tm_mon + 1));
        }
        break;
    case TIME_ACCURACY::DAYS:
        date = Common::format(__UTF8("%d年%d月%d日"), ret.tm_year + 1900, ret.tm_mon + 1, ret.tm_mday);
        if (info.endTime != 0) {
            date.append(__UTF8("——"));
            ret = *localtime(&info.endTime);
            date.append(Common::format(__UTF8("%d年%d月%d日"), ret.tm_year + 1900, ret.tm_mon + 1, ret.tm_mday));
        }
        break;
    case TIME_ACCURACY::HOURS:
        date = Common::format(__UTF8("%d年%d月%d日%.2d点"), ret.tm_year + 1900, ret.tm_mon + 1, ret.tm_mday, ret.tm_hour);
        if (info.endTime != 0) {
            date.append(__UTF8("——"));
            ret = *localtime(&info.endTime);
            date.append(Common::format(__UTF8("%d年%d月%d日%.2d点"), ret.tm_year + 1900, ret.tm_mon + 1, ret.tm_mday, ret.tm_hour));
        }
        break;
    case TIME_ACCURACY::MINUTES:
        date = Common::format(__UTF8("%d年%d月%d日%.2d:%.2d"), ret.tm_year + 1900, ret.tm_mon + 1, ret.tm_mday, ret.tm_hour, ret.tm_min);
        if (info.endTime != 0) {
            date.append(__UTF8("——"));
            ret = *localtime(&info.endTime);
            date.append(Common::format(__UTF8("%d年%d月%d日%.2d:%.2d"), ret.tm_year + 1900, ret.tm_mon + 1, ret.tm_mday, ret.tm_hour, ret.tm_min));
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

    const CompetitionInfo &competition = _competitions[idx];

    CommonWebViewScene *scene = CommonWebViewScene::create(competition.name, competition.url, CommonWebViewScene::ContentType::URL);
    Director::getInstance()->pushScene(scene);
}
