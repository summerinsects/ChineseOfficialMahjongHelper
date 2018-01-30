#ifndef __SCORE_SHEET_SCENE_H__
#define __SCORE_SHEET_SCENE_H__

#include "../BaseScene.h"
#include "Record.h"

class ScoreSheetScene : public BaseScene {
public:
    virtual bool init() override;

    CREATE_FUNC(ScoreSheetScene);

private:
    float _cellWidth = 0.0f;
    cocos2d::Label *_nameLabel[4];
    cocos2d::ui::Button *_lockButton = nullptr;
    cocos2d::Label *_totalLabel[4];
    cocos2d::Label *_scoreLabels[16][4];
    cocos2d::ui::Button *_recordButton[16];
    cocos2d::ui::Widget *_detailWidget[16];
    cocos2d::Label *_fanNameLabel[16];
    cocos2d::Label *_rankLabels[4];
    cocos2d::Label *_timeLabel = nullptr;

    Record _record;
    bool _isGlobal = false;
    bool _isTotalMode = false;
    std::string _prevName[4];

    bool initWithRecord(Record *record);

    void skipScores(size_t handIdx, int (&totalScores)[4]) const;
    void fillScoresForSingleMode(size_t handIdx, int (&totalScores)[4]);
    void fillScoresForTotalMode(size_t handIdx, int (&totalScores)[4]);
    void fillDetail(size_t handIdx);
    void cleanRow(size_t handIdx);
    void refreshRank(const int (&totalScores)[4]);
    void refreshStartTime();
    void refreshEndTime();
    void recover();
    void reset();
    void onNameButton(cocos2d::Ref *sender, size_t idx);
    void editName(size_t idx);
    bool submitName(const char *text, size_t idx);
    void editNameAllAtOnce();
    void onLockButton(cocos2d::Ref *sender);
    void onRecordButton(cocos2d::Ref *sender, size_t handIdx);
    void onDetailButton(cocos2d::Ref *sender, size_t handIdx);
    void editRecord(size_t handIdx, bool modify);
    void onTimeScheduler(float dt);
    void onInstructionButton(cocos2d::Ref *sender);
    void onModeButton(cocos2d::Ref *sender);
    void onHistoryButton(cocos2d::Ref *sender);
    void onResetButton(cocos2d::Ref *sender);
    void onPursuitButton(cocos2d::Ref *sender);
    void onScoreButton(cocos2d::Ref *sender, size_t idx);
};

#endif // __SCORE_SHEET_SCENE_H__
