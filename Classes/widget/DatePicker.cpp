#include "DatePicker.h"
#include "CheckBoxScale9.h"
#include "../UICommon.h"
#include "../UIColors.h"
#include "../utils/common.h"

USING_NS_CC;

#define C4B_BLUE cocos2d::Color4B(44, 121, 178, 255)
#define C4B_RED  cocos2d::Color4B(254, 87, 110, 255)

#define LOWER_BOUND 1800
#define UPPER_BOUND 2299

static const char *weekTexts[] = { __UTF8("日"), __UTF8("一"), __UTF8("二"), __UTF8("三"), __UTF8("四"), __UTF8("五"), __UTF8("六") };

static const char *SolarTermsText[] = {
    __UTF8("小寒"), __UTF8("大寒"),
    __UTF8("立春"), __UTF8("雨水"), __UTF8("惊蛰"), __UTF8("春分"), __UTF8("清明"), __UTF8("谷雨"),
    __UTF8("立夏"), __UTF8("小满"), __UTF8("芒种"), __UTF8("夏至"), __UTF8("小暑"), __UTF8("大暑"),
    __UTF8("立秋"), __UTF8("处暑"), __UTF8("白露"), __UTF8("秋分"), __UTF8("寒露"), __UTF8("霜降"),
    __UTF8("立冬"), __UTF8("小雪"), __UTF8("大雪"), __UTF8("冬至")
};

static const char *CelestialStem[10] = {
    __UTF8("甲"), __UTF8("乙"), __UTF8("丙"), __UTF8("丁"), __UTF8("戊"), __UTF8("己"), __UTF8("庚"), __UTF8("辛"), __UTF8("壬"), __UTF8("癸")
};

static const char *TerrestrialBranch[12] = {
    __UTF8("子"), __UTF8("丑"), __UTF8("寅"), __UTF8("卯"), __UTF8("辰"), __UTF8("巳"), __UTF8("午"), __UTF8("未"), __UTF8("申"), __UTF8("酉"), __UTF8("戌"), __UTF8("亥")
};

static const char *ChineseZodiac[12] = {
    __UTF8("鼠"), __UTF8("牛"), __UTF8("虎"), __UTF8("兔"), __UTF8("龙"), __UTF8("蛇"), __UTF8("马"), __UTF8("羊"), __UTF8("猴"), __UTF8("鸡"), __UTF8("狗"), __UTF8("猪")
};

static const char *Chinese_MonthText[] = {
    __UTF8("正月"), __UTF8("二月"), __UTF8("三月"), __UTF8("四月"), __UTF8("五月"), __UTF8("六月"), __UTF8("七月"), __UTF8("八月"), __UTF8("九月"), __UTF8("十月"), __UTF8("十一月"), __UTF8("十二月")
};

static const char *Chinese_DayText[] = {
    __UTF8("初一"), __UTF8("初二"), __UTF8("初三"), __UTF8("初四"), __UTF8("初五"), __UTF8("初六"), __UTF8("初七"), __UTF8("初八"), __UTF8("初九"), __UTF8("初十"),
    __UTF8("十一"), __UTF8("十二"), __UTF8("十三"), __UTF8("十四"), __UTF8("十五"), __UTF8("十六"), __UTF8("十七"), __UTF8("十八"), __UTF8("十九"), __UTF8("二十"),
    __UTF8("廿一"), __UTF8("廿二"), __UTF8("廿三"), __UTF8("廿四"), __UTF8("廿五"), __UTF8("廿六"), __UTF8("廿七"), __UTF8("廿八"), __UTF8("廿九"), __UTF8("三十")
};

