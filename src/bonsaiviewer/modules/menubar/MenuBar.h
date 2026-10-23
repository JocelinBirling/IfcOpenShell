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

#ifndef IFCINTERFACE_MODULES_MENUBAR_H
#define IFCINTERFACE_MODULES_MENUBAR_H

#include <QObject>

#include <functional>

class QMainWindow;
class QMenu;
class ViewportWindow;

namespace bonsaiviewer {
class SessionState;
namespace services {
class CategoryOverrideFilter;
class PanelManager;
}
}

// TI01 — Menu Bar (see 00_Technical_Implementation/BonsaiViewerTI01).
// Four QMenus on the main window's menu bar, wiring existing code:
// File reuses the project commands and the settings dialog, Display
// binds the four category toggles to B03 (IfcDrawings disabled until
// TI19 lands), Window is generated from B13, Help opens the
// documentation and shows the About popup (B17). File operations are
// the ribbon PROJECT group's commands — no new logic here, only wiring.

namespace bonsaiviewer::modules::menubar {

class MenuBar : public QObject {
    Q_OBJECT

public:
    explicit MenuBar(QMainWindow* window,
                     bonsaiviewer::SessionState* session_state,
                     ViewportWindow* viewport,
                     bonsaiviewer::services::CategoryOverrideFilter* category_filter,
                     bonsaiviewer::services::PanelManager* panel_manager,
                     QObject* parent = nullptr);

    // Fills the File > Open Recent submenu (repulated on each show).
    // The shell owns the recent-projects logic.
    void setOpenRecentPopulator(std::function<void(QMenu*)> populator);

private:
    void buildFileMenu(QMenu* menu);
    void buildDisplayMenu(QMenu* menu);
    void buildViewMenu(QMenu* menu);
    void buildWindowMenu();
    void buildHelpMenu(QMenu* menu);

    QMainWindow* window_ = nullptr;
    bonsaiviewer::SessionState* session_state_ = nullptr;
    ViewportWindow* viewport_ = nullptr;
    bonsaiviewer::services::CategoryOverrideFilter* category_filter_ = nullptr;
    bonsaiviewer::services::PanelManager* panel_manager_ = nullptr;
    QMenu* window_menu_ = nullptr;
    std::function<void(QMenu*)> recent_populator_;
};

} // namespace bonsaiviewer::modules::menubar

#endif
