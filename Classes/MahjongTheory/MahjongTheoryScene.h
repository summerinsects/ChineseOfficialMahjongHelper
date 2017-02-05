#ifndef __MAHJONG_THEORY_SCENE_H__
#define __MAHJONG_THEORY_SCENE_H__

#include "../BaseLayer.h"
#include "../mahjong-algorithm/wait_and_win.h"
#include <unordered_map>

namespace cw {
    class TableViewCell;
    class TableView;
}

class HandTilesWidget;

class MahjongTheoryScene : public BaseLayer, cocos2d::ui::EditBoxDelegate {
public:
    static cocos2d::Scene *createScene();

    virtual bool init() override;

    CREATE_FUNC(MahjongTheoryScene);

private:
    cocos2d::ui::EditBox *_editBox;
    HandTilesWidget *_handTilesWidget;
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

    virtual void editBoxReturn(cocos2d::ui::EditBox *editBox) override;
    void setRandomInput();
    void onGuideButton(cocos2d::Ref *sender);
    bool parseInput(const char *input);
    void calculate();
    void filterResultsByFlag(uint8_t flag);
    uint8_t getFilterFlag() const;
    void onTileButton(cocos2d::Ref *sender);
    void onStandingTileEvent();
    void deduce(mahjong::tile_t discardTile, mahjong::tile_t servingTile);

    float _cellWidth;
    float _discardLabelWidth;
    float _servingLabelWidth1;
    float _servingLabelWidth2;
    float _waitingLabelWidth1;
    float _waitingLabelWidth2;
    float _totalLabelWidth;
    std::unordered_map<uint16_t, int> _cellHeightMap;
    cocos2d::Size tableCellSizeAtIndex(cw::TableView *table, ssize_t idx);
    cw::TableViewCell *tableCellAtIndex(cw::TableView *table, ssize_t idx);
};

#endif
