#include "ScoreSheetScene.h"
#include <algorithm>
#include <iterator>
#include "../mahjong-algorithm/stringify.h"
#include "UICommon.h"
#include "../widget/AlertDialog.h"
#include "../widget/Toast.h"
#include "../widget/HandTilesWidget.h"
#include "RecordScene.h"
#include "RecordHistoryScene.h"
#include "LittleFan.h"

USING_NS_CC;

static const Color3B C3B_RED = Color3B(254, 87, 110);
static const Color3B C3B_BLUE = Color3B(44, 121, 178);
static const Color3B C3B_GREEN = Color3B(49, 155, 28);
static const Color3B C3B_GRAY = Color3B(96, 96, 96);

static Record g_currentRecord;

static void readFromFile(Record &record) {
    std::string path = FileUtils::getInstance()->getWritablePath();
    path.append("record.json");
    ReadRecordFromFile(path.c_str(), record);
}

static void writeToFile(const Record &record) {
    std::string path = FileUtils::getInstance()->getWritablePath();
    path.append("record.json");
    WriteRecordToFile(path.c_str(), record);
}

bool ScoreSheetScene::init() {
    readFromFile(g_currentRecord);
    return ScoreSheetScene::initWithRecord(&g_currentRecord);
}

bool ScoreSheetScene::initWithRecord(Record *record) {
    if (UNLIKELY(!BaseScene::initWithTitle("国标麻将记分器"))) {
        return false;
    }

    memset(_totalScores, 0, sizeof(_totalScores));

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    // 用来绘制表格线的根结点
    DrawNode *drawNode = DrawNode::create();
    this->addChild(drawNode);

    const float gap = visibleSize.width / 6;  // 分成6份
    _cellWidth = gap;

    const int cellCount = 21;  // 姓名+开局+每圈+累计+16盘+名次=21行
    const float cellHeight = std::min<float>((visibleSize.height - 85.0f) / cellCount, 20.0f);
    const float tableHeight = cellHeight * cellCount;
    const float tableOffsetY = (visibleSize.height - 85.0f - tableHeight) * 0.5f + 25.0f;

    drawNode->setPosition(Vec2(origin.x, origin.y + tableOffsetY));

    // 上方4个按钮
    const float buttonGap = (visibleSize.width - 4.0f - 55.0f) / 3.0f;

    const float xPos = origin.x + 2.0f + 55.0f * 0.5f;
    const float yPos = origin.y + tableOffsetY + tableHeight + 15.0f;
    static const char *titleText[4] = { "追分策略", "清空表格", "历史记录", "使用说明" };
    static void (ScoreSheetScene::*callbacks[4])(Ref *) = {
        &ScoreSheetScene::onPursuitButton, &ScoreSheetScene::onResetButton, &ScoreSheetScene::onHistoryButton, &ScoreSheetScene::onInstructionButton
    };

    ui::Button *topButtons[4];
    for (int i = 0; i < 4; ++i) {
        ui::Button *button = UICommon::createButton();
        this->addChild(button);
        button->setScale9Enabled(true);
        button->setContentSize(Size(55.0f, 20.0f));
        button->setTitleFontSize(12);
        button->setTitleText(titleText[i]);
        button->setPosition(Vec2(xPos + buttonGap * i, yPos));
        button->addClickEventListener(std::bind(callbacks[i], this, std::placeholders::_1));
        cw::scaleLabelToFitWidth(button->getTitleLabel(), 50.0f);

        topButtons[i] = button;
    }
    topButtons[1]->setEnabled(record == &g_currentRecord);
    topButtons[2]->setEnabled(record == &g_currentRecord);

    // 底部时间label
    Label *label = Label::createWithSystemFont("当前时间", "Arial", 12);
    this->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(origin.x + 5.0f, origin.y + tableOffsetY - 12.0f));
    label->setColor(Color3B::BLACK);
    _timeLabel = label;

    // 5条竖线
    for (int i = 0; i < 5; ++i) {
        const float x = gap * (i + 1);
        drawNode->drawLine(Vec2(x, 0.0f), Vec2(x, tableHeight), Color4F::BLACK);
    }

    // cellCount+1条横线
    for (int i = 0; i < cellCount + 1; ++i) {
        const float y = cellHeight * i;
        drawNode->drawLine(Vec2(0.0f, y), Vec2(visibleSize.width, y),
            (i > 0 && i < 16) ? Color4F(0.3f, 0.3f, 0.3f, 1.0f) : Color4F::BLACK);
    }

    // 每一列中间位置
    const float colPosX[6] = { gap * 0.5f, gap * 1.5f, gap * 2.5f, gap * 3.5f, gap * 4.5f, gap * 5.5f };

    // 第1栏：选手姓名
    const float line1Y = tableHeight - cellHeight * 0.5f;
    label = Label::createWithSystemFont("选手姓名", "Arail", 12);
    label->setColor(Color3B::ORANGE);
    label->setPosition(Vec2(colPosX[0], line1Y));
    drawNode->addChild(label);
    cw::scaleLabelToFitWidth(label, gap - 4.0f);

    // 4个用于弹出输入框的AlertLayer及同位置的label
    // 这里不直接使用Button内部Label，因为内部Label在点击后会恢复scale
    for (int i = 0; i < 4; ++i) {
        ui::Widget *widget = ui::Widget::create();
        widget->setTouchEnabled(true);
        widget->setPosition(Vec2(colPosX[i + 1], line1Y));
        widget->setContentSize(Size(gap, cellHeight));
        drawNode->addChild(widget);
        widget->addClickEventListener(std::bind(&ScoreSheetScene::onNameButton, this, std::placeholders::_1, i));

        label = Label::createWithSystemFont("", "Arail", 12);
        label->setColor(Color3B::ORANGE);
        label->setPosition(Vec2(colPosX[i + 1], line1Y));
        drawNode->addChild(label);
        _nameLabel[i] = label;
    }

    ui::Button *button = UICommon::createButton();
    drawNode->addChild(button, -1);
    button->setScale9Enabled(true);
    button->setContentSize(Size(gap, cellHeight));
    button->setTitleFontSize(12);
    button->setTitleText("锁定");
    button->setPosition(Vec2(colPosX[5], line1Y));
    button->addClickEventListener(std::bind(&ScoreSheetScene::onLockButton, this, std::placeholders::_1));
    _lockButton = button;

    // 第2、3栏：座位
    const float line2Y = tableHeight - cellHeight * 1.5f;
    const float line3Y = tableHeight - cellHeight * 2.5f;
    static const char *row0Text[] = {"开局座位", "东", "南", "西", "北"};
    static const char *row1Text[] = {"每圈座位", "东南北西", "南东西北", "西北东南", "北西南东"};
    for (int i = 0; i < 5; ++i) {
        label = Label::createWithSystemFont(row0Text[i], "Arail", 12);
        label->setColor(Color3B::BLACK);
        label->setPosition(Vec2(colPosX[i], line2Y));
        drawNode->addChild(label);
        cw::scaleLabelToFitWidth(label, gap - 4.0f);

        label = Label::createWithSystemFont(row1Text[i], "Arail", 12);
        label->setColor(Color3B::BLACK);
        label->setPosition(Vec2(colPosX[i], line3Y));
        drawNode->addChild(label);
        cw::scaleLabelToFitWidth(label, gap - 4.0f);
    }

    // 第4栏：累计
    const float line4Y = tableHeight - cellHeight * 3.5f;
    label = Label::createWithSystemFont("累计", "Arail", 12);
    label->setColor(Color3B::ORANGE);
    label->setPosition(Vec2(colPosX[0], line4Y));
    drawNode->addChild(label);
    cw::scaleLabelToFitWidth(label, gap - 4.0f);

    for (int i = 0; i < 4; ++i) {
        label = Label::createWithSystemFont("+0", "Arail", 12);
        label->setColor(Color3B::ORANGE);
        label->setPosition(Vec2(colPosX[i + 1], line4Y));
        drawNode->addChild(label);
        _totalLabel[i] = label;

        ui::Widget *widget = ui::Widget::create();
        widget->setTouchEnabled(true);
        widget->setPosition(Vec2(colPosX[i + 1], line4Y));
        widget->setContentSize(Size(gap, cellHeight));
        drawNode->addChild(widget);
        widget->addClickEventListener(std::bind(&ScoreSheetScene::onScoreButton, this, std::placeholders::_1, i));
    }

    // 第5栏：名次
    const float line5Y = tableHeight - cellHeight * 4.5f;

    label = Label::createWithSystemFont("名次", "Arail", 12);
    label->setColor(Color3B::ORANGE);
    label->setPosition(Vec2(colPosX[0], line5Y));
    drawNode->addChild(label);
    cw::scaleLabelToFitWidth(label, gap - 4.0f);

    for (int i = 0; i < 4; ++i) {
        label = Label::createWithSystemFont("", "Arail", 12);
        label->setColor(Color3B::ORANGE);
        label->setPosition(Vec2(colPosX[i + 1], line5Y));
        drawNode->addChild(label);

        _rankLabels[i] = label;
    }

    label = Label::createWithSystemFont("番种备注", "Arail", 12);
    label->setColor(Color3B::BLACK);
    label->setPosition(Vec2(colPosX[5], line5Y));
    drawNode->addChild(label);
    cw::scaleLabelToFitWidth(label, gap - 4.0f);

    // 第6~21栏，东风东~北风北的计分
    for (int k = 0; k < 16; ++k) {
        const float y = tableHeight - cellHeight * (5.5f + k);

        // 东风东~北风北名字
        label = Label::createWithSystemFont(handNameText[k], "Arail", 12);
        label->setColor(C3B_GRAY);
        label->setPosition(Vec2(colPosX[0], y));
        drawNode->addChild(label);
        cw::scaleLabelToFitWidth(label, gap - 4.0f);

        // 四位选手得分
        for (int i = 0; i < 4; ++i) {
            _scoreLabels[k][i] = Label::createWithSystemFont("", "Arail", 12);
            _scoreLabels[k][i]->setPosition(Vec2(colPosX[i + 1], y));
            drawNode->addChild(_scoreLabels[k][i]);
        }

        // 计分按钮
        ui::Button *button = UICommon::createButton();
        drawNode->addChild(button, -1);
        button->setScale9Enabled(true);
        button->setContentSize(Size(gap, cellHeight));
        button->setTitleFontSize(12);
        button->setTitleText("计分");
        button->setPosition(Vec2(colPosX[5], y));
        button->addClickEventListener(std::bind(&ScoreSheetScene::onRecordButton, this, std::placeholders::_1, k));
        button->setEnabled(false);
        button->setVisible(false);
        _recordButton[k] = button;

        // 备注的番种label
        label = Label::createWithSystemFont("", "Arail", 12);
        label->setColor(C3B_GRAY);
        label->setPosition(Vec2(colPosX[5], y));
        drawNode->addChild(label);
        label->setVisible(false);
        _fanNameLabel[k] = label;

        // 查看详情按钮
        ui::Widget *widget = ui::Widget::create();
        drawNode->addChild(widget);
        widget->setTouchEnabled(true);
        widget->setContentSize(Size(gap, cellHeight));
        widget->setPosition(Vec2(colPosX[5], y));
        widget->addClickEventListener(std::bind(&ScoreSheetScene::onDetailButton, this, std::placeholders::_1, k));
        widget->setEnabled(false);
        _detailWidget[k] = widget;
    }

    // 恢复界面数据
    _isGlobal = (record == &g_currentRecord);
    memcpy(&_record, record, sizeof(_record));
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
    _detailWidget[handIdx]->setEnabled(false);
}