static std::string GetChineseDateTextLong(const calendar::ChineseDate &date) {
    int cz = (date.year + 8) % 12;
    std::string str;
    str.reserve(64);
    str.append(CelestialStem[(date.year + 6) % 10]);
    str.append(TerrestrialBranch[cz]);
    str.append(__UTF8("（"));
    str.append(ChineseZodiac[cz]);
    str.append(__UTF8("）年"));
    if (date.leap) str.append(__UTF8("闰"));
    str.append(Chinese_MonthText[date.month - 1]);
    str.append(Chinese_DayText[date.day - 1]);
    return str;
}

static std::string GetChineseDateTextShort(const calendar::ChineseDate &date) {
    if (date.day > 1) {
        return Chinese_DayText[date.day - 1];
    }
    else {
        std::string str;
        str.reserve(16);
        if (date.leap) str.append(__UTF8("闰"));
        str.append(Chinese_MonthText[date.month - 1]);
        str.append(date.major ? __UTF8("大") : __UTF8("小"));
        return str;
    }
}

struct Festival {
    int priority;  // 优先级
    const char *name;  // 节日名
    int cal;  // 0农历，2公历
    int month;
    int day;
    int since;  // 从多少年起
    int week;  // 星期几（公历有一些节日，如母亲节、父亲节是固定在星期的）
    int offset;  // 偏移，如上面母亲节、父亲节，在第2第3个星期日
};

#define WINTER_99_PRIORITY 1
#define DOG_DAYS_PRIORITY 1

static std::array<Festival, 37> g_festivals = {
    Festival{ 3, __UTF8("元旦"), 0, 1, 1 },
    Festival{ 3, __UTF8("春节"), 0, 1, 1, 1912},
    Festival{ 3, __UTF8("元宵节"), 0, 1, 15 },
    Festival{ 2, __UTF8("龙抬头"), 0, 2, 2 },
    Festival{ 2, __UTF8("上巳节"), 0, 3, 3 },
    Festival{ 2, __UTF8("佛诞"), 0, 4, 8 },
    Festival{ 3, __UTF8("端午节"), 0, 5, 5 },
    Festival{ 2, __UTF8("七夕"), 0, 7, 7 },
    Festival{ 2, __UTF8("中元节"), 0, 7, 15 },
    Festival{ 3, __UTF8("中秋节"), 0, 8, 15 },
    Festival{ 2, __UTF8("重阳节"), 0, 9, 9 },
    Festival{ 2, __UTF8("寒衣节"), 0, 10, 1 },
    Festival{ 2, __UTF8("下元节"), 0, 10, 15 },
    Festival{ 2, __UTF8("腊八节"), 0, 12, 8 },
    Festival{ 2, __UTF8("北方小年"), 0, 12, 23 },
    Festival{ 2, __UTF8("南方小年"), 0, 12, 24 },
    Festival{ 3, __UTF8("除夕"), 0, 12, 30 },
    Festival{ 3, __UTF8("元旦"), 2, 1, 1, 1912 },
    Festival{ 2, __UTF8("情人节"), 2, 2, 14 },
    Festival{ 3, __UTF8("妇女节"), 2, 3, 8, 1976 },
    Festival{ 2, __UTF8("植树节"), 2, 3, 12, 1979 },
    Festival{ 2, __UTF8("愚人节"), 2, 4, 1 },
    Festival{ 3, __UTF8("劳动节"), 2, 5, 1, 1950 },
    Festival{ 3, __UTF8("青年节"), 2, 5, 4, 1950 },
    Festival{ 2, __UTF8("母亲节"), 2, 5, 0, 1913, 0, 2 },
    Festival{ 3, __UTF8("儿童节"), 2, 6, 1, 1950 },
    Festival{ 2, __UTF8("父亲节"), 2, 6, 0, 1910, 0, 3 },
    Festival{ 3, __UTF8("建党节"), 2, 7, 1, 1938 },
    Festival{ 3, __UTF8("建军节"), 2, 8, 1, 1933 },
    Festival{ 2, __UTF8("抗战胜利"), 2, 9, 3, 1945 },
    Festival{ 3, __UTF8("教师节"), 2, 9, 9, 1985 },
    Festival{ 2, __UTF8("烈士纪念日"), 2, 9, 30, 2014 },
    Festival{ 3, __UTF8("国庆节"), 2, 10, 1, 1949 },
    Festival{ 2, __UTF8("感恩节"), 2, 11, 0, 0, 4, 4 },
    Festival{ 3, __UTF8("国家公祭日"), 2, 12, 13, 2014 },
    Festival{ 2, __UTF8("平安夜"), 2, 12, 24 },
    Festival{ 2, __UTF8("圣诞节"), 2, 12, 25 }
};

