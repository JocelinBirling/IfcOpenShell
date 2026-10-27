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

#ifndef IFCINTERFACE_SERVICES_PANELMANAGER_H
#define IFCINTERFACE_SERVICES_PANELMANAGER_H

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QStringList>

#include <functional>
#include <map>

class QMainWindow;
class QMenu;
class QDockWidget;
class QWidget;

// B13 — PanelManager (see 00_Technical_Implementation/
// BonsaiViewerTI00). Registry of panel factories, per-tab dock layout,
// add / remove panels at runtime, the Window menu of TI01, and the user
// tabs of TI21. Panels register with a stable id and a factory; the dock
// widget is created lazily on first show. Each tab keeps its own dock
// state (QMainWindow::saveState per tab — the lighter path of TI21); a
// tab without a saved state starts with only the central 3D view.

namespace bonsaiviewer::services {

class PanelManager : public QObject {
    Q_OBJECT

public:
    using PanelFactory = std::function<QWidget*()>;

    explicit PanelManager(QMainWindow* main_window, QObject* parent = nullptr);

    // The dock widget is created on first show, docked in `area`.
    // `title` is the Window menu label (TI01).
    void registerPanel(const QString& id, const QString& title,
                       PanelFactory factory,
                       Qt::DockWidgetArea area = Qt::RightDockWidgetArea);
    // Register a dock widget the shell already created (the hand-rolled
    // panels of MainWindow): same Window menu / visibility treatment,
    // no lazy factory.
    void adoptPanel(const QString& id, const QString& title, QDockWidget* dock);
    QStringList registeredPanels() const;
    QString panelTitle(const QString& id) const;

    // Visibility in the current tab; true when created and not hidden.
    void setPanelVisible(const QString& id, bool visible);
    bool isPanelVisible(const QString& id) const;
    void togglePanel(const QString& id);

    // Rebuild `menu` with one checkable action per registered panel
    // (TI01). Call again on panelRegistered.
    void buildWindowMenu(QMenu* menu);

    // Tabs. System tabs are declared by the shell at startup; the "+"
    // tab of TI21 calls addTab. removeTab drops the stored layout; the
    // delete confirmation (B17) is the caller's job.
    QString addTab(const QString& name);
    void renameTab(const QString& tab_id, const QString& name);
    void removeTab(const QString& tab_id);
    QStringList tabIds() const;
    QString tabName(const QString& tab_id) const;
    void switchTab(const QString& tab_id);
    QString currentTab() const { return current_tab_; }

    // TI21 phase 2 (Save Layouts): the dock layout stored for a tab, empty
    // when the tab has none yet. snapshotCurrentTab stores the live layout
    // of the current tab first; reloadCurrentTab applies its stored layout.
    QByteArray tabLayout(const QString& tab_id) const;
    void setTabLayout(const QString& tab_id, const QByteArray& layout);
    void snapshotCurrentTab();
    void reloadCurrentTab();

signals:
    void panelRegistered(const QString& id);
    void tabsChanged();
    void currentTabChanged(const QString& tab_id);

private:
    void saveCurrentTabState();
    void hideAllPanels();

    struct Panel {
        QString id;
        QString title;
        PanelFactory factory;
        Qt::DockWidgetArea area = Qt::RightDockWidgetArea;
        QDockWidget* dock = nullptr;
    };

    struct Tab {
        QString id;
        QString name;
        QByteArray layout;
        bool has_layout = false;
    };

    QMainWindow* main_window_ = nullptr;
    std::map<QString, Panel> panels_;
    std::map<QString, Tab> tabs_;
    int next_tab_number_ = 1;
    QString current_tab_;
};

} // namespace bonsaiviewer::services

#endif
