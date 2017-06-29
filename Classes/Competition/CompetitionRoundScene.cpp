#include "CompetitionRoundScene.h"
#include "Competition.h"
#include "../common.h"

USING_NS_CC;

CompetitionRoundScene *CompetitionRoundScene::create(const std::shared_ptr<CompetitionData> &competitionData) {
    CompetitionRoundScene *ret = new (std::nothrow) CompetitionRoundScene();
    if (ret != nullptr && ret->initWithData(competitionData)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool CompetitionRoundScene::initWithData(const std::shared_ptr<CompetitionData> &competitionData) {
    if (UNLIKELY(!BaseScene::initWithTitle("第N轮"))) {
        return false;
    }

    _competitionData = competitionData;

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    _colWidth[0] = visibleSize.width * 0.1f;
    _colWidth[1] = visibleSize.width * 0.2f;
    _colWidth[2] = visibleSize.width * 0.1f;
    _colWidth[3] = visibleSize.width * 0.1f;
    _colWidth[4] = visibleSize.width * 0.125f;
    _colWidth[5] = visibleSize.width * 0.125f;
    _colWidth[6] = visibleSize.width * 0.125f;
    _colWidth[7] = visibleSize.width * 0.125f;

    _posX[0] = _colWidth[0] * 0.5f;
    _posX[1] = _posX[0] + _colWidth[0] * 0.5f + _colWidth[1] * 0.5f;
    _posX[2] = _posX[1] + _colWidth[1] * 0.5f + _colWidth[2] * 0.5f;
    _posX[3] = _posX[2] + _colWidth[2] * 0.5f + _colWidth[3] * 0.5f;
    _posX[4] = _posX[3] + _colWidth[3] * 0.5f + _colWidth[4] * 0.5f;
    _posX[5] = _posX[4] + _colWidth[4] * 0.5f + _colWidth[5] * 0.5f;
    _posX[6] = _posX[5] + _colWidth[5] * 0.5f + _colWidth[6] * 0.5f;
    _posX[7] = _posX[6] + _colWidth[6] * 0.5f + _colWidth[7] * 0.5f;

    cw::TableView *tableView = cw::TableView::create();
    tableView->setContentSize(Size(visibleSize.width, visibleSize.height - 115.0f));
    tableView->setDelegate(this);
    tableView->setDirection(ui::ScrollView::Direction::VERTICAL);
    tableView->setVerticalFillOrder(cw::TableView::VerticalFillOrder::TOP_DOWN);

    tableView->setScrollBarPositionFromCorner(Vec2(5, 2));
    tableView->setScrollBarWidth(4);
    tableView->setScrollBarOpacity(0x99);
    tableView->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    tableView->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f - 55.0f));
    tableView->reloadData();
    this->addChild(tableView);

    return true;
}

ssize_t CompetitionRoundScene::numberOfCellsInTableView(cw::TableView *table) {
    return _competitionData->players.size();
}

cocos2d::Size CompetitionRoundScene::tableCellSizeForIndex(cw::TableView *table, ssize_t idx) {
    return Size(0, 30);
}

cw::TableViewCell *CompetitionRoundScene::tableCellAtIndex(cw::TableView *table, ssize_t idx) {
    typedef cw::TableViewCellEx<Label *[8], ui::EditBox *, LayerColor *[2]> CustomCell;
    CustomCell *cell = (CustomCell *)table->dequeueCell();

    if (cell == nullptr) {
        cell = CustomCell::create();

        CustomCell::ExtDataType &ext = cell->getExtData();
        Label *(&labels)[8] = std::get<0>(ext);
        ui::EditBox *&editBox = std::get<1>(ext);
        LayerColor *(&layerColor)[2] = std::get<2>(ext);

        Size visibleSize = Director::getInstance()->getVisibleSize();

        layerColor[0] = LayerColor::create(Color4B(0xC0, 0xC0, 0xC0, 0x10), visibleSize.width, 29);
        cell->addChild(layerColor[0]);

        layerColor[1] = LayerColor::create(Color4B(0x80, 0x80, 0x80, 0x10), visibleSize.width, 29);
        cell->addChild(layerColor[1]);

        for (int i = 0; i < 8; ++i) {
            Label *label = Label::createWithSystemFont("", "Arail", 12);
            label->setColor(Color3B::BLACK);
            cell->addChild(label);
            label->setPosition(Vec2(_posX[i], 15.0f));
            labels[i] = label;
        }
    }

    const CustomCell::ExtDataType ext = cell->getExtData();
    Label *const (&labels)[8] = std::get<0>(ext);
    ui::EditBox *editBox = std::get<1>(ext);
    LayerColor *const (&layerColor)[2] = std::get<2>(ext);

    layerColor[0]->setVisible(!(idx & 1));
    layerColor[1]->setVisible(!!(idx & 1));

    const CompetitionPlayer &player = _competitionData->players[idx];
    labels[0]->setString(std::to_string(player.serial));
    labels[1]->setString(player.name);
    labels[2]->setString("桌号");
    labels[3]->setString("座位");
    labels[4]->setString("标准分");
    labels[5]->setString("比赛分");
    labels[6]->setString("标准分");
    labels[7]->setString("比赛分");

    return cell;
}