struct AllFestivals {
    Festival data[16];
    unsigned count;
};

// 春社：立春后五戊
// 设首戊为D号，五戊为：D+50-28=D+22（平年）；D+50-29=D+21（闰年），有落入4月的可能
static void GetSpringSocial(AllFestivals &festivals, int y, int m, bool leap) {
    int d = calendar::Gregorian_GetSolarTerm(y, 2);
    int s = calendar::Gregorian_GetStemBranch(y, 2, d) % 10;  // 立春日天干
    d += (s <= 4 ? 4 : 14) - s;
    d += (leap ? 21 : 22);
    if ((m == 3 && d <= 31) || (m == 4 && d > 31)) {
        festivals.data[festivals.count++] = Festival{ 2, __UTF8("春社"), 2, m, d <= 31 ? d : d - 31 };
    }
}

// 秋社：立秋后第五个戊日
// 设首戊为D号，五戊为：D+50-31=D+19，有落入10月的可能
static void GetAutumnSocial(AllFestivals &festivals, int y, int m) {
    int d = calendar::Gregorian_GetSolarTerm(y, 14);
    int s = calendar::Gregorian_GetStemBranch(y, 8, d) % 10;  // 立秋日天干
    d += (s <= 4 ? 4 : 14) - s;
    d += 19;
    if ((m == 9 && d <= 31) || (m == 10 && d > 31)) {
        festivals.data[festivals.count++] = Festival{ 2, __UTF8("秋社"), 2, m, d <= 31 ? d : d - 31 };
    }
}

