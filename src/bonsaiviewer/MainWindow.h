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

#ifndef IFCINTERFACE_SHELL_MAINWINDOW_H
#define IFCINTERFACE_SHELL_MAINWINDOW_H

#include <QHash>
#include <QMainWindow>
#include <QStringList>

class QLabel;
#include <QElapsedTimer>
class QDockWidget;
class QMenu;
class QProgressBar;
class QStackedWidget;
class QToolButton;
class QVBoxLayout;
namespace bonsaiviewer { class SessionState; }
namespace bonsaiviewer::components { class TabBar; }
namespace bonsaiviewer::components { class GenericTopPanel; }
namespace bonsaiviewer::modules::models { class ModelsPanel; }
namespace bonsaiviewer::modules::models { class ModelsPanelView; }
namespace bonsaiviewer::modules::spatial_hierarchy { class SpatialHierarchyPanel; }
namespace bonsaiviewer::modules::spatial_hierarchy { class SpatialHierarchyPanelView; }
namespace bonsaiviewer::modules::properties { class PropertiesPanel; }
namespace bonsaiviewer::modules::properties { class PropertiesPanelView; }
namespace bonsaiviewer::modules::text_digger { class TextDiggerPanel; }
namespace bonsaiviewer::modules::text_digger { class TextDiggerView; }
namespace bonsaiviewer::modules::viewport { class ViewportView; }
namespace bonsaiviewer::modules::viewport { class ViewportPanel; }
namespace bonsaiviewer::modules::easy_load { class EasyLoadView; }
namespace bonsaiviewer::services { class ActiveView; }
namespace bonsaiviewer::services { class ActionPipeline; }
namespace bonsaiviewer::services { class AppearanceService; }
namespace bonsaiviewer::services { class CameraService; }
namespace bonsaiviewer::services { class CategoryOverrideFilter; }
namespace bonsaiviewer::services { class EntitySetStore; }
namespace bonsaiviewer::services { class FilterRunner; }
namespace bonsaiviewer::services { class FilterStore; }
namespace bonsaiviewer::services { class IdsBridge; }
namespace bonsaiviewer::services { class IfcDataAccess; }
namespace bonsaiviewer::services { class PanelManager; }
namespace bonsaiviewer::services { class PickingService; }
namespace bonsaiviewer::services { class PropertyCatalog; }
namespace bonsaiviewer::services { class PropertyIndex; }
namespace bonsaiviewer::services { class QueryEngine; }
namespace bonsaiviewer::services { class SectionPlaneManager; }
namespace bonsaiviewer::services { class SelectionService; }
namespace bonsaiviewer::services { class UndoStack; }
namespace bonsaiviewer::modules::filters { class FiltersPanel; }
namespace bonsaiviewer::modules::filters { class FiltersPanelView; }
namespace bonsaiviewer::modules::menubar { class MenuBar; }
namespace bonsaiviewer::modules::table { class TablePanel; }
namespace bonsaiviewer::modules::table { class TablePanelView; }
namespace bonsaiviewer::modules::table { class TableStore; }
namespace bonsaiviewer::modules::table { class TablesPanel; }
namespace bonsaiviewer::modules::table { class TablesView; }
namespace bonsaiviewer::components { class Panel; }
class QTreeWidget;