static const char *fan_short_name[] = {
    "",
    "大四喜", "大三元", "绿一色", "九莲", "四杠", "连七对", "十三幺",
    "清幺九", "小四喜", "小三元", "字一色", "四暗", "一色双龙",
    "一色四同顺", "一色四节",
    "一色四步", "三杠", "混幺九",
    "七对", "七星", "全双刻", "清一色", "一色三同顺", "一色三节", "全大", "全中", "全小",
    "清龙", "三色双龙", "一色三步", "全带五", "三同刻", "三暗",
    "全不靠", "组合龙", "大于五", "小于五", "三风刻",
    "花龙", "推不倒", "三色三同顺", "三色三节", "无番和", "妙手", "海底", "杠开", "抢杠",
    "碰碰和", "混一色", "三色三步", "五门", "全求人", "双暗杠", "双箭",
    "全带幺", "不求人", "双明杠", "绝张",
    "箭刻", "圈风", "门风", "门清", "平和", "四归一", "双同刻", "双暗", "暗杠", "断幺",
    "一般高", "喜相逢", "连六", "老少", "幺九", "明杠", "缺门", "无字", "边张", "嵌张", "单钓", "自摸",
    "花牌"
};

static int __get1bitscount(uint64_t v) {
    int cnt = 0;
    while (v) {
        v &= (v - 1);
        ++cnt;
    }
    return cnt;
}

