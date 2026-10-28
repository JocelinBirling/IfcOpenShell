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
 ********************************************************************************/

#include "FacetPages.h"

#include "../property_selection/PropertyPicker.h"
#include "ValueEditor.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPointer>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>

namespace bonsaiviewer::modules::filters {

using bonsaiviewer::services::IdsBridge;
using bonsaiviewer::services::IdsFacet;
using bonsaiviewer::services::IdsValue;

namespace {

QLabel* secondaryLabel(QWidget* parent) {
    auto* label = new QLabel(parent);
    label->setProperty("textRole", "secondary");
    label->setWordWrap(true);
    return label;
}

QString joinShort(const QStringList& values, int shown = 6) {
    if (values.size() <= shown) return values.join(", ");
    return values.mid(0, shown).join(", ") + QString(" (+%1)").arg(values.size() - shown);
}

// The class names a class value lists (empty for a pattern).
QStringList listedClasses(const IdsValue& value) {
    switch (value.kind) {
    case IdsValue::Kind::Simple: return {value.simple};
    case IdsValue::Kind::Enumeration: return value.values;
    default: return {};
    }
}

bool hasClass(const IdsFacet& entity);

// The elements whose properties are listed: the entity facet's, or every
// product when no class is chosen yet (a property can be picked first).
IdsFacet listedEntity(const IdsFacet& entity) {
    if (hasClass(entity)) return entity;
    IdsFacet all = IdsFacet::entity("IfcProduct");
    all.include_subtypes = true;
    return all;
}

bool hasClass(const IdsFacet& entity) {
    switch (entity.name.kind) {
    case IdsValue::Kind::Simple: return !entity.name.simple.isEmpty();
    case IdsValue::Kind::Enumeration: return !entity.name.values.isEmpty();
    case IdsValue::Kind::Pattern: return !entity.name.pattern.isEmpty();
    default: return false;
    }
}

QComboBox* cardinalityCombo(QWidget* parent) {
    auto* combo = new QComboBox(parent);
    combo->addItem("Required", "required");
    combo->addItem("Optional", "optional");
    combo->addItem("Prohibited", "prohibited");
    return combo;
}

void setCardinality(QComboBox* combo, const QString& cardinality) {
    const int index = combo->findData(cardinality);
    combo->setCurrentIndex(index < 0 ? 0 : index);
}

// The help under a cardinality combo; `what` names the information
// ("the attribute", "a classification"...).
QString cardinalityHelp(const QString& cardinality, const QString& what) {
    if (cardinality == "optional") {
        return QString("Elements without %1 are selected too; when present, it must match.").arg(what);
    }
    if (cardinality == "prohibited") {
        return QString("Only the elements WITHOUT %1 (matching the values) are selected.").arg(what);
    }
    return QString("%1 must exist and match.").arg(what.left(1).toUpper() + what.mid(1));
}

// The attribute names most IDS checks use, for the completion.
const QStringList& commonAttributes() {
    static const QStringList names = {"Name", "Description", "ObjectType", "Tag", "GlobalId", "LongName",
                                      "PredefinedType", "Elevation", "Identification", "LongDescription",
                                      "OperationType", "UserDefinedOperationType", "ElementType"};
    return names;
}

// IFC measure types for the data type of a property facet.
const QStringList& commonDataTypes() {
    static const QStringList names = {"IFCLABEL", "IFCTEXT", "IFCIDENTIFIER", "IFCBOOLEAN", "IFCLOGICAL",
                                      "IFCINTEGER", "IFCREAL", "IFCCOUNTMEASURE", "IFCLENGTHMEASURE",
                                      "IFCPOSITIVELENGTHMEASURE", "IFCAREAMEASURE", "IFCVOLUMEMEASURE",
                                      "IFCMASSMEASURE", "IFCPLANEANGLEMEASURE", "IFCRATIOMEASURE",
                                      "IFCPOSITIVERATIOMEASURE", "IFCTHERMALTRANSMITTANCEMEASURE",
                                      "IFCTHERMODYNAMICTEMPERATUREMEASURE", "IFCPOWERMEASURE",
                                      "IFCPRESSUREMEASURE", "IFCTIMEMEASURE", "IFCDATE", "IFCDATETIME"};
    return names;
}

} // namespace

const std::vector<FacetTypeEntry>& facetTypes() {
    static const std::vector<FacetTypeEntry> entries = {
        {"Entity (IFC class)", true, IdsFacet::Type::Entity},
        {"Property", true, IdsFacet::Type::Property},
        {"Attribute", true, IdsFacet::Type::Attribute},
        {"Classification", true, IdsFacet::Type::Classification},
        {"Material", true, IdsFacet::Type::Material},
        {"Part of", true, IdsFacet::Type::PartOf},
    };
    return entries;
}

// ---------------------------------------------------------------------------
// EntityFacetPage
// ---------------------------------------------------------------------------

EntityFacetPage::EntityFacetPage(IdsBridge& bridge, IfcPathProvider ifc_path, QWidget* parent)
    : QWidget(parent),
      bridge_(bridge),
      ifc_path_(std::move(ifc_path))
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(new QLabel("<b>Entity</b> — which IFC classes the layer selects", this));

