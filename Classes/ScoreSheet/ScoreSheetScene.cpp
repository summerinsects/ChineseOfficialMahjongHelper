#include "ScoreSheetScene.h"
#include "Record.h"
#include "RecordScene.h"
#include "HistoryScene.h"
#include "../widget/AlertView.h"
#include "../widget/CWEditBoxDelegate.h"
#include "../widget/HandTilesWidget.h"
#include "../common.h"
#include "../mahjong-algorithm/fan_calculator.h"

#include "json/stringbuffer.h"
#include "json/prettywriter.h"

USING_NS_CC;

static Record g_currentRecord;

static forceinline bool isCStringEmpty(const char *str) {
    return *str == '\0';
}

static void readFromJson() {
    std::string fileName = FileUtils::getInstance()->getWritablePath();
    fileName.append("record.json");
    std::string str = FileUtils::getInstance()->getStringFromFile(fileName);
    CCLOG("%s", str.c_str());
    try {
        rapidjson::Document doc;
        doc.Parse<0>(str.c_str());
        if (doc.HasParseError()) {
            return;
        }

        JsonToRecord(doc, g_currentRecord);
    }
    catch (std::exception &e) {
        CCLOG("%s %s", __FUNCTION__, e.what());
    }
}

static void writeToJson() {
    try {
        rapidjson::Document doc(rapidjson::Type::kObjectType);
        RecordToJson(g_currentRecord, doc, doc.GetAllocator());

        rapidjson::StringBuffer buf;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buf);
        doc.Accept(writer);

        CCLOG("%.*s", buf.GetSize(), buf.GetString());

        std::string path = FileUtils::getInstance()->getWritablePath();
        path.append("record.json");
        FILE *file = fopen(path.c_str(), "wb");
        if (LIKELY(file != nullptr)) {
            fwrite(buf.GetString(), 1, buf.GetSize(), file);
            fclose(file);
        }
    }
    catch (std::exception &e) {
        CCLOG("%s %s", __FUNCTION__, e.what());
    }
}

Scene *ScoreSheetScene::createScene() {
    auto scene = Scene::create();
    auto layer = ScoreSheetScene::create();
    scene->addChild(layer);
    return scene;
}

