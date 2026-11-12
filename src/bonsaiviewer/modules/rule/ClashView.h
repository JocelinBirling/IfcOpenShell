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

#ifndef IFCINTERFACE_MODULES_RULE_CLASHVIEW_H
#define IFCINTERFACE_MODULES_RULE_CLASHVIEW_H

#include "Results.h"
#include "RuleDocument.h"
#include "RuleTree.h"

#include "../../ElementRegistry.h"
#include "../../services/clash/ObjectClashIndex.h"
#include "../../services/clash/Orchestrator.h"

#include <QHash>
#include <QMap>
#include <QObject>
#include <QPointer>

#include <atomic>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <mutex>
#include <thread>
#include <vector>

class QToolButton;
class QWidget;
class ViewportWindow;

namespace ifcclash { class Session; }
namespace bonsaiviewer { class SessionState; }
namespace bonsaiviewer::services {
class ActionPipeline;
class ActiveView;
class AppearanceService;
class IdsBridge;
class SelectionService;
class UndoStack;
}

namespace bonsaiviewer::modules::rule {

class RulePanel;
class RuleLibraryPanel;
class RuleConfigPanel;

// BonsaiViewer09 (TI09) — the controller of the GeometryRule and Clash
// Filter workspaces: runs the rules on the IfcClash_Plus engine in a worker
// thread (one engine Session for the loaded models, kept between runs),
// turns the outcomes into result rows, colours the pairs (clash layer of
// B04), drives the selection, the isolation and the overlays of the
// geometry blocks, and saves / loads the rule file.
class ClashView : public QObject {
    Q_OBJECT

public:
    struct Services {
        bonsaiviewer::SessionState& session;
        ViewportWindow& viewport;
        bonsaiviewer::services::SelectionService& selection;
        bonsaiviewer::services::ActionPipeline& pipeline;
        bonsaiviewer::services::ActiveView& view;
        bonsaiviewer::services::AppearanceService& appearance;
        bonsaiviewer::services::UndoStack& undo;
        bonsaiviewer::services::IdsBridge& ids; // the IDS editor of the Edit window
        bonsaiviewer::services::clash::ObjectClashIndex& index; // the Clash section of the Properties panel
    };

    ClashView(Services services, QWidget* host, QObject* parent = nullptr);
    ~ClashView() override;

    RuleDocument& document() { return document_; }
    RulePanel* rulePanel() const { return rule_panel_; }
    RuleLibraryPanel* libraryPanel() const { return library_; }
    RuleConfigPanel* configPanel() const { return config_; }
    ResultPanel* resultPanel() const { return results_; }
    ResultInfoPanel* resultInfoPanel() const { return info_; }
    // The ribbon content of the two workspaces (the specific top panel).
    QWidget* geometryRuleRibbon() const { return geometry_ribbon_; }
    QWidget* clashFilterRibbon() const { return filter_ribbon_; }

    // Offers to save a modified rule file; false when the user cancels.
    bool confirmDiscard();

signals:
    // A run finished with results: the shell shows the Clash Filter workspace.
    void showResultsRequested();

private:
    void run(const std::vector<clash::RuleNode>& nodes);
    void stop();
    void showSelection(const clash::Select& select, const QString& title);
    void onOutcome(const clash::RuleOutcome& outcome);
    void onRunFinished(bool cancelled, const QString& error);
    // Runs `work` on the worker thread with the engine session.
    bool startWork(const QString& label, std::function<void(ifcclash::Session&, size_t)> work);
    bool prepareModels(QString* error);

    void applyColours();
    void clearColours();
    void drawOverlay(int row);
    void clearOverlay();
    void isolateRows(const std::vector<int>& rows);
    void saveRules(); // always asks where to save (a "Save As")
    void loadRules();
    void saveResults();
    void editRule(std::shared_ptr<clash::RuleCheck> rule);
    void editFolder(std::shared_ptr<clash::RuleFolder> folder);
    // The preview of an IDS editor: EXPRESS ids of the active model -> isolate.
    void isolateExpressIds(const std::vector<int>& express_ids);
    // "Display Input": runs a rule and draws the zones and boxes it checked,
    // without touching the results, the colours or the visibility.
    void displayInput(std::shared_ptr<clash::RuleCheck> rule);
    void drawDisplay(const std::vector<std::shared_ptr<const ifcclash::ResultRecord>>& records);
    void setColourRed(bool on);
    // The Must "Test" of the Edit window: runs a copy of the rule, answers
    // "n groups: p pass, f fail" (the Result panel is not touched).
    void testRule(std::shared_ptr<clash::RuleCheck> rule, std::function<void(const QString&)> answer);
    QWidget* buildGeometryRuleRibbon();
    QWidget* buildClashFilterRibbon();

    ResultRow rowOf(const clash::RuleOutcome& outcome, const clash::ClashResult& result, const QString& group) const;
    std::vector<std::uint32_t> objectsOf(const clash::Elements& keys) const;

    Services s_;
    QWidget* host_ = nullptr;
    RuleDocument document_;
    RulePanel* rule_panel_ = nullptr;
    RuleLibraryPanel* library_ = nullptr;
    RuleConfigPanel* config_ = nullptr;
    ResultPanel* results_ = nullptr;
    ResultInfoPanel* info_ = nullptr;
    QWidget* geometry_ribbon_ = nullptr;
    QWidget* filter_ribbon_ = nullptr;
    QToolButton* run_button_ = nullptr;
    QToolButton* stop_button_ = nullptr;

    // The engine: one session for the models of the run, created and used
    // on the worker thread only.
    std::thread worker_;
    std::atomic<bool> busy_{false};
    std::unique_ptr<ifcclash::CancelToken> cancel_;
    std::shared_ptr<ifcclash::Session> engine_;
    QStringList engine_key_; // paths and mtimes of the session
    std::vector<std::uint32_t> run_models_; // file index -> session model id
    QStringList run_paths_;
    // Per session model: EXPRESS id -> element of the registry (names, object ids).
    QHash<std::uint32_t, QHash<int, bonsaiviewer::BasicElementInfo>> elements_;

    // The run in progress, then the results shown.
    std::vector<ResultRow> pending_rows_;
    std::set<QString> pending_nodes_; // the rules of the run (their old rows are replaced)
    // The elements each rule run checked (viewer object ids), for the Clash
    // section of the Properties panel ("checked, no result").
    struct RuleInputs {
        QString label, type;
        std::vector<std::uint32_t> sources, targets, not_checked;
    };
    std::map<QString, RuleInputs> rule_inputs_, pending_inputs_;
    void publishIndex();
    // A click on a rule (or folder, or the file) of the tree: its results, if it ran.
    void showResultsOf(const std::optional<TreeRef>& ref);
    QMap<QString, QString> result_texts_;
    std::vector<ResultPanel::Message> run_messages_; // node id (empty: the run), text
    std::vector<std::uint32_t> coloured_;
    std::vector<std::uint32_t> context_;
    bool always_context_ = false;
    // The geometry of the current result drawn in the 3D view, per kind
    // (ribbon RESULT GEOMETRY): all off, the results are shown as they are.
    struct OverlayKinds {
        bool points = false, zones = false, obbs = false, axes = false, faces = false, rays = false;
        bool any() const { return points || zones || obbs || axes || faces || rays; }
    } overlay_;
    bool overlay_drawn_ = false;
    bool colour_red_ = false; // GeometryRule "Color Red": every object red // the line slot holds our lines (else: left alone)
};

} // namespace bonsaiviewer::modules::rule

#endif // IFCINTERFACE_MODULES_RULE_CLASHVIEW_H
