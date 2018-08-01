#ifndef _DATE_PICKER_H_
#define _DATE_PICKER_H_

#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include "../cocos-wheels/CWCommon.h"

#define DATE_PICKER_Z_ORDER 100

class DatePicker : cocos2d::Layer {
public:
    struct Date {
        int year;
        int month;
        int day;
    };
    typedef std::function<void (DatePicker *thiz, bool confirm)> Callback;

    CREATE_FUNC_WITH_PARAM_2(DatePicker, const Date *, date, Callback &&, callback);
    bool init(const Date *date, Callback &&callback);

    const Date &getDate() { return _picked; }

    void showInScene(cocos2d::Scene *scene) {
        scene->addChild(this, DATE_PICKER_Z_ORDER);
    }

    void dismiss() {
        this->removeFromParent();
    }

private:
    void refreshResultLabel();

    void setupDayContainer();
    void setupMonthContainer();
    void setupYearContainer();

    void onSwitchButton(cocos2d::Ref *sender);
    void onUpButton(cocos2d::Ref *sender);
    void onDownButton(cocos2d::Ref *sender);

    void onDayBox(cocos2d::Ref *sender, cocos2d::ui::CheckBox::EventType event);
    void onMonthBox(cocos2d::Ref *sender, cocos2d::ui::CheckBox::EventType event);
    void onYearBox(cocos2d::Ref *sender, cocos2d::ui::CheckBox::EventType event);

    void onPositiveButton(cocos2d::Ref *sender);
    void onNegativeButton(cocos2d::Ref *sender);

    cocos2d::Node *_daysContainer = nullptr;
    cocos2d::Node *_monthsContainer = nullptr;
    cocos2d::Node *_yearsContainer = nullptr;
    cocos2d::ui::CheckBox *_dayBoxes[42];
    cocos2d::ui::CheckBox *_monthBoxes[12];
    cocos2d::ui::CheckBox *_yearBoxes[10];

    cocos2d::ui::Button *_switchButton = nullptr;
    cocos2d::Label *_resultLabel = nullptr;

    enum class PICK_STATE { DAY, MONTH, YEAR };
    PICK_STATE _state = PICK_STATE::DAY;

    int _dayOffset = 0;
    int _decadeStart = 0;

    Date _today;
    Date _picked;
    Callback _callback;
};

#endif
