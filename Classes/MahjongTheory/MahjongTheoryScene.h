#ifndef __MAHJONG_THEORY_SCENE_H__
#define __MAHJONG_THEORY_SCENE_H__

#include "../BaseScene.h"
#include <unordered_map>
#include "../mahjong-algorithm/shanten.h"
#include "../cocos-wheels/CWTableView.h"

class HandTilesWidget;

class MahjongTheoryScene : public BaseScene, cocos2d::ui::EditBoxDelegate, cw::TableViewDelegate {
public:
    virtual bool init() override;

    CREATE_FUNC(MahjongTheoryScene);

private:
    cocos2d::ui::EditBox *_editBox = nullptr;
    HandTilesWidget *_handTilesWidget = nullptr;
    cocos2d::ui::Button *_undoButton = nullptr;
    cocos2d::ui::Button *_redoButton = nullptr;
    cocos2d::ui::CheckBox *_checkBoxes[4];
    cw::TableView *_tableView = nullptr;

    mahjong::tile_table_t _handTilesTable;

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
    void showInputAlert();
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

    float _cellWidth = 0.0f;
    float _discardLabelWidth = 0.0f;
    float _servingLabelWidth1 = 0.0f;
    float _servingLabelWidth2 = 0.0f;
    float _waitingLabelWidth1 = 0.0f;
    float _waitingLabelWidth2 = 0.0f;
    float _totalLabelWidth = 0.0f;
    std::unordered_map<uint16_t, int> _cellHeightMap;

    virtual ssize_t numberOfCellsInTableView(cw::TableView *table) override;
    virtual float tableCellSizeForIndex(cw::TableView *table, ssize_t idx) override;
    virtual cw::TableViewCell *tableCellAtIndex(cw::TableView *table, ssize_t idx) override;
};

#endif
