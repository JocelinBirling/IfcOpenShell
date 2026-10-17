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

#include "MainWindow.h"

#include "../ifcviewer/AppSettings.h"
#include "../ifcviewer/Federation.h"
#include "../ifcviewer/OverlayRenderer.h"
#include "../ifcviewer/SceneLoader.h"
#include "../ifcviewer/ViewportWindow.h"
#include "SessionState.h"
#include "ElementRegistry.h"
#include "components/Buttons.h"
#include "components/Panel.h"
#include "components/Style.h"
#include "components/Tabs.h"
#include "modules/models/Commands.h"
#include "modules/todo/Panel.h"
#include "modules/rule/Catalogue.h"
#include "modules/rule/ResultInfoPanel.h"
#include "modules/rule/ResultPanel.h"
#include "modules/rule/RulePanel.h"
#include "modules/rule/RuleConfigPanel.h"
#include "modules/rule/RuleLibraryPanel.h"
#include "modules/rule/engineproxy/EngineClient.h"
#include "modules/rule/engineproxy/RuleFileJson.h"
#include "modules/models/View.h"
#include "modules/models/Panel.h"
#include "modules/project/Commands.h"
#include "modules/project/RecentProjects.h"
#include "modules/properties/View.h"
#include "modules/properties/Panel.h"
#include "modules/settings/Dialog.h"
#include "modules/spatial_hierarchy/View.h"
#include "modules/spatial_hierarchy/Panel.h"
#include "modules/viewport/Commands.h"
#include "modules/viewport/Panel.h"
#include "modules/viewport/View.h"

#include <QAction>
#include <QDebug>
#include <QDockWidget>
#include <QFile>
#include <QFileDialog>
#include <QJsonDocument>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QShortcut>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QStatusBar>
#include <QProgressBar>
#include <QToolButton>
#include <QTreeWidgetItemIterator>
#include <QVBoxLayout>

#include <cmath>
#include <unordered_map>