static void GetFestivals(int y, int m, int mm, int dd, bool l, AllFestivals &festivals) {
    for (auto &f : g_festivals) {
        int fm = f.month, d;
        switch (f.cal) {
        case 0:
            d = f.day;
            // 农历节日
            // 1. 闰月中不可能有节日，过滤掉已经过了的节日
            // 2. 公历一个月最多31天，当1号是A月B日时，31号可能是(A+1)月(B+1)日
            // 3. 当1号是A月最后一天时，即2号是(A+1)月初一时，31号可能是(A+2)月初一
            if ((!l && fm == mm && d >= dd) || ((fm == mm + 1 || (fm == 1 && mm == 12)) && (d < dd + 2)) || (dd > 28 && fm == (mm + 1) % 12 + 1 && d < 2)) {
                festivals.data[festivals.count++] = f;
            }
            break;
        case 2:
            if (y >= f.since && m == fm) {
                festivals.data[festivals.count++] = f;
            }
            break;
        }
    }

    int solarTerms[2];
    for (int i = 0; i < 2; ++i) {
        unsigned idx = m * 2 - 2 + i;
        solarTerms[i] = calendar::Gregorian_GetSolarTerm(y, idx);
        festivals.data[festivals.count++] = Festival{ 2, SolarTermsText[idx], 2, m, solarTerms[i] };
    }

    int d, s;
    switch (m) {
    case 1:
        // 1月有（二九）、三九、四九、五九
        // 设冬至为W号，三九为：W+9*2-31=W-13
        d = calendar::Gregorian_GetSolarTerm(y - 1, 23) - 13;
        if (d > 9) {
            festivals.data[festivals.count++] = Festival{ WINTER_99_PRIORITY, __UTF8("二九"), 2, 1, d - 9 };
        }
        festivals.data[festivals.count++] = Festival{ WINTER_99_PRIORITY, __UTF8("三九"), 2, 1, d };
        festivals.data[festivals.count++] = Festival{ WINTER_99_PRIORITY, __UTF8("四九"), 2, 1, d + 9 };
        festivals.data[festivals.count++] = Festival{ WINTER_99_PRIORITY, __UTF8("五九"), 2, 1, d + 18 };
        break;
    case 2:
        // 2月有六九、七九、八九
        // 设冬至为W号，六九为：W+9*5-31-31=W-17
        d = calendar::Gregorian_GetSolarTerm(y - 1, 23) - 17;
        festivals.data[festivals.count++] = Festival{ WINTER_99_PRIORITY, __UTF8("六九"), 2, 2, d };
        festivals.data[festivals.count++] = Festival{ WINTER_99_PRIORITY, __UTF8("七九"), 2, 2, d + 9 };
        festivals.data[festivals.count++] = Festival{ WINTER_99_PRIORITY, __UTF8("八九"), 2, 2, d + 18 };
        break;
    case 3:
        // 3月有九九
        // 设冬至为W号，九九为：W+9*8-31-31-28=w-18（平年）；W+9*8-31-31-29=W-19（闰年）
        l = calendar::Gregorian_IsLeapYear(y);
        d = calendar::Gregorian_GetSolarTerm(y - 1, 23) - (l ? 19 : 18);
        festivals.data[festivals.count++] = Festival{ WINTER_99_PRIORITY, __UTF8("九九"), 2, 3, d };

        // 春社
        GetSpringSocial(festivals, y, 3, l);
        break;
    case 4:
        // 寒食节，清明前一天
        festivals.data[festivals.count++] = Festival{ 2, __UTF8("寒食节"), 2, 4, solarTerms[0] - 1};

        // 春社
        GetSpringSocial(festivals, y, 4, calendar::Gregorian_IsLeapYear(y));
        break;
    case 5:
        break;
    case 6:
        // 芒种见丙入梅
        // 由于紫金山天文台目前只能查到2022年~2024年，这三年未出现的芒种当天是丙日的情况，只能根据2024出梅的算法猜测亦如此
        d = solarTerms[0];
        s = calendar::Gregorian_GetStemBranch(y, 6, d) % 10;  // 芒种日天干
        festivals.data[festivals.count++] = Festival{ 1, __UTF8("入梅"), 2, 6, d + (s > 2 ? 12 : 2) - s };
        break;
    case 7:
        // 7月有初伏、中伏
        // 初伏：夏至后的第三个庚日；中伏：夏至后的第四个庚日
        // 夏至最晚在6月22日，极端情况前一日（21日）为庚日，那么初伏将在7月21日，中伏在31日，不会落入8月
        // 对于夏至当天是庚日的情况，紫金山天文台的算法是也算一次
        // 例如2023年夏至是6月21日，干支为庚戌，7月11日为初伏，21日为中伏
        d = calendar::Gregorian_GetSolarTerm(y, 11);  // 夏至日期
        s = calendar::Gregorian_GetStemBranch(y, 6, d) % 10;  // 夏至日天干
        if (s <= 6) {
            d -= 4 + s;
        }
        else {
            d += 6 - s;
        }
        festivals.data[festivals.count++] = Festival{ DOG_DAYS_PRIORITY, __UTF8("初伏"), 2, 7, d };
        festivals.data[festivals.count++] = Festival{ DOG_DAYS_PRIORITY, __UTF8("中伏"), 2, 7, d + 10 };

        // 小暑见未出梅
        // 对于小暑当天是未日的情况，紫金山天文台的算法是当天即出梅
        // 例如2024年小暑是7月6日，干支为辛未
        d = solarTerms[0];
        s = calendar::Gregorian_GetStemBranch(y, 7, d) % 12;  // 小暑日地支
        festivals.data[festivals.count++] = Festival{ 1, __UTF8("出梅"), 2, 7, d + (s <= 7 ? 7 : 19) - s };
        break;
    case 8:
        // 8月有末伏
        // 末伏：立秋后第一个庚日
        // 对于立秋当天是庚日的情况，网络上主流的算法是直接算末伏，这里保持跟网上一致
        // 由于紫金山天文台目前只能查到2022年~2024年，这三年未出现的立秋当天是庚日的情况，只能根据2023初伏的算法猜测亦如此
        d = solarTerms[0];  // 立秋日期
        s = calendar::Gregorian_GetStemBranch(y, 8, d) % 10;  // 立秋日天干
        festivals.data[festivals.count++] = Festival{ DOG_DAYS_PRIORITY, __UTF8("末伏"), 2, 8,  d + (s <= 6 ? 6 : 16) - s };
        break;
    case 9:
    case 10:
        // 秋社
        GetAutumnSocial(festivals, y, m);
        break;
    case 11:
        break;
    case 12:
        // 冬至日当天就是一九第1天，冬至如果出现在22号之后，二九就会落在次年1月
        d = calendar::Gregorian_GetSolarTerm(y, 23) + 9;
        if (d <= 31) {
            festivals.data[festivals.count++] = Festival{ WINTER_99_PRIORITY, __UTF8("二九"), 2, 12, d };
        }
        break;
    default:
        break;
    }
}

