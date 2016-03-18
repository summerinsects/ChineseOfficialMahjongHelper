#include "ScoreSheetScene.h"
#include "RecordScene.h"
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
#define PRId64 "lld"
#endif
#include "../wheels/cppJSON.hpp"

#pragma execution_character_set("utf-8")

USING_NS_CC;

static char g_name[4][255];
static int g_scores[16][4];
static int g_currentIndex;
static time_t g_startTime;
static time_t g_endTime;

Scene *ScoreSheetScene::createScene() {
    auto scene = Scene::create();
    auto layer = ScoreSheetScene::create();
    scene->addChild(layer);
    return scene;
}

bool ScoreSheetScene::init() {
    if (!Layer::init()) {
        return false;
    }

    auto listener = EventListenerKeyboard::create();
    listener->onKeyReleased = CC_CALLBACK_2(Layer::onKeyReleased, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);

    memset(_scores, 0, sizeof(_scores));

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    Label *tileLabel = Label::createWithSystemFont("国标麻将记分器", "Arial", 24);
    this->addChild(tileLabel);
    tileLabel->setPosition(Vec2(origin.x + visibleSize.width * 0.5f,
        origin.y + visibleSize.height - tileLabel->getContentSize().height * 0.5f));

    ui::Button *backBtn = ui::Button::create();
    this->addChild(backBtn);
    backBtn->setScale9Enabled(true);
    backBtn->setContentSize(Size(45, 20));
    backBtn->setTitleFontSize(16);
    backBtn->setTitleText("返回");
    backBtn->setPosition(Vec2(origin.x + 15, origin.y + visibleSize.height - 10));
    backBtn->addClickEventListener([](Ref *) {
        Director::getInstance()->popScene();
    });

    ui::Button *button = ui::Button::create();
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(45.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText("重置");
    button->setPosition(Vec2(origin.x + visibleSize.width - 23, origin.y + visibleSize.height - 35));
    button->addClickEventListener([this](Ref *) { reset(); });

    _timeLabel = Label::createWithSystemFont("当前时间", "Arial", 12);
    this->addChild(_timeLabel);
    _timeLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    _timeLabel->setPosition(Vec2(origin.x + 5, (origin.y + visibleSize.height - 430) * 0.5f - 10));

    DrawNode *node = DrawNode::create();
    this->addChild(node);
    node->setPosition(Vec2(origin.x, (origin.y + visibleSize.height - 430) * 0.5f));

    const float gap = visibleSize.width / 6;

    for (int i = 0; i < 5; ++i) {
        const float x = gap * (i + 1);
        node->drawLine(Vec2(x, 0.0f), Vec2(x, 400.0f), Color4F::WHITE);
    }

    for (int i = 0; i < 21; ++i) {
        const float y = 20.0f * i;
        node->drawLine(Vec2(0.0f, y), Vec2(visibleSize.width, y), Color4F::WHITE);
    }

    Label *label = Label::createWithSystemFont("选手姓名", "Arail", 12);
    label->setPosition(Vec2(gap * 0.5f, 390));
    node->addChild(label);

    for (int i = 0; i < 4; ++i) {
        _editBox[i] = ui::EditBox::create(Size(gap, 20.0f), ui::Scale9Sprite::create());
        _editBox[i]->setPosition(Vec2(gap * (i + 1.5f), 390));
        _editBox[i]->setFontSize(12);
        node->addChild(_editBox[i]);

        _nameLabel[i] = Label::createWithSystemFont("", "Arail", 12);
        _nameLabel[i]->setPosition(Vec2(gap * (i + 1.5f), 390));
        node->addChild(_nameLabel[i]);
    }

    _lockButton = ui::Button::create();
    node->addChild(_lockButton);
    _lockButton->setScale9Enabled(true);
    _lockButton->setContentSize(Size(gap, 20.0f));
    _lockButton->setTitleFontSize(12);
    _lockButton->setTitleText("锁定");
    _lockButton->setPosition(Vec2(gap * 5.5f, 390));
    _lockButton->addClickEventListener(std::bind(&ScoreSheetScene::lockCallback, this, std::placeholders::_1));

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

    label = Label::createWithSystemFont("累计", "Arail", 12);
    label->setPosition(Vec2(gap * 0.5f, 330));
    node->addChild(label);

    for (int i = 0; i < 4; ++i) {
        _totalLabel[i] = Label::createWithSystemFont("0", "Arail", 12);
        _totalLabel[i]->setPosition(Vec2(gap * (i + 1.5f), 330));
        node->addChild(_totalLabel[i]);
    }

    const char *nameText[] = {"东风东", "东风南", "东风西", "东风北", "南风东", "南风南", "南风西", "南风北",
        "西风东", "西风南", "西风西", "西风北", "北风东", "北风南", "北风西", "北风北"};
    for (int k = 0; k < 16; ++k) {
        const float y = 10 + (15 - k) * 20;
        label = Label::createWithSystemFont(nameText[k], "Arail", 12);
        label->setPosition(Vec2(gap * 0.5f, y));
        node->addChild(label);

        for (int i = 0; i < 4; ++i) {
            _scoreLabels[k][i] = Label::createWithSystemFont("", "Arail", 12);
            _scoreLabels[k][i]->setPosition(Vec2(gap * (i + 1.5f), y));
            node->addChild(_scoreLabels[k][i]);
        }

        _recordButton[k] = ui::Button::create();
        node->addChild(_recordButton[k]);
        _recordButton[k]->setScale9Enabled(true);
        _recordButton[k]->setContentSize(Size(gap, 20.0f));
        _recordButton[k]->setTitleFontSize(12);
        _recordButton[k]->setTitleText("计分");
        _recordButton[k]->setPosition(Vec2(gap * 5.5f, y));
        _recordButton[k]->addClickEventListener(std::bind(&ScoreSheetScene::recordCallback, this, std::placeholders::_1, k));

        _recordButton[k]->setVisible(false);
    }

    recover();
    return true;
}

void ScoreSheetScene::onKeyReleased(EventKeyboard::KeyCode keyCode, Event *unusedEvent) {
    if (keyCode == EventKeyboard::KeyCode::KEY_BACK) {
        Director::getInstance()->popScene();
    }
}

static void readFromJson() {
    char str[1024] = "";
    std::string path = FileUtils::getInstance()->getWritablePath();
    path.append("record.json");
    FILE *file = fopen(path.c_str(), "rb");
    if (file == nullptr) {
        return;
    }

    fread(str, 1, sizeof(str), file);
    CCLOG("%s", str);
    fclose(file);

    try {
        jw::cppJSON json;
        json.Parse(str);
        jw::cppJSON::iterator it = json.find("name");
        if (it != json.end()) {
            auto vec = it->as<std::vector<std::string> >();
            if (vec.size() == 4) {
                for (int i = 0; i < 4; ++i) {
                    char *str = nullptr;
                    base64Decode((const unsigned char *)vec[i].c_str(), vec[i].length(), (unsigned char **)&str);
                    strncpy(g_name[i], str, sizeof(g_name[i]));
                    free(str);
                }
            }
        }
        it = json.find("record");
        if (it != json.end()) {
            auto vec = it->as<std::vector<std::vector<int> > >();
            g_currentIndex = vec.size();
            for (int i = 0; i < g_currentIndex; ++i) {
                memcpy(g_scores[i], &vec[i][0], sizeof(g_scores[i]));
            }
        }
        it = json.find("startTime");
        if (it != json.end()) {
            g_startTime = it->as<time_t>();
        }
        it = json.find("endTime");
        if (it != json.end()) {
            g_endTime = it->as<time_t>();
        }
    }
    catch (std::exception &e) {
        CCLOG("%s %s", __FUNCTION__, e.what());
    }
}

static void writeToJson() {
    try {
        jw::cppJSON json(jw::cppJSON::ValueType::Object);
        std::vector<std::string> nameVec;
        for (int i = 0; i < 4; ++i) {
            char *base64Name = nullptr;
            base64Encode((const unsigned char *)g_name[i], strlen(g_name[i]) + 1, &base64Name);
            nameVec.push_back(base64Name);
            free(base64Name);
        }
        json.insert(std::make_pair("name", std::move(nameVec)));
        std::vector<std::vector<int> > scoreVec;
        scoreVec.reserve(16);
        for (int k = 0; k < g_currentIndex; ++k) {
            scoreVec.push_back(std::vector<int>({ g_scores[k][0], g_scores[k][1], g_scores[k][2], g_scores[k][3] }));
        }
        json.insert(std::make_pair("record", std::move(scoreVec)));
        json.insert(std::make_pair("startTime", g_startTime));
        json.insert(std::make_pair("endTime", g_endTime));
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

void ScoreSheetScene::recover() {
    readFromJson();

    bool empty = false;
    for (int i = 0; i < 4; ++i) {
        if (g_name[i][0] == '\0') {
            empty = true;
            break;
        }
    }

    if (empty) {
        memset(g_scores, 0, sizeof(g_scores));
        g_currentIndex = 0;
        timeScheduler(0.0f);
        this->schedule(schedule_selector(ScoreSheetScene::timeScheduler), 1.0f);
        return;
    }

    for (int i = 0; i < 4; ++i) {
        _editBox[i]->setVisible(false);
        _editBox[i]->setEnabled(false);
        _nameLabel[i]->setString(g_name[i]);
        _nameLabel[i]->setVisible(true);
    }

    _lockButton->setEnabled(false);
    _lockButton->setVisible(false);

    memset(_scores, 0, sizeof(_scores));
    for (int k = 0; k < g_currentIndex; ++k) {
        for (int i = 0; i < 4; ++i) {
            _scoreLabels[k][i]->setString(StringUtils::format("%+d", g_scores[k][i]));
            _scores[i] += g_scores[k][i];
        }

        _recordButton[k]->setVisible(false);
        _recordButton[k]->setEnabled(false);
    }

    for (int i = 0; i < 4; ++i) {
        _totalLabel[i]->setString(StringUtils::format("%+d", _scores[i]));
    }

    if (g_currentIndex < 16) {
        _recordButton[g_currentIndex]->setVisible(true);
        _recordButton[g_currentIndex]->setEnabled(true);

        char str[255] = "开始时间：";
        size_t len = strlen(str);
        strftime(str + len, sizeof(str) - len, "%Y-%m-%d %H:%M", localtime(&g_startTime));
        _timeLabel->setString(str);
    }
    else {
        char str[255] = "起止时间：";
        size_t len = strlen(str);
        len += strftime(str + len, sizeof(str) - len, "%Y-%m-%d %H:%M", localtime(&g_startTime));
        strcpy(str + len, " -- ");
        len += 4;
        strftime(str + len, sizeof(str) - len, "%Y-%m-%d %H:%M", localtime(&g_endTime));
        _timeLabel->setString(str);
    }
}

void ScoreSheetScene::reset() {
    memset(g_name, 0, sizeof(g_name));
    memset(g_scores, 0, sizeof(g_scores));
    g_currentIndex = 0;

    memset(_scores, 0, sizeof(_scores));
    for (int i = 0; i < 4; ++i) {
        _editBox[i]->setText("");
        _editBox[i]->setVisible(true);
        _editBox[i]->setEnabled(true);
        _nameLabel[i]->setVisible(false);
        _totalLabel[i]->setString("+0");
    }

    _lockButton->setEnabled(true);
    _lockButton->setVisible(true);
    timeScheduler(0.0f);
    this->schedule(schedule_selector(ScoreSheetScene::timeScheduler), 1.0f);

    for (int k = 0; k < 16; ++k) {
        for (int i = 0; i < 4; ++i) {
            _scoreLabels[k][i]->setString("");
        }
        _recordButton[k]->setVisible(false);
        _recordButton[k]->setEnabled(false);
    }
}

void ScoreSheetScene::lockCallback(cocos2d::Ref *sender) {
    for (int i = 0; i < 4; ++i) {
        const char *str = _editBox[i]->getText();
        if (str[0] == '\0') {
            return;
        }
        strncpy(g_name[i], str, sizeof(g_name[i]));
    }

    memset(_scores, 0, sizeof(_scores));

    for (int i = 0; i < 4; ++i) {
        _editBox[i]->setVisible(false);
        _editBox[i]->setEnabled(false);
        _nameLabel[i]->setVisible(true);
        _nameLabel[i]->setString(g_name[i]);
    }

    _recordButton[0]->setVisible(true);
    _recordButton[0]->setEnabled(true);
    _lockButton->setEnabled(false);
    _lockButton->setVisible(false);

    this->unschedule(schedule_selector(ScoreSheetScene::timeScheduler));

    char str[255] = "开始时间：";
    size_t len = strlen(str);
    g_startTime = time(nullptr);
    strftime(str + len, sizeof(str) - len, "%Y-%m-%d %H:%M", localtime(&g_startTime));
    _timeLabel->setString(str);
}

void ScoreSheetScene::recordCallback(cocos2d::Ref *sender, int index) {
    const char *name[] = { g_name[0], g_name[1], g_name[2], g_name[3] };
    Director::getInstance()->pushScene(RecordScene::createScene(index, name, [this, index](RecordScene *scene) {
        const int (&scores)[4] = scene->getScoreTable();
        uint64_t pointsFlag = scene->getPointsFlag();
        for (int i = 0; i < 4; ++i) {
            g_scores[index][i] = scores[i];
            _scoreLabels[index][i]->setString(StringUtils::format("%+d", scores[i]));
            _scores[i] += scores[i];
            _totalLabel[i]->setString(StringUtils::format("%+d", _scores[i]));
        }

        _recordButton[g_currentIndex]->setVisible(false);
        _recordButton[g_currentIndex]->setEnabled(false);
        if (++g_currentIndex < 16) {
            _recordButton[g_currentIndex]->setVisible(true);
            _recordButton[g_currentIndex]->setEnabled(true);
        }
        else {
            g_endTime = time(nullptr);
            char str[255] = "起止时间：";
            size_t len = strlen(str);
            len += strftime(str + len, sizeof(str) - len, "%Y-%m-%d %H:%M", localtime(&g_startTime));
            strcpy(str + len, " -- ");
            len += 4;
            strftime(str + len, sizeof(str) - len, "%Y-%m-%d %H:%M", localtime(&g_endTime));
            _timeLabel->setString(str);
        }
        writeToJson();
    }));
}

void ScoreSheetScene::timeScheduler(float dt) {
    char str[255] = "当前时间：";
    size_t len = strlen(str);
    time_t t = time(nullptr);
    strftime(str + len, sizeof(str) - len, "%Y-%m-%d %H:%M", localtime(&t));
    _timeLabel->setString(str);
}