    auto* form = new QFormLayout();
    auto* class_cell = new QWidget(this);
    auto* class_layout = new QVBoxLayout(class_cell);
    class_layout->setContentsMargins(0, 0, 0, 0);
    class_layout->setSpacing(2);
    // Value = one class, List = several classes, Pattern = class names.
    class_editor_ = new ValueEditor(ValueEditor::AllowSimple | ValueEditor::AllowEnumeration |
                                        ValueEditor::AllowPattern,
                                    class_cell);
    class_editor_->setPlaceholder("IfcWall");
    class_layout->addWidget(class_editor_);
    class_status_ = secondaryLabel(class_cell);
    class_status_->hide();
    class_layout->addWidget(class_status_);
    subtypes_check_ = new QCheckBox("Include the subtypes", class_cell);
    subtypes_check_->setToolTip("IDS matches the class exactly: without the subtypes, IfcWall does not "
                                "select IfcWallStandardCase.");
    class_layout->addWidget(subtypes_check_);
    form->addRow("IFC class", class_cell);
    covered_label_ = secondaryLabel(this);
    form->addRow("Covers", covered_label_);
    count_label_ = secondaryLabel(this);
    form->addRow("In the model", count_label_);
    layout->addLayout(form);

    auto* predefined_box = new QGroupBox("PredefinedType", this);
    auto* predefined_layout = new QVBoxLayout(predefined_box);
    predefined_editor_ = new ValueEditor(ValueEditor::AllowText, predefined_box);
    predefined_editor_->setPlaceholder("any PredefinedType");
    predefined_layout->addWidget(predefined_editor_);
    predefined_note_ = secondaryLabel(predefined_box);
    predefined_layout->addWidget(predefined_note_);
    layout->addWidget(predefined_box);
    layout->addStretch(1);

    info_timer_ = new QTimer(this);
    info_timer_->setSingleShot(true);
    info_timer_->setInterval(350);
    connect(info_timer_, &QTimer::timeout, this, &EntityFacetPage::requestClassInfo);

    connect(class_editor_, &ValueEditor::changed, this, [this]() {
        if (loading_) return;
        updateClassStatus();
        info_timer_->start();
        emit changed();
    });
    connect(subtypes_check_, &QCheckBox::toggled, this, [this](bool) {
        if (loading_) return;
        info_timer_->start();
        emit changed();
    });
    connect(predefined_editor_, &ValueEditor::changed, this, [this]() {
        if (!loading_) emit changed();
    });
}

void EntityFacetPage::setFacet(const IdsFacet& facet) {
    base_ = facet;
    loading_ = true;
    class_editor_->setValue(facet.name);
    subtypes_check_->setChecked(facet.include_subtypes);
    predefined_editor_->clearSuggestions();
    predefined_editor_->setValue(facet.predefined_type);
    covered_label_->clear();
    count_label_->clear();
    predefined_note_->clear();
    loading_ = false;
    loadClasses();
    updateClassStatus();
    requestClassInfo();
}

IdsFacet EntityFacetPage::facet() const {
    IdsFacet facet = base_; // keeps what the page does not show (URI, instructions)
    facet.type = IdsFacet::Type::Entity;
    facet.name = class_editor_->value();
    facet.include_subtypes = subtypes_check_->isChecked();
    facet.predefined_type = predefined_editor_->value();
    return facet;
}

