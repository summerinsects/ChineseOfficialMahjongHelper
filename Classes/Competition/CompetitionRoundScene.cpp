#include "CompetitionRoundScene.h"
#include "../common.h"

USING_NS_CC;

bool CompetitionRoundScene::init() {
    if (UNLIKELY(!BaseScene::initWithTitle("第N轮"))) {
        return false;
    }

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    _colWidth[0] = visibleSize.width * 0.2f;
    _colWidth[1] = visibleSize.width * 0.1f;
    _colWidth[2] = visibleSize.width * 0.1f;
    _colWidth[3] = visibleSize.width * 0.1f;
    _colWidth[4] = visibleSize.width * 0.125f;
    _colWidth[5] = visibleSize.width * 0.125f;
    _colWidth[6] = visibleSize.width * 0.125f;
    _colWidth[7] = visibleSize.width * 0.125f;

    cw::TableView *tableView = cw::TableView::create();
    tableView->setContentSize(Size(visibleSize.width, visibleSize.height - 150));
    tableView->setDelegate(this);
    tableView->setDirection(ui::ScrollView::Direction::VERTICAL);
    tableView->setVerticalFillOrder(cw::TableView::VerticalFillOrder::TOP_DOWN);

    tableView->setScrollBarPositionFromCorner(Vec2(5, 5));
    tableView->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    tableView->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f - 70.0f));
    tableView->reloadData();
    this->addChild(tableView);

    DrawNode *drawNode = DrawNode::create();
    tableView->getInnerContainer()->addChild(drawNode);

    float h = tableView->getInnerContainerSize().height;
    float xPos = 0;
    for (int i = 0; i < 7; ++i) {
        xPos += _colWidth[i];
        drawNode->drawLine(Vec2(xPos, 0), Vec2(xPos, h), Color4F::BLACK);
    }

    return true;
}

ssize_t CompetitionRoundScene::numberOfCellsInTableView(cw::TableView *table) {
    return 12;
}

cocos2d::Size CompetitionRoundScene::tableCellSizeForIndex(cw::TableView *table, ssize_t idx) {
    return Size(0, 20);
}

cw::TableViewCell *CompetitionRoundScene::tableCellAtIndex(cw::TableView *table, ssize_t idx) {
    typedef cw::TableViewCellEx<Label *[7], ui::EditBox *, LayerColor *[2]> CustomCell;
    CustomCell *cell = (CustomCell *)table->dequeueCell();

    if (cell == nullptr) {
        cell = CustomCell::create();

        CustomCell::ExtDataType &ext = cell->getExtData();
        Label *(&label)[7] = std::get<0>(ext);
        ui::EditBox *&editBox = std::get<1>(ext);
        LayerColor *(&layerColor)[2] = std::get<2>(ext);

        Size visibleSize = Director::getInstance()->getVisibleSize();

        layerColor[0] = LayerColor::create(Color4B(0xC0, 0xC0, 0xC0, 0x10), visibleSize.width, 20);
        cell->addChild(layerColor[0]);

        layerColor[1] = LayerColor::create(Color4B(0x80, 0x80, 0x80, 0x10), visibleSize.width, 20);
        cell->addChild(layerColor[1]);
    }

    const CustomCell::ExtDataType ext = cell->getExtData();
    Label *const (&label)[7] = std::get<0>(ext);
    ui::EditBox *editBox = std::get<1>(ext);
    LayerColor *const (&layerColor)[2] = std::get<2>(ext);

    layerColor[0]->setVisible(!(idx & 1));
    layerColor[1]->setVisible(!!(idx & 1));

    return cell;
}