static std::string GetShortFanText(const Record::Detail &detail) {
    if (detail.fan == 0) {
        return "荒庄";
    }

    uint64_t fanFlag = detail.fan_flag;
    if (fanFlag != 0) {
        if (__get1bitscount(fanFlag) > 1) {
            // 选取标记的最大的两个番种显示出来
            unsigned fan0 = 0, fan1 = 0;
            for (unsigned n = mahjong::BIG_FOUR_WINDS; n < mahjong::DRAGON_PUNG; ++n) {
                if (TEST_FAN(fanFlag, n)) {
                    fan0 = n;
                    break;
                }
            }
            for (unsigned n = fan0 + 1; n < mahjong::DRAGON_PUNG; ++n) {
                if (TEST_FAN(fanFlag, n)) {
                    fan1 = n;
                    break;
                }
            }

            // 「一色XXX」与「混一色」、「清一色」、「绿一色」合并「一色」二字
            // 1.「一色XXX」出现在fan0
            if (fan0 == mahjong::PURE_SHIFTED_CHOWS
                || fan0 == mahjong::FOUR_PURE_SHIFTED_CHOWS
                || fan0 == mahjong::PURE_SHIFTED_PUNGS
                || fan0 == mahjong::FOUR_PURE_SHIFTED_PUNGS
                || fan0 == mahjong::PURE_TRIPLE_CHOW
                || fan0 == mahjong::QUADRUPLE_CHOW) {
                if (fan1 == mahjong::HALF_FLUSH || fan1 == mahjong::FULL_FLUSH) {
                    std::string fanName0 = fan_short_name[fan0];
                    std::string fanName1 = fan_short_name[fan1];
                    return fanName1.append(fanName0.erase(0, strlen("一色")));
                }
                else {
                    // 太长的直接用最大番
                    return mahjong::fan_name[fan0];
                }
            }

            // 2.「一色XXX」出现在fan0
            if (fan1 == mahjong::PURE_SHIFTED_CHOWS
                || fan1 == mahjong::FOUR_PURE_SHIFTED_CHOWS
                || fan1 == mahjong::PURE_SHIFTED_PUNGS
                || fan1 == mahjong::FOUR_PURE_SHIFTED_PUNGS
                || fan1 == mahjong::PURE_TRIPLE_CHOW
                || fan1 == mahjong::QUADRUPLE_CHOW) {
                if (fan0 == mahjong::FULL_FLUSH || fan0 == mahjong::ALL_GREEN) {
                    std::string fanName0 = fan_short_name[fan0];
                    std::string fanName1 = fan_short_name[fan1];
                    return fanName0.append(fanName1.erase(0, strlen("一色")));
                }
                else {
                    // 太长的直接用最大番
                    return mahjong::fan_name[fan0];
                }
            }

            // 「三色XXX」
            // 1.「五门齐」与「三色XXX」可以省略「三色」二字，观察番种表，「五门齐」出现在「三色XXX」之后，故而只有一个判断
            if (fan0 == mahjong::MIXED_SHIFTED_CHOWS
                || fan0 == mahjong::MIXED_TRIPLE_CHOW
                || fan0 == mahjong::MIXED_SHIFTED_PUNGS) {
                if (fan1 == mahjong::ALL_TYPES) {
                    std::string fanName0 = fan_short_name[fan0];
                    std::string fanName1 = fan_short_name[fan1];
                    return fanName1.append(fanName0.erase(0, strlen("三色")));
                }
                else {
                    // 太长的直接用最大番
                    return mahjong::fan_name[fan0];
                }
            }

            // 2.「三色XXX」与更大的番复合，只显示最大的番
            if (fan1 == mahjong::MIXED_SHIFTED_CHOWS
                || fan1 == mahjong::MIXED_TRIPLE_CHOW
                || fan1 == mahjong::MIXED_SHIFTED_PUNGS) {
                return mahjong::fan_name[fan0];
            }

            // 「双龙会」系列不复合
            if (fan0 == mahjong::PURE_TERMINAL_CHOWS
                || fan0 == mahjong::THREE_SUITED_TERMINAL_CHOWS) {
                return mahjong::fan_name[fan0];
            }

            // 组合两个番名
            std::string fanName0 = fan_short_name[fan0];
            std::string fanName1 = fan_short_name[fan1];
            return fanName0.append(fanName1);
        }
        else {
            for (unsigned n = mahjong::BIG_FOUR_WINDS; n < mahjong::DRAGON_PUNG; ++n) {
                if (TEST_FAN(fanFlag, n)) {
                    return mahjong::fan_name[n];
                }
            }
        }
    }

    uint16_t uniqueFan = detail.unique_fan;
    uint64_t multipleFan = detail.multiple_fan;
    if (uniqueFan != 0 || multipleFan != 0) {
        if (TEST_UNIQUE_FAN(uniqueFan, UNIQUE_FAN_CONCEALED_HAND)
            && TEST_UNIQUE_FAN(uniqueFan, UNIQUE_FAN_ALL_SIMPLES)
            && TEST_UNIQUE_FAN(uniqueFan, UNIQUE_FAN_ALL_CHOWS)) {
            return "门断平";
        }

        std::string fanText;
        int fans = 0;
        for (unsigned n = 0; n < 8; ++n) {
            if (TEST_UNIQUE_FAN(uniqueFan, n)) {
                fanText.append(fan_short_name[uniqueFanTable[n]]);
                if (++fans > 1) {
                    break;
                }
            }
        }

        if (fanText.empty()) {
            for (unsigned n = 0; n < 2; ++n) {
                if (MULTIPLE_FAN_COUNT(multipleFan, n) > 0) {
                    fanText.append(fan_short_name[multipleFanTable[n]]);
                    if (++fans > 1) {
                        break;
                    }
                }
            }
        }

        if (fanText.empty()) {
            if (MULTIPLE_FAN_COUNT(multipleFan, 6) == 2) {
                fanText.append("双幺九");
            }
        }

        if (!fanText.empty()) {
            if (fans == 1) {
                fanText.append("凑番");
            }
            return fanText;
        }
    }

    return "未标记番种";
}

void ScoreSheetScene::fillRow(size_t handIdx) {
    int scoreTable[4];
    const Record::Detail &detail = _record.detail[handIdx];
    TranslateDetailToScoreTable(detail, scoreTable);

    // 填入这一盘四位选手的得分
    for (int i = 0; i < 4; ++i) {
        _scoreLabels[handIdx][i]->setString(Common::format("%+d", scoreTable[i]));
        _totalScores[i] += scoreTable[i];  // 更新总分
    }

    // 使用不同颜色
    RecordScene::SetScoreLabelColor(_scoreLabels[handIdx], scoreTable, detail.win_flag, detail.claim_flag, detail.penalty_scores);

    // 禁用并隐藏这一行的计分按钮
    _recordButton[handIdx]->setVisible(false);
    _recordButton[handIdx]->setEnabled(false);
    _detailWidget[handIdx]->setEnabled(true);

    Label *label = _fanNameLabel[handIdx];
    label->setString(GetShortFanText(detail));
    label->setVisible(true);
    cw::scaleLabelToFitWidth(label, _cellWidth - 4.0f);
}

void ScoreSheetScene::refreshRank() {
    unsigned rank[4] = {0};
    CalculateRankFromScore(_totalScores, rank);

    static const char *text[] = { "一", "二", "三", "四" };
    for (int i = 0; i < 4; ++i) {
        _rankLabels[i]->setVisible(true);
        _rankLabels[i]->setString(text[rank[i]]);
    }
}

void ScoreSheetScene::refreshStartTime() {
    struct tm ret = *localtime(&_record.start_time);
    _timeLabel->setString(Common::format("开始时间：%d年%d月%d日%.2d:%.2d",
        ret.tm_year + 1900, ret.tm_mon + 1, ret.tm_mday, ret.tm_hour, ret.tm_min));
    _timeLabel->setScale(1);
}

void ScoreSheetScene::refreshEndTime() {
    struct tm ret0 = *localtime(&_record.start_time);
    struct tm ret1 = *localtime(&_record.end_time);
    _timeLabel->setString(Common::format("起止时间：%d年%d月%d日%.2d:%.2d——%d年%d月%d日%.2d:%.2d",
        ret0.tm_year + 1900, ret0.tm_mon + 1, ret0.tm_mday, ret0.tm_hour, ret0.tm_min,
        ret1.tm_year + 1900, ret1.tm_mon + 1, ret1.tm_mday, ret1.tm_hour, ret1.tm_min));

    Size visibleSize = Director::getInstance()->getVisibleSize();
    cw::scaleLabelToFitWidth(_timeLabel, visibleSize.width - 10.0f);
}

void ScoreSheetScene::recover() {
    // 备份名字
    char name[4][NAME_SIZE];
    memcpy(name, _record.name, sizeof(name));

    // 显示名字的label
    for (int i = 0; i < 4; ++i) {
        _nameLabel[i]->setString(name[i]);
        _nameLabel[i]->setVisible(true);
        cw::scaleLabelToFitWidth(_nameLabel[i], _cellWidth - 4.0f);
    }

    // 如果开始时间为0，说明未锁定
    if (_record.start_time == 0) {
        memset(&_record, 0, sizeof(_record));
        memcpy(_record.name, name, sizeof(name)); // 恢复名字
        onTimeScheduler(0.0f);
        this->schedule(schedule_selector(ScoreSheetScene::onTimeScheduler), 1.0f);
        return;
    }

    // 禁用和隐藏锁定按钮
    _lockButton->setEnabled(false);
    _lockButton->setVisible(false);

    // 初始化总分
    memset(_totalScores, 0, sizeof(_totalScores));

    // 逐行填入数据
    for (size_t i = 0; i < _record.current_index; ++i) {
        fillRow(i);
    }
    for (size_t i = _record.current_index; i < 16; ++i) {
        cleanRow(i);
    }

    // 刷新总分和名次label
    for (int i = 0; i < 4; ++i) {
        _totalLabel[i]->setString(Common::format("%+d", _totalScores[i]));
        _rankLabels[i]->setVisible(false);
    }
    if (_record.current_index > 0) {
        refreshRank();
    }

    // 如果不是北风北，则显示下一行的计分按钮
    if (_record.current_index < 16) {
        _recordButton[_record.current_index]->setVisible(true);
        _recordButton[_record.current_index]->setEnabled(true);

        refreshStartTime();
    }
    else {
        refreshEndTime();
    }
    this->unschedule(schedule_selector(ScoreSheetScene::onTimeScheduler));
}

