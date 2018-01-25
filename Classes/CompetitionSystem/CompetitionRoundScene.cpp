#include "CompetitionRoundScene.h"
#include <array>
#include "../UICommon.h"
#include "../widget/Toast.h"
#include "Competition.h"
#include "CompetitionTableScene.h"

USING_NS_CC;

bool CompetitionRoundScene::initWithData(const std::shared_ptr<CompetitionData> &competitionData, size_t currentRound) {
    if (UNLIKELY(!BaseScene::initWithTitle(Common::format(__UTF8("%s第%") __UTF8(PRIzu) __UTF8("/%") __UTF8(PRIzu) __UTF8("轮"),
        competitionData->name.c_str(), currentRound + 1, competitionData->round_count)))) {
        return false;
    }

    _competitionData = competitionData;
    _currentRound = currentRound;

    CompetitionRound::sortPlayers(_currentRound, competitionData->players, _players);

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    // 上一轮按钮
    ui::Button *button = UICommon::createButton();
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(55.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText(__UTF8("上一轮"));
    button->setPosition(Vec2(origin.x + visibleSize.width * 0.5f - 65.0f, origin.y + visibleSize.height - 45.0f));
    button->addClickEventListener([this](Ref *) {
        if (_currentRound > 0) {
            Director::getInstance()->replaceScene(CompetitionRoundScene::create(_competitionData, _currentRound - 1));
        }
    });
    button->setEnabled(_currentRound > 0);

    // 登记成绩按钮
    button = UICommon::createButton();
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(55.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText(__UTF8("登记成绩"));
    button->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height - 45.0f));
    button->addClickEventListener(std::bind(&CompetitionRoundScene::onReportButton, this, std::placeholders::_1));

    // 下一轮按钮
    button = UICommon::createButton();
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(55.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText(__UTF8("下一轮"));
    button->setPosition(Vec2(origin.x + visibleSize.width * 0.5f + 65.0f, origin.y + visibleSize.height - 45.0f));
    button->addClickEventListener([this](Ref *) {
        size_t nextRound = _currentRound + 1;
        if (nextRound < _competitionData->round_count) {
            if (_competitionData->isRoundFinished(_currentRound)) {
                if (_currentRound == _competitionData->rounds.size() - 1) {
                    _competitionData->startNewRound();
                    _competitionData->writeToFile();
                }
                Director::getInstance()->replaceScene(CompetitionRoundScene::create(_competitionData, nextRound));
            }
            else {
                Toast::makeText(this, __UTF8("当前一轮尚未结束，请先将所有桌的成绩登记完毕。"), Toast::LENGTH_LONG)->show();
            }
        }
    });
    button->setEnabled(_currentRound + 1 < _competitionData->round_count);

    // 列宽
    _colWidth[0] = visibleSize.width * 0.08f;
    _colWidth[1] = visibleSize.width * 0.08f;
    _colWidth[2] = visibleSize.width * 0.2f;
    _colWidth[3] = visibleSize.width * 0.16f;
    _colWidth[4] = visibleSize.width * 0.16f;
    _colWidth[5] = visibleSize.width * 0.16f;
    _colWidth[6] = visibleSize.width * 0.16f;

    // 中心位置
    cw::calculateColumnsCenterX(_colWidth, 7, _posX);

    // 表头
    static const char *titleTexts[] = { __UTF8("名次"), __UTF8("编号"), __UTF8("选手姓名"), __UTF8("本轮标准分"), __UTF8("本轮比赛分"), __UTF8("累计标准分"), __UTF8("累计比赛分") };
    for (int i = 0; i < 7; ++i) {
        Label *label = Label::createWithSystemFont(titleTexts[i], "Arail", 12);
        label->setColor(Color3B::BLACK);
        this->addChild(label);
        label->setPosition(Vec2(origin.x + _posX[i], visibleSize.height - 70.0f));
        cw::scaleLabelToFitWidth(label, _colWidth[i] - 4.0f);
    }

    const float tableHeight = visibleSize.height - 85.0f;

    // 表格
    cw::TableView *tableView = cw::TableView::create();
    tableView->setContentSize(Size(visibleSize.width, tableHeight));
    tableView->setDelegate(this);
    tableView->setDirection(ui::ScrollView::Direction::VERTICAL);
    tableView->setVerticalFillOrder(cw::TableView::VerticalFillOrder::TOP_DOWN);

    tableView->setScrollBarPositionFromCorner(Vec2(5.0f, 2.0f));
    tableView->setScrollBarWidth(4.0f);
    tableView->setScrollBarOpacity(0x99);
    tableView->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    tableView->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f - 37.5f));
    tableView->reloadData();
    this->addChild(tableView);
    _tableView = tableView;

    // 表头的线
    DrawNode *drawNode = DrawNode::create();
    this->addChild(drawNode);
    drawNode->setPosition(Vec2(origin.x, visibleSize.height - 80.0f));
    drawNode->drawLine(Vec2(0.0f, 0.0f), Vec2(visibleSize.width, 0.0f), Color4F::BLACK);
    drawNode->drawLine(Vec2(0.0f, 20.0f), Vec2(visibleSize.width, 20.0f), Color4F::BLACK);
    for (int i = 0; i < 6; ++i) {
        const float posX = _posX[i] + _colWidth[i] * 0.5f;
        drawNode->drawLine(Vec2(posX, 0.0f), Vec2(posX, 20.0f), Color4F::BLACK);
    }

    // 当表格可拖动时，画下方一条横线
    if (tableView->getInnerContainerSize().height > tableHeight) {
        const float posY = -tableHeight;
        drawNode->drawLine(Vec2(0.0f, posY), Vec2(visibleSize.width, posY), Color4F::BLACK);
    }

    return true;
}

