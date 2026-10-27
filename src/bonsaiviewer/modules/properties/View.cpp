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

#include "View.h"

#include "Panel.h"

#include "../../ElementRegistry.h"
#include "../../SessionState.h"
#include "../../services/ActionPipeline.h"
#include "../../services/ContextMenuBuilder.h"
#include "../../services/EntitySetStore.h"
#include "../../services/IfcDataAccess.h"
#include "../../services/SelectionService.h"
#include "../table/Table.h"
#include "../table/View.h"

#include "element.h"  // helpers: get_predefined_type, get_type, get_container
#include "pset.h"     // helpers: get_psets

#include <QClipboard>
#include <QColor>
#include <QGuiApplication>
#include <QMap>

#include <cstdint>
#include <optional>
#include <unordered_map>
#include <utility>

namespace bonsaiviewer::modules::properties {

namespace {

// A property/quantity value formatted for a single-line cell. std::nullopt for
// IFC null and compound values (entity references, lists, maps), which are
// omitted from the flat property table.
std::optional<QString> formatPropertyValue(const property_value& value) {
    if (const auto* text = value.get_if<std::string>()) {
        return QString::fromStdString(*text);
    }
    if (const auto* flag = value.get_if<bool>()) {
        return *flag ? QStringLiteral("True") : QStringLiteral("False");
    }
    if (const auto* integer = value.get_if<std::int64_t>()) {
        return QString::number(static_cast<qlonglong>(*integer));
    }
    if (const auto* real = value.get_if<double>()) {
        return QString::number(*real);
    }
    return std::nullopt;
}

// element_properties (set name -> {property name -> value}) into the panel's
// PropertySet list, dropping the internal "id" key and non-scalar values, and
// omitting sets that end up empty.
QList<PropertySet> toPropertySets(const element_properties& sets) {
    QList<PropertySet> result;
    for (const auto& [set_name, properties] : sets) {
        PropertySet set;
        set.title = QString::fromStdString(set_name);
        for (const auto& [property_name, value] : properties) {
            if (property_name == "id") {
                continue;  // definition instance id, not a real property
            }
            if (auto formatted = formatPropertyValue(value)) {
                set.rows.append({QString::fromStdString(property_name), *formatted});
            }
        }
        if (!set.rows.isEmpty()) {
            result.append(set);
        }
    }
    return result;
}

// The value of pset.property as shown in the panel (an empty string for a
// compound value), or nullopt when the sets do not have that property.
std::optional<QString> findValue(const element_properties& sets, const QString& pset, const QString& property) {
    for (const auto& [set_name, properties] : sets) {
        if (QString::fromStdString(set_name) != pset) continue;
        for (const auto& [property_name, value] : properties) {
            if (property_name == "id") continue;
            if (QString::fromStdString(property_name) == property) {
                return formatPropertyValue(value).value_or(QString());
            }
        }
    }
    return std::nullopt;
}

// Distinct, readable colours for the value groups (hue steps spread the first ones).
QColor groupColour(int index) {
    return QColor::fromHsv((index * 137) % 360, 170, 235);
}

} // namespace

PropertiesPanelView::PropertiesPanelView(PropertiesPanel* widget,
                                         bonsaiviewer::SessionState* session_state,
                                         services::IfcDataAccess& data_access,
                                         services::ActionPipeline& actions,
                                         services::SelectionService& selection,
                                         services::EntitySetStore& box,
                                         QObject* parent)
    : QObject(parent),
      widget_(widget),
      session_state_(session_state),
      data_access_(data_access),
      actions_(actions),
      selection_(selection),
      box_(box)
{
    connect(session_state_, &bonsaiviewer::SessionState::selectionChanged, this, [this](uint32_t object_id) {
        // A deselect (click on empty space → object_id 0) leaves the panel
        // showing the last active object rather than resetting to the empty
        // placeholder. Project reset/open below still clear it explicitly.
        if (object_id == 0) return;
        refresh(object_id);
    });
    connect(session_state_, &bonsaiviewer::SessionState::projectReset, this, [this]() {
        refresh(0);
    });
    connect(session_state_, &bonsaiviewer::SessionState::projectOpened, this, [this](const QString&) {
        refresh(0);
    });
    connect(widget_, &PropertiesPanel::propertyValueContextRequested, this,
            [this](const QString& pset, const QString& property, const QString& value, const QPoint& pos) {
                showValueMenu(pset, property, value, pos);
            });
    connect(widget_, &PropertiesPanel::propertyNameContextRequested, this,
            [this](const QString& pset, const QString& property, const QPoint& pos) {
                showNameMenu(pset, property, pos);
            });
    refresh(0);
}

void PropertiesPanelView::refresh(uint32_t object_id) {
    auto* registry = session_state_->elementRegistry();

    // Empty default: nothing selected → "No item selected" with empty sections.
    // Real data is filled in below when an object resolves.
    PropertiesPanelState state;
    state.entity = {"No item selected", ""};

    auto entity = registry ? registry->findEntity(object_id)
                           : std::optional<express::base>{};
    if (entity) {
        state.entity.entity_class = QString::fromStdString(entity->declaration().name());
        if (auto predefined_type = get_predefined_type(*entity)) {
            state.entity.predefined_type = QString::fromStdString(*predefined_type);
        }
        // Direct EXPRESS attributes, primitives only — lists / entity refs omitted.
        for (const auto& [name, value] : get_scalar_attributes(*entity)) {
            state.attributes.append({QString::fromStdString(name), QString::fromStdString(value)});
        }
        // Relationships: the construction type and the spatial container, shown
        // by name (falling back to the entity class when unnamed).
        auto display_name = [](const express::base& related) -> QString {
            if (auto name = get_string_attribute(related, "Name"); name && !name->empty()) {
                return QString::fromStdString(*name);
            }
            return QString::fromStdString(related.declaration().name());
        };
        if (express::base type = get_type(*entity)) {
            state.relationships.append({"Type", display_name(type)});
        }
        if (express::base container = get_container(*entity)) {
            state.relationships.append({"Container", display_name(container)});
        }
        // Property sets (Pset_*) and quantity sets (Qto_* / BaseQuantities),
        // occurrence values inheriting from the type.
        state.property_sets = toPropertySets(get_psets(*entity, /*psets_only=*/true, /*qtos_only=*/false));
        state.quantity_sets = toPropertySets(get_psets(*entity, /*psets_only=*/false, /*qtos_only=*/true));
    } else if (registry) {
        // No live IFC source for this object — typical when a pure-geometry
        // .ifcview was loaded without its .ifc/.rdb sibling.  Fall back
        // to the basic info cached in the element registry so the panel still
        // shows class / GlobalId / Name for visible elements.
        auto info = registry->findBasicElementInfo(object_id);
        if (info && !info->type.isEmpty()) {
            state.entity.entity_class = info->type;
            // Geometry only — no live IFC entity to read a predefined type from.
            state.entity.predefined_type = "N/A";
        }
        if (info && !info->guid.isEmpty()) {
            state.attributes.append({"GlobalId", info->guid});
        }
        if (info && !info->name.isEmpty()) {
            state.attributes.append({"Name", info->name});
        }
    }
    widget_->render(state);
}

std::vector<std::pair<std::uint32_t, std::optional<QString>>> PropertiesPanelView::scan(
    const QString& pset, const QString& property, bool box_only) const {
    std::vector<std::uint32_t> scope;
    if (box_only) {
        scope = box_.current();
    } else {
        for (std::uint32_t session_model_id : session_state_->sessionModelIds()) {
            for (const auto& info : session_state_->elementRegistry()->basicElementInfoForModel(session_model_id)) {
                scope.push_back(info.object_id);
            }
        }
    }
    std::vector<std::pair<std::uint32_t, std::optional<QString>>> result;
    result.reserve(scope.size());
    for (std::uint32_t object_id : scope) {
        // Synchronous read: one entity per element of the scope.
        std::optional<QString> value = findValue(data_access_.propertySets(object_id), pset, property);
        if (!value) value = findValue(data_access_.quantitySets(object_id), pset, property);
        result.emplace_back(object_id, std::move(value));
    }
    return result;
}

void PropertiesPanelView::showValueMenu(const QString& pset, const QString& property,
                                        const QString& value, const QPoint& pos) {
    // The elements (all the loaded models) whose value of pset.property is the same text.
    const auto similar = [this, pset, property, value] {
        std::vector<std::uint32_t> ids;
        for (const auto& [object_id, found] : scan(pset, property, false)) {
            if (found && *found == value) ids.push_back(object_id);
        }
        return ids;
    };

    services::ContextMenuBuilder menu;
    menu.addItem("copy",
                 "Copy the value",
                 [value] { QGuiApplication::clipboard()->setText(value); });
    menu.addItem("isolate_similar",
                 "Isolate similar",
                 [this, similar] {
                     const auto ids = similar();
                     if (!ids.empty()) actions_.run(ids, services::ActionPipeline::Action::Isolate);
                 });
    menu.addItem("select_similar",
                 "Select similar",
                 [this, similar] {
                     selection_.setSelection(similar(), services::SelectionSource::Service);
                 });
    menu.exec(pos, widget_);
}

void PropertiesPanelView::showNameMenu(const QString& pset, const QString& property, const QPoint& pos) {
    services::ContextMenuBuilder menu;
    menu.addItem("isolate_property",
                 "Isolate the property",
                 [this, pset, property] {
                     std::vector<std::uint32_t> ids;
                     for (const auto& [object_id, found] : scan(pset, property, false)) {
                         if (found) ids.push_back(object_id);
                     }
                     if (!ids.empty()) actions_.run(ids, services::ActionPipeline::Action::Isolate);
                 });
    menu.addItem("colour_by_property",
                 "Colour by property",
                 [this, pset, property] { colourByProperty(pset, property, false); });
    menu.addItem("colour_box_by_property",
                 "Colour the box by property",
                 [this, pset, property] { colourByProperty(pset, property, true); });
    menu.exec(pos, widget_);
}

void PropertiesPanelView::colourByProperty(const QString& pset, const QString& property, bool box_only) {
    if (box_only && box_.current().empty()) {
        session_state_->setStatusMessage("Properties", "The box is empty");
        return;
    }
    QMap<QString, std::vector<std::uint32_t>> groups;  // value -> elements, sorted by value
    std::vector<std::uint32_t> with_property;
    std::vector<std::uint32_t> without_property;
    for (const auto& [object_id, value] : scan(pset, property, box_only)) {
        if (!value) {
            without_property.push_back(object_id);
            continue;
        }
        groups[*value].push_back(object_id);
        with_property.push_back(object_id);
    }

    // Elements without the property are hidden in the viewer.
    if (!without_property.empty()) {
        actions_.run(without_property, services::ActionPipeline::Action::RemoveFromView);
    }
    // One colour per value; the same colour goes to the viewer and to the table's Color column.
    std::unordered_map<std::uint32_t, QColor> colours;
    int index = 0;
    for (auto it = groups.cbegin(); it != groups.cend(); ++it) {
        const QColor colour = it.key().isEmpty() ? QColor(150, 150, 150) : groupColour(index++);
        actions_.colour(it.value(), colour);
        for (std::uint32_t object_id : it.value()) colours[object_id] = colour;
    }

    if (table_view_) {
        table::TableColumn value_column;
        value_column.kind = table::TableColumn::Kind::Property;
        value_column.pset = pset;
        value_column.property = property;
        value_column.group_key = true;
        table::TableColumn count_column;
        count_column.kind = table::TableColumn::Kind::Count;
        table::TableColumn color_column;
        color_column.kind = table::TableColumn::Kind::Color;
        table_view_->showTemporaryTable(QString("Colour by %1").arg(property),
                                        {value_column, count_column, color_column}, with_property, colours);
    }
    session_state_->setStatusMessage(
        "Properties", QString("%1 element(s) coloured by %2, %3 value(s)")
                          .arg(static_cast<int>(with_property.size()))
                          .arg(property)
                          .arg(static_cast<int>(groups.size())));
}

} // namespace bonsaiviewer::modules::properties