namespace bonsaiviewer::shell {

namespace {

using namespace bonsaiviewer::modules::rule;

// Wireframe segments of a world-space OBB drawing item: the 12 edges of
// the box rebuilt from its center, orthonormal axes and half sizes.
std::vector<float> obbWireframe(const engineproxy::DisplayItem& item) {
    std::array<std::array<float, 3>, 8> corners;
    for (int i = 0; i < 8; ++i) {
        const float signs[3] = {(i & 1) ? 1.0f : -1.0f,
                                (i & 2) ? 1.0f : -1.0f,
                                (i & 4) ? 1.0f : -1.0f};
        for (int component = 0; component < 3; ++component) {
            float value = item.obb_center[component];
            for (int axis = 0; axis < 3; ++axis) {
                value += signs[axis] * item.obb_half_sizes[axis] * item.obb_axes[axis][component];
            }
            corners[i][component] = value;
        }
    }
    static constexpr int kEdges[12][2] = {
        {0, 1}, {1, 3}, {3, 2}, {2, 0},  // bottom face
        {4, 5}, {5, 7}, {7, 6}, {6, 4},  // top face
        {0, 4}, {1, 5}, {3, 7}, {2, 6},  // verticals
    };
    std::vector<float> segments;
    segments.reserve(12 * 6);
    for (const auto& edge : kEdges) {
        for (int endpoint : edge) {
            for (int component = 0; component < 3; ++component) {
                segments.push_back(corners[endpoint][component]);
            }
        }
    }
    return segments;
}

// The overlay line groups of one engine display payload (the display
// contract of the integration spec): detection zones dashed magenta,
// object OBBs solid yellow, the other shapes in the engine's own color.
std::vector<OverlayRenderer::LineGroup> overlayFromDisplayItems(
        const std::vector<engineproxy::DisplayItem>& items) {
    std::vector<OverlayRenderer::LineGroup> groups;
    OverlayRenderer::LineGroup zones;
    zones.color[0] = 0.8f; zones.color[1] = 0.2f; zones.color[2] = 1.0f; zones.color[3] = 0.9f;
    zones.line_width = 2.0f;
    zones.dash_period_px = 12.0f;
    OverlayRenderer::LineGroup obbs;
    obbs.color[0] = 1.0f; obbs.color[1] = 0.85f; obbs.color[2] = 0.1f; obbs.color[3] = 0.9f;
    obbs.line_width = 1.5f;
    OverlayRenderer::LineGroup others;
    others.color[0] = 0.6f; others.color[1] = 0.7f; others.color[2] = 0.6f; others.color[3] = 0.9f;
    others.line_width = 1.5f;

    const auto append_segments = [](OverlayRenderer::LineGroup& group,
                                     const std::vector<float>& segments) {
        for (float coordinate : segments) group.world_xyz.push_back(coordinate);
    };

    for (const engineproxy::DisplayItem& item : items) {
        if (item.kind == QStringLiteral("obb")) {
            append_segments(item.category == QStringLiteral("zone") ? zones : obbs,
                            obbWireframe(item));
        } else if (item.kind == QStringLiteral("segments")) {
            for (const auto& segment : item.segments) {
                for (float coordinate : segment) others.world_xyz.push_back(coordinate);
            }
        } else if (item.kind == QStringLiteral("edge") && !item.segments.empty()) {
            for (float coordinate : item.segments.front()) others.world_xyz.push_back(coordinate);
        } else if (item.kind == QStringLiteral("face") && item.corners.size() >= 2) {
            for (size_t i = 0; i < item.corners.size(); ++i) {
                const auto& a = item.corners[i];
                const auto& b = item.corners[(i + 1) % item.corners.size()];
                for (float coordinate : a) others.world_xyz.push_back(coordinate);
                for (float coordinate : b) others.world_xyz.push_back(coordinate);
            }
        } else if (item.kind == QStringLiteral("axis")) {
            // A segment centered on the item, along its direction.
            for (int component = 0; component < 3; ++component) {
                others.world_xyz.push_back(item.center[component] - 0.5f * item.length * item.direction[component]);
            }
            for (int component = 0; component < 3; ++component) {
                others.world_xyz.push_back(item.center[component] + 0.5f * item.length * item.direction[component]);
            }
        }
    }
    if (!zones.world_xyz.empty()) groups.push_back(std::move(zones));
    if (!obbs.world_xyz.empty()) groups.push_back(std::move(obbs));
    if (!others.world_xyz.empty()) groups.push_back(std::move(others));
    return groups;
}

} // namespace

MainWindow::~MainWindow() {
    // The engine proxy (a child QObject) shuts its process down itself.
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    session_state_ = new bonsaiviewer::SessionState(this);
    auto on_mutated = [this]() {
        setWindowModified(true);
        updateWindowTitle();
    };
    auto on_clean_state = [this]() {
        setWindowModified(false);
        updateWindowTitle();
    };
    connect(session_state_, &bonsaiviewer::SessionState::federationChanged, this, on_mutated);
    connect(session_state_, &bonsaiviewer::SessionState::modelsChanged,     this, on_mutated);
    connect(session_state_, &bonsaiviewer::SessionState::projectReset,      this, on_clean_state);
    connect(session_state_, &bonsaiviewer::SessionState::projectOpened,     this, on_clean_state);
    connect(session_state_, &bonsaiviewer::SessionState::projectSaved,      this, on_clean_state);

    // Every successful open/save (local or cloud) feeds the recent list.
    auto remember_recent = [](const QString& path) {
        modules::project::RecentProjects::add(path);
    };
    connect(session_state_, &bonsaiviewer::SessionState::projectOpened, this, remember_recent);
    connect(session_state_, &bonsaiviewer::SessionState::projectSaved,  this, remember_recent);
    setupChrome();
    setupViewport();
    setupPanels();
    setupStatus();
    setupLoader();
    setupRibbon();
    setupEngineConnections();
    resize(1720, 980);
}

void MainWindow::setupChrome() {
    setObjectName("appWindow");
    setDockOptions(QMainWindow::AllowNestedDocks |
                   QMainWindow::AllowTabbedDocks |
                   QMainWindow::GroupedDragging);

    auto bind_shortcut = [this](const QKeySequence& sequence, auto handler) {
        auto* shortcut = new QShortcut(sequence, this);
        shortcut->setContext(Qt::WindowShortcut);
        connect(shortcut, &QShortcut::activated, this, handler);
    };
    bind_shortcut(QKeySequence("Ctrl+Shift+L"), [this]() {
        modules::viewport::commands::toggleDistance(*viewport_widget_->viewport());
    });
    bind_shortcut(QKeySequence("Ctrl+Shift+A"), [this]() {
        modules::viewport::commands::toggleArea(*viewport_widget_->viewport());
    });
    bind_shortcut(QKeySequence("Ctrl+Shift+V"), [this]() {
        modules::viewport::commands::toggleVolume(*viewport_widget_->viewport());
    });
    bind_shortcut(QKeySequence(Qt::SHIFT | Qt::Key_F), [this]() {
        modules::viewport::commands::fly(*session_state_, *viewport_widget_->viewport());
    });
    bind_shortcut(QKeySequence(Qt::Key_K), [this]() {
        modules::viewport::commands::toggleSection(*session_state_, *viewport_widget_->viewport());
    });
    bind_shortcut(QKeySequence(Qt::SHIFT | Qt::Key_K), [this]() {
        modules::viewport::commands::clearSection(*session_state_, *viewport_widget_->viewport());
    });
    bind_shortcut(QKeySequence(Qt::Key_H), [this]() {
        modules::viewport::commands::hideSelected(*viewport_widget_->viewport());
    });
    bind_shortcut(QKeySequence(Qt::SHIFT | Qt::Key_H), [this]() {
        modules::viewport::commands::isolateSelected(*viewport_widget_->viewport());
    });
    bind_shortcut(QKeySequence(Qt::ALT | Qt::Key_H), [this]() {
        modules::viewport::commands::showAll(*viewport_widget_->viewport());
    });
    updateWindowTitle();
}

QToolButton* MainWindow::makePanelToggle(const QString& text, QDockWidget* dock) {
    auto* button = components::buttons::makeButton(text, ":/icons/sidebar-expand.svg", this);
    button->setCheckable(true);
    button->setChecked(dock->isVisible());
    connect(button, &QToolButton::toggled, dock, [dock](bool checked) {
        dock->setVisible(checked);
        if (checked) dock->raise();
    });
    connect(dock, &QDockWidget::visibilityChanged, button, [button](bool visible) {
        const QSignalBlocker blocker(button);
        button->setChecked(visible);
    });
    return button;
}

void MainWindow::populateRecentMenu(QMenu* menu) {
    menu->clear();

    const QStringList recent = modules::project::RecentProjects::list();
    if (recent.isEmpty()) {
        QAction* empty = menu->addAction("No Recent Projects");
        empty->setEnabled(false);
        return;
    }

    for (const QString& path : recent) {
        QAction* action = menu->addAction(QFileInfo(path).fileName());
        action->setToolTip(path);
        connect(action, &QAction::triggered, this, [this, path]() {
            const bool opened = modules::project::commands::openProjectPath(
                *session_state_, *this, *viewport_widget_->viewport(), path);
            // A recent entry that no longer loads is dropped so it stops
            // cluttering the menu (e.g. moved/deleted file).
            if (!opened && !QFileInfo::exists(path)) {
                modules::project::RecentProjects::remove(path);
            }
        });
    }

    menu->addSeparator();
    QAction* clear = menu->addAction("Clear Recent Projects");
    connect(clear, &QAction::triggered, this, []() {
        modules::project::RecentProjects::clear();
    });
}

QWidget* MainWindow::buildHomeRibbonPage() {
    auto* page = new QFrame(this);
    page->setObjectName("ribbonPage");
    auto* row = new QHBoxLayout(page);
    row->setContentsMargins(6, 4, 6, 4);
    row->setSpacing(0);

    auto* new_project = components::buttons::makeButton("New Project", ":/icons/plus-square.svg", this);
    connect(new_project, &QToolButton::clicked, this, [this]() {
        modules::project::commands::newProject(
            *session_state_, *this, *viewport_widget_->viewport());
    });
    auto* open_project = components::buttons::makeButton("Open Project", ":/icons/download-square.svg", this);
    connect(open_project, &QToolButton::clicked, this, [this]() {
        modules::project::commands::openProject(
            *session_state_, *this, *viewport_widget_->viewport());
    });
    auto* open_cloud = components::buttons::makeButton("Open Cloud", ":/icons/cloud-square.svg", this);
    connect(open_cloud, &QToolButton::clicked, this, [this]() {
        modules::project::commands::openCloudProject(
            *session_state_, *this, *viewport_widget_->viewport());
    });
    auto* open_recent = components::buttons::makeButton("Open Recent", ":/icons/clock-rotate-right.svg", this);
    auto* recent_menu = new QMenu(open_recent);
    recent_menu->setToolTipsVisible(true);
    open_recent->setMenu(recent_menu);
    open_recent->setPopupMode(QToolButton::InstantPopup);
    connect(recent_menu, &QMenu::aboutToShow, this, [this, recent_menu]() {
        populateRecentMenu(recent_menu);
    });
    auto* save_project = components::buttons::makeButton("Save Project", ":/icons/floppy-disk.svg", this);
    connect(save_project, &QToolButton::clicked, this, [this]() {
        modules::project::commands::saveProjectDialog(*session_state_, *this);
    });

    auto* add_model = components::buttons::makeButton("Add Model", ":/icons/cube.svg", this);
    connect(add_model, &QToolButton::clicked, this, [this]() {
        modules::models::commands::addModel(*session_state_, *this);
    });
    auto* sync_from_cloud = components::buttons::makeButton("Sync From Cloud", ":/icons/refresh-double.svg", this);
    connect(sync_from_cloud, &QToolButton::clicked, this, [this]() {
        modules::project::commands::syncCloudProject(
            *session_state_, *this, *viewport_widget_->viewport());
    });

    auto* settings_button = components::buttons::makeButton("Settings", ":/icons/settings.svg", this);
    connect(settings_button, &QToolButton::clicked, this, [this]() {
        modules::settings::SettingsDialog dialog(session_state_, this);
        dialog.exec();
    });

    components::buttons::addButtonGroups(row, {
        components::buttons::makeButtonGroup("PROJECT", {new_project, open_project, open_cloud, open_recent, save_project}, this),
        components::buttons::makeButtonGroup("MODELS", {add_model, sync_from_cloud}, this),
        components::buttons::makeButtonGroup("SETTINGS", {settings_button}, this),
    });
    row->addStretch(1);
    return page;
}

QWidget* MainWindow::buildNavigateRibbonPage() {
    auto* page = new QFrame(this);
    page->setObjectName("ribbonPage");
    auto* row = new QHBoxLayout(page);
    row->setContentsMargins(2, 4, 2, 4);
    row->setSpacing(0);

    auto* set_home = components::buttons::makeButton("Set Home", ":/icons/home.svg", this);
    connect(set_home, &QToolButton::clicked, this, [this]() {
        modules::viewport::commands::setHome(*session_state_, *viewport_widget_->viewport());
    });
    auto* go_home = components::buttons::makeButton("Go Home", ":/icons/home-alt.svg", this);
    connect(go_home, &QToolButton::clicked, this, [this]() {
        modules::viewport::commands::goHome(*session_state_, *viewport_widget_->viewport());
    });
    auto* view_all = components::buttons::makeButton("View All", ":/icons/cube-scan.svg", this);
    connect(view_all, &QToolButton::clicked, this, [this]() {
        if (viewport_widget_) viewport_widget_->viewport()->viewAll();
    });
    auto* view_selected = components::buttons::makeButton("View Selected", ":/icons/cube-scan-solid.svg", this);
    connect(view_selected, &QToolButton::clicked, this, [this]() {
        modules::viewport::commands::viewSelected(*viewport_widget_->viewport());
    });

    auto* plan_view = components::buttons::makeButton("Plan", ":/icons/planimetry.svg", this);
    connect(plan_view, &QToolButton::clicked, this, [this]() {
        if (viewport_widget_) viewport_widget_->viewport()->setStandardView(90.0f, 90.0f);
    });
    auto* front_view = components::buttons::makeButton("Front", ":/icons/city.svg", this);
    connect(front_view, &QToolButton::clicked, this, [this]() {
        if (viewport_widget_) viewport_widget_->viewport()->setStandardView(0.0f, 0.0f);
    });
    auto* side_view = components::buttons::makeButton("Side", ":/icons/building.svg", this);
    connect(side_view, &QToolButton::clicked, this, [this]() {
        if (viewport_widget_) viewport_widget_->viewport()->setStandardView(90.0f, 0.0f);
    });
    auto* align_object = components::buttons::makeButton("Align Object", ":/icons/cellar.svg", this);
    connect(align_object, &QToolButton::clicked, this, [this]() {
        session_state_->setStatusMessage("Orientation", "Align to object coming soon");
    });
    auto* projection_button = components::buttons::makeButton("Perspective", ":/icons/perspective-view.svg", this);
    connect(projection_button, &QToolButton::clicked, this, [this, projection_button]() {
        if (!viewport_widget_) return;
        viewport_widget_->viewport()->toggleProjection();
        projection_button->setText(
            viewport_widget_->viewport()->projectionOrtho() ? "Ortho" : "Perspective");
    });

    auto* fly_mode = components::buttons::makeButton("Fly", ":/icons/drone.svg", this);
    connect(fly_mode, &QToolButton::clicked, this, [this]() {
        modules::viewport::commands::fly(*session_state_, *viewport_widget_->viewport());
    });
    auto* section_mode = components::buttons::makeButton("Section", ":/icons/cube-cut-with-curve.svg", this);
    connect(section_mode, &QToolButton::clicked, this, [this]() {
        modules::viewport::commands::toggleSection(*session_state_, *viewport_widget_->viewport());
    });

    components::buttons::addButtonGroups(row, {
        components::buttons::makeButtonGroup("CAMERA", {set_home, go_home, view_all, view_selected}, this),
        components::buttons::makeButtonGroup("ORIENTATION", {plan_view, front_view, side_view, align_object, projection_button}, this),
        components::buttons::makeButtonGroup("MODE", {fly_mode, section_mode}, this),
    });
    row->addStretch(1);
    return page;
}

QWidget* MainWindow::buildInspectRibbonPage() {
    auto* page = new QFrame(this);
    page->setObjectName("ribbonPage");
    auto* row = new QHBoxLayout(page);
    row->setContentsMargins(2, 4, 2, 4);
    row->setSpacing(0);

    auto* hide_selected = components::buttons::makeButton("Hide", ":/icons/eye-closed.svg", this);
    connect(hide_selected, &QToolButton::clicked, this, [this]() {
        modules::viewport::commands::hideSelected(*viewport_widget_->viewport());
    });
    auto* isolate_selected = components::buttons::makeButton("Isolate", ":/icons/eye-solid.svg", this);
    connect(isolate_selected, &QToolButton::clicked, this, [this]() {
        modules::viewport::commands::isolateSelected(*viewport_widget_->viewport());
    });
    auto* show_all = components::buttons::makeButton("Show All", ":/icons/eye.svg", this);
    connect(show_all, &QToolButton::clicked, this, [this]() {
        modules::viewport::commands::showAll(*viewport_widget_->viewport());
    });
    auto* invert_selection = components::buttons::makeButton("Invert", ":/icons/intersect.svg", this);
    connect(invert_selection, &QToolButton::clicked, this, [this]() {
        modules::viewport::commands::invertVisibility(*viewport_widget_->viewport());
    });

    auto* distance = components::buttons::makeButton("Distance", ":/icons/select-edge3d.svg", this);
    connect(distance, &QToolButton::clicked, this, [this]() {
        modules::viewport::commands::toggleDistance(*viewport_widget_->viewport());
    });
    auto* area = components::buttons::makeButton("Area", ":/icons/select-face3d.svg", this);
    connect(area, &QToolButton::clicked, this, [this]() {
        modules::viewport::commands::toggleArea(*viewport_widget_->viewport());
    });
    auto* volume = components::buttons::makeButton("Volume", ":/icons/select-point3d.svg", this);
    connect(volume, &QToolButton::clicked, this, [this]() {
        modules::viewport::commands::toggleVolume(*viewport_widget_->viewport());
    });

    components::buttons::addButtonGroups(row, {
        components::buttons::makeButtonGroup("SELECTION", {hide_selected, isolate_selected, show_all, invert_selection}, this),
        components::buttons::makeButtonGroup("MEASURE", {distance, area, volume}, this),
    });
    row->addStretch(1);
    return page;
}

QWidget* MainWindow::buildPanelsRibbonPage() {
    auto* page = new QFrame(this);
    page->setObjectName("ribbonPage");
    auto* row = new QHBoxLayout(page);
    row->setContentsMargins(2, 4, 2, 4);
    row->setSpacing(0);

    components::buttons::addButtonGroups(row, {
        components::buttons::makeButtonGroup("DATA", {
            makePanelToggle("Models", models_panel_),
            makePanelToggle("Spatial", spatial_panel_),
            makePanelToggle("Layers", layers_panel_),
            makePanelToggle("Properties", properties_panel_)
        }, this),
        components::buttons::makeButtonGroup("QUERY", {
            makePanelToggle("Views", stored_views_panel_),
            makePanelToggle("Search", search_panel_),
            makePanelToggle("Sheets", spreadsheet_panel_),
            makePanelToggle("Audit", audit_panel_)
        }, this),
        components::buttons::makeButtonGroup("COLLABORATE", {
            makePanelToggle("Issues", issues_panel_)
        }, this),
    });
    row->addStretch(1);
    return page;
}

QWidget* MainWindow::buildGeometryRuleRibbonPage() {
    auto* page = new QFrame(this);
    page->setObjectName("ribbonPage");
    auto* row = new QHBoxLayout(page);
    row->setContentsMargins(2, 4, 2, 4);
    row->setSpacing(0);

    auto* color_all_red = components::buttons::makeButton("Color Red", ":/icons/paint.svg", this);
    color_all_red->setCheckable(true);
    connect(color_all_red, &QToolButton::toggled, this, [this](bool checked) {
        if (checked) modules::viewport::commands::colorAllRed(*viewport_widget_->viewport());
        else         modules::viewport::commands::clearColors(*viewport_widget_->viewport());
    });

    auto* run_rule = components::buttons::makeButton("Run Rule", ":/icons/intersect.svg", this);
    run_rule_button_ = run_rule;
    connect(run_rule, &QToolButton::clicked, this, [this]() { runGeometryRules(); });

    auto* show_results = components::buttons::makeButton("Show Results", ":/icons/check.svg", this);
    connect(show_results, &QToolButton::clicked, this, [this]() {
        // The results live in the Clash Filter workspace.
        if (clash_filter_page_index_ >= 0) ribbon_tabs_->setCurrentIndex(clash_filter_page_index_);
    });

    components::buttons::addButtonGroups(row, {
        components::buttons::makeButtonGroup("RUN", {run_rule, show_results}, this),
        components::buttons::makeButtonGroup("COLOR", {color_all_red}, this),
    });
    row->addStretch(1);
    return page;
}

QWidget* MainWindow::buildClashFilterRibbonPage() {
    auto* page = new QFrame(this);
    page->setObjectName("ribbonPage");
    auto* row = new QHBoxLayout(page);
    row->setContentsMargins(2, 4, 2, 4);
    row->setSpacing(0);

    // ---- Context View --------------------------------------------------
    auto* save_context = components::buttons::makeButton("Save Context", ":/icons/floppy-disk.svg", this);
    connect(save_context, &QToolButton::clicked, this, [this]() {
        context_ids_ = viewport_widget_->viewport()->visibleObjectIds();
        context_saved_ = !context_ids_.empty();
        session_state_->setStatusMessage("Clash Filter",
            context_saved_
                ? QString("Context saved: %1 object(s)").arg(context_ids_.size())
                : QString("No visible object to save"));
    });

    auto* show_context = components::buttons::makeButton("Show Context", ":/icons/eye.svg", this);
    connect(show_context, &QToolButton::clicked, this, [this]() {
        if (!context_saved_) {
            session_state_->setStatusMessage("Clash Filter", "No saved context");
            return;
        }
        viewport_widget_->viewport()->setObjectsVisible(context_ids_, true);
        session_state_->setStatusMessage("Clash Filter",
            QString("Context shown: %1 object(s)").arg(context_ids_.size()));
    });

    auto* always_context = components::buttons::makeButton(
        "Always Display Context", ":/icons/cube-dots.svg", this);
    always_context->setCheckable(true);
    connect(always_context, &QToolButton::toggled, this, [this](bool checked) {
        always_display_context_ = checked;
    });

    auto* color_result = components::buttons::makeButton("Color Result", ":/icons/paint.svg", this);
    connect(color_result, &QToolButton::clicked, this, [this]() {
        std::vector<uint32_t> object_ids;
        for (const auto& row_item : last_result_rows_) {
            if (row_item.source_object_id) object_ids.push_back(row_item.source_object_id);
            if (row_item.target_object_id) object_ids.push_back(row_item.target_object_id);
        }
        // 0xAABBGGRR: red for every result element.
        viewport_widget_->viewport()->colorElements(object_ids, 0xFF0000FF);
        session_state_->setStatusMessage("Clash Filter",
            QString("%1 result object(s) colored red").arg(object_ids.size()));
    });

    // ---- Regroup Result ------------------------------------------------
    auto* filter_non_determined = components::buttons::makeButton(
        "Filter Only Non Determined", ":/icons/filter.svg", this);
    filter_non_determined->setCheckable(true);
    connect(filter_non_determined, &QToolButton::toggled, this, [this](bool checked) {
        result_content_->setFilterOnlyNonDetermined(checked);
    });

    auto* group_source = components::buttons::makeButton(
        "Group by source entity", ":/icons/cube.svg", this);
    group_source->setCheckable(true);
    auto* group_target = components::buttons::makeButton(
        "Group by target entity", ":/icons/cube.svg", this);
    group_target->setCheckable(true);
    connect(group_source, &QToolButton::toggled, this, [this, group_target](bool checked) {
        if (checked) group_target->setChecked(false);
        result_content_->setGroupMode(
            checked ? modules::rule::ResultPanel::GroupMode::BySource
                    : modules::rule::ResultPanel::GroupMode::Flat);
    });
    connect(group_target, &QToolButton::toggled, this, [this, group_source](bool checked) {
        if (checked) group_source->setChecked(false);
        result_content_->setGroupMode(
            checked ? modules::rule::ResultPanel::GroupMode::ByTarget
                    : modules::rule::ResultPanel::GroupMode::Flat);
    });

    components::buttons::addButtonGroups(row, {
        components::buttons::makeButtonGroup(
            "CONTEXT VIEW", {save_context, show_context, always_context, color_result}, this),
        components::buttons::makeButtonGroup(
            "REGROUP RESULT", {filter_non_determined, group_source, group_target}, this),
    });
    row->addStretch(1);
    return page;
}

void MainWindow::setupEngineConnections() {
    namespace engineproxy = modules::rule::engineproxy;
    engine_client_ = new engineproxy::EngineClient(this);

    connect(engine_client_, &engineproxy::EngineClient::engineReady, this,
            [this](const QString& version) {
        session_state_->setStatusMessage(
            "GeometryRule", QString("Rule engine ready: ifcclash_plus %1").arg(version));
        // The engine's registry replaces the static catalogue mirror.
        engine_client_->requestCatalogue();
    });
    connect(engine_client_, &engineproxy::EngineClient::engineFailed, this,
            [this](const QString& error) { session_state_->setStatusMessage("GeometryRule", error); });
    connect(engine_client_, &engineproxy::EngineClient::engineLog, this,
            [this](const QString& level, const QString& message) {
        qDebug() << "rule engine" << level << ":" << message;
    });
    connect(engine_client_, &engineproxy::EngineClient::catalogueReady, this,
            [this](const QJsonArray& rules, const QJsonArray& facets) {
        const int count = modules::rule::setCatalogueFromEngine(rules, facets);
        if (rule_library_content_) rule_library_content_->rebuildSections();
        if (count > 0) {
            session_state_->setStatusMessage(
                "GeometryRule",
                QString("Rule catalogue: %1 definition(s) loaded from the engine").arg(count));
        }
    });
    connect(engine_client_, &engineproxy::EngineClient::progress, this,
            [this](int percent, const QString& message) {
        if (percent >= 0) session_state_->setProgress(percent);
        if (!message.isEmpty()) session_state_->setStatusMessage("GeometryRule", message);
    });
    connect(engine_client_, &engineproxy::EngineClient::ruleResult, this,
            [this](const QString& node_id, const QString& rule_type,
                   const std::vector<engineproxy::EngineResult>& results) {
        (void)node_id;
        collectEngineResults(rule_type, results);
    });
    connect(engine_client_, &engineproxy::EngineClient::displayItemsReady, this,
            [this](const QString& node_id, const QString& rule_type,
                   const std::vector<engineproxy::DisplayItem>& items) {
        (void)node_id;
        showEngineDisplay(rule_type, items);
    });
    connect(engine_client_, &engineproxy::EngineClient::runFinished, this,
            [this](bool cancelled, const QString& error) {
        if (!error.isEmpty()) session_state_->setStatusMessage("GeometryRule", error);
        finishGeometryRuleRun(cancelled);
    });
    connect(engine_client_, &engineproxy::EngineClient::selectFinished, this,
            [this](const std::vector<engineproxy::ElementRef>& elements) {
        showFacetSelectionResult(elements);
    });
}

void MainWindow::runGeometryRules(QTreeWidgetItem* root, bool preview) {
    if (!rule_content_ || !engine_client_) return;
    if (engine_client_->isJobRunning()) {
        session_state_->setStatusMessage("GeometryRule", "A rule run is already in progress");
        return;
    }

    // The scope: the whole open file, or the file / folder / rule item.
    const QJsonObject scope =
        modules::rule::engineproxy::scopeToJson(rule_content_->tree(), root);
    if (scope.isEmpty() || scope.value("contains").toArray().isEmpty()) {
        session_state_->setStatusMessage(
            "GeometryRule",
            preview
                ? "No display input: add facets to the rule's source and target slots"
                : "No runnable rule: add a rule and drop facets into its source and target slots");
        return;
    }

    // The models of the session, as file paths for the engine to open.
    QStringList paths;
    std::vector<uint32_t> model_ids;
    int skipped_models = 0;
    for (uint32_t session_model_id : session_state_->sessionModelIds()) {
        const QString path = session_state_->loader()->filePath(session_model_id);
        if (!path.isEmpty()) {
            paths << path;
            model_ids.push_back(session_model_id);
        } else {
            // No file on disk (e.g. a stream with no source path): the
            // model takes no part in this run.
            ++skipped_models;
        }
    }
    if (paths.isEmpty()) {
        session_state_->setStatusMessage(
            "GeometryRule", preview ? "Load a model first" : "Load a model before running rules");
        return;
    }
    run_model_ids_ = model_ids;
    rule_run_skipped_models_ = skipped_models;
    last_result_rows_.clear();
    run_source_objects_.clear();
    run_target_objects_.clear();
    run_is_preview_ = preview;

    if (run_rule_button_) run_rule_button_->setEnabled(false);
    session_state_->beginProgress("GeometryRule");
    if (!engine_client_->requestRun(scope, paths, preview)) {
        // Not configured / process failure: engineFailed already reported it.
        session_state_->endProgress();
        if (run_rule_button_) run_rule_button_->setEnabled(true);
    }
}

void MainWindow::collectEngineResults(const QString& rule_type,
                                      const std::vector<modules::rule::engineproxy::EngineResult>& results) {
    // Map IFC step ids back to viewport object ids, per model. An element
    // can have several object ids (net + gross split in the streamer); all
    // of them get colored, the first one found is used for selection.
    std::unordered_map<uint32_t, std::vector<BasicElementInfo>> elements_by_model;
    const auto infos_for = [this, &elements_by_model](uint32_t session_model_id) {
        auto it = elements_by_model.find(session_model_id);
        if (it == elements_by_model.end()) {
            it = elements_by_model.emplace(
                session_model_id,
                session_state_->elementRegistry()->basicElementInfoForModel(
                    session_model_id)).first;
        }
        return &it->second;
    };

    for (const auto& result : results) {
        modules::rule::ResultRow row;
        row.rule_name = rule_type;
        row.rule_type = rule_type;
        row.criticity = result.criticity.join(QStringLiteral(", "));
        row.actor = result.actor.join(QStringLiteral(", "));
        if (!result.measure_name.isEmpty() && !std::isnan(result.measure_value)) {
            row.measure = QStringLiteral("%1 = %2")
                              .arg(result.measure_name)
                              .arg(result.measure_value, 0, 'g', 4);
        }

        const auto resolve = [&](const modules::rule::engineproxy::ElementRef& ref,
                                 bool source) -> bool {
            if (!ref.isValid()) return false;
            if (ref.model < 0 || ref.model >= static_cast<int>(run_model_ids_.size())) return false;
            const uint32_t session_model_id = run_model_ids_[ref.model];
            if (source) row.source_session_model_id = session_model_id;
            else        row.target_session_model_id = session_model_id;
            const std::vector<BasicElementInfo>* infos = infos_for(session_model_id);
            for (const auto& info : *infos) {
                if (info.ifc_id != ref.step_id) continue;
                if (source) {
                    run_source_objects_.push_back(info.object_id);
                    if (!row.source_object_id) row.source_object_id = info.object_id;
                } else {
                    run_target_objects_.push_back(info.object_id);
                    if (!row.target_object_id) row.target_object_id = info.object_id;
                }
            }
            return true;
        };

        if (result.has_source && resolve(result.source, true)) {
            row.source_name = result.source.name;
            if (row.source_name.isEmpty()) {
                row.source_name = QStringLiteral("%1 #%2")
                                      .arg(result.source.ifc_type)
                                      .arg(result.source.step_id);
            }
            row.source_model_name =
                session_state_->loader()->displayName(row.source_session_model_id);
        }
        if (result.has_target && resolve(result.target, false)) {
            row.target_name = result.target.name;
            if (row.target_name.isEmpty()) {
                row.target_name = QStringLiteral("%1 #%2")
                                      .arg(result.target.ifc_type)
                                      .arg(result.target.step_id);
            }
            row.target_model_name =
                session_state_->loader()->displayName(row.target_session_model_id);
        }
        last_result_rows_.push_back(std::move(row));
    }
}

void MainWindow::showEngineDisplay(const QString& rule_type,
                                    const std::vector<modules::rule::engineproxy::DisplayItem>& items) {
    std::vector<OverlayRenderer::LineGroup> groups = overlayFromDisplayItems(items);
    viewport_widget_->viewport()->setOverlayLines(groups);
    session_state_->setStatusMessage(
        "GeometryRule",
        QString("Display input (%1): %2 drawing item(s) — lines clear with a "
                "measurement tool or a new display")
            .arg(rule_type)
            .arg(items.size()));
}

void MainWindow::finishGeometryRuleRun(bool cancelled) {
    if (run_rule_button_) run_rule_button_->setEnabled(true);
    session_state_->endProgress();

    if (run_is_preview_) {
        run_is_preview_ = false;
        return;
    }

    // 0xAABBGGRR: red for sources, orange for targets.
    viewport_widget_->viewport()->colorElements(run_source_objects_, 0xFF0000FF);
    viewport_widget_->viewport()->colorElements(run_target_objects_, 0xFF00A5FF);

    // The results replace the previous run's; the info panel resets and the
    // Clash Filter workspace comes to the front so the pairs are visible.
    result_content_->setResults(last_result_rows_);
    result_info_content_->showResult(nullptr);
    if (clash_filter_page_index_ >= 0) {
        ribbon_tabs_->setCurrentIndex(clash_filter_page_index_);
    }

    QString detail = QString("Rule run: %1 result(s) — %2 source(s) red, %3 target(s) orange")
                         .arg(last_result_rows_.size())
                         .arg(run_source_objects_.size())
                         .arg(run_target_objects_.size());
    if (cancelled) detail += QStringLiteral(" (run cancelled)");
    if (rule_run_skipped_models_ > 0) {
        detail += QString(" (%1 model(s) without a file path were skipped)")
                      .arg(rule_run_skipped_models_);
    }
    session_state_->setStatusMessage("GeometryRule", detail);
}

void MainWindow::showResultPair(int row_index) {
    if (row_index < 0 || row_index >= static_cast<int>(last_result_rows_.size())) return;
    const modules::rule::ResultRow& row = last_result_rows_[row_index];

    std::vector<uint32_t> pair;
    if (row.source_object_id) pair.push_back(row.source_object_id);
    if (row.target_object_id && row.target_object_id != row.source_object_id) {
        pair.push_back(row.target_object_id);
    }
    viewport_widget_->viewport()->setSelectedObjectIds(pair);
    viewport_widget_->viewport()->isolateSelectedElements();
    // The saved context stays on the view on top of the isolated pair.
    if (always_display_context_ && context_saved_) {
        viewport_widget_->viewport()->setObjectsVisible(context_ids_, true);
    }
    viewport_widget_->viewport()->focusOnSelectedObject();
}

void MainWindow::showRuleDisplayInput(QTreeWidgetItem* rule_item) {
    if (!rule_item) return;
    runGeometryRules(rule_item, /*preview=*/true);
}

void MainWindow::showFacetSelection(QTreeWidgetItem* facet_item) {
    if (!facet_item || !engine_client_) return;

    const QJsonObject select =
        modules::rule::engineproxy::facetSelectToJson(facet_item);
    if (select.isEmpty()) return;

    QStringList paths;
    std::vector<uint32_t> model_ids;
    for (uint32_t session_model_id : session_state_->sessionModelIds()) {
        const QString path = session_state_->loader()->filePath(session_model_id);
        if (!path.isEmpty()) {
            paths << path;
            model_ids.push_back(session_model_id);
        }
    }
    if (paths.isEmpty()) {
        session_state_->setStatusMessage("GeometryRule", "Load a model first");
        return;
    }
    run_model_ids_ = model_ids;
    engine_client_->requestSelect(select, paths);
}

void MainWindow::showFacetSelectionResult(
        const std::vector<modules::rule::engineproxy::ElementRef>& elements) {
    std::vector<uint32_t> object_ids;
    std::unordered_map<uint32_t, std::vector<BasicElementInfo>> elements_by_model;
    for (const auto& ref : elements) {
        if (!ref.isValid() || ref.model < 0
            || ref.model >= static_cast<int>(run_model_ids_.size())) {
            continue;
        }
        const uint32_t session_model_id = run_model_ids_[ref.model];
        auto it = elements_by_model.find(session_model_id);
        if (it == elements_by_model.end()) {
            it = elements_by_model.emplace(
                session_model_id,
                session_state_->elementRegistry()->basicElementInfoForModel(
                    session_model_id)).first;
        }
        for (const BasicElementInfo& info : it->second) {
            if (info.ifc_id == ref.step_id) object_ids.push_back(info.object_id);
        }
    }
    // SPEC GeometryRule 4.5: the page empties and only the facet's elements
    // stay on screen.
    viewport_widget_->viewport()->setObjectsVisible(
        viewport_widget_->viewport()->visibleObjectIds(), false);
    viewport_widget_->viewport()->setObjectsVisible(object_ids, true);
    session_state_->setStatusMessage(
        "GeometryRule",
        QString("Facet selection: %1 element(s) shown").arg(object_ids.size()));
}

void MainWindow::saveRuleFile() {
    if (!rule_content_) return;
    QTreeWidgetItem* file_item = rule_content_->tree()->openRuleFile();
    if (!file_item) {
        session_state_->setStatusMessage("GeometryRule", "No RuleFile open to save");
        return;
    }
    const QString path = QFileDialog::getSaveFileName(
        this, "Save Rule File", QString(), "Rule Files (*.ifcrule.json)");
    if (path.isEmpty()) return;

    const QJsonObject json = modules::rule::engineproxy::ruleFileToJson(file_item);
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        session_state_->setStatusMessage("GeometryRule", "Could not write the rule file");
        return;
    }
    file.write(QJsonDocument(json).toJson(QJsonDocument::Indented));
    file.close();
    session_state_->setStatusMessage("GeometryRule", "Rule file saved: " + path);
}

