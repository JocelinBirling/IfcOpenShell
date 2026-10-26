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

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
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
};

QString facetLabel(const IdsFacet& facet) {
    return QString("%1  %2").arg(facet.type == IdsFacet::Type::Entity ? "Entity" : "Property",
                                 facet.summary());
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

SelectionEditor::SelectionEditor(IdsBridge& bridge, IfcPathProvider ifc_path, QWidget* parent)
    : QWidget(parent),
      bridge_(bridge),
      ifc_path_(std::move(ifc_path))
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
    facet_buttons->addWidget(new QLabel("Facets (all must pass)", facets_side), 1);
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
    for (QWidget* page : {static_cast<QWidget*>(entity_page_), static_cast<QWidget*>(property_page_)}) {
        auto* scroll = new QScrollArea(facet_pages_);
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // the page follows the width
        scroll->setWidget(page);
        facet_pages_->addWidget(scroll); // kEntityPage, kPropertyPage
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
        } else {
            if (facet.pset.isAny() || !valueIsComplete(facet.pset) || facet.property.isAny() ||
                !valueIsComplete(facet.property)) {
                return "a property facet needs its property set and its property";
            }
            if (facet.cardinality != "prohibited" && !valueIsComplete(facet.value)) {
                return QString("the value of %1 is incomplete (empty list, pattern, range or length)")
                    .arg(facet.summary());
            }
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
    if (facet.type == IdsFacet::Type::Entity) {
        entity_page_->setFacet(facet);
        facet_pages_->setCurrentIndex(kEntityPage);
    } else {
        property_page_->setFacet(facet);
        facet_pages_->setCurrentIndex(kPropertyPage);
    }
    current_facet_ = index;
    remove_facet_button_->setEnabled(true);
}

void SelectionEditor::storeFacetFromPage() {
    if (current_facet_ < 0 || current_facet_ >= static_cast<int>(fields_.facets.size())) return;
    auto& facet = fields_.facets[current_facet_];
    facet = facet.type == IdsFacet::Type::Entity ? entity_page_->facet() : property_page_->facet();
}

void SelectionEditor::addFacet(IdsFacet::Type type) {
    storeFacetFromPage();
    auto& facets = fields_.facets;
    if (type == IdsFacet::Type::Entity) {
        const bool has_entity = std::any_of(facets.begin(), facets.end(), [](const IdsFacet& f) {
            return f.type == IdsFacet::Type::Entity;
        });
        if (has_entity) return;
        facets.insert(facets.begin(), IdsFacet::entity("IfcWall")); // the entity stays first
        syncFacetList();
        selectFacet(0);
    } else {
        IdsFacet facet = IdsFacet::propertyFacet();
        facet.pset = IdsValue::simpleValue({});
        facet.property = IdsValue::simpleValue({});
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

IdsFacet SelectionEditor::entityFacet() const {
    for (const auto& facet : fields_.facets) {
        if (facet.type == IdsFacet::Type::Entity) return facet;
    }
    return {};
}

void SelectionEditor::updateAddFacetMenu() {
    const bool has_entity = std::any_of(fields_.facets.begin(), fields_.facets.end(), [](const IdsFacet& f) {
        return f.type == IdsFacet::Type::Entity;
    });
    const auto& entries = facetTypes();
    const auto actions = add_facet_button_->menu()->actions();
    for (int i = 0; i < actions.size() && i < static_cast<int>(entries.size()); ++i) {
        const bool entity = entries[i].available && entries[i].type == IdsFacet::Type::Entity;
        actions[i]->setEnabled(entries[i].available && !(entity && has_entity));
    }
}

void SelectionEditor::schedulePreview() {
    preview_timer_->start();
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
