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

#include <QElapsedTimer>
#include <QHash>
#include <QMainWindow>
#include <QStringList>
#include <thread>
#include <vector>

#include "modules/rule/ResultPanel.h"
#include "modules/rule/engineproxy/EngineClient.h"

class QLabel;
class QDockWidget;
class QMenu;
class QProgressBar;
class QStackedWidget;
class QToolButton;
class QTreeWidgetItem;
namespace bonsaiviewer { class SessionState; }
namespace bonsaiviewer { class ElementRegistry; }
namespace bonsaiviewer::components { class TabBar; }
namespace bonsaiviewer::modules::models { class ModelsPanel; }
namespace bonsaiviewer::modules::models { class ModelsPanelView; }
namespace bonsaiviewer::modules::spatial_hierarchy { class SpatialHierarchyPanel; }
namespace bonsaiviewer::modules::spatial_hierarchy { class SpatialHierarchyPanelView; }
namespace bonsaiviewer::modules::properties { class PropertiesPanel; }
namespace bonsaiviewer::modules::properties { class PropertiesPanelView; }
namespace bonsaiviewer::modules::rule { class RulePanel; }
namespace bonsaiviewer::modules::rule { class RuleLibraryPanel; }
namespace bonsaiviewer::modules::rule { class ResultInfoPanel; }
namespace bonsaiviewer::modules::viewport { class ViewportView; }
namespace bonsaiviewer::modules::viewport { class ViewportPanel; }

namespace bonsaiviewer::shell {

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private:
    void setupChrome();
    void setupRibbon();
    void setupViewport();
    void setupPanels();
    void setupStatus();
    void setupLoader();
    QWidget* buildHomeRibbonPage();
    QWidget* buildNavigateRibbonPage();
    QWidget* buildInspectRibbonPage();
    QWidget* buildPanelsRibbonPage();
    QWidget* buildGeometryRuleRibbonPage();
    QWidget* buildClashFilterRibbonPage();
    // The engine IPC wiring: lifecycle, catalogue, run and display events.
    void setupEngineConnections();
    // Serializes the subtree under `root` (the whole open file when null)
    // to the engine's RuleFile JSON and sends it as a run (or a preview:
    // display input only) over the models of the session. The results
    // stream in through the ruleResult signal into finishGeometryRuleRun().
    void runGeometryRules(QTreeWidgetItem* root = nullptr, bool preview = false);
    void finishGeometryRuleRun(bool cancelled);
    // One rule_result event of a run: maps the engine's cross-references
    // to viewport objects and appends the Result rows.
    void collectEngineResults(const QString& rule_type,
                              const std::vector<modules::rule::engineproxy::EngineResult>& results);
    // One display event: the engine's drawing items as overlay lines.
    void showEngineDisplay(const QString& rule_type,
                           const std::vector<modules::rule::engineproxy::DisplayItem>& items);
    // The select reply of a facet preview: colors the matching elements.
    void showFacetSelectionResult(
        const std::vector<modules::rule::engineproxy::ElementRef>& elements);
    // Right-click on a rule: draws its detection zones and object OBBs as
    // overlay lines (the engine's display contract).
    void showRuleDisplayInput(QTreeWidgetItem* rule_item);
    // Right-click / double-click on a facet: colors its matching elements.
    void showFacetSelection(QTreeWidgetItem* facet_item);
    // Save / Load of the open RuleFile (serialization.py JSON format).
    void saveRuleFile();
    void loadRuleFile();
    // Double-click on a result row: isolate the pair (with the saved
    // context when "Always Display Context" is on) and zoom to it.
    void showResultPair(int row_index);
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
    QStackedWidget* ribbon_pages_ = nullptr;
    bonsaiviewer::modules::viewport::ViewportPanel* viewport_widget_ = nullptr;
    bonsaiviewer::modules::viewport::ViewportView* viewport_view_ = nullptr;
    bonsaiviewer::SessionState* session_state_ = nullptr;
    bonsaiviewer::modules::models::ModelsPanel* models_panel_ = nullptr;
    bonsaiviewer::modules::spatial_hierarchy::SpatialHierarchyPanel* spatial_panel_ = nullptr;
    QDockWidget* layers_panel_ = nullptr;
    bonsaiviewer::modules::properties::PropertiesPanel* properties_panel_ = nullptr;
    QDockWidget* stored_views_panel_ = nullptr;
    QDockWidget* search_panel_ = nullptr;
    QDockWidget* spreadsheet_panel_ = nullptr;
    QDockWidget* audit_panel_ = nullptr;
    // Clash Filter workspace: the Result / Result Info docks (left column)
    // and the viewer context ("Context View" ribbon group).
    QDockWidget* result_dock_ = nullptr;
    QDockWidget* result_info_dock_ = nullptr;
    modules::rule::ResultPanel* result_content_ = nullptr;
    modules::rule::ResultInfoPanel* result_info_content_ = nullptr;
    std::vector<modules::rule::ResultRow> last_result_rows_;
    std::vector<uint32_t> context_ids_;
    bool context_saved_ = false;
    bool always_display_context_ = false;
    int clash_filter_page_index_ = -1;
    QDockWidget* issues_panel_ = nullptr;
    QDockWidget* rule_panel_ = nullptr;
    QDockWidget* rule_library_panel_ = nullptr;
    modules::rule::RuleLibraryPanel* rule_library_content_ = nullptr;
    QDockWidget* rule_config_panel_ = nullptr;
    // The Rule tree's owning panel; the run path walks its tree to build specs.
    modules::rule::RulePanel* rule_content_ = nullptr;
    QToolButton* run_rule_button_ = nullptr;
    // The rule engine proxy: one persistent Python process (JSON Lines).
    modules::rule::engineproxy::EngineClient* engine_client_ = nullptr;
    // Model index of the current run -> session model id (the engine
    // references the results by their index in list_ifc_path).
    std::vector<uint32_t> run_model_ids_;
    // The objects colored at the end of the run: sources red, targets orange.
    std::vector<uint32_t> run_source_objects_;
    std::vector<uint32_t> run_target_objects_;
    bool run_is_preview_ = false;
    // Models of the current run whose IFC file was not open yet; reported in
    // the run's completion message.
    int rule_run_skipped_models_ = 0;
    bonsaiviewer::modules::models::ModelsPanelView* models_view_ = nullptr;
    bonsaiviewer::modules::spatial_hierarchy::SpatialHierarchyPanelView* spatial_view_ = nullptr;
    bonsaiviewer::modules::properties::PropertiesPanelView* properties_view_ = nullptr;
};

} // namespace bonsaiviewer::shell

#endif
