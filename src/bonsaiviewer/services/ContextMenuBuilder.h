// This file was generated with the assistance of an AI coding tool.
/********************************************************************************
 *                                                                              *
 * This file is part of Bonsai.                                                 *
 *                                                                              *
 * Bonsai is free software: you can redistribute it and/or modify               *
 * it under the terms of the GNU General Public License as published by         *
 * the Free Software Foundation, either version 3.0 of the License, or          *
 * (at your option) any later version.                                           *
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

#ifndef IFCINTERFACE_SERVICES_CONTEXTMENUBUILDER_H
#define IFCINTERFACE_SERVICES_CONTEXTMENUBUILDER_H

#include <QString>

#include <functional>
#include <vector>

class QMenu;
class QPoint;
class QWidget;

// B15 — ContextMenuBuilder (see 00_Technical_Implementation/
// BonsaiViewerTI00). Declarative context menus: describe the items with
// their enablement / visibility predicates and their trigger callback,
// then build / exec. Enablement and visibility are evaluated at build
// time, so the same builder can be re-populated per invocation. Consumers:
// properties rows (TI14), measurements (TI11), viewport objects (TI20).

namespace bonsaiviewer::services {

class ContextMenuBuilder {
public:
    struct Item {
        QString id;
        QString label;
        // Optional predicates; absent means always enabled / visible.
        std::function<bool()> is_enabled;
        std::function<bool()> is_visible;
        std::function<void()> trigger;
    };

    ContextMenuBuilder& addItem(Item item);
    // Positional form, for C++17 call sites (MSVC builds the viewer in C++17,
    // which has no designated initializers).
    ContextMenuBuilder& addItem(const QString& id,
                                const QString& label,
                                std::function<void()> trigger,
                                std::function<bool()> is_enabled = {},
                                std::function<bool()> is_visible = {});
    ContextMenuBuilder& addSeparator();

    // Evaluate the predicates and build the QMenu. Ownership passes to the
    // caller (exec deletes it on close).
    QMenu* build(QWidget* parent = nullptr) const;

    // Build and exec at `global_pos`. Returns when the menu closes.
    void exec(const QPoint& global_pos, QWidget* parent = nullptr) const;

private:
    struct Entry {
        bool separator = false;
        Item item;
    };
    std::vector<Entry> entries_;
};

} // namespace bonsaiviewer::services

#endif
