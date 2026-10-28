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

#include "PanelManager.h"

#include <QAction>
#include <QDockWidget>
#include <QMainWindow>
#include <QMenu>

#include <algorithm>
#include <functional>

namespace bonsaiviewer::services {

PanelManager::PanelManager(QMainWindow* main_window, QObject* parent)
    : QObject(parent), main_window_(main_window) {}

void PanelManager::registerPanel(const QString& id, const QString& title,
                                 PanelFactory factory, Qt::DockWidgetArea area) {
    Panel panel;
    panel.id = id;
    panel.title = title;
    panel.factory = std::move(factory);
    panel.area = area;
    panels_[id] = std::move(panel);
    emit panelRegistered(id);
}

void PanelManager::adoptPanel(const QString& id, const QString& title, QDockWidget* dock,
                              const QString& group) {
    if (!dock) return;
    Panel panel;
    panel.id = id;
    panel.title = title;
    panel.group = group;
    panel.dock = dock;
    panels_[id] = std::move(panel);
    emit panelRegistered(id);
}

QStringList PanelManager::registeredPanels() const {
    QStringList ids;
    for (const auto& [id, panel] : panels_) {
        static_cast<void>(panel);
        ids.append(id);
    }
    return ids;
}

QString PanelManager::panelTitle(const QString& id) const {
    const auto it = panels_.find(id);
    return it != panels_.end() ? it->second.title : QString();
}

void PanelManager::setPanelVisible(const QString& id, bool visible) {
    const auto it = panels_.find(id);
    if (it == panels_.end()) return;
    Panel& panel = it->second;
    if (!panel.dock) {
        if (!visible) return;
        QWidget* content = panel.factory ? panel.factory() : nullptr;
        if (!content) return;
        panel.dock = new QDockWidget(panel.title, main_window_);
        panel.dock->setObjectName(QStringLiteral("dock-") + id);
        panel.dock->setWidget(content);
        content->setParent(panel.dock);
        main_window_->addDockWidget(panel.area, panel.dock);
    }
    panel.dock->setVisible(visible);
}

bool PanelManager::isPanelVisible(const QString& id) const {
    const auto it = panels_.find(id);
    return it != panels_.end() && it->second.dock && !it->second.dock->isHidden();
}

void PanelManager::togglePanel(const QString& id) {
    setPanelVisible(id, !isPanelVisible(id));
}

namespace {

// The Window submenus, in menu order (BonsaiViewer01).
const QStringList kWindowGroups{QStringLiteral("Trees"), QStringLiteral("Properties"),
                                QStringLiteral("Analyze")};

// Refreshes the check marks of every panel action, submenus included.
void refreshChecks(QMenu* menu, const std::function<bool(const QString&)>& visible) {
    for (QAction* action : menu->actions()) {
        if (action->menu()) {
            refreshChecks(action->menu(), visible);
            continue;
        }
        const QString id = action->data().toString();
        if (!id.isEmpty()) action->setChecked(visible(id));
    }
}

} // namespace

void PanelManager::buildWindowMenu(QMenu* menu) {
    if (!menu) return;
    menu->clear();
    // Adds the panels of `group` (empty: ungrouped) to `target`, sorted by title.
    auto addPanels = [this, menu](QMenu* target, const QString& group) {
        QList<QPair<QString, QString>> items;  // (title, id)
        for (const auto& [id, panel] : panels_) {
            if (panel.group == group) items.append({panel.title, id});
        }
        std::sort(items.begin(), items.end());
        for (const auto& [title, id] : items) {
            QAction* action = target->addAction(title);
            action->setCheckable(true);
            action->setData(id);
            action->setChecked(isPanelVisible(id));
            // The check mark is the wanted state: it follows the click, not a toggle
            // of a state that may have changed since the menu was built.
            QObject::connect(action, &QAction::triggered, menu,
                             [this, id](bool checked) { setPanelVisible(id, checked); });
        }
        return items.size();
    };
    for (const QString& group : kWindowGroups) {
        addPanels(menu->addMenu(group), group);
    }
    const bool has_ungrouped = std::any_of(panels_.begin(), panels_.end(),
                                           [](const auto& entry) { return entry.second.group.isEmpty(); });
    if (has_ungrouped) {
        menu->addSeparator();
        addPanels(menu, QString());
    }
    // Panels are also shown / hidden elsewhere (workspaces, Panels ribbon,
    // close button): the check marks are refreshed each time the menu opens.
    QObject::disconnect(menu, &QMenu::aboutToShow, this, nullptr);
    QObject::connect(menu, &QMenu::aboutToShow, this, [this, menu]() {
        refreshChecks(menu, [this](const QString& id) { return isPanelVisible(id); });
    });
}

QString PanelManager::addTab(const QString& name) {
    const QString id = QStringLiteral("tab-%1").arg(next_tab_number_++);
    Tab tab;
    tab.id = id;
    tab.name = name;
    tabs_[id] = tab;
    emit tabsChanged();
    return id;
}

void PanelManager::renameTab(const QString& tab_id, const QString& name) {
    const auto it = tabs_.find(tab_id);
    if (it == tabs_.end()) return;
    it->second.name = name;
    emit tabsChanged();
}

void PanelManager::removeTab(const QString& tab_id) {
    if (tabs_.find(tab_id) == tabs_.end()) return;
    // Never leave the session without a tab: the last one stays.
    if (tabs_.size() == 1) return;
    const bool was_current = tab_id == current_tab_;
    tabs_.erase(tab_id);
    if (was_current) {
        current_tab_.clear();
        switchTab(tabs_.begin()->first);
    }
    emit tabsChanged();
}

QStringList PanelManager::tabIds() const {
    QStringList ids;
    for (const auto& [id, tab] : tabs_) ids.append(id);
    return ids;
}

QString PanelManager::tabName(const QString& tab_id) const {
    const auto it = tabs_.find(tab_id);
    return it != tabs_.end() ? it->second.name : QString();
}

void PanelManager::switchTab(const QString& tab_id) {
    if (tabs_.find(tab_id) == tabs_.end()) return;
    if (tab_id == current_tab_) return;
    saveCurrentTabState();
    current_tab_ = tab_id;
    const Tab& tab = tabs_[tab_id];
    if (tab.has_layout) {
        main_window_->restoreState(tab.layout);
    } else {
        // A fresh tab starts with only the central 3D view (TI21).
        hideAllPanels();
    }
    emit currentTabChanged(tab_id);
}

QByteArray PanelManager::tabLayout(const QString& tab_id) const {
    const auto it = tabs_.find(tab_id);
    return it != tabs_.end() && it->second.has_layout ? it->second.layout : QByteArray();
}

void PanelManager::setTabLayout(const QString& tab_id, const QByteArray& layout) {
    const auto it = tabs_.find(tab_id);
    if (it == tabs_.end() || layout.isEmpty()) return;
    it->second.layout = layout;
    it->second.has_layout = true;
}

void PanelManager::snapshotCurrentTab() {
    saveCurrentTabState();
}

void PanelManager::reloadCurrentTab() {
    const auto it = tabs_.find(current_tab_);
    if (it != tabs_.end() && it->second.has_layout) main_window_->restoreState(it->second.layout);
}

void PanelManager::saveCurrentTabState() {
    if (current_tab_.isEmpty()) return;
    const auto it = tabs_.find(current_tab_);
    if (it == tabs_.end()) return;
    it->second.layout = main_window_->saveState();
    it->second.has_layout = true;
}

void PanelManager::hideAllPanels() {
    for (auto& [id, panel] : panels_) {
        if (panel.dock) panel.dock->hide();
    }
}

} // namespace bonsaiviewer::services
