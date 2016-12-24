#include "ScoreTable.h"
#include "ScoreDefinition.h"
#include "../widget/CWTableView.h"
#include "../mahjong-algorithm/points_calculator.h"

USING_NS_CC;

Scene *ScoreTableScene::createScene() {
    auto scene = Scene::create();
    auto layer = ScoreTableScene::create();
    scene->addChild(layer);
    return scene;
}

static const int pointsLevel[] = { 1, 2, 4, 6, 8, 12, 16, 24, 32, 48, 64, 88 };  // 番种
static const size_t eachLevelCounts[] = { 13, 10, 4, 7, 8, 5, 6, 9, 3, 2, 6, 7 };  // 各档次番种的个数
static const size_t eachLevelBeginIndex[] =
#if HAS_CONCEALED_KONG_AND_MELDED_KONG
{ 70, 60, 56, 48, 39, 34, 28, 19, 16, 14, 8, 1 };
#else
{ 69, 59, 55, 48, 39, 34, 28, 19, 16, 14, 8, 1 };
#endif

static inline size_t computeRowsAlign4(size_t cnt) {
    return (cnt >> 2) + !!(cnt & 0x3);
}

bool ScoreTableScene::init() {
    if (!BaseLayer::initWithTitle("国标麻将番种表")) {
        return false;
    }

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    cw::TableView *tableView = cw::TableView::create();
    tableView->setContentSize(Size(visibleSize.width - 10, visibleSize.height - 35));
    tableView->setTableViewCallback([this](cw::TableView *table, cw::TableView::CallbackType type, intptr_t param) {
        switch (type) {
        case cw::TableView::CallbackType::CELL_SIZE: {
            auto p = (cw::TableView::CellSizeParam *)param;
            size_t cnt = eachLevelCounts[p->idx];
            float height = computeRowsAlign4(cnt) * 25.0f;
            p->size = Size(0, height + 15.0f);
            return 0;
        }
        case cw::TableView::CallbackType::CELL_AT_INDEX:
            return (intptr_t)tableCellAtIndex(table, param);
        case cw::TableView::CallbackType::NUMBER_OF_CELLS:
            return (intptr_t)12;
        }
        return 0;
    });

    tableView->setDirection(ui::ScrollView::Direction::VERTICAL);
    tableView->setVerticalFillOrder(cw::TableView::VerticalFillOrder::TOP_DOWN);

    tableView->setScrollBarPositionFromCorner(Vec2(5, 5));
    tableView->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    tableView->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f - 15.0f));
    tableView->reloadData();
    this->addChild(tableView);

    return true;
}

cw::TableViewCell *ScoreTableScene::tableCellAtIndex(cw::TableView *table, ssize_t idx) {
    typedef cw::TableViewCellEx<Label *, ui::Button *[13]> CustomCell;
    CustomCell *cell = (CustomCell *)table->dequeueCell();

    if (cell == nullptr) {
        cell = CustomCell::create();

        CustomCell::ExtDataType &ext = cell->getExtData();
        Label *&label = std::get<0>(ext);
        ui::Button *(&buttons)[13] = std::get<1>(ext);

        label = Label::createWithSystemFont("1番", "Arial", 12);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        cell->addChild(label);

        for (size_t k = 0; k < 13; ++k) {
            size_t idx0 = eachLevelBeginIndex[idx] + k;
            ui::Button *button = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
            button->setScale9Enabled(true);
            button->setContentSize(Size(65.0f, 20.0f));
            button->setTitleColor(Color3B::BLACK);
            button->setTitleFontSize(12);
            button->addClickEventListener(std::bind(&ScoreTableScene::onPointsNameButton, this, std::placeholders::_1));

            cell->addChild(button);
            buttons[k] = button;
        }
    }

    const size_t currentLevelCount = eachLevelCounts[idx];

    Size visibleSize = Director::getInstance()->getVisibleSize();
    const float gap = (visibleSize.width - 10.0f) * 0.25f;
    size_t totalRows = computeRowsAlign4(currentLevelCount);

    const CustomCell::ExtDataType ext = cell->getExtData();
    Label *label = std::get<0>(ext);
    ui::Button *const (&buttons)[13] = std::get<1>(ext);

    label->setString(StringUtils::format("%d番", pointsLevel[idx]));
    label->setPosition(Vec2(5.0f, totalRows * 25.0f + 7.0f));

    for (size_t k = 0; k < currentLevelCount; ++k) {
        size_t idx0 = eachLevelBeginIndex[idx] + k;
        ui::Button *button = buttons[k];
        button->setTitleText(mahjong::points_name[idx0]);
        button->setTag(idx0);
        button->setVisible(true);
        button->setEnabled(true);
        size_t col = k & 0x3;
        size_t row = k >> 2;
        button->setPosition(Vec2(gap * (col + 0.5f), (totalRows - row - 0.5f) * 25.0f));
    }

    for (size_t k = currentLevelCount; k < 13; ++k) {
        buttons[k]->setVisible(false);
        buttons[k]->setEnabled(false);
    }

    return cell;
}

void ScoreTableScene::onPointsNameButton(cocos2d::Ref *sender) {
    ui::Button *button = (ui::Button *)sender;
    size_t idx = button->getTag();
    Director::getInstance()->pushScene(ScoreDefinitionScene::createScene(idx));
}
