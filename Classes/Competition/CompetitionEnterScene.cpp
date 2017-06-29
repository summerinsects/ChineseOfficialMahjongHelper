#include "CompetitionEnterScene.h"
#include "CompetitionRoundScene.h"
#include "Competition.h"
#include "../common.h"
#include "../widget/AlertView.h"

USING_NS_CC;

CompetitionEnterScene *CompetitionEnterScene::create(const std::string &name, unsigned num, unsigned round) {
    auto ret = new (std::nothrow) CompetitionEnterScene();
    if (ret != nullptr && ret->initWithName(name, num, round)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool CompetitionEnterScene::initWithName(const std::string &name, unsigned num, unsigned round) {
    if (UNLIKELY(!BaseScene::initWithTitle(name))) {
        return false;
    }

    _competitionData = std::make_shared<CompetitionData>();

    _competitionData->name = name;
    _competitionData->round_count = round;
    _competitionData->current_round = 0;
    _competitionData->players.resize(num);
    for (unsigned i = 0; i < num; ++i) {
        _competitionData->players[i].serial = 1 + i;
    }

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    Label *label = Label::createWithSystemFont("报名表", "Arail", 12);
    label->setColor(Color3B::BLACK);
    this->addChild(label);
    label->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height - 45.0f));

    // 下一步按钮
    ui::Button *button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(55.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText("下一步");
    button->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height - 70.0f));
    button->addClickEventListener(std::bind(&CompetitionEnterScene::onOkButton, this, std::placeholders::_1));

    label = Label::createWithSystemFont("序号", "Arail", 12);
    label->setColor(Color3B::BLACK);
    this->addChild(label);
    label->setPosition(Vec2(origin.x + 20.0f, origin.y + visibleSize.height - 100.0f));

    label = Label::createWithSystemFont("选手姓名", "Arail", 12);
    label->setColor(Color3B::BLACK);
    this->addChild(label);
    label->setPosition(Vec2(origin.x + visibleSize.width * 0.25f + 15.0f, origin.y + visibleSize.height - 100.0f));

    label = Label::createWithSystemFont("序号", "Arail", 12);
    label->setColor(Color3B::BLACK);
    this->addChild(label);
    label->setPosition(Vec2(origin.x + visibleSize.width * 0.5f + 20.0f, origin.y + visibleSize.height - 100.0f));

    label = Label::createWithSystemFont("选手姓名", "Arail", 12);
    label->setColor(Color3B::BLACK);
    this->addChild(label);
    label->setPosition(Vec2(origin.x + visibleSize.width * 0.75f + 15.0f, origin.y + visibleSize.height - 100.0f));

    cw::TableView *tableView = cw::TableView::create();
    tableView->setContentSize(Size(visibleSize.width, visibleSize.height - 115.0f));
    tableView->setDelegate(this);
    tableView->setDirection(ui::ScrollView::Direction::VERTICAL);
    tableView->setVerticalFillOrder(cw::TableView::VerticalFillOrder::TOP_DOWN);

    tableView->setScrollBarPositionFromCorner(Vec2(5, 2));
    tableView->setScrollBarWidth(4);
    tableView->setScrollBarOpacity(0x99);
    tableView->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    tableView->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f - 55.0f));
    tableView->reloadData();
    this->addChild(tableView);
    _tableView = tableView;

    return true;
}

ssize_t CompetitionEnterScene::numberOfCellsInTableView(cw::TableView *table) {
    return _competitionData->players.size() >> 1;
}

cocos2d::Size CompetitionEnterScene::tableCellSizeForIndex(cw::TableView *table, ssize_t idx) {
    return Size(0, 30);
}

