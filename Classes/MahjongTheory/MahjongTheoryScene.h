#ifndef __MAHJONG_THEORY_SCENE_H__
#define __MAHJONG_THEORY_SCENE_H__

#include "../BaseLayer.h"
#include "../mahjong-algorithm/shanten.h"
#include "../widget/CWTableView.h"
#include <unordered_map>

class HandTilesWidget;

class MahjongTheoryScene : public BaseLayer, cocos2d::ui::EditBoxDelegate, cw::TableViewDelegate {
public:
    static cocos2d::Scene *createScene();

    virtual bool init() override;

    CREATE_FUNC(MahjongTheoryScene);

private:
    cocos2d::ui::EditBox *_editBox;
    HandTilesWidget *_handTilesWidget;
    cocos2d::ui::Button *_undoButton;
    cocos2d::ui::Button *_redoButton;
    cocos2d::ui::CheckBox *_checkBoxes[4];
    cw::TableView *_tableView;

    int _handTilesTable[mahjong::TILE_TABLE_SIZE];

    struct ResultEx : mahjong::enum_result_t {
        int count_in_tiles;
        int count_total;
    };
    std::vector<mahjong::enum_result_t> _allResults;
    std::vector<ResultEx> _resultSources;
    std::vector<size_t> _orderedIndices;

    typedef struct {
        mahjong::hand_tiles_t handTiles;
        mahjong::tile_t servingTile;
        std::vector<mahjong::enum_result_t> allResults;
    } StateData;
    std::vector<StateData> _undoCache;
    std::vector<StateData> _redoCache;

    virtual void editBoxReturn(cocos2d::ui::EditBox *editBox) override;
    void setRandomInput();
    void onGuideButton(cocos2d::Ref *sender);
    bool parseInput(const char *input);
    void calculate();
    void filterResultsByFlag(uint8_t flag);
    uint8_t getFilterFlag() const;
    void onTileButton(cocos2d::Ref *sender);
    void onStandingTileEvent();
    void recoverFromState(StateData &state);
    void onUndoButton(cocos2d::Ref *sender);
    void onRedoButton(cocos2d::Ref *sender);
    void deduce(mahjong::tile_t discardTile, mahjong::tile_t servingTile);

    float _cellWidth;
    float _discardLabelWidth;
    float _servingLabelWidth1;
    float _servingLabelWidth2;
    float _waitingLabelWidth1;
    float _waitingLabelWidth2;
    float _totalLabelWidth;
    std::unordered_map<uint16_t, int> _cellHeightMap;

    virtual ssize_t numberOfCellsInTableView(cw::TableView *table) override;
    virtual cocos2d::Size tableCellSizeForIndex(cw::TableView *table, ssize_t idx) override;
    virtual cw::TableViewCell *tableCellAtIndex(cw::TableView *table, ssize_t idx) override;
};

#endif