void MainWindow::loadRuleFile() {
    if (!rule_content_) return;
    const QString path = QFileDialog::getOpenFileName(
        this, "Load Rule File", QString(), "Rule Files (*.ifcrule.json);;All Files (*)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        session_state_->setStatusMessage("GeometryRule", "Could not read the rule file");
        return;
    }
    QJsonParseError parse_error;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parse_error);
    file.close();
    if (parse_error.error != QJsonParseError::NoError || !document.isObject()) {
        session_state_->setStatusMessage("GeometryRule", "Not a valid rule file: " + path);
        return;
    }
    QTreeWidgetItem* file_item =
        modules::rule::engineproxy::ruleFileFromJson(rule_content_->tree(), document.object());
    if (!file_item) {
        session_state_->setStatusMessage("GeometryRule", "Not a rule file document: " + path);
        return;
    }
    session_state_->setStatusMessage("GeometryRule", "Rule file loaded: " + path);
}

void MainWindow::setupRibbon() {
    auto* shell = new QFrame(this);

    auto* shell_layout = new QVBoxLayout(shell);
    shell_layout->setContentsMargins(0, 0, 0, 0);
    shell_layout->setSpacing(0);

    ribbon_tabs_ = new components::TabBar(shell);
    ribbon_tabs_->addTab("Home");
    ribbon_tabs_->addTab("Navigate");
    ribbon_tabs_->addTab("Inspect");
    ribbon_tabs_->addTab("Panels");
    ribbon_tabs_->addTab("GeometryRule");
    ribbon_tabs_->addTab("Clash Filter");
    ribbon_tabs_->setCurrentIndex(0);

    auto* ribbon_band = new QFrame(shell);
    ribbon_band->setObjectName("ribbonBand");
    auto* band_layout = new QVBoxLayout(ribbon_band);
    band_layout->setContentsMargins(0, 0, 0, 0);
    band_layout->setSpacing(0);

    ribbon_pages_ = new QStackedWidget(ribbon_band);
    ribbon_pages_->addWidget(buildHomeRibbonPage());
    ribbon_pages_->addWidget(buildNavigateRibbonPage());
    ribbon_pages_->addWidget(buildInspectRibbonPage());
    ribbon_pages_->addWidget(buildPanelsRibbonPage());
    QWidget* geometry_rule_page = buildGeometryRuleRibbonPage();
    ribbon_pages_->addWidget(geometry_rule_page);
    QWidget* clash_filter_page = buildClashFilterRibbonPage();
    ribbon_pages_->addWidget(clash_filter_page);
    clash_filter_page_index_ = ribbon_pages_->indexOf(clash_filter_page);

    band_layout->addWidget(ribbon_pages_);
    shell_layout->addWidget(ribbon_tabs_);
    shell_layout->addWidget(ribbon_band);

    connect(ribbon_tabs_, &components::TabBar::currentChanged,
            ribbon_pages_, &QStackedWidget::setCurrentIndex);

    // Workspace switching. GeometryRule and Clash Filter are full-width
    // workspaces: both hide Models / Spatial Hierarchy. GeometryRule swaps
    // the left column for Rule Library / Rule Config; Clash Filter swaps it
    // for Result / Result Info. The Rule panel lives in both workspaces
    // only (SPEC_Clash_Filter_UI.md 5).
    connect(ribbon_tabs_, &components::TabBar::currentChanged, this,
            [this, geometry_rule_page, clash_filter_page](int index) {
        const bool on_geometry_rule = ribbon_pages_->widget(index) == geometry_rule_page;
        const bool on_clash_filter = ribbon_pages_->widget(index) == clash_filter_page;
        const bool on_rule_workspace = on_geometry_rule || on_clash_filter;
        models_panel_->setVisible(!on_rule_workspace);
        spatial_panel_->setVisible(!on_rule_workspace);
        rule_library_panel_->setVisible(on_geometry_rule);
        rule_config_panel_->setVisible(on_geometry_rule);
        result_dock_->setVisible(on_clash_filter);
        result_info_dock_->setVisible(on_clash_filter);
        rule_panel_->setVisible(on_rule_workspace);
    });

    setMenuWidget(shell);
}

