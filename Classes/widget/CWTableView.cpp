#include "CWTableView.h"

using namespace cocos2d;
using namespace cocos2d::ui;

namespace cw {
    TableView::TableView()
    : _vordering(VerticalFillOrder::BOTTOM_UP)
    , _isUsedCellsDirty(false)
    , _delegate(nullptr) {

    }

    TableView::~TableView() {
    }

    TableView *TableView::create() {
        TableView *table = new (std::nothrow) TableView();
        if (table != nullptr && table->init()) {
            table->autorelease();
            return table;
        }
        CC_SAFE_DELETE(table);
        return NULL;
    }

    bool TableView::init() {
        if (!ScrollView::init()) {
            return false;
        }
        _cellsPositions.clear();

        _cellsUsed.clear();
        _cellsFreed.clear();
        _indices.clear();
        _vordering = VerticalFillOrder::BOTTOM_UP;
        this->setDirection(Direction::VERTICAL);

        return true;
    }

    void TableView::onSizeChanged() {
        Size orginSize = _innerContainer->getContentSize();
        Vec2 orginPos = _innerContainer->getPosition();
        ScrollView::onSizeChanged();
        if (orginSize.equals(_innerContainer->getContentSize())) {
            _innerContainer->setPosition(orginPos);
        }
    }

    void TableView::setVerticalFillOrder(VerticalFillOrder fillOrder) {
        if (_vordering != fillOrder) {
            _vordering = fillOrder;
            if (!_cellsUsed.empty()) {
                this->reloadData();
            }
        }
    }

    TableView::VerticalFillOrder TableView::getVerticalFillOrder() {
        return _vordering;
    }

    void TableView::setDirection(Direction dir) {
        switch (_direction) {
        case Direction::NONE:
        case Direction::HORIZONTAL:
        case Direction::VERTICAL:
            ui::ScrollView::setDirection(dir);
            break;
        case Direction::BOTH:
            CCAssert(0, "TableView doesn't support both diretion!");
            break;
        default:
            CCAssert(0, "Unknown diretion!");
            break;
        }
    }

    void TableView::reloadData() {
        for (const auto &cell : _cellsUsed) {
            _delegate->tableCellWillRecycle(this, cell);
            _cellsFreed.pushBack(cell);

            cell->reset();
            if (cell->getParent() == _innerContainer) {
                _innerContainer->removeChild(cell, true);
            }
        }

        _indices.clear();
        _cellsUsed.clear();

        ssize_t cellsCount = _delegate->numberOfCellsInTableView(this);
        this->_updateCellPositions(cellsCount);
        this->_updateInnerContainerSize(cellsCount);
        if (cellsCount > 0) {
            _scrollViewDidScroll(cellsCount);
            this->processScrollingEvent();
        }
    }

    void TableView::reloadDataInplacement() {
        Vec2 pos = getInnerContainerPosition();
        const Vec2 offset1 = minContainerOffset();

        for (const auto &cell : _cellsUsed) {
            _delegate->tableCellWillRecycle(this, cell);
            _cellsFreed.pushBack(cell);

            cell->reset();
            if (cell->getParent() == _innerContainer) {
                _innerContainer->removeChild(cell, true);
            }
        }

        _indices.clear();
        _cellsUsed.clear();

        ssize_t cellsCount = _delegate->numberOfCellsInTableView(this);
        this->_updateCellPositions(cellsCount);
        this->_updateInnerContainerSize(cellsCount);

        const Vec2 offset2 = minContainerOffset();
        if (_direction == Direction::HORIZONTAL) {
            pos.x = std::max(pos.x, offset2.x);
            setInnerContainerPosition(pos);
        }
        else {
            if (_vordering == VerticalFillOrder::BOTTOM_UP) {
                pos.y = std::max(pos.y, offset2.y);
                setInnerContainerPosition(pos);
            }
            else {
                float temp = offset1.y - pos.y;
                pos.y = offset2.y - temp;
                pos.y = std::min(pos.y, 0.0f);
                setInnerContainerPosition(pos);
            }
        }

        if (cellsCount > 0) {
            _scrollViewDidScroll(cellsCount);
            this->processScrollingEvent();
        }
    }