void ScoreSheetScene::reset() {
    // 如果选手姓名不为空，则保存上次对局的姓名
    if (std::all_of(std::begin(_record.name), std::end(_record.name), [](const char (&s)[NAME_SIZE]) { return s[0] != '\0'; })) {
        for (int i = 0; i < 4; ++i) {
            _prevName[i] = _record.name[i];
        }
    }

    memset(&_record, 0, sizeof(_record));
    if (_isGlobal) {
        writeToFile(_record);
    }

    memset(_totalScores, 0, sizeof(_totalScores));

    for (int i = 0; i < 4; ++i) {
        _nameLabel[i]->setVisible(false);
        _totalLabel[i]->setString("+0");
        _rankLabels[i]->setVisible(false);
    }

    _lockButton->setEnabled(true);
    _lockButton->setVisible(true);
    onTimeScheduler(0.0f);
    this->schedule(schedule_selector(ScoreSheetScene::onTimeScheduler), 1.0f);

    for (size_t i = 0; i < 16; ++i) {
        cleanRow(i);
    }
}

void ScoreSheetScene::onNameButton(cocos2d::Ref *, size_t idx) {
    if (_record.start_time == 0) {
        editNameAllAtOnce();
    }
    else {
        const char *message = (_record.current_index < 16) ? "对局已经开始，是否要修改选手姓名？" : "对局已经结束，是否要修改选手姓名？";
        AlertDialog::Builder(this)
            .setTitle("提示")
            .setMessage(message)
            .setNegativeButton("否", nullptr)
            .setPositiveButton("是", [this, idx](AlertDialog *, int) { editName(idx); return true; })
            .create()->show();
    }
}

static const char *s_wind[] = { "东", "南", "西", "北" };

namespace {
    class NameEditBoxDelegate : public cocos2d::ui::EditBoxDelegate {
        typedef std::function<void (cocos2d::ui::EditBox *, cocos2d::ui::EditBoxDelegate::EditBoxEndAction)> ccEditBoxEditingDidEndWithAction;
        ccEditBoxEditingDidEndWithAction _callback;

    public:
        NameEditBoxDelegate(ccEditBoxEditingDidEndWithAction &&callback) : _callback(callback) { }

        virtual ~NameEditBoxDelegate() { CCLOG("%s", __FUNCTION__); }

        virtual void editBoxReturn(cocos2d::ui::EditBox *) override { }

        virtual void editBoxEditingDidEndWithAction(cocos2d::ui::EditBox *editBox, EditBoxEndAction action) override {
            _callback(editBox, action);
        }
    };
}

void ScoreSheetScene::editName(size_t idx) {
    ui::EditBox *editBox = UICommon::createEditBox(Size(120.0f, 20.0f));
    editBox->setInputMode(ui::EditBox::InputMode::SINGLE_LINE);
    editBox->setInputFlag(ui::EditBox::InputFlag::SENSITIVE);
    editBox->setReturnType(ui::EditBox::KeyboardReturnType::DONE);
    editBox->setFontColor(Color3B::BLACK);
    editBox->setFontSize(12);
    editBox->setText(_record.name[idx]);
    editBox->setMaxLength(NAME_SIZE - 1);
    editBox->setPlaceholderFontColor(Color4B::GRAY);
    editBox->setPlaceHolder("输入选手姓名");

    auto delegate = std::make_shared<NameEditBoxDelegate>([this, idx](ui::EditBox *editBox, ui::EditBoxDelegate::EditBoxEndAction action) {
        if (action == ui::EditBoxDelegate::EditBoxEndAction::RETURN) {
            AlertDialog *dialog = (AlertDialog *)editBox->getUserData();
            if (submitName(editBox->getText(), idx)) {
                dialog->scheduleOnce([dialog](float) {
                    dialog->dismiss();
                }, 0.0f, "dismiss_dialog");
            }
        }
    });
    editBox->setDelegate(delegate.get());

    AlertDialog *dialog = AlertDialog::Builder(this)
        .setTitle(Common::format("开局座位「%s」", s_wind[idx]))
        .setContentNode(editBox)
        .setCloseOnTouchOutside(false)
        .setNegativeButton("取消", nullptr)
        .setPositiveButton("确定", [this, editBox, idx, delegate](AlertDialog *, int) {
        const char *text = editBox->getText();
        if (text != nullptr) {
            return submitName(text, idx);
        }
        return false;
    }).create();
    dialog->show();

    editBox->setUserData(dialog);

    // 自动打开editBox
    editBox->scheduleOnce([editBox](float) {
        editBox->touchDownAction(editBox, ui::Widget::TouchEventType::ENDED);
    }, 0.0f, "open_keyboard");
}

bool ScoreSheetScene::submitName(const char *text, size_t idx) {
    std::string name = text;
    Common::trim(name);

    // 开始后不允许清空名字
    if (name.empty()) {
        Toast::makeText(this, "对局开始后不允许清空名字", Toast::LENGTH_LONG)->show();
        return false;
    }

    if (name.length() > NAME_SIZE - 1) {
        name.erase(NAME_SIZE - 1);
    }

    for (size_t i = 0; i < 4; ++i) {
        if (i != idx && name.compare(_record.name[i]) == 0) {
            Toast::makeText(this, "选手姓名不能相同", Toast::LENGTH_LONG)->show();
            return false;
        }
    }

    strncpy(_record.name[idx], name.c_str(), NAME_SIZE - 1);
    _nameLabel[idx]->setVisible(true);
    _nameLabel[idx]->setString(name);
    cw::scaleLabelToFitWidth(_nameLabel[idx], _cellWidth - 4.0f);

    if (_record.current_index >= 16) {
        RecordHistoryScene::modifyRecord(&_record);
    }

    if (_isGlobal) {
        writeToFile(_record);
    }
    return true;
}

