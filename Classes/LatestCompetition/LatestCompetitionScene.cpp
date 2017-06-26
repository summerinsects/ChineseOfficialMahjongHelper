#include "LatestCompetitionScene.h"
#include "../widget/LoadingView.h"
#include "../widget/AlertView.h"
#include "network/HttpClient.h"
#include "json/document.h"
#include "json/stringbuffer.h"

USING_NS_CC;

bool LatestCompetitionScene::init() {
    if (UNLIKELY(!BaseScene::initWithTitle("近期赛事"))) {
        return false;
    }

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    _tableView = cw::TableView::create();
    _tableView->setContentSize(Size(visibleSize.width - 5.0f, visibleSize.height - 35));
    _tableView->setDelegate(this);
    _tableView->setDirection(ui::ScrollView::Direction::VERTICAL);
    _tableView->setVerticalFillOrder(cw::TableView::VerticalFillOrder::TOP_DOWN);

    _tableView->setScrollBarPositionFromCorner(Vec2(2, 2));
    _tableView->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    _tableView->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f - 15.0f));
    this->addChild(_tableView);

    LoadingView *loadingView = LoadingView::create();
    this->addChild(loadingView);
    loadingView->setPosition(origin);

    network::HttpRequest *request = new (std::nothrow) network::HttpRequest();
    request->setRequestType(network::HttpRequest::Type::GET);
    request->setUrl("https://raw.githubusercontent.com/summerinsects/ChineseOfficialMahjongHelper/master/proj.android/build-cfg.json");

    auto thiz = makeRef(this);  // 保证线程回来之前不析构
    request->setResponseCallback([thiz, loadingView](network::HttpClient *client, network::HttpResponse *response) {
        if (LIKELY(thiz->getReferenceCount() > 2)) {
            loadingView->removeFromParent();
        }

        if (response == nullptr) {
            AlertView::showWithMessage("提示", "获取近期赛事失败", 12, nullptr, nullptr);
            return;
        }

        log("HTTP Status Code: %ld", response->getResponseCode());

        if (!response->isSucceed()) {
            log("response failed");
            log("error buffer: %s", response->getErrorBuffer());
            AlertView::showWithMessage("提示", "获取近期赛事失败", 12, nullptr, nullptr);
            return;
        }

        std::vector<char> *buffer = response->getResponseData();
        printf("%.*s", (int)buffer->size(), &buffer->front());

        thiz->_competitions.clear();
        CompetitionInfo info;
        info.name = "2017年逍遥宫线下组织广州分会6月联赛";
        info.startTime = time(nullptr);
        info.endTime = time(nullptr);
        info.url = "adsff";
        for (int i = 0; i < 10; ++i) thiz->_competitions.push_back(info);
        thiz->_tableView->reloadDataInplacement();
    });

    network::HttpClient::getInstance()->send(request);
    request->release();

    return true;
}

ssize_t LatestCompetitionScene::numberOfCellsInTableView(cw::TableView *table) {
    return _competitions.size();
}

cocos2d::Size LatestCompetitionScene::tableCellSizeForIndex(cw::TableView *table, ssize_t idx) {
    return Size(0, 50);
}

cw::TableViewCell *LatestCompetitionScene::tableCellAtIndex(cw::TableView *table, ssize_t idx) {
    typedef cw::TableViewCellEx<LayerColor *[2], Label *[2], ui::Button *> CustomCell;
    CustomCell *cell = (CustomCell *)table->dequeueCell();

    if (cell == nullptr) {
        cell = CustomCell::create();

        Size visibleSize = Director::getInstance()->getVisibleSize();
        const float width = visibleSize.width - 5.0f;

        CustomCell::ExtDataType &ext = cell->getExtData();
        LayerColor *(&layerColor)[2] = std::get<0>(ext);
        Label *(&label)[2] = std::get<1>(ext);
        ui::Button *&detailBtn = std::get<2>(ext);

        layerColor[0] = LayerColor::create(Color4B(0xC0, 0xC0, 0xC0, 0x10), width, 48);
        cell->addChild(layerColor[0]);
        layerColor[0]->setPosition(Vec2(0, 1));

        layerColor[1] = LayerColor::create(Color4B(0x80, 0x80, 0x80, 0x10), width, 48);
        cell->addChild(layerColor[1]);
        layerColor[1]->setPosition(Vec2(0, 1));

        label[0] = Label::createWithSystemFont("", "Arail", 10);
        label[0]->setColor(Color3B::BLACK);
        cell->addChild(label[0]);
        label[0]->setPosition(Vec2(5.0f, 35.0f));
        label[0]->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);

        label[1] = Label::createWithSystemFont("", "Arail", 10);
        label[1]->setColor(Color3B::BLACK);
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
    LayerColor *const (&layerColor)[2] = std::get<0>(ext);
    Label *const (&label)[2] = std::get<1>(ext);
    ui::Button *detailBtn = std::get<2>(ext);

    layerColor[0]->setVisible(!(idx & 1));
    layerColor[1]->setVisible(!!(idx & 1));

    detailBtn->setUserData(reinterpret_cast<void *>(idx));

    const CompetitionInfo &info = _competitions[idx];

    char buf[sizeof("年月日") + 8];
    strftime(buf, sizeof(buf), "%Y年%m月%d日", localtime(&info.startTime));

    std::string date = buf;
    if (info.endTime != 0) {
        date.append("-");
        strftime(buf, sizeof(buf), "%Y年%m月%d日", localtime(&info.endTime));
        date.append(buf);
    }
    label[0]->setString(date);
    label[1]->setString(info.name);

    detailBtn->setVisible(!info.url.empty());

    return cell;
}

void LatestCompetitionScene::onDetailButton(cocos2d::Ref *sender) {
    ui::Button *button = (ui::Button *)sender;
    size_t idx = reinterpret_cast<size_t>(button->getUserData());
    Application::getInstance()->openURL(_competitions[idx].url);
}
