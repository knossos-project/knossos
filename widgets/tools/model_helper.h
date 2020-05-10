/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2018
 *  Max-Planck-Gesellschaft zur Foerderung der Wissenschaften e.V.
 *
 *  KNOSSOS is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 of
 *  the License as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 *  For further information, visit https://knossos.app
 *  or contact knossosteam@gmail.com
 */

#pragma once

#include <QItemSelection>

template<typename Elem>
Elem & getElem(Elem & elem) {
    return elem;
}
template<typename Elem>
Elem & getElem(Elem * elem) {
    return *elem;
}
template<typename Elem>
Elem & getElem(const std::reference_wrapper<Elem> & elem) {
    return elem.get();
}
template<typename M, typename D, typename S>
auto deltaBlockSelection(const M & model, const D dataFromIndex, const S isAlreadySelected, const bool inverter = false){
    QItemSelection selectedItems;

    bool blockSelection{false};
    int blockStartIndex{0};

    for (int rowIndex{0}; rowIndex < model.rowCount(); ++rowIndex) {
        const auto & elem = dataFromIndex(rowIndex);
        const auto alreadySelected = isAlreadySelected(rowIndex) ? !inverter : inverter;
        const auto selected = getElem(elem).selected ? !inverter : inverter;
        if (!blockSelection && selected && !alreadySelected) {// start block selection
            blockSelection = true;
            blockStartIndex = rowIndex;
        }
        if (blockSelection && (!selected || alreadySelected)) {// end block selection
            selectedItems.select(model.index(blockStartIndex, 0), model.index(rowIndex-1, model.columnCount()-1));
            blockSelection = false;
        }
    }
    // finish last blockselection – if any
    if (blockSelection) {
        selectedItems.select(model.index(blockStartIndex, 0), model.index(model.rowCount()-1, model.columnCount()-1));
    }

    return selectedItems;
}
template<typename M, typename D>
auto deltaBlockSelection(const M & model, const D dataFromIndex){
    return deltaBlockSelection(model, dataFromIndex, [](int){return false;});
}
inline auto blockSelection = [](const auto & model, const auto & data){
    return deltaBlockSelection(model, [&data](auto index){return data[index];}, [](int){return false;});
};

inline auto threeWaySorting = [](auto & table, auto & sortIndex){// emulate ability for the user to disable sorting
    return [&table, &sortIndex](const int index){
        if (index == sortIndex && table.header()->sortIndicatorOrder() == Qt::SortOrder::AscendingOrder) {// asc (-1) → desc (==) → asc (==)
            table.sortByColumn(sortIndex = -1, Qt::SortOrder::AscendingOrder);
        } else {
            sortIndex = index;
        }
    };
};
