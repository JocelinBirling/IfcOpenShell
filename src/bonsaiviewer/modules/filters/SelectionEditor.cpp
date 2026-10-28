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

#include "SelectionEditor.h"

#include "FacetClipboard.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QShortcut>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStackedWidget>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>

namespace bonsaiviewer::modules::filters {

using bonsaiviewer::services::IdsBridge;
using bonsaiviewer::services::IdsFacet;
using bonsaiviewer::services::IdsValue;

namespace {

// The facet pages, by facet type.
enum FacetPage {
    kNoFacetPage = 0,
    kEntityPage,
    kPropertyPage,
    kAttributePage,
    kClassificationPage,
    kMaterialPage,
    kPartOfPage,
};

FacetPage pageOf(IdsFacet::Type type) {
    switch (type) {
    case IdsFacet::Type::Entity: return kEntityPage;
    case IdsFacet::Type::Property: return kPropertyPage;
    case IdsFacet::Type::Attribute: return kAttributePage;
    case IdsFacet::Type::Classification: return kClassificationPage;
    case IdsFacet::Type::Material: return kMaterialPage;
    case IdsFacet::Type::PartOf: return kPartOfPage;
    }
    return kNoFacetPage;
}

QString facetLabel(const IdsFacet& facet) {
    return QString("%1  %2").arg(IdsFacet::typeLabel(facet.type), facet.summary());
}

bool valueIsComplete(const IdsValue& value) {
    switch (value.kind) {
    case IdsValue::Kind::Any: return true;
    case IdsValue::Kind::Simple: return !value.simple.trimmed().isEmpty();
    case IdsValue::Kind::Enumeration: return !value.values.isEmpty();
    case IdsValue::Kind::Pattern: return !value.pattern.isEmpty();
    case IdsValue::Kind::Range:
    case IdsValue::Kind::Length: return !value.min.isEmpty() || !value.max.isEmpty();
    }
    return false;
}

} // namespace

SelectionEditor::SelectionEditor(IdsBridge& bridge, IfcPathProvider ifc_path, QWidget* parent, Mode mode)
    : QWidget(parent),
      bridge_(bridge),
      ifc_path_(std::move(ifc_path)),
      mode_(mode)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    auto* facets_side = new QWidget(splitter);
    auto* facets_layout = new QVBoxLayout(facets_side);
    facets_layout->setContentsMargins(0, 0, 0, 0);
    // The facet buttons sit above the list so a short window never hides
    // them.
    auto* facet_buttons = new QHBoxLayout();
    title_label_ = new QLabel("Facets (all must pass)", facets_side);
    facet_buttons->addWidget(title_label_, 1);
    add_facet_button_ = new QToolButton(facets_side);
    add_facet_button_->setText("+ Add");
    add_facet_button_->setToolTip("Add a facet (entity, property…)");
    add_facet_button_->setPopupMode(QToolButton::InstantPopup);
    add_facet_button_->setMenu(new QMenu(add_facet_button_));
    remove_facet_button_ = new QPushButton("Remove", facets_side);
    facet_buttons->addWidget(add_facet_button_);
    facet_buttons->addWidget(remove_facet_button_);
    facets_layout->addLayout(facet_buttons);
    facet_list_ = new QListWidget(facets_side);
    facet_list_->setWordWrap(true);
    facet_list_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    facets_layout->addWidget(facet_list_, 1);

