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

#include "MenuBar.h"

#include "../../SessionState.h"
#include "../../../ifcviewer/ViewportWindow.h"
#include "../models/Commands.h"
#include "../project/Commands.h"
#include "../settings/Dialog.h"
#include "../viewport/Commands.h"
#include "../../services/CategoryOverrideFilter.h"
#include "../../services/PanelManager.h"
#include "../../services/PopupService.h"

#include <QAction>
#include <QDesktopServices>
#include <QIcon>
#include <QMainWindow>
#include <QMenuBar>
#include <QMenu>
#include <QUrl>

namespace bonsaiviewer::modules::menubar {

namespace {

struct CategorySpec {
    const char* ifc_class;
    const char* label;
    // TI19 (Display Drawings) is not deployable yet — TI01 parks the
    // IfcDrawings toggle as disabled until it lands.
    bool enabled;
};

const CategorySpec kCategories[] = {
    {"IfcSpace", "IfcSpaces", true},
    {"IfcOpeningElement", "IfcOpeningElements", true},
    {"IfcZone", "IfcZones", true},
    {"IfcDrawing", "IfcDrawings", false},
};

// BonsaiViewer01: the icons of the File menu, from the resources.
QIcon icon(const char* name) {
    return QIcon(QStringLiteral(":/icons/%1.svg").arg(QLatin1String(name)));
}

} // namespace

MenuBar::MenuBar(QMainWindow* window,
                 bonsaiviewer::SessionState* session_state,
                 ViewportWindow* viewport,
                 bonsaiviewer::services::CategoryOverrideFilter* category_filter,
                 bonsaiviewer::services::PanelManager* panel_manager,
                 QObject* parent)
    : QObject(parent),
      window_(window),
      session_state_(session_state),
      viewport_(viewport),
      category_filter_(category_filter),
      panel_manager_(panel_manager) {
    buildFileMenu(window->menuBar()->addMenu(QStringLiteral("File")));
    buildDisplayMenu(window->menuBar()->addMenu(QStringLiteral("Display")));
    buildViewMenu(window->menuBar()->addMenu(QStringLiteral("View")));
    window_menu_ = window->menuBar()->addMenu(QStringLiteral("Window"));
    buildWindowMenu();
    // Help always sits at the far right: it is added last (menu bars lay
    // menus out left to right, Help is the final entry).
    buildHelpMenu(window->menuBar()->addMenu(QStringLiteral("Help")));
}

void MenuBar::setOpenRecentPopulator(std::function<void(QMenu*)> populator) {
    recent_populator_ = std::move(populator);
}

void MenuBar::buildFileMenu(QMenu* menu) {
    connect(menu->addAction(icon("plus-square"), QStringLiteral("New")), &QAction::triggered, this, [this]() {
        modules::project::commands::newProject(*session_state_, *window_, *viewport_);
    });
    connect(menu->addAction(icon("folder"), QStringLiteral("Open")), &QAction::triggered, this, [this]() {
        modules::project::commands::openProject(*session_state_, *window_, *viewport_);
    });
    connect(menu->addAction(icon("cloud-square"), QStringLiteral("Open Cloud")), &QAction::triggered, this, [this]() {
        modules::project::commands::openCloudProject(*session_state_, *window_, *viewport_);
    });
    QMenu* recent = menu->addMenu(icon("clock-rotate-right"), QStringLiteral("Open Recent"));
    recent->setToolTipsVisible(true);
    connect(recent, &QMenu::aboutToShow, this, [this, recent]() {
        if (recent_populator_) recent_populator_(recent);
    });
    menu->addSeparator();
    connect(menu->addAction(icon("plus"), QStringLiteral("Add Model")), &QAction::triggered, this, [this]() {
        modules::models::commands::addModel(*session_state_, *window_);
    });
    connect(menu->addAction(icon("refresh-double"), QStringLiteral("Sync From Cloud")), &QAction::triggered, this, [this]() {
        modules::project::commands::syncCloudProject(*session_state_, *window_, *viewport_);
    });
    menu->addSeparator();
    connect(menu->addAction(icon("floppy-disk"), QStringLiteral("Save")), &QAction::triggered, this, [this]() {
        modules::project::commands::saveProject(*session_state_, *window_);
    });
    connect(menu->addAction(icon("floppy-disk-arrow-in"), QStringLiteral("Save As")), &QAction::triggered, this, [this]() {
        modules::project::commands::saveProjectAs(*session_state_, *window_);
    });
    menu->addSeparator();
    connect(menu->addAction(icon("settings"), QStringLiteral("Settings")), &QAction::triggered, this, [this]() {
        modules::settings::SettingsDialog dialog(session_state_, window_);
        dialog.exec();
    });
}

void MenuBar::buildDisplayMenu(QMenu* menu) {
    for (const CategorySpec& spec : kCategories) {
        auto* action = menu->addAction(QString::fromUtf8(spec.label));
        action->setCheckable(true);
        action->setEnabled(spec.enabled);
        // B03 restores its disabled set at startup and persists every
        // change (B14), so the menu is a view of B03's state — no second
        // persistence path here.
        action->setChecked(category_filter_
                               ? category_filter_->isCategoryEnabled(
                                     QString::fromUtf8(spec.ifc_class))
                               : true);
        const QString ifc_class = QString::fromUtf8(spec.ifc_class);
        connect(action, &QAction::toggled, this, [this, ifc_class](bool checked) {
            if (category_filter_) category_filter_->setCategoryEnabled(ifc_class, checked);
        });
        if (category_filter_) {
            connect(category_filter_, &bonsaiviewer::services::CategoryOverrideFilter::categoryChanged,
                    this, [action, ifc_class](const QString& changed, bool enabled) {
                        if (changed == ifc_class) action->setChecked(enabled);
                    });
        }
    }
}

void MenuBar::buildViewMenu(QMenu* menu) {
    connect(menu->addAction(QStringLiteral("Fly")), &QAction::triggered, this, [this]() {
        if (viewport_) {
            modules::viewport::commands::fly(*session_state_, *viewport_);
        }
    });
}

void MenuBar::buildWindowMenu() {
    if (!panel_manager_) {
        window_menu_->addAction(tr("No panels registered"))->setEnabled(false);
        return;
    }
    panel_manager_->buildWindowMenu(window_menu_);
    // Rebuild when panels register later (TI01: rebuilt when panels
    // register).
    connect(panel_manager_, &bonsaiviewer::services::PanelManager::panelRegistered,
            this, [this]() {
                if (!panel_manager_) return;
                panel_manager_->buildWindowMenu(window_menu_);
            });
}

void MenuBar::buildHelpMenu(QMenu* menu) {
    connect(menu->addAction(QStringLiteral("Documentation")), &QAction::triggered, this, []() {
        QDesktopServices::openUrl(QUrl(QStringLiteral("https://docs.bonsaiflows.com")));
    });
    connect(menu->addAction(QStringLiteral("About")), &QAction::triggered, this, [this]() {
        services::popup::information(window_,
            QStringLiteral("Bonsai Viewer " BONSAIVIEWER_VERSION
                           "\n\nLicence: GPL-3.0-or-later.\n"
                           "Bonsai is free software: you can redistribute it "
                           "and/or modify it under the terms of the GNU "
                           "General Public License as published by the Free "
                           "Software Foundation, either version 3 of the "
                           "License, or (at your option) any later version."));
    });
}

} // namespace bonsaiviewer::modules::menubar
