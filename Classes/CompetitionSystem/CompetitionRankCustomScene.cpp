#include "CompetitionRankCustomScene.h"
#include <array>
#include "../cocos-wheels/CWEditBoxDelegate.h"
#include "../widget/AlertView.h"
#include "Competition.h"

USING_NS_CC;

static const char *seatText[] = { "东", "南", "西", "北" };

bool CompetitionRankCustomScene::initWithData(const std::shared_ptr<CompetitionData> &competitionData, size_t currentRound) {
    if (UNLIKELY(!BaseScene::initWithTitle("自定义排座次"))) {
        return false;
    }

    _competitionData = competitionData;
    _currentRound = currentRound;

    _playerFlags.resize(competitionData->players.size());
    _playerIndices.assign(_playerFlags.size(), INVALID_INDEX);
    _tableCount = _playerIndices.size() >> 2;

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    // 列宽
    _colWidth[0] = visibleSize.width * 0.1f;
    _colWidth[1] = visibleSize.width * 0.1f;
    _colWidth[2] = visibleSize.width * 0.1f;
    _colWidth[3] = visibleSize.width * 0.2f;
    _colWidth[4] = visibleSize.width * 0.1f;
    _colWidth[5] = visibleSize.width * 0.1f;
    _colWidth[6] = visibleSize.width * 0.1f;
    _colWidth[7] = visibleSize.width * 0.2f;

    // 中心位置
    _posX[0] = _colWidth[0] * 0.5f;
    _posX[1] = _posX[0] + _colWidth[0] * 0.5f + _colWidth[1] * 0.5f;
    _posX[2] = _posX[1] + _colWidth[1] * 0.5f + _colWidth[2] * 0.5f;
    _posX[3] = _posX[2] + _colWidth[2] * 0.5f + _colWidth[3] * 0.5f;
    _posX[4] = _posX[3] + _colWidth[3] * 0.5f + _colWidth[4] * 0.5f;
    _posX[5] = _posX[4] + _colWidth[4] * 0.5f + _colWidth[5] * 0.5f;
    _posX[6] = _posX[5] + _colWidth[5] * 0.5f + _colWidth[6] * 0.5f;
    _posX[7] = _posX[6] + _colWidth[6] * 0.5f + _colWidth[7] * 0.5f;

    // 表头
    const char *titleTexts[] = { "桌号", "座次", "编号", "选手姓名", "桌号", "座次", "编号", "选手姓名" };
    for (int i = 0; i < 8; ++i) {
        Label *label = Label::createWithSystemFont(titleTexts[i], "Arail", 12);
        label->setColor(Color3B::BLACK);
        this->addChild(label);
        label->setPosition(Vec2(origin.x + _posX[i], visibleSize.height - 70.0f));
    }

    const float tableHeight = visibleSize.height - 110.0f;

    // 表格
    cw::TableView *tableView = cw::TableView::create();
    tableView->setContentSize(Size(visibleSize.width, tableHeight));
    tableView->setDelegate(this);
    tableView->setDirection(ui::ScrollView::Direction::VERTICAL);
    tableView->setVerticalFillOrder(cw::TableView::VerticalFillOrder::TOP_DOWN);

    tableView->setScrollBarPositionFromCorner(Vec2(5, 2));
    tableView->setScrollBarWidth(4);
    tableView->setScrollBarOpacity(0x99);
    tableView->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    tableView->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f - 25.0f));
    tableView->reloadData();
    _tableView = tableView;
    this->addChild(tableView);

    // 表头的线
    DrawNode *drawNode = DrawNode::create();
    this->addChild(drawNode);
    drawNode->setPosition(Vec2(origin.x, visibleSize.height - 80.0f));
    drawNode->drawLine(Vec2(0, 0), Vec2(visibleSize.width, 0), Color4F::BLACK);
    drawNode->drawLine(Vec2(0, 20), Vec2(visibleSize.width, 20), Color4F::BLACK);
    for (int i = 0; i < 7; ++i) {
        const float posX = _posX[i] + _colWidth[i] * 0.5f;
        drawNode->drawLine(Vec2(posX, 0), Vec2(posX, 20), Color4F::BLACK);
    }

    // 当表格可拖动时，画下方一条线
    if (tableView->getInnerContainerSize().height > tableHeight) {
        float posY = -tableHeight;
        drawNode->drawLine(Vec2(0, posY), Vec2(visibleSize.width, posY), Color4F::BLACK);
    }

    // 确定按钮
    _okButton = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png", "source_material/btn_square_disabled.png");
    this->addChild(_okButton);
    _okButton->setScale9Enabled(true);
    _okButton->setContentSize(Size(50.0f, 20.0f));
    _okButton->setTitleFontSize(12);
    _okButton->setTitleText("确定");
    _okButton->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + 15.0f));
    //_okButton->addClickEventListener([](Ref *) { cocos2d::Director::getInstance()->popScene(); });
    //_okButton->setEnabled(false);

    return true;
}