    facet_pages_ = new QStackedWidget(splitter);
    auto* empty_page = new QLabel("Select a facet on the left, or add one.", facet_pages_);
    empty_page->setAlignment(Qt::AlignCenter);
    empty_page->setProperty("textRole", "secondary");
    facet_pages_->addWidget(empty_page); // kNoFacetPage
    entity_page_ = new EntityFacetPage(bridge_, ifc_path_, facet_pages_);
    property_page_ = new PropertyFacetPage(bridge_, ifc_path_, [this]() { return entityFacet(); },
                                           facet_pages_);
    attribute_page_ = new SimpleFacetPage(IdsFacet::Type::Attribute, facet_pages_);
    classification_page_ = new SimpleFacetPage(IdsFacet::Type::Classification, facet_pages_);
    material_page_ = new SimpleFacetPage(IdsFacet::Type::Material, facet_pages_);
    part_of_page_ = new PartOfFacetPage(bridge_, ifc_path_, facet_pages_);
    for (QWidget* page : std::initializer_list<QWidget*>{entity_page_, property_page_, attribute_page_,
                                                         classification_page_, material_page_, part_of_page_}) {
        auto* scroll = new QScrollArea(facet_pages_);
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // the page follows the width
        scroll->setWidget(page);
        facet_pages_->addWidget(scroll); // in the FacetPage order
    }
    splitter->addWidget(facets_side);
    splitter->addWidget(facet_pages_);
    // Extra width goes to the facet page, not to the facet list.
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({190, 700});
    splitter->setMinimumHeight(180);
    layout->addWidget(splitter, 1);

    auto* preview_row = new QHBoxLayout();
    preview_label_ = new QLabel(this);
    preview_label_->setWordWrap(true);
    preview_button_ = new QPushButton("Test", this);
    preview_button_->setToolTip("Count the elements this selection holds in the active model");
    isolate_button_ = new QPushButton("Isolate in the view", this);
    isolate_button_->setToolTip("Isolate the previewed elements in the viewer");
    isolate_button_->setEnabled(false);
    preview_row->addWidget(new QLabel("<b>Preview</b>", this));
    preview_row->addWidget(preview_label_, 1);
    preview_row->addWidget(preview_button_);
    preview_row->addWidget(isolate_button_);
    layout->addLayout(preview_row);
    if (mode_ == Mode::Clause) {
        for (QWidget* widget : std::initializer_list<QWidget*>{preview_label_, preview_button_, isolate_button_}) {
            widget->hide();
        }
        preview_row->itemAt(0)->widget()->hide();
    }

    preview_timer_ = new QTimer(this);
    preview_timer_->setSingleShot(true);
    preview_timer_->setInterval(900);
    connect(preview_timer_, &QTimer::timeout, this, &SelectionEditor::runPreview);
    connect(preview_button_, &QPushButton::clicked, this, &SelectionEditor::runPreview);
    connect(isolate_button_, &QPushButton::clicked, this, [this]() {
        if (!preview_ids_.empty()) emit isolateRequested(preview_ids_);
    });

    // Facet wiring: the pages write back into the current facet.
    connect(facet_list_, &QListWidget::currentRowChanged, this, [this](int row) {
        storeFacetFromPage();
        showFacet(row);
    });
    for (const FacetTypeEntry& entry : facetTypes()) {
        QAction* action = add_facet_button_->menu()->addAction(entry.label);
        action->setEnabled(entry.available);
        if (entry.available) {
            connect(action, &QAction::triggered, this, [this, type = entry.type]() { addFacet(type); });
        }
    }
    connect(remove_facet_button_, &QPushButton::clicked, this, [this]() {
        if (current_facet_ >= 0) removeFacet(current_facet_);
    });
    const auto on_page_changed = [this]() {
        storeFacetFromPage();
        if (current_facet_ >= 0 && current_facet_ < facet_list_->count()) {
            facet_list_->item(current_facet_)->setText(facetLabel(fields_.facets[current_facet_]));
        }
        schedulePreview();
        emit changed();
    };
    connect(entity_page_, &EntityFacetPage::changed, this, on_page_changed);
    connect(property_page_, &PropertyFacetPage::changed, this, on_page_changed);
    for (SimpleFacetPage* page : {attribute_page_, classification_page_, material_page_}) {
        connect(page, &SimpleFacetPage::changed, this, on_page_changed);
    }
    connect(part_of_page_, &PartOfFacetPage::changed, this, on_page_changed);