bool ScoreSheetScene::init() {
    if (UNLIKELY(!BaseLayer::initWithTitle("国标麻将记分器"))) {
        return false;
    }

    memset(_totalScores, 0, sizeof(_totalScores));

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    const float buttonWidth = 54;
    const float buttonGap = (visibleSize.width - 4 - buttonWidth) / 3;

    // 使用说明按钮
    ui::Button *button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(55.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText("使用说明");
    button->setPosition(Vec2(origin.x + 2 + buttonWidth * 0.5f + buttonGap * 3, origin.y + visibleSize.height - 45));
    button->addClickEventListener(std::bind(&ScoreSheetScene::onInstructionButton, this, std::placeholders::_1));

    // 历史记录按钮
    button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(55.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText("历史记录");
    button->setPosition(Vec2(origin.x + 2 + buttonWidth * 0.5f + buttonGap * 2, origin.y + visibleSize.height - 45));
    button->addClickEventListener(std::bind(&ScoreSheetScene::onHistoryButton, this, std::placeholders::_1));

    // 重置按钮
    button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(55.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText("重置");
    button->setPosition(Vec2(origin.x + 2 + buttonWidth * 0.5f + buttonGap, origin.y + visibleSize.height - 45));
    button->addClickEventListener(std::bind(&ScoreSheetScene::onResetButton, this, std::placeholders::_1));

    // 追分策略按钮
    button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(55.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText("追分策略");
    button->setPosition(Vec2(origin.x + 2 + buttonWidth * 0.5f, origin.y + visibleSize.height - 45));
    button->addClickEventListener(std::bind(&ScoreSheetScene::onPursuitButton, this, std::placeholders::_1));
    scaleLabelToFitWidth(button->getTitleLabel(), 50.0f);

    // 时间label
    _timeLabel = Label::createWithSystemFont("当前时间", "Arial", 12);
    this->addChild(_timeLabel);
    _timeLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    _timeLabel->setPosition(Vec2(origin.x + 5, (origin.y + visibleSize.height - 435) * 0.5f - 10));
    _timeLabel->setColor(Color3B::BLACK);

    // 用来绘制表格线的根结点
    DrawNode *node = DrawNode::create();
    this->addChild(node);
    node->setPosition(Vec2(origin.x, (origin.y + visibleSize.height - 435) * 0.5f));

    const float gap = visibleSize.width / 6;  // 分成6份
    _cellWidth = gap;

    // 5条竖线
    for (int i = 0; i < 5; ++i) {
        const float x = gap * (i + 1);
        node->drawLine(Vec2(x, 0.0f), Vec2(x, 400.0f), Color4F::BLACK);
    }

    // 21条横线
    for (int i = 0; i < 21; ++i) {
        const float y = 20.0f * i;
        node->drawLine(Vec2(0.0f, y), Vec2(visibleSize.width, y),
            (i > 0 && i < 16) ? Color4F(0.3f, 0.3f, 0.3f, 1.0f) : Color4F::BLACK);
    }

    // 第1栏：选手姓名
    Label *label = Label::createWithSystemFont("选手姓名", "Arail", 12);
    label->setColor(Color3B::ORANGE);
    label->setPosition(Vec2(gap * 0.5f, 390));
    node->addChild(label);
    scaleLabelToFitWidth(label, gap - 4);

    // 4个用于弹出输入框的AlertLayer及同位置的label
    for (int i = 0; i < 4; ++i) {
        ui::Button *button = ui::Button::create();
        button->setScale9Enabled(true);
        button->setPosition(Vec2(gap * (i + 1.5f), 390));
        button->setContentSize(Size(gap, 20.0f));
        node->addChild(button);
        button->addClickEventListener(std::bind(&ScoreSheetScene::onNameButton, this, std::placeholders::_1, i));

        _nameLabel[i] = Label::createWithSystemFont("", "Arail", 12);
        _nameLabel[i]->setColor(Color3B::ORANGE);
        _nameLabel[i]->setPosition(Vec2(gap * (i + 1.5f), 390));
        node->addChild(_nameLabel[i]);
    }

    _lockButton = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
    node->addChild(_lockButton);
    _lockButton->setScale9Enabled(true);
    _lockButton->setContentSize(Size(gap, 20.0f));
    _lockButton->setTitleFontSize(12);
    _lockButton->setTitleText("锁定");
    _lockButton->setPosition(Vec2(gap * 5.5f, 390));
    _lockButton->addClickEventListener(std::bind(&ScoreSheetScene::onLockButton, this, std::placeholders::_1));

    // 第2、3栏：座位
    const char *row0Text[] = {"开局座位", "东", "南", "西", "北"};
    const char *row1Text[] = {"每圈座位", "东南北西", "南东西北", "西北东南", "北西南东"};
    for (int i = 0; i < 5; ++i) {
        label = Label::createWithSystemFont(row0Text[i], "Arail", 12);
        label->setColor(Color3B::BLACK);
        label->setPosition(Vec2(gap * (i + 0.5f), 370));
        node->addChild(label);
        scaleLabelToFitWidth(label, gap - 4);

        label = Label::createWithSystemFont(row1Text[i], "Arail", 12);
        label->setColor(Color3B::BLACK);
        label->setPosition(Vec2(gap * (i + 0.5f), 350));
        node->addChild(label);
        scaleLabelToFitWidth(label, gap - 4);
    }

    // 第4栏：累计
    label = Label::createWithSystemFont("累计", "Arail", 12);
    label->setColor(Color3B::ORANGE);
    label->setPosition(Vec2(gap * 0.5f, 330));
    node->addChild(label);
    scaleLabelToFitWidth(label, gap - 4);

    label = Label::createWithSystemFont("备注", "Arail", 12);
    label->setColor(Color3B::BLACK);
    label->setPosition(Vec2(gap * 5.5f, 330));
    node->addChild(label);
    scaleLabelToFitWidth(label, gap - 4);

    for (int i = 0; i < 4; ++i) {
        _totalLabel[i] = Label::createWithSystemFont("+0", "Arail", 12);
        _totalLabel[i]->setColor(Color3B::ORANGE);
        _totalLabel[i]->setPosition(Vec2(gap * (i + 1.5f), 330));
        node->addChild(_totalLabel[i]);

        ui::Button *button = ui::Button::create();
        button->setScale9Enabled(true);
        button->setPosition(Vec2(gap * (i + 1.5f), 330));
        button->setContentSize(Size(gap, 20.0f));
        node->addChild(button);
        button->addClickEventListener(std::bind(&ScoreSheetScene::onScoreButton, this, std::placeholders::_1, i));
    }

    // 第5~20栏，东风东~北风北的计分
    for (int k = 0; k < 16; ++k) {
        const float y = 10 + (15 - k) * 20;

        // 东风东~北风北名字
        label = Label::createWithSystemFont(handNameText[k], "Arail", 12);
        label->setColor(Color3B(0x60, 0x60, 0x60));
        label->setPosition(Vec2(gap * 0.5f, y));
        node->addChild(label);
        scaleLabelToFitWidth(label, gap - 4);

        // 四位选手得分
        for (int i = 0; i < 4; ++i) {
            _scoreLabels[k][i] = Label::createWithSystemFont("", "Arail", 12);
            _scoreLabels[k][i]->setPosition(Vec2(gap * (i + 1.5f), y));
            node->addChild(_scoreLabels[k][i]);
        }

        // 计分按钮
        ui::Button *button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
        node->addChild(button);
        button->setScale9Enabled(true);
        button->setContentSize(Size(gap, 20.0f));
        button->setTitleFontSize(12);
        button->setTitleText("计分");
        button->setPosition(Vec2(gap * 5.5f, y));
        button->addClickEventListener(std::bind(&ScoreSheetScene::onRecordButton, this, std::placeholders::_1, k));
        button->setEnabled(false);
        button->setVisible(false);
        _recordButton[k] = button;

        // 备注的番种label
        Label *label = Label::createWithSystemFont("", "Arail", 12);
        label->setColor(Color3B(0x60, 0x60, 0x60));
        label->setPosition(Vec2(gap * 5.5f, y));
        node->addChild(label);
        label->setVisible(false);
        _fanNameLabel[k] = label;

        // 查看详情按钮
        button = ui::Button::create();
        node->addChild(button);
        button->setScale9Enabled(true);
        button->setContentSize(Size(gap, 20.0f));
        button->setTitleFontSize(12);
        button->setPosition(Vec2(gap * 5.5f, y));
        button->addClickEventListener(std::bind(&ScoreSheetScene::onDetailButton, this, std::placeholders::_1, k));
        button->setEnabled(false);
        _detailButton[k] = button;
    }

    // 从json中读，并恢复界面数据
    readFromJson();
    recover();
    return true;
}

void ScoreSheetScene::cleanRow(size_t handIdx) {
    for (int i = 0; i < 4; ++i) {
        _scoreLabels[handIdx][i]->setString("");
    }
    _recordButton[handIdx]->setVisible(false);
    _recordButton[handIdx]->setEnabled(false);
    _fanNameLabel[handIdx]->setVisible(false);
    _detailButton[handIdx]->setEnabled(false);
}

void ScoreSheetScene::fillRow(size_t handIdx) {
    int scoreTable[4];
    const Record::Detail &detail = g_currentRecord.detail[handIdx];
    translateDetailToScoreTable(detail, scoreTable);

    // 填入这一盘四位选手的得分
    for (int i = 0; i < 4; ++i) {
        _scoreLabels[handIdx][i]->setString(StringUtils::format("%+d", scoreTable[i]));
        _totalScores[i] += scoreTable[i];  // 更新总分
        if (scoreTable[i] > 0) {
            _scoreLabels[handIdx][i]->setColor(Color3B(224, 45, 45));
        }
        else if (scoreTable[i] < 0) {
            _scoreLabels[handIdx][i]->setColor(Color3B(37, 153, 14));
        }
        else {
            _scoreLabels[handIdx][i]->setColor(Color3B(0x60, 0x60, 0x60));
        }
    }

    // 禁用并隐藏这一行的计分按钮
    _recordButton[handIdx]->setVisible(false);
    _recordButton[handIdx]->setEnabled(false);
    _detailButton[handIdx]->setEnabled(true);

    Label *label = _fanNameLabel[handIdx];
    const char *fanName = nullptr;
    if (detail.score == 0) {
        fanName = "荒庄";
    }

    // 选取标记的最大番种显示出来
    uint64_t fanFlag = detail.fan_flag;
    if (fanFlag != 0) {
        for (unsigned n = mahjong::BIG_FOUR_WINDS; n < mahjong::DRAGON_PUNG; ++n) {
            if (TEST_FAN(fanFlag, n)) {
                unsigned idx = n;
                fanName = mahjong::fan_name[idx];
                break;
            }
        }
    }

    // 将未标记番种的显示为其他凑番
    if (fanName == nullptr) {
        fanName = "其他凑番";
    }

    label->setString(fanName);
    label->setVisible(true);
    scaleLabelToFitWidth(label, _cellWidth - 4);
}

void ScoreSheetScene::refreshStartTime() {
    char str[255] = "开始时间：";
    size_t len = strlen(str);
    strftime(str + len, sizeof(str) - len, "%Y-%m-%d %H:%M", localtime(&g_currentRecord.start_time));
    _timeLabel->setString(str);
}

void ScoreSheetScene::refreshEndTime() {
    char str[255] = "起止时间：";
    size_t len = strlen(str);
    len += strftime(str + len, sizeof(str) - len, "%Y-%m-%d %H:%M", localtime(&g_currentRecord.start_time));
    strcpy(str + len, " -- ");
    len += 4;
    strftime(str + len, sizeof(str) - len, "%Y-%m-%d %H:%M", localtime(&g_currentRecord.end_time));
    _timeLabel->setString(str);
}

void ScoreSheetScene::recover() {
    // 有选手名字为空，则清空数据
    const char (&name)[4][255] = g_currentRecord.name;
    if (std::any_of(std::begin(name), std::end(name), &isCStringEmpty)) {
        memset(&g_currentRecord, 0, sizeof(g_currentRecord));
        onTimeScheduler(0.0f);
        this->schedule(schedule_selector(ScoreSheetScene::onTimeScheduler), 1.0f);
        return;
    }

    // 禁用和隐藏名字输入框，显示名字的label
    for (int i = 0; i < 4; ++i) {
        _nameLabel[i]->setString(name[i]);
        _nameLabel[i]->setVisible(true);
        scaleLabelToFitWidth(_nameLabel[i], _cellWidth - 4);
    }

    // 禁用和隐藏锁定按钮
    _lockButton->setEnabled(false);
    _lockButton->setVisible(false);

    // 初始化总分
    memset(_totalScores, 0, sizeof(_totalScores));

    // 逐行填入数据
    for (size_t i = 0; i < g_currentRecord.current_index; ++i) {
        fillRow(i);
    }
    for (size_t i = g_currentRecord.current_index; i < 16; ++i) {
        cleanRow(i);
    }

    // 刷新总分label
    for (int i = 0; i < 4; ++i) {
        _totalLabel[i]->setString(StringUtils::format("%+d", _totalScores[i]));
    }

    // 如果不是北风北，则显示下一行的计分按钮
    if (g_currentRecord.current_index < 16) {
        _recordButton[g_currentRecord.current_index]->setVisible(true);
        _recordButton[g_currentRecord.current_index]->setEnabled(true);

        refreshStartTime();
    }
    else {
        refreshEndTime();
    }
    this->unschedule(schedule_selector(ScoreSheetScene::onTimeScheduler));
}

void ScoreSheetScene::reset() {
    memset(&g_currentRecord, 0, sizeof(g_currentRecord));
    writeToJson();

    memset(_totalScores, 0, sizeof(_totalScores));

    for (int i = 0; i < 4; ++i) {
        _nameLabel[i]->setVisible(false);
        _totalLabel[i]->setString("+0");
    }

    _lockButton->setEnabled(true);
    _lockButton->setVisible(true);
    onTimeScheduler(0.0f);
    this->schedule(schedule_selector(ScoreSheetScene::onTimeScheduler), 1.0f);

    for (int i = 0; i < 16; ++i) {
        cleanRow(i);
    }
}

void ScoreSheetScene::onNameButton(cocos2d::Ref *sender, size_t idx) {
    if (_lockButton->isVisible() && _lockButton->isEnabled()) {
        editName(idx);
    }
    else {
        if (g_currentRecord.current_index < 16) {
            AlertView::showWithMessage("提示", "对局已经开始，是否要修改选手姓名？",
                std::bind(&ScoreSheetScene::editName, this, idx), nullptr);
        }
        else {
            AlertView::showWithMessage("提示", "对局已经结束，是否要修改选手姓名？",
                std::bind(&ScoreSheetScene::editName, this, idx), nullptr);
        }
    }
}

void ScoreSheetScene::editName(size_t idx) {
    ui::EditBox *editBox = ui::EditBox::create(Size(120.0f, 20.0f), ui::Scale9Sprite::create("source_material/btn_square_normal.png"));
    editBox->setFontColor(Color3B::BLACK);
    editBox->setFontSize(12);
    editBox->setText(g_currentRecord.name[idx]);
    editBox->setPlaceholderFontColor(Color4B::GRAY);
    editBox->setPlaceHolder("输入选手姓名");
    editBox->setInputMode(ui::EditBox::InputMode::SINGLE_LINE);
    editBox->setInputFlag(ui::EditBox::InputFlag::SENSITIVE);
    editBox->setReturnType(ui::EditBox::KeyboardReturnType::DONE);

    const char *wind[] = { "东", "南", "西", "北" };
    char title[64];
    snprintf(title, sizeof(title), "开局座位「%s」", wind[idx]);
    AlertView::showWithNode(title, editBox, [this, editBox, idx]() {
        const char *text = editBox->getText();
        if (!isCStringEmpty(text)) {
            strncpy(g_currentRecord.name[idx], text, 255);
            _nameLabel[idx]->setVisible(true);
            _nameLabel[idx]->setString(text);
            scaleLabelToFitWidth(_nameLabel[idx], _cellWidth - 4);
        }
    }, nullptr);
    editBox->touchDownAction(editBox, ui::Widget::TouchEventType::ENDED);
}

void ScoreSheetScene::onLockButton(cocos2d::Ref *sender) {
    const char (&name)[4][255] = g_currentRecord.name;
    if (std::any_of(std::begin(name), std::end(name), &isCStringEmpty)) {
        AlertView::showWithMessage("锁定", "请先录入四位参赛选手姓名", nullptr, nullptr);
        return;
    }

    memset(_totalScores, 0, sizeof(_totalScores));

    for (int i = 0; i < 4; ++i) {
        _nameLabel[i]->setVisible(true);
        _nameLabel[i]->setString(g_currentRecord.name[i]);
        scaleLabelToFitWidth(_nameLabel[i], _cellWidth - 4);
    }

    _recordButton[0]->setVisible(true);
    _recordButton[0]->setEnabled(true);
    _lockButton->setEnabled(false);
    _lockButton->setVisible(false);

    this->unschedule(schedule_selector(ScoreSheetScene::onTimeScheduler));

    g_currentRecord.start_time = time(nullptr);
    refreshStartTime();

    writeToJson();
}

void ScoreSheetScene::onRecordButton(cocos2d::Ref *sender, size_t handIdx) {
    editRecord(handIdx, false);
}

void ScoreSheetScene::editRecord(size_t handIdx, bool modify) {
    const char *name[] = { g_currentRecord.name[0], g_currentRecord.name[1], g_currentRecord.name[2], g_currentRecord.name[3] };
    auto scene = RecordScene::createScene(handIdx, name, modify ? &g_currentRecord.detail[handIdx] : nullptr,
        [this, handIdx](const Record::Detail &detail) {
        bool isModify = (handIdx != g_currentRecord.current_index);

        // 更新数据时，要先删除原来的记录
        if (isModify) {
            int scores[4];
            translateDetailToScoreTable(g_currentRecord.detail[handIdx], scores);
            for (int i = 0; i < 4; ++i) {
                _totalScores[i] -= scores[i];
            }
        }

        // 将计分面板的数据更新到当前数据中
        memcpy(&g_currentRecord.detail[handIdx], &detail, sizeof(Record::Detail));

        // 填入当前行
        fillRow(handIdx);

        // 更新总分
        for (int i = 0; i < 4; ++i) {
            _totalLabel[i]->setString(StringUtils::format("%+d", _totalScores[i]));
        }

        if (isModify) {
            HistoryScene::modifyRecord(g_currentRecord);
        }
        else {
            // 如果不是北风北，则显示下一行的计分按钮，否则一局结束，并增加新的历史记录
            if (++g_currentRecord.current_index < 16) {
                _recordButton[g_currentRecord.current_index]->setVisible(true);
                _recordButton[g_currentRecord.current_index]->setEnabled(true);
            }
            else {
                g_currentRecord.end_time = time(nullptr);
                refreshEndTime();
                HistoryScene::modifyRecord(g_currentRecord);
            }
        }
        writeToJson();
    });
    Director::getInstance()->pushScene(scene);
}

static std::string stringifyDetail(const Record::Detail &detail) {
    std::string ret;

    int wc = detail.win_claim;
    int winIndex = WIN_INDEX(wc);
    int claimIndex = CLAIM_INDEX(wc);
    if (winIndex == claimIndex) {
        ret.append(StringUtils::format("「%s」自摸%d番。\n", g_currentRecord.name[winIndex], detail.score));
    }
    else {
        ret.append(StringUtils::format("「%s」和%d番，「%s」放炮。\n", g_currentRecord.name[winIndex], detail.score, g_currentRecord.name[claimIndex]));
    }

    uint64_t fanFlag = detail.fan_flag;
    if (fanFlag != 0) {
        std::string fanText;
        for (unsigned n = mahjong::BIG_FOUR_WINDS; n < mahjong::DRAGON_PUNG; ++n) {
            if (TEST_FAN(fanFlag, n)) {
                unsigned idx = n;
                fanText.append("「");
                //if (LIKELY(!fanText.empty())) {
                //    fanText.append("、");
                //}
                fanText.append(mahjong::fan_name[idx]);
                fanText.append("」");
            }
        }

        if (!fanText.empty()) {
            ret.append("和出番种：");
            ret.append(fanText);
            ret.append("等。\n");
        }
    }

    if (detail.false_win != 0) {
        for (int i = 0; i < 4; ++i) {
            if (TEST_FALSE_WIN(detail.false_win, i)) {
                ret.append("「");
                ret.append(g_currentRecord.name[i]);
                ret.append("」");
            }
        }
        ret.append("错和。\n");
    }

    return ret;
}

void ScoreSheetScene::onDetailButton(cocos2d::Ref *sender, size_t handIdx) {
    const Record::Detail &detail = g_currentRecord.detail[handIdx];
    if (detail.score == 0) {
        AlertView::showWithMessage(std::string(handNameText[handIdx]).append("详情"),
            "荒庄。\n\n是否需要修改这盘的记录？",
            std::bind(&ScoreSheetScene::editRecord, this, handIdx, true), nullptr);
        return;
    }

    std::string message = stringifyDetail(detail);
    message.append("\n是否需要修改这盘的记录？");

    if (detail.win_hand.tile_count == 0 || detail.win_hand.win_tile == 0) {
        AlertView::showWithMessage(std::string(handNameText[handIdx]).append("详情"), message,
            std::bind(&ScoreSheetScene::editRecord, this, handIdx, true), nullptr);
        return;
    }

    RecordScene::CalculateParam param;
    RecordScene::_WinHandToCalculateParam(detail.win_hand, param);

    Size visibleSize = Director::getInstance()->getVisibleSize();
    const float maxWidth = visibleSize.width * 0.8f - 10;

    // 花（使用emoji代码）
    Label *flowerLabel = Label::createWithSystemFont(StringUtils::format("\xE2\x9D\x80x%d", param.flower_count), "Arial", 12);
    flowerLabel->setColor(Color3B(224, 45, 45));
    const Size &flowerSize = flowerLabel->getContentSize();

    // 手牌
    Node *tilesNode = HandTilesWidget::createStaticNode(param.hand_tiles, param.win_tile);
    Size tilesNodeSize = tilesNode->getContentSize();
    if (tilesNodeSize.width > maxWidth) {
        const float scale = maxWidth / tilesNodeSize.width;
        tilesNode->setScale(scale);
        tilesNodeSize.width = maxWidth;
        tilesNodeSize.height *= scale;
    }

    // 描述文本
    Label *label = Label::createWithSystemFont(message, "Arial", 12);
    label->setColor(Color3B::BLACK);
    if (label->getContentSize().width > maxWidth) {  // 当宽度超过时，设置范围，使文本换行
        label->setDimensions(maxWidth, 0);
    }
    const Size &labelSize = label->getContentSize();

    Node *container = Node::create();
    container->setContentSize(Size(maxWidth, labelSize.height + 10 + tilesNodeSize.height + 5 + flowerSize.height));

    container->addChild(flowerLabel);
    flowerLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    flowerLabel->setPosition(Vec2(0, labelSize.height + 10 + tilesNodeSize.height + 5 + flowerSize.height * 0.5f));

    container->addChild(tilesNode);
    tilesNode->setPosition(Vec2(maxWidth * 0.5f, labelSize.height + 10 + tilesNodeSize.height * 0.5f));

    container->addChild(label);
    label->setPosition(Vec2(maxWidth * 0.5f, labelSize.height * 0.5f));

    AlertView::showWithNode(std::string(handNameText[handIdx]).append("详情"), container,
        std::bind(&ScoreSheetScene::editRecord, this, handIdx, true), nullptr);
}

void ScoreSheetScene::onTimeScheduler(float dt) {
    char str[255] = "当前时间：";
    size_t len = strlen(str);
    time_t t = time(nullptr);
    strftime(str + len, sizeof(str) - len, "%Y-%m-%d %H:%M", localtime(&t));
    _timeLabel->setString(str);
}

void ScoreSheetScene::onInstructionButton(cocos2d::Ref *sender) {
    Size visibleSize = Director::getInstance()->getVisibleSize();
    const float width = visibleSize.width * 0.8f - 10;

    Label *label = Label::createWithSystemFont(
        "1. 使用步骤：点击「选手姓名」一栏，输入四名选手姓名，点击「锁定」，开始「记分」。\n"
        "2. 计分时如果有标记番种，则「备注」一栏会选取一个最大的番种名予以显示。\n"
        "3. 对于已经记分的，点击「备注」一栏可修改记录。\n"
        "4. 对局未完成时，点击「累计」一栏处，可显示分差并有快捷计算追分选项。\n"
        "5. 「北风北」记分完成后，会自动添加入「历史记录」。\n"
        "6. 「历史记录」里的内容只要不卸载程序就会一直保存。\n"
        "7. 「重置」可清空表格。", "Arial", 10);
    label->setColor(Color3B::BLACK);
    label->setDimensions(width, 0);
    AlertView::showWithNode("使用说明", label, nullptr, nullptr);
}

void ScoreSheetScene::onHistoryButton(cocos2d::Ref *sender) {
    Director::getInstance()->pushScene(HistoryScene::createScene([this](const Record &record) {
        const char (&name)[4][255] = g_currentRecord.name;
        if (std::any_of(std::begin(name), std::end(name), &isCStringEmpty)
            || g_currentRecord.current_index == 16) {
            memcpy(&g_currentRecord, &record, sizeof(g_currentRecord));
            recover();
            return true;
        }
        AlertView::showWithMessage("提示", "当前一局尚未完成时不支持查看历史记录", nullptr, nullptr);
        return false;
    }));
}

void ScoreSheetScene::onResetButton(cocos2d::Ref *sender) {
    const char (&name)[4][255] = g_currentRecord.name;
    if (std::any_of(std::begin(name), std::end(name), &isCStringEmpty)) {
        reset();
        return;
    }

    if (g_currentRecord.current_index == 16) {
        reset();
        return;
    }

    AlertView::showWithMessage("重置", "重置操作会清空当前已记录的信息，未打满北风北的记录将会丢失，确定要这样做吗？",
        std::bind(&ScoreSheetScene::reset, this), nullptr);
}

static void showPursuit(int delta) {
    if (delta == 0) {
        AlertView::showWithMessage("追分与保位", "平分", nullptr, nullptr);
        return;
    }

    std::string msg;
    msg.reserve(256);
    if (delta < 0) {
        delta = -delta;
    }
    msg.append(StringUtils::format("分差%d分\n\n超分需", delta));

    int d1 = delta - 32;
    if (d1 < 8) {
        msg.append("任意和牌");
    }
    else {
        int d2 = d1 >> 1;
        if (d2 < 8) {
            msg.append(StringUtils::format("任意自摸或对点，旁点至少%d番", d1 + 1));
        }
        else {
            int d4 = d2 >> 1;
            if (d4 < 8) {
                msg.append(StringUtils::format("任意自摸，对点至少%d番，旁点至少%d番", d2 + 1, d1 + 1));
            }
            else {
                msg.append(StringUtils::format("自摸至少%d番，对点至少%d番，旁点至少%d番", d4 + 1, d2 + 1, d1 + 1));
            }
        }
    }

    if (delta <= 8) {
        msg.append("\n\n点炮无法保位");
    }
    else {
        int d2 = d1 >> 1;
        if (d2 <= 8) {
            msg.append(StringUtils::format("\n\n对点无法保位，保位可旁点至多%d番", delta - 1));
        }
        else {
            msg.append(StringUtils::format("\n\n保位可对点至多%d番，旁点至多%d番", (d1 & 1) ? d2 : d2 - 1, delta - 1));
        }
    }

    AlertView::showWithMessage("追分与保位", msg, nullptr, nullptr);
}

static DrawNode *createTable(const int (&totalScores)[4]) {
    // 下标
    int indices[4] = { 0, 1, 2, 3 };
    // 按分数排序
    std::stable_sort(std::begin(indices), std::end(indices), [&totalScores](int a, int b) {
        return totalScores[a] > totalScores[b];
    });

    Size visibleSize = Director::getInstance()->getVisibleSize();

    const float width = visibleSize.width * 0.8f - 20;
    const float height = 10 * 20;
    const float gap = width / 6;

    DrawNode *drawNode = DrawNode::create();
    drawNode->setContentSize(Size(width, height));

    // 横线
    drawNode->drawLine(Vec2(0, 0), Vec2(width, 0), Color4F::BLACK);
    drawNode->drawLine(Vec2(0, 20), Vec2(width, 20), Color4F::BLACK);
    drawNode->drawLine(Vec2(0, 40), Vec2(width, 40), Color4F::BLACK);
    drawNode->drawLine(Vec2(gap, 60), Vec2(width, 60), Color4F::BLACK);
    drawNode->drawLine(Vec2(0, 80), Vec2(width, 80), Color4F::BLACK);
    drawNode->drawLine(Vec2(0, 100), Vec2(width, 100), Color4F::BLACK);
    drawNode->drawLine(Vec2(gap, 120), Vec2(width, 120), Color4F::BLACK);
    drawNode->drawLine(Vec2(gap, 140), Vec2(width, 140), Color4F::BLACK);
    drawNode->drawLine(Vec2(0, 160), Vec2(width, 160), Color4F::BLACK);
    drawNode->drawLine(Vec2(0, 180), Vec2(width, 180), Color4F::BLACK);
    drawNode->drawLine(Vec2(0, 200), Vec2(width, 200), Color4F::BLACK);

    // 竖线
    drawNode->drawLine(Vec2(0, 0), Vec2(0, 200), Color4F::BLACK);
    for (int i = 1; i < 6; ++i) {
        const float x = i * gap;
        drawNode->drawLine(Vec2(x, 0), Vec2(x, 20), Color4F::BLACK);
        drawNode->drawLine(Vec2(x, 40), Vec2(x, 80), Color4F::BLACK);
        drawNode->drawLine(Vec2(x, 100), Vec2(x, 160), Color4F::BLACK);
        drawNode->drawLine(Vec2(x, 180), Vec2(x, 200), Color4F::BLACK);
    }
    drawNode->drawLine(Vec2(width, 0), Vec2(width, 200), Color4F::BLACK);

    Label *label = Label::createWithSystemFont("追者", "Arail", 12);
    label->setColor(Color3B::ORANGE);
    label->setPosition(Vec2(gap * 0.5f, 190));
    drawNode->addChild(label);
    scaleLabelToFitWidth(label, gap - 4);

    label = Label::createWithSystemFont("被追", "Arail", 12);
    label->setColor(Color3B::ORANGE);
    label->setPosition(Vec2(gap * 1.5f, 190));
    drawNode->addChild(label);
    scaleLabelToFitWidth(label, gap - 4);

    label = Label::createWithSystemFont("分差", "Arail", 12);
    label->setColor(Color3B::ORANGE);
    label->setPosition(Vec2(gap * 2.5f, 190));
    drawNode->addChild(label);
    scaleLabelToFitWidth(label, gap - 4);

    label = Label::createWithSystemFont("自摸", "Arail", 12);
    label->setColor(Color3B(0x60, 0x60, 0x60));
    label->setPosition(Vec2(gap * 3.5f, 190));
    drawNode->addChild(label);
    scaleLabelToFitWidth(label, gap - 4);

    label = Label::createWithSystemFont("对点", "Arail", 12);
    label->setColor(Color3B(0x60, 0x60, 0x60));
    label->setPosition(Vec2(gap * 4.5f, 190));
    drawNode->addChild(label);
    scaleLabelToFitWidth(label, gap - 4);

    label = Label::createWithSystemFont("旁点", "Arail", 12);
    label->setColor(Color3B(0x60, 0x60, 0x60));
    label->setPosition(Vec2(gap * 5.5f, 190));
    drawNode->addChild(label);
    scaleLabelToFitWidth(label, gap - 4);

    // 4位
    label = Label::createWithSystemFont(g_currentRecord.name[indices[3]], "Arail", 12);
    label->setColor(Color3B::ORANGE);
    label->setPosition(Vec2(gap * 0.5f, 130));
    drawNode->addChild(label);
    scaleLabelToFitWidth(label, gap - 4);

    // 追1位
    label = Label::createWithSystemFont(g_currentRecord.name[indices[0]], "Arail", 12);
    label->setColor(Color3B::ORANGE);
    label->setPosition(Vec2(gap * 1.5f, 150));
    drawNode->addChild(label);
    scaleLabelToFitWidth(label, gap - 4);

    int delta = totalScores[indices[0]] - totalScores[indices[3]];
    int d = delta - 32;
    int d1 = d + 1;
    int d2 = (d >> 1) + 1;
    int d4 = (d >> 2) + 1;

    // 分差
    label = Label::createWithSystemFont(StringUtils::format("%d", delta), "Arail", 12);
    label->setColor(Color3B::ORANGE);
    label->setPosition(Vec2(gap * 2.5f, 150));
    drawNode->addChild(label);
    scaleLabelToFitWidth(label, gap - 4);

    // 自摸
    label = Label::createWithSystemFont(d4 <= 8 ? std::string("8") : StringUtils::format("%d", d4), "Arail", 12);
    label->setColor(Color3B(0x60, 0x60, 0x60));
    label->setPosition(Vec2(gap * 3.5f, 150));
    drawNode->addChild(label);
    scaleLabelToFitWidth(label, gap - 4);

    // 对点
    label = Label::createWithSystemFont(d2 <= 8 ? std::string("8") : StringUtils::format("%d", d2), "Arail", 12);
    label->setColor(Color3B(0x60, 0x60, 0x60));
    label->setPosition(Vec2(gap * 4.5f, 150));
    drawNode->addChild(label);
    scaleLabelToFitWidth(label, gap - 4);

    // 旁点
    label = Label::createWithSystemFont(d1 <= 8 ? std::string("8") : StringUtils::format("%d", d1), "Arail", 12);
    label->setColor(Color3B(0x60, 0x60, 0x60));
    label->setPosition(Vec2(gap * 5.5f, 150));
    drawNode->addChild(label);
    scaleLabelToFitWidth(label, gap - 4);

    // 追2位
    label = Label::createWithSystemFont(g_currentRecord.name[indices[1]], "Arail", 12);
    label->setColor(Color3B::ORANGE);
    label->setPosition(Vec2(gap * 1.5f, 130));
    drawNode->addChild(label);
    scaleLabelToFitWidth(label, gap - 4);

    delta = totalScores[indices[1]] - totalScores[indices[3]];
    d = delta - 32;
    d1 = d + 1;
    d2 = (d >> 1) + 1;
    d4 = (d >> 2) + 1;

    // 分差
    label = Label::createWithSystemFont(StringUtils::format("%d", delta), "Arail", 12);
    label->setColor(Color3B::ORANGE);
    label->setPosition(Vec2(gap * 2.5f, 130));
    drawNode->addChild(label);
    scaleLabelToFitWidth(label, gap - 4);

    // 自摸
    label = Label::createWithSystemFont(d4 <= 8 ? std::string("8") : StringUtils::format("%d", d4), "Arail", 12);
    label->setColor(Color3B(0x60, 0x60, 0x60));
    label->setPosition(Vec2(gap * 3.5f, 130));
    drawNode->addChild(label);
    scaleLabelToFitWidth(label, gap - 4);

    // 对点
    label = Label::createWithSystemFont(d2 <= 8 ? std::string("8") : StringUtils::format("%d", d2), "Arail", 12);
    label->setColor(Color3B(0x60, 0x60, 0x60));
    label->setPosition(Vec2(gap * 4.5f, 130));
    drawNode->addChild(label);
    scaleLabelToFitWidth(label, gap - 4);

    // 旁点
    label = Label::createWithSystemFont(d1 <= 8 ? std::string("8") : StringUtils::format("%d", d1), "Arail", 12);
    label->setColor(Color3B(0x60, 0x60, 0x60));
    label->setPosition(Vec2(gap * 5.5f, 130));
    drawNode->addChild(label);
    scaleLabelToFitWidth(label, gap - 4);

    // 追3位
    label = Label::createWithSystemFont(g_currentRecord.name[indices[2]], "Arail", 12);
    label->setColor(Color3B::ORANGE);
    label->setPosition(Vec2(gap * 1.5f, 110));
    drawNode->addChild(label);
    scaleLabelToFitWidth(label, gap - 4);

    delta = totalScores[indices[2]] - totalScores[indices[3]];
    d = delta - 32;
    d1 = d + 1;
    d2 = (d >> 1) + 1;
    d4 = (d >> 2) + 1;

    // 分差
    label = Label::createWithSystemFont(StringUtils::format("%d", delta), "Arail", 12);
    label->setColor(Color3B::ORANGE);
    label->setPosition(Vec2(gap * 2.5f, 110));
    drawNode->addChild(label);
    scaleLabelToFitWidth(label, gap - 4);

    // 自摸
    label = Label::createWithSystemFont(d4 <= 8 ? std::string("8") : StringUtils::format("%d", d4), "Arail", 12);
    label->setColor(Color3B(0x60, 0x60, 0x60));
    label->setPosition(Vec2(gap * 3.5f, 110));
    drawNode->addChild(label);
    scaleLabelToFitWidth(label, gap - 4);

    // 对点
    label = Label::createWithSystemFont(d2 <= 8 ? std::string("8") : StringUtils::format("%d", d2), "Arail", 12);
    label->setColor(Color3B(0x60, 0x60, 0x60));
    label->setPosition(Vec2(gap * 4.5f, 110));
    drawNode->addChild(label);
    scaleLabelToFitWidth(label, gap - 4);

    // 旁点
    label = Label::createWithSystemFont(d1 <= 8 ? std::string("8") : StringUtils::format("%d", d1), "Arail", 12);
    label->setColor(Color3B(0x60, 0x60, 0x60));
    label->setPosition(Vec2(gap * 5.5f, 110));
    drawNode->addChild(label);
    scaleLabelToFitWidth(label, gap - 4);

    // 3位
    label = Label::createWithSystemFont(g_currentRecord.name[indices[2]], "Arail", 12);
    label->setColor(Color3B::ORANGE);
    label->setPosition(Vec2(gap * 0.5f, 60));
    drawNode->addChild(label);
    scaleLabelToFitWidth(label, gap - 4);

    // 追1位
    label = Label::createWithSystemFont(g_currentRecord.name[indices[0]], "Arail", 12);
    label->setColor(Color3B::ORANGE);
    label->setPosition(Vec2(gap * 1.5f, 70));
    drawNode->addChild(label);
    scaleLabelToFitWidth(label, gap - 4);

    delta = totalScores[indices[0]] - totalScores[indices[2]];
    d = delta - 32;
    d1 = d + 1;
    d2 = (d >> 1) + 1;
    d4 = (d >> 2) + 1;

    // 分差
    label = Label::createWithSystemFont(StringUtils::format("%d", delta), "Arail", 12);
    label->setColor(Color3B::ORANGE);
    label->setPosition(Vec2(gap * 2.5f, 70));
    drawNode->addChild(label);
    scaleLabelToFitWidth(label, gap - 4);

    // 自摸
    label = Label::createWithSystemFont(d4 <= 8 ? std::string("8") : StringUtils::format("%d", d4), "Arail", 12);
    label->setColor(Color3B(0x60, 0x60, 0x60));
    label->setPosition(Vec2(gap * 3.5f, 70));
    drawNode->addChild(label);
    scaleLabelToFitWidth(label, gap - 4);

    // 对点
    label = Label::createWithSystemFont(d2 <= 8 ? std::string("8") : StringUtils::format("%d", d2), "Arail", 12);
    label->setColor(Color3B(0x60, 0x60, 0x60));
    label->setPosition(Vec2(gap * 4.5f, 70));
    drawNode->addChild(label);
    scaleLabelToFitWidth(label, gap - 4);

    // 旁点
    label = Label::createWithSystemFont(d1 <= 8 ? std::string("8") : StringUtils::format("%d", d1), "Arail", 12);
    label->setColor(Color3B(0x60, 0x60, 0x60));
    label->setPosition(Vec2(gap * 5.5f, 70));
    drawNode->addChild(label);
    scaleLabelToFitWidth(label, gap - 4);

    // 追2位
    label = Label::createWithSystemFont(g_currentRecord.name[indices[1]], "Arail", 12);
    label->setColor(Color3B::ORANGE);
    label->setPosition(Vec2(gap * 1.5f, 50));
    drawNode->addChild(label);
    scaleLabelToFitWidth(label, gap - 4);

    delta = totalScores[indices[1]] - totalScores[indices[2]];
    d = delta - 32;
    d1 = d + 1;
    d2 = (d >> 1) + 1;
    d4 = (d >> 2) + 1;

    // 分差
    label = Label::createWithSystemFont(StringUtils::format("%d", delta), "Arail", 12);
    label->setColor(Color3B::ORANGE);
    label->setPosition(Vec2(gap * 2.5f, 50));
    drawNode->addChild(label);
    scaleLabelToFitWidth(label, gap - 4);

    // 自摸
    label = Label::createWithSystemFont(d4 <= 8 ? std::string("8") : StringUtils::format("%d", d4), "Arail", 12);
    label->setColor(Color3B(0x60, 0x60, 0x60));
    label->setPosition(Vec2(gap * 3.5f, 50));
    drawNode->addChild(label);
    scaleLabelToFitWidth(label, gap - 4);

    // 对点
    label = Label::createWithSystemFont(d2 <= 8 ? std::string("8") : StringUtils::format("%d", d2), "Arail", 12);
    label->setColor(Color3B(0x60, 0x60, 0x60));
    label->setPosition(Vec2(gap * 4.5f, 50));
    drawNode->addChild(label);
    scaleLabelToFitWidth(label, gap - 4);

    // 旁点
    label = Label::createWithSystemFont(d1 <= 8 ? std::string("8") : StringUtils::format("%d", d1), "Arail", 12);
    label->setColor(Color3B(0x60, 0x60, 0x60));
    label->setPosition(Vec2(gap * 5.5f, 50));
    drawNode->addChild(label);
    scaleLabelToFitWidth(label, gap - 4);

    // 2位
    label = Label::createWithSystemFont(g_currentRecord.name[indices[1]], "Arail", 12);
    label->setColor(Color3B::ORANGE);
    label->setPosition(Vec2(gap * 0.5f, 10));
    drawNode->addChild(label);
    scaleLabelToFitWidth(label, gap - 4);

    // 追1位
    label = Label::createWithSystemFont(g_currentRecord.name[indices[0]], "Arail", 12);
    label->setColor(Color3B::ORANGE);
    label->setPosition(Vec2(gap * 1.5f, 10));
    drawNode->addChild(label);
    scaleLabelToFitWidth(label, gap - 4);

    delta = totalScores[indices[0]] - totalScores[indices[1]];
    d = delta - 32;
    d1 = d + 1;
    d2 = (d >> 1) + 1;
    d4 = (d >> 2) + 1;

    // 分差
    label = Label::createWithSystemFont(StringUtils::format("%d", delta), "Arail", 12);
    label->setColor(Color3B::ORANGE);
    label->setPosition(Vec2(gap * 2.5f, 10));
    drawNode->addChild(label);
    scaleLabelToFitWidth(label, gap - 4);

    // 自摸
    label = Label::createWithSystemFont(d4 <= 8 ? std::string("8") : StringUtils::format("%d", d4), "Arail", 12);
    label->setColor(Color3B(0x60, 0x60, 0x60));
    label->setPosition(Vec2(gap * 3.5f, 10));
    drawNode->addChild(label);
    scaleLabelToFitWidth(label, gap - 4);

    // 对点
    label = Label::createWithSystemFont(d2 <= 8 ? std::string("8") : StringUtils::format("%d", d2), "Arail", 12);
    label->setColor(Color3B(0x60, 0x60, 0x60));
    label->setPosition(Vec2(gap * 4.5f, 10));
    drawNode->addChild(label);
    scaleLabelToFitWidth(label, gap - 4);

    // 旁点
    label = Label::createWithSystemFont(d1 <= 8 ? std::string("8") : StringUtils::format("%d", d1), "Arail", 12);
    label->setColor(Color3B(0x60, 0x60, 0x60));
    label->setPosition(Vec2(gap * 5.5f, 10));
    drawNode->addChild(label);
    scaleLabelToFitWidth(label, gap - 4);

    return drawNode;
}

void ScoreSheetScene::onPursuitButton(cocos2d::Ref *sender) {
    const char (&name)[4][255] = g_currentRecord.name;
    ui::Widget *rootWidget = nullptr;

    // 当当前一局比赛未结束时，显示快捷分差按钮
    if (std::none_of(std::begin(name), std::end(name), &isCStringEmpty)
        && g_currentRecord.current_index < 16) {
        rootWidget = ui::Widget::create();

        DrawNode *drawNode = createTable(_totalScores);
        const Size &drawNodeSize = drawNode->getContentSize();

        rootWidget->setContentSize(Size(drawNodeSize.width, drawNodeSize.height + 30));
        rootWidget->addChild(drawNode);
        drawNode->setPosition(Vec2(0, 30));
    }

    // 自定义分差输入
    ui::EditBox *editBox = ui::EditBox::create(Size(120.0f, 20.0f), ui::Scale9Sprite::create("source_material/btn_square_normal.png"));
    editBox->setInputFlag(ui::EditBox::InputFlag::SENSITIVE);
    editBox->setInputMode(ui::EditBox::InputMode::NUMERIC);
    editBox->setFontColor(Color4B::BLACK);
    editBox->setFontSize(12);
    editBox->setPlaceholderFontColor(Color4B::GRAY);
    editBox->setPlaceHolder("输入任意分差");

    if (rootWidget != nullptr) {
        rootWidget->addChild(editBox);
        const Size &rootSize = rootWidget->getContentSize();
        editBox->setPosition(Vec2(rootSize.width * 0.5f, 10.0f));
    }
    else {
        rootWidget = editBox;
    }

    // EditBox的代理
    std::shared_ptr<cw::EditBoxDelegate> delegate = std::make_shared<cw::EditBoxDelegate>(
        [](ui::EditBox *editBox) {
        const char *text = editBox->getText();
        if (!isCStringEmpty(text)) {
            int delta = atoi(text);
            showPursuit(delta);
        }
    });
    editBox->setDelegate(delegate.get());

    // 使这个代理随AlertView一起析构
    AlertView::showWithNode("追分策略", rootWidget, [editBox, delegate]() {
        const char *text = editBox->getText();
        if (!isCStringEmpty(text)) {
            int delta = atoi(text);
            showPursuit(delta);
        }
    }, nullptr);
}

void ScoreSheetScene::onScoreButton(cocos2d::Ref *sender, size_t idx) {
    const char (&name)[4][255] = g_currentRecord.name;
    if (std::any_of(std::begin(name), std::end(name), &isCStringEmpty)
        || g_currentRecord.current_index == 16) {
        return;
    }

    static const size_t cmpIdx[][3] = { { 1, 2, 3 }, { 0, 2, 3 }, { 0, 1, 3 }, { 0, 1, 2 } };

    ui::Widget *rootWidget = ui::Widget::create();
    rootWidget->setContentSize(Size(150.0f, 70.0f));

    for (int i = 0; i < 3; ++i) {
        size_t dst = cmpIdx[idx][i];
        int delta = _totalScores[idx] - _totalScores[dst];

        ui::Button *button = ui::Button::create("source_material/btn_square_selected.png", "source_material/btn_square_highlighted.png");
        button->setScale9Enabled(true);
        button->setContentSize(Size(150.0f, 20.0f));
        button->setTitleFontSize(12);
        if (delta > 0) {
            button->setTitleText(StringUtils::format("领先「%s」%d分", name[dst], delta));
        }
        else if (delta < 0) {
            button->setTitleText(StringUtils::format("落后「%s」%d分", name[dst], -delta));
        }
        else {
            button->setTitleText(StringUtils::format("与「%s」平分", name[dst]));
        }
        scaleLabelToFitWidth(button->getTitleLabel(), 148.0f);
        rootWidget->addChild(button);
        button->setPosition(Vec2(75.0f, 60.0f - i * 25.0f));
        button->addClickEventListener([delta](Ref *) {
            showPursuit(delta);
        });
    }

    AlertView::showWithNode(name[idx], rootWidget, nullptr, nullptr);
}
