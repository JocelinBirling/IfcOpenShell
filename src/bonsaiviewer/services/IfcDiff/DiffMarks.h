// This file was generated with the assistance of an AI coding tool.
/********************************************************************************
 *                                                                              *
 * This file is part of Bonsai.                                                 *
 *                                                                              *
 * Bonsai is free software: you can redistribute it and/or modify               *
 * it under the terms of the GNU General Public License as published by         *
 * the Free Software Foundation, either version 3.0 of the License, or          *
 * (at your option) any later version.                                          *
 *                                                                              *
 * Bonsai is distributed in the hope that it will be useful,                    *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of               *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                 *
 * GNU General Public License for more details.                                 *
 *                                                                              *
 * You should have received a copy of the GNU General Public License            *
 * along with this program. If not, see <http://www.gnu.org/licenses/>.         *
 *                                                                              *
 ********************************************************************************/

#ifndef BONSAIVIEWER_SERVICES_IFCDIFF_DIFFMARKS_H
#define BONSAIVIEWER_SERVICES_IFCDIFF_DIFFMARKS_H

#include "IfcDiff.h"

#include <QObject>

#include <cstdint>
#include <map>
#include <utility>

// BonsaiViewer34 — the property and attribute changes of the last diff run,
// element by element, for the Properties panel (BonsaiViewer14) to colour
// its rows: green added, red removed, orange changed (old -> new tooltip).
// An element of a diffed model is keyed by its model and EXPRESS id; both the
// Before and the After element of a changed GlobalId point to its change.

namespace bonsaiviewer::services::ifcdiff {

class DiffMarks : public QObject {
    Q_OBJECT

public:
    using Key = std::pair<std::uint32_t, int>; // (session model id, EXPRESS id)
    struct Mark {
        const ElementChange* change = nullptr;
        bool before = false; // the element is the Before one
    };

    explicit DiffMarks(QObject* parent = nullptr) : QObject(parent) {}

    void setChanges(std::map<Key, Mark> changes) {
        changes_ = std::move(changes);
        emit changed();
    }
    void clear() {
        if (changes_.empty()) return;
        changes_.clear();
        emit changed();
    }
    // Null when the element has no property or attribute change.
    const Mark* markOf(std::uint32_t session_model_id, int ifc_id) const {
        const auto found = changes_.find({session_model_id, ifc_id});
        return found == changes_.end() ? nullptr : &found->second;
    }

signals:
    void changed();

private:
    std::map<Key, Mark> changes_;
};

} // namespace bonsaiviewer::services::ifcdiff

#endif // BONSAIVIEWER_SERVICES_IFCDIFF_DIFFMARKS_H
