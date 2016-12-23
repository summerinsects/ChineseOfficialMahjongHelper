#include "ScoreSheetScene.h"
#include "Record.h"
#include "RecordScene.h"
#include "HistoryScene.h"
#include "../widget/AlertLayer.h"
#include "../widget/CWEditBoxDelegate.h"
#include "../common.h"
#include "../mahjong-algorithm/points_calculator.h"

USING_NS_CC;

static Record g_currentRecord;

static inline bool isCStringEmpty(const char *str) {
    return *str == '\0';
}

static void readFromJson() {
    std::string fileName = FileUtils::getInstance()->getWritablePath();
    fileName.append("record.json");
    std::string str = FileUtils::getInstance()->getStringFromFile(fileName);
    CCLOG("%s", str.c_str());
    try {
        jw::cppJSON json;
        json.Parse(str.c_str());
        fromJson(&g_currentRecord, json);
    }
    catch (std::exception &e) {
        CCLOG("%s %s", __FUNCTION__, e.what());
    }
}

static void writeToJson() {
    try {
        jw::cppJSON json(jw::cppJSON::ValueType::Object);
        toJson(g_currentRecord, &json);

        std::string str = json.stringfiy();
        CCLOG("%s", str.c_str());

        std::string path = FileUtils::getInstance()->getWritablePath();
        path.append("record.json");
        FILE *file = fopen(path.c_str(), "wb");
        if (file != nullptr) {
            fwrite(str.c_str(), 1, str.length(), file);
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
    if (!BaseLayer::initWithTitle("国标麻将记分器")) {
        return false;
    }

    memset(_totalScores, 0, sizeof(_totalScores));

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    // 历史记录按钮
    ui::Button *button = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(55.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleColor(Color3B::BLACK);
    button->setTitleText("历史记录");
    button->setPosition(Vec2(origin.x + visibleSize.width - 28, origin.y + visibleSize.height - 12));
    button->addClickEventListener([this](Ref *) {
        Director::getInstance()->pushScene(HistoryScene::createScene([this](const Record &record) {
            if (g_currentRecord.current_index == 16) {
                CCLOG("currentIndex == 16");
                memcpy(&g_currentRecord, &record, sizeof(g_currentRecord));
                recover();
            }
        }));
    });

    // 重置按钮
    button = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(55.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleColor(Color3B::BLACK);
    button->setTitleText("重置");
    button->setPosition(Vec2(origin.x + visibleSize.width - 28, origin.y + visibleSize.height - 35));
    button->addClickEventListener(std::bind(&ScoreSheetScene::onResetButton, this, std::placeholders::_1));

    // 追分计算按钮
    button = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(55.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleColor(Color3B::BLACK);
    button->setTitleText("追分计算");
    button->setPosition(Vec2(origin.x + 28, origin.y + visibleSize.height - 40));
    button->addClickEventListener(std::bind(&ScoreSheetScene::onPursuitButton, this, std::placeholders::_1));

    // 时间label
    _timeLabel = Label::createWithSystemFont("当前时间", "Arial", 12);
    this->addChild(_timeLabel);
    _timeLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    _timeLabel->setPosition(Vec2(origin.x + 5, (origin.y + visibleSize.height - 430) * 0.5f - 10));

    // 用来绘制表格线的根结点
    DrawNode *node = DrawNode::create();
    this->addChild(node);
    node->setPosition(Vec2(origin.x, (origin.y + visibleSize.height - 430) * 0.5f));

    const float gap = visibleSize.width / 6;  // 分成6份
    _cellWidth = gap;

    // 5条竖线
    for (int i = 0; i < 5; ++i) {
        const float x = gap * (i + 1);
        node->drawLine(Vec2(x, 0.0f), Vec2(x, 400.0f), Color4F::WHITE);
    }

    // 21条横线
    for (int i = 0; i < 21; ++i) {
        const float y = 20.0f * i;
        node->drawLine(Vec2(0.0f, y), Vec2(visibleSize.width, y),
            (i > 0 && i < 16) ? Color4F::GRAY : Color4F::WHITE);
    }

    // 第1栏：选手姓名
    Label *label = Label::createWithSystemFont("选手姓名", "Arail", 12);
    label->setColor(Color3B::YELLOW);
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
        _nameLabel[i]->setColor(Color3B::YELLOW);
        _nameLabel[i]->setPosition(Vec2(gap * (i + 1.5f), 390));
        node->addChild(_nameLabel[i]);
    }

    _lockButton = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
    node->addChild(_lockButton);
    _lockButton->setScale9Enabled(true);
    _lockButton->setContentSize(Size(gap, 20.0f));
    _lockButton->setTitleFontSize(12);
    _lockButton->setTitleColor(Color3B::BLACK);
    _lockButton->setTitleText("锁定");
    _lockButton->setPosition(Vec2(gap * 5.5f, 390));
    _lockButton->addClickEventListener(std::bind(&ScoreSheetScene::onLockButton, this, std::placeholders::_1));

    // 第2、3栏：座位
    const char *row0Text[] = {"开局座位", "东", "南", "西", "北"};
    const char *row1Text[] = {"每圈座位", "东南北西", "南东西北", "西北东南", "北西南东"};
    for (int i = 0; i < 5; ++i) {
        label = Label::createWithSystemFont(row0Text[i], "Arail", 12);
        label->setPosition(Vec2(gap * (i + 0.5f), 370));
        node->addChild(label);
        scaleLabelToFitWidth(label, gap - 4);

        label = Label::createWithSystemFont(row1Text[i], "Arail", 12);
        label->setPosition(Vec2(gap * (i + 0.5f), 350));
        node->addChild(label);
        scaleLabelToFitWidth(label, gap - 4);
    }

    // 第4栏：累计
    label = Label::createWithSystemFont("累计", "Arail", 12);
    label->setColor(Color3B::YELLOW);
    label->setPosition(Vec2(gap * 0.5f, 330));
    node->addChild(label);
    scaleLabelToFitWidth(label, gap - 4);

    label = Label::createWithSystemFont("备注", "Arail", 12);
    label->setPosition(Vec2(gap * 5.5f, 330));
    node->addChild(label);
    scaleLabelToFitWidth(label, gap - 4);

    for (int i = 0; i < 4; ++i) {
        _totalLabel[i] = Label::createWithSystemFont("+0", "Arail", 12);
        _totalLabel[i]->setColor(Color3B::YELLOW);
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
        label->setColor(Color3B::GRAY);
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
        _recordButton[k] = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
        node->addChild(_recordButton[k]);
        _recordButton[k]->setScale9Enabled(true);
        _recordButton[k]->setContentSize(Size(gap, 20.0f));
        _recordButton[k]->setTitleFontSize(12);
        _recordButton[k]->setTitleColor(Color3B::BLACK);
        _recordButton[k]->setTitleText("计分");
        _recordButton[k]->setPosition(Vec2(gap * 5.5f, y));
        _recordButton[k]->addClickEventListener(std::bind(&ScoreSheetScene::onRecordButton, this, std::placeholders::_1, k));
        _recordButton[k]->setEnabled(false);
        _recordButton[k]->setVisible(false);

        // 备注的番种label
        _pointNameLabel[k] = Label::createWithSystemFont("", "Arail", 12);
        _pointNameLabel[k]->setColor(Color3B::GRAY);
        _pointNameLabel[k]->setPosition(Vec2(gap * 5.5f, y));
        node->addChild(_pointNameLabel[k]);
        _pointNameLabel[k]->setVisible(false);

        // 查看详情按钮
        _detailButton[k] = ui::Button::create();
        node->addChild(_detailButton[k]);
        _detailButton[k]->setScale9Enabled(true);
        _detailButton[k]->setContentSize(Size(gap, 20.0f));
        _detailButton[k]->setTitleFontSize(12);
        _detailButton[k]->setPosition(Vec2(gap * 5.5f, y));
        _detailButton[k]->addClickEventListener(std::bind(&ScoreSheetScene::onDetailButton, this, std::placeholders::_1, k));
        _detailButton[k]->setEnabled(false);
    }

    // 从json中读，并恢复界面数据
    readFromJson();
    recover();
    return true;
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
            _scoreLabels[handIdx][i]->setColor(Color3B::RED);
        }
        else if (scoreTable[i] < 0) {
            _scoreLabels[handIdx][i]->setColor(Color3B::GREEN);
        }
        else {
            _scoreLabels[handIdx][i]->setColor(Color3B::GRAY);
        }
    }

    // 禁用并隐藏这一行的计分按钮
    _recordButton[handIdx]->setVisible(false);
    _recordButton[handIdx]->setEnabled(false);
    _detailButton[handIdx]->setEnabled(true);

    Label *label = _pointNameLabel[handIdx];
    bool pointsNameVisible = false;
    if (detail.score == 0) {
        label->setString("荒庄");
        label->setVisible(true);
        pointsNameVisible = true;
    }

    // 选取标记的最大番种显示出来

    uint64_t pointsFlag = detail.points_flag;
    if (pointsFlag != 0) {
        for (unsigned n = 0; n < 64; ++n) {
            if ((1ULL << n) & pointsFlag) {
                unsigned idx = n;
#if HAS_CONCEALED_KONG_AND_MELDED_KONG
                if (idx >= mahjong::POINT_TYPE::CONCEALED_KONG_AND_MELDED_KONG) {
                    ++idx;
                }
#endif
                label->setString(mahjong::points_name[idx]);
                label->setVisible(true);
                pointsNameVisible = true;
                break;
            }
        }
    }
    if (!pointsNameVisible) {
        label->setVisible(false);
    }
    else {
        scaleLabelToFitWidth(label, _cellWidth - 4);
    }
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
    for (size_t k = 0; k < g_currentRecord.current_index; ++k) {
        fillRow(k);
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

    for (int k = 0; k < 16; ++k) {
        for (int i = 0; i < 4; ++i) {
            _scoreLabels[k][i]->setString("");
        }
        _recordButton[k]->setVisible(false);
        _recordButton[k]->setEnabled(false);
        _pointNameLabel[k]->setVisible(false);
        _detailButton[k]->setEnabled(false);
    }
}

void ScoreSheetScene::onNameButton(cocos2d::Ref *sender, size_t idx) {
    if (_lockButton->isVisible() && _lockButton->isEnabled()) {
        editName(idx);
    }
    else {
        AlertLayer::showWithMessage("提示", "对局已经开始，是否要修改选手姓名？",
            std::bind(&ScoreSheetScene::editName, this, idx), nullptr);
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
    AlertLayer::showWithNode(title, editBox, [this, editBox, idx]() {
        const char *text = editBox->getText();
        if (*text != '\0') {
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
        AlertLayer::showWithMessage("锁定", "请先录入四位参赛选手姓名", nullptr, nullptr);
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
                HistoryScene::addRecord(g_currentRecord);
            }
        }
        writeToJson();
    });
    Director::getInstance()->pushScene(scene);
}

void ScoreSheetScene::onDetailButton(cocos2d::Ref *sender, size_t handIdx) {
    std::string message = handNameText[handIdx], str;
    const Record::Detail &detail = g_currentRecord.detail[handIdx];
    if (detail.score == 0) {
        message.append("荒庄。\n");
    }
    else {
        int wc = detail.win_claim;
        int winIndex = (wc & 0x80) ? 3 : (wc & 0x40) ? 2 : (wc & 0x20) ? 1 : (wc & 0x10) ? 0 : -1;
        int claimIndex = (wc & 0x8) ? 3 : (wc & 0x4) ? 2 : (wc & 0x2) ? 1 : (wc & 0x1) ? 0 : -1;
        if (winIndex == claimIndex) {
            message.append("「");
            message.append(g_currentRecord.name[winIndex]);
            message.append(StringUtils::format("」自摸%d番。\n", detail.score));
        }
        else {
            message.append("「");
            message.append(g_currentRecord.name[winIndex]);
            message.append(StringUtils::format("」和%d番，「", detail.score));
            message.append(g_currentRecord.name[claimIndex]);
            message.append("」放炮。\n");
        }

        uint64_t pointsFlag = detail.points_flag;
        if (pointsFlag != 0) {
            for (unsigned n = 0; n < 64; ++n) {
                if ((1ULL << n) & pointsFlag) {
                    unsigned idx = n;
#if HAS_CONCEALED_KONG_AND_MELDED_KONG
                    if (idx >= mahjong::POINT_TYPE::CONCEALED_KONG_AND_MELDED_KONG) {
                        ++idx;
                    }
#endif
                    if (!str.empty()) {
                        str.append("、");
                    }
                    str.append(mahjong::points_name[idx]);
                }
            }
        }

        if (!str.empty()) {
            message.append("和出番种：");
            message.append(str);
            message.append("。\n");
        }
    }

    if (detail.false_win != 0) {
        str.clear();
        for (int i = 0; i < 4; ++i) {
            if (detail.false_win & (1 << i)) {
                if (!str.empty()) {
                    str.append("、");
                }
                str.append(g_currentRecord.name[i]);
            }
        }
        message.append(str);
        message.append("错和。\n");
    }

    message.append("\n是否需要修改这盘的记录？");

    str = handNameText[handIdx];
    str.append("详情");
    AlertLayer::showWithMessage(str, message,
        std::bind(&ScoreSheetScene::editRecord, this, handIdx, true), nullptr);
}

void ScoreSheetScene::onTimeScheduler(float dt) {
    char str[255] = "当前时间：";
    size_t len = strlen(str);
    time_t t = time(nullptr);
    strftime(str + len, sizeof(str) - len, "%Y-%m-%d %H:%M", localtime(&t));
    _timeLabel->setString(str);
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

    AlertLayer::showWithMessage("重置", "重置操作会清空当前已记录的信息，未打满北风北的记录将会丢失，确定要这样做吗？",
        std::bind(&ScoreSheetScene::reset, this), nullptr);
}

static void showPursuit(int delta) {
    std::string msg;
    msg.reserve(128);
    if (delta == 0) {
        msg = "平分";
    }
    else {
        if (delta < 0) {
            delta = -delta;
            msg.append(StringUtils::format("领先%d分，对手超分需：", delta));
        }
        else {
            msg.append(StringUtils::format("落后%d分，超分需：", delta));
        }

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
    }
    AlertLayer::showWithMessage("追分计算", msg, nullptr, nullptr);
}

void ScoreSheetScene::onPursuitButton(cocos2d::Ref *sender) {
    const char (&name)[4][255] = g_currentRecord.name;
    ui::Widget *rootWidget = nullptr;

    // 当当前一局比赛未结束时，显示快捷分差按钮
    if (std::none_of(std::begin(name), std::end(name), &isCStringEmpty)
        && g_currentRecord.current_index < 16) {
        rootWidget = ui::Widget::create();
        rootWidget->setContentSize(Size(150.0f, 200.0f));

        Label *label = Label::createWithSystemFont("快捷选择当前局面分差", "Arial", 12);
        label->setColor(Color3B::BLACK);
        rootWidget->addChild(label);
        label->setPosition(Vec2(75.0f, 190.0f));

        static std::pair<int, int> pairwise[6] = {
            std::make_pair(0, 1), std::make_pair(0, 2), std::make_pair(0, 3),
            std::make_pair(1, 2), std::make_pair(1, 3), std::make_pair(2, 3),
        };

        for (int i = 0; i < 6; ++i) {
            int delta = _totalScores[pairwise[i].first] - _totalScores[pairwise[i].second];

            ui::Button *button = ui::Button::create("source_material/btn_square_selected.png", "source_material/btn_square_highlighted.png");
            button->setScale9Enabled(true);
            button->setContentSize(Size(150.0f, 20.0f));
            button->setTitleFontSize(12);
            if (delta > 0) {
                button->setTitleText(StringUtils::format("「%s」领先「%s」%d分", name[pairwise[i].first], name[pairwise[i].second], delta));
            }
            else if (delta < 0) {
                delta = -delta;
                button->setTitleText(StringUtils::format("「%s」落后「%s」%d分", name[pairwise[i].first], name[pairwise[i].second], delta));
            }
            else {
                button->setTitleText(StringUtils::format("「%s」与「%s」平分", name[pairwise[i].first], name[pairwise[i].second]));
            }
            scaleLabelToFitWidth(button->getTitleRenderer(), 148.0f);
            rootWidget->addChild(button);
            button->setPosition(Vec2(75.0f, 170.0f - i * 25.0f));
            button->addClickEventListener([delta](Ref *) {
                showPursuit(delta);
            });
        }
    }

    // 自定义分差输入
    ui::EditBox *editBox = ui::EditBox::create(Size(100.0f, 20.0f), ui::Scale9Sprite::create("source_material/btn_square_normal.png"));
    editBox->setInputFlag(ui::EditBox::InputFlag::SENSITIVE);
    editBox->setInputMode(ui::EditBox::InputMode::NUMERIC);
    editBox->setFontColor(Color4B::BLACK);
    editBox->setFontSize(12);
    editBox->setPlaceholderFontColor(Color4B::GRAY);
    editBox->setPlaceHolder("输入任意分差");

    if (rootWidget != nullptr) {
        rootWidget->addChild(editBox);
        editBox->setPosition(Vec2(75.0f, 10.0f));
    }
    else {
        rootWidget = editBox;
    }

    // EditBox的代理
    std::shared_ptr<cw::EditBoxDelegate> delegate = std::make_shared<cw::EditBoxDelegate>(
        [](ui::EditBox *editBox) {
        const char *text = editBox->getText();
        if (*text != '\0') {
            int delta = atoi(text);
            showPursuit(delta);
        }
    });
    editBox->setDelegate(delegate.get());

    // 使这个代理随AlertLayer一起析构
    AlertLayer::showWithNode("追分计算", rootWidget, [editBox, delegate]() {
        const char *text = editBox->getText();
        if (*text != '\0') {
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
        scaleLabelToFitWidth(button->getTitleRenderer(), 148.0f);
        rootWidget->addChild(button);
        button->setPosition(Vec2(75.0f, 60.0f - i * 25.0f));
        button->addClickEventListener([delta](Ref *) {
            showPursuit(-delta);
        });
    }

    AlertLayer::showWithNode(name[idx], rootWidget, nullptr, nullptr);
}