    // Copy / paste of facets (IDS XML on the clipboard).
    facet_list_->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(facet_list_, &QListWidget::customContextMenuRequested, this, &SelectionEditor::showFacetMenu);
    auto* copy_shortcut = new QShortcut(QKeySequence::Copy, facet_list_);
    copy_shortcut->setContext(Qt::WidgetShortcut);
    connect(copy_shortcut, &QShortcut::activated, this, [this]() { copyFacets(false); });
    auto* paste_shortcut = new QShortcut(QKeySequence::Paste, facet_list_);
    paste_shortcut->setContext(Qt::WidgetShortcut);
    connect(paste_shortcut, &QShortcut::activated, this, &SelectionEditor::pasteFacets);
}

IdsBridge::SelectionFields SelectionEditor::defaultFields() {
    IdsBridge::SelectionFields fields;
    fields.facets.push_back(IdsFacet::entity("IfcWall"));
    return fields;
}

QString SelectionEditor::validationError(const IdsBridge::SelectionFields& fields) {
    int entities = 0;
    for (const auto& facet : fields.facets) {
        if (facet.type == IdsFacet::Type::Entity) {
            ++entities;
            if (facet.name.isAny() || !valueIsComplete(facet.name)) return "the entity facet needs an IFC class";
            if (!valueIsComplete(facet.predefined_type)) return "the PredefinedType is incomplete";
            continue;
        }
        if (facet.type == IdsFacet::Type::Property &&
            (facet.pset.isAny() || !valueIsComplete(facet.pset) || facet.property.isAny() ||
             !valueIsComplete(facet.property))) {
            return "a property facet needs its property set and its property";
        }
        if (facet.type == IdsFacet::Type::Attribute && (facet.property.isAny() || !valueIsComplete(facet.property))) {
            return "an attribute facet needs the attribute name";
        }
        if (facet.type == IdsFacet::Type::PartOf && (facet.name.isAny() || !valueIsComplete(facet.name))) {
            return "a part of facet needs the parent class";
        }
        if (!valueIsComplete(facet.system) || !valueIsComplete(facet.predefined_type)) {
            return QString("%1 is incomplete (empty list, pattern, range or length)").arg(facet.summary());
        }
        if (facet.cardinality != "prohibited" && !valueIsComplete(facet.value)) {
            return QString("the value of %1 is incomplete (empty list, pattern, range or length)")
                .arg(facet.summary());
        }
    }
    if (entities != 1) return "the selection needs exactly one entity facet";
    return {};
}

void SelectionEditor::setFields(const IdsBridge::SelectionFields& fields) {
    fields_ = fields;
    syncFacetList();
    selectFacet(facet_list_->count() > 0 ? 0 : -1);
    preview_ids_.clear();
    isolate_button_->setEnabled(false);
    schedulePreview();
}

IdsBridge::SelectionFields SelectionEditor::fields() {
    storeFacetFromPage();
    return fields_;
}

void SelectionEditor::syncFacetList() {
    // Callers store the page first and select a facet after (selectFacet).
    current_facet_ = -1;
    {
        const QSignalBlocker blocker(facet_list_);
        facet_list_->clear();
        for (const auto& facet : fields_.facets) facet_list_->addItem(facetLabel(facet));
    }
    updateAddFacetMenu();
}

void SelectionEditor::selectFacet(int index) {
    if (index >= facet_list_->count()) index = facet_list_->count() - 1;
    {
        const QSignalBlocker blocker(facet_list_);
        facet_list_->setCurrentRow(index);
    }
    showFacet(index);
}

void SelectionEditor::showFacet(int index) {
    current_facet_ = -1;
    if (index < 0 || index >= static_cast<int>(fields_.facets.size())) {
        facet_pages_->setCurrentIndex(kNoFacetPage);
        remove_facet_button_->setEnabled(false);
        return;
    }
    const IdsFacet& facet = fields_.facets[index];
    switch (facet.type) {
    case IdsFacet::Type::Entity: entity_page_->setFacet(facet); break;
    case IdsFacet::Type::Property: property_page_->setFacet(facet); break;
    case IdsFacet::Type::Attribute: attribute_page_->setFacet(facet); break;
    case IdsFacet::Type::Classification: classification_page_->setFacet(facet); break;
    case IdsFacet::Type::Material: material_page_->setFacet(facet); break;
    case IdsFacet::Type::PartOf: part_of_page_->setFacet(facet); break;
    }
    facet_pages_->setCurrentIndex(pageOf(facet.type));
    current_facet_ = index;
    remove_facet_button_->setEnabled(true);
}

