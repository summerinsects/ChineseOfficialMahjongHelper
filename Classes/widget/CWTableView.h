#ifndef _CW_TABLE_VIEW_H_
#define _CW_TABLE_VIEW_H_

#include "ui/UIScrollView.h"
#include <set>
#include <vector>
#include <tuple>

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

    class TableView : public cocos2d::ui::ScrollView {
        DECLARE_CLASS_GUI_INFO

    public:
        enum class VerticalFillOrder {
            TOP_DOWN,
            BOTTOM_UP
        };

        typedef std::function<void (TableView *table, TableViewCell *cell)> ccTableViewCellWillRecycleCallback;

        enum class CallbackType {
            CELL_SIZE,  // param for pointer to CellSizeParam
            CELL_AT_INDEX,  // param for pointer to idx, returns TableViewCell*
            NUMBER_OF_CELLS, // param for pointer to idx return size_t
        };

        struct CellSizeParam {
            ssize_t idx;
            cocos2d::Size size;
        };
        typedef std::function<intptr_t (TableView *table, CallbackType, intptr_t param)> ccTableViewCallback;

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

        void setTableViewCellWillRecycleCallback(const ccTableViewCellWillRecycleCallback &callback);
        void setTableViewCallback(const ccTableViewCallback &callback);

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
        void inplaceReloadData();

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

    protected:
        virtual void onSizeChanged() override;

        virtual void moveInnerContainer(const cocos2d::Vec2 &deltaMove, bool canStartBounceBack) override;

        long __indexFromOffset(const cocos2d::Vec2 &offset, ssize_t cellsCount);
        long _indexFromOffset(cocos2d::Vec2 offset, ssize_t cellsCount);
        cocos2d::Vec2 __offsetFromIndex(ssize_t index);
        cocos2d::Vec2 _offsetFromIndex(ssize_t index);

        void _moveCellOutOfSight(TableViewCell *cell);
        void _setIndexForCell(ssize_t index, TableViewCell *cell);
        void _addCellIfNecessary(TableViewCell *cell);

        void _updateCellPositions(ssize_t cellsCount);
        void _scrollViewDidScroll(ssize_t cellsCount);

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

        ccTableViewCellWillRecycleCallback _tableViewCellWillRecycleCallback;
        ccTableViewCallback _tableViewCallback;

        cocos2d::ui::ScrollView::Direction _oldDirection;

        bool _isUsedCellsDirty;

    //public:
        void _updateContentSize(ssize_t cellsCount);

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