cw::TableViewCell *CompetitionEnterScene::tableCellAtIndex(cw::TableView *table, ssize_t idx) {
    typedef cw::TableViewCellEx<Label *[4], ui::Button *[2], LayerColor *[2]> CustomCell;
    CustomCell *cell = (CustomCell *)table->dequeueCell();

    Size visibleSize = Director::getInstance()->getVisibleSize();

    if (cell == nullptr) {
        cell = CustomCell::create();

        CustomCell::ExtDataType &ext = cell->getExtData();
        Label *(&labels)[4] = std::get<0>(ext);
        ui::Button *(&buttons)[2] = std::get<1>(ext);
        LayerColor *(&layerColor)[2] = std::get<2>(ext);

        layerColor[0] = LayerColor::create(Color4B(0xC0, 0xC0, 0xC0, 0x10), visibleSize.width, 29);
        cell->addChild(layerColor[0]);

        layerColor[1] = LayerColor::create(Color4B(0x80, 0x80, 0x80, 0x10), visibleSize.width, 29);
        cell->addChild(layerColor[1]);

        Label *label = Label::createWithSystemFont("", "Arail", 12);
        label->setColor(Color3B::BLACK);
        cell->addChild(label);
        label->setPosition(Vec2(20.0f, 15.0f));
        labels[0] = label;

        label = Label::createWithSystemFont("", "Arail", 12);
        label->setColor(Color3B::BLACK);
        cell->addChild(label);
        label->setPosition(Vec2(visibleSize.width * 0.25f + 15.0f, 15.0f));
        labels[1] = label;

        label = Label::createWithSystemFont("", "Arail", 12);
        label->setColor(Color3B::BLACK);
        cell->addChild(label);
        label->setPosition(Vec2(visibleSize.width * 0.5f + 20.0f, 15.0f));
        labels[2] = label;

        label = Label::createWithSystemFont("", "Arail", 12);
        label->setColor(Color3B::BLACK);
        cell->addChild(label);
        label->setPosition(Vec2(visibleSize.width * 0.75f + 15.0f, 15.0f));
        labels[3] = label;

        ui::Button *button = ui::Button::create();
        button->setScale9Enabled(true);
        button->setPosition(labels[1]->getPosition());
        button->setContentSize(Size(visibleSize.width * 0.5f - 50.0f, 25.0f));
        cell->addChild(button);
        button->addClickEventListener(std::bind(&CompetitionEnterScene::onNameButton, this, std::placeholders::_1));
        buttons[0] = button;

        button = ui::Button::create();
        button->setScale9Enabled(true);
        button->setPosition(labels[3]->getPosition());
        button->setContentSize(Size(visibleSize.width * 0.5f - 50.0f, 25.0f));
        cell->addChild(button);
        button->addClickEventListener(std::bind(&CompetitionEnterScene::onNameButton, this, std::placeholders::_1));
        buttons[1] = button;
    }

    const CustomCell::ExtDataType ext = cell->getExtData();
    Label *const (&labels)[4] = std::get<0>(ext);
    ui::Button *const (&buttons)[2] = std::get<1>(ext);
    LayerColor *const (&layerColor)[2] = std::get<2>(ext);

    layerColor[0]->setVisible(!(idx & 1));
    layerColor[1]->setVisible(!!(idx & 1));

    size_t idx0 = idx << 1;
    size_t idx1 = idx0 | 1;
    labels[0]->setString(std::to_string(idx0 + 1));
    buttons[0]->setUserData(reinterpret_cast<void *>(idx0));
    labels[2]->setString(std::to_string(idx1 + 1));
    buttons[1]->setUserData(reinterpret_cast<void *>(idx1));

    const std::string &name0 = _competitionData->players[idx0].name;
    const std::string &name1 = _competitionData->players[idx1].name;

    if (name0.empty()) {
        labels[1]->setColor(Color3B::GRAY);
        labels[1]->setString("选手姓名");
    }
    else {
        labels[1]->setColor(Color3B::BLACK);
        labels[1]->setString(name0);
    }
    Common::scaleLabelToFitWidth(labels[1], visibleSize.width * 0.5f - 50.0f);

    if (name1.empty()) {
        labels[3]->setColor(Color3B::GRAY);
        labels[3]->setString("选手姓名");
    }
    else {
        labels[3]->setColor(Color3B::BLACK);
        labels[3]->setString(name1);
    }
    Common::scaleLabelToFitWidth(labels[3], visibleSize.width * 0.5f - 50.0f);

    return cell;
}

void CompetitionEnterScene::onOkButton(cocos2d::Ref *sender) {
    if (std::any_of(_competitionData->players.begin(), _competitionData->players.end(),
        [](const CompetitionPlayer &p) { return p.name.empty(); })) {
        AlertView::showWithMessage("提示", "请录入所有选手姓名", 12, nullptr, nullptr);
        return;
    }

    // TODO:
    Director::getInstance()->pushScene(CompetitionRoundScene::create(_competitionData));
}

void CompetitionEnterScene::onNameButton(cocos2d::Ref *sender) {
    ui::Button *button = (ui::Button *)sender;
    size_t idx = reinterpret_cast<size_t>(button->getUserData());

    ui::EditBox *editBox = ui::EditBox::create(Size(120.0f, 20.0f), ui::Scale9Sprite::create("source_material/btn_square_normal.png"));
    editBox->setInputMode(ui::EditBox::InputMode::SINGLE_LINE);
    editBox->setInputFlag(ui::EditBox::InputFlag::SENSITIVE);
    editBox->setReturnType(ui::EditBox::KeyboardReturnType::DONE);
    editBox->setFontColor(Color3B::BLACK);
    editBox->setFontSize(12);
    editBox->setText(_competitionData->players[idx].name.c_str());
    editBox->setPlaceholderFontColor(Color4B::GRAY);
    editBox->setPlaceHolder("输入选手姓名");

    AlertView::showWithNode(Common::format<64>("序号「%lu」", (unsigned long)idx + 1), editBox, [this, editBox, idx]() {
        _competitionData->players[idx].name = editBox->getText();
        _tableView->updateCellAtIndex(idx >> 1);
    }, nullptr);

    Director::getInstance()->getScheduler()->schedule([editBox](float) {
        editBox->touchDownAction(editBox, ui::Widget::TouchEventType::ENDED);
    }, this, 0.0f, 0, 0.0f, false, "open_keyboard");
}
