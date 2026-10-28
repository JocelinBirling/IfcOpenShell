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
#include "../../services/IdsPropertyChecks.h"
#include "../../services/IfcDataAccess.h"
#include "../../services/SelectionService.h"
#include "../../../ifcviewer/Federation.h"
#include "../../../ifcviewer/SceneLoader.h"
#include "../../../ifcparse/file.h"
#include "../../../ifcparse/schema.h"
#include "../table/Table.h"
#include "../table/View.h"

#include "element.h"  // helpers: get_predefined_type, get_type, get_container, get_aggregate, get_groups, get_parts, …
#include "pset.h"     // helpers: get_psets

#include <QClipboard>
#include <QColor>
#include <QGuiApplication>
#include <QMap>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace bonsaiviewer::modules::properties {

namespace {

// BonsaiViewer24 "Color Properties": marks a row with one IDS check (a
// failure wins over a pass).
void markRow(bonsaiviewer::modules::properties::KeyValueRow& row,
             const bonsaiviewer::services::IdsPropertyChecks::Check& check) {
    row.ids_checked = true;
    row.ids_status = (row.ids_status < 0 || !check.passed) ? -1 : 1;
    const QString line = QString(check.passed ? "✔ " : "✘ ") + check.text;
    row.ids_text = row.ids_text.isEmpty() ? line : row.ids_text + "\n" + line;
}

std::string upperNameOf(const bonsaiviewer::ports::ifctester::Param& param) {
    return param.isSimple() ? param.simple : param.toString();
}

// Marks the rows the IDS checks of the shown element concern. A failed
// requirement on a property or attribute the element does not have adds a
// "missing" row (in the property set it names). Then: without Color
// Properties the marks carry no colour; with Filter Properties only the
// marked rows (and their sets) stay.
void applyIdsChecks(bonsaiviewer::modules::properties::PropertiesPanelState& state,
                    const std::vector<bonsaiviewer::services::IdsPropertyChecks::Check>& checks,
                    bool colour,
                    bool filter) {
    namespace it = bonsaiviewer::ports::ifctester;
    using bonsaiviewer::modules::properties::KeyValueRow;
    using bonsaiviewer::modules::properties::PropertySet;
    const auto matches = [](const it::Param& param, const QString& text) {
        return param.matches(it::Value(text.toStdString()));
    };
    for (const auto& check : checks) {
        const it::Facet& facet = check.facet;
        bool found = false;
        if (facet.type == it::FacetType::Attribute) {
            for (KeyValueRow& row : state.attributes) {
                if (!matches(facet.name, row.key)) continue;
                markRow(row, check);
                found = true;
            }
            if (!found && !check.passed) {
                KeyValueRow missing{QString::fromStdString(upperNameOf(facet.name)), "missing"};
                markRow(missing, check);
                state.attributes.append(missing);
            }
            continue;
        }
        if (facet.type != it::FacetType::Property) continue;
        for (QList<PropertySet>* sets : {&state.property_sets, &state.quantity_sets}) {
            for (PropertySet& set : *sets) {
                if (!matches(facet.propertySet, set.title)) continue;
                for (KeyValueRow& row : set.rows) {
                    if (!matches(facet.baseName, row.key)) continue;
                    markRow(row, check);
                    found = true;
                }
            }
        }
        if (found || check.passed) continue;
        const QString set_title = QString::fromStdString(upperNameOf(facet.propertySet));
        QList<PropertySet>& sets = set_title.startsWith("Qto_") ? state.quantity_sets : state.property_sets;
        auto set = std::find_if(sets.begin(), sets.end(), [&](const PropertySet& s) { return s.title == set_title; });
        if (set == sets.end()) {
            sets.append(PropertySet{set_title, {}});
            set = sets.end() - 1;
        }
        KeyValueRow missing{QString::fromStdString(upperNameOf(facet.baseName)), "missing"};
        markRow(missing, check);
        set->rows.append(missing);
    }

    const auto keep_checked = [&](QList<KeyValueRow>& rows) {
        QList<KeyValueRow> kept;
        for (KeyValueRow& row : rows) {
            if (!colour) row.ids_status = 0;
            if (!filter || row.ids_checked) kept.append(row);
        }
        rows = kept;
    };
    keep_checked(state.attributes);
    for (QList<PropertySet>* sets : {&state.property_sets, &state.quantity_sets}) {
        QList<PropertySet> kept;
        for (PropertySet& set : *sets) {
            keep_checked(set.rows);
            if (!set.rows.isEmpty()) kept.append(set);
        }
        *sets = kept;
    }
}

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

// Name of a related entity, or its GlobalId when the Name is empty.
QString relatedLabel(const express::base& related) {
    if (auto name = get_string_attribute(related, "Name"); name && !name->empty()) {
        return QString::fromStdString(*name);
    }
    if (auto guid = get_string_attribute(related, "GlobalId"); guid && !guid->empty()) {
        return QString::fromStdString(*guid);
    }
    return QString::fromStdString(related.declaration().name());
}

bool isSpatial(const express::base& element) {
    const auto& declaration = element.declaration();
    return declaration.is("IfcSpatialStructureElement") || declaration.is("IfcSpatialElement");
}

// Relationships rows (BonsaiViewer14), after the Model row: Type, the spatial
// chain Site / Building / Storey / Space, then System / Zone / Asset / Group.
// Every row links to its entity.
void appendRelationships(const express::base& entity, std::uint32_t session_model_id,
                         QList<RelationshipRow>& rows) {
    auto link = [&](const QString& key, const express::base& related) {
        rows.append({key, relatedLabel(related), session_model_id, static_cast<int>(related.id())});
    };
    try {
        if (express::base type = get_type(entity); type && type.id() != entity.id()) {
            link("Type", type);
        }

        // Walk up from the container (from the aggregate parent for a spatial
        // element), keeping the nearest level of each kind.
        express::base site, building, storey, space;
        express::base level = isSpatial(entity) ? get_aggregate(entity) : get_container(entity);
        for (int guard = 0; level && guard < 64; ++guard) {
            const auto& declaration = level.declaration();
            if (declaration.is("IfcSite")) { if (!site) site = level; }
            else if (declaration.is("IfcBuilding")) { if (!building) building = level; }
            else if (declaration.is("IfcBuildingStorey")) { if (!storey) storey = level; }
            else if (declaration.is("IfcSpace")) { if (!space) space = level; }
            level = get_aggregate(level);
        }
        if (site) link("Site", site);
        if (building) link("Building", building);
        if (storey) link("Storey", storey);
        if (space) link("Space", space);

        // Groups the entity is directly assigned to, by family.
        QList<RelationshipRow> systems, zones, assets, groups;
        for (const express::base& group : get_groups(entity)) {
            const auto& declaration = group.declaration();
            const RelationshipRow row{QString(), relatedLabel(group), session_model_id,
                                      static_cast<int>(group.id())};
            if (declaration.is("IfcZone")) zones.append(row);
            else if (declaration.is("IfcSystem")) systems.append(row);
            else if (declaration.is("IfcAsset")) assets.append(row);
            else groups.append(row);
        }
        for (auto [key, family] : {std::pair{QStringLiteral("System"), &systems},
                                   std::pair{QStringLiteral("Zone"), &zones},
                                   std::pair{QStringLiteral("Asset"), &assets},
                                   std::pair{QStringLiteral("Group"), &groups}}) {
            for (RelationshipRow row : *family) {
                row.key = key;
                rows.append(row);
            }
        }
    } catch (const std::exception&) {
        // Unsupported schema: keep the rows found so far.
    }
}

// An element and its parts, recursively (geometry often sits on the parts).
void collectWithParts(const express::base& element, std::unordered_set<int>& ids) {
    if (!element || !ids.insert(static_cast<int>(element.id())).second) return;
    for (const express::base& part : get_parts(element)) collectWithParts(part, ids);
}

// A spatial element, what it contains, and its spatial children, recursively.
void collectSpatial(const express::base& element, std::unordered_set<int>& ids) {
    if (!element || !ids.insert(static_cast<int>(element.id())).second) return;
    for (const express::base& contained : get_contained_elements(element)) collectWithParts(contained, ids);
    for (const express::base& part : get_parts(element)) {
        if (isSpatial(part)) collectSpatial(part, ids);
        else collectWithParts(part, ids);
    }
}

// The members of a group; a member that is a group adds its own members.
void collectGroup(const express::base& group, std::unordered_set<int>& ids, std::unordered_set<int>& seen) {
    if (!group || !seen.insert(static_cast<int>(group.id())).second) return;
    for (const express::base& member : get_group_members(group)) {
        if (member.declaration().is("IfcGroup")) collectGroup(member, ids, seen);
        else collectWithParts(member, ids);
    }
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
    connect(session_state_, &bonsaiviewer::SessionState::entityInspectRequested, this,
            [this](uint32_t session_model_id, int ifc_id) { open(session_model_id, ifc_id); });
    // The IFC source opens after the geometry: re-read the shown entity.
    connect(session_state_, &bonsaiviewer::SessionState::modelDataSourceReady, this,
            [this](uint32_t session_model_id) {
                if (shown_.session_model_id == session_model_id) render();
            });
    connect(widget_, &PropertiesPanel::relationshipActivated, this, [this](int row) {
        if (row < 0 || row >= relationships_.size()) return;
        const RelationshipRow& target = relationships_.at(row);
        open(target.session_model_id, target.ifc_id);
    });
    connect(widget_, &PropertiesPanel::backRequested, this, [this]() { goBack(); });
    connect(widget_, &PropertiesPanel::relationshipContextRequested, this,
            [this](int row, const QPoint& pos) { showRelationshipMenu(row, pos); });
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
    history_.clear();
    shown_ = {};
    if (object_id != 0) {
        auto* registry = session_state_->elementRegistry();
        if (auto info = registry ? registry->findBasicElementInfo(object_id) : std::nullopt) {
            shown_ = {info->session_model_id, info->ifc_id, object_id};
        }
    }
    render();
}

void PropertiesPanelView::open(std::uint32_t session_model_id, int ifc_id) {
    if (session_model_id == 0 || ifc_id == 0) return;
    if (shown_.ifc_id != 0) history_.append(shown_);
    shown_ = {session_model_id, ifc_id, 0};
    render();
}

void PropertiesPanelView::goBack() {
    if (history_.isEmpty()) return;
    shown_ = history_.takeLast();
    render();
}

QString PropertiesPanelView::modelName(std::uint32_t session_model_id) const {
    const QString model_id = session_state_->modelIdForSessionModelId(session_model_id);
    if (auto* federation = session_state_->federation()) {
        if (const auto* model = federation->findById(model_id); model && !model->display_name.isEmpty()) {
            return model->display_name;
        }
    }
    auto* loader = session_state_->loader();
    return loader ? loader->displayName(session_model_id) : QString();
}

void PropertiesPanelView::render() {
    // Empty default: nothing selected → "No item selected" with empty sections.
    PropertiesPanelState state;
    state.entity = {"No item selected", ""};
    state.can_go_back = !history_.isEmpty();

    express::base entity;
    if (shown_.ifc_id != 0) {
        auto* loader = session_state_->loader();
        if (auto* file = loader ? loader->ifcFile(shown_.session_model_id) : nullptr) {
            try {
                entity = file->instance_by_id(shown_.ifc_id);
            } catch (...) {
                entity = {};
            }
        }
        // The model is known even before the IFC data source is open.
        const QString model = modelName(shown_.session_model_id);
        if (!model.isEmpty()) state.relationships.append({"Model", model, shown_.session_model_id, 0});
    }

    if (entity) {
        state.entity.entity_class = QString::fromStdString(entity.declaration().name());
        if (auto predefined_type = get_predefined_type(entity)) {
            state.entity.predefined_type = QString::fromStdString(*predefined_type);
        }
        // Direct EXPRESS attributes, primitives only — lists / entity refs omitted.
        for (const auto& [name, value] : get_scalar_attributes(entity)) {
            state.attributes.append({QString::fromStdString(name), QString::fromStdString(value)});
        }
        appendRelationships(entity, shown_.session_model_id, state.relationships);
        // Property sets (Pset_*) and quantity sets (Qto_* / BaseQuantities),
        // occurrence values inheriting from the type.
        state.property_sets = toPropertySets(get_psets(entity, /*psets_only=*/true, /*qtos_only=*/false));
        state.quantity_sets = toPropertySets(get_psets(entity, /*psets_only=*/false, /*qtos_only=*/true));
    } else if (shown_.object_id != 0) {
        // No live IFC source for this object — typical when a pure-geometry
        // .ifcview was loaded without its .ifc/.rdb sibling, or before it is
        // open. Fall back to the basic info cached in the element registry so
        // the panel still shows class / GlobalId / Name for visible elements.
        auto* registry = session_state_->elementRegistry();
        auto info = registry ? registry->findBasicElementInfo(shown_.object_id) : std::nullopt;
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
    if (ids_checks_ != nullptr && shown_.ifc_id != 0 && (ids_checks_->colour() || ids_checks_->filter())) {
        // Filter Properties on an element no requirement checks: nothing to show.
        static const std::vector<bonsaiviewer::services::IdsPropertyChecks::Check> none;
        const auto* checks = ids_checks_->checksOf(shown_.session_model_id, shown_.ifc_id);
        applyIdsChecks(state, checks ? *checks : none, ids_checks_->colour(), ids_checks_->filter());
    }
    relationships_ = state.relationships;
    widget_->render(state);
}

void PropertiesPanelView::setIdsChecks(bonsaiviewer::services::IdsPropertyChecks* checks) {
    ids_checks_ = checks;
    if (ids_checks_ != nullptr) {
        connect(ids_checks_, &bonsaiviewer::services::IdsPropertyChecks::changed, this, [this]() { render(); });
    }
}

std::vector<std::uint32_t> PropertiesPanelView::relatedObjectIds(const RelationshipRow& row) const {
    std::vector<std::uint32_t> object_ids;
    auto* registry = session_state_->elementRegistry();
    if (registry == nullptr || row.session_model_id == 0) return object_ids;
    const auto elements = registry->basicElementInfoForModel(row.session_model_id);

    // The Model row: every element of the model.
    if (row.ifc_id == 0) {
        object_ids.reserve(elements.size());
        for (const auto& info : elements) object_ids.push_back(info.object_id);
        return object_ids;
    }

    auto* loader = session_state_->loader();
    ifcopenshell::file* file = loader ? loader->ifcFile(row.session_model_id) : nullptr;
    if (file == nullptr) return object_ids;
    std::unordered_set<int> ifc_ids;
    try {
        const express::base related = file->instance_by_id(row.ifc_id);
        const auto& declaration = related.declaration();
        if (declaration.is("IfcTypeObject")) {
            for (const express::base& occurrence : get_type_occurrences(related)) {
                collectWithParts(occurrence, ifc_ids);
            }
        } else if (declaration.is("IfcGroup")) {
            std::unordered_set<int> seen;
            collectGroup(related, ifc_ids, seen);
        } else if (isSpatial(related)) {
            collectSpatial(related, ifc_ids);
        } else {
            collectWithParts(related, ifc_ids);
        }
    } catch (...) {
        return object_ids;
    }
    // Keep the ids that have geometry (the element registry).
    for (const auto& info : elements) {
        if (ifc_ids.count(info.ifc_id) != 0) object_ids.push_back(info.object_id);
    }
    return object_ids;
}

void PropertiesPanelView::showRelationshipMenu(int row, const QPoint& pos) {
    if (row < 0 || row >= relationships_.size()) return;
    const RelationshipRow target = relationships_.at(row);
    services::ContextMenuBuilder menu;
    menu.addItem("isolate_related",
                 "Isolate",
                 [this, target] {
                     const auto ids = relatedObjectIds(target);
                     if (ids.empty()) {
                         session_state_->setStatusMessage("Properties", "No element to isolate");
                         return;
                     }
                     actions_.run(ids, services::ActionPipeline::Action::Isolate);
                 });
    menu.exec(pos, widget_);
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
