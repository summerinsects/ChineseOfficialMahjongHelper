#ifdef _MSC_VER
#pragma warning(disable: 4351)
#endif

#include "FanTable.h"
#include "FanDefinition.h"
#include "../mahjong-algorithm/fan_calculator.h"
#include "../common.h"

USING_NS_CC;

Scene *FanTableScene::createScene() {
    auto scene = Scene::create();
    auto layer = FanTableScene::create();
    scene->addChild(layer);
    return scene;
}

static const int fanLevel[] = { 1, 2, 4, 6, 8, 12, 16, 24, 32, 48, 64, 88 };  // 番种
static const size_t eachLevelCounts[] = { 13, 10, 4, 7, 9, 5, 6, 9, 3, 2, 6, 7 };  // 各档次番种的个数
static const size_t eachLevelBeginIndex[] = { 69, 59, 55, 48, 39, 34, 28, 19, 16, 14, 8, 1 };

static inline size_t computeRowsAlign4(size_t cnt) {
    return (cnt >> 2) + !!(cnt & 0x3);
}

bool FanTableScene::init() {
    if (!BaseLayer::initWithTitle("国标麻将番种表")) {
        return false;
    }

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    Label *label = Label::createWithSystemFont("基本计分原则", "Arial", 12);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    this->addChild(label);
    label->setPosition(Vec2(origin.x + 10, origin.y + visibleSize.height - 40));
    if (!UserDefault::getInstance()->getBoolForKey("night_mode")) {
        label->setColor(Color3B::BLACK);
    }

    for (size_t i = 0; i < 5; ++i) {
        ui::Button *button = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
        button->setScale9Enabled(true);
        button->setContentSize(Size(90.0f, 20.0f));
        button->setTitleColor(Color3B::BLACK);
        button->setTitleFontSize(12);
        button->setTitleText(principle_title[i]);
        button->setUserData(reinterpret_cast<void *>(100 + i));
        button->addClickEventListener(std::bind(&FanTableScene::onPointsNameButton, this, std::placeholders::_1));

        this->addChild(button);

        size_t col = i & 0x1;
        size_t row = i >> 1;
        button->setPosition(Vec2(origin.x + visibleSize.width * 0.25f + col * visibleSize.width * 0.5f,
            origin.y + visibleSize.height - 65 - row * 30));
    }

    cw::TableView *tableView = cw::TableView::create();
    tableView->setContentSize(Size(visibleSize.width - 10, visibleSize.height - 150));
    tableView->setDelegate(this);
    tableView->setDirection(ui::ScrollView::Direction::VERTICAL);
    tableView->setVerticalFillOrder(cw::TableView::VerticalFillOrder::TOP_DOWN);

    tableView->setScrollBarPositionFromCorner(Vec2(5, 5));
    tableView->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    tableView->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f - 70.0f));
    tableView->reloadData();
    this->addChild(tableView);

    return true;
}

ssize_t FanTableScene::numberOfCellsInTableView(cw::TableView *table) {
    return 12;
}

cocos2d::Size FanTableScene::tableCellSizeForIndex(cw::TableView *table, ssize_t idx) {
    size_t cnt = eachLevelCounts[idx];
    float height = computeRowsAlign4(cnt) * 25.0f;
    return Size(0, height + 15.0f);
}

cw::TableViewCell *FanTableScene::tableCellAtIndex(cw::TableView *table, ssize_t idx) {
    typedef cw::TableViewCellEx<Label *, ui::Button *[13]> CustomCell;
    CustomCell *cell = (CustomCell *)table->dequeueCell();

    Size visibleSize = Director::getInstance()->getVisibleSize();
    const float gap = (visibleSize.width - 10.0f) * 0.25f;

    if (cell == nullptr) {
        cell = CustomCell::create();

        CustomCell::ExtDataType &ext = cell->getExtData();
        Label *&label = std::get<0>(ext);
        ui::Button *(&buttons)[13] = std::get<1>(ext);

        label = Label::createWithSystemFont("1番", "Arial", 12);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        cell->addChild(label);
        if (!UserDefault::getInstance()->getBoolForKey("night_mode")) {
            label->setColor(Color3B::BLACK);
        }

        for (size_t k = 0; k < 13; ++k) {
            ui::Button *button = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
            button->setScale9Enabled(true);
            button->setContentSize(Size(gap - 5.0f, 20.0f));
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
    ui::Button *const (&buttons)[13] = std::get<1>(ext);

    label->setString(StringUtils::format("%d番", fanLevel[idx]));
    label->setPosition(Vec2(5.0f, totalRows * 25.0f + 7.0f));

    for (size_t k = 0; k < currentLevelCount; ++k) {
        size_t idx0 = eachLevelBeginIndex[idx] + k;
        ui::Button *button = buttons[k];
        button->setTitleText(mahjong::fan_name[idx0]);
        button->setUserData(reinterpret_cast<void *>(idx0));
        button->setVisible(true);
        button->setEnabled(true);
        size_t col = k & 0x3;
        size_t row = k >> 2;
        button->setPosition(Vec2(gap * (col + 0.5f), (totalRows - row - 0.5f) * 25.0f));

        scaleLabelToFitWidth(button->getTitleLabel(), gap - 10.0f);
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
    Director::getInstance()->pushScene(FanDefinitionScene::createScene(idx));
}