void SelectionEditor::storeFacetFromPage() {
    if (current_facet_ < 0 || current_facet_ >= static_cast<int>(fields_.facets.size())) return;
    auto& facet = fields_.facets[current_facet_];
    switch (facet.type) {
    case IdsFacet::Type::Entity: facet = entity_page_->facet(); break;
    case IdsFacet::Type::Property: facet = property_page_->facet(); break;
    case IdsFacet::Type::Attribute: facet = attribute_page_->facet(); break;
    case IdsFacet::Type::Classification: facet = classification_page_->facet(); break;
    case IdsFacet::Type::Material: facet = material_page_->facet(); break;
    case IdsFacet::Type::PartOf: facet = part_of_page_->facet(); break;
    }
}

void SelectionEditor::addFacet(IdsFacet::Type type) {
    storeFacetFromPage();
    auto& facets = fields_.facets;
    const bool has_entity = std::any_of(facets.begin(), facets.end(), [](const IdsFacet& f) {
        return f.type == IdsFacet::Type::Entity;
    });
    if (type == IdsFacet::Type::Entity && !has_entity) {
        facets.insert(facets.begin(), IdsFacet::entity("IfcWall")); // the entity stays first
        syncFacetList();
        selectFacet(0);
    } else if (type == IdsFacet::Type::Entity && mode_ == Mode::Selection) {
        return;
    } else if (type == IdsFacet::Type::Entity) {
        facets.push_back(IdsFacet::entity("IfcWall"));
        syncFacetList();
        selectFacet(facet_list_->count() - 1);
    } else {
        IdsFacet facet = IdsFacet::ofType(type);
        if (type == IdsFacet::Type::Property) {
            facet.pset = IdsValue::simpleValue({});
            facet.property = IdsValue::simpleValue({});
        }
        if (type == IdsFacet::Type::Attribute) facet.property = IdsValue::simpleValue("Name");
        if (type == IdsFacet::Type::PartOf) {
            facet.name = IdsValue::simpleValue("IfcBuildingStorey");
            facet.include_subtypes = false;
        }
        facets.push_back(facet);
        syncFacetList();
        selectFacet(facet_list_->count() - 1);
    }
    schedulePreview();
    emit changed();
}

void SelectionEditor::removeFacet(int index) {
    auto& facets = fields_.facets;
    if (index < 0 || index >= static_cast<int>(facets.size())) return;
    current_facet_ = -1; // do not store the page back into the removed facet
    facets.erase(facets.begin() + index);
    syncFacetList();
    selectFacet(facets.empty() ? -1 : std::min(index, static_cast<int>(facets.size()) - 1));
    schedulePreview();
    emit changed();
}

void SelectionEditor::copyFacets(bool all) {
    storeFacetFromPage();
    std::vector<IdsFacet> facets;
    if (all) {
        facets = fields_.facets;
    } else if (current_facet_ >= 0 && current_facet_ < static_cast<int>(fields_.facets.size())) {
        facets.push_back(fields_.facets[current_facet_]);
    }
    facet_clipboard::copy(this, facets);
}