void EntityFacetPage::loadClasses() {
    const QString path = ifc_path_ ? ifc_path_() : QString();
    if (classes_loaded_ && path == classes_path_) return;
    classes_path_ = path;
    QPointer<EntityFacetPage> guard(this);
    bridge_.classes(path, [this, guard](bool ok, const QStringList& classes, const QString&) {
        if (guard.isNull() || !ok) return;
        known_classes_ = classes;
        classes_loaded_ = true;
        class_editor_->setCompletions(classes);
        updateClassStatus();
    });
}

void EntityFacetPage::updateClassStatus() {
    const auto show = [this](const QString& text) {
        class_status_->setText(text);
        class_status_->setVisible(!text.isEmpty());
    };
    const IdsFacet current = facet();
    if (!hasClass(current)) {
        show("Give at least one class (e.g. IfcWall, IfcSlab, IfcDoor).");
        return;
    }
    if (!classes_loaded_) {
        show({});
        return;
    }
    QStringList unknown;
    for (const QString& name : listedClasses(current.name)) {
        const bool known = std::any_of(known_classes_.begin(), known_classes_.end(), [&name](const QString& c) {
            return c.compare(name, Qt::CaseInsensitive) == 0;
        });
        if (!known) unknown << name;
    }
    if (!unknown.isEmpty()) {
        show(QString("⚠ Not in the model's schema: %1.").arg(unknown.join(", ")));
    } else {
        show(info_warning_.isEmpty() ? QString() : QString("⚠ ") + info_warning_);
    }
}

void EntityFacetPage::requestClassInfo() {
    const IdsFacet current = facet();
    const int generation = ++info_generation_;
    info_warning_.clear();
    if (!hasClass(current)) {
        covered_label_->clear();
        count_label_->clear();
        return;
    }
    const QString path = ifc_path_ ? ifc_path_() : QString();
    QPointer<EntityFacetPage> guard(this);
    bridge_.classInfo(path, current.name, current.include_subtypes,
                      [this, guard, generation, path](bool ok, const IdsBridge::ClassInfo& info,
                                                      const QString& error) {
        if (guard.isNull() || generation != info_generation_) return; // stale reply
        if (!ok) {
            covered_label_->setText(error);
            count_label_->clear();
            predefined_editor_->setSuggestions({});
            return;
        }
        info_warning_ = info.warning;
        updateClassStatus();
        covered_label_->setText(info.classes.isEmpty() ? QString("—") : joinShort(info.classes));
        count_label_->setText(info.count >= 0 ? QString("%1 element(s)").arg(info.count)
                                              : (path.isEmpty() ? QString("no model loaded")
                                                                : QString()));
        predefined_editor_->setSuggestions(info.predefined_types);
        predefined_note_->setText(
            info.predefined_types.isEmpty()
                ? QString("No PredefinedType for these classes in this schema.")
                : QString("Values of the schema: %1. USERDEFINED matches the ObjectType.")
                      .arg(joinShort(info.predefined_types, 8)));
    });
}

// ---------------------------------------------------------------------------
// PropertyFacetPage
// ---------------------------------------------------------------------------

