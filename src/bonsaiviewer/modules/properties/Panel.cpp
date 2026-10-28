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

#include "Panel.h"

#include "../../components/KeyValueTable.h"
#include "../../components/Section.h"
#include "../../components/Style.h"
#include "../../components/SvgIcon.h"

#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QToolButton>
#include <QVBoxLayout>

#include <optional>

namespace {

// BonsaiViewer24 "Color Properties": a checked row in green or red.
bonsaiviewer::components::KeyValueTableRow tableRow(const bonsaiviewer::modules::properties::KeyValueRow& row) {
    bonsaiviewer::components::KeyValueTableRow table_row{row.key, row.value, "keyValueValueLabel", "", "", 0};
    if (row.ids_status != 0) {
        table_row.status_color = row.ids_status > 0 ? QStringLiteral("#2ea043") : QStringLiteral("#dc322f");
        table_row.tooltip = row.ids_text;
    }
    return table_row;
}

QWidget* makePropertySetPanel(const bonsaiviewer::modules::properties::PropertySet& property_set,
                              const bonsaiviewer::modules::properties::PropertyContextHandler& on_context,
                              QWidget* parent = nullptr) {
    auto* group = new QGroupBox(property_set.title, parent);
    group->setObjectName("propertySetBox");
    auto* layout = new QVBoxLayout(group);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(0);

    QList<bonsaiviewer::components::KeyValueTableRow> rows;
    for (const auto& row : property_set.rows) {
        rows.append(tableRow(row));
    }
    auto* table = new bonsaiviewer::components::KeyValueTable(rows, group);
    // The rows of the table are the rows of property_set (as filtered).
    QObject::connect(table, &bonsaiviewer::components::KeyValueTable::contextMenuRequested, table,
                     [property_set, on_context](int row, bool on_value, const QPoint& pos) {
                         on_context(property_set, row, on_value, pos);
                     });
    layout->addWidget(table);
    return group;
}

void clearLayout(QLayout* layout) {
    if (!layout) return;
    while (QLayoutItem* item = layout->takeAt(0)) {
        if (QWidget* w = item->widget()) delete w;
        delete item;
    }
}

// A container for a section's set widgets, laid out like the section body so it
// can be swapped/rebuilt in one place without touching the filter field.
QWidget* makeSetContainer(QWidget* parent) {
    auto* container = new QWidget(parent);
    auto* layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(bonsaiviewer::components::style::metrics::padding);
    return container;
}

QWidget* makeAttributeList(const QList<bonsaiviewer::modules::properties::KeyValueRow>& rows, QWidget* parent = nullptr) {
    QList<bonsaiviewer::components::KeyValueTableRow> table_rows;
    for (const auto& row : rows) {
        table_rows.append(tableRow(row));
    }
    return new bonsaiviewer::components::KeyValueTable(table_rows, parent);
}

// Rows with an ifc_id are links (with the pointer icon); the others are plain.
bonsaiviewer::components::KeyValueTable* makeRelationshipList(
    const QList<bonsaiviewer::modules::properties::RelationshipRow>& rows, QWidget* parent = nullptr) {
    QList<bonsaiviewer::components::KeyValueTableRow> table_rows;
    for (const auto& row_data : rows) {
        const bool link = row_data.ifc_id != 0;
        table_rows.append({row_data.key,
                           row_data.value,
                           "keyValueValueLabel",
                           link ? ":/icons/cursor-pointer.svg" : "",
                           "keyValueTrailingIconLabel",
                           72,
                           link});
    }
    return new bonsaiviewer::components::KeyValueTable(table_rows, parent);
}

QLabel* makeEmptyStateLabel(const QString& text, QWidget* parent = nullptr) {
    auto* label = new QLabel(text, parent);
    label->setObjectName("panelSectionEmptyLabel");
    return label;
}

QWidget* makeFilterWrapper(QLineEdit** field_out, QWidget* parent = nullptr) {
    auto* wrapper = new QWidget(parent);
    wrapper->setObjectName("panelSectionFilterWrapper");
    auto* layout = new QVBoxLayout(wrapper);
    layout->setContentsMargins(bonsaiviewer::components::style::metrics::section_body_padding,
                               0,
                               bonsaiviewer::components::style::metrics::section_body_padding,
                               0);
    layout->setSpacing(0);

    auto* field = new QLineEdit(wrapper);
    field->setClearButtonEnabled(true);
    field->addAction(bonsaiviewer::components::icons::makeSvgIcon(":/icons/filter.svg"), QLineEdit::LeadingPosition);
    field->setVisible(false);
    layout->addWidget(field);

    if (field_out) *field_out = field;
    return wrapper;
}

// The entity header. With can_go_back, a Back button leads the row; it is
// returned through back_out (nullptr otherwise).
QFrame* makeEntityBox(const bonsaiviewer::modules::properties::EntitySummary& entity,
                      bool can_go_back,
                      QToolButton** back_out,
                      QWidget* parent = nullptr) {
    auto* entity_box = new QFrame(parent);
    entity_box->setObjectName("entityClassBox");
    auto* entity_layout = new QHBoxLayout(entity_box);
    entity_layout->setContentsMargins(10, 8, 10, 8);
    entity_layout->setSpacing(10);

    *back_out = nullptr;
    if (can_go_back) {
        auto* back = new QToolButton(entity_box);
        back->setObjectName("propertiesBackButton");
        back->setText(QStringLiteral("\u2190"));
        back->setToolTip("Back");
        back->setAutoRaise(true);
        entity_layout->addWidget(back, 0, Qt::AlignVCenter);
        *back_out = back;
    }

    auto* entity_icon = new QLabel(entity_box);
    entity_icon->setPixmap(bonsaiviewer::components::icons::makeSvgPixmap(":/icons/cube-dots.svg", QSize(28, 28)));
    entity_icon->setAlignment(Qt::AlignCenter);

    auto* entity_text = new QWidget(entity_box);
    auto* entity_text_layout = new QVBoxLayout(entity_text);
    entity_text_layout->setContentsMargins(0, 0, 0, 0);
    entity_text_layout->setSpacing(2);

    auto* entity_class_label = new QLabel(entity.entity_class, entity_text);
    entity_class_label->setObjectName("entityClassLabel");
    auto* entity_type_label = new QLabel(entity.predefined_type, entity_text);
    entity_type_label->setProperty("textRole", "secondary");

    entity_text_layout->addWidget(entity_class_label);
    entity_text_layout->addWidget(entity_type_label);
    entity_layout->addWidget(entity_icon, 0, Qt::AlignVCenter);
    entity_layout->addWidget(entity_text, 1, Qt::AlignVCenter);
    return entity_box;
}

} // namespace