void MainWindow::setupViewport() {
    viewport_widget_ = new modules::viewport::ViewportPanel(this);
    setCentralWidget(viewport_widget_);
}

void MainWindow::setupPanels() {
    models_panel_ = new modules::models::ModelsPanel(
        session_state_, viewport_widget_->viewport(), this);
    spatial_panel_ = new modules::spatial_hierarchy::SpatialHierarchyPanel(this);
    properties_panel_ = new modules::properties::PropertiesPanel(this);

    models_view_ = new modules::models::ModelsPanelView(
        models_panel_, session_state_, viewport_widget_->viewport(), this);
    spatial_view_ = new modules::spatial_hierarchy::SpatialHierarchyPanelView(spatial_panel_, session_state_, this);
    properties_view_ = new modules::properties::PropertiesPanelView(properties_panel_, session_state_, this);

    layers_panel_ = new components::Panel("Layers", new modules::todo::TodoPanel("Layers", this), this);
    stored_views_panel_ = new components::Panel(
        "Stored Views", new modules::todo::TodoPanel("Stored Views", this), this);
    search_panel_ = new components::Panel(
        "Search and Query", new modules::todo::TodoPanel("Search and Query", this), this);
    spreadsheet_panel_ = new components::Panel(
        "Spreadsheet", new modules::todo::TodoPanel("Spreadsheet", this), this);
    audit_panel_ = new components::Panel("Audit", new modules::todo::TodoPanel("Audit", this), this);
    issues_panel_ = new components::Panel("Issues", new modules::todo::TodoPanel("Issues", this), this);
    // Clash Filter workspace: Result (top) and Result Info (bottom) replace
    // the old Clash dock — every run and result view flows through them.
    result_content_ = new modules::rule::ResultPanel(this);
    result_info_content_ = new modules::rule::ResultInfoPanel(this);
    result_dock_ = new components::Panel("Result", result_content_, this);
    result_info_dock_ = new components::Panel("Result Info", result_info_content_, this);
    auto* rule_content = new modules::rule::RulePanel(this);
    rule_content_ = rule_content;
    auto* rule_library_content = new modules::rule::RuleLibraryPanel(this);
    rule_library_content_ = rule_library_content;
    auto* rule_config_content = new modules::rule::RuleConfigPanel(this);
    rule_panel_ = new components::Panel("Rule", rule_content, this);
    rule_library_panel_ = new components::Panel("Rule Library", rule_library_content, this);
    rule_config_panel_ = new components::Panel("Rule Config", rule_config_content, this);
    connect(rule_library_content, &modules::rule::RuleLibraryPanel::ruleDefinitionSelected,
            rule_config_content, &modules::rule::RuleConfigPanel::showDefinition);
    connect(rule_content, &modules::rule::RulePanel::ruleNodeSelected,
            rule_config_content, &modules::rule::RuleConfigPanel::showInstance);

    // The Rule tree drives the run intents (context menus, double-click).
    connect(rule_content->tree(), &modules::rule::RuleTreeWidget::ruleRunRequested, this,
            [this](QTreeWidgetItem* item) { runGeometryRules(item); });
    connect(rule_content->tree(), &modules::rule::RuleTreeWidget::displayInputRequested, this,
            [this](QTreeWidgetItem* item) { showRuleDisplayInput(item); });
    connect(rule_content->tree(), &modules::rule::RuleTreeWidget::facetSelectionRequested, this,
            [this](QTreeWidgetItem* item) { showFacetSelection(item); });
    connect(rule_content->tree(), &modules::rule::RuleTreeWidget::ruleDoubleClicked, this,
            [this](QTreeWidgetItem* item) {
        if (modules::rule::RuleTreeWidget::isFacetItem(item)) showFacetSelection(item);
        else                                        runGeometryRules(item);
    });
    connect(rule_content, &modules::rule::RulePanel::saveRequested, this,
            [this]() { saveRuleFile(); });
    connect(rule_content, &modules::rule::RulePanel::loadRequested, this,
            [this]() { loadRuleFile(); });

    // The Result panel drives the Result Info panel and the viewport.
    connect(result_content_, &modules::rule::ResultPanel::currentResultChanged, this,
            [this](int row_index) {
        if (row_index >= 0) result_info_content_->showResultByIndex(&last_result_rows_, row_index);
        else                result_info_content_->showResult(nullptr);
    });
    connect(result_content_, &modules::rule::ResultPanel::resultShowRequested, this,
            [this](int row_index) { showResultPair(row_index); });
    connect(result_content_, &modules::rule::ResultPanel::objectsShowRequested, this,
            [this](const std::vector<uint32_t>& object_ids) {
        // "Afficher les objets": show the whole context again with the
        // selected results' objects selected.
        viewport_widget_->viewport()->showAllElements();
        viewport_widget_->viewport()->setSelectedObjectIds(object_ids);
    });

    addDockWidget(Qt::LeftDockWidgetArea, models_panel_);
    addDockWidget(Qt::LeftDockWidgetArea, spatial_panel_);
    splitDockWidget(models_panel_, spatial_panel_, Qt::Vertical);

    addDockWidget(Qt::RightDockWidgetArea, properties_panel_);
    addDockWidget(Qt::RightDockWidgetArea, layers_panel_);
    addDockWidget(Qt::RightDockWidgetArea, stored_views_panel_);
    addDockWidget(Qt::RightDockWidgetArea, search_panel_);
    addDockWidget(Qt::RightDockWidgetArea, spreadsheet_panel_);
    addDockWidget(Qt::RightDockWidgetArea, audit_panel_);
    addDockWidget(Qt::RightDockWidgetArea, issues_panel_);
    addDockWidget(Qt::RightDockWidgetArea, rule_panel_);
    splitDockWidget(properties_panel_, rule_panel_, Qt::Vertical);
    // The GeometryRule workspace swaps the left column: Rule Library sits
    // where Models was, Rule Config where Spatial Hierarchy was. The Clash
    // Filter workspace puts Result / Result Info there instead.
    addDockWidget(Qt::LeftDockWidgetArea, rule_library_panel_);
    addDockWidget(Qt::LeftDockWidgetArea, rule_config_panel_);
    splitDockWidget(rule_library_panel_, rule_config_panel_, Qt::Vertical);
    addDockWidget(Qt::LeftDockWidgetArea, result_dock_);
    addDockWidget(Qt::LeftDockWidgetArea, result_info_dock_);
    splitDockWidget(result_dock_, result_info_dock_, Qt::Vertical);

    tabifyDockWidget(properties_panel_, layers_panel_);
    tabifyDockWidget(layers_panel_, stored_views_panel_);
    tabifyDockWidget(stored_views_panel_, search_panel_);
    tabifyDockWidget(search_panel_, spreadsheet_panel_);
    tabifyDockWidget(spreadsheet_panel_, audit_panel_);
    tabifyDockWidget(audit_panel_, issues_panel_);
    properties_panel_->raise();

    layers_panel_->hide();
    stored_views_panel_->hide();
    search_panel_->hide();
    spreadsheet_panel_->hide();
    audit_panel_->hide();
    issues_panel_->hide();
    rule_panel_->hide();
    rule_library_panel_->hide();
    rule_config_panel_->hide();
    result_dock_->hide();
    result_info_dock_->hide();

    resizeDocks({models_panel_, properties_panel_}, {290, 330}, Qt::Horizontal);
    resizeDocks({models_panel_, spatial_panel_}, {280, 240}, Qt::Vertical);
}