PropertyFacetPage::PropertyFacetPage(IdsBridge& bridge,
                                     IfcPathProvider ifc_path,
                                     std::function<IdsFacet()> entity,
                                     QWidget* parent)
    : QWidget(parent),
      bridge_(bridge),
      ifc_path_(std::move(ifc_path)),
      entity_(std::move(entity))
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    auto* title = new QLabel("<b>Property</b> — a property (or quantity) the elements must carry", this);
    title->setWordWrap(true);
    layout->addWidget(title);
    auto* browse_row = new QHBoxLayout();
    status_label_ = secondaryLabel(this);
    browse_row->addWidget(status_label_, 1);
    browse_button_ = new QPushButton("Browse properties…", this);
    browse_button_->setToolTip("Search the properties of the model and double-click one to use it");
    browse_button_->setEnabled(false);
    browse_row->addWidget(browse_button_);
    layout->addLayout(browse_row);

    auto* form = new QFormLayout();
    pset_editor_ = new ValueEditor(ValueEditor::AllowText, this);
    pset_editor_->setPlaceholder("Pset_WallCommon");
    form->addRow("Property set", pset_editor_);
    property_editor_ = new ValueEditor(ValueEditor::AllowText, this);
    property_editor_->setPlaceholder("FireRating");
    form->addRow("Property", property_editor_);
    data_type_combo_ = new QComboBox(this);
    data_type_combo_->setEditable(true);
    data_type_combo_->addItem(QString());
    data_type_combo_->addItems(commonDataTypes());
    data_type_combo_->setToolTip("The IFC data type the value must have (e.g. IFCLABEL); empty = any type");
    form->addRow("Data type", data_type_combo_);
    cardinality_combo_ = cardinalityCombo(this);
    form->addRow("Cardinality", cardinality_combo_);
    cardinality_help_ = secondaryLabel(this);
    form->addRow("", cardinality_help_);
    layout->addLayout(form);

    auto* value_box = new QGroupBox("Value", this);
    auto* value_layout = new QVBoxLayout(value_box);
    value_editor_ = new ValueEditor(ValueEditor::AllowAll, value_box);
    value_editor_->setPlaceholder("any value (the property only has to exist)");
    value_layout->addWidget(value_editor_);
    layout->addWidget(value_box);
    layout->addStretch(1);

    values_timer_ = new QTimer(this);
    values_timer_->setSingleShot(true);
    values_timer_->setInterval(400);
    connect(values_timer_, &QTimer::timeout, this, &PropertyFacetPage::loadValues);

    connect(browse_button_, &QPushButton::clicked, this, &PropertyFacetPage::openPicker);
    connect(pset_editor_, &ValueEditor::changed, this, [this]() {
        if (loading_) return;
        updatePropertySuggestions();
        values_timer_->start();
        emit changed();
    });
    connect(property_editor_, &ValueEditor::changed, this, [this]() {
        if (loading_) return;
        values_timer_->start();
        emit changed();
    });
    connect(value_editor_, &ValueEditor::changed, this, [this]() {
        if (!loading_) emit changed();
    });
    connect(value_editor_, &ValueEditor::reloadRequested, this, &PropertyFacetPage::loadValues);
    connect(cardinality_combo_, &QComboBox::currentIndexChanged, this, [this](int) {
        updateCardinalityHelp();
        if (!loading_) emit changed();
    });
    connect(data_type_combo_, &QComboBox::currentTextChanged, this, [this](const QString&) {
        if (!loading_) emit changed();
    });
}

void PropertyFacetPage::setFacet(const IdsFacet& facet) {
    base_ = facet;
    loading_ = true;
    pset_editor_->clearSuggestions();
    property_editor_->clearSuggestions();
    value_editor_->clearSuggestions();
    pset_editor_->setValue(facet.pset);
    property_editor_->setValue(facet.property);
    value_editor_->setValue(facet.value);
    data_type_combo_->setCurrentText(facet.data_type);
    setCardinality(cardinality_combo_, facet.cardinality);
    loading_ = false;
    updateCardinalityHelp();
    loadProperties();
}

IdsFacet PropertyFacetPage::facet() const {
    IdsFacet facet = base_; // keeps what the page does not show (URI, instructions)
    facet.type = IdsFacet::Type::Property;
    facet.pset = pset_editor_->value();
    facet.property = property_editor_->value();
    facet.value = value_editor_->value();
    facet.data_type = data_type_combo_->currentText().trimmed().toUpper();
    facet.cardinality = cardinality_combo_->currentData().toString();
    return facet;
}

void PropertyFacetPage::updateCardinalityHelp() {
    const QString cardinality = cardinality_combo_->currentData().toString();
    if (cardinality == "optional") {
        cardinality_help_->setText("Elements without the property are selected too; when present, "
                                   "its value must match.");
    } else if (cardinality == "prohibited") {
        cardinality_help_->setText("Only the elements WITHOUT this property are selected (the value is "
                                   "ignored).");
    } else {
        cardinality_help_->setText("The property must exist and its value must match.");
    }
    value_editor_->setEnabled(cardinality != "prohibited");
}

