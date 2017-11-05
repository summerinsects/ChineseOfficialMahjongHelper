#include "FanTableScene.h"
#include <array>
#include "../mahjong-algorithm/fan_calculator.h"
#include "FanDefinitionScene.h"

USING_NS_CC;

static const char *principle_title[] = { "不重复原则", "不拆移原则", "不得相同原则", "就高不就低", "套算一次原则" };

static const int fanLevel[] = { 0, 1, 2, 4, 6, 8, 12, 16, 24, 32, 48, 64, 88 };  // 番种
static const size_t eachLevelCounts[] = { 5, 13, 10, 4, 7, 9, 5, 6, 9, 3, 2, 6, 7 };  // 各档次番种的个数
static const size_t eachLevelBeginIndex[] = { 0, 69, 59, 55, 48, 39, 34, 28, 19, 16, 14, 8, 1 };

static FORCE_INLINE size_t computeRowsAlign4(size_t cnt) {
    return (cnt >> 2) + !!(cnt & 0x3);
}

bool FanTableScene::init() {
    if (UNLIKELY(!BaseScene::initWithTitle("国标麻将番种表"))) {
        return false;
    }

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    cw::TableView *tableView = cw::TableView::create();
    tableView->setDirection(ui::ScrollView::Direction::VERTICAL);
    tableView->setScrollBarPositionFromCorner(Vec2(2.0f, 2.0f));
    tableView->setScrollBarWidth(4.0f);
    tableView->setScrollBarOpacity(0x99);
    tableView->setBounceEnabled(true);
    tableView->setContentSize(Size(visibleSize.width - 5.0f, visibleSize.height - 35.0f));
    tableView->setDelegate(this);
    tableView->setVerticalFillOrder(cw::TableView::VerticalFillOrder::TOP_DOWN);

    tableView->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    tableView->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f - 15.0f));
    tableView->reloadData();
    this->addChild(tableView);

    return true;
}

ssize_t FanTableScene::numberOfCellsInTableView(cw::TableView *) {
    return 13;
}

float FanTableScene::tableCellSizeForIndex(cw::TableView *, ssize_t idx) {
    size_t cnt = eachLevelCounts[idx];
    float height = computeRowsAlign4(cnt) * 25.0f;
    return (height + 15.0f);
}

cw::TableViewCell *FanTableScene::tableCellAtIndex(cw::TableView *table, ssize_t idx) {
    typedef cw::TableViewCellEx<Label *, std::array<ui::Button *, 13> > CustomCell;
    CustomCell *cell = (CustomCell *)table->dequeueCell();

    Size visibleSize = Director::getInstance()->getVisibleSize();
    const float gap = (visibleSize.width - 5.0f) * 0.25f;

    if (cell == nullptr) {
        cell = CustomCell::create();

        CustomCell::ExtDataType &ext = cell->getExtData();
        Label *&label = std::get<0>(ext);
        std::array<ui::Button *, 13> &buttons = std::get<1>(ext);

        label = Label::createWithSystemFont("1番", "Arial", 12);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        cell->addChild(label);
        label->setColor(Color3B::BLACK);

        for (size_t k = 0; k < 13; ++k) {
            ui::Button *button = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
            button->setScale9Enabled(true);
            button->setContentSize(Size(gap - 4.0f, 20.0f));
            button->setTitleColor(Color3B::BLACK);
            button->setTitleFontSize(12);
            button->addClickEventListener(std::bind(&FanTableScene::onPointsNameButton, this, std::placeholders::_1));

            cell->addChild(button);
            buttons[k] = button;
        }
    }

    const size_t currentLevelCount = eachLevelCounts[idx];
    size_t totalRows = computeRowsAlign4(currentLevelCount);

    const CustomCell::ExtDataType ext = cell->getExtData();
    Label *label = std::get<0>(ext);
    const std::array<ui::Button *, 13> &buttons = std::get<1>(ext);

    size_t idx0;
    const char **titleTexts;
    if (fanLevel[idx] == 0) {
        label->setString("基本计分原则");
        idx0 = 100;
        titleTexts = &principle_title[0];
    }
    else {
        label->setString(std::to_string(fanLevel[idx]).append("番"));
        idx0 = eachLevelBeginIndex[idx];
        titleTexts = &mahjong::fan_name[idx0];
    }
    label->setPosition(Vec2(5.0f, totalRows * 25.0f + 7.0f));

    for (size_t k = 0; k < currentLevelCount; ++k) {
        ui::Button *button = buttons[k];
        button->setTitleText(titleTexts[k]);
        button->setUserData(reinterpret_cast<void *>(idx0 + k));
        button->setVisible(true);
        button->setEnabled(true);
        size_t col = k & 0x3;
        size_t row = k >> 2;
        button->setPosition(Vec2(gap * (col + 0.5f), (totalRows - row - 0.5f) * 25.0f));

        cw::scaleLabelToFitWidth(button->getTitleLabel(), gap - 8.0f);
    }

    for (size_t k = currentLevelCount; k < 13; ++k) {
        buttons[k]->setVisible(false);
        buttons[k]->setEnabled(false);
    }

    return cell;
}

void FanTableScene::onPointsNameButton(cocos2d::Ref *sender) {
    ui::Button *button = (ui::Button *)sender;
    size_t idx = reinterpret_cast<size_t>(button->getUserData());
    Director::getInstance()->pushScene(FanDefinitionScene::create(idx));
}