ssize_t CompetitionRankCustomScene::numberOfCellsInTableView(cw::TableView *table) {
    return ((_tableCount >> 1) + (_tableCount & 1));
}

cocos2d::Size CompetitionRankCustomScene::tableCellSizeForIndex(cw::TableView *table, ssize_t idx) {
    return Size(0, 80);
}

cw::TableViewCell *CompetitionRankCustomScene::tableCellAtIndex(cw::TableView *table, ssize_t idx) {
    typedef cw::TableViewCellEx<std::array<ui::Widget *, 2>, std::array<Label *, 2>, std::array<std::array<std::array<Label *, 2>, 4>, 2>, std::array<std::array<ui::Widget *, 4>, 2>, std::array<LayerColor *, 2> > CustomCell;
    CustomCell *cell = (CustomCell *)table->dequeueCell();

    if (cell == nullptr) {
        cell = CustomCell::create();

        CustomCell::ExtDataType &ext = cell->getExtData();
        std::array<ui::Widget *, 2> &rootWidgets = std::get<0>(ext);
        std::array<Label *, 2> &tableLabels = std::get<1>(ext);
        std::array<std::array<std::array<Label *, 2>, 4>, 2> &labels = std::get<2>(ext);
        std::array<std::array<ui::Widget *, 4>, 2> &touchedWidgets = std::get<3>(ext);
        std::array<LayerColor *, 2> &layerColors = std::get<4>(ext);

        Size visibleSize = Director::getInstance()->getVisibleSize();

        // 背景色
        layerColors[0] = LayerColor::create(Color4B(0xC0, 0xC0, 0xC0, 0x10), visibleSize.width, 79);
        cell->addChild(layerColors[0]);

        layerColors[1] = LayerColor::create(Color4B(0x10, 0x10, 0x10, 0x10), visibleSize.width, 79);
        cell->addChild(layerColors[1]);

        DrawNode *drawNodes[2];
        const float halfWidth = visibleSize.width * 0.5f;
        for (int n = 0; n < 2; ++n) {
            ui::Widget *rootWidget = ui::Widget::create();
            cell->addChild(rootWidget);
            rootWidget->setPosition(Vec2(halfWidth * n, 0));
            rootWidgets[n] = rootWidget;

            // 桌号
            Label *label = Label::createWithSystemFont("", "Arail", 12);
            label->setColor(Color3B::BLACK);
            rootWidget->addChild(label);
            label->setPosition(Vec2(_posX[0], 40.0f));
            tableLabels[n] = label;

            // 座次
            for (int i = 0; i < 4; ++i) {
                const float posY = static_cast<float>(70 - i * 20);

                Label *label = Label::createWithSystemFont(seatText[i], "Arail", 12);
                label->setColor(Color3B::BLACK);
                rootWidget->addChild(label);
                label->setPosition(Vec2(_posX[1], posY));
            }

            // 编号、选手姓名
            Color3B textColor[] = { Color3B(0x60, 0x60, 0x60), Color3B::ORANGE, Color3B(254, 87, 110), Color3B(44, 121, 178) };
            for (int i = 0; i < 4; ++i) {
                const float posY = static_cast<float>(70 - i * 20);

                for (int k = 0; k < 2; ++k) {
                    Label *label = Label::createWithSystemFont("", "Arail", 12);
                    label->setColor(textColor[k]);
                    rootWidget->addChild(label);
                    label->setPosition(Vec2(_posX[2 + k], posY));
                    labels[n][i][k] = label;
                }

                // 按钮
                ui::Widget *widget = ui::Widget::create();
                widget->setTouchEnabled(true);
                widget->setPosition(Vec2(_posX[3], posY));
                widget->setContentSize(Size(_colWidth[3], 20));
                rootWidget->addChild(widget);
                widget->addClickEventListener(std::bind(&CompetitionRankCustomScene::onNameWidget, this, std::placeholders::_1));
                touchedWidgets[n][i] = widget;
            }

            // 画线
            DrawNode *drawNode = DrawNode::create();
            rootWidget->addChild(drawNode);
            drawNode->drawLine(Vec2(0, 0), Vec2(halfWidth, 0), Color4F::BLACK);
            drawNode->drawLine(Vec2(0, 80), Vec2(halfWidth, 80), Color4F::BLACK);
            const float posX = halfWidth * (n + 1);
            for (int i = 0; i < 3; ++i) {
                const float posY = 20.0f * (i + 1);
                drawNode->drawLine(Vec2(_colWidth[n * 4], posY), Vec2(posX, posY), Color4F::BLACK);
            }
            for (int i = 0; i < 3; ++i) {
                const float posX = _posX[i] + _colWidth[i] * 0.5f;
                drawNode->drawLine(Vec2(posX, 0), Vec2(posX, 80), Color4F::BLACK);
            }
            drawNodes[n] = drawNode;
        }
        drawNodes[0]->drawLine(Vec2(halfWidth, 0), Vec2(halfWidth, 80), Color4F::BLACK);
    }

    const CustomCell::ExtDataType ext = cell->getExtData();
    const std::array<ui::Widget *, 2> &rootWidgets = std::get<0>(ext);
    const std::array<Label *, 2> &tableLabels = std::get<1>(ext);
    const std::array<std::array<std::array<Label *, 2>, 4>, 2> &labels = std::get<2>(ext);
    const std::array<std::array<ui::Widget *, 4>, 2> &touchedWidgets = std::get<3>(ext);
    const std::array<LayerColor *, 2> &layerColors = std::get<4>(ext);

    layerColors[0]->setVisible(!(idx & 1));
    layerColors[1]->setVisible(!!(idx & 1));

    const std::vector<CompetitionPlayer> &players = _competitionData->players;
    ssize_t realTableIdx = idx << 1;
    for (ssize_t n = 0; n < 2; ++n) {
        if (realTableIdx + n >= _tableCount) {
            rootWidgets[n]->setVisible(false);
        }
        else {
            rootWidgets[n]->setVisible(true);

            tableLabels[n]->setString(std::to_string(realTableIdx + n + 1));

            // 编号、选手姓名
            for (ssize_t i = 0; i < 4; ++i) {
                ssize_t realIndex = ((realTableIdx + n) * 4) + i;
                ptrdiff_t playerIndex = _playerIndices[realIndex];
                if (playerIndex == INVALID_INDEX) {
                    labels[n][i][0]->setString("");
                    labels[n][i][1]->setString("选择");
                    labels[n][i][1]->setColor(Color3B::GRAY);
                }
                else {
                    const CompetitionPlayer &player = players[playerIndex];
                    labels[n][i][0]->setString(std::to_string(player.serial + 1));
                    labels[n][i][1]->setString(player.name);
                    labels[n][i][1]->setColor(Color3B::ORANGE);
                }

                for (int k = 0; k < 2; ++k) {
                    Common::scaleLabelToFitWidth(labels[n][i][k], _colWidth[2 + k]);
                }

                touchedWidgets[n][i]->setUserData(reinterpret_cast<void *>(realIndex));
            }
        }
    }

    return cell;
}