void PropertyFacetPage::loadProperties() {
    const QString path = ifc_path_ ? ifc_path_() : QString();
    const int generation = ++properties_generation_;
    psets_.clear();
    samples_.clear();
    browse_button_->setEnabled(false);
    value_editor_->setReloadVisible(!path.isEmpty());
    if (path.isEmpty()) {
        status_label_->setText("No model loaded: type the names freely.");
        return;
    }
    const IdsFacet entity = listedEntity(entity_ ? entity_() : IdsFacet());
    status_label_->setText("Reading the properties of the model…");
    QPointer<PropertyFacetPage> guard(this);
    bridge_.properties(path, entity, [this, guard, generation, entity](bool ok,
                                                                       const IdsBridge::PropertyMap& psets,
                                                                       const IdsBridge::SampleMap& samples,
                                                                       const QString& error) {
        if (guard.isNull() || generation != properties_generation_) return;
        if (!ok) {
            status_label_->setText(error);
            return;
        }
        psets_ = psets;
        samples_ = samples;
        QList<QPair<QString, int>> sets;
        for (auto it = psets.begin(); it != psets.end(); ++it) {
            int count = 0;
            for (int value : it.value()) count = std::max(count, value);
            sets.append({it.key(), count});
        }
        std::sort(sets.begin(), sets.end(), [](const auto& a, const auto& b) { return a.second > b.second; });
        pset_editor_->setModelValues(sets);
        status_label_->setText(QString("%1 property set(s) found on the %2 element(s).")
                                   .arg(psets.size())
                                   .arg(entity.summary()));
        browse_button_->setEnabled(!psets.isEmpty());
        updatePropertySuggestions();
        loadValues();
    });
}

void PropertyFacetPage::openPicker() {
    const IdsFacet entity = listedEntity(entity_ ? entity_() : IdsFacet());
    auto* picker = new property_selection::PropertyPicker(psets_, samples_,
                                      QString("Properties found on the %1 elements of the active model.")
                                          .arg(entity.summary()),
                                      this);
    picker->setAttribute(Qt::WA_DeleteOnClose);
    connect(picker, &property_selection::PropertyPicker::picked, this, [this](const QString& pset, const QString& property) {
        loading_ = true;
        pset_editor_->setValue(IdsValue::simpleValue(pset));
        property_editor_->setValue(IdsValue::simpleValue(property));
        loading_ = false;
        updatePropertySuggestions();
        loadValues();
        emit changed();
    });
    picker->open();
}

void PropertyFacetPage::updatePropertySuggestions() {
    const IdsValue pset = pset_editor_->value();
    QList<QPair<QString, int>> properties;
    if (pset.kind == IdsValue::Kind::Simple && psets_.contains(pset.simple)) {
        const auto& names = psets_[pset.simple];
        for (auto it = names.begin(); it != names.end(); ++it) properties.append({it.key(), it.value()});
        std::sort(properties.begin(), properties.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });
    }
    property_editor_->setModelValues(properties);
}

void PropertyFacetPage::loadValues() {
    const QString path = ifc_path_ ? ifc_path_() : QString();
    const IdsValue pset = pset_editor_->value();
    const IdsValue property = property_editor_->value();
    const int generation = ++values_generation_;
    if (path.isEmpty() || pset.kind != IdsValue::Kind::Simple || property.kind != IdsValue::Kind::Simple) {
        return;
    }
    const IdsFacet entity = listedEntity(entity_ ? entity_() : IdsFacet());
    QPointer<PropertyFacetPage> guard(this);
    bridge_.values(path, entity, pset.simple, property.simple,
                   [this, guard, generation](bool ok, const IdsBridge::ValueStats& stats, const QString&) {
        if (guard.isNull() || generation != values_generation_ || !ok) return;
        value_editor_->setModelValues(stats.values, stats.missing);
        if (stats.base == "double" || stats.base == "integer") {
            value_editor_->setRangeBase(stats.base);
        }
    });
}

// ---------------------------------------------------------------------------
// SimpleFacetPage — attribute, classification, material
// ---------------------------------------------------------------------------

