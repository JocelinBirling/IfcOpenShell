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

#ifndef IFCINTERFACE_MODULES_UNIFIED_TREE_VIEW_H
#define IFCINTERFACE_MODULES_UNIFIED_TREE_VIEW_H

#include "../../services/UnifiedTreeBuilder.h"

#include <QObject>
#include <QString>

#include <cstdint>

class QTreeWidgetItem;
class ViewportWindow;

namespace bonsaiviewer { class SessionState; }
namespace bonsaiviewer::services {
class ActionPipeline;
class SelectionService;
}

namespace bonsaiviewer::modules::unified_tree {

class UnifiedTreePanel;

// BonsaiViewer13 — the Unified Tree view. Settings (reference models,
// tolerance, merges) live in memory; Run collects the storeys, rooms and
// elements of the open models (IFC data source + viewport AABBs), runs the
// UnifiedTreeBuilder and shows the result. The index is not saved.
class UnifiedTreeView : public QObject {
    Q_OBJECT
public:
    UnifiedTreeView(UnifiedTreePanel* panel,
                    bonsaiviewer::SessionState& session,
                    ViewportWindow& viewport,
                    bonsaiviewer::services::SelectionService& selection,
                    bonsaiviewer::services::ActionPipeline& actions,
                    QObject* parent = nullptr);

    // BonsaiViewer32: the Unified tree of the Trees panels shows the same
    // computation (settings, Run, result) as this panel.
    const services::unified::Result& result() const { return result_; }
    QString status() const { return status_; }
    bool isRunning() const { return running_; }
    void openSettings();
    void run();

signals:
    void resultChanged();
    void statusChanged();

private:
    void setStatus(const QString& text);
    void clear(const QString& status);
    // Collects the input of the builder; false (with `error`) when a model
    // cannot be read.
    bool collect(services::unified::Input& input, QString& error) const;
    QString modelName(std::uint32_t session_model_id) const;
    // The model with the most IfcSpace, the default reference.
    std::uint32_t defaultReferenceModel() const;

    void populate();
    QTreeWidgetItem* makeItem(int node) const;
    void fillChildren(QTreeWidgetItem* item);
    int nodeOf(const QTreeWidgetItem* item) const;
    void showMenu(const QPoint& pos);

    UnifiedTreePanel* panel_ = nullptr;
    bonsaiviewer::SessionState& session_;
    ViewportWindow& viewport_;
    bonsaiviewer::services::SelectionService& selection_;
    bonsaiviewer::services::ActionPipeline& actions_;
    services::unified::Settings settings_;
    bool settings_chosen_ = false;
    services::unified::Result result_;
    QString status_ = QStringLiteral("Not computed. Choose the settings, then Run.");
    bool running_ = false;
};

} // namespace bonsaiviewer::modules::unified_tree

#endif
