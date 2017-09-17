#include "CompetitionRoundScene.h"
#include <array>
#include "../widget/AlertView.h"
#include "Competition.h"
#include "CompetitionTableScene.h"

USING_NS_CC;

CompetitionRoundScene *CompetitionRoundScene::create(const std::shared_ptr<CompetitionData> &competitionData, unsigned currentRound) {
    CompetitionRoundScene *ret = new (std::nothrow) CompetitionRoundScene();
    if (ret != nullptr && ret->initWithData(competitionData, currentRound)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool CompetitionRoundScene::initWithData(const std::shared_ptr<CompetitionData> &competitionData, unsigned currentRound) {
    if (UNLIKELY(!BaseScene::initWithTitle(Common::format<256>("%s第%u/%lu轮",
        competitionData->name.c_str(), currentRound + 1, static_cast<unsigned long>(competitionData->round_count))))) {
        return false;
    }

    _competitionData = competitionData;
    _currentRound = currentRound;

    CompetitionRound::sortPlayers(_currentRound, competitionData->players, _players);

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    // 上一轮按钮
    ui::Button *button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(55.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText("上一轮");
    button->setPosition(Vec2(origin.x + visibleSize.width * 0.5f - 65, origin.y + visibleSize.height - 70.0f));
    button->addClickEventListener([this](Ref *) {
        if (_currentRound > 0) {
            Director::getInstance()->replaceScene(CompetitionRoundScene::create(_competitionData, _currentRound - 1));
        }
    });
    button->setEnabled(_currentRound > 0);

    // 排列座位按钮
    button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(55.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText("排列座位");
    button->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height - 70.0f));
    button->addClickEventListener(std::bind(&CompetitionRoundScene::onRankButton, this, std::placeholders::_1));

    // 下一轮按钮
    button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(55.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText("下一轮");
    button->setPosition(Vec2(origin.x + visibleSize.width * 0.5f + 65, origin.y + visibleSize.height - 70.0f));
    button->addClickEventListener([this](Ref *) {
        unsigned nextRound = _currentRound + 1;
        if (nextRound < _competitionData->round_count) {
            if (_competitionData->isRoundFinished(_currentRound)) {
                Director::getInstance()->replaceScene(CompetitionRoundScene::create(_competitionData, nextRound));
            }
            else {
                AlertView::showWithMessage("下一轮", "当前一轮尚未结束，请先将所有桌的成绩登记完毕。", 12, nullptr, nullptr);
            }
        }
    });
    button->setEnabled(_currentRound + 1 < _competitionData->round_count);

    _colWidth[0] = visibleSize.width * 0.1f;
    _colWidth[1] = visibleSize.width * 0.1f;
    _colWidth[2] = visibleSize.width * 0.2f;

    _colWidth[3] = visibleSize.width * 0.15f;
    _colWidth[4] = visibleSize.width * 0.15f;
    _colWidth[5] = visibleSize.width * 0.15f;
    _colWidth[6] = visibleSize.width * 0.15f;

    _posX[0] = _colWidth[0] * 0.5f;
    _posX[1] = _posX[0] + _colWidth[0] * 0.5f + _colWidth[1] * 0.5f;
    _posX[2] = _posX[1] + _colWidth[1] * 0.5f + _colWidth[2] * 0.5f;
    _posX[3] = _posX[2] + _colWidth[2] * 0.5f + _colWidth[3] * 0.5f;
    _posX[4] = _posX[3] + _colWidth[3] * 0.5f + _colWidth[4] * 0.5f;
    _posX[5] = _posX[4] + _colWidth[4] * 0.5f + _colWidth[5] * 0.5f;
    _posX[6] = _posX[5] + _colWidth[5] * 0.5f + _colWidth[6] * 0.5f;

    const char *titleTexts[] = { "名次", "编号", "选手姓名", "标准分", "比赛分", "标准分", "比赛分" };
    for (int i = 0; i < 7; ++i) {
        Label *label = Label::createWithSystemFont(titleTexts[i], "Arail", 12);
        label->setColor(Color3B::BLACK);
        this->addChild(label);
        label->setPosition(Vec2(origin.x + _posX[i], visibleSize.height - 100.0f));
        Common::scaleLabelToFitWidth(label, _colWidth[i] - 4);
    }

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
    _tableView = tableView;

    return true;
}

ssize_t CompetitionRoundScene::numberOfCellsInTableView(cw::TableView *table) {
    return _players.size();
}

cocos2d::Size CompetitionRoundScene::tableCellSizeForIndex(cw::TableView *table, ssize_t idx) {
    return Size(0, 30);
}

cw::TableViewCell *CompetitionRoundScene::tableCellAtIndex(cw::TableView *table, ssize_t idx) {
    typedef cw::TableViewCellEx<std::array<Label *, 7>, std::array<LayerColor *, 2> > CustomCell;
    CustomCell *cell = (CustomCell *)table->dequeueCell();

    if (cell == nullptr) {
        cell = CustomCell::create();

        CustomCell::ExtDataType &ext = cell->getExtData();
        std::array<Label *, 7> &labels = std::get<0>(ext);
        std::array<LayerColor *, 2> &layerColors = std::get<1>(ext);

        Size visibleSize = Director::getInstance()->getVisibleSize();

        layerColors[0] = LayerColor::create(Color4B(0xC0, 0xC0, 0xC0, 0x10), visibleSize.width, 29);
        cell->addChild(layerColors[0]);

        layerColors[1] = LayerColor::create(Color4B(0x80, 0x80, 0x80, 0x10), visibleSize.width, 29);
        cell->addChild(layerColors[1]);

        for (int i = 0; i < 7; ++i) {
            Label *label = Label::createWithSystemFont("", "Arail", 12);
            label->setColor(Color3B::BLACK);
            cell->addChild(label);
            label->setPosition(Vec2(_posX[i], 15.0f));
            labels[i] = label;
        }
    }

    const CustomCell::ExtDataType ext = cell->getExtData();
    const std::array<Label *, 7> &labels = std::get<0>(ext);
    const std::array<LayerColor *, 2> &layerColors = std::get<1>(ext);

    layerColors[0]->setVisible(!(idx & 1));
    layerColors[1]->setVisible(!!(idx & 1));

    const CompetitionPlayer &player = *_players[idx];
    labels[0]->setString(std::to_string(idx + 1));
    labels[1]->setString(std::to_string(player.serial));
    labels[2]->setString(player.name);

    std::pair<float, int> ret = player.getCurrentScoresByRound(_currentRound);
    labels[3]->setString(CompetitionResult::standardScoreToString(ret.first));
    Common::scaleLabelToFitWidth(labels[3], _colWidth[3] - 4);
    labels[4]->setString(std::to_string(ret.second));
    Common::scaleLabelToFitWidth(labels[4], _colWidth[4] - 4);

    ret = player.getTotalScoresByRound(_currentRound);
    labels[5]->setString(CompetitionResult::standardScoreToString(ret.first));
    Common::scaleLabelToFitWidth(labels[5], _colWidth[5] - 4);
    labels[6]->setString(std::to_string(ret.second));
    Common::scaleLabelToFitWidth(labels[6], _colWidth[6] - 4);

    return cell;
}

void CompetitionRoundScene::onRankButton(cocos2d::Ref *sender) {
    CompetitionTableScene *scene = CompetitionTableScene::create(_competitionData, _currentRound);
    scene->setOnExitCallback([this]() {
        CompetitionRound::sortPlayers(_currentRound, _competitionData->players, _players);
        _tableView->reloadData();
    });
    Director::getInstance()->pushScene(scene);
}