static const Festival *PickFestival(const calendar::GregorianDate &g, const calendar::ChineseDate &c, int week, const AllFestivals &festivals) {
    const Festival *res = nullptr;
    for (unsigned i = 0, l = festivals.count; i < l; ++i) {
        auto &f = festivals.data[i];
        bool pick = false;
        int since = f.since;
        switch (f.cal) {
        case 0:
            // 农历节日不会出现在闰月
            if ((!since || c.year >= since) && !c.leap && c.month == f.month) {
                int d1 = f.day, d2 = c.day;
                // 类似除夕那种在三十的节日，如果当月没有三十，则改为廿九
                if (d1 == d2 || (d1 == 30 && d2 == 29 && !c.major)) {
                    pick = true;
                }
            }
            break;
        case 2:
            if ((!since || g.year >= since) && g.month == f.month) {
                int d1 = f.day, d2 = g.day;
                if (d1 == d2) {
                    pick = true;
                }
                else if (d1 == 0) {
                    // 固定在星期几的节日
                    if (week == f.week) {
                        if (f.offset >= 0) {
                            int t = f.offset * 7 - d2;
                            if (t >= 0 && t <= 6) {
                                pick = true;
                            }
                        }
                    }
                }
            }
            break;
        default:
            break;
        }

        if (pick && (res == nullptr || res->priority < f.priority || res->since < f.since)) {
            res = &f;
        }
    }

    return res;
}