SimpleFacetPage::SimpleFacetPage(IdsFacet::Type type, QWidget* parent)
    : QWidget(parent),
      type_(type)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    auto* form = new QFormLayout();
    QString title;
    switch (type) {
    case IdsFacet::Type::Attribute:
        title = "<b>Attribute</b> — a direct IFC attribute of the elements (Name, Description, Tag…)";
        name_editor_ = new ValueEditor(ValueEditor::AllowText, this);
        name_editor_->setPlaceholder("Name");
        name_editor_->setCompletions(commonAttributes());
        form->addRow("Attribute", name_editor_);
        break;
    case IdsFacet::Type::Classification:
        title = "<b>Classification</b> — a classification reference of the elements (or of their type)";
        name_editor_ = new ValueEditor(ValueEditor::AllowText, this);
        name_editor_->setPlaceholder("any classification system (e.g. Uniclass 2015)");
        form->addRow("System", name_editor_);
        break;
    default:
        title = "<b>Material</b> — the material of the elements (or of their type): material, layer, "
                "profile or constituent name or category";
        break;
    }
    auto* title_label = new QLabel(title, this);
    title_label->setWordWrap(true);
    layout->addWidget(title_label);
    value_editor_ = new ValueEditor(type == IdsFacet::Type::Attribute ? ValueEditor::AllowAll : ValueEditor::AllowText,
                                    this);
    value_editor_->setPlaceholder(type == IdsFacet::Type::Attribute      ? "any value (the attribute only has to be set)"
                                  : type == IdsFacet::Type::Classification ? "any reference (e.g. Ss_25_10)"
                                                                           : "any material (e.g. Concrete)");
    form->addRow(type == IdsFacet::Type::Classification ? "Reference" : type == IdsFacet::Type::Material ? "Material"
                                                                                                         : "Value",
                 value_editor_);
    cardinality_combo_ = cardinalityCombo(this);
    form->addRow("Cardinality", cardinality_combo_);
    cardinality_help_ = secondaryLabel(this);
    form->addRow("", cardinality_help_);
    layout->addLayout(form);
    layout->addStretch(1);

    const auto emit_changed = [this]() {
        if (!loading_) emit changed();
    };
    if (name_editor_) connect(name_editor_, &ValueEditor::changed, this, emit_changed);
    connect(value_editor_, &ValueEditor::changed, this, emit_changed);
    connect(cardinality_combo_, &QComboBox::currentIndexChanged, this, [this, emit_changed](int) {
        updateCardinalityHelp();
        emit_changed();
    });
    updateCardinalityHelp();
}

void SimpleFacetPage::setFacet(const IdsFacet& facet) {
    base_ = facet;
    loading_ = true;
    if (type_ == IdsFacet::Type::Attribute) name_editor_->setValue(facet.property);
    if (type_ == IdsFacet::Type::Classification) name_editor_->setValue(facet.system);
    value_editor_->setValue(facet.value);
    setCardinality(cardinality_combo_, facet.cardinality);
    loading_ = false;
    updateCardinalityHelp();
}

IdsFacet SimpleFacetPage::facet() const {
    IdsFacet facet = base_; // keeps what the page does not show (URI, instructions)
    facet.type = type_;
    if (type_ == IdsFacet::Type::Attribute) facet.property = name_editor_->value();
    if (type_ == IdsFacet::Type::Classification) facet.system = name_editor_->value();
    facet.value = value_editor_->value();
    facet.cardinality = cardinality_combo_->currentData().toString();
    return facet;
}

void SimpleFacetPage::updateCardinalityHelp() {
    const QString what = type_ == IdsFacet::Type::Attribute        ? "the attribute"
                         : type_ == IdsFacet::Type::Classification ? "a matching classification"
                                                                   : "a matching material";
    cardinality_help_->setText(cardinalityHelp(cardinality_combo_->currentData().toString(), what));
}

// ---------------------------------------------------------------------------
// PartOfFacetPage
// ---------------------------------------------------------------------------

