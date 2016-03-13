#ifndef __SCORE_SHEET_SCENE_H__
#define __SCORE_SHEET_SCENE_H__

#include "cocos2d.h"
#include "ui/CocosGUI.h"

class ScoreSheetScene : public cocos2d::Layer {
public:
    static cocos2d::Scene *createScene();

    virtual bool init() override;

    virtual void onKeyReleased(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event *unusedEvent) override;
    CREATE_FUNC(ScoreSheetScene);

private:
    cocos2d::ui::EditBox *_editBox[4];
    int _scores[4];
    cocos2d::ui::Button *_lockButton;
    cocos2d::Label *_totalLabel[4];
    cocos2d::Label *_scoreLabels[16][4];
    cocos2d::ui::Button *_recordButton[16];
    cocos2d::Label *_timeLabel;

    void recover();
    void reset();
    void lockCallback(cocos2d::Ref *sender);
    void recordCallback(cocos2d::Ref *sender, int index);
    void timeScheduler(float dt);
};

#endif // __SCORE_SHEET_SCENE_H__