void ScoreSheetScene::editNameAllAtOnce() {
    const float limitWidth = std::min(AlertDialog::maxWidth(), 180.0f);

    Node *rootNode = Node::create();
    rootNode->setContentSize(Size(limitWidth, 125.0f));

    ui::Button *button1 = UICommon::createButton();
    rootNode->addChild(button1);
    button1->setScale9Enabled(true);
    button1->setContentSize(Size(55.0f, 20.0f));
    button1->setTitleFontSize(12);
    button1->setTitleText("清空全部");
    button1->setPosition(Vec2((limitWidth - 110.0f) / 3.0f + 27.5f, 115.0f));
    cw::scaleLabelToFitWidth(button1->getTitleLabel(), 50.0f);

    ui::Button *button2 = UICommon::createButton();
    rootNode->addChild(button2);
    button2->setScale9Enabled(true);
    button2->setContentSize(Size(55.0f, 20.0f));
    button2->setTitleFontSize(12);
    button2->setTitleText("随机排座");
    button2->setPosition(Vec2(limitWidth - button1->getPositionX(), 115.0f));
    cw::scaleLabelToFitWidth(button2->getTitleLabel(), 50.0f);

    // 输入框
    const float editBoxWidth = limitWidth - 20 - 50;
    const float editBoxPosX = editBoxWidth * 0.5f + 20.0f;
    std::array<ui::EditBox *, 4> editBoxes;
    for (int i = 0; i < 4; ++i) {
        const float yPos = 85.0f - i * 25.0f;
        Label *label = Label::createWithSystemFont(s_wind[i], "Arial", 12);
        label->setColor(Color3B::BLACK);
        rootNode->addChild(label);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        label->setPosition(Vec2(5.0f, yPos));

        ui::EditBox *editBox = UICommon::createEditBox(Size(editBoxWidth, 20.0f));
        editBox->setInputMode(ui::EditBox::InputMode::SINGLE_LINE);
        editBox->setInputFlag(ui::EditBox::InputFlag::SENSITIVE);
        editBox->setReturnType(ui::EditBox::KeyboardReturnType::NEXT);
        editBox->setFontColor(Color3B::BLACK);
        editBox->setFontSize(12);
        editBox->setText(_record.name[i]);
        editBox->setMaxLength(NAME_SIZE - 1);
        rootNode->addChild(editBox);
        editBox->setPosition(Vec2(editBoxPosX, yPos));

        editBoxes[i] = editBox;
    }

    // 上下移按钮
    const float upPosX = limitWidth - 10.0f;
    const float downPosX = limitWidth - 35.0f;
    for (int i = 0; i < 4; ++i) {
        const float yPos = 85.0f - i * 25.0f;
        ui::Button *button = UICommon::createButton();
        rootNode->addChild(button);
        button->setScale9Enabled(true);
        button->setContentSize(Size(20.0f, 20.0f));
        button->setTitleFontSize(12);
        button->setTitleText("\xE2\xAC\x86\xEF\xB8\x8E");
        button->setPosition(Vec2(upPosX, yPos));
        button->setEnabled(i != 0);
        button->addClickEventListener([editBoxes, i](Ref *) {
            std::string temp = editBoxes[i]->getText();
            editBoxes[i]->setText(editBoxes[i - 1]->getText());
            editBoxes[i - 1]->setText(temp.c_str());
        });

        button = UICommon::createButton();
        rootNode->addChild(button);
        button->setScale9Enabled(true);
        button->setContentSize(Size(20.0f, 20.0f));
        button->setTitleFontSize(12);
        button->setTitleText("\xE2\xAC\x87\xEF\xB8\x8E");
        button->setPosition(Vec2(downPosX, yPos));
        button->setEnabled(i != 3);
        button->addClickEventListener([editBoxes, i](Ref *) {
            std::string temp = editBoxes[i]->getText();
            editBoxes[i]->setText(editBoxes[i + 1]->getText());
            editBoxes[i + 1]->setText(temp.c_str());
        });
    }

    // 如果选手姓名皆为空，则填入上次对局的姓名
    if (std::all_of(std::begin(_record.name), std::end(_record.name), [](const char (&s)[NAME_SIZE]) { return s[0] == '\0'; })) {
        for (size_t i = 0; i < 4; ++i) {
            editBoxes[i]->setText(_prevName[i].c_str());
        }
    }

    // EditBox的代理，使得能连续输入
    auto delegate = std::make_shared<NameEditBoxDelegate>([editBoxes](ui::EditBox *editBox, ui::EditBoxDelegate::EditBoxEndAction action) {
        if (action == ui::EditBoxDelegate::EditBoxEndAction::TAB_TO_NEXT) {
            auto it = std::find(editBoxes.begin(), editBoxes.end(), editBox);
            if (it != editBoxes.end() && ++it != editBoxes.end()) {
                editBox = *it;
                editBox->scheduleOnce([editBox](float) {
                    editBox->touchDownAction(editBox, cocos2d::ui::Widget::TouchEventType::ENDED);
                }, 0.0f, "open_keyboard");
            }
        }
    });

    editBoxes[0]->setDelegate(delegate.get());
    editBoxes[1]->setDelegate(delegate.get());
    editBoxes[2]->setDelegate(delegate.get());

    AlertDialog::Builder(this)
        .setTitle("输入选手姓名")
        .setContentNode(rootNode)
        .setCloseOnTouchOutside(false)
        .setNegativeButton("取消", nullptr)
        .setPositiveButton("确定", [this, editBoxes, delegate](AlertDialog *, int) {
        // 获取四个输入框内容
        std::string names[4];
        for (int i = 0; i < 4; ++i) {
            const char *text = editBoxes[i]->getText();
            if (text != nullptr) {
                std::string &name = names[i];
                name = text;
                Common::trim(name);

                if (name.length() > NAME_SIZE - 1) {
                    name.erase(NAME_SIZE - 1);
                }
            }
        }

        // 检查重名
        for (size_t i = 0; i < 4; ++i) {
            for (size_t k = i + 1; k < 4; ++k) {
                if (!names[k].empty() && names[k].compare(names[i]) == 0) {
                    Toast::makeText(this, "选手姓名不能相同", Toast::LENGTH_LONG)->show();
                    return false;
                }
            }
        }

        // 提交
        for (int i = 0; i < 4; ++i) {
            strncpy(_record.name[i], names[i].c_str(), NAME_SIZE - 1);
            _nameLabel[i]->setVisible(true);
            _nameLabel[i]->setString(names[i]);
            cw::scaleLabelToFitWidth(_nameLabel[i], _cellWidth - 4.0f);
        }

        if (_record.current_index >= 16) {
            RecordHistoryScene::modifyRecord(&_record);
        }

        if (_isGlobal) {
            writeToFile(_record);
        }
        return true;
    }).create()->show();

    // 清空全部
    button1->addClickEventListener([editBoxes](Ref *) {
        for (ui::EditBox *editBox : editBoxes) {
            editBox->setText("");
        }
    });

    // 重新定庄
    button2->addClickEventListener([this, editBoxes](Ref *) {
        // 获取四个输入框内容
        std::string names[4];
        for (int i = 0; i < 4; ++i) {
            const char *text = editBoxes[i]->getText();
            if (text != nullptr) {
                std::string &name = names[i];
                name = text;
                Common::trim(name);

                if (name.length() > NAME_SIZE - 1) {
                    name.erase(NAME_SIZE - 1);
                }
            }
            if (names[i].empty()) {
                Toast::makeText(this, "请先输入四位参赛选手姓名", Toast::LENGTH_LONG)->show();
                return;
            }
        }

        srand(static_cast<unsigned>(time(nullptr)));
        std::random_shuffle(std::begin(names), std::end(names));
        for (int i = 0; i < 4; ++i) {
            editBoxes[i]->setText(names[i].c_str());
        }
    });
}

void ScoreSheetScene::onLockButton(cocos2d::Ref *) {
    const char (&name)[4][NAME_SIZE] = _record.name;
    if (std::any_of(std::begin(name), std::end(name), &Common::isCStringEmpty)) {
        Toast::makeText(this, "请先输入四位参赛选手姓名", Toast::LENGTH_LONG)->show();
        return;
    }

    memset(_totalScores, 0, sizeof(_totalScores));

    for (int i = 0; i < 4; ++i) {
        _nameLabel[i]->setVisible(true);
        _nameLabel[i]->setString(name[i]);
        cw::scaleLabelToFitWidth(_nameLabel[i], _cellWidth - 4.0f);
    }

    _recordButton[0]->setVisible(true);
    _recordButton[0]->setEnabled(true);
    _lockButton->setEnabled(false);
    _lockButton->setVisible(false);

    this->unschedule(schedule_selector(ScoreSheetScene::onTimeScheduler));

    _record.start_time = time(nullptr);
    refreshStartTime();

    if (_isGlobal) {
        writeToFile(_record);
    }
}

void ScoreSheetScene::onRecordButton(cocos2d::Ref *, size_t handIdx) {
    editRecord(handIdx, false);
}

