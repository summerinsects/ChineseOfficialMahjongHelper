#include "ScoreSheetScene.h"
#include "Record.h"
#include "RecordScene.h"
#include "HistoryScene.h"
#include "../widget/AlertLayer.h"
#include "../common.h"
#include "../mahjong-algorithm/points_calculator.h"

USING_NS_CC;

static Record g_currentRecord;

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
            if (g_currentRecord.currentIndex == 16) {
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
    button->addClickEventListener([this](Ref *) {
        AlertLayer::showWithMessage("重置", "重置会清空当前已记录的信息，未打满北风北的记录将会丢失，确定要这样做吗？",
            std::bind(&ScoreSheetScene::reset, this), nullptr);
    });

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

    // 4个输入框及同位置的label
    for (int i = 0; i < 4; ++i) {
        _editBox[i] = ui::EditBox::create(Size(gap, 20.0f), ui::Scale9Sprite::create());
        _editBox[i]->setPosition(Vec2(gap * (i + 1.5f), 390));
        _editBox[i]->setFontColor(Color3B::YELLOW);
        _editBox[i]->setFontSize(12);
        node->addChild(_editBox[i]);

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

        label = Label::createWithSystemFont(row1Text[i], "Arail", 12);
        label->setPosition(Vec2(gap * (i + 0.5f), 350));
        node->addChild(label);
    }

    // 第4栏：累计
    label = Label::createWithSystemFont("累计", "Arail", 12);
    label->setColor(Color3B::YELLOW);
    label->setPosition(Vec2(gap * 0.5f, 330));
    node->addChild(label);

    label = Label::createWithSystemFont("备注", "Arail", 12);
    label->setPosition(Vec2(gap * 5.5f, 330));
    node->addChild(label);

    for (int i = 0; i < 4; ++i) {
        _totalLabel[i] = Label::createWithSystemFont("+0", "Arail", 12);
        _totalLabel[i]->setColor(Color3B::YELLOW);
        _totalLabel[i]->setPosition(Vec2(gap * (i + 1.5f), 330));
        node->addChild(_totalLabel[i]);
    }

    // 第5~20栏，东风东~北风北的计分
    for (int k = 0; k < 16; ++k) {
        const float y = 10 + (15 - k) * 20;

        // 东风东~北风北名字
        label = Label::createWithSystemFont(handNameText[k], "Arail", 12);
        label->setColor(Color3B::GRAY);
        label->setPosition(Vec2(gap * 0.5f, y));
        node->addChild(label);

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
        _detailButton[k]->setVisible(false);
    }

    // 从json中读，并恢复界面数据
    readFromJson();
    recover();
    return true;
}

void ScoreSheetScene::fillRow(size_t handIdx) {
    // 填入这一盘四位选手的得分
    for (int i = 0; i < 4; ++i) {
        _scoreLabels[handIdx][i]->setString(StringUtils::format("%+d", g_currentRecord.scores[handIdx][i]));
        _totalScores[i] += g_currentRecord.scores[handIdx][i];  // 更新总分
        if (g_currentRecord.scores[handIdx][i] > 0) {
            _scoreLabels[handIdx][i]->setColor(Color3B::RED);
        }
        else if (g_currentRecord.scores[handIdx][i] < 0) {
            _scoreLabels[handIdx][i]->setColor(Color3B::GREEN);
        }
        else {
            _scoreLabels[handIdx][i]->setColor(Color3B::GRAY);
        }
    }

    // 禁用并隐藏这一行的计分按钮
    _recordButton[handIdx]->setVisible(false);
    _recordButton[handIdx]->setEnabled(false);

    _detailButton[handIdx]->setVisible(true);
    _detailButton[handIdx]->setEnabled(true);

    // 选取标记的最大番种显示出来
    bool pointsNameVisible = false;
    if (g_currentRecord.pointsFlag[handIdx] != 0) {
        for (unsigned n = 0; n < 64; ++n) {
            if ((1ULL << n) & g_currentRecord.pointsFlag[handIdx]) {
                unsigned idx = n;
#if HAS_CONCEALED_KONG_AND_MELDED_KONG
                if (idx >= mahjong::POINT_TYPE::CONCEALED_KONG_AND_MELDED_KONG) {
                    ++idx;
                }
#endif
                _pointNameLabel[handIdx]->setString(mahjong::points_name[idx]);
                _pointNameLabel[handIdx]->setVisible(true);
                pointsNameVisible = true;
                break;
            }
        }
    }
    if (!pointsNameVisible) {
        _pointNameLabel[handIdx]->setVisible(false);
    }
}