    TableViewCell *TableView::cellAtIndex(ssize_t idx) {
        if (_indices.find(idx) != _indices.end()) {
            for (const auto &cell : _cellsUsed) {
                if (cell->getIdx() == idx) {
                    return cell;
                }
            }
        }

        return nullptr;
    }

    void TableView::updateCellAtIndex(ssize_t idx) {
        if (idx == CC_INVALID_INDEX) {
            return;
        }
        ssize_t countOfItems = _delegate->numberOfCellsInTableView(this);
        if (0 == countOfItems || idx > countOfItems-1) {
            return;
        }

        TableViewCell *cell = this->cellAtIndex(idx);
        if (cell != nullptr) {
            this->_moveCellOutOfSight(cell);
        }
        cell = _delegate->tableCellAtIndex(this, idx);
        this->_setIndexForCell(idx, cell);
        this->_addCellIfNecessary(cell);
    }

    void TableView::insertCellAtIndex(ssize_t idx) {
        if (idx == CC_INVALID_INDEX) {
            return;
        }

        ssize_t countOfItems = _delegate->numberOfCellsInTableView(this);
        if (0 == countOfItems || idx > countOfItems-1) {
            return;
        }

        long newIdx = 0;

        auto cell = cellAtIndex(idx);
        if (cell != nullptr) {
            newIdx = _cellsUsed.getIndex(cell);
            // Move all cells behind the inserted position
            for (ssize_t i = newIdx; i < _cellsUsed.size(); ++i) {
                cell = _cellsUsed.at(i);
                this->_setIndexForCell(cell->getIdx()+1, cell);
            }
        }

        //insert a new cell
        cell = _delegate->tableCellAtIndex(this, idx);
        this->_setIndexForCell(idx, cell);
        this->_addCellIfNecessary(cell);

        this->_updateCellPositions(countOfItems);
        this->_updateInnerContainerSize(countOfItems);

        if (countOfItems > 0) {
            _scrollViewDidScroll(countOfItems);
            this->processScrollingEvent();
        }
    }

    void TableView::removeCellAtIndex(ssize_t idx) {
        if (idx == CC_INVALID_INDEX) {
            return;
        }

        ssize_t countOfItems = _delegate->numberOfCellsInTableView(this);
        if (0 == countOfItems || idx + 1 > countOfItems) {
            return;
        }

        TableViewCell* cell = this->cellAtIndex(idx);
        if (cell == nullptr) {
            return;
        }

        ssize_t newIdx = _cellsUsed.getIndex(cell);

        //remove first
        this->_moveCellOutOfSight(cell);

        _indices.erase(idx);
        this->_updateCellPositions(countOfItems);
        this->_updateInnerContainerSize(countOfItems);

        for (ssize_t i = _cellsUsed.size() - 1; i > newIdx; --i) {
            cell = _cellsUsed.at(i);
            this->_setIndexForCell(cell->getIdx() - 1, cell);
        }

        if (countOfItems > 0) {
            _scrollViewDidScroll(countOfItems);
            this->processScrollingEvent();
        }
    }

    TableViewCell *TableView::dequeueCell() {
        TableViewCell *cell;
        if (_cellsFreed.empty()) {
            cell = nullptr;
        }
        else {
            cell = _cellsFreed.back();
            cell->retain();
            _cellsFreed.popBack();
            cell->autorelease();
        }
        return cell;
    }

    void TableView::_addCellIfNecessary(TableViewCell *cell) {
        if (cell->getParent() != _innerContainer) {
            _innerContainer->addChild(cell);
        }
        _cellsUsed.pushBack(cell);
        _indices.insert(cell->getIdx());
        _isUsedCellsDirty = true;
    }

    void TableView::_updateInnerContainerSize(ssize_t cellsCount) {
        Size size = Size::ZERO;
        if (cellsCount > 0) {
            float maxPosition = _cellsPositions[cellsCount];
            size = _contentSize;
            (_direction == Direction::HORIZONTAL ? size.width : size.height) = maxPosition;
        }

        this->setInnerContainerSize(size);
    }