void MainWindow::setupStatus() {
    status_mode_label_ = new QLabel("Ready", this);
    status_selection_label_ = new QLabel("No selection", this);
    status_perf_label_ = new QLabel(this);
    status_memory_label_ = new QLabel(this);
    status_memory_label_->setVisible(false);
    status_memory_label_->setToolTip(
        "The geometry in view needs more GPU memory than is available, so the "
        "viewer keeps the largest on-screen parts resident and streams the rest "
        "as you move. Right-click a model in the Models panel and choose "
        "\"Unload Model\" to free its GPU memory for the others.");
    status_progress_bar_ = new QProgressBar(this);
    status_perf_label_->setVisible(AppSettings::instance().showStats());
    status_progress_bar_->setMaximumWidth(200);
    status_progress_bar_->setVisible(false);

    statusBar()->setSizeGripEnabled(false);
    statusBar()->addWidget(status_mode_label_);
    statusBar()->addWidget(status_selection_label_, 1);
    statusBar()->addPermanentWidget(status_memory_label_);
    statusBar()->addPermanentWidget(status_perf_label_);
    statusBar()->addPermanentWidget(status_progress_bar_);

    connect(&AppSettings::instance(), &AppSettings::showStatsChanged, this, [this](bool show) {
        status_perf_label_->setVisible(show);
        if (!show) status_perf_label_->clear();
    });

    // Nav mouse preset. The WGPU_NAV_PRESET env var is a dev override applied at
    // ViewportWindow construction; otherwise apply the persisted Settings choice
    // here, and re-apply live whenever the user changes it in the dialog.
    if (!std::getenv("WGPU_NAV_PRESET")) {
        if (auto* vp = viewport_widget_->viewport())
            vp->applyNavPreset(AppSettings::navPresetName(AppSettings::instance().navPreset()));
    }
    connect(&AppSettings::instance(), &AppSettings::navPresetChanged, this,
            [this](AppSettings::NavPreset preset) {
                if (auto* vp = viewport_widget_->viewport())
                    vp->applyNavPreset(AppSettings::navPresetName(preset));
            });

    // Backface culling: apply the persisted choice and re-apply live on change.
    if (auto* vp = viewport_widget_->viewport())
        vp->setBackfaceCulling(AppSettings::instance().backfaceCulling());
    connect(&AppSettings::instance(), &AppSettings::backfaceCullingChanged, this,
            [this](bool enabled) {
                if (auto* vp = viewport_widget_->viewport())
                    vp->setBackfaceCulling(enabled);
            });

    connect(session_state_, &bonsaiviewer::SessionState::statusMessageChanged,
            this, [this](const QString& mode, const QString& detail) {
        status_mode_label_->setText(mode);
        status_selection_label_->setText(detail);
    });
    connect(session_state_, &bonsaiviewer::SessionState::progressBegan, this, [this](const QString&) {
        // Start in indeterminate mode (spinning bar). The first concrete
        // setProgress(...) call below switches it to a determinate 0-100 bar.
        status_progress_bar_->setRange(0, 0);
        status_progress_bar_->setVisible(true);
    });
    connect(session_state_, &bonsaiviewer::SessionState::progressChanged, this, [this](int percent) {
        if (status_progress_bar_->maximum() == 0) status_progress_bar_->setRange(0, 100);
        status_progress_bar_->setValue(percent);
    });
    connect(session_state_, &bonsaiviewer::SessionState::progressEnded, this, [this]() {
        status_progress_bar_->setVisible(false);
    });
    session_state_->setStatusMessage("Ready", "No selection");
}

