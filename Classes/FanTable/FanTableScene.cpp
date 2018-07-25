#include "FanTableScene.h"
#include <array>
#include <algorithm>
#include "../mahjong-algorithm/fan_calculator.h"
#include "../mahjong-algorithm/stringify.h"
#include "../TilesImage.h"
#include "../UIColors.h"
#include "../widget/CommonWebViewScene.h"
#include "../widget/LoadingView.h"

USING_NS_CC;

static const char *principle_title[] = { __UTF8("不重复原则"), __UTF8("不拆移原则"), __UTF8("不得相同原则"), __UTF8("就高不就低"), __UTF8("套算一次原则") };

static std::vector<std::string> g_principles;
static std::vector<std::string> g_definitions;

namespace {
    typedef struct {
        const char *const title;
        const char *const *fan_names;
        const size_t count;
        const size_t begin_index;
    } CellDetail;
}

static const CellDetail cellDetails[13] = {
    { __UTF8("基本计分原则"), principle_title, 5, 100 },
    { __UTF8("1番"), &mahjong::fan_name[mahjong::PURE_DOUBLE_CHOW], 13, mahjong::PURE_DOUBLE_CHOW },
    { __UTF8("2番"), &mahjong::fan_name[mahjong::DRAGON_PUNG], 10, mahjong::DRAGON_PUNG },
    { __UTF8("4番"), &mahjong::fan_name[mahjong::OUTSIDE_HAND], 4, mahjong::OUTSIDE_HAND },
    { __UTF8("6番"), &mahjong::fan_name[mahjong::ALL_PUNGS], 7, mahjong::ALL_PUNGS },
    { __UTF8("8番"), &mahjong::fan_name[mahjong::MIXED_STRAIGHT], 9, mahjong::MIXED_STRAIGHT },
    { __UTF8("12番"), &mahjong::fan_name[mahjong::LESSER_HONORS_AND_KNITTED_TILES], 5, mahjong::LESSER_HONORS_AND_KNITTED_TILES },
    { __UTF8("16番"), &mahjong::fan_name[mahjong::PURE_STRAIGHT], 6, mahjong::PURE_STRAIGHT },
    { __UTF8("24番"), &mahjong::fan_name[mahjong::SEVEN_PAIRS], 9, mahjong::SEVEN_PAIRS },
    { __UTF8("32番"), &mahjong::fan_name[mahjong::FOUR_PURE_SHIFTED_CHOWS], 3, mahjong::FOUR_PURE_SHIFTED_CHOWS },
    { __UTF8("48番"), &mahjong::fan_name[mahjong::QUADRUPLE_CHOW], 2, mahjong::QUADRUPLE_CHOW },
    { __UTF8("64番"), &mahjong::fan_name[mahjong::ALL_TERMINALS], 6, mahjong::ALL_TERMINALS },
    { __UTF8("88番"), &mahjong::fan_name[mahjong::BIG_FOUR_WINDS], 7, mahjong::BIG_FOUR_WINDS }
};

static FORCE_INLINE size_t computeRowsAlign4(size_t cnt) {
    return (cnt >> 2) + ((cnt & 0x3) != 0);
}