void ScoreSheetScene::editRecord(size_t handIdx, bool modify) {
    const std::array<const char *, 4> name = { _record.name[0], _record.name[1], _record.name[2], _record.name[3] };
    auto scene = RecordScene::create(handIdx, name, modify ? &_record.detail[handIdx] : nullptr,
        [this, handIdx](const Record::Detail &detail) {
        bool isModify = (handIdx != _record.current_index);

        // 更新数据时，要先删除原来的记录
        if (isModify) {
            int scores[4];
            TranslateDetailToScoreTable(_record.detail[handIdx], scores);
            for (int i = 0; i < 4; ++i) {
                _totalScores[i] -= scores[i];
            }
        }

        // 将计分面板的数据更新到当前数据中
        memcpy(&_record.detail[handIdx], &detail, sizeof(Record::Detail));

        // 填入当前行
        fillRow(handIdx);

        // 更新总分
        for (int i = 0; i < 4; ++i) {
            _totalLabel[i]->setString(Common::format("%+d", _totalScores[i]));
        }

        // 更新名次
        refreshRank();

        if (isModify) {
            if (_record.end_time != 0) {
                RecordHistoryScene::modifyRecord(&_record);
            }
        }
        else {
            // 如果不是北风北，则显示下一行的计分按钮，否则一局结束，并增加新的历史记录
            if (++_record.current_index < 16) {
                _recordButton[_record.current_index]->setVisible(true);
                _recordButton[_record.current_index]->setEnabled(true);
            }
            else {
                _record.end_time = time(nullptr);
                refreshEndTime();
                RecordHistoryScene::modifyRecord(&_record);
            }
        }

        if (_isGlobal) {
            writeToFile(_record);
        }
    });
    Director::getInstance()->pushScene(scene);
}

static std::string GetLongFanText(const Record::Detail &detail) {
    std::string fanText;

    uint64_t fanFlag = detail.fan_flag;
    uint16_t uniqueFan = detail.unique_fan;
    uint64_t multipleFan = detail.multiple_fan;

    // 大番
    if (fanFlag != 0) {
        for (unsigned n = mahjong::BIG_FOUR_WINDS; n < mahjong::DRAGON_PUNG; ++n) {
            if (TEST_FAN(fanFlag, n)) {
                fanText.append("「");
                fanText.append(mahjong::fan_name[n]);
                fanText.append("」");
            }
        }
    }

    // 小番
    if (uniqueFan != 0 || multipleFan != 0) {
        uint16_t littleFanTable[25] = { 0 };

        for (unsigned n = 0; n < 14; ++n) {
            if (TEST_UNIQUE_FAN(uniqueFan, n)) {
                int idx = static_cast<int>(uniqueFanTable[n]) - static_cast<int>(mahjong::DRAGON_PUNG);
                littleFanTable[idx] = 1;
            }
        }

        for (unsigned n = 0; n < 9; ++n) {
            uint16_t cnt = MULTIPLE_FAN_COUNT(multipleFan, n);
            if (cnt > 0) {
                int idx = static_cast<int>(multipleFanTable[n]) - static_cast<int>(mahjong::DRAGON_PUNG);
                littleFanTable[idx] = cnt;
            }
        }

        for (int i = 0; i < 25; ++i) {
            uint16_t cnt = littleFanTable[i];
            if (cnt > 0) {
                fanText.append("「");
                fanText.append(mahjong::fan_name[static_cast<int>(mahjong::DRAGON_PUNG) + i]);
                if (cnt > 1) {
                    fanText.append("x");
                    fanText.append(std::to_string(cnt));
                }
                fanText.append("」");

            }
        }
    }

    if (!fanText.empty()) {
        fanText.append("等");
    }

    return fanText;
}

static std::string stringifyDetail(const Record *record, size_t handIdx) {
    const Record::Detail &detail = record->detail[handIdx];

    std::string fanText = GetLongFanText(detail);

    std::string ret;

    int8_t wf = detail.win_flag;
    int8_t cf = detail.claim_flag;
    int winIndex = WIN_CLAIM_INDEX(wf);
    int claimIndex = WIN_CLAIM_INDEX(cf);
    if (winIndex == claimIndex) {
        ret.append(Common::format("「%s」自摸%s%hu番。\n", record->name[winIndex], fanText.c_str(), detail.fan));
    }
    else {
        ret.append(Common::format("「%s」和%s%hu番，「%s」点炮。\n", record->name[winIndex], fanText.c_str(), detail.fan, record->name[claimIndex]));
    }

    return ret;
}

void ScoreSheetScene::onDetailButton(cocos2d::Ref *, size_t handIdx) {
    const Record::Detail &detail = _record.detail[handIdx];
    if (detail.fan == 0) {
        AlertDialog::Builder(this)
            .setTitle(std::string(handNameText[handIdx]).append("详情"))
            .setMessage("荒庄。\n\n是否需要修改这盘的记录？")
            .setNegativeButton("取消", nullptr)
            .setPositiveButton("确定", [this, handIdx](AlertDialog *, int) {editRecord(handIdx, true); return true; })
            .create()->show();
        return;
    }

    std::string message = stringifyDetail(&_record, handIdx);
    message.append("\n是否需要修改这盘的记录？");

    if (Common::isCStringEmpty(detail.win_hand.tiles)) {
        AlertDialog::Builder(this)
            .setTitle(std::string(handNameText[handIdx]).append("详情"))
            .setMessage(message)
            .setNegativeButton("取消", nullptr)
            .setPositiveButton("确定", [this, handIdx](AlertDialog *, int) {editRecord(handIdx, true); return true; })
            .create()->show();
        return;
    }

    const Record::Detail::WinHand &win_hand = detail.win_hand;
    mahjong::hand_tiles_t hand_tiles;
    mahjong::tile_t win_tile;
    mahjong::string_to_tiles(detail.win_hand.tiles, &hand_tiles, &win_tile);

    const float maxWidth = AlertDialog::maxWidth();

    // 花（使用emoji代码）
    Label *flowerLabel = nullptr;
    if (win_hand.flower_count > 0) {
        flowerLabel = Label::createWithSystemFont(std::string(EMOJI_FLOWER_8, win_hand.flower_count * (sizeof(EMOJI_FLOWER) - 1)), "Arial", 12);
        flowerLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
#if CC_TARGET_PLATFORM == CC_PLATFORM_WIN32
        flowerLabel->setColor(Color3B(224, 45, 45));
#endif
    }

    // 手牌
    Node *tilesNode = HandTilesWidget::createStaticNode(hand_tiles, win_tile);
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
        label->setDimensions(maxWidth, 0.0f);
    }
    const Size &labelSize = label->getContentSize();

    Node *container = Node::create();
    if (detail.win_hand.flower_count > 0) {
        const Size &flowerSize = flowerLabel->getContentSize();
        container->setContentSize(Size(maxWidth, labelSize.height + 10 + tilesNodeSize.height + 5 + flowerSize.height));

        container->addChild(flowerLabel);
        flowerLabel->setPosition(Vec2(0, labelSize.height + 10 + tilesNodeSize.height + 5 + flowerSize.height * 0.5f));
    }
    else {
        container->setContentSize(Size(maxWidth, labelSize.height + 10 + tilesNodeSize.height));
    }

    container->addChild(tilesNode);
    tilesNode->setPosition(Vec2(maxWidth * 0.5f, labelSize.height + 10 + tilesNodeSize.height * 0.5f));

    container->addChild(label);
    label->setPosition(Vec2(maxWidth * 0.5f, labelSize.height * 0.5f));

    AlertDialog::Builder(this)
        .setTitle(std::string(handNameText[handIdx]).append("详情"))
        .setContentNode(container)
        .setNegativeButton("取消", nullptr)
        .setPositiveButton("确定", [this, handIdx](AlertDialog *, int) {editRecord(handIdx, true); return true; })
        .create()->show();
}

