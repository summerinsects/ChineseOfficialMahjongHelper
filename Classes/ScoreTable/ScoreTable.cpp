#include "ScoreTable.h"
#include "ScoreDefinition.h"
#include "../mahjong-algorithm/points_calculator.h"

USING_NS_CC;

Scene *ScoreTableScene::createScene() {
    auto scene = Scene::create();
    auto layer = ScoreTableScene::create();
    scene->addChild(layer);
    return scene;
}

static cocos2d::ui::Button *createPointButton(size_t idx) {
    ui::Button *button = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_highlighted.png");
    button->setScale9Enabled(true);
    button->setContentSize(Size(66.0f, 20.0f));
    button->setTitleColor(Color3B::BLACK);
    button->setTitleFontSize(12);
    button->setTitleText(mahjong::points_name[idx]);
    button->addClickEventListener([idx](Ref *) {
        Director::getInstance()->pushScene(ScoreDefinitionScene::createScene(idx));
    });
    return button;
}

bool ScoreTableScene::init() {
    if (!BaseLayer::initWithTitle("国标麻将番种表")) {
        return false;
    }

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    ui::Widget *innerNode = ui::Widget::create();
    innerNode->setAnchorPoint(Vec2::ANCHOR_BOTTOM_LEFT);
    static const float innerNodeHeight = 768.0f;  // 25行 * 24像素 + 12行 * 14像素
    innerNode->setContentSize(Size(visibleSize.width, innerNodeHeight));

    static const int points[] = { 1, 2, 4, 6, 8, 12, 16, 24, 32, 48, 64, 88 };  // 番种
    static const size_t beginIndex[] =
#if HAS_CONCEALED_KONG_AND_MELDED_KONG
    { 70, 60, 56, 48, 39, 34, 28, 19, 16, 14, 8, 1 };
#else
    { 69, 59, 55, 48, 39, 34, 28, 19, 16, 14, 8, 1 };
#endif
    static const size_t counts[] = { 13, 10, 4, 7, 8, 5, 6, 9, 3, 2, 6, 7 };  // 各档次番种的个数
    float y = innerNodeHeight;
    const float gap = (visibleSize.width - 4.0f) * 0.25f;
    for (int i = 0; i < 12; ++i) {
        Label *label = Label::createWithSystemFont(StringUtils::format("%d番", points[i]), "Arial", 12);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        innerNode->addChild(label);
        label->setPosition(Vec2(5.0f, y - 7.0f));
        y -= 14.0f;

        // 每行排4个番种
        for (size_t k = 0; k < counts[i]; ++k) {
            size_t col = k % 4;
            if (k > 0 && col == 0) {
                y -= 24.0f;
            }
            size_t idx = beginIndex[i] + k;
            ui::Button *button = createPointButton(idx);
            innerNode->addChild(button);
            button->setPosition(Vec2(gap * (col + 0.5f), y - 12.0f));
        }
        y -= 24.0f;
    }

    // ScrollView
    ui::ScrollView *scrollView = ui::ScrollView::create();
    scrollView->setDirection(ui::ScrollView::Direction::VERTICAL);
    scrollView->setContentSize(Size(visibleSize.width, visibleSize.height - 35));
    scrollView->setScrollBarPositionFromCorner(Vec2(10, 10));
    scrollView->setInnerContainerSize(innerNode->getContentSize());
    scrollView->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    scrollView->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f - 15.0f));
    this->addChild(scrollView);

    scrollView->addChild(innerNode);

    return true;
}