void SelectionEditor::pasteFacets() {
    std::vector<IdsFacet> pasted = facet_clipboard::paste(this);
    if (pasted.empty()) return;
    storeFacetFromPage();
    auto& facets = fields_.facets;
    bool has_entity = std::any_of(facets.begin(), facets.end(), [](const IdsFacet& f) {
        return f.type == IdsFacet::Type::Entity;
    });
    int skipped = 0;
    int added = 0;
    for (const IdsFacet& facet : pasted) {
        if (facet.type == IdsFacet::Type::Entity) {
            if (has_entity && mode_ == Mode::Selection) {
                ++skipped; // a selection has one entity facet
                continue;
            }
            facets.insert(facets.begin(), facet);
            has_entity = true;
        } else {
            facets.push_back(facet);
        }
        ++added;
    }
    if (skipped > 0) {
        QMessageBox::information(this, "Paste facets",
                                 "The selection already has an entity facet: the pasted entity facet was "
                                 "left out. Remove the current one first to replace it.");
    }
    if (added == 0) return;
    syncFacetList();
    selectFacet(facet_list_->count() - 1);
    schedulePreview();
    emit changed();
}

void SelectionEditor::showFacetMenu(const QPoint& position) {
    QMenu menu(this);
    QAction* copy = menu.addAction("Copy facet");
    copy->setShortcut(QKeySequence::Copy);
    copy->setEnabled(current_facet_ >= 0);
    QAction* copy_all = menu.addAction("Copy all the facets");
    copy_all->setEnabled(!fields_.facets.empty());
    QAction* paste = menu.addAction("Paste facets");
    paste->setShortcut(QKeySequence::Paste);
    paste->setEnabled(facet_clipboard::canPaste());
    QAction* chosen = menu.exec(facet_list_->viewport()->mapToGlobal(position));
    if (chosen == copy) copyFacets(false);
    else if (chosen == copy_all) copyFacets(true);
    else if (chosen == paste) pasteFacets();
}

IdsFacet SelectionEditor::entityFacet() const {
    for (const auto& facet : fields_.facets) {
        if (facet.type == IdsFacet::Type::Entity) return facet;
    }
    return entity_source_ ? entity_source_() : IdsFacet();
}

void SelectionEditor::setTitle(const QString& title) {
    title_label_->setText(title);
}

void SelectionEditor::setEntitySource(std::function<IdsFacet()> source) {
    entity_source_ = std::move(source);
}

void SelectionEditor::updateAddFacetMenu() {
    const bool has_entity = std::any_of(fields_.facets.begin(), fields_.facets.end(), [](const IdsFacet& f) {
        return f.type == IdsFacet::Type::Entity;
    });
    const auto& entries = facetTypes();
    const auto actions = add_facet_button_->menu()->actions();
    for (int i = 0; i < actions.size() && i < static_cast<int>(entries.size()); ++i) {
        const bool entity = entries[i].available && entries[i].type == IdsFacet::Type::Entity;
        actions[i]->setEnabled(entries[i].available && !(entity && has_entity && mode_ == Mode::Selection));
    }
}

void SelectionEditor::schedulePreview() {
    if (mode_ == Mode::Selection) preview_timer_->start();
}

void SelectionEditor::runPreview() {
    preview_timer_->stop();
    storeFacetFromPage();
    preview_ids_.clear();
    isolate_button_->setEnabled(false);
    const int generation = ++preview_generation_;
    const QString path = ifc_path_ ? ifc_path_() : QString();
    if (path.isEmpty()) {
        preview_label_->setText("No model loaded: the preview needs an active model.");
        return;
    }
    if (entityFacet().name.isAny()) {
        preview_label_->setText("Add an IFC class.");
        return;
    }
    preview_label_->setText("Counting…");
    QPointer<SelectionEditor> guard(this);
    bridge_.preview(path, fields_, [this, guard, generation](bool ok, std::vector<int> ids, const QString& error) {
        if (guard.isNull() || generation != preview_generation_) return; // stale
        if (!ok) {
            preview_label_->setText("⚠ " + error);
            return;
        }
        preview_label_->setText(QString("%1 element(s) selected in the active model.").arg(ids.size()));
        preview_ids_ = std::move(ids);
        isolate_button_->setEnabled(!preview_ids_.empty());
    });
}

} // namespace bonsaiviewer::modules::filters
