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

#ifndef IFCINTERFACE_PANELS_PROPERTIESPANELVIEW_H
#define IFCINTERFACE_PANELS_PROPERTIESPANELVIEW_H

#include "Types.h"

#include <QList>
#include <QObject>
#include <QPoint>
#include <QString>

#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace bonsaiviewer { class SessionState; }
namespace bonsaiviewer::services {
class ActionPipeline;
class EntitySetStore;
class IfcDataAccess;
class SelectionService;
}
namespace bonsaiviewer::modules::table { class TablePanelView; }

namespace bonsaiviewer::modules::properties {

class PropertiesPanel;

// TI14 — Select By Property: the right-click functions of the Properties panel.
// On a value: Copy the value, Isolate similar, Select similar. On a property
// name: Isolate the property, Colour by property, Colour the box by property.
class PropertiesPanelView : public QObject {
    Q_OBJECT
public:
    PropertiesPanelView(PropertiesPanel* widget,
                        bonsaiviewer::SessionState* session_state,
                        bonsaiviewer::services::IfcDataAccess& data_access,
                        bonsaiviewer::services::ActionPipeline& actions,
                        bonsaiviewer::services::SelectionService& selection,
                        bonsaiviewer::services::EntitySetStore& box,
                        QObject* parent = nullptr);

    // "Colour by property" shows its value table in the Table panel.
    void setTableView(table::TablePanelView* table_view) { table_view_ = table_view; }

private:
    // The entity shown in the panel: a model entity (session_model_id +
    // ifc_id), and the object id when it comes from the viewport selection.
    struct Shown {
        std::uint32_t session_model_id = 0;
        int ifc_id = 0;
        std::uint32_t object_id = 0;
    };

    // A viewport selection: shows the object and empties the Back history.
    void refresh(uint32_t object_id);
    // Shows an entity of a model (with or without geometry), keeping the
    // current one in the Back history (BonsaiViewer14).
    void open(std::uint32_t session_model_id, int ifc_id);
    void goBack();
    void render();
    QString modelName(std::uint32_t session_model_id) const;
    // Right click on a Relationships row: Isolate the related elements.
    void showRelationshipMenu(int row, const QPoint& pos);
    // The object ids (with geometry) reached through a Relationships row:
    // the whole model, the occurrences of a type, the content of a spatial
    // element, or the members of a group.
    std::vector<std::uint32_t> relatedObjectIds(const RelationshipRow& row) const;
    void showValueMenu(const QString& pset, const QString& property, const QString& value, const QPoint& pos);
    void showNameMenu(const QString& pset, const QString& property, const QPoint& pos);
    // Every element of the scope (all the loaded models, or the box) with its
    // value of pset.property; nullopt when the element does not have it.
    std::vector<std::pair<std::uint32_t, std::optional<QString>>> scan(const QString& pset,
                                                                       const QString& property,
                                                                       bool box_only) const;
    // Elements with the property are grouped by value (one colour each, grey
    // for an empty value); elements without it are hidden. Shown in the Table panel.
    void colourByProperty(const QString& pset, const QString& property, bool box_only);

    PropertiesPanel* widget_ = nullptr;
    bonsaiviewer::SessionState* session_state_ = nullptr;
    bonsaiviewer::services::IfcDataAccess& data_access_;
    bonsaiviewer::services::ActionPipeline& actions_;
    bonsaiviewer::services::SelectionService& selection_;
    bonsaiviewer::services::EntitySetStore& box_;
    table::TablePanelView* table_view_ = nullptr;
    Shown shown_;
    QList<Shown> history_;
    QList<RelationshipRow> relationships_;  // as rendered, for the link clicks
};

} // namespace bonsaiviewer::modules::properties

#endif