PartOfFacetPage::PartOfFacetPage(IdsBridge& bridge, IfcPathProvider ifc_path, QWidget* parent)
    : QWidget(parent),
      bridge_(bridge),
      ifc_path_(std::move(ifc_path))
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    auto* title = new QLabel("<b>Part of</b> — the elements must be part of an element of this class "
                             "(storey, assembly, group, system, host element…)",
                             this);
    title->setWordWrap(true);
    layout->addWidget(title);

    auto* form = new QFormLayout();
    relation_combo_ = new QComboBox(this);
    relation_combo_->addItem("Any relation (the whole parent chain)", QString());
    relation_combo_->addItem("Aggregation (IfcRelAggregates)", "IFCRELAGGREGATES");
    relation_combo_->addItem("Group / system (IfcRelAssignsToGroup)", "IFCRELASSIGNSTOGROUP");
    relation_combo_->addItem("Spatial container (IfcRelContainedInSpatialStructure)",
                             "IFCRELCONTAINEDINSPATIALSTRUCTURE");
    relation_combo_->addItem("Nesting (IfcRelNests)", "IFCRELNESTS");
    relation_combo_->addItem("Void / fill (IfcRelVoidsElement, IfcRelFillsElement)",
                             "IFCRELVOIDSELEMENT IFCRELFILLSELEMENT");
    form->addRow("Relation", relation_combo_);

    auto* class_cell = new QWidget(this);
    auto* class_layout = new QVBoxLayout(class_cell);
    class_layout->setContentsMargins(0, 0, 0, 0);
    class_layout->setSpacing(2);
    class_editor_ = new ValueEditor(ValueEditor::AllowSimple | ValueEditor::AllowEnumeration |
                                        ValueEditor::AllowPattern,
                                    class_cell);
    class_editor_->setPlaceholder("IfcBuildingStorey");
    class_layout->addWidget(class_editor_);
    subtypes_check_ = new QCheckBox("Include the subtypes", class_cell);
    class_layout->addWidget(subtypes_check_);
    form->addRow("Parent class", class_cell);
    predefined_editor_ = new ValueEditor(ValueEditor::AllowText, this);
    predefined_editor_->setPlaceholder("any PredefinedType");
    form->addRow("PredefinedType", predefined_editor_);
    cardinality_combo_ = cardinalityCombo(this);
    form->addRow("Cardinality", cardinality_combo_);
    cardinality_help_ = secondaryLabel(this);
    form->addRow("", cardinality_help_);
    layout->addLayout(form);
    layout->addStretch(1);

    const auto emit_changed = [this]() {
        if (!loading_) emit changed();
    };
    connect(relation_combo_, &QComboBox::currentIndexChanged, this, [emit_changed](int) { emit_changed(); });
    connect(class_editor_, &ValueEditor::changed, this, emit_changed);
    connect(subtypes_check_, &QCheckBox::toggled, this, [emit_changed](bool) { emit_changed(); });
    connect(predefined_editor_, &ValueEditor::changed, this, emit_changed);
    connect(cardinality_combo_, &QComboBox::currentIndexChanged, this, [this, emit_changed](int) {
        updateCardinalityHelp();
        emit_changed();
    });
    updateCardinalityHelp();
}

void PartOfFacetPage::setFacet(const IdsFacet& facet) {
    base_ = facet;
    loading_ = true;
    const int relation = relation_combo_->findData(facet.relation.toUpper());
    relation_combo_->setCurrentIndex(relation < 0 ? 0 : relation);
    class_editor_->setValue(facet.name);
    subtypes_check_->setChecked(facet.include_subtypes);
    predefined_editor_->setValue(facet.predefined_type);
    setCardinality(cardinality_combo_, facet.cardinality);
    loading_ = false;
    updateCardinalityHelp();
    loadClasses();
}

IdsFacet PartOfFacetPage::facet() const {
    IdsFacet facet = base_; // keeps what the page does not show (URI, instructions)
    facet.type = IdsFacet::Type::PartOf;
    facet.relation = relation_combo_->currentData().toString();
    facet.name = class_editor_->value();
    facet.include_subtypes = subtypes_check_->isChecked();
    facet.predefined_type = predefined_editor_->value();
    facet.cardinality = cardinality_combo_->currentData().toString();
    return facet;
}

void PartOfFacetPage::loadClasses() {
    const QString path = ifc_path_ ? ifc_path_() : QString();
    if (classes_loaded_ && path == classes_path_) return;
    classes_path_ = path;
    QPointer<PartOfFacetPage> guard(this);
    bridge_.classes(path, [this, guard](bool ok, const QStringList& classes, const QString&) {
        if (guard.isNull() || !ok) return;
        classes_loaded_ = true;
        class_editor_->setCompletions(classes);
    });
}

void PartOfFacetPage::updateCardinalityHelp() {
    cardinality_help_->setText(cardinalityHelp(cardinality_combo_->currentData().toString(), "the relation"));
}

} // namespace bonsaiviewer::modules::filters
