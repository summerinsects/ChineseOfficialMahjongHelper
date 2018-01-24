#include "FanTableScene.h"
#include <array>
#include "../mahjong-algorithm/fan_calculator.h"
#include "FanDefinitionScene.h"

USING_NS_CC;

static const Color3B C3B_GRAY = Color3B(96, 96, 96);

static const char *principle_title[] = { "不重复原则", "不拆移原则", "不得相同原则", "就高不就低", "套算一次原则" };

namespace {
    typedef struct {
        const char *const title;
        const char **fan_names;
        const size_t count;
        const size_t begin_index;
    } CellDetail;
}

static const CellDetail cellDetails[13] = {
    { "基本计分原则", principle_title, 5, 100 },
    { "1番", &mahjong::fan_name[mahjong::PURE_DOUBLE_CHOW], 13, mahjong::PURE_DOUBLE_CHOW },
    { "2番", &mahjong::fan_name[mahjong::DRAGON_PUNG], 10, mahjong::DRAGON_PUNG },
    { "4番", &mahjong::fan_name[mahjong::OUTSIDE_HAND], 4, mahjong::OUTSIDE_HAND },
    { "6番", &mahjong::fan_name[mahjong::ALL_PUNGS], 7, mahjong::ALL_PUNGS },
    { "8番", &mahjong::fan_name[mahjong::MIXED_STRAIGHT], 9, mahjong::MIXED_STRAIGHT },
    { "12番", &mahjong::fan_name[mahjong::LESSER_HONORS_AND_KNITTED_TILES], 5, mahjong::LESSER_HONORS_AND_KNITTED_TILES },
    { "16番", &mahjong::fan_name[mahjong::PURE_STRAIGHT], 6, mahjong::PURE_STRAIGHT },
    { "24番", &mahjong::fan_name[mahjong::SEVEN_PAIRS], 9, mahjong::SEVEN_PAIRS },
    { "32番", &mahjong::fan_name[mahjong::FOUR_PURE_SHIFTED_CHOWS], 3, mahjong::FOUR_PURE_SHIFTED_CHOWS },
    { "48番", &mahjong::fan_name[mahjong::QUADRUPLE_CHOW], 2, mahjong::QUADRUPLE_CHOW },
    { "64番", &mahjong::fan_name[mahjong::ALL_TERMINALS], 6, mahjong::ALL_TERMINALS },
    { "88番", &mahjong::fan_name[mahjong::BIG_FOUR_WINDS], 7, mahjong::BIG_FOUR_WINDS }
};

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
    size_t cnt = cellDetails[idx].count;
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
        ui::Button **buttons = std::get<1>(ext).data();

        label = Label::createWithSystemFont("1番", "Arial", 12);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        cell->addChild(label);
        label->setColor(Color3B::BLACK);

        for (size_t k = 0; k < 13; ++k) {
            ui::Button *button = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
            button->setScale9Enabled(true);
            button->setContentSize(Size(gap - 4.0f, 20.0f));
            button->setTitleColor(C3B_GRAY);
            button->setTitleFontSize(12);
            button->addClickEventListener(std::bind(&FanTableScene::onFanNameButton, this, std::placeholders::_1));

            cell->addChild(button);
            buttons[k] = button;
        }
    }

    const CellDetail &detail = cellDetails[idx];
    const size_t currentLevelCount = detail.count;
    size_t totalRows = computeRowsAlign4(currentLevelCount);

    const CustomCell::ExtDataType &ext = cell->getExtData();
    Label *label = std::get<0>(ext);
    ui::Button *const *buttons = std::get<1>(ext).data();

    label->setString(detail.title);
    label->setPosition(Vec2(5.0f, totalRows * 25.0f + 7.0f));

    size_t idx0 = detail.begin_index;
    const char **titleTexts = detail.fan_names;

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

void FanTableScene::onFanNameButton(cocos2d::Ref *sender) {
    ui::Button *button = (ui::Button *)sender;
    size_t idx = reinterpret_cast<size_t>(button->getUserData());
    Director::getInstance()->pushScene(FanDefinitionScene::create(idx));
}