void CompetitionRankCustomScene::onNameWidget(cocos2d::Ref *sender) {
    ui::Widget *widget = (ui::Widget *)sender;
    ssize_t realIndex = reinterpret_cast<ssize_t>(widget->getUserData());

    ptrdiff_t playerIndex = _playerIndices[realIndex];
    if (playerIndex == INVALID_INDEX) {
        showSelectPlayerAlert(realIndex);
    }
    else {
        std::string title = Common::format("%" PRIS "桌%s位", (realIndex >> 2) + 1, seatText[realIndex & 3]);
        AlertView::showWithMessage(title, "该座位已经有人了，是否重新选择", 12, [this, playerIndex, realIndex]() {
            _playerIndices[realIndex] = INVALID_INDEX;
            _playerFlags[playerIndex] = false;
            _tableView->updateCellAtIndex(realIndex >> 3);

            showSelectPlayerAlert(realIndex);
        }, nullptr);
    }
}

void CompetitionRankCustomScene::showSelectPlayerAlert(ssize_t realIndex) {
    class AlertInnerNode : public Node, cw::TableViewDelegate {
    private:
        const std::vector<CompetitionPlayer> *_players = nullptr;
        const std::vector<uint8_t> *_playerFlags = nullptr;
        const std::vector<ptrdiff_t> *_playerIndices = nullptr;

        std::vector<uint8_t> _currentFlags;

    public:
        const std::vector<uint8_t> &getCurrentFlags() { return _currentFlags; }

        CREATE_FUNC_WITH_PARAM_4(AlertInnerNode, initWithPlayers, const std::vector<CompetitionPlayer> *, players, const std::vector<uint8_t> *, playerFlags, const std::vector<ptrdiff_t> *, playerIndices, ssize_t, realIndex);
        bool initWithPlayers(const std::vector<CompetitionPlayer> *players, const std::vector<uint8_t> *playerFlags, const std::vector<ptrdiff_t> *playerIndices, ssize_t realIndex) {
            if (UNLIKELY(!Node::init())) {
                return false;
            }

            _players = players;
            _playerFlags = playerFlags;
            _playerIndices = playerIndices;

            _currentFlags.resize(playerFlags->size());

            Size visibleSize = Director::getInstance()->getVisibleSize();
            const float width = visibleSize.width * 0.8f - 10;
            const float height = visibleSize.height * 0.8f - 80;

            this->setContentSize(Size(width, height));

            // 表格
            cw::TableView *tableView = cw::TableView::create();
            tableView->setContentSize(Size(width, height));
            tableView->setDelegate(this);
            tableView->setDirection(ui::ScrollView::Direction::VERTICAL);
            tableView->setVerticalFillOrder(cw::TableView::VerticalFillOrder::TOP_DOWN);

            tableView->setScrollBarPositionFromCorner(Vec2(5, 2));
            tableView->setScrollBarWidth(4);
            tableView->setScrollBarOpacity(0x99);
            tableView->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
            tableView->setPosition(Vec2(width * 0.5f, height * 0.5f));
            tableView->reloadData();
            this->addChild(tableView);

            return true;
        }

        virtual ssize_t numberOfCellsInTableView(cw::TableView *table) override {
            return _playerFlags->size();
        }

        virtual cocos2d::Size tableCellSizeForIndex(cw::TableView *table, ssize_t idx) override {
            return Size(0, 30);
        }

        virtual cw::TableViewCell *tableCellAtIndex(cw::TableView *table, ssize_t idx) override {
            typedef cw::TableViewCellEx<ui::CheckBox *, std::array<Label *, 2>, std::array<LayerColor *, 2> > CustomCell;
            CustomCell *cell = (CustomCell *)table->dequeueCell();

            Size visibleSize = Director::getInstance()->getVisibleSize();
            const float cellWidth = visibleSize.width * 0.8f - 10;

            if (cell == nullptr) {
                cell = CustomCell::create();

                CustomCell::ExtDataType &ext = cell->getExtData();
                ui::CheckBox *&checkBox = std::get<0>(ext);
                std::array<Label *, 2> &labels = std::get<1>(ext);
                std::array<LayerColor *, 2> &layerColors = std::get<2>(ext);

                // 背景色
                layerColors[0] = LayerColor::create(Color4B(0xC0, 0xC0, 0xC0, 0x10), cellWidth, 29);
                cell->addChild(layerColors[0]);

                layerColors[1] = LayerColor::create(Color4B(0x10, 0x10, 0x10, 0x10), cellWidth, 29);
                cell->addChild(layerColors[1]);

                // 选择框
                checkBox = ui::CheckBox::create("source_material/btn_square_normal.png", "", "source_material/btn_square_highlighted.png", "source_material/btn_square_disabled.png", "source_material/btn_square_disabled.png");
                cell->addChild(checkBox);
                checkBox->setZoomScale(0.0f);
                checkBox->ignoreContentAdaptWithSize(false);
                checkBox->setContentSize(Size(20.0f, 20.0f));
                checkBox->setPosition(Vec2(15.0f, 15.0f));
                checkBox->addEventListener(std::bind(&AlertInnerNode::onCheckBox, this, std::placeholders::_1, std::placeholders::_2));

                // 编号、名字
                const Color3B textColors[] = { Color3B::BLACK, Color3B::ORANGE };
                const float posX[] = { 40, (cellWidth - 55) * 0.5f };
                for (int i = 0; i < 2; ++i) {
                    Label *label = Label::createWithSystemFont("", "Arail", 12);
                    label->setColor(textColors[i]);
                    cell->addChild(label);
                    label->setPosition(Vec2(posX[i], 15.0f));
                    labels[i] = label;
                }
            }

            const CustomCell::ExtDataType &ext = cell->getExtData();
            ui::CheckBox *checkBox = std::get<0>(ext);
            const std::array<Label *, 2> &labels = std::get<1>(ext);
            const std::array<LayerColor *, 2> &layerColors = std::get<2>(ext);

            layerColors[0]->setVisible(!(idx & 1));
            layerColors[1]->setVisible(!!(idx & 1));

            const CompetitionPlayer &player = _players->at(idx);
            labels[0]->setString(std::to_string(player.serial + 1));
            Common::scaleLabelToFitWidth(labels[0], 18);
            labels[1]->setString(player.name);
            Common::scaleLabelToFitWidth(labels[1], (cellWidth - 55) * 0.5f - 4);

            checkBox->setUserData(reinterpret_cast<void *>(idx));
            checkBox->setEnabled(!_playerFlags->at(idx));
            checkBox->setSelected(!!_currentFlags[idx]);

            return cell;
        }

        void onCheckBox(cocos2d::Ref *sender, cocos2d::ui::CheckBox::EventType event) {
            ui::CheckBox *checkBox = (ui::CheckBox *)sender;
            ssize_t idx = reinterpret_cast<ssize_t>(checkBox->getUserData());
            _currentFlags[idx] = !!(event == ui::CheckBox::EventType::SELECTED);
        }
    };

    // vs2013需要用this->
    AlertInnerNode *innerNode = AlertInnerNode::create(&_competitionData->players, &this->_playerFlags, &this->_playerIndices, realIndex);
    std::string title = Common::format("%" PRIS "桌%s位", (realIndex >> 2) + 1, seatText[realIndex & 3]);
    AlertView::showWithNode(title, innerNode, [this, innerNode, realIndex]() {
        const std::vector<uint8_t> &currentFlags = innerNode->getCurrentFlags();
        std::vector<size_t> selected;
        selected.reserve(8);
        for (size_t i = 0, cnt = currentFlags.size(); i < cnt; ++i) {
            if (currentFlags[i]) {
                selected.push_back(i);
            }
        }

        if (selected.size() == 1) {
            size_t idx = selected[0];

            _playerIndices[realIndex] = idx;
            _playerFlags[idx] = true;
        }
        _tableView->updateCellAtIndex(realIndex >> 3);
    }, nullptr);
}
