#include "DatePicker.h"
#include "CheckBoxScale9.h"
#include "../UICommon.h"
#include "../UIColors.h"
#include "../utils/common.h"
#include "calendar-inline.h"

USING_NS_CC;

#define C4B_BLUE cocos2d::Color4B(44, 121, 178, 255)
#define C4B_RED  cocos2d::Color4B(254, 87, 110, 255)

#define LOWER_BOUND 1900
#define UPPER_BOUND 2099

static const char *weekTexts[] = { __UTF8("日"), __UTF8("一"), __UTF8("二"), __UTF8("三"), __UTF8("四"), __UTF8("五"), __UTF8("六") };

bool DatePicker::init(const Date *date, Callback &&callback) {
    if (!Layer::init()) {
        return false;
    }

    _callback.swap(callback);

    time_t now = time(nullptr);
    struct tm ret = *localtime(&now);
    _today.year = ret.tm_year + 1900;
    _today.month = ret.tm_mon + 1;
    _today.day = ret.tm_mday;
    if (date == nullptr) {
        memcpy(&_picked, &_today, sizeof(_picked));
    }
    else {
        memcpy(&_picked, date, sizeof(_picked));
    }

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    // 监听返回键
    EventListenerKeyboard *keyboardListener = EventListenerKeyboard::create();
    keyboardListener->onKeyReleased = [this](EventKeyboard::KeyCode keyCode, Event *event) {
        if (keyCode == EventKeyboard::KeyCode::KEY_BACK) {
            event->stopPropagation();
            onNegativeButton(nullptr);
        }
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(keyboardListener, this);

    // 遮罩
    this->addChild(LayerColor::create(Color4B(0, 0, 0, 127)));

    // 30*7+6*2+2*5=232
    const float totalWidth = 232.0f;

    // 背景
    LayerColor *background = LayerColor::create(Color4B(255, 255, 255, 245));
    this->addChild(background);
    background->setIgnoreAnchorPointForPosition(false);
    background->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f));

    float totalHeight = 0.0f;
    ui::Button *button = ui::Button::create("source_material/btn_square_disabled.png", "source_material/btn_square_selected.png");
    background->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(totalWidth * 0.5f, 30.0f));
    button->setTitleFontSize(14);
    button->setTitleText(__UTF8("取消"));
    button->setPosition(Vec2(totalWidth * 0.25f, 15.0f));
    button->addClickEventListener(std::bind(&DatePicker::onNegativeButton, this, std::placeholders::_1));

    button = UICommon::createButton();
    background->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(totalWidth * 0.5f, 30.0f));
    button->setTitleFontSize(14);
    button->setTitleText(__UTF8("确定"));
    button->setPosition(Vec2(totalWidth * 0.75f, 15.0f));
    button->addClickEventListener(std::bind(&DatePicker::onPositiveButton, this, std::placeholders::_1));

    totalHeight += 30.0f;

    const std::string backGround = "source_material/btn_square_normal.png";
    const std::string backGroundSelected = "source_material/btn_square_selected.png";
    const std::string cross = "source_material/btn_square_highlighted.png";
    const std::string noImage = "";

    // 30*6+5*2+15+2*5=215
    const float containerHeight = 215.0f;
    const Size containerSize = Size(totalWidth, containerHeight);

    // 日
    Node *container = Node::create();
    container->setContentSize(containerSize);

    for (int i = 0; i < 7; ++i) {
        Label *label = Label::createWithSystemFont(weekTexts[i], "Arial", 12);
        label->setTextColor(i > 0 && i < 6 ? C4B_GRAY : C4B_RED);
        container->addChild(label);
        label->setPosition(Vec2(20.0f + i * 32.0f, containerHeight - 10.0f));
    }

    for (int i = 0; i < 42; ++i) {
        div_t ret = div(i, 7);
        CheckBoxScale9 *checkBox = CheckBoxScale9::create();
        checkBox->loadTextures(backGround, backGroundSelected, cross, noImage, noImage);
        checkBox->setZoomScale(0.0f);
        checkBox->ignoreContentAdaptWithSize(false);
        checkBox->setContentSize(Size(30.0f, 30.0f));
        checkBox->setTag(i);
        checkBox->addEventListener(std::bind(&DatePicker::onDayBox, this, std::placeholders::_1, std::placeholders::_2));
        container->addChild(checkBox);
        checkBox->setPosition(Vec2(20.0f + ret.rem * 32.0f, containerHeight - 35.0f - ret.quot * 32.0f));
        _dayBoxes[i] = checkBox;

        Label *label = Label::createWithSystemFont("", "Arial", 14);
        label->setTextColor(C4B_GRAY);
        checkBox->addChild(label);
        label->setPosition(Vec2(15.0f, 20.0f));
        _dayLabelsLarge[i] = label;

        label = Label::createWithSystemFont("", "Arial", 7);
        label->setTextColor(C4B_GRAY);
        checkBox->addChild(label);
        label->setPosition(Vec2(15.0f, 6.0f));
        _dayLabelsSmall[i] = label;
    }

    background->addChild(container);
    container->setIgnoreAnchorPointForPosition(false);
    container->setAnchorPoint(Vec2::ANCHOR_MIDDLE_BOTTOM);
    container->setPosition(Vec2(totalWidth * 0.5f, 35.0f));
    _daysContainer = container;

    // 月
    container = Node::create();
    container->setContentSize(containerSize);

    static const char *monthTexts[] = {
        __UTF8("1月"), __UTF8("2月"), __UTF8("3月"), __UTF8("4月"), __UTF8("5月"), __UTF8("6月"),
        __UTF8("7月"), __UTF8("8月"), __UTF8("9月"), __UTF8("10月"), __UTF8("11月"), __UTF8("12月")
    };
    for (int i = 0; i < 12; ++i) {
        div_t ret = div(i, 4);
        CheckBoxScale9 *checkBox = CheckBoxScale9::create();
        checkBox->loadTextures(backGround, backGroundSelected, cross, noImage, noImage);
        checkBox->setZoomScale(0.0f);
        checkBox->ignoreContentAdaptWithSize(false);
        checkBox->setContentSize(Size(54.0f, 54.0f));
        checkBox->setTag(i);
        checkBox->addEventListener(std::bind(&DatePicker::onMonthBox, this, std::placeholders::_1, std::placeholders::_2));
        container->addChild(checkBox);
        checkBox->setPosition(Vec2(32.0f + ret.rem * 56.0f, containerHeight - 32.0f - ret.quot * 56.0f));
        _monthBoxes[i] = checkBox;

        Label *label = Label::createWithSystemFont(monthTexts[i], "Arial", 12);
        label->setTextColor(C4B_GRAY);
        checkBox->addChild(label);
        label->setPosition(Vec2(27.0f, 27.0f));
        checkBox->setUserData(label);
    }

    background->addChild(container);
    container->setIgnoreAnchorPointForPosition(false);
    container->setAnchorPoint(Vec2::ANCHOR_MIDDLE_BOTTOM);
    container->setPosition(Vec2(totalWidth * 0.5f, 35.0f));
    _monthsContainer = container;

    // 年
    container = Node::create();
    container->setContentSize(containerSize);

    for (int i = 0; i < 10; ++i) {
        div_t ret = div(i, 4);
        CheckBoxScale9 *checkBox = CheckBoxScale9::create();
        checkBox->loadTextures(backGround, backGroundSelected, cross, noImage, noImage);
        checkBox->setZoomScale(0.0f);
        checkBox->ignoreContentAdaptWithSize(false);
        checkBox->setContentSize(Size(54.0f, 54.0f));
        checkBox->setTag(i);
        checkBox->addEventListener(std::bind(&DatePicker::onYearBox, this, std::placeholders::_1, std::placeholders::_2));
        container->addChild(checkBox);
        checkBox->setPosition(Vec2(32.0f + ret.rem * 56.0f, containerHeight - 32.0f - ret.quot * 56.0f));
        _yearBoxes[i] = checkBox;

        Label *label = Label::createWithSystemFont("", "Arial", 12);
        label->setTextColor(C4B_GRAY);
        checkBox->addChild(label);
        label->setPosition(Vec2(27.0f, 27.0f));
        checkBox->setUserData(label);
    }

    background->addChild(container);
    container->setIgnoreAnchorPointForPosition(false);
    container->setAnchorPoint(Vec2::ANCHOR_MIDDLE_BOTTOM);
    container->setPosition(Vec2(totalWidth * 0.5f, 35.0f));
    _yearsContainer = container;

    totalHeight += containerSize.height + 10.0f;

    // 用来切换的按钮
    button = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_selected.png");
    button->setScale9Enabled(true);
    button->setContentSize(Size(80.0f, 20.0f));
    button->setTitleColor(C3B_GRAY);
    button->setTitleFontSize(12);
    button->addClickEventListener(std::bind(&DatePicker::onSwitchButton, this, std::placeholders::_1));
    background->addChild(button);
    button->setPosition(Vec2(70.0f, totalHeight + 15.0f));
    _switchButton = button;

    // 返回今天按钮
    button = UICommon::createButton();
    button->setScale9Enabled(true);
    button->setContentSize(Size(55.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText(__UTF8("返回今天"));
    button->addClickEventListener(std::bind(&DatePicker::onTodayButton, this, std::placeholders::_1));
    background->addChild(button);
    button->setPosition(Vec2(totalWidth - 32.5f, totalHeight + 15.0f));

    // 左右按钮
    button = ui::Button::create("icon/left-circle.png");
    background->addChild(button);
    button->setScale(20 / button->getContentSize().width);
    button->setPosition(Vec2(15.0f, totalHeight + 15.0f));
    button->setColor(Color3B(51, 204, 255));
    button->addClickEventListener(std::bind(&DatePicker::onBackwardButton, this, std::placeholders::_1));

    button = ui::Button::create("icon/right-circle.png");
    background->addChild(button);
    button->setScale(20 / button->getContentSize().width);
    button->setPosition(Vec2(125.0f, totalHeight + 15.0f));
    button->setColor(Color3B(51, 204, 255));
    button->addClickEventListener(std::bind(&DatePicker::onForwardButton, this, std::placeholders::_1));

    totalHeight += 25.0f;

    // 上方背景及所选日期显示
    LayerColor *bg2 = LayerColor::create(C4B_BLUE_THEME, totalWidth, 40.0f);
    background->addChild(bg2);
    bg2->setPosition(Vec2(0.0f, totalHeight + 5.0f));

    Label *label = Label::createWithSystemFont("", "Arial", 16);
    background->addChild(label);
    label->setPosition(Vec2(totalWidth * 0.5f, totalHeight + 30.0f));
    _titleLabel = label;

    label = Label::createWithSystemFont("", "Arial", 10);
    background->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_RIGHT);
    label->setPosition(Vec2(totalWidth - 2.0f, totalHeight + 12.0f));
    _chineseDateLabel = label;

    totalHeight += 45.0f;

    background->setContentSize(Size(totalWidth, totalHeight));
    if (totalWidth > visibleSize.width * 0.8f || totalHeight > visibleSize.height * 0.8f) {
        const float scale = std::min(visibleSize.width * 0.8f / totalWidth, visibleSize.height * 0.8f / totalHeight);
        background->setScale(scale);
    }

    // 触摸监听
    EventListenerTouchOneByOne *touchListener = EventListenerTouchOneByOne::create();
    touchListener->setSwallowTouches(true);
    touchListener->onTouchBegan = [this](Touch *touch, Event *event) {
        return true;
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(touchListener, this);
    _touchListener = touchListener;

    setupDayContainer();

    return true;
}

static void adjustDate(calendar::GregorianDate *date) {
    int lastDay = calendar::Gregorian_LastDayOfMonth(date->year, date->month);
    date->day = std::min(date->day, lastDay);
}

void DatePicker::refreshTitle() {
    _titleLabel->setString(Common::format(__UTF8("%d年%d月%d日 星期%s"),
        _picked.year, _picked.month, _picked.day, weekTexts[calendar::Gregorian_CaculateWeekDay(_picked.year, _picked.month, _picked.day)]));
    _chineseDateLabel->setString(calendar::GetChineseDateTextLong(calendar::Gregorian2Chinese(_picked)));
}

static void setupLabelSmall(cocos2d::Label *label, int dayOffset, const std::pair<int, int> &solarTerms, const calendar::GregorianDate &dateGC, const calendar::ChineseDate &dateCC) {
    std::pair<calendar::GregorianDateFestival, int> gf = calendar::GetGregorianFestival(dateGC, dayOffset, solarTerms);
    std::pair<calendar::ChineseDateFestival, int> cf = calendar::GetChineseFestival(dateCC);

    if (gf.second == 3) {
        label->setString(calendar::GregorianDateFestivalNames[gf.first]);
        label->setTextColor(C4B_RED);
    }
    else if (cf.second >= 2) {
        label->setString(calendar::ChineseDateFestivalNames[cf.first]);
        label->setTextColor(C4B_RED);
    }
    else if (gf.second >= 2) {
        label->setString(calendar::GregorianDateFestivalNames[gf.first]);
        label->setTextColor(C4B_RED);
    }
    else if (solarTerms.first == dateGC.day) {
        label->setString(calendar::SolarTermsText[dateGC.month * 2 - 2]);
        label->setTextColor(C4B_RED);
    }
    else if (solarTerms.second == dateGC.day) {
        label->setString(calendar::SolarTermsText[dateGC.month * 2 - 1]);
        label->setTextColor(C4B_RED);
    }
    else if (cf.second > 0) {
        label->setString(calendar::ChineseDateFestivalNames[cf.first]);
        label->setTextColor(C4B_RED);
    }
    else if (gf.second > 0) {
        label->setString(calendar::GregorianDateFestivalNames[gf.first]);
        label->setTextColor(C4B_RED);
    }
    else {  // 没有节气的话，再显示日子
        if (dateCC.day > 1) {  // 不是初一
            label->setString(calendar::Chinese_DayText[dateCC.day - 1]);
            label->setTextColor(C4B_GRAY);
        }
        else {
            // 将初一显示为（闰）某月大/小的形式
            label->setString(calendar::GetChineseDateTextShort(dateCC));
            label->setTextColor(C4B_GRAY);
        }
    }
    cw::scaleLabelToFitWidth(label, 26.0f);
}

void DatePicker::setupDayContainer() {
    adjustDate(&_picked);
    refreshTitle();

    _state = PICK_STATE::DAY;
    _daysContainer->setVisible(true);
    _monthsContainer->setVisible(false);
    _yearsContainer->setVisible(false);
    _switchButton->setTitleText(Common::format(__UTF8("%d年%d月"), _picked.year, _picked.month));

    char str[32];

    // 本月的1号是星期几，那么前面的天数属于上个月
    int weekday = calendar::Gregorian_CaculateWeekDay(_picked.year, _picked.month, 1);
    for (int i = 0; i < weekday; ++i) {
        _dayBoxes[i]->setVisible(false);
    }
    _dayOffset = weekday;

    // 公历
    calendar::GregorianDate dateGC = {_picked.year, _picked.month, 1};
    int lastDayGC = calendar::Gregorian_LastDayOfMonth(dateGC.year, dateGC.month);

    // 农历
    calendar::ChineseDate dateCC = calendar::Gregorian2Chinese(dateGC);
    int lastDayCC = dateCC.is_long ? 30 : 29;  // 农历月最后一天

    // 本月的两个节气
    std::pair<int, int> solarTerms = calendar::GetSolarTermsInGregorianMonth(dateGC.year, dateGC.month);

    // 本月的天数
    for (int i = 0; i < lastDayGC; ++i) {
        int idx = weekday + i;
        ui::CheckBox *checkBox = _dayBoxes[idx];
        checkBox->setVisible(true);
        checkBox->setSelected(false);

        snprintf(str, sizeof(str), "%d", dateGC.day);
        Label *label = _dayLabelsLarge[idx];
        label->setString(str);
        label->setTextColor(C4B_GRAY);

        label = _dayLabelsSmall[idx];
        setupLabelSmall(label, weekday, solarTerms, dateGC, dateCC);

        // 日期增加
        ++dateGC.day;
        ++dateCC.day;
        if (dateCC.day > lastDayCC) {  // 超过农历月最后一天，重新计算
            dateCC = calendar::Gregorian2Chinese(dateGC);
            lastDayCC = dateCC.is_long ? 30 : 29;
        }
    }

    // 超出的属于下个月
    for (int i = weekday + lastDayGC; i < 42; ++i) {
        _dayBoxes[i]->setVisible(false);
    }

    // 高亮今天
    if (_picked.year == _today.year && _picked.month == _today.month) {
        int idx = weekday + _today.day - 1;
        Label *label = _dayLabelsLarge[idx];
        label->setTextColor(C4B_BLUE);

        label = _dayLabelsSmall[idx];
        if (label->getTextColor() == C4B_GRAY) {
            label->setTextColor(C4B_BLUE);
        }
    }
    _dayBoxes[_dayOffset + _picked.day - 1]->setSelected(true);
}

void DatePicker::setupMonthContainer() {
    adjustDate(&_picked);
    refreshTitle();

    _state = PICK_STATE::MONTH;
    _daysContainer->setVisible(false);
    _monthsContainer->setVisible(true);
    _yearsContainer->setVisible(false);
    _switchButton->setTitleText(Common::format(__UTF8("%d年"), _picked.year));

    if (_picked.year == _today.year) {
        ((Label *)_monthBoxes[_today.month - 1]->getUserData())->setTextColor(C4B_BLUE);
    }
    else {
        ((Label *)_monthBoxes[_today.month - 1]->getUserData())->setTextColor(C4B_GRAY);
    }

    for (int i = 0; i < 12; ++i) {
        _monthBoxes[i]->setSelected(_picked.month - 1 == i);
    }
}

void DatePicker::setupYearContainer() {
    _state = PICK_STATE::YEAR;
    _daysContainer->setVisible(false);
    _monthsContainer->setVisible(false);
    _yearsContainer->setVisible(true);

    _yearBoxes[_picked.year % 10]->setSelected(false);

    char str[64];

    snprintf(str, sizeof(str), "%d - %d", _decadeStart, _decadeStart + 9);
    _switchButton->setTitleText(str);

    for (int i = 0; i < 10; ++i) {
        ui::CheckBox *checkBox = _yearBoxes[i];
        Label *label = (Label *)checkBox->getUserData();
        int yy = _decadeStart + i;
        snprintf(str, sizeof(str), "%d", yy);
        label->setString(str);

        if (yy == _today.year) {
            label->setTextColor(C4B_BLUE);
        }
        else {
            label->setTextColor(C4B_GRAY);
        }

        checkBox->setSelected(yy == _picked.year);
    }
}

void DatePicker::onSwitchButton(cocos2d::Ref *) {
    switch (_state) {
    case PICK_STATE::DAY:
        setupMonthContainer();
        _state = PICK_STATE::MONTH;
        break;
    case PICK_STATE::MONTH:
        _monthsContainer->setVisible(false);
        _yearsContainer->setVisible(true);
        _decadeStart = _picked.year / 10 * 10;
        setupYearContainer();
        _state = PICK_STATE::YEAR;
        break;
    case PICK_STATE::YEAR:
        break;
    default:
        UNREACHABLE();
        break;
    }
}

void DatePicker::onTodayButton(cocos2d::Ref *) {
    if (_state == PICK_STATE::DAY && _picked.year == _today.year && _picked.month == _today.month) {
        // 本月内直接跳转
        if (_picked.day != _today.day) {
            _dayBoxes[_dayOffset + _picked.day - 1]->setSelected(false);
            _picked.day = _today.day;
            _dayBoxes[_dayOffset + _today.day - 1]->setSelected(true);
            refreshTitle();
        }
    }
    else {
        memcpy(&_picked, &_today, sizeof(_picked));
        setupDayContainer();
    }
}

void DatePicker::onBackwardButton(cocos2d::Ref *) {
    switch (_state) {
    case PICK_STATE::DAY:
        if (_picked.month > 1) {
            --_picked.month;
            setupDayContainer();
        }
        else if (_picked.year > LOWER_BOUND) {
            --_picked.year;
            _picked.month = 12;
            setupDayContainer();
        }
        break;
    case PICK_STATE::MONTH:
        if (_picked.year > LOWER_BOUND) {
            --_picked.year;
            setupMonthContainer();
        }
        break;
    case PICK_STATE::YEAR:
        if (_decadeStart - 10 >= LOWER_BOUND) {
            _decadeStart -= 10;
            setupYearContainer();
        }
        break;
    default:
        UNREACHABLE();
        break;
    }
}

void DatePicker::onForwardButton(cocos2d::Ref *) {
    switch (_state) {
    case PICK_STATE::DAY:
        if (_picked.month < 12) {
            ++_picked.month;
            setupDayContainer();
        }
        else if (_picked.year < UPPER_BOUND) {
            ++_picked.year;
            _picked.month = 1;
            setupDayContainer();
        }
        break;
    case PICK_STATE::MONTH:
        if (_picked.year < UPPER_BOUND) {
            ++_picked.year;
            setupMonthContainer();
        }
        break;
    case PICK_STATE::YEAR:
        if (_decadeStart + 10 <= UPPER_BOUND) {
            _decadeStart += 10;
            setupYearContainer();
        }
        break;
    default:
        UNREACHABLE();
        break;
    }
}

void DatePicker::onDayBox(cocos2d::Ref *sender, cocos2d::ui::CheckBox::EventType event) {
    ui::CheckBox *checkBox = (ui::CheckBox *)sender;
    if (event == ui::CheckBox::EventType::UNSELECTED) {
        checkBox->setSelected(true);
    }
    else {
        int day = checkBox->getTag() + 1 - _dayOffset;
        if (day != _picked.day) {
            _dayBoxes[_dayOffset + _picked.day - 1]->setSelected(false);
            _picked.day = day;
            refreshTitle();
        }
    }
}

void DatePicker::onMonthBox(cocos2d::Ref *sender, cocos2d::ui::CheckBox::EventType event) {
    ui::CheckBox *checkBox = (ui::CheckBox *)sender;
    if (event == ui::CheckBox::EventType::UNSELECTED) {
        checkBox->setSelected(true);
    }
    else {
        int month = checkBox->getTag() + 1;
        _monthBoxes[_picked.month - 1]->setSelected(false);
        _picked.month = month;
    }
    setupDayContainer();
}

void DatePicker::onYearBox(cocos2d::Ref *sender, cocos2d::ui::CheckBox::EventType event) {
    ui::CheckBox *checkBox = (ui::CheckBox *)sender;
    if (event == ui::CheckBox::EventType::UNSELECTED) {
        checkBox->setSelected(true);
    }
    else {
        int year = checkBox->getTag() + _decadeStart;
        _yearBoxes[_picked.year % 10]->setSelected(false);
        _picked.year = year;
    }
    setupMonthContainer();
}

void DatePicker::onPositiveButton(cocos2d::Ref *) {
    this->retain();
    _callback(this, true);
    this->release();
    this->dismiss();
}

void DatePicker::onNegativeButton(cocos2d::Ref *) {
    this->retain();
    _callback(this, false);
    this->release();
    this->dismiss();
}
