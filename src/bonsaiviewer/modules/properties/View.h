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
    void refresh(uint32_t object_id);
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
};

} // namespace bonsaiviewer::modules::properties

#endif