void ScoreSheetScene::onTimeScheduler(float) {
    time_t t = time(nullptr);
    struct tm ret = *localtime(&t);
    _timeLabel->setString(Common::format("当前时间：%d年%d月%d日%.2d:%.2d",
        ret.tm_year + 1900, ret.tm_mon + 1, ret.tm_mday, ret.tm_hour, ret.tm_min));
    _timeLabel->setScale(1);
}

void ScoreSheetScene::onInstructionButton(cocos2d::Ref *) {
    Label *label = Label::createWithSystemFont(
        "1. 使用步骤：点击「选手姓名」一栏，输入四名选手姓名，点击「锁定」，开始「记分」。\n"
        "2. 计分时如果有标记番种，则「番种备注」一栏会选取一个最大的番种名予以显示。\n"
        "3. 对于已经记分的，点击「番种备注」一栏可修改记录。\n"
        "4. 对局未完成时，点击「累计」一栏处，可显示分差并有快捷计算追分选项。\n"
        "5. 「北风北」记分完成后，会自动添加入「历史记录」。\n"
        "6. 「历史记录」里的内容只要不卸载程序就会一直保存。",
        "Arial", 10, Size(AlertDialog::maxWidth(), 0.0f));
    label->setColor(Color3B::BLACK);

    AlertDialog::Builder(this)
        .setTitle("使用说明")
        .setContentNode(label)
        .setPositiveButton("确定", nullptr)
        .create()->show();
}

void ScoreSheetScene::onHistoryButton(cocos2d::Ref *) {
    Director::getInstance()->pushScene(RecordHistoryScene::create([this](Record *record) {
        if (UNLIKELY(g_currentRecord.start_time == record->start_time)) {  // 我们认为开始时间相同的为同一个记录
            Director::getInstance()->popScene();
        }
        else {
            auto scene = new (std::nothrow) ScoreSheetScene();
            scene->initWithRecord(record);
            scene->autorelease();
            Director::getInstance()->pushScene(scene);
        }
    }));
}

void ScoreSheetScene::onResetButton(cocos2d::Ref *) {
    // 未开始或已结束时，随便清空
    if (_record.start_time == 0 || _record.current_index == 16) {
        reset();
        return;
    }

    Node *rootNode = Node::create();
    rootNode->setContentSize(Size(200.0f, 70.0f));

    Label *label = Label::createWithSystemFont("在清空表格之前，请选择「保存」或「丢弃」", "Arail", 12);
    rootNode->addChild(label);
    label->setPosition(Vec2(100.0f, 60.0f));
    label->setColor(C3B_GRAY);
    cw::scaleLabelToFitWidth(label, 200.0f);

    ui::RadioButtonGroup *radioGroup = ui::RadioButtonGroup::create();
    rootNode->addChild(radioGroup);

    ui::RadioButton *radioButton = UICommon::createRadioButton();
    radioButton->setZoomScale(0.0f);
    radioButton->ignoreContentAdaptWithSize(false);
    radioButton->setContentSize(Size(20.0f, 20.0f));
    radioButton->setPosition(Vec2(10.0f, 60.0f - 1 * 25.0f));
    rootNode->addChild(radioButton);
    radioGroup->addRadioButton(radioButton);

    label = Label::createWithSystemFont("保存，将未打完盘数标记为荒庄", "Arial", 12);
    label->setColor(Color3B::BLACK);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    cw::scaleLabelToFitWidth(label, 175.0f);
    radioButton->addChild(label);
    label->setPosition(Vec2(25.0f, 10.0f));

    radioButton = UICommon::createRadioButton();
    radioButton->setZoomScale(0.0f);
    radioButton->ignoreContentAdaptWithSize(false);
    radioButton->setContentSize(Size(20.0f, 20.0f));
    radioButton->setPosition(Vec2(10.0f, 60.0f - 2 * 25.0f));
    rootNode->addChild(radioButton);
    radioGroup->addRadioButton(radioButton);

    label = Label::createWithSystemFont("丢弃，不保存当前一局记录", "Arial", 12);
    label->setColor(Color3B::BLACK);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    cw::scaleLabelToFitWidth(label, 175.0f);
    radioButton->addChild(label);
    label->setPosition(Vec2(25.0f, 10.0f));

    AlertDialog::Builder(this)
        .setTitle("清空表格")
        .setContentNode(rootNode)
        .setCloseOnTouchOutside(false)
        .setNegativeButton("取消", nullptr)
        .setPositiveButton("确定", [this, radioGroup](AlertDialog *, int) {
        if (radioGroup->getSelectedButtonIndex() == 0) {
            _record.current_index = 16;
            _record.end_time = time(nullptr);
            RecordHistoryScene::modifyRecord(&_record);
        }
        reset();
        return true;
    }).create()->show();
}

static void showPursuit(int delta) {
    if (delta == 0) {
        AlertDialog::Builder(Director::getInstance()->getRunningScene())
            .setTitle("追分与保位")
            .setMessage("平分")
            .setPositiveButton("确定", nullptr)
            .create()->show();
        return;
    }

    std::string msg;
    msg.reserve(256);
    if (delta < 0) {
        delta = -delta;
    }
    msg.append(Common::format("分差%d分\n\n超分需", delta));

    int d1 = delta - 32;
    if (d1 < 8) {
        msg.append("任意和牌");
    }
    else {
        int d2 = d1 >> 1;
        if (d2 < 8) {
            msg.append(Common::format("任意自摸或对点，旁点至少%d番", d1 + 1));
        }
        else {
            int d4 = d2 >> 1;
            if (d4 < 8) {
                msg.append(Common::format("任意自摸，对点至少%d番，旁点至少%d番", d2 + 1, d1 + 1));
            }
            else {
                msg.append(Common::format("自摸至少%d番，对点至少%d番，旁点至少%d番", d4 + 1, d2 + 1, d1 + 1));
            }
        }
    }

    if (delta <= 8) {
        msg.append("\n\n点炮无法保位");
    }
    else {
        int d2 = d1 >> 1;
        if (d2 <= 8) {
            msg.append(Common::format("\n\n对点无法保位，保位可旁点至多%d番", delta - 1));
        }
        else {
            msg.append(Common::format("\n\n保位可对点至多%d番，旁点至多%d番", (d1 & 1) ? d2 : d2 - 1, delta - 1));
        }
    }

    AlertDialog::Builder(Director::getInstance()->getRunningScene())
        .setTitle("追分与保位")
        .setMessage(msg)
        .setPositiveButton("确定", nullptr)
        .create()->show();
}

