#include "CompetitionTableScene.h"
#include <array>
#include "../UICommon.h"
#include "../widget/AlertDialog.h"
#include "../widget/Toast.h"
#include "Competition.h"
#include "CompetitionRankCustomScene.h"
#include "EditBoxDelegateWrapper.hpp"

USING_NS_CC;

// in Record.cpp
extern "C++" void CalculateRankFromScore(const int (&scores)[4], unsigned (&ranks)[4]);
extern "C++" void RankToStandardScore(const unsigned (&ranks)[4], unsigned (&ss12)[4]);

static const char *seatText[] = { __UTF8("东"), __UTF8("南"), __UTF8("西"), __UTF8("北") };

bool CompetitionTableScene::init(const std::shared_ptr<CompetitionData> &competitionData, size_t currentRound) {
    if (UNLIKELY(!BaseScene::initWithTitle(Common::format(__UTF8("%s第%") __UTF8(PRIzu) __UTF8("/%") __UTF8(PRIzu) __UTF8("轮"),
        competitionData->name.c_str(), currentRound + 1, competitionData->round_count)))) {
        return false;
    }

    _competitionData = competitionData;
    _currentRound = currentRound;
    _competitionTables = &_competitionData->rounds[currentRound].tables;

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    // 列宽
    _colWidth[0] = visibleSize.width * 0.1f;
    _colWidth[1] = visibleSize.width * 0.1f;
    _colWidth[2] = visibleSize.width * 0.1f;
    _colWidth[3] = visibleSize.width * 0.2f;
    _colWidth[4] = visibleSize.width * 0.15f;
    _colWidth[5] = visibleSize.width * 0.15f;
    _colWidth[6] = visibleSize.width * 0.2f;

    // 中心位置
    cw::calculateColumnsCenterX(_colWidth, 7, _posX);

    // 表头
    static const char *titleTexts[] = { __UTF8("桌号"), __UTF8("座次"), __UTF8("编号"), __UTF8("选手姓名"), __UTF8("标准分"), __UTF8("比赛分") };
    for (int i = 0; i < 6; ++i) {
        Label *label = Label::createWithSystemFont(titleTexts[i], "Arail", 12);
        label->setColor(Color3B::BLACK);
        this->addChild(label);
        label->setPosition(Vec2(origin.x + _posX[i], visibleSize.height - 45.0f));
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
    tableView->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f - 12.5f));
    tableView->reloadData();
    _tableView = tableView;
    this->addChild(tableView);

    // 表头的线
    DrawNode *drawNode = DrawNode::create();
    this->addChild(drawNode);
    drawNode->setPosition(Vec2(origin.x, visibleSize.height - 55.0f));
    drawNode->drawLine(Vec2(0.0f, 0.0f), Vec2(visibleSize.width, 0.0f), Color4F::BLACK);
    drawNode->drawLine(Vec2(0.0f, 20.0f), Vec2(visibleSize.width, 20.0f), Color4F::BLACK);
    for (int i = 0; i < 7; ++i) {
        const float posX = _posX[i] + _colWidth[i] * 0.5f;
        drawNode->drawLine(Vec2(posX, 0.0f), Vec2(posX, 20.0f), Color4F::BLACK);
    }

    // 当表格可拖动时，画下方一条线
    if (tableView->getInnerContainerSize().height > tableHeight) {
        float posY = -tableHeight;
        drawNode->drawLine(Vec2(0.0f, posY), Vec2(visibleSize.width, posY), Color4F::BLACK);
    }
    tableView->setOnEnterCallback(std::bind(&cw::TableView::reloadDataInplacement, tableView));

    // 排列座位按钮
    ui::Button *button = UICommon::createButton();
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(55.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText(__UTF8("排列座位"));
    button->setPosition(Vec2(origin.x + visibleSize.width * 0.25f, origin.y + 15.0f));
    button->addClickEventListener([this](Ref *) { showArrangeAlert(); });

    // 提交按钮
    button = UICommon::createButton();
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(50.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText(__UTF8("提交"));
    button->setPosition(Vec2(origin.x + visibleSize.width * 0.75f, origin.y + 15.0f));
    button->addClickEventListener([](Ref *) { Director::getInstance()->popScene(); });
    button->setEnabled(_competitionData->isRoundFinished(_currentRound));
    _submitButton = button;

    // 有空位
    if (std::any_of(_competitionTables->begin(), _competitionTables->end(), [](const CompetitionTable &table) {
        return std::any_of(std::begin(table.player_indices), std::end(table.player_indices), [](ptrdiff_t idx) { return idx == INVALID_INDEX; });
    })) {
        this->scheduleOnce([this](float) { showArrangeAlert(); }, 0.0f, "show_rank_alert");
    }

    return true;
}

ssize_t CompetitionTableScene::numberOfCellsInTableView(cw::TableView *) {
    return _competitionTables->size();
}

float CompetitionTableScene::tableCellSizeForIndex(cw::TableView *, ssize_t) {
    return 80.0f;
}

cw::TableViewCell *CompetitionTableScene::tableCellAtIndex(cw::TableView *table, ssize_t idx) {
    typedef cw::TableViewCellEx<Label *, std::array<std::array<Label *, 4>, 4>, std::array<ui::Button *, 2>, std::array<LayerColor *, 2> > CustomCell;
    CustomCell *cell = (CustomCell *)table->dequeueCell();

    if (cell == nullptr) {
        cell = CustomCell::create();

        CustomCell::ExtDataType &ext = cell->getExtData();
        Label *&tableLabel = std::get<0>(ext);
        std::array<std::array<Label *, 4>, 4> &labels = std::get<1>(ext);
        ui::Button **buttons = std::get<2>(ext).data();
        LayerColor **layerColors = std::get<3>(ext).data();

        Size visibleSize = Director::getInstance()->getVisibleSize();

        // 背景色
        layerColors[0] = LayerColor::create(Color4B(0x10, 0x10, 0x10, 0x10), visibleSize.width, 80.0f);
        cell->addChild(layerColors[0]);

        layerColors[1] = LayerColor::create(Color4B(0xC0, 0xC0, 0xC0, 0x10), visibleSize.width, 80.0f);
        cell->addChild(layerColors[1]);

        // 桌号
        tableLabel = Label::createWithSystemFont("", "Arail", 12);
        tableLabel->setColor(Color3B::BLACK);
        cell->addChild(tableLabel);
        tableLabel->setPosition(Vec2(_posX[0], 40.0f));

        // 座次
        for (int i = 0; i < 4; ++i) {
            const float posY = static_cast<float>(70 - i * 20);

            Label *label = Label::createWithSystemFont(seatText[i], "Arail", 12);
            label->setColor(Color3B::BLACK);
            cell->addChild(label);
            label->setPosition(Vec2(_posX[1], posY));
        }

        // 编号、选手姓名、标准分、比赛分，共4个Label
        Color3B textColor[] = { Color3B(0x60, 0x60, 0x60), Color3B::ORANGE, Color3B(254, 87, 110), Color3B(44, 121, 178) };
        for (int i = 0; i < 4; ++i) {
            const float posY = static_cast<float>(70 - i * 20);

            for (int k = 0; k < 4; ++k) {
                Label *label = Label::createWithSystemFont("", "Arail", 12);
                label->setColor(textColor[k]);
                cell->addChild(label);
                label->setPosition(Vec2(_posX[2 + k], posY));
                labels[i][k] = label;
            }
        }

        // 输入按钮
        const float buttonWidth = std::min(_colWidth[6] - 8.0f, 35.0f);
        ui::Button *button = UICommon::createButton();
        cell->addChild(button);
        button->setScale9Enabled(true);
        button->setContentSize(Size(buttonWidth, 20.0f));
        button->setTitleFontSize(12);
        button->setTitleText(__UTF8("输入"));
        button->setPosition(Vec2(_posX[6], 55.0f));
        button->addClickEventListener(std::bind(&CompetitionTableScene::onRecordButton, this, std::placeholders::_1));
        cw::scaleLabelToFitWidth(button->getTitleLabel(), _colWidth[5] - 10.0f);
        buttons[0] = button;

        // 清空按钮
        button = UICommon::createButton();
        cell->addChild(button);
        button->setScale9Enabled(true);
        button->setContentSize(Size(buttonWidth, 20.0f));
        button->setTitleFontSize(12);
        button->setTitleText(__UTF8("清空"));
        button->setPosition(Vec2(_posX[6], 25.0f));
        button->addClickEventListener(std::bind(&CompetitionTableScene::onClearButton, this, std::placeholders::_1));
        cw::scaleLabelToFitWidth(button->getTitleLabel(), _colWidth[5] - 10.0f);
        buttons[1] = button;

        // 画线
        DrawNode *drawNode = DrawNode::create();
        cell->addChild(drawNode);
        drawNode->drawLine(Vec2(0.0f, 0.0f), Vec2(visibleSize.width, 0.0f), Color4F::BLACK);
        drawNode->drawLine(Vec2(0.0f, 80.0f), Vec2(visibleSize.width, 80.0f), Color4F::BLACK);
        const float posX = visibleSize.width - _colWidth[6];
        for (int i = 0; i < 3; ++i) {
            const float posY = 20.0f * (i + 1);
            drawNode->drawLine(Vec2(_colWidth[0], posY), Vec2(posX, posY), Color4F::BLACK);
        }
        for (int i = 0; i < 6; ++i) {
            const float posX1 = _posX[i] + _colWidth[i] * 0.5f;
            drawNode->drawLine(Vec2(posX1, 0.0f), Vec2(posX1, 80.0f), Color4F::BLACK);
        }
    }

    const CustomCell::ExtDataType &ext = cell->getExtData();
    Label *tableLabel = std::get<0>(ext);
    const std::array<std::array<Label *, 4>, 4> &labels = std::get<1>(ext);
    ui::Button *const *buttons = std::get<2>(ext).data();
    LayerColor *const *layerColors = std::get<3>(ext).data();

    layerColors[0]->setVisible((idx & 1) == 0);
    layerColors[1]->setVisible((idx & 1) != 0);

    const CompetitionTable &currentTable = _competitionTables->at(idx);
    tableLabel->setString(std::to_string(currentTable.serial + 1));  // 桌号

    const std::vector<CompetitionPlayer> &players = _competitionData->players;

    // 编号、选手姓名、标准分、比赛分，共4个Label
    for (int i = 0; i < 4; ++i) {
        ptrdiff_t playerIndex = currentTable.player_indices[i];
        if (playerIndex == INVALID_INDEX) {
            for (int k = 0; k < 4; ++k) {
                labels[i][k]->setString("");
            }
        }
        else {
            const CompetitionPlayer *player = &players[currentTable.player_indices[i]];
            labels[i][0]->setString(std::to_string(player->serial + 1));
            labels[i][1]->setString(player->name);
            std::pair<float, int> ret = player->getCurrentScoresByRound(_currentRound);
            labels[i][2]->setString(CompetitionResult::standardScoreToString(ret.first));
            labels[i][3]->setString(std::to_string(ret.second));

            for (int k = 0; k < 4; ++k) {
                cw::scaleLabelToFitWidth(labels[i][k], _colWidth[2 + k] - 4.0f);
            }
        }
    }

    buttons[0]->setUserData(reinterpret_cast<void *>(idx));
    buttons[1]->setUserData(reinterpret_cast<void *>(idx));

    return cell;
}

void CompetitionTableScene::onClearButton(cocos2d::Ref *sender) {
    ui::Button *button = (ui::Button *)sender;
    size_t table = reinterpret_cast<size_t>(button->getUserData());
    const CompetitionTable &currentTable = _competitionTables->at(table);
    std::vector<CompetitionPlayer> &players = _competitionData->players;
    bool updateFlag = false;
    for (int i = 0; i < 4; ++i) {
        ptrdiff_t idx = currentTable.player_indices[i];
        if (idx == INVALID_INDEX) {
            continue;
        }

        CompetitionResult &result = players[idx].competition_results[_currentRound];
        result.rank = 0;
        result.standard_score12 = 0;
        result.competition_score = 0;
        updateFlag = true;
    }

    if (updateFlag) {
        // 刷新外面的UI
        _submitButton->setEnabled(_competitionData->isRoundFinished(_currentRound));
        _tableView->updateCellAtIndex(table);
        _competitionData->writeToFile();
    }
}

void CompetitionTableScene::onRecordButton(cocos2d::Ref *sender) {
    ui::Button *button = (ui::Button *)sender;
    size_t table = reinterpret_cast<size_t>(button->getUserData());
    const CompetitionTable &currentTable = _competitionTables->at(table);

    // 有空位
    if (std::any_of(std::begin(currentTable.player_indices), std::end(currentTable.player_indices),
        [](ptrdiff_t idx) { return idx == INVALID_INDEX; })) {
        AlertDialog::Builder(this)
            .setTitle(__UTF8("登记成绩"))
            .setMessage(__UTF8("请先排座位"))
            .setNegativeButton(__UTF8("取消"), nullptr)
            .setPositiveButton(__UTF8("确定"), [this](AlertDialog *, int) { showArrangeAlert(); return true; })
            .create()->show();
        return;
    }

    CompetitionResult result[4];
    const std::vector<CompetitionPlayer> &players = _competitionData->players;
    for (int i = 0; i < 4; ++i) {
        result[i] = players[currentTable.player_indices[i]].competition_results[_currentRound];
    };

    showRecordAlert(table, result);
}

namespace {
    class AlertInnerNode : public Node, ui::EditBoxDelegate {
    private:
        CompetitionResult _results[4];

        float _colWidth[5];
        float _posX[5];

        Label *_checkLabel = nullptr;
        ui::EditBox *_editBoxes[4][2];

    public:
        const CompetitionResult (&getResults())[4] { return _results; }

        typedef CompetitionResult CompetitionResultArray[4];
        CREATE_FUNC_WITH_PARAM_3(AlertInnerNode, const CompetitionResultArray &, prevResults,
            const std::vector<CompetitionPlayer> &, players, const CompetitionTable &, currentTable);

        bool init(const CompetitionResult (&prevResults)[4], const std::vector<CompetitionPlayer> &players, const CompetitionTable &currentTable) {
            if (UNLIKELY(!Node::init())) {
                return false;
            }

            memcpy(_results, prevResults, sizeof(_results));

            const float width = AlertDialog::maxWidth();
            const float height = 100;

            // 列宽
            _colWidth[0] = width * 0.15f;
            _colWidth[1] = width * 0.15f;
            _colWidth[2] = width * 0.3f;
            _colWidth[3] = width * 0.2f;
            _colWidth[4] = width * 0.2f;

            // 中心位置
            cw::calculateColumnsCenterX(_colWidth, 5, _posX);

            this->setContentSize(Size(width, height + 55.0f));

            // 表格画在DrawNode上
            DrawNode *drawNode = DrawNode::create();
            this->addChild(drawNode);
            drawNode->setPosition(Vec2(0.0f, 55.0f));

            // 横线
            for (int i = 0; i < 6; ++i) {
                const float y = static_cast<float>(i * 20);
                drawNode->drawLine(Vec2(0.0f, y), Vec2(width, y), Color4F::BLACK);
            }

            // 竖线
            drawNode->drawLine(Vec2(0.0f, 0.0f), Vec2(0.0f, 100.0f), Color4F::BLACK);
            for (int i = 0; i < 5; ++i) {
                const float x = _posX[i] + _colWidth[i] * 0.5f;
                drawNode->drawLine(Vec2(x, 0.0f), Vec2(x, 100.0f), Color4F::BLACK);
            }

            // 检查
            Label *label = Label::createWithSystemFont(__UTF8("检查：标准分总和7，比赛分总和0"), "Arail", 10);
            label->setColor(Color3B(254, 87, 110));
            this->addChild(label);
            label->setPosition(Vec2(width * 0.5f, 45.0f));
            cw::scaleLabelToFitWidth(label, width - 4.0f);
            _checkLabel = label;

            static const char *titleTexts[] = { __UTF8("座次"), __UTF8("编号"), __UTF8("选手姓名"), __UTF8("标准分"), __UTF8("比赛分") };
            for (int i = 0; i < 5; ++i) {
                label = Label::createWithSystemFont(titleTexts[i], "Arail", 12);
                label->setColor(Color3B::BLACK);
                drawNode->addChild(label);
                label->setPosition(Vec2(_posX[i], 90.0f));
                cw::scaleLabelToFitWidth(label, _colWidth[i] - 4.0f);
            }

            Color3B textColors[] = { Color3B::BLACK, Color3B(0x60, 0x60, 0x60), Color3B::ORANGE,
                Color3B(254, 87, 110), Color3B(44, 121, 178) };
            for (int i = 0; i < 4; ++i) {
                const CompetitionPlayer *player = &players[currentTable.player_indices[i]];

                const float posY = 70.0f - 20.0f * i;
                std::string text[5] = { seatText[i], std::to_string(player->serial + 1), player->name,
                    CompetitionResult::standardScoreToString(_results[i].standard_score12), std::to_string(_results[i].competition_score)
                };

                for (int k = 0; k < 3; ++k) {
                    label = Label::createWithSystemFont(text[k], "Arail", 12);
                    label->setColor(textColors[k]);
                    drawNode->addChild(label);
                    label->setPosition(Vec2(_posX[k], posY));
                    cw::scaleLabelToFitWidth(label, _colWidth[k] - 4.0f);
                }

                for (int k = 0; k < 2; ++k) {
                    ui::EditBox *editBox = ui::EditBox::create(Size(_colWidth[k + 3], 20.0f), ui::Scale9Sprite::create());
                    drawNode->addChild(editBox);
                    editBox->setInputFlag(ui::EditBox::InputFlag::SENSITIVE);
                    editBox->setInputMode(ui::EditBox::InputMode::SINGLE_LINE);
                    editBox->setReturnType(ui::EditBox::KeyboardReturnType::DONE);
                    editBox->setFontColor(textColors[k + 3]);
                    editBox->setFontSize(12);
                    editBox->setTextHorizontalAlignment(TextHAlignment::CENTER);
                    editBox->setText(text[k + 3].c_str());
                    editBox->setMaxLength(6);
                    editBox->setPosition(Vec2(_posX[k + 3], posY));
                    editBox->setTag(i << 4 | k);
                    editBox->setDelegate(this);
                    _editBoxes[i][k] = editBox;
                }
            }

            // 自动计算按钮
            ui::Button *button = UICommon::createButton();
            this->addChild(button);
            button->setScale9Enabled(true);
            button->setContentSize(Size(55.0f, 20.0f));
            button->setTitleFontSize(12);
            button->setTitleText(__UTF8("自动计算"));
            button->setPosition(Vec2(width * 0.5f, 25.0f));
            button->addClickEventListener([this](Ref *) {
                calculateRanks();
                calculateStandardScores();
            });

            // 说明文本
            label = Label::createWithSystemFont(__UTF8("如采用4210标准分赛制，可使用「自动计算」"), "Arail", 10);
            label->setColor(Color3B(0x60, 0x60, 0x60));
            this->addChild(label);
            label->setPosition(Vec2(width * 0.5f, 5.0f));
            cw::scaleLabelToFitWidth(label, width);

            refreshCheckLabel();

            return true;
        }

        void calculateRanks() {
            int scores[4] = {
                _results[0].competition_score,
                _results[1].competition_score,
                _results[2].competition_score,
                _results[3].competition_score
            };
            unsigned ranks[4];
            CalculateRankFromScore(scores, ranks);

            for (int i = 0; i < 4; ++i) {
                _results[i].rank = ranks[i] + 1;
            }
        }

    private:
        virtual void editBoxReturn(ui::EditBox *editBox) override {
            if (Common::isCStringEmpty(editBox->getText())) {
                editBox->setText("0");
            }

            int tag = editBox->getTag();
            int i = tag >> 4, k = tag & 0xF;

            switch (k) {
            case 0: _results[i].standard_score12 = static_cast<unsigned>(round(atof(editBox->getText()) * 12)); break;
            case 1: _results[i].competition_score = atoi(editBox->getText()); break;
            default: break;
            }
            refreshCheckLabel();
        }

        void refreshCheckLabel() {
            float ss = 0.0f;
            int cs = 0;
            for (int i = 0; i < 4; ++i) {
                ss += _results[i].standard_score12;
                cs += _results[i].competition_score;
            }
            _checkLabel->setString(Common::format(__UTF8("检查：标准分总和%s，比赛分总和%d"), CompetitionResult::standardScoreToString(ss).c_str(), cs));
        }

        void calculateStandardScores() {
            unsigned ranks[4] = { _results[0].rank - 1, _results[1].rank - 1, _results[2].rank - 1, _results[3].rank - 1 };
            unsigned ss12[4];
            RankToStandardScore(ranks, ss12);

            for (int i = 0; i < 4; ++i) {
                CompetitionResult &result = _results[i];
                result.standard_score12 = ss12[i];
                _editBoxes[i][0]->setText(CompetitionResult::standardScoreToString(ss12[i]).c_str());
            }

            refreshCheckLabel();
        }
    };
}

void CompetitionTableScene::showRecordAlert(size_t table, const CompetitionResult (&prevResult)[4]) {
    AlertInnerNode *rootNode = AlertInnerNode::create(prevResult,
        _competitionData->players, _competitionTables->at(table));

    AlertDialog::Builder(this)
        .setTitle(Common::format(__UTF8("第%") __UTF8(PRIzu) __UTF8("桌成绩"), table + 1))
        .setContentNode(rootNode)
        .setNegativeButton(__UTF8("取消"), nullptr)
        .setPositiveButton(__UTF8("确定"), [this, table, rootNode](AlertDialog *, int) {
        rootNode->calculateRanks();
        const CompetitionResult (*results)[4] = &rootNode->getResults();

        // 如果有顺位为0，则提示重新输入
        if (std::any_of(std::begin(*results), std::end(*results),
            [](const CompetitionResult &result) { return result.rank == 0; })) {
            Toast::makeText(this, __UTF8("数据错误，请重新输入"), Toast::LENGTH_LONG)->show();
            return false;
        }

        // 更新数据
        std::vector<CompetitionPlayer> &players = _competitionData->players;
        CompetitionTable &currentTable = _competitionTables->at(table);
        for (int i = 0; i < 4; ++i) {
            players[currentTable.player_indices[i]].competition_results[_currentRound] = (*results)[i];
        }

        // 刷新外面的UI
        _submitButton->setEnabled(_competitionData->isRoundFinished(_currentRound));
        _tableView->updateCellAtIndex(table);
        _competitionData->writeToFile();
        return true;
    }).create()->show();
}

void CompetitionTableScene::showArrangeAlert() {
    if (_competitionData->isRoundStarted(_currentRound)) {
        Toast::makeText(this, __UTF8("开始记录成绩后不允许重新排座位"), Toast::LENGTH_LONG)->show();
        return;
    }

    Node *rootNode = Node::create();
    rootNode->setContentSize(Size(80.0f, 95.0f));

    ui::RadioButtonGroup *radioGroup = ui::RadioButtonGroup::create();
    rootNode->addChild(radioGroup);

    static const char *titles[] = { __UTF8("随机"), __UTF8("蛇形"), __UTF8("高高碰"), __UTF8("自定义") };

    for (int i = 0; i < 4; ++i) {
        const float yPos = 85.0f - i * 25.0f;

        ui::RadioButton *radioButton = UICommon::createRadioButton();
        rootNode->addChild(radioButton);
        radioButton->setZoomScale(0.0f);
        radioButton->ignoreContentAdaptWithSize(false);
        radioButton->setContentSize(Size(20.0f, 20.0f));
        radioButton->setPosition(Vec2(10.0f, yPos));
        radioGroup->addRadioButton(radioButton);

        Label *label = Label::createWithSystemFont(titles[i], "Arial", 12);
        label->setColor(Color3B::BLACK);
        rootNode->addChild(label);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        label->setPosition(Vec2(25.0f, yPos));
    }

    AlertDialog::Builder(this)
        .setTitle(__UTF8("排列座位"))
        .setContentNode(rootNode)
        .setNegativeButton(__UTF8("取消"), nullptr)
        .setPositiveButton(__UTF8("确定"), [this, radioGroup](AlertDialog *, int) {
        switch (radioGroup->getSelectedButtonIndex()) {
        case 0: _competitionData->rankTablesByRandom(_currentRound); break;
        case 1: _competitionData->rankTablesBySnake(_currentRound); break;
        case 2: _competitionData->rankTablesByScores(_currentRound); break;
        case 3: Director::getInstance()->pushScene(CompetitionRankCustomScene::create(_competitionData, _currentRound)); return true;
        default: return false;
        }
        _tableView->reloadData();
        _competitionData->writeToFile();
        return true;
    }).create()->show();
}