void MainWindow::setupLoader() {
    session_state_->createLoader(viewport_widget_->viewport());
    viewport_view_ = new modules::viewport::ViewportView(
        session_state_, viewport_widget_->viewport(), this);

    // Load errors surface through SessionState as a session-level signal; the
    // status text + progress are already cleared there, we only show the modal.
    connect(session_state_, &bonsaiviewer::SessionState::loadError, this,
            [this](const QString& message) {
        QMessageBox::warning(this, "Bonsai Viewer", message);
    });

    connect(viewport_widget_->viewport(), &ViewportWindow::frameStatsUpdated, this,
            [this](const ViewportWindow::FrameStats& stats) {
        const double mb = 1.0 / (1024.0 * 1024.0);

        // Missing chunks are normal for a moment after every camera move
        // while streaming catches up; only a shortfall that persists means
        // the view does not fit, and only that is worth telling the user.
        constexpr qint64 kShortfallNoticeMs = 3000;
        if (stats.chunks_wanted_missing == 0) {
            memory_shortfall_since_.invalidate();
            status_memory_label_->setVisible(false);
        } else {
            if (!memory_shortfall_since_.isValid()) memory_shortfall_since_.start();
            if (memory_shortfall_since_.elapsed() >= kShortfallNoticeMs) {
                status_memory_label_->setText(
                    QString("GPU memory full: %1 of %2 visible chunks (%3 MB) not loaded")
                        .arg(stats.chunks_wanted_missing)
                        .arg(stats.chunks_wanted)
                        .arg(double(stats.wanted_missing_bytes) * mb, 0, 'f', 0));
                status_memory_label_->setVisible(true);
            }
        }

        if (!status_perf_label_->isVisible()) return;
        QString text =
            QString("%1 fps | %2 ms | %3/%4 obj | %5/%6 tri | %7 draws | VRAM %8/%9 MB")
                .arg(stats.fps, 0, 'f', 1)
                .arg(stats.frame_time_ms, 0, 'f', 1)
                .arg(stats.visible_objects)
                .arg(stats.total_objects)
                .arg(stats.visible_triangles)
                .arg(stats.total_triangles)
                .arg(stats.gl_draw_calls)
                .arg(double(stats.vram_used_bytes) * mb, 0, 'f', 0)
                .arg(double(stats.vram_capacity_bytes) * mb, 0, 'f', 0);
        // The budget is where the pool may grow to; the pool can also sit
        // a sub-buffer above it (a release would undershoot). Show it
        // only when it tells the user something capacity does not.
        if (stats.vram_budget_bytes > 0
            && stats.vram_budget_bytes != stats.vram_capacity_bytes) {
            text += QString(" (budget %1)")
                .arg(double(stats.vram_budget_bytes) * mb, 0, 'f', 0);
        }
        // Device total is only known when a driver backend answered.
        if (stats.device_vram_total_bytes > 0) {
            text += QString(" | Device %1/%2 MB")
                .arg(double(stats.device_vram_used_bytes) * mb, 0, 'f', 0)
                .arg(double(stats.device_vram_total_bytes) * mb, 0, 'f', 0);
        }
        if (stats.chunks_wanted_missing > 0) {
            text += QString(" | %1/%2 chunks waiting")
                .arg(stats.chunks_wanted_missing)
                .arg(stats.chunks_wanted);
        }
        status_perf_label_->setText(text);
    });
    connect(viewport_widget_->viewport(), &ViewportWindow::objectPicked,
            this, [this](uint32_t object_id) {
        session_state_->setSelectedObjectId(object_id);
        session_state_->notifySelectionChanged();
    });
}

void MainWindow::updateWindowTitle() {
    auto* federation = session_state_->federation();
    const QString project_path = federation->filePath();
    if (project_path.isEmpty() && federation->models().empty()) {
        setWindowTitle("Bonsai Viewer");
    } else if (project_path.isEmpty()) {
        setWindowTitle("untitled[*] - Bonsai Viewer");
    } else {
        setWindowTitle(QFileInfo(project_path).fileName() + "[*] - Bonsai Viewer");
    }
}

} // namespace bonsaiviewer::shell