namespace bonsaiviewer::shell {

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    void setupChrome();
    void setupRibbon();
    void setupViewport();
    void setupPanels();
    void setupStatus();
    void setupLoader();
    void setupMenuBar();
    // TI21 phase 1: the workspace tabs (Home, Filter, Table) and their
    // default panel layouts.
    void setupWorkspaces();
    void applyWorkspaceDefaults(int index);
    void onWorkspaceChanged(int index);
    // TI05: fill the generic top panel (search bar neighbours: display,
    // box, zoom, colour groups).
    void populateGenericTopPanel(components::GenericTopPanel* panel);
    // TI02: viewport-level interactions the shell owns — the Perpendicular
    // View right click and the section gizmo right-click delete menu.
    bool eventFilter(QObject* watched, QEvent* event) override;
    QWidget* buildInspectRibbonPage();
    void updateWindowTitle();
    QToolButton* makePanelToggle(const QString& text, QDockWidget* dock);
    // Rebuilds `menu` from the persisted recent-projects list. Wired to the
    // menu's aboutToShow so it always reflects the current MRU state.
    void populateRecentMenu(QMenu* menu);

private:
    QLabel* status_mode_label_ = nullptr;
    QLabel* status_selection_label_ = nullptr;
    QLabel* status_perf_label_ = nullptr;
    // Shown while the visible geometry persistently exceeds what fits in
    // GPU memory (see onFrameStats): the user's cue to unload models.
    QLabel* status_memory_label_ = nullptr;
    QElapsedTimer memory_shortfall_since_;
    QProgressBar* status_progress_bar_ = nullptr;
    bonsaiviewer::components::TabBar* ribbon_tabs_ = nullptr;
    // TI21 phase 1: the PanelManager tab id of each workspace (Home, Filter,
    // Table, in ribbon order) and the ones whose default layout is applied.
    QStringList workspace_ids_;
    // Layout of the central area: generic panel on top, 3D view below.
    QVBoxLayout* viewport_area_layout_ = nullptr;
    QStringList workspace_defaults_applied_;
    QStackedWidget* ribbon_pages_ = nullptr;
    bonsaiviewer::modules::viewport::ViewportPanel* viewport_widget_ = nullptr;
    // BonsaiViewer22: drag and drop of IFC files onto the viewport panel.
    bonsaiviewer::modules::easy_load::EasyLoadView* easy_load_view_ = nullptr;
    bonsaiviewer::modules::viewport::ViewportView* viewport_view_ = nullptr;
    bonsaiviewer::SessionState* session_state_ = nullptr;
    // TI00 phase 1 services (B01/B02/B03/B05), created in setupLoader once
    // the viewport and the element registry exist.
    bonsaiviewer::services::SelectionService* selection_service_ = nullptr;
    bonsaiviewer::services::CategoryOverrideFilter* category_filter_ = nullptr;
    bonsaiviewer::services::ActiveView* active_view_ = nullptr;
    bonsaiviewer::services::PropertyIndex* property_index_ = nullptr;
    // TI02: the display-function services (B04 colour, B10 fit view and
    // perpendicular framing, B09 picking, B11 section toggles).
    bonsaiviewer::services::IfcDataAccess* data_access_ = nullptr;
    bonsaiviewer::services::AppearanceService* appearance_service_ = nullptr;
    // BonsaiViewer27: CTRL+Z over the display state (visibility and colours).
    bonsaiviewer::services::UndoStack* undo_stack_ = nullptr;
    bonsaiviewer::services::CameraService* camera_service_ = nullptr;
    bonsaiviewer::services::PickingService* picking_service_ = nullptr;
    bonsaiviewer::services::SectionPlaneManager* section_manager_ = nullptr;
    // TI03 search bar services: B06 parses and evaluates, B07 carries the
    // -box scope, B08 executes the isolate / add / remove actions.
    bonsaiviewer::services::QueryEngine* query_engine_ = nullptr;
    bonsaiviewer::services::EntitySetStore* entity_set_store_ = nullptr;
    bonsaiviewer::services::ActionPipeline* action_pipeline_ = nullptr;
    // TI06 filter rules: the IDS engine bridge (IfcTester worker), the
    // persisted filter tree, and the layer-semantics executor.
    bonsaiviewer::services::IdsBridge* ids_bridge_ = nullptr;
    bonsaiviewer::services::FilterStore* filter_store_ = nullptr;
    bonsaiviewer::services::FilterRunner* filter_runner_ = nullptr;
    // TI02 Perpendicular View: armed on the button, consumed by the
    // first right click in the viewport.
    bool perpendicular_mode_ = false;
    // TI01: B13 owns the Window menu; the menu bar (TI01) feeds it the
    // hand-rolled docks and rebuilds on registration.
    bonsaiviewer::services::PanelManager* panel_manager_ = nullptr;
    bonsaiviewer::modules::menubar::MenuBar* menu_bar_ = nullptr;
    bonsaiviewer::modules::models::ModelsPanel* models_panel_ = nullptr;
    bonsaiviewer::modules::spatial_hierarchy::SpatialHierarchyPanel* spatial_panel_ = nullptr;
    QDockWidget* layers_panel_ = nullptr;
    bonsaiviewer::modules::properties::PropertiesPanel* properties_panel_ = nullptr;
    QDockWidget* stored_views_panel_ = nullptr;
    QDockWidget* search_panel_ = nullptr;
    bonsaiviewer::modules::filters::FiltersPanel* filters_panel_ = nullptr;
    bonsaiviewer::modules::table::TablePanel* table_panel_ = nullptr;
    bonsaiviewer::modules::table::TablesPanel* tables_panel_ = nullptr;
    // TI12: the Ifc Class Tree, tabbed with the Spatial Hierarchy.
    bonsaiviewer::components::Panel* class_tree_panel_ = nullptr;
    QTreeWidget* class_tree_widget_ = nullptr;
    QDockWidget* audit_panel_ = nullptr;
    QDockWidget* clash_panel_ = nullptr;
    QDockWidget* issues_panel_ = nullptr;
    bonsaiviewer::modules::models::ModelsPanelView* models_view_ = nullptr;
    bonsaiviewer::modules::spatial_hierarchy::SpatialHierarchyPanelView* spatial_view_ = nullptr;
    bonsaiviewer::modules::properties::PropertiesPanelView* properties_view_ = nullptr;
    bonsaiviewer::modules::text_digger::TextDiggerPanel* text_digger_panel_ = nullptr;
    bonsaiviewer::modules::text_digger::TextDiggerView* text_digger_view_ = nullptr;
    bonsaiviewer::modules::filters::FiltersPanelView* filters_view_ = nullptr;
    bonsaiviewer::services::PropertyCatalog* property_catalog_ = nullptr;
    bonsaiviewer::modules::table::TablePanelView* table_view_ = nullptr;
    // TI07 phase 2: the saved tables (folders and configurations) and their tree.
    bonsaiviewer::modules::table::TableStore* table_store_ = nullptr;
    bonsaiviewer::modules::table::TablesView* tables_view_ = nullptr;
};

} // namespace bonsaiviewer::shell

#endif
