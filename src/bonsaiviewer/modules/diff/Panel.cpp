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

#include "../../components/Section.h"
#include "../property_selection/PropertyField.h"
#include "../../services/IfcDiff/IfcDiff.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QHash>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

namespace bonsaiviewer::modules::diff {

namespace {

constexpr auto kLastIdsDirectoryKey = "diff/lastIdsDirectory";

// The diff colours of the viewport (spec 34).
QString legendLine(const QString& colour, const QString& text) {
    return QString("<span style=\"color:%1; font-size:14pt;\">■</span> %2").arg(colour, text);
}

// Hover help, as the ribbon buttons: the name in bold, then what it is for.
void describe(QWidget* widget, const QString& title, const QString& text) {
    widget->setToolTip(QString("<b>%1</b><br>%2").arg(title, text));
}

// What each IfcDiff relationship compares.
QString relationshipHelp(const std::string& name) {
    if (name == "attributes")
        return "The direct attributes of the element: Name, Description, ObjectType, Tag, PredefinedType... "
               "References to other entities are not compared here.";
    if (name == "geometry")
        return "The placement (moved, rotated), the openings and projections, and the Body representation, "
               "item by item (dimensions, profile, extrusion...). Feeds the Geometry buttons.";
    if (name == "type")
        return "The type object of the element (IfcTypeObject), compared by GlobalId.";
    if (name == "property")
        return "The property sets and quantity sets, property by property and value by value. "
               "Feeds the Properties buttons and the colours of the Properties panel.";
    if (name == "container")
        return "The spatial container of the element (storey, space...), compared by GlobalId.";
    if (name == "aggregate")
        return "The whole the element is part of (IfcRelAggregates), compared by GlobalId.";
    if (name == "classification")
        return "The classification references of the element (their codes).";
    return {};
}

// What each property change kind means (the property is compared on the two
// versions of the same element).
QString propertyHelp(SelectKind kind) {
    switch (kind) {
    case SelectKind::PropertyAdded: return "The elements with a property that exists in After only.";
    case SelectKind::PropertyRemoved: return "The elements with a property that exists in Before only.";
    case SelectKind::ValueAdded: return "The elements with a property empty in Before and filled in After.";
    case SelectKind::ValueRemoved: return "The elements with a property filled in Before and empty in After.";
    case SelectKind::ValueChanged: return "The elements with a property whose value differs (text, number, boolean...).";
    case SelectKind::ValueIncreased: return "The elements with a numeric property whose value is greater in After.";
    case SelectKind::ValueDecreased: return "The elements with a numeric property whose value is smaller in After.";
    }
    return {};
}

void fillModelCombo(QComboBox* combo, const QList<QPair<std::uint32_t, QString>>& models) {
    const QVariant current = combo->currentData();
    combo->clear();
    combo->addItem(QString(), QVariant::fromValue<std::uint32_t>(0));
    for (const auto& [id, name] : models) combo->addItem(name, QVariant::fromValue<std::uint32_t>(id));
    const int index = combo->findData(current);
    combo->setCurrentIndex(index >= 0 ? index : 0);
}

} // namespace

DiffPanel::DiffPanel(services::PropertyCatalog& catalog, QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    // 1. Model selectors.
    auto* models_section = new components::Section("Models", components::SectionHeaderMode::Visible, this);
    auto* models = new QWidget(models_section);
    auto* models_layout = new QGridLayout(models);
    models_layout->setContentsMargins(0, 0, 0, 0);
    before_combo_ = new QComboBox(models);
    after_combo_ = new QComboBox(models);
    describe(before_combo_, "Before model",
             "The old version of the model, the reference of the comparison. "
             "Its deleted elements are shown in red, its modified ones in blue.");
    describe(after_combo_, "After model",
             "The new version of the model, compared with the Before model element by element (GlobalId). "
             "Its added elements are shown in green, its modified ones in orange.");
    auto* before_label = new QLabel("Before model", models);
    auto* after_label = new QLabel("After model", models);
    before_label->setToolTip(before_combo_->toolTip());
    after_label->setToolTip(after_combo_->toolTip());
    models_layout->addWidget(before_label, 0, 0);
    models_layout->addWidget(before_combo_, 0, 1);
    models_layout->addWidget(after_label, 1, 0);
    models_layout->addWidget(after_combo_, 1, 1);
    models_layout->setColumnStretch(1, 1);
    models_section->addBodyWidget(models);
    layout->addWidget(models_section);

    // 2. Relationships and 3. options.
    auto* compare_section = new components::Section("Compare", components::SectionHeaderMode::Visible, this);
    auto* checks = new QWidget(compare_section);
    auto* checks_layout = new QGridLayout(checks);
    checks_layout->setContentsMargins(0, 0, 0, 0);
    int index = 0;
    for (const std::string& name : services::ifcdiff::relationshipNames()) {
        auto* check = new QCheckBox(QString::fromStdString(name), checks);
        check->setChecked(name == "attributes" || name == "geometry" || name == "property");
        describe(check, QString::fromStdString(name), relationshipHelp(name));
        relationship_checks_[name] = check;
        checks_layout->addWidget(check, index / 2, index % 2);
        ++index;
    }
    compare_section->addBodyWidget(checks);
    shallow_check_ = new QCheckBox("First difference only (is_shallow)", compare_section);
    describe(shallow_check_, "First difference only",
             "Stops at the first difference found on each element: faster on large models, "
             "but the details of an element can be incomplete (an element with a changed attribute "
             "is not checked for properties or geometry). Leave it off to use the Properties and "
             "Geometry buttons.");
    compare_section->addBodyWidget(shallow_check_);
    auto* ids_row = new QWidget(compare_section);
    auto* ids_layout = new QHBoxLayout(ids_row);
    ids_layout->setContentsMargins(0, 0, 0, 0);
    ids_label_ = new QLabel("IDS shortlist: none", ids_row);
    ids_label_->setProperty("textRole", "secondary");
    auto* ids_button = new QPushButton("IDS…", ids_row);
    ids_clear_button_ = new QPushButton("Clear", ids_row);
    describe(ids_button, "IDS shortlist",
             "Optional. Restricts the property comparison to what an IDS file asks for:<br>"
             "• only the properties named in the requirements of its specifications are compared;<br>"
             "• only on the elements the specifications apply to.<br>"
             "Use it to focus on the contractual data (the properties a client or a BIM protocol "
             "requires) instead of the hundreds of properties an authoring tool exports. "
             "Attributes and geometry are not affected.");
    describe(ids_clear_button_, "Clear the IDS", "Back to comparing every property set and quantity set.");
    ids_label_->setToolTip(ids_button->toolTip());
    ids_clear_button_->setEnabled(false);
    ids_layout->addWidget(ids_label_, 1);
    ids_layout->addWidget(ids_button);
    ids_layout->addWidget(ids_clear_button_);
    compare_section->addBodyWidget(ids_row);
    layout->addWidget(compare_section);

    // 4. Run.
    auto* run_row = new QWidget(this);
    auto* run_layout = new QHBoxLayout(run_row);
    run_layout->setContentsMargins(0, 0, 0, 0);
    run_button_ = new QPushButton("Run", run_row);
    auto* export_button = new QPushButton("Export JSON…", run_row);
    describe(run_button_, "Run",
             "Compares the Before and After models with the checked relationships, colours the result "
             "in the 3D view and in the Properties panel. A new run replaces the previous one (CTRL+Z "
             "brings back the previous colours).");
    describe(export_button, "Export JSON",
             "Saves the result as a JSON file, in the format of the IfcDiff command line "
             "(added, deleted, changed), with the detail of each change.");
    run_layout->addWidget(run_button_, 1);
    run_layout->addWidget(export_button);
    layout->addWidget(run_row);
    summary_label_ = new QLabel(this);
    summary_label_->setWordWrap(true);
    summary_label_->setProperty("textRole", "secondary");
    layout->addWidget(summary_label_);

    // 5. Result actions.
    result_box_ = new QWidget(this);
    auto* result_layout = new QVBoxLayout(result_box_);
    result_layout->setContentsMargins(0, 0, 0, 0);
    auto* view_section = new components::Section("View", components::SectionHeaderMode::Visible, result_box_);
    // The four colours of the viewport, each one a legend line with its
    // Select / Isolate buttons, like the Properties and Geometry rows.
    auto* cases = new QWidget(view_section);
    auto* cases_layout = new QGridLayout(cases);
    cases_layout->setContentsMargins(0, 0, 0, 0);
    struct CaseRow {
        DiffCase kind;
        const char* colour;
        QString label;
        QString help;
    };
    const QList<CaseRow> case_rows{
        {DiffCase::Deleted, "#E6194B", "Deleted (Before)", "The elements of the Before model that are not in the After model."},
        {DiffCase::Added, "#3CB44B", "Added (After)", "The elements of the After model that are not in the Before model."},
        {DiffCase::ModifiedBefore, "#3B6FE0", "Modified (Before)",
         "The Before version of the elements that changed (any checked relationship)."},
        {DiffCase::ModifiedAfter, "#F08A24", "Modified (After)",
         "The After version of the elements that changed (any checked relationship)."},
    };
    int case_row = 0;
    for (const CaseRow& entry : case_rows) {
        auto* label = new QLabel(legendLine(entry.colour, entry.label), cases);
        label->setTextFormat(Qt::RichText);
        describe(label, entry.label, entry.help);
        auto* select = new QPushButton("Select", cases);
        auto* isolate = new QPushButton("Isolate", cases);
        describe(select, "Select — " + entry.label, entry.help + "<br>Selects these elements in their model.");
        describe(isolate, "Isolate — " + entry.label, entry.help + "<br>Shows only these elements.");
        cases_layout->addWidget(label, case_row, 0);
        cases_layout->addWidget(select, case_row, 1);
        cases_layout->addWidget(isolate, case_row, 2);
        const DiffCase k = entry.kind;
        connect(select, &QPushButton::clicked, this, [this, k]() { emit caseSelectRequested(k, false); });
        connect(isolate, &QPushButton::clicked, this, [this, k]() { emit caseSelectRequested(k, true); });
        ++case_row;
    }
    cases_layout->setColumnStretch(0, 1);
    view_section->addBodyWidget(cases);
    auto* view_row = new QWidget(view_section);
    auto* view_layout = new QHBoxLayout(view_row);
    view_layout->setContentsMargins(0, 0, 0, 0);
    swap_button_ = new QPushButton(view_row);
    describe(swap_button_, "Swap transparency",
             "A modified element exists in both models at the same place. One side is shown at 50 % "
             "opacity so that the other can be seen through it; this button swaps the two sides.");
    auto* clear_button = new QPushButton("Clear Diff", view_row);
    describe(clear_button, "Clear Diff",
             "Removes the diff colours of the 3D view and of the Properties panel. The result stays "
             "available for the buttons and the report. CTRL+Z brings the colours back.");
    view_layout->addWidget(swap_button_, 1);
    view_layout->addWidget(clear_button);
    view_section->addBodyWidget(view_row);
    result_layout->addWidget(view_section);

    auto* properties_section = new components::Section("Properties", components::SectionHeaderMode::Visible, result_box_);
    property_field_ = new property_selection::PropertyField(catalog, properties_section);
    describe(property_field_, "Property",
             "Restricts the buttons below to one property (Pset and Property). "
             "Empty: every compared property.");
    properties_section->addBodyWidget(property_field_);
    auto* kinds = new QWidget(properties_section);
    auto* kinds_layout = new QGridLayout(kinds);
    kinds_layout->setContentsMargins(0, 0, 0, 0);
    const QList<QPair<SelectKind, QString>> kind_rows{
        {SelectKind::PropertyAdded, "Property added"},   {SelectKind::PropertyRemoved, "Property removed"},
        {SelectKind::ValueAdded, "Value added"},         {SelectKind::ValueRemoved, "Value removed"},
        {SelectKind::ValueChanged, "Value changed"},     {SelectKind::ValueIncreased, "Value increased"},
        {SelectKind::ValueDecreased, "Value decreased"},
    };
    int row = 0;
    for (const auto& [kind, label] : kind_rows) {
        auto* select = new QPushButton("Select", kinds);
        auto* isolate = new QPushButton("Isolate", kinds);
        const QString help = propertyHelp(kind);
        describe(select, "Select — " + label,
                 help + "<br>Selects these elements in both models (the Properties panel shows the active one).");
        describe(isolate, "Isolate — " + label, help + "<br>Shows only these elements, in both models.");
        auto* row_label = new QLabel(label, kinds);
        describe(row_label, label, help);
        kinds_layout->addWidget(row_label, row, 0);
        kinds_layout->addWidget(select, row, 1);
        kinds_layout->addWidget(isolate, row, 2);
        const SelectKind k = kind;
        connect(select, &QPushButton::clicked, this, [this, k]() { emit selectRequested(k, false); });
        connect(isolate, &QPushButton::clicked, this, [this, k]() { emit selectRequested(k, true); });
        ++row;
    }
    kinds_layout->setColumnStretch(0, 1);
    properties_section->addBodyWidget(kinds);
    result_layout->addWidget(properties_section);

    // The same buttons for what the geometry comparison found.
    auto* geometry_section = new components::Section("Geometry", components::SectionHeaderMode::Visible, result_box_);
    auto* geometry_kinds = new QWidget(geometry_section);
    auto* geometry_layout = new QGridLayout(geometry_kinds);
    geometry_layout->setContentsMargins(0, 0, 0, 0);
    const QList<QPair<GeometryKind, QString>> geometry_rows{
        {GeometryKind::Any, "Geometry changed"},
        {GeometryKind::Moved, "Moved"},
        {GeometryKind::Rotated, "Rotated"},
        {GeometryKind::Shape, "Shape changed"},
        {GeometryKind::Openings, "Openings changed"},
    };
    const QHash<int, QString> geometry_tips{
        {static_cast<int>(GeometryKind::Any), "The elements with a geometry change of any kind below."},
        {static_cast<int>(GeometryKind::Moved), "The elements whose placement location changed (beyond the model precision)."},
        {static_cast<int>(GeometryKind::Rotated), "The elements whose placement orientation changed."},
        {static_cast<int>(GeometryKind::Shape),
         "The elements whose Body representation changed: dimensions, profile, extrusion, number of items..."},
        {static_cast<int>(GeometryKind::Openings), "The elements with openings or projections added or removed."},
    };
    row = 0;
    for (const auto& [kind, label] : geometry_rows) {
        auto* select = new QPushButton("Select", geometry_kinds);
        auto* isolate = new QPushButton("Isolate", geometry_kinds);
        const QString tip = geometry_tips.value(static_cast<int>(kind));
        describe(select, "Select — " + label,
                 tip + "<br>Selects these elements in both models (the Properties panel shows the active one).");
        describe(isolate, "Isolate — " + label, tip + "<br>Shows only these elements, in both models.");
        auto* row_label = new QLabel(label, geometry_kinds);
        describe(row_label, label, tip);
        geometry_layout->addWidget(row_label, row, 0);
        geometry_layout->addWidget(select, row, 1);
        geometry_layout->addWidget(isolate, row, 2);
        const GeometryKind k = kind;
        connect(select, &QPushButton::clicked, this, [this, k]() { emit geometrySelectRequested(k, false); });
        connect(isolate, &QPushButton::clicked, this, [this, k]() { emit geometrySelectRequested(k, true); });
        ++row;
    }
    geometry_layout->setColumnStretch(0, 1);
    geometry_section->addBodyWidget(geometry_kinds);
    result_layout->addWidget(geometry_section);

    auto* report_row = new QWidget(result_box_);
    auto* report_layout = new QHBoxLayout(report_row);
    report_layout->setContentsMargins(0, 0, 0, 0);
    auto* report_button = new QPushButton("Show Report", report_row);
    describe(report_button, "Show Report",
             "A table of every change, one row per change: attributes, properties (old and new values), "
             "geometry, relationships. Click a row to select the element, double click to isolate it; "
             "it can be filtered, sorted and exported to CSV.");
    auto* compare_button = new QPushButton("Compare Element", report_row);
    describe(compare_button, "Compare Element",
             "The selected element side by side: its Before and After versions, field by field "
             "(attributes, then every property), the differences coloured.");
    report_layout->addWidget(report_button);
    report_layout->addWidget(compare_button);
    result_layout->addWidget(report_row);
    layout->addWidget(result_box_);
    layout->addStretch(1);

    setBeforeTransparent(false);
    setResultAvailable(false);

    connect(ids_button, &QPushButton::clicked, this, &DiffPanel::chooseIds);
    connect(ids_clear_button_, &QPushButton::clicked, this, [this]() {
        ids_path_.clear();
        ids_label_->setText("IDS shortlist: none");
        ids_label_->setToolTip("<b>IDS shortlist</b><br>None: every property set and quantity set is compared.");
        ids_clear_button_->setEnabled(false);
    });
    connect(run_button_, &QPushButton::clicked, this, &DiffPanel::runRequested);
    connect(export_button, &QPushButton::clicked, this, &DiffPanel::exportRequested);
    connect(clear_button, &QPushButton::clicked, this, &DiffPanel::clearRequested);
    connect(swap_button_, &QPushButton::clicked, this, &DiffPanel::swapTransparencyRequested);
    connect(report_button, &QPushButton::clicked, this, &DiffPanel::reportRequested);
    connect(compare_button, &QPushButton::clicked, this, &DiffPanel::compareRequested);
}

void DiffPanel::setModels(const QList<QPair<std::uint32_t, QString>>& models) {
    fillModelCombo(before_combo_, models);
    fillModelCombo(after_combo_, models);
    // Nothing chosen yet: the first two models, in load order.
    if (beforeModel() == 0 && afterModel() == 0 && models.size() >= 2) {
        before_combo_->setCurrentIndex(1);
        after_combo_->setCurrentIndex(2);
    }
}

std::uint32_t DiffPanel::beforeModel() const {
    return before_combo_->currentData().value<std::uint32_t>();
}

std::uint32_t DiffPanel::afterModel() const {
    return after_combo_->currentData().value<std::uint32_t>();
}

std::set<std::string> DiffPanel::relationships() const {
    std::set<std::string> names;
    for (const auto& [name, check] : relationship_checks_) {
        if (check->isChecked()) names.insert(name);
    }
    return names;
}

bool DiffPanel::isShallow() const {
    return shallow_check_->isChecked();
}

QString DiffPanel::propertySet() const {
    return property_field_->propertySet().trimmed();
}

QString DiffPanel::propertyName() const {
    return property_field_->propertyName().trimmed();
}

void DiffPanel::setRunning(bool running) {
    run_button_->setEnabled(!running);
    run_button_->setText(running ? "Running…" : "Run");
}

void DiffPanel::setSummary(const QString& text) {
    summary_label_->setText(text);
}

void DiffPanel::setResultAvailable(bool available) {
    result_box_->setEnabled(available);
}

void DiffPanel::setBeforeTransparent(bool before) {
    swap_button_->setText(before ? "Transparent: Before" : "Transparent: After");
}

void DiffPanel::chooseIds() {
    QSettings settings;
    const QString path = QFileDialog::getOpenFileName(this, "IDS shortlist", settings.value(kLastIdsDirectoryKey).toString(),
                                                      "IDS (*.ids *.xml);;All files (*)");
    if (path.isEmpty()) return;
    settings.setValue(kLastIdsDirectoryKey, QFileInfo(path).absolutePath());
    ids_path_ = path;
    ids_label_->setText(QString("IDS shortlist: %1").arg(QFileInfo(path).fileName()));

    ids_clear_button_->setEnabled(true);
    ids_label_->setToolTip(QString("<b>IDS shortlist: %1</b><br>Only the properties its specifications require "
                                   "are compared, on the elements they apply to.<br>%2")
                               .arg(QFileInfo(path).fileName().toHtmlEscaped(), path.toHtmlEscaped()));
}

} // namespace bonsaiviewer::modules::diff