bool FanTableScene::init() {
    if (UNLIKELY(!BaseScene::initWithTitle(__UTF8("国标麻将番种表")))) {
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

        label = Label::createWithSystemFont(__UTF8("1番"), "Arial", 12);
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
    const char *const *titleTexts = detail.fan_names;

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
    asyncShowFanDefinition(idx);
}

static void replaceTilesToImage(std::string &text, float scale) {
    const int width = static_cast<int>(TILE_WIDTH * scale);
    const int height = static_cast<int>(TILE_HEIGHT * scale);

    char tilesStr[128];
    mahjong::tile_t tiles[14];
    char imgStr[1024];

    std::string::size_type pos = text.find('[');
    while (pos != std::string::npos) {
        const char *str = text.c_str();
        int readLen;
        if (sscanf(str + pos + 1, "%[^]]%n", tilesStr, &readLen) != EOF
            && str[pos + readLen + 1] == ']') {

            size_t totalWriteLen = 0;
            const char *p = tilesStr;
            if (*p == '_') {
                int writeLen = snprintf(imgStr, sizeof(imgStr),
                    "<img src=\"tiles/bg.png\" width=\"%d\" height=\"%d\"/>", width, height);
                totalWriteLen += writeLen;
                ++p;
            }

            intptr_t tilesCnt = mahjong::parse_tiles(p, tiles, 14);
            for (intptr_t i = 0; i < tilesCnt; ++i) {
                int writeLen = snprintf(imgStr + totalWriteLen, sizeof(imgStr) - totalWriteLen,
                    "<img src=\"%s\" width=\"%d\" height=\"%d\"/>", tilesImageName[tiles[i]], width, height);
                totalWriteLen += writeLen;
            }

            if (tilesStr[readLen - 1] == '_') {
                int writeLen = snprintf(imgStr + totalWriteLen, sizeof(imgStr) - totalWriteLen,
                    "<img src=\"tiles/bg.png\" width=\"%d\" height=\"%d\"/>", width, height);
                totalWriteLen += writeLen;
            }

            text.replace(pos, static_cast<std::string::size_type>(readLen + 2), imgStr);
            pos = text.find('[', pos + totalWriteLen);
        }
        else {
            pos = text.find('[', pos + 1);
        }
    }
}

static void showFanDefinition(size_t idx) {
    const char *title = idx < 100 ? mahjong::fan_name[idx] : principle_title[idx - 100];
    const std::string &text = idx < 100 ? g_definitions[idx] : g_principles[idx - 100];
    CommonWebViewScene *scene = CommonWebViewScene::create(title, text, CommonWebViewScene::ContentType::HTML);
    Director::getInstance()->pushScene(scene);
}

void FanTableScene::asyncShowFanDefinition(size_t idx) {
    if (LIKELY(g_definitions.size() == 82 && g_principles.size() == 5)) {
        showFanDefinition(idx);
        return;
    }

    auto runningScene = makeRef(Director::getInstance()->getRunningScene());  // 保证线程回来之前不析构

    LoadingView *loadingView = LoadingView::create();
    loadingView->showInScene(runningScene.get());

#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_IOS) && !defined(CC_PLATFORM_OS_TVOS)
    Size visibleSize = Director::getInstance()->getVisibleSize();

    float scale = 1.0f;
    float maxWidth = (visibleSize.width - 10) / 18;
    if (maxWidth < 25) {
        scale = maxWidth / TILE_WIDTH;
    }
#else
    float scale = 0.5f;
#endif

    std::thread([runningScene, idx, scale, loadingView]() {
        // 读文件
        auto definitions = std::make_shared<std::vector<std::string> >();
        ValueVector valueVec = FileUtils::getInstance()->getValueVectorFromFile("text/score_definition.xml");
        if (valueVec.size() == 82) {
            definitions->reserve(82);
            std::transform(valueVec.begin(), valueVec.end(), std::back_inserter(*definitions), [scale](const Value &value) {
                std::string ret = value.asString();
                replaceTilesToImage(ret, scale);
                return std::move(ret);
            });
        }

        auto principles = std::make_shared<std::vector<std::string> >();
        valueVec = FileUtils::getInstance()->getValueVectorFromFile("text/score_principles.xml");
        if (valueVec.size() == 5) {
            principles->reserve(5);
            std::transform(valueVec.begin(), valueVec.end(), std::back_inserter(*principles), [scale](const Value &value) {
                std::string ret = value.asString();
                replaceTilesToImage(ret, scale);
                return std::move(ret);
            });
        }

        // 切换到cocos线程
        Director::getInstance()->getScheduler()->performFunctionInCocosThread([runningScene, idx, loadingView, definitions, principles]() {
            g_definitions.swap(*definitions);
            g_principles.swap(*principles);

            if (LIKELY(runningScene->isRunning())) {
                loadingView->dismiss();
                showFanDefinition(idx);
            }
        });
    }).detach();
}
