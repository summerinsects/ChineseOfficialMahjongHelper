#include "CompetitionTableScene.h"
#include "Competition.h"
#include "../common.h"
#include "../widget/AlertView.h"

USING_NS_CC;

CompetitionTableScene *CompetitionTableScene::create(const std::shared_ptr<CompetitionData> &competitionData, unsigned currentRound) {
    CompetitionTableScene *ret = new (std::nothrow) CompetitionTableScene();
    if (ret != nullptr && ret->initWithData(competitionData, currentRound)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool CompetitionTableScene::initWithData(const std::shared_ptr<CompetitionData> &competitionData, unsigned currentRound) {
    if (UNLIKELY(!BaseScene::initWithTitle(Common::format<256>("%s第%u轮", competitionData->name.c_str(), currentRound + 1)))) {
        return false;
    }

    _competitionData = competitionData;
    _currentRound = currentRound;
    _competitionTables = &_competitionData->round[currentRound].tables;

    if (_competitionTables->empty()) {
        rankBySerial();
    }

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    // 排列座位按钮
    ui::Button *button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(55.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText("排列座位");
    button->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height - 70.0f));
    button->addClickEventListener(std::bind(&CompetitionTableScene::onRankButton, this, std::placeholders::_1));

    // 列宽
    _colWidth[0] = visibleSize.width * 0.1f;
    _colWidth[1] = visibleSize.width * 0.1f;
    _colWidth[2] = visibleSize.width * 0.1f;
    _colWidth[3] = visibleSize.width * 0.2f;
    _colWidth[4] = visibleSize.width * 0.15f;
    _colWidth[5] = visibleSize.width * 0.15f;
    _colWidth[6] = visibleSize.width * 0.2f;

    // 中心位置
    _posX[0] = _colWidth[0] * 0.5f;
    _posX[1] = _posX[0] + _colWidth[0] * 0.5f + _colWidth[1] * 0.5f;
    _posX[2] = _posX[1] + _colWidth[1] * 0.5f + _colWidth[2] * 0.5f;
    _posX[3] = _posX[2] + _colWidth[2] * 0.5f + _colWidth[3] * 0.5f;
    _posX[4] = _posX[3] + _colWidth[3] * 0.5f + _colWidth[4] * 0.5f;
    _posX[5] = _posX[4] + _colWidth[4] * 0.5f + _colWidth[5] * 0.5f;
    _posX[6] = _posX[5] + _colWidth[5] * 0.5f + _colWidth[6] * 0.5f;

    const char *titleTexts[] = { "桌号", "座次", "编号", "选手姓名", "标准分", "比赛分" };
    for (int i = 0; i < 6; ++i) {
        Label *label = Label::createWithSystemFont(titleTexts[i], "Arail", 12);
        label->setColor(Color3B::BLACK);
        this->addChild(label);
        label->setPosition(Vec2(origin.x + _posX[i], visibleSize.height - 100.0f));
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
    _tableView = tableView;
    this->addChild(tableView);

    return true;
}

ssize_t CompetitionTableScene::numberOfCellsInTableView(cw::TableView *table) {
    return _competitionTables->size();
}

cocos2d::Size CompetitionTableScene::tableCellSizeForIndex(cw::TableView *table, ssize_t idx) {
    return Size(0, 120);
}

cw::TableViewCell *CompetitionTableScene::tableCellAtIndex(cw::TableView *table, ssize_t idx) {
    typedef cw::TableViewCellEx<Label *, Label *[4], Label *[4], Label *[4], Label *[4], ui::Button *, LayerColor *[2]> CustomCell;
    CustomCell *cell = (CustomCell *)table->dequeueCell();

    if (cell == nullptr) {
        cell = CustomCell::create();

        CustomCell::ExtDataType &ext = cell->getExtData();
        Label *&tableLabel = std::get<0>(ext);
        Label *(&serialLabels)[4] = std::get<1>(ext);
        Label *(&nameLabels)[4] = std::get<2>(ext);
        Label *(&standardLabels)[4] = std::get<3>(ext);
        Label *(&competitionLabels)[4] = std::get<4>(ext);
        ui::Button *&button = std::get<5>(ext);
        LayerColor *(&layerColors)[2] = std::get<6>(ext);

        Size visibleSize = Director::getInstance()->getVisibleSize();

        layerColors[0] = LayerColor::create(Color4B(0xC0, 0xC0, 0xC0, 0x10), visibleSize.width, 119);
        cell->addChild(layerColors[0]);

        layerColors[1] = LayerColor::create(Color4B(0x80, 0x80, 0x80, 0x10), visibleSize.width, 119);
        cell->addChild(layerColors[1]);

        tableLabel = Label::createWithSystemFont("", "Arail", 12);
        tableLabel->setColor(Color3B::BLACK);
        cell->addChild(tableLabel);
        tableLabel->setPosition(Vec2(_posX[0], 60.0f));

        static const char *seatText[] = { "东", "南", "西", "北" };
        for (int i = 0; i < 4; ++i) {
            Label *label = Label::createWithSystemFont(seatText[i], "Arail", 12);
            label->setColor(Color3B::BLACK);
            cell->addChild(label);
            label->setPosition(Vec2(_posX[1], (float)(105 - i * 30)));

            label = Label::createWithSystemFont("", "Arail", 12);
            label->setColor(Color3B::BLACK);
            cell->addChild(label);
            label->setPosition(Vec2(_posX[2], (float)(105 - i * 30)));
            serialLabels[i] = label;

            label = Label::createWithSystemFont("", "Arail", 12);
            label->setColor(Color3B::BLACK);
            cell->addChild(label);
            label->setPosition(Vec2(_posX[3], (float)(105 - i * 30)));
            nameLabels[i] = label;

            label = Label::createWithSystemFont("", "Arail", 12);
            label->setColor(Color3B::BLACK);
            cell->addChild(label);
            label->setPosition(Vec2(_posX[4], (float)(105 - i * 30)));
            standardLabels[i] = label;

            label = Label::createWithSystemFont("", "Arail", 12);
            label->setColor(Color3B::BLACK);
            cell->addChild(label);
            label->setPosition(Vec2(_posX[5], (float)(105 - i * 30)));
            competitionLabels[i] = label;
        }

        button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
        cell->addChild(button);
        button->setScale9Enabled(true);
        button->setContentSize(Size(_colWidth[6] - 8, 20.0f));
        button->setTitleFontSize(12);
        button->setTitleText("登记成绩");
        button->setPosition(Vec2(_posX[6], 60.0f));
        button->addClickEventListener(std::bind(&CompetitionTableScene::onRecordButton, this, std::placeholders::_1));
        Common::scaleLabelToFitWidth(button->getTitleLabel(), _colWidth[6] - 10);
    }

    const CustomCell::ExtDataType ext = cell->getExtData();
    Label *tableLabel = std::get<0>(ext);
    Label *const (&serialLabels)[4] = std::get<1>(ext);
    Label *const (&nameLabels)[4] = std::get<2>(ext);
    Label *const (&standardLabels)[4] = std::get<3>(ext);
    Label *const (&competitionLabels)[4] = std::get<4>(ext);
    ui::Button *button = std::get<5>(ext);
    LayerColor *const (&layerColors)[2] = std::get<6>(ext);

    layerColors[0]->setVisible(!(idx & 1));
    layerColors[1]->setVisible(!!(idx & 1));

    tableLabel->setString(std::to_string(idx + 1));

    for (int i = 0; i < 4; ++i) {
        const CompetitionTable &currentTable = _competitionTables->at(idx);
        const CompetitionPlayer *player = currentTable.players[i];
        if (player == nullptr) {
            serialLabels[i]->setString("");
            nameLabels[i]->setString("");
            standardLabels[i]->setString("");
            competitionLabels[i]->setString("");
        }
        else {
            serialLabels[i]->setString(std::to_string(player->serial));
            nameLabels[i]->setString(player->name);
            std::pair<float, int> ret = player->getCurrentScoresByRound(_currentRound);
            standardLabels[i]->setString(CompetitionResult::getStandardScoreString(ret.first));
            competitionLabels[i]->setString(std::to_string(ret.second));
        }
    }

    button->setUserData(reinterpret_cast<void *>(idx));

    return cell;
}

void CompetitionTableScene::onRecordButton(cocos2d::Ref *sender) {
    ui::Button *button = (ui::Button *)sender;
    size_t table = reinterpret_cast<size_t>(button->getUserData());
    const CompetitionTable &currentTable = _competitionTables->at(table);

    DrawNode *drawNode = DrawNode::create();

    Size visibleSize = Director::getInstance()->getVisibleSize();

    // 列宽
    const float colWidth[5] = {
        visibleSize.width * 0.1f,
        visibleSize.width * 0.2f,
        visibleSize.width * 0.15f,
        visibleSize.width * 0.15f,
        visibleSize.width * 0.15f,
    };

    // 中心位置
    const float posX[5] = {
        colWidth[0] * 0.5f,
        posX[0] + colWidth[0] * 0.5f + colWidth[1] * 0.5f,
        posX[1] + colWidth[1] * 0.5f + colWidth[2] * 0.5f,
        posX[2] + colWidth[2] * 0.5f + colWidth[3] * 0.5f,
        posX[3] + colWidth[3] * 0.5f + colWidth[4] * 0.5f
    };

    const float width = visibleSize.width * 0.75f;
    const float height = 100;
    drawNode->setContentSize(Size(width, height));

    drawNode->drawRect(Vec2(0, 0), Vec2(width, height), Color4F::BLACK);
    for (int i = 0; i < 4; ++i) {
        drawNode->drawLine(Vec2(0, i * 20 + 20), Vec2(width, i * 20 + 20), Color4F::BLACK);
    }


    for (int i = 0; i < 4; ++i) {
        const float x = posX[i] + colWidth[i] * 0.5f;
        drawNode->drawLine(Vec2(x, 0), Vec2(x, 100), Color4F::BLACK);
    }

    const char *titleTexts[] = { "编号", "选手姓名", "顺位", "标准分", "比赛分" };
    for (int i = 0; i < 5; ++i) {
        Label *label = Label::createWithSystemFont(titleTexts[i], "Arail", 12);
        label->setColor(Color3B::BLACK);
        drawNode->addChild(label);
        label->setPosition(Vec2(posX[i], 90.0f));
    }

    for (int i = 0; i < 4; ++i) {
        const CompetitionPlayer *player = currentTable.players[i];
        if (player == nullptr) {
            continue;
        }

        Label *label = Label::createWithSystemFont(std::to_string(player->serial), "Arail", 12);
        label->setColor(Color3B::BLACK);
        drawNode->addChild(label);
        label->setPosition(Vec2(posX[0], 70.0f - 20.0f * i));

        label = Label::createWithSystemFont(player->name, "Arail", 12);
        label->setColor(Color3B::BLACK);
        drawNode->addChild(label);
        label->setPosition(Vec2(posX[1], 70.0f - 20.0f * i));

        for (int k = 0; k < 3; ++k) {
            ui::EditBox *editBox = ui::EditBox::create(Size(colWidth[2 + k], 20.0f), ui::Scale9Sprite::create());
            editBox->setInputFlag(ui::EditBox::InputFlag::SENSITIVE);
            editBox->setInputMode(ui::EditBox::InputMode::NUMERIC);
            editBox->setReturnType(ui::EditBox::KeyboardReturnType::DONE);
            editBox->setTextHorizontalAlignment(TextHAlignment::CENTER);
            editBox->setFontColor(Color4B::BLACK);
            editBox->setFontSize(12);
            editBox->setPlaceHolder(titleTexts[k + 2]);
            drawNode->addChild(editBox);
            editBox->setPosition(Vec2(posX[2 + k], 70.0f - 20.0f * i));
        }
    }

    AlertView::showWithNode(Common::format<128>("第%lu桌成绩", (unsigned long)table + 1), drawNode, nullptr, nullptr);
}

void CompetitionTableScene::rankBySerial() {
    std::vector<CompetitionPlayer> &players = _competitionData->players;
    const size_t cnt = players.size();
    _competitionTables->resize(cnt / 4);
    for (size_t i = 0; i < cnt; i += 4) {
        CompetitionTable &table = _competitionTables->at(i / 4);
        table.players[0] = &players[i];
        table.players[1] = &players[i + 1];
        table.players[2] = &players[i + 2];
        table.players[3] = &players[i + 3];
    }
}

void CompetitionTableScene::rankByRandom() {
    std::vector<CompetitionPlayer> &players = _competitionData->players;
    const size_t cnt = players.size();
    _competitionTables->resize(cnt / 4);

    std::vector<CompetitionPlayer *> temp;
    temp.reserve(players.size());
    std::transform(players.begin(), players.end(), std::back_inserter(temp), [](CompetitionPlayer &p) { return &p; });
    std::random_shuffle(temp.begin(), temp.end());

    for (size_t i = 0; i < cnt; i += 4) {
        CompetitionTable &table = _competitionTables->at(i / 4);
        table.players[0] = temp[i];
        table.players[1] = temp[i + 1];
        table.players[2] = temp[i + 2];
        table.players[3] = temp[i + 3];
    }
}

void CompetitionTableScene::rankByScores() {

}

void CompetitionTableScene::rankBySerialSnake() {
    std::vector<CompetitionPlayer> &players = _competitionData->players;
    const size_t cnt = players.size();
    _competitionTables->resize(cnt / 4);

    size_t east = 0;
    size_t south = players.size() / 2 - 1;
    size_t west = south + 1;
    size_t north = players.size() - 1;
    for (size_t i = 0; i < cnt; i += 4) {
        CompetitionTable &table = _competitionTables->at(i / 4);
        table.players[0] = &players[east++];
        table.players[1] = &players[south--];
        table.players[2] = &players[west++];
        table.players[3] = &players[north--];
    }
}

void CompetitionTableScene::rankByScoresSnake() {

}

void CompetitionTableScene::onRankButton(cocos2d::Ref *sender) {
    Node *rootNode = Node::create();
    rootNode->setContentSize(Size(80, 120));

    ui::RadioButtonGroup *radioGroup = ui::RadioButtonGroup::create();
    rootNode->addChild(radioGroup);

    static const char *titles[] = { "随机", "编号蛇形", "排名蛇形", "高高碰", "自定义" };

    for (int i = 0; i < 5; ++i) {
        ui::RadioButton *radioButton = ui::RadioButton::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
        rootNode->addChild(radioButton);
        radioButton->setZoomScale(0.0f);
        radioButton->ignoreContentAdaptWithSize(false);
        radioButton->setContentSize(Size(20.0f, 20.0f));
        radioButton->setPosition(Vec2(10.0f, 110.0f - i * 25.0f));
        radioGroup->addRadioButton(radioButton);

        Label *label = Label::createWithSystemFont(titles[i], "Arial", 12);
        label->setColor(Color3B::BLACK);
        rootNode->addChild(label);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        label->setPosition(Vec2(25.0f, 110.0f - i * 25.0f));
    }

    AlertView::showWithNode("排列座位", rootNode, [this, radioGroup]() {
        switch (radioGroup->getSelectedButtonIndex()) {
        case 0: rankByRandom(); _tableView->reloadData(); break;
        case 1: rankBySerialSnake(); _tableView->reloadData(); break;
        case 2: break;
        case 3: break;
        case 4: break;
        default: break;
        }
    }, nullptr);
}
