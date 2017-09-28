#include "CompetitionMainScene.h"
#include <array>
#include "../cocos-wheels/CWEditBoxDelegate.h"
#include "../widget/AlertView.h"
#include "Competition.h"
#include "CompetitionEnrollScene.h"
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
            if (_competitionData->isEnrollmentOver()) {
                scene = CompetitionRoundScene::create(_competitionData, _competitionData->rounds.size() - 1);
            }
            else {
                scene = CompetitionEnrollScene::create(_competitionData);
            }
            Director::getInstance()->pushScene(scene);
        }
    });

    button->setOnEnterCallback([this, button]() {
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

void CompetitionMainScene::showNewCompetitionAlert(const std::string &name, size_t player, size_t round) {
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
    editBox->setReturnType(ui::EditBox::KeyboardReturnType::NEXT);
    editBox->setFontColor(Color4B::BLACK);
    editBox->setFontSize(12);
    editBox->setText(name.c_str());
    rootNode->addChild(editBox);
    editBox->setPosition(Vec2(135, 75));
    editBox->setTag(0);
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
    snprintf(buf, sizeof(buf), "%" PRIzu, player);

    editBox = ui::EditBox::create(Size(50.0f, 20.0f), ui::Scale9Sprite::create("source_material/btn_square_normal.png"));
    editBox->setInputFlag(ui::EditBox::InputFlag::SENSITIVE);
    editBox->setInputMode(ui::EditBox::InputMode::NUMERIC);
    editBox->setReturnType(ui::EditBox::KeyboardReturnType::NEXT);
    editBox->setFontColor(Color4B::BLACK);
    editBox->setFontSize(12);
    editBox->setText(buf);
    rootNode->addChild(editBox);
    editBox->setPosition(Vec2(85, 45));
    editBox->setTag(1);
    editBoxes[1] = editBox;

    label = Label::createWithSystemFont("比赛轮数", "Arial", 12);
    label->setColor(Color3B::BLACK);
    rootNode->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(5, 15));

    snprintf(buf, sizeof(buf), "%" PRIzu, round);

    editBox = ui::EditBox::create(Size(50.0f, 20.0f), ui::Scale9Sprite::create("source_material/btn_square_normal.png"));
    editBox->setInputFlag(ui::EditBox::InputFlag::SENSITIVE);
    editBox->setInputMode(ui::EditBox::InputMode::NUMERIC);
    editBox->setReturnType(ui::EditBox::KeyboardReturnType::DONE);
    editBox->setFontColor(Color4B::BLACK);
    editBox->setFontSize(12);
    editBox->setText(buf);
    rootNode->addChild(editBox);
    editBox->setPosition(Vec2(85, 15));
    editBox->setTag(2);
    editBoxes[2] = editBox;

    // EditBox的代理，使得能连续输入
    auto delegate = std::make_shared<cw::EditBoxEndWithActionDelegate>([editBoxes](ui::EditBox *editBox, ui::EditBoxDelegate::EditBoxEndAction action) {
        if (action == ui::EditBoxDelegate::EditBoxEndAction::TAB_TO_NEXT) {
            int tag = editBox->getTag();
            editBox = editBoxes[tag + 1];
            editBox->scheduleOnce([editBox](float) {
                editBox->touchDownAction(editBox, ui::Widget::TouchEventType::ENDED);
            }, 0.0f, "open_keyboard");
        }
    });
    editBoxes[0]->setDelegate(delegate.get());
    editBoxes[1]->setDelegate(delegate.get());

    AlertView::showWithNode("新建比赛", rootNode, [this, editBoxes, delegate]() {
        std::string name;
        size_t player = 8, round = 5;

        const char *text = editBoxes[0]->getText();
        if (*text != '\0') {
            name = text;
        }

        text = editBoxes[1]->getText();
        if (*text != '\0') {
            player = atoll(text);
        }

        text = editBoxes[2]->getText();
        if (*text != '\0') {
            round = atoll(text);
        }

        if (name.empty()) {
            AlertView::showWithMessage("新建比赛", "请输入赛事名称", 12,
                std::bind(&CompetitionMainScene::showNewCompetitionAlert, this, name, player, round), nullptr);
            return;
        }

        if ((player & 0x3) || player == 0) {
            AlertView::showWithMessage("新建比赛", "参赛人数必须为4的倍数，且大于0", 12,
                std::bind(&CompetitionMainScene::showNewCompetitionAlert, this, name, player, round), nullptr);
            return;
        }

        if (round == 0) {
            AlertView::showWithMessage("新建比赛", "比赛轮数必须大于0", 12,
                std::bind(&CompetitionMainScene::showNewCompetitionAlert, this, name, player, round), nullptr);
            return;
        }

        Label *label = Label::createWithSystemFont(StringUtils::format("「%s」\n%" PRIzu "人\n%" PRIzu "轮", name.c_str(), player, round), "Arial", 12);
        label->setColor(Color3B::BLACK);
        label->setHorizontalAlignment(TextHAlignment::CENTER);

        AlertView::showWithNode("新建比赛", label, [this, name, player, round]() {
            _competitionData->prepare(name, player, round);
            _competitionData->writeToFile(FileUtils::getInstance()->getWritablePath().append("competition.json"));
            Director::getInstance()->pushScene(CompetitionEnrollScene::create(_competitionData));
        }, nullptr);
    }, nullptr);
}
