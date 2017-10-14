#ifndef _CW_TABLE_VIEW_H_
#define _CW_TABLE_VIEW_H_

#include <set>
#include <vector>
#include <tuple>
#include "ui/UIScrollView.h"

namespace cw {
    /**
     * Abstract class for SWTableView cell node
     */
    class TableViewCell : public cocos2d::ui::Widget {
    public:
        CREATE_FUNC(TableViewCell);

        TableViewCell() {}
        /**
         * The index used internally by SWTableView and its subclasses
         */
        ssize_t getIdx() const { return _idx; }
        void setIdx(ssize_t uIdx) { _idx = uIdx; }
        /**
         * Cleans up any resources linked to this cell and resets <code>idx</code> property.
         */
        void reset() { _idx = cocos2d::CC_INVALID_INDEX; }

    private:
        ssize_t _idx;
    };

    template <class ..._ARGS> class TableViewCellEx : public TableViewCell {
    public:
        static TableViewCellEx<_ARGS...> *create() {
            TableViewCellEx<_ARGS...> *ret = new (std::nothrow) TableViewCellEx<_ARGS...>();
            if (ret != nullptr && ret->init()) {
                ret->autorelease();
                return ret;
            }
            CC_SAFE_DELETE(ret);
            return nullptr;
        }

        typedef std::tuple<_ARGS...> ExtDataType;

        std::tuple<_ARGS...> &getExtData() {
            return _extData;
        }

        const std::tuple<_ARGS...> &getExtData() const {
            return _extData;
        }

    protected:
        std::tuple<_ARGS...> _extData;
    };

    class TableView;

    /**
     * Data source that governs table backend data.
     */
    class TableViewDelegate {
    public:
        /**
         * @js NA
         * @lua NA
         */
        virtual ~TableViewDelegate() { }

        /**
         * Returns number of cells in a given table view.
         *
         * @return number of cells
         */
        virtual ssize_t numberOfCellsInTableView(TableView *table) = 0;

        /**
         * cell size for a given index
         *
         * @param idx the index of a cell to get a size
         * @return size of a cell at given index, height of a cell if the table view is VERTICAL or width if HORIZONTAL

         */
        virtual float tableCellSizeForIndex(TableView *table, ssize_t idx) {
            CC_UNUSED_PARAM(table);
            CC_UNUSED_PARAM(idx);
            return 0.0f;
        }

        /**
         * a cell instance at a given index
         *
         * @param idx index to search for a cell
         * @return cell found at idx
         */
        virtual TableViewCell* tableCellAtIndex(TableView *table, ssize_t idx) = 0;

        /**
         * Delegate called when the cell is about to be recycled. Immediately
         * after this call the cell will be removed from the scene graph and
         * recycled.
         *
         * @param table table contains the given cell
         * @param cell  cell that is pressed
         * @js NA
         * @lua NA
         */
        virtual void tableCellWillRecycle(TableView *table, TableViewCell *cell) {
            CC_UNUSED_PARAM(table);
            CC_UNUSED_PARAM(cell);
        }
    };

    class TableView : public cocos2d::ui::ScrollView {
        DECLARE_CLASS_GUI_INFO

    public:
        enum class VerticalFillOrder {
            TOP_DOWN,
            BOTTOM_UP
        };

        /**
         * Default constructor
         */
        TableView();

        /**
         * Default destructor
         */
        virtual ~TableView();

        /**
         * Allocates and initializes.
         */
        static TableView *create();
        virtual bool init() override;

        /**
         * delegate
         */
        TableViewDelegate *getDelegate() { return _delegate; }
        void setDelegate(TableViewDelegate *pDelegate) { _delegate = pDelegate; }

        /**
         * determines how cell is ordered and filled in the view.
         */
        void setVerticalFillOrder(VerticalFillOrder order);
        VerticalFillOrder getVerticalFillOrder();

        /**
         * Updates the content of the cell at a given index.
         *
         * @param idx index to find a cell
         */
        void updateCellAtIndex(ssize_t idx);
        /**
         * Inserts a new cell at a given index
         *
         * @param idx location to insert
         */
        void insertCellAtIndex(ssize_t idx);
        /**
         * Removes a cell at a given index
         *
         * @param idx index to find a cell
         */
        void removeCellAtIndex(ssize_t idx);
        /**
         * reloads data from data source.  the view will be refreshed.
         */
        void reloadData();
        void reloadDataInplacement();

        /**
         * Dequeues a free cell if available. nil if not.
         *
         * @return free cell
         */
        TableViewCell *dequeueCell();

        /**
         * Returns an existing cell at a given index. Returns nil if a cell is nonexistent at the moment of query.
         *
         * @param idx index
         * @return a cell at a given index
         */
        TableViewCell *cellAtIndex(ssize_t idx);

        virtual void setDirection(cocos2d::ui::ScrollView::Direction dir) override;

        cocos2d::Vec2 maxContainerOffset();
        cocos2d::Vec2 minContainerOffset();

        /**
         * @brief Jump to specific cell
         * @param cellIndex Specifies the cell's index
         */
        void jumpToCell(ssize_t cellIndex);

        /**
         * @brief Scroll to specific cell
         * @param cellIndex Specifies the cell's index
         * @param timeInSec Scroll time
         * @param attenuated Whether scroll speed attenuate or not
         */
        void scrollToCell(ssize_t cellIndex, float timeInSec, bool attenuated);

    protected:
        virtual void onSizeChanged() override;

        virtual void moveInnerContainer(const cocos2d::Vec2 &deltaMove, bool canStartBounceBack) override;

        ssize_t __indexFromOffset(const cocos2d::Vec2 &offset, ssize_t cellsCount);
        ssize_t _indexFromOffset(cocos2d::Vec2 offset, ssize_t cellsCount);
        cocos2d::Vec2 __offsetFromIndex(ssize_t index);
        cocos2d::Vec2 _offsetFromIndex(ssize_t index);

        void _moveCellOutOfSight(TableViewCell *cell);
        void _setIndexForCell(ssize_t index, TableViewCell *cell);
        void _addCellIfNecessary(TableViewCell *cell);

        void _updateCellPositions(ssize_t cellsCount);
        void _scrollViewDidScroll(ssize_t cellsCount);

        cocos2d::Vec2 _destinationFromIndex(ssize_t index);

        /**
         * vertical direction of cell filling
         */
        VerticalFillOrder _vordering;

        /**
         * index set to query the indexes of the cells used.
         */
        std::set<ssize_t> _indices;

        /**
         * vector with all cell positions
         */
        std::vector<float> _cellsPositions;
        //NSMutableIndexSet *indices_;
        /**
         * cells that are currently in the table
         */
        cocos2d::Vector<TableViewCell *> _cellsUsed;
        /**
         * free list of cells
         */
        cocos2d::Vector<TableViewCell *> _cellsFreed;

        TableViewDelegate *_delegate;

        bool _isUsedCellsDirty;

        void _updateInnerContainerSize(ssize_t cellsCount);

    public:
        void updateInnerContainerSize();

    private:
        using cocos2d::ui::ScrollView::setInnerContainerSize;
        using cocos2d::ui::ScrollView::addChild;
        using cocos2d::ui::ScrollView::removeAllChildren;
        using cocos2d::ui::ScrollView::removeAllChildrenWithCleanup;
        using cocos2d::ui::ScrollView::removeChild;

        using cocos2d::ui::ScrollView::addEventListenerScrollView;
        using cocos2d::ui::ScrollView::addEventListener;
    };
}

#endif