void ScoreSheetScene::refreshStartTime() {
    char str[255] = "开始时间：";
    size_t len = strlen(str);
    strftime(str + len, sizeof(str) - len, "%Y-%m-%d %H:%M", localtime(&g_currentRecord.startTime));
    _timeLabel->setString(str);
}

void ScoreSheetScene::refreshEndTime() {
    char str[255] = "起止时间：";
    size_t len = strlen(str);
    len += strftime(str + len, sizeof(str) - len, "%Y-%m-%d %H:%M", localtime(&g_currentRecord.startTime));
    strcpy(str + len, " -- ");
    len += 4;
    strftime(str + len, sizeof(str) - len, "%Y-%m-%d %H:%M", localtime(&g_currentRecord.endTime));
    _timeLabel->setString(str);
}

void ScoreSheetScene::recover() {
    // 选手名字是否为空
    bool empty = false;
    for (int i = 0; i < 4; ++i) {
        if (g_currentRecord.name[i][0] == '\0') {
            empty = true;
            break;
        }
    }

    // 有选手名字为空，则清空数据
    if (empty) {
        memset(&g_currentRecord, 0, sizeof(g_currentRecord));
        onTimeScheduler(0.0f);
        this->schedule(schedule_selector(ScoreSheetScene::onTimeScheduler), 1.0f);
        return;
    }

    // 禁用和隐藏名字输入框，显示名字的label
    for (int i = 0; i < 4; ++i) {
        _editBox[i]->setVisible(false);
        _editBox[i]->setEnabled(false);
        _nameLabel[i]->setString(g_currentRecord.name[i]);
        _nameLabel[i]->setVisible(true);
    }

    // 禁用和隐藏锁定按钮
    _lockButton->setEnabled(false);
    _lockButton->setVisible(false);

    // 初始化总分
    memset(_totalScores, 0, sizeof(_totalScores));

    // 逐行填入数据
    for (size_t k = 0; k < g_currentRecord.currentIndex; ++k) {
        fillRow(k);
    }

    // 刷新总分label
    for (int i = 0; i < 4; ++i) {
        _totalLabel[i]->setString(StringUtils::format("%+d", _totalScores[i]));
    }

    // 如果不是北风北，则显示下一行的计分按钮
    if (g_currentRecord.currentIndex < 16) {
        _recordButton[g_currentRecord.currentIndex]->setVisible(true);
        _recordButton[g_currentRecord.currentIndex]->setEnabled(true);

        refreshStartTime();
    }
    else {
        refreshEndTime();
    }
}

void ScoreSheetScene::reset() {
    memset(&g_currentRecord, 0, sizeof(g_currentRecord));

    memset(_totalScores, 0, sizeof(_totalScores));

    for (int i = 0; i < 4; ++i) {
        _editBox[i]->setText("");
        _editBox[i]->setVisible(true);
        _editBox[i]->setEnabled(true);
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
    }
}

void ScoreSheetScene::onLockButton(cocos2d::Ref *sender) {
    for (int i = 0; i < 4; ++i) {
        const char *str = _editBox[i]->getText();
        if (str[0] == '\0') {
            AlertLayer::showWithMessage("锁定", "请先录入四位参赛选手姓名", nullptr, nullptr);
            return;
        }
        strncpy(g_currentRecord.name[i], str, sizeof(g_currentRecord.name[i]));
    }

    memset(_totalScores, 0, sizeof(_totalScores));

    for (int i = 0; i < 4; ++i) {
        _editBox[i]->setVisible(false);
        _editBox[i]->setEnabled(false);
        _nameLabel[i]->setVisible(true);
        _nameLabel[i]->setString(g_currentRecord.name[i]);
    }

    _recordButton[0]->setVisible(true);
    _recordButton[0]->setEnabled(true);
    _lockButton->setEnabled(false);
    _lockButton->setVisible(false);

    this->unschedule(schedule_selector(ScoreSheetScene::onTimeScheduler));

    g_currentRecord.startTime = time(nullptr);
    refreshStartTime();
}