    void TableView::updateInnerContainerSize() {
        ssize_t cellsCount = _delegate->numberOfCellsInTableView(this);
        this->_updateCellPositions(cellsCount);
    }

    Vec2 TableView::_offsetFromIndex(ssize_t index) {
        Vec2 offset = this->__offsetFromIndex(index);

        if (_vordering == VerticalFillOrder::TOP_DOWN) {
            Size cellSize = _delegate->tableCellSizeForIndex(this, index);
            offset.y = _innerContainer->getContentSize().height - offset.y - cellSize.height;
        }
        return offset;
    }

    Vec2 TableView::__offsetFromIndex(ssize_t index) {
        Vec2 offset;
        (_direction == Direction::HORIZONTAL ? offset.x : offset.y) = _cellsPositions[index];
        return offset;
    }

    long TableView::_indexFromOffset(Vec2 offset, ssize_t cellsCount) {
        long index = 0;
        const long maxIdx = cellsCount - 1;

        if (_vordering == VerticalFillOrder::TOP_DOWN) {
            offset.y = _innerContainer->getContentSize().height - offset.y;
        }
        index = this->__indexFromOffset(offset, cellsCount);
        if (index != -1) {
            index = std::max(0L, index);
            if (index > maxIdx) {
                index = CC_INVALID_INDEX;
            }
        }

        return index;
    }

    long TableView::__indexFromOffset(const Vec2 &offset, ssize_t cellsCount) {
        long low = 0;
        long high = cellsCount - 1;
        const float search = (_direction == Direction::HORIZONTAL ? offset.x : offset.y);

        // binary search
        while (high >= low) {
            long index = low + (high - low) / 2;
            const float cellStart = _cellsPositions[index];
            const float cellEnd = _cellsPositions[index + 1];

            if (search >= cellStart && search <= cellEnd) {
                return index;
            }
            else if (search < cellStart) {
                high = index - 1;
            }
            else {
                low = index + 1;
            }
        }
        return (low <= 0) ? 0 : -1;
    }

    void TableView::_moveCellOutOfSight(TableViewCell *cell) {
        _delegate->tableCellWillRecycle(this, cell);
        _cellsFreed.pushBack(cell);
        _cellsUsed.eraseObject(cell);
        _isUsedCellsDirty = true;

        _indices.erase(cell->getIdx());
        cell->reset();

        if (cell->getParent() == _innerContainer) {
            _innerContainer->removeChild(cell, true);
        }
    }

    void TableView::_setIndexForCell(ssize_t index, TableViewCell *cell) {
        cell->setAnchorPoint(Vec2(0.0f, 0.0f));
        cell->setPosition(this->_offsetFromIndex(index));
        cell->setIdx(index);
    }

    void TableView::_updateCellPositions(ssize_t cellsCount) {
        _cellsPositions.resize(cellsCount + 1, 0.0f);

        if (cellsCount > 0) {
            float currentPos = 0;
            Size cellSize;
            for (ssize_t i = 0; i < cellsCount; ++i) {
                _cellsPositions[i] = currentPos;
                cellSize = _delegate->tableCellSizeForIndex(this, i);
                currentPos += (_direction == Direction::HORIZONTAL ? cellSize.width : cellSize.height);
            }
            _cellsPositions[cellsCount] = currentPos;//1 extra value allows us to get right/bottom of the last cell
        }
    }

    Vec2 TableView::maxContainerOffset() {
        return Vec2(0, 0);
    }

    Vec2 TableView::minContainerOffset() {
        const Size &contentSize = _innerContainer->getContentSize();
        const Size &viewSize = _contentSize;
        return Vec2(viewSize.width - contentSize.width * _innerContainer->getScaleX(),
                    viewSize.height - contentSize.height * _innerContainer->getScaleY());
    }