bool DatePicker::init(const Date *date, Callback &&callback) {
    if (!Layer::init()) {
        return false;
    }

    _callback.swap(callback);

    time_t now = time(nullptr);
    struct tm tmnow = *localtime(&now);
    _today.year = tmnow.tm_year + 1900;
    _today.month = tmnow.tm_mon + 1;
    _today.day = tmnow.tm_mday;
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

        Label *label = Label::createWithSystemFont("", "Arial", 12);
        label->setTextColor(C4B_GRAY);
        checkBox->addChild(label);
        label->setPosition(Vec2(15.0f, 20.0f));
        _dayLabelsLarge[i] = label;

        label = Label::createWithSystemFont("", "Arial", 7);
        label->setTextColor(C4B_GRAY);
        checkBox->addChild(label);
        label->setPosition(Vec2(15.0f, 7.0f));
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
    label->setPosition(Vec2(totalWidth - 5.0f, totalHeight + 12.0f));
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
    touchListener->onTouchBegan = [this](Touch *, Event *) {
        return true;
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(touchListener, this);
    _touchListener = touchListener;

    setupDayContainer();

    return true;
}

static void adjustDate(calendar::GregorianDate *date) {
    auto lastDay = calendar::Gregorian_DaysInMonth(date->year, date->month);
    date->day = std::min(date->day, lastDay);
}

void DatePicker::refreshTitle(const calendar::ChineseDate *date) {
    _titleLabel->setString(Common::format(__UTF8("%d年%d月%d日 星期%s"),
        _picked.year, _picked.month, _picked.day, weekTexts[calendar::Gregorian_WeekDay(_picked.year, _picked.month, _picked.day)]));
    _chineseDateLabel->setString(GetChineseDateTextLong(date != nullptr ? *date : calendar::Gregorian2Chinese(_picked)));
}

static void setupLabelSmall(cocos2d::Label *label,
    const calendar::GregorianDate &dateGC, const calendar::ChineseDate &dateCC,
    int week, const AllFestivals &festivals) {
    const Festival *festival = PickFestival(dateGC, dateCC, week, festivals);

    // 优先顺序：优先度>1的节日、二十四节气、优先度<=的节日
    if (festival != nullptr) {
        label->setString(festival->name);
        label->setTextColor(C4B_RED);
    }
    else {  // 没有节气的话，再显示日子
        if (dateCC.day > 1) {  // 不是初一
            label->setString(Chinese_DayText[dateCC.day - 1]);
            label->setTextColor(C4B_GRAY);
        }
        else {
            // 将初一显示为（闰）某月大/小的形式
            label->setString(GetChineseDateTextShort(dateCC));
            label->setTextColor(C4B_GRAY);
        }
    }
    cw::scaleLabelToFitWidth(label, 26.0f);
}

void DatePicker::setupDayContainer() {
    adjustDate(&_picked);

    _state = PICK_STATE::DAY;
    _daysContainer->setVisible(true);
    _monthsContainer->setVisible(false);
    _yearsContainer->setVisible(false);
    _switchButton->setTitleText(Common::format(__UTF8("%d年%d月"), _picked.year, _picked.month));

    char str[32];

    // 本月的1号是星期几，那么前面的天数属于上个月
    int weekday = calendar::Gregorian_WeekDay(_picked.year, _picked.month, 1);
    for (int i = 0; i < weekday; ++i) {
        _dayBoxes[i]->setVisible(false);
    }
    _dayOffset = weekday;

    // 公历
    calendar::GregorianDate dateGC = {_picked.year, _picked.month, 1};
    int lastDayGC = calendar::Gregorian_DaysInMonth(dateGC.year, dateGC.month);

    // 农历
    calendar::ChineseDate dateCC = calendar::Gregorian2Chinese(dateGC);
    int lastDayCC = dateCC.major ? 30 : 29;  // 农历月最后一天

    AllFestivals festivals{};
    GetFestivals(dateGC.year, dateGC.month, dateCC.month, dateCC.day, dateCC.leap, festivals);

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
        setupLabelSmall(label, dateGC, dateCC, (weekday + i) % 7, festivals);

        _chineseDate[idx] = dateCC;

        // 日期增加
        ++dateGC.day;
        ++dateCC.day;
        if (dateCC.day > lastDayCC) {  // 超过农历月最后一天，农历到下一个月
            dateCC = calendar::Gregorian2Chinese(dateGC);
            lastDayCC = dateCC.major ? 30 : 29;
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

    refreshTitle(&_chineseDate[_dayOffset + _picked.day - 1]);
}

void DatePicker::setupMonthContainer() {
    adjustDate(&_picked);

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

    refreshTitle(nullptr);
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
            refreshTitle(&_chineseDate[_dayOffset + _picked.day - 1]);
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
            refreshTitle(&_chineseDate[_dayOffset + _picked.day - 1]);
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