void ScoreSheetScene::onRecordButton(cocos2d::Ref *sender, size_t handIdx) {
    const char *name[] = { g_currentRecord.name[0], g_currentRecord.name[1], g_currentRecord.name[2], g_currentRecord.name[3] };
    Director::getInstance()->pushScene(RecordScene::createScene(handIdx, name, [this, handIdx](RecordScene *scene) {
        if (handIdx != g_currentRecord.currentIndex) return;

        // 将计分面板的数据更新到当前数据中
        memcpy(g_currentRecord.scores[handIdx], scene->getScoreTable(), sizeof(g_currentRecord.scores[handIdx]));
        g_currentRecord.pointsFlag[handIdx] = scene->getPointsFlag();

        // 填入当前行
        fillRow(handIdx);

        // 更新总分
        for (int i = 0; i < 4; ++i) {
            _totalLabel[i]->setString(StringUtils::format("%+d", _totalScores[i]));
        }

        // 如果不是北风北，则显示下一行的计分按钮，否则一局结束，并增加新的历史记录
        if (++g_currentRecord.currentIndex < 16) {
            _recordButton[g_currentRecord.currentIndex]->setVisible(true);
            _recordButton[g_currentRecord.currentIndex]->setEnabled(true);
        }
        else {
            g_currentRecord.endTime = time(nullptr);
            refreshEndTime();
            HistoryScene::addRecord(g_currentRecord);
        }
        writeToJson();
    }));
}

void ScoreSheetScene::onDetailButton(cocos2d::Ref *sender, size_t handIdx) {
    std::string str;
    if (g_currentRecord.pointsFlag[handIdx] != 0) {
        for (unsigned n = 0; n < 64; ++n) {
            if ((1ULL << n) & g_currentRecord.pointsFlag[handIdx]) {
                unsigned idx = n;
#if HAS_CONCEALED_KONG_AND_MELDED_KONG
                if (idx >= mahjong::POINT_TYPE::CONCEALED_KONG_AND_MELDED_KONG) {
                    ++idx;
                }
#endif
                str.append(mahjong::points_name[idx]);
                str.append("\n");
            }
        }
        AlertLayer::showWithMessage(handNameText[handIdx], str, nullptr, nullptr);
    }
}

void ScoreSheetScene::onTimeScheduler(float dt) {
    char str[255] = "当前时间：";
    size_t len = strlen(str);
    time_t t = time(nullptr);
    strftime(str + len, sizeof(str) - len, "%Y-%m-%d %H:%M", localtime(&t));
    _timeLabel->setString(str);
}

void ScoreSheetScene::onPursuitButton(cocos2d::Ref *sender) {
    ui::EditBox *editBox = ui::EditBox::create(Size(100.0f, 20.0f), ui::Scale9Sprite::create("source_material/btn_square_normal.png"));
    editBox->setInputFlag(ui::EditBox::InputFlag::SENSITIVE);
    editBox->setInputMode(ui::EditBox::InputMode::NUMERIC);
    editBox->setFontColor(Color4B::BLACK);
    editBox->setFontSize(12);
    editBox->setPlaceholderFontColor(Color4B::GRAY);
    editBox->setPlaceHolder("输入分差");

    AlertLayer::showWithNode("追分计算", editBox, [editBox]() {
        int score = atoi(editBox->getText());
        std::string msg = StringUtils::format("分差%d，", score);
        if (score <= 0) {
            msg.append("无须追分");
        }
        else {
            int d1 = score - 32;
            if (d1 < 8) {
                msg.append("须任意和牌");
            }
            else {
                int d2 = d1 >> 1;
                if (d2 < 8) {
                    msg.append(StringUtils::format("须任意自摸或对点，旁点%d番", d1 + 1));
                }
                else {
                    int d4 = d2 >> 1;
                    if (d4 < 8) {
                        msg.append(StringUtils::format("须任意自摸，对点%d番，旁点%d番", d2 + 1, d1 + 1));
                    }
                    else {
                        msg.append(StringUtils::format("须自摸%d番，对点%d番，旁点%d番", d4 + 1, d2 + 1, d1 + 1));
                    }
                }
            }
        }
        AlertLayer::showWithMessage("追分计算", msg, nullptr, nullptr);
    }, nullptr);
}