static DrawNode *createPursuitTable(const char (&name)[4][NAME_SIZE], const int (&totalScores)[4]) {
    // 下标
    int indices[4] = { 0, 1, 2, 3 };
    // 按分数排序
    std::stable_sort(std::begin(indices), std::end(indices), [&totalScores](int a, int b) {
        return totalScores[a] > totalScores[b];
    });

    const float width = AlertDialog::maxWidth();
    const float height = 10 * 20;

    // 列宽
    const float colWidth[6] = {
        width * 0.2f, width * 0.2f, width * 0.15f, width * 0.15f, width * 0.15f, width * 0.15f
    };
    // 中心位置
    float xPos[6];
    cw::calculateColumnsCenterX(colWidth, 6, xPos);

    DrawNode *drawNode = DrawNode::create();
    drawNode->setContentSize(Size(width, height));

    // 横线
    const float startX[11] = { 0, colWidth[0], colWidth[0], 0, 0, colWidth[0], 0, 0, 0, 0, 0 };
    for (int i = 0; i < 11; ++i) {
        drawNode->drawLine(Vec2(startX[i], 20.0f * i), Vec2(width, 20.0f * i), Color4F::BLACK);
    }

    // 竖线
    drawNode->drawLine(Vec2(0.0f, 0.0f), Vec2(0.0f, 200.0f), Color4F::BLACK);
    for (int i = 0; i < 5; ++i) {
        const float x = xPos[i] + colWidth[i] * 0.5f;
        drawNode->drawLine(Vec2(x, 0.0f), Vec2(x, 60.0f), Color4F::BLACK);
        drawNode->drawLine(Vec2(x, 80.0f), Vec2(x, 120.0f), Color4F::BLACK);
        drawNode->drawLine(Vec2(x, 140.0f), Vec2(x, 160.0f), Color4F::BLACK);
        drawNode->drawLine(Vec2(x, 180.0f), Vec2(x, 200.0f), Color4F::BLACK);
    }
    drawNode->drawLine(Vec2(width, 0.0f), Vec2(width, 200.0f), Color4F::BLACK);

    static const char *titleText[] = { "追者", "被追", "分差", "自摸", "对点", "旁点" };
    static const Color3B titleColor[] = { Color3B::ORANGE, Color3B::ORANGE, C3B_GRAY, C3B_RED, C3B_BLUE, C3B_GREEN };

    for (int i = 0; i < 6; ++i) {
        Label *label = Label::createWithSystemFont(titleText[i], "Arail", 12);
        label->setColor(titleColor[i]);
        label->setPosition(Vec2(xPos[i], 190.0f));
        drawNode->addChild(label);
        cw::scaleLabelToFitWidth(label, colWidth[i] - 4.0f);
    }

    const float nameY[3] = { 150, 100, 30 };
    const float numbersStartY[3] = { 150, 110, 50 };

    // n位。n=1表示二位，以此类推
    for (int n = 1; n < 4; ++n) {
        Label *label = Label::createWithSystemFont(name[indices[n]], "Arail", 12);
        label->setColor(titleColor[0]);
        label->setPosition(Vec2(xPos[0], nameY[n - 1]));
        drawNode->addChild(label);
        cw::scaleLabelToFitWidth(label, colWidth[0] - 4.0f);

        // 追k位。k=0表示1位，以此类推
        for (int k = 0; k < n; ++k) {
            const float posY = numbersStartY[n - 1] - k * 20;

            label = Label::createWithSystemFont(name[indices[k]], "Arail", 12);
            label->setColor(titleColor[1]);
            label->setPosition(Vec2(xPos[1], posY));
            drawNode->addChild(label);
            cw::scaleLabelToFitWidth(label, colWidth[1] - 4.0f);

            int delta = totalScores[indices[k]] - totalScores[indices[n]];
            int d = delta - 32;
            int numbers[4];
            numbers[0] = delta;  // 分差
            numbers[1] = std::max((d >> 2) + 1, 8);  // 自摸
            numbers[2] = std::max((d >> 1) + 1, 8);  // 对点
            numbers[3] = std::max(d + 1, 8);  // 旁点

            for (int i = 0; i < 4; ++i) {
                label = Label::createWithSystemFont(std::to_string(numbers[i]), "Arail", 12);
                label->setColor(titleColor[i + 2]);
                label->setPosition(Vec2(xPos[2 + i], posY));
                drawNode->addChild(label);
                cw::scaleLabelToFitWidth(label, colWidth[2 + i] - 4.0f);
            }
        }
    }

    return drawNode;
}

namespace {
    class PursuitEditBoxDelegate : public cocos2d::ui::EditBoxDelegate {
    public:
        virtual void editBoxReturn(cocos2d::ui::EditBox *editBox) override {
            const char *text = editBox->getText();
            if (!Common::isCStringEmpty(text)) {
                int delta = atoi(text);
                showPursuit(delta);
            }
        }
    };
}

void ScoreSheetScene::onPursuitButton(cocos2d::Ref *) {
    const char (&name)[4][NAME_SIZE] = _record.name;
    Node *rootNode = nullptr;

    // 当当前一局比赛未结束时，显示快捷分差按钮
    if (std::none_of(std::begin(name), std::end(name), &Common::isCStringEmpty)
        && _record.current_index < 16) {
        rootNode = Node::create();

        DrawNode *drawNode = createPursuitTable(name, _totalScores);
        const Size &drawNodeSize = drawNode->getContentSize();

        rootNode->setContentSize(Size(drawNodeSize.width, drawNodeSize.height + 30.0f));
        rootNode->addChild(drawNode);
        drawNode->setPosition(Vec2(0.0f, 30.0f));
    }

    // 自定义分差输入
    ui::EditBox *editBox = UICommon::createEditBox(Size(120.0f, 20.0f));
    editBox->setInputFlag(ui::EditBox::InputFlag::SENSITIVE);
    editBox->setInputMode(ui::EditBox::InputMode::NUMERIC);
    editBox->setReturnType(ui::EditBox::KeyboardReturnType::DONE);
    editBox->setFontColor(Color4B::BLACK);
    editBox->setFontSize(12);
    editBox->setPlaceholderFontColor(Color4B::GRAY);
    editBox->setPlaceHolder("输入任意分差");

    if (rootNode != nullptr) {
        rootNode->addChild(editBox);
        const Size &rootSize = rootNode->getContentSize();
        editBox->setPosition(Vec2(rootSize.width * 0.5f, 10.0f));
    }
    else {
        rootNode = editBox;
    }

    // EditBox的代理
    auto delegate = std::make_shared<PursuitEditBoxDelegate>();
    editBox->setDelegate(delegate.get());

    // 使这个代理随AlertDialog一起析构
    AlertDialog::Builder(this)
        .setTitle("追分策略")
        .setContentNode(rootNode)
        .setCloseOnTouchOutside(false)
        .setNegativeButton("取消", nullptr)
        .setPositiveButton("确定", [editBox, delegate](AlertDialog *, int) {
        const char *text = editBox->getText();
        if (!Common::isCStringEmpty(text)) {
            int delta = atoi(text);
            showPursuit(delta);
        }
        return false;
    }).create()->show();
}

void ScoreSheetScene::onScoreButton(cocos2d::Ref *, size_t idx) {
    const char (&name)[4][NAME_SIZE] = _record.name;
    if (std::any_of(std::begin(name), std::end(name), &Common::isCStringEmpty)
        || _record.current_index == 16) {
        return;
    }

    static const size_t cmpIdx[][3] = { { 1, 2, 3 }, { 0, 2, 3 }, { 0, 1, 3 }, { 0, 1, 2 } };

    Node *rootNode = Node::create();
    rootNode->setContentSize(Size(150.0f, 70.0f));

    for (int i = 0; i < 3; ++i) {
        size_t dst = cmpIdx[idx][i];
        int delta = _totalScores[idx] - _totalScores[dst];

        ui::Button *button = UICommon::createButton();
        button->setScale9Enabled(true);
        button->setContentSize(Size(150.0f, 20.0f));
        button->setTitleFontSize(12);
        if (delta > 0) {
            button->setTitleText(Common::format("领先「%s」%d分", name[dst], delta));
        }
        else if (delta < 0) {
            button->setTitleText(Common::format("落后「%s」%d分", name[dst], -delta));
        }
        else {
            button->setTitleText(Common::format("与「%s」平分", name[dst]));
        }
        cw::scaleLabelToFitWidth(button->getTitleLabel(), 148.0f);
        rootNode->addChild(button);
        button->setPosition(Vec2(75.0f, 60.0f - i * 25.0f));
        button->addClickEventListener([delta](Ref *) {
            showPursuit(delta);
        });
    }

    AlertDialog::Builder(this)
        .setTitle(name[idx])
        .setContentNode(rootNode)
        .setPositiveButton("确定", nullptr)
        .create()->show();
}