namespace bonsaiviewer::modules::properties {

PropertiesPanel::PropertiesPanel(QWidget* parent)
    : components::Panel("Properties", nullptr, parent, false, true)
{
}

void PropertiesPanel::render(const PropertiesPanelState& state) {
    clearBodyWidgets();
    // The previous widgets were just deleted — drop the stale container pointers
    // before rebuilding so a stray filter pass can't touch them.
    property_sets_data_ = state.property_sets;
    quantity_sets_data_ = state.quantity_sets;
    properties_container_ = nullptr;
    quantities_container_ = nullptr;

    auto* entity_section = new components::Section("", components::SectionHeaderMode::Hidden, this);
    QToolButton* back = nullptr;
    entity_section->addBodyWidget(makeEntityBox(state.entity, state.can_go_back, &back, this));
    if (back) {
        connect(back, &QToolButton::clicked, this, &PropertiesPanel::backRequested);
    }

    auto* attributes_section = new components::Section("Attributes", components::SectionHeaderMode::Visible, this);
    attributes_section->addBodyWidget(makeAttributeList(state.attributes, this));
    attributes_section->setExpanded(attributes_expanded_);

    auto* relationships_section = new components::Section("Relationships", components::SectionHeaderMode::Visible, this);
    auto* relationship_list = makeRelationshipList(state.relationships, this);
    connect(relationship_list, &components::KeyValueTable::linkActivated,
            this, &PropertiesPanel::relationshipActivated);
    connect(relationship_list, &components::KeyValueTable::contextMenuRequested, this,
            [this](int row, bool, const QPoint& pos) { emit relationshipContextRequested(row, pos); });
    relationships_section->addBodyWidget(relationship_list);
    relationships_section->setExpanded(relationships_expanded_);

    auto* properties_section = new components::Section("Properties", components::SectionHeaderMode::Visible, this);
    auto* properties_filter_toggle = new QToolButton(properties_section);
    properties_filter_toggle->setObjectName("panelSectionFilterToggle");
    properties_filter_toggle->setCheckable(true);
    properties_filter_toggle->setIcon(components::icons::makeSvgIcon(":/icons/filter.svg"));
    properties_filter_toggle->setAutoRaise(true);
    properties_section->addHeaderWidget(properties_filter_toggle);
    QLineEdit* properties_filter_field = nullptr;
    auto* properties_filter_wrapper = makeFilterWrapper(&properties_filter_field, properties_section);
    properties_filter_field->setPlaceholderText("Filter properties or sets");
    properties_filter_field->setText(properties_filter_text_);
    properties_filter_wrapper->setVisible(properties_filter_visible_);
    properties_filter_field->setVisible(properties_filter_visible_);
    connect(properties_filter_toggle, &QToolButton::toggled, properties_filter_field, [this, properties_filter_field, properties_filter_wrapper](bool visible) {
        properties_filter_visible_ = visible;
        properties_filter_field->setVisible(visible);
        properties_filter_wrapper->setVisible(visible);
        if (visible) properties_filter_field->setFocus();
    });
    connect(properties_filter_field, &QLineEdit::textChanged, this, [this](const QString& text) {
        properties_filter_text_ = text;
        rebuildPropertyWidgets();
    });
    properties_section->addBodyWidget(properties_filter_wrapper);
    properties_container_ = makeSetContainer(properties_section);
    properties_section->addBodyWidget(properties_container_);
    rebuildPropertyWidgets();
    properties_section->setExpanded(properties_expanded_);
    properties_filter_toggle->setChecked(properties_filter_visible_);

    auto* quantities_section = new components::Section("Quantities", components::SectionHeaderMode::Visible, this);
    auto* quantities_filter_toggle = new QToolButton(quantities_section);
    quantities_filter_toggle->setObjectName("panelSectionFilterToggle");
    quantities_filter_toggle->setCheckable(true);
    quantities_filter_toggle->setIcon(components::icons::makeSvgIcon(":/icons/filter.svg"));
    quantities_filter_toggle->setAutoRaise(true);
    quantities_section->addHeaderWidget(quantities_filter_toggle);
    QLineEdit* quantities_filter_field = nullptr;
    auto* quantities_filter_wrapper = makeFilterWrapper(&quantities_filter_field, quantities_section);
    quantities_filter_field->setPlaceholderText("Filter quantities or sets");
    quantities_filter_field->setText(quantities_filter_text_);
    quantities_filter_wrapper->setVisible(quantities_filter_visible_);
    quantities_filter_field->setVisible(quantities_filter_visible_);
    connect(quantities_filter_toggle, &QToolButton::toggled, quantities_filter_field, [this, quantities_filter_field, quantities_filter_wrapper](bool visible) {
        quantities_filter_visible_ = visible;
        quantities_filter_field->setVisible(visible);
        quantities_filter_wrapper->setVisible(visible);
        if (visible) quantities_filter_field->setFocus();
    });
    connect(quantities_filter_field, &QLineEdit::textChanged, this, [this](const QString& text) {
        quantities_filter_text_ = text;
        rebuildQuantityWidgets();
    });
    quantities_section->addBodyWidget(quantities_filter_wrapper);
    quantities_container_ = makeSetContainer(quantities_section);
    quantities_section->addBodyWidget(quantities_container_);
    rebuildQuantityWidgets();
    quantities_section->setExpanded(quantities_expanded_);
    quantities_filter_toggle->setChecked(quantities_filter_visible_);

    if (auto* button = attributes_section->findChild<QToolButton*>("panelSectionHeaderButton")) {
        connect(button, &QToolButton::toggled, this, [this](bool expanded) {
            attributes_expanded_ = expanded;
        });
    }
    if (auto* button = relationships_section->findChild<QToolButton*>("panelSectionHeaderButton")) {
        connect(button, &QToolButton::toggled, this, [this](bool expanded) {
            relationships_expanded_ = expanded;
        });
    }
    if (auto* button = properties_section->findChild<QToolButton*>("panelSectionHeaderButton")) {
        connect(button, &QToolButton::toggled, this, [this](bool expanded) {
            properties_expanded_ = expanded;
        });
    }
    if (auto* button = quantities_section->findChild<QToolButton*>("panelSectionHeaderButton")) {
        connect(button, &QToolButton::toggled, this, [this](bool expanded) {
            quantities_expanded_ = expanded;
        });
    }

    addBodyWidget(entity_section);
    addBodyWidget(attributes_section);
    addBodyWidget(relationships_section);
    addBodyWidget(properties_section);
    addBodyWidget(quantities_section);
}

namespace {

// Filter one set: keep it if the filter is empty, or its name matches (then all
// rows are kept), or some property name/value matches (then only those rows).
// Returns nullopt when nothing in the set matches.
std::optional<PropertySet> filterSet(const PropertySet& set, const QString& text) {
    if (text.isEmpty() || set.title.contains(text, Qt::CaseInsensitive)) {
        return set;
    }
    PropertySet filtered;
    filtered.title = set.title;
    for (const auto& row : set.rows) {
        if (row.key.contains(text, Qt::CaseInsensitive) ||
            row.value.contains(text, Qt::CaseInsensitive)) {
            filtered.rows.append(row);
        }
    }
    if (filtered.rows.isEmpty()) return std::nullopt;
    return filtered;
}

// Rebuild a set container's contents from raw data under the current filter,
// dropping non-matching rows, with a placeholder when nothing is shown.
void rebuildSetContainer(QWidget* container,
                         const QList<PropertySet>& sets,
                         const QString& filter_text,
                         const QString& empty_text,
                         const QString& no_match_text,
                         const bonsaiviewer::modules::properties::PropertyContextHandler& on_context) {
    if (!container) return;
    auto* layout = qobject_cast<QVBoxLayout*>(container->layout());
    if (!layout) return;
    clearLayout(layout);

    const QString text = filter_text.trimmed();
    int shown = 0;
    for (const auto& set : sets) {
        if (auto filtered = filterSet(set, text)) {
            layout->addWidget(makePropertySetPanel(*filtered, on_context, container));
            ++shown;
        }
    }
    if (shown == 0) {
        layout->addWidget(makeEmptyStateLabel(sets.isEmpty() ? empty_text : no_match_text, container));
    }
}

} // namespace

void PropertiesPanel::rebuildPropertyWidgets() {
    rebuildSetContainer(properties_container_, property_sets_data_, properties_filter_text_,
                        "No properties", "No matching properties", contextHandler());
}

void PropertiesPanel::rebuildQuantityWidgets() {
    rebuildSetContainer(quantities_container_, quantity_sets_data_, quantities_filter_text_,
                        "No quantities", "No matching quantities", contextHandler());
}

PropertyContextHandler PropertiesPanel::contextHandler() {
    return [this](const PropertySet& set, int row, bool on_value, const QPoint& pos) {
        if (row < 0 || row >= set.rows.size()) return;
        const KeyValueRow& item = set.rows.at(row);
        if (on_value) {
            emit propertyValueContextRequested(set.title, item.key, item.value, pos);
        } else {
            emit propertyNameContextRequested(set.title, item.key, pos);
        }
    };
}

} // namespace bonsaiviewer::modules::properties
