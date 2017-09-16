#include "CompetitionMainScene.h"
#include "CompetitionEnterScene.h"
#include "CompetitionRoundScene.h"
#include "../common.h"
#include "../widget/AlertView.h"
#include "Competition.h"

USING_NS_CC;

bool CompetitionMainScene::init() {
    if (UNLIKELY(!BaseScene::initWithTitle("比赛"))) {
        return false;
    }

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    ui::Button *button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(90.0, 32.0f));
    button->setTitleFontSize(20);
    button->setTitleText("创建比赛");
    button->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f + 50));
    button->addClickEventListener([this](Ref *) {
        this->showCompetitionCreatingAlert("", 8, 5);
    });

    button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(90.0, 32.0f));
    button->setTitleFontSize(20);
    button->setTitleText("继续");
    button->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f));
    button->addClickEventListener([](Ref *) {

    });

    button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(90.0, 32.0f));
    button->setTitleFontSize(20);
    button->setTitleText("历史记录");
    button->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f - 50));
    button->addClickEventListener([](Ref *) {

    });

    _competitionData = std::make_shared<CompetitionData>();

    std::string fileName = FileUtils::getInstance()->getWritablePath();
    fileName.append("competition.json");
    _competitionData->readFromFile(fileName.c_str());
    if (_competitionData->finish_time == 0) {
        this->scheduleOnce([this](float) {
            Director::getInstance()->pushScene(CompetitionRoundScene::create(_competitionData, _competitionData->current_round));
        }, 0.0f, "push_round_scene");
    }

    return true;
}

void CompetitionMainScene::showCompetitionCreatingAlert(const std::string &name, unsigned num, unsigned round) {
    Node *rootNode = Node::create();
    rootNode->setContentSize(Size(215, 90));

    Label *label = Label::createWithSystemFont("赛事名称", "Arial", 12);
    label->setColor(Color3B::BLACK);
    rootNode->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(5, 75));

    ui::EditBox *editBox1 = ui::EditBox::create(Size(150.0f, 20.0f), ui::Scale9Sprite::create("source_material/btn_square_normal.png"));
    editBox1->setInputFlag(ui::EditBox::InputFlag::SENSITIVE);
    editBox1->setInputMode(ui::EditBox::InputMode::SINGLE_LINE);
    editBox1->setReturnType(ui::EditBox::KeyboardReturnType::DONE);
    editBox1->setFontColor(Color4B::BLACK);
    editBox1->setFontSize(12);
    editBox1->setText(name.c_str());
    rootNode->addChild(editBox1);
    editBox1->setPosition(Vec2(135, 75));
#if 1  // test
    editBox1->setText("测试比赛");
#endif

    label = Label::createWithSystemFont("参赛人数", "Arial", 12);
    label->setColor(Color3B::BLACK);
    rootNode->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(5, 45));

    char buf[32];
    snprintf(buf, sizeof(buf), "%u", num);

    ui::EditBox *editBox2 = ui::EditBox::create(Size(50.0f, 20.0f), ui::Scale9Sprite::create("source_material/btn_square_normal.png"));
    editBox2->setInputFlag(ui::EditBox::InputFlag::SENSITIVE);
    editBox2->setInputMode(ui::EditBox::InputMode::NUMERIC);
    editBox2->setReturnType(ui::EditBox::KeyboardReturnType::DONE);
    editBox2->setFontColor(Color4B::BLACK);
    editBox2->setFontSize(12);
    editBox2->setText(buf);
    rootNode->addChild(editBox2);
    editBox2->setPosition(Vec2(85, 45));

    label = Label::createWithSystemFont("比赛轮数", "Arial", 12);
    label->setColor(Color3B::BLACK);
    rootNode->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(5, 15));

    snprintf(buf, sizeof(buf), "%u", round);

    ui::EditBox *editBox3 = ui::EditBox::create(Size(50.0f, 20.0f), ui::Scale9Sprite::create("source_material/btn_square_normal.png"));
    editBox3->setInputFlag(ui::EditBox::InputFlag::SENSITIVE);
    editBox3->setInputMode(ui::EditBox::InputMode::NUMERIC);
    editBox3->setReturnType(ui::EditBox::KeyboardReturnType::DONE);
    editBox3->setFontColor(Color4B::BLACK);
    editBox3->setFontSize(12);
    editBox3->setText(buf);
    rootNode->addChild(editBox3);
    editBox3->setPosition(Vec2(85, 15));

    AlertView::showWithNode("创建比赛", rootNode, [this, editBox1, editBox2, editBox3]() {
        std::string name;
        unsigned num = 8, round = 5;

        const char *text = editBox1->getText();
        if (*text != '\0') {
            name = text;
        }

        text = editBox2->getText();
        if (*text != '\0') {
            num = atoi(text);
        }

        text = editBox3->getText();
        if (*text != '\0') {
            round = atoi(text);
        }

        if (name.empty()) {
            AlertView::showWithMessage("创建比赛", "请输入赛事名称", 12,
                std::bind(&CompetitionMainScene::showCompetitionCreatingAlert, this, name, num, round), nullptr);
            return;
        }

        if ((num & 0x3) || num == 0) {
            AlertView::showWithMessage("创建比赛", "参赛人数必须为4的倍数，且大于0", 12,
                std::bind(&CompetitionMainScene::showCompetitionCreatingAlert, this, name, num, round), nullptr);
            return;
        }

        if (round == 0) {
            AlertView::showWithMessage("创建比赛", "比赛轮数必须大于0", 12,
                std::bind(&CompetitionMainScene::showCompetitionCreatingAlert, this, name, num, round), nullptr);
            return;
        }

        Label *label = Label::createWithSystemFont(StringUtils::format("「%s」\n%u人\n%u轮", name.c_str(), num, round), "Arial", 12);
        label->setColor(Color3B::BLACK);
        label->setHorizontalAlignment(TextHAlignment::CENTER);

        AlertView::showWithNode("创建比赛", label, [this, name, num, round]() {
            _competitionData->name = name;
            _competitionData->rounds.resize(round);
            _competitionData->current_round = 0;
            _competitionData->players.resize(num);

            for (unsigned i = 0; i < num; ++i) {
                _competitionData->players[i].serial = 1 + i;
            }

            Director::getInstance()->pushScene(CompetitionEnterScene::create(_competitionData));
        }, nullptr);
    }, nullptr);
}

