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

#ifndef IFCINTERFACE_MODULES_DIFF_VIEW_H
#define IFCINTERFACE_MODULES_DIFF_VIEW_H

#include "Panel.h"
#include "../../services/IfcDiff/IfcDiff.h"

#include <QHash>
#include <QObject>
#include <QPointer>

#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

namespace bonsaiviewer { class SessionState; }
namespace bonsaiviewer::components { class Panel; }
namespace bonsaiviewer::services {
class ActionPipeline;
class AppearanceService;
class PropertyCatalog;
class SelectionService;
class UndoStack;
namespace ifcdiff { class DiffMarks; }
}

namespace bonsaiviewer::modules::diff {

class ReportDialog;

// BonsaiViewer34 — drives the IfcDiff panel: runs the C++ IfcDiff on the
// Before and After models (the IFC files the viewer has open), colours the
// result in the viewport (AppearanceService diff layer, undo state stored
// first), publishes the property changes for the Properties panel, and
// serves the selection buttons, the report and the element comparison.
// The result and its colours are dropped when a model is loaded or removed.
class DiffPanelView : public QObject {
    Q_OBJECT

public:
    DiffPanelView(bonsaiviewer::components::Panel* panel,
                  bonsaiviewer::SessionState& session,
                  bonsaiviewer::services::SelectionService& selection,
                  bonsaiviewer::services::ActionPipeline& pipeline,
                  bonsaiviewer::services::AppearanceService& appearance,
                  bonsaiviewer::services::UndoStack& undo,
                  bonsaiviewer::services::PropertyCatalog& catalog,
                  bonsaiviewer::services::ifcdiff::DiffMarks& marks,
                  QObject* parent = nullptr);

private:
    void refreshModels();
    QString modelName(std::uint32_t session_model_id) const;
    void run();
    // Drops the result: colours, Properties panel marks, report.
    void dropResult();
    void exportJson();

    // Viewport colours (spec 34 table) and the transparency of one side of
    // the modified elements. The callers store the undo state.
    void applyColours();
    void clearColours();
    void swapTransparency();
    void publishMarks(bool on);

    void select(SelectKind kind, bool isolate);
    void selectGeometry(GeometryKind kind, bool isolate);
    void selectCase(DiffCase kind, bool isolate);
    void showReport();
    void compare(const QString& global_id);
    void compareSelection();

    // (EXPRESS id -> object id) of the elements with geometry of a model.
    QHash<int, std::uint32_t> objectIdsOf(std::uint32_t session_model_id) const;
    // The object ids of a changed element in both models.
    std::vector<std::uint32_t> objectsOf(const bonsaiviewer::services::ifcdiff::ElementChange& change) const;
    // Runs `apply` with one undo state stored before it, the display services
    // recording nothing meanwhile.
    void withUndo(const std::function<void()>& apply);

    DiffPanel* panel_ = nullptr;
    bonsaiviewer::SessionState& session_;
    bonsaiviewer::services::SelectionService& selection_;
    bonsaiviewer::services::ActionPipeline& pipeline_;
    bonsaiviewer::services::AppearanceService& appearance_;
    bonsaiviewer::services::UndoStack& undo_;
    bonsaiviewer::services::ifcdiff::DiffMarks& marks_;
    QPointer<ReportDialog> report_;

    // The last run.
    std::optional<bonsaiviewer::services::ifcdiff::Result> result_;
    std::uint32_t before_model_ = 0;
    std::uint32_t after_model_ = 0;
    QHash<int, std::uint32_t> before_objects_;
    QHash<int, std::uint32_t> after_objects_;
    std::vector<std::uint32_t> deleted_ids_;
    std::vector<std::uint32_t> added_ids_;
    std::vector<std::uint32_t> modified_before_ids_;
    std::vector<std::uint32_t> modified_after_ids_;
    bool before_transparent_ = false; // default: the After side at 50 %
    bool running_ = false;
};

} // namespace bonsaiviewer::modules::diff

#endif // IFCINTERFACE_MODULES_DIFF_VIEW_H
