#include "CompetitionMainScene.h"
#include <array>
#include "../widget/AlertView.h"
#include "Competition.h"
#include "CompetitionEnterScene.h"
#include "CompetitionRoundScene.h"

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
    button->setTitleText("新建比赛");
    button->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f + 50));
    button->addClickEventListener([this](Ref *) {
        if (_competitionData->start_time != 0 && _competitionData->finish_time == 0) {
            AlertView::showWithMessage("新建比赛", "当前有未完成的比赛，新建比赛将会覆盖旧的比赛，是否继续？", 12,
                std::bind(&CompetitionMainScene::showNewCompetitionAlert, this, "", 8, 5), nullptr);
        }
        else {
            this->showNewCompetitionAlert("", 8, 5);
        }
    });

    button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(90.0, 32.0f));
    button->setTitleFontSize(20);
    button->setTitleText("继续比赛");
    button->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f));
    button->addClickEventListener([this](Ref *) {
        if (_competitionData->start_time != 0 && _competitionData->finish_time == 0) {
            Scene *scene = nullptr;
            if (_competitionData->isRegistrationFull()) {
                scene = CompetitionRoundScene::create(_competitionData, _competitionData->current_round);
            }
            else {
                scene = CompetitionEnterScene::create(_competitionData);
            }
            Director::getInstance()->pushScene(scene);
        }
    });

    this->setOnEnterCallback([this, button]() {
        button->setEnabled(_competitionData->start_time != 0 && _competitionData->finish_time == 0);
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
    _competitionData->readFromFile(FileUtils::getInstance()->getWritablePath().append("competition.json"));

    return true;
}

void CompetitionMainScene::showNewCompetitionAlert(const std::string &name, unsigned num, unsigned round) {
    Node *rootNode = Node::create();
    rootNode->setContentSize(Size(215, 90));

    Label *label = Label::createWithSystemFont("赛事名称", "Arial", 12);
    label->setColor(Color3B::BLACK);
    rootNode->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(5, 75));

    std::array<ui::EditBox *, 3> editBoxes;

    ui::EditBox *editBox = ui::EditBox::create(Size(150.0f, 20.0f), ui::Scale9Sprite::create("source_material/btn_square_normal.png"));
    editBox->setInputFlag(ui::EditBox::InputFlag::SENSITIVE);
    editBox->setInputMode(ui::EditBox::InputMode::SINGLE_LINE);
    editBox->setReturnType(ui::EditBox::KeyboardReturnType::DONE);
    editBox->setFontColor(Color4B::BLACK);
    editBox->setFontSize(12);
    editBox->setText(name.c_str());
    rootNode->addChild(editBox);
    editBox->setPosition(Vec2(135, 75));
    editBoxes[0] = editBox;
#if 1  // test
    editBox->setText("测试比赛");
#endif

    label = Label::createWithSystemFont("参赛人数", "Arial", 12);
    label->setColor(Color3B::BLACK);
    rootNode->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(5, 45));

    char buf[32];
    snprintf(buf, sizeof(buf), "%u", num);

    editBox = ui::EditBox::create(Size(50.0f, 20.0f), ui::Scale9Sprite::create("source_material/btn_square_normal.png"));
    editBox->setInputFlag(ui::EditBox::InputFlag::SENSITIVE);
    editBox->setInputMode(ui::EditBox::InputMode::NUMERIC);
    editBox->setReturnType(ui::EditBox::KeyboardReturnType::DONE);
    editBox->setFontColor(Color4B::BLACK);
    editBox->setFontSize(12);
    editBox->setText(buf);
    rootNode->addChild(editBox);
    editBox->setPosition(Vec2(85, 45));
    editBoxes[1] = editBox;

    label = Label::createWithSystemFont("比赛轮数", "Arial", 12);
    label->setColor(Color3B::BLACK);
    rootNode->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(5, 15));

    snprintf(buf, sizeof(buf), "%u", round);

    editBox = ui::EditBox::create(Size(50.0f, 20.0f), ui::Scale9Sprite::create("source_material/btn_square_normal.png"));
    editBox->setInputFlag(ui::EditBox::InputFlag::SENSITIVE);
    editBox->setInputMode(ui::EditBox::InputMode::NUMERIC);
    editBox->setReturnType(ui::EditBox::KeyboardReturnType::DONE);
    editBox->setFontColor(Color4B::BLACK);
    editBox->setFontSize(12);
    editBox->setText(buf);
    rootNode->addChild(editBox);
    editBox->setPosition(Vec2(85, 15));
    editBoxes[2] = editBox;

    AlertView::showWithNode("新建比赛", rootNode, [this, editBoxes]() {
        std::string name;
        unsigned num = 8, round = 5;

        const char *text = editBoxes[0]->getText();
        if (*text != '\0') {
            name = text;
        }

        text = editBoxes[1]->getText();
        if (*text != '\0') {
            num = atoi(text);
        }

        text = editBoxes[2]->getText();
        if (*text != '\0') {
            round = atoi(text);
        }

        if (name.empty()) {
            AlertView::showWithMessage("新建比赛", "请输入赛事名称", 12,
                std::bind(&CompetitionMainScene::showNewCompetitionAlert, this, name, num, round), nullptr);
            return;
        }

        if ((num & 0x3) || num == 0) {
            AlertView::showWithMessage("新建比赛", "参赛人数必须为4的倍数，且大于0", 12,
                std::bind(&CompetitionMainScene::showNewCompetitionAlert, this, name, num, round), nullptr);
            return;
        }

        if (round == 0) {
            AlertView::showWithMessage("新建比赛", "比赛轮数必须大于0", 12,
                std::bind(&CompetitionMainScene::showNewCompetitionAlert, this, name, num, round), nullptr);
            return;
        }

        Label *label = Label::createWithSystemFont(StringUtils::format("「%s」\n%u人\n%u轮", name.c_str(), num, round), "Arial", 12);
        label->setColor(Color3B::BLACK);
        label->setHorizontalAlignment(TextHAlignment::CENTER);

        AlertView::showWithNode("新建比赛", label, [this, name, num, round]() {
            _competitionData->name = name;
            _competitionData->rounds.resize(round);
            _competitionData->current_round = 0;
            _competitionData->players.resize(num);

            _competitionData->prepare();

            Director::getInstance()->pushScene(CompetitionEnterScene::create(_competitionData));
        }, nullptr);
    }, nullptr);
}

