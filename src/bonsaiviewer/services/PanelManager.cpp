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

#include <QDockWidget>
#include <QMainWindow>
#include <QMenu>

#include <algorithm>

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

void PanelManager::buildWindowMenu(QMenu* menu) {
    if (!menu) return;
    menu->clear();
    for (const auto& [id, panel] : panels_) {
        QAction* action = menu->addAction(panel.title);
        action->setCheckable(true);
        action->setChecked(isPanelVisible(id));
        QObject::connect(action, &QAction::triggered, menu,
                         [this, id]() { togglePanel(id); });
    }
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