ssize_t CompetitionRoundScene::numberOfCellsInTableView(cw::TableView *) {
    return _players.size();
}

float CompetitionRoundScene::tableCellSizeForIndex(cw::TableView *, ssize_t) {
    return 20.0f;
}

cw::TableViewCell *CompetitionRoundScene::tableCellAtIndex(cw::TableView *table, ssize_t idx) {
    typedef cw::TableViewCellEx<std::array<Label *, 7>, std::array<LayerColor *, 2> > CustomCell;
    CustomCell *cell = (CustomCell *)table->dequeueCell();

    if (cell == nullptr) {
        cell = CustomCell::create();

        CustomCell::ExtDataType &ext = cell->getExtData();
        Label **labels = std::get<0>(ext).data();
        LayerColor **layerColors = std::get<1>(ext).data();

        Size visibleSize = Director::getInstance()->getVisibleSize();

        // 背景色
        layerColors[0] = LayerColor::create(Color4B(0x10, 0x10, 0x10, 0x10), visibleSize.width, 20.0f);
        cell->addChild(layerColors[0]);

        layerColors[1] = LayerColor::create(Color4B(0xC0, 0xC0, 0xC0, 0x10), visibleSize.width, 20.0f);
        cell->addChild(layerColors[1]);

        // 名次、编号、选手姓名、本轮标准分、本轮比赛分、累计标准分、累计比赛分，共7个Label
        Color3B textColor[] = { Color3B::BLACK, Color3B(0x60, 0x60, 0x60), Color3B::ORANGE,
            Color3B(254, 87, 110), Color3B(44, 121, 178), Color3B(254, 87, 110), Color3B(44, 121, 178) };
        for (int i = 0; i < 7; ++i) {
            Label *label = Label::createWithSystemFont("", "Arail", 12);
            label->setColor(textColor[i]);
            cell->addChild(label);
            label->setPosition(Vec2(_posX[i], 10.0f));
            labels[i] = label;
        }

        // 画线
        DrawNode *drawNode = DrawNode::create();
        cell->addChild(drawNode);
        drawNode->drawLine(Vec2(0.0f, 0.0f), Vec2(visibleSize.width, 0.0f), Color4F::BLACK);
        drawNode->drawLine(Vec2(0.0f, 20.0f), Vec2(visibleSize.width, 20.0f), Color4F::BLACK);
        for (int i = 0; i < 6; ++i) {
            const float posX = _posX[i] + _colWidth[i] * 0.5f;
            drawNode->drawLine(Vec2(posX, 0.0f), Vec2(posX, 20.0f), Color4F::BLACK);
        }
    }

    const CustomCell::ExtDataType &ext = cell->getExtData();
    Label *const *labels = std::get<0>(ext).data();
    LayerColor *const *layerColors = std::get<1>(ext).data();

    layerColors[0]->setVisible(!(idx & 1));
    layerColors[1]->setVisible(!!(idx & 1));

    const CompetitionPlayer &player = *_players[idx];

    // 名次、编号、选手姓名
    labels[0]->setString(std::to_string(idx + 1));
    labels[1]->setString(std::to_string(player.serial + 1));
    labels[2]->setString(player.name);

    // 本轮标准分和比赛分
    std::pair<float, int> ret = player.getCurrentScoresByRound(_currentRound);
    labels[3]->setString(CompetitionResult::standardScoreToString(ret.first));
    labels[4]->setString(std::to_string(ret.second));

    // 累计标准分和比赛分
    ret = player.getTotalScoresByRound(_currentRound);
    labels[5]->setString(CompetitionResult::standardScoreToString(ret.first));
    labels[6]->setString(std::to_string(ret.second));

    for (int i = 0; i < 7; ++i) {
        cw::scaleLabelToFitWidth(labels[i], _colWidth[i] - 4.0f);
    }

    return cell;
}

void CompetitionRoundScene::onReportButton(cocos2d::Ref *) {
    CompetitionTableScene *scene = CompetitionTableScene::create(_competitionData, _currentRound);
    scene->setOnExitCallback([this]() {
        CompetitionRound::sortPlayers(_currentRound, _competitionData->players, _players);
        _tableView->reloadData();
    });
    Director::getInstance()->pushScene(scene);
}