    void TableView::_scrollViewDidScroll(ssize_t cellsCount) {
        if (0 == cellsCount) {
            return;
        }

        if (_isUsedCellsDirty) {
            _isUsedCellsDirty = false;
            std::sort(_cellsUsed.begin(), _cellsUsed.end(), [](TableViewCell *a, TableViewCell *b) -> bool{
                return a->getIdx() < b->getIdx();
            });
        }

        ssize_t startIdx = 0, endIdx = 0, idx = 0, maxIdx = 0;
        Vec2 offset = _innerContainer->getPosition();
        Vec2 maxOffset = maxContainerOffset();
        Vec2 minOffset = minContainerOffset();
        offset.x = std::min(maxOffset.x, offset.x);
        offset.x = std::max(minOffset.x, offset.x);
        offset.y = std::min(maxOffset.y, offset.y);
        offset.y = std::max(minOffset.y, offset.y);

        offset.x = -offset.x;
        offset.y = -offset.y;
        maxIdx = std::max((long)cellsCount - 1, 0L);

        if (_vordering == VerticalFillOrder::TOP_DOWN) {
            offset.y = offset.y + _contentSize.height / _innerContainer->getScaleY();
        }
        startIdx = this->_indexFromOffset(offset, cellsCount);
        if (startIdx == CC_INVALID_INDEX) {
            startIdx = cellsCount - 1;
        }

        if (_vordering == VerticalFillOrder::TOP_DOWN) {
            offset.y -= _contentSize.height / _innerContainer->getScaleY();
        }
        else {
            offset.y += _contentSize.height / _innerContainer->getScaleY();
        }
        offset.x += _contentSize.width / _innerContainer->getScaleX();

        endIdx = this->_indexFromOffset(offset, cellsCount);
        if (endIdx == CC_INVALID_INDEX) {
            endIdx = cellsCount - 1;
        }

#if 0 // For Testing.
        int i = 0;
        std::for_each(_cellsUsed.begin(), _cellsUsed.end(), [&i](TableViewCell* pCell) {
            log("cells Used index %d, value = %lu", i, pCell->getIdx());
            ++i;
        });
        log("---------------------------------------");
        i = 0;
        std::for_each(_cellsFreed.begin(), _cellsFreed.end(), [&i](TableViewCell* pCell) {
            log("cells freed index %d, value = %lu", i, pCell->getIdx());
            ++i;
        });
        log("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
#endif

        if (!_cellsUsed.empty()) {
            auto cell = _cellsUsed.at(0);
            idx = cell->getIdx();

            while (idx < startIdx) {
                this->_moveCellOutOfSight(cell);
                CC_BREAK_IF(_cellsUsed.empty());
                cell = _cellsUsed.at(0);
                idx = cell->getIdx();
            }
        }
        if (!_cellsUsed.empty()) {
            auto cell = _cellsUsed.back();
            idx = cell->getIdx();

            while (idx <= maxIdx && idx > endIdx) {
                this->_moveCellOutOfSight(cell);
                CC_BREAK_IF(_cellsUsed.empty());
                cell = _cellsUsed.back();
                idx = cell->getIdx();
            }
        }

        for (ssize_t i = startIdx; i <= endIdx; ++i) {
            if (_indices.find(i) == _indices.end()) {
                this->updateCellAtIndex(i);
            }
        }
    }

    void TableView::moveInnerContainer(const Vec2& deltaMove, bool canStartBounceBack) {
        ScrollView::moveInnerContainer(deltaMove, canStartBounceBack);
        ssize_t cellsCount = _delegate->numberOfCellsInTableView(this);
        _scrollViewDidScroll(cellsCount);
    }

    cocos2d::Vec2 TableView::_destinationFromIndex(ssize_t index) {
        Vec2 offset = __offsetFromIndex(index);
        if (_vordering == VerticalFillOrder::TOP_DOWN) {
            offset.y = _innerContainer->getContentSize().height - offset.y - _contentSize.height / _innerContainer->getScaleY();
        }

        Vec2 temp = maxContainerOffset();
        if (offset < temp) {
            offset = temp;
        }

        return -offset;
    }

    void TableView::jumpToCell(ssize_t cellIndex) {
        Vec2 des = _destinationFromIndex(cellIndex);
        this->jumpToDestination(des);
    }

    void TableView::scrollToCell(ssize_t cellIndex, float timeInSec, bool attenuated) {
        Vec2 des = _destinationFromIndex(cellIndex);
        this->startAutoScrollToDestination(des, timeInSec, attenuated);
    }
}
