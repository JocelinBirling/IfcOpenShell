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

#include "ContextMenuBuilder.h"

#include <QAction>
#include <QMenu>

namespace bonsaiviewer::services {

ContextMenuBuilder& ContextMenuBuilder::addItem(Item item) {
    entries_.push_back(Entry{false, std::move(item)});
    return *this;
}

ContextMenuBuilder& ContextMenuBuilder::addSeparator() {
    entries_.push_back(Entry{true, {}});
    return *this;
}

QMenu* ContextMenuBuilder::build(QWidget* parent) const {
    auto* menu = new QMenu(parent);
    for (const Entry& entry : entries_) {
        if (entry.separator) {
            // A separator only makes sense between visible items; a menu
            // never ends on a dangling separator.
            if (!menu->actions().empty()) {
                menu->addSeparator();
            }
            continue;
        }
        const Item& item = entry.item;
        if (item.is_visible && !item.is_visible()) continue;
        QAction* action = menu->addAction(item.label);
        action->setEnabled(!item.is_enabled || item.is_enabled());
        if (item.trigger) {
            QObject::connect(action, &QAction::triggered, menu,
                             [trigger = item.trigger]() { trigger(); });
        }
    }
    return menu;
}

void ContextMenuBuilder::exec(const QPoint& global_pos, QWidget* parent) const {
    QMenu* menu = build(parent);
    menu->setAttribute(Qt::WA_DeleteOnClose);
    menu->exec(global_pos);
}

} // namespace bonsaiviewer::services
