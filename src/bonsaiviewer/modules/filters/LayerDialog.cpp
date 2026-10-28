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

#include "LayerDialog.h"

#include "../property_selection/PropertyPicker.h"

#include "../../services/PopupService.h"

#include <QButtonGroup>
#include <QColorDialog>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPointer>
#include <QPushButton>
#include <QRadioButton>
#include <QScreen>
#include <QScrollArea>
#include <QSpinBox>
#include <QStackedWidget>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>

namespace bonsaiviewer::modules::filters {

using bonsaiviewer::services::FilterAction;
using bonsaiviewer::services::IdsBridge;
using bonsaiviewer::services::IdsFacet;

namespace {

enum SelectionPage { kOwnPage = 0, kAndPage };

// The actions offered by "+ Action", in the order of FilterAction::Type.
const std::vector<std::pair<FilterAction::Type, QString>>& actionTypes() {
    static const std::vector<std::pair<FilterAction::Type, QString>> types = {
        {FilterAction::Type::Isolate, "Isolate in the view"},
        {FilterAction::Type::AddToView, "Add to the view"},
        {FilterAction::Type::RemoveFromView, "Remove from the view"},
        {FilterAction::Type::Colour, "Apply Color"},
        {FilterAction::Type::Transparency, "Transparency"},
        {FilterAction::Type::AutoColourByProperty, "Automatic Color"},
    };
    return types;
}

QString typeName(FilterAction::Type type) {
    for (const auto& [t, name] : actionTypes()) {
        if (t == type) return name;
    }
    return {};
}

void setColourSwatch(QPushButton* button, const QColor& colour) {
    button->setText(colour.isValid() ? colour.name() : QStringLiteral("Colour…"));
    button->setStyleSheet(colour.isValid()
        ? QString("QPushButton { background-color: %1; color: %2; }")
              .arg(colour.name(), colour.lightnessF() > 0.55 ? "black" : "white")
        : QString());
}

} // namespace

FilterEditLayer LayerDialog::defaultLayer(bool is_and) {
    FilterEditLayer layer;
    layer.is_and = is_and;
    if (!is_and) layer.fields = SelectionEditor::defaultFields();
    return layer;
}

QString LayerDialog::actionLabel(const FilterAction& action) {
    switch (action.type) {
    case FilterAction::Type::Isolate: return "Isolate";
    case FilterAction::Type::AddToView: return "Add to view";
    case FilterAction::Type::RemoveFromView: return "Remove from view";
    case FilterAction::Type::Colour: return QString("Colour %1").arg(action.colour.name());
    case FilterAction::Type::Transparency: return QString("Transparency %1%").arg(action.transparency_percent);
    case FilterAction::Type::AutoColourByProperty:
        return QString("Automatic Color (%1.%2)").arg(action.pset, action.property);
    }
    return {};
}

QString LayerDialog::validationError(const FilterEditLayer& layer) {
    if (!layer.is_and) {
        if (const QString error = SelectionEditor::validationError(layer.fields); !error.isEmpty()) return error;
    }
    for (const auto& action : layer.actions) {
        if (action.type == FilterAction::Type::AutoColourByProperty &&
            (action.pset.isEmpty() || action.property.isEmpty())) {
            return "the automatic colour needs a property set and a property";
        }
    }
    return {};
}

LayerDialog::LayerDialog(int index, const FilterEditLayer& layer, IdsBridge& ids_bridge,
                         IfcPathProvider ifc_path, QWidget* parent)
    : components::Dialog(parent),
      index_(index),
      layer_(layer),
      ids_bridge_(ids_bridge),
      ifc_path_(std::move(ifc_path))
{
    setWindowTitle(QString("Layer %1").arg(index + 1));
    setModal(true);
    setMinimumSize(760, 560);
    if (const QScreen* screen = parent != nullptr ? parent->screen() : QGuiApplication::primaryScreen()) {
        const QRect available = screen->availableGeometry();
        resize(std::min(1100, available.width() * 9 / 10), std::min(860, available.height() * 9 / 10));
    }

    auto* body = new QWidget(this);
    auto* layout = new QVBoxLayout(body);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    // ① Type.
    auto* type_box = new QGroupBox("① Type", body);
    auto* type_layout = new QHBoxLayout(type_box);
    own_selection_ = new QRadioButton("New selection", type_box);
    and_selection_ = new QRadioButton("AND — reuse the elements of the layers above", type_box);
    and_selection_->setEnabled(index > 0 || layer_.is_and);
    and_selection_->setToolTip(index > 0 ? "The actions apply to the elements selected above."
                                         : "The first layer has nothing above it to reuse.");
    auto* group = new QButtonGroup(type_box);
    group->addButton(own_selection_);
    group->addButton(and_selection_);
    type_layout->addWidget(own_selection_);
    type_layout->addWidget(and_selection_);
    type_layout->addStretch(1);
    layout->addWidget(type_box);

    // ② Selection.
    auto* selection_box = new QGroupBox("② Selection", body);
    auto* selection_layout = new QVBoxLayout(selection_box);
    selection_stack_ = new QStackedWidget(selection_box);
    selection_editor_ = new SelectionEditor(ids_bridge_, ifc_path_, selection_stack_);
    selection_editor_->setFields(layer_.is_and ? SelectionEditor::defaultFields() : layer_.fields);
    selection_stack_->addWidget(selection_editor_);  // kOwnPage
    auto* and_page = new QLabel(index > 0
        ? QString("This layer uses the elements of layers 1–%1: no selection of its own.").arg(index)
        : QString("This layer has no selection of its own."), selection_stack_);
    and_page->setAlignment(Qt::AlignCenter);
    and_page->setWordWrap(true);
    and_page->setProperty("textRole", "secondary");
    selection_stack_->addWidget(and_page);  // kAndPage
    selection_layout->addWidget(selection_stack_, 1);
    layout->addWidget(selection_box, 3);

    // ③ Actions.
    auto* actions_box = new QGroupBox("③ Actions (run in this order)", body);
    auto* actions_box_layout = new QVBoxLayout(actions_box);
    auto* scroll = new QScrollArea(actions_box);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    actions_host_ = new QWidget(scroll);
    actions_layout_ = new QVBoxLayout(actions_host_);
    actions_layout_->setContentsMargins(0, 0, 0, 0);
    actions_layout_->setSpacing(4);
    scroll->setWidget(actions_host_);
    actions_box_layout->addWidget(scroll, 1);
    auto* add_action = new QToolButton(actions_box);
    add_action->setText("+ Action");
    add_action->setPopupMode(QToolButton::InstantPopup);
    auto* add_menu = new QMenu(add_action);
    for (const auto& [type, name] : actionTypes()) {
        const FilterAction::Type t = type;
        add_menu->addAction(name, this, [this, t]() { addAction(t); });
    }
    add_action->setMenu(add_menu);
    auto* add_row = new QHBoxLayout();
    add_row->addWidget(add_action);
    add_row->addStretch(1);
    actions_box_layout->addLayout(add_row);
    layout->addWidget(actions_box, 2);

    addBodyWidget(body);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &LayerDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    addFooterWidget(buttons);

    connect(selection_editor_, &SelectionEditor::isolateRequested, this, &LayerDialog::isolateRequested);
    connect(own_selection_, &QRadioButton::toggled, this, [this](bool) { updateType(); });

    (layer_.is_and ? and_selection_ : own_selection_)->setChecked(true);
    updateType();
    rebuildActions();
}

void LayerDialog::updateType() {
    layer_.is_and = and_selection_->isChecked();
    selection_stack_->setCurrentIndex(layer_.is_and ? kAndPage : kOwnPage);
}

void LayerDialog::rebuildActions() {
    while (QLayoutItem* item = actions_layout_->takeAt(0)) {
        if (QWidget* widget = item->widget()) {
            widget->hide();  // deleted later: it may be the sender of the current signal
            widget->deleteLater();
        }
        delete item;
    }
    for (int i = 0; i < static_cast<int>(layer_.actions.size()); ++i) {
        actions_layout_->addWidget(actionRow(i));
    }
    if (layer_.actions.empty()) {
        auto* empty = new QLabel("No action: the elements wait for the action of a later layer.", actions_host_);
        empty->setProperty("textRole", "secondary");
        actions_layout_->addWidget(empty);
    }
    actions_layout_->addStretch(1);
}

QWidget* LayerDialog::actionRow(int index) {
    FilterAction& action = layer_.actions[static_cast<size_t>(index)];
    auto* row = new QWidget(actions_host_);
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    auto* name = new QLabel(QString("%1. %2").arg(index + 1).arg(typeName(action.type)), row);
    name->setMinimumWidth(170);  // the parameters line up from one row to the next
    layout->addWidget(name);

    switch (action.type) {
    case FilterAction::Type::Colour: {
        auto* swatch = new QPushButton(row);
        swatch->setFixedWidth(100);
        setColourSwatch(swatch, action.colour);
        connect(swatch, &QPushButton::clicked, this, [this, index, swatch]() {
            FilterAction& a = layer_.actions[static_cast<size_t>(index)];
            const QColor picked = QColorDialog::getColor(a.colour, this, "Pick a colour");
            if (!picked.isValid()) return;
            a.colour = picked;
            setColourSwatch(swatch, picked);
        });
        layout->addWidget(swatch);
        break;
    }
    case FilterAction::Type::Transparency: {
        auto* spin = new QSpinBox(row);
        spin->setRange(0, 100);
        spin->setSuffix("%");
        spin->setValue(action.transparency_percent);
        connect(spin, &QSpinBox::valueChanged, this, [this, index](int value) {
            layer_.actions[static_cast<size_t>(index)].transparency_percent = value;
        });
        layout->addWidget(spin);
        break;
    }
    case FilterAction::Type::AutoColourByProperty: {
        auto* pset = new QLineEdit(action.pset, row);
        pset->setPlaceholderText("Pset_WallCommon");
        auto* property = new QLineEdit(action.property, row);
        property->setPlaceholderText("FireRating");
        connect(pset, &QLineEdit::textChanged, this, [this, index](const QString& text) {
            layer_.actions[static_cast<size_t>(index)].pset = text.trimmed();
        });
        connect(property, &QLineEdit::textChanged, this, [this, index](const QString& text) {
            layer_.actions[static_cast<size_t>(index)].property = text.trimmed();
        });
        auto* browse = new QPushButton("Browse…", row);
        browse->setToolTip("Search the properties of the active model and double-click one to use it");
        connect(browse, &QPushButton::clicked, this, [this, pset, property]() {
            const QString path = ifc_path_ ? ifc_path_() : QString();
            if (path.isEmpty()) {
                services::popup::information(this, "No model loaded: type the property set and the property.");
                return;
            }
            // Every product: the colour applies to whatever the layer selected.
            IdsFacet all = IdsFacet::entity("IfcProduct");
            all.include_subtypes = true;
            QPointer<QLineEdit> pset_guard(pset);
            QPointer<QLineEdit> property_guard(property);
            ids_bridge_.properties(path, all, [this, pset_guard, property_guard](
                                                  bool ok, const IdsBridge::PropertyMap& psets,
                                                  const IdsBridge::SampleMap& samples, const QString& error) {
                if (pset_guard.isNull() || property_guard.isNull()) return;
                if (!ok) {
                    services::popup::information(this, error);
                    return;
                }
                auto* picker = new property_selection::PropertyPicker(
                    psets, samples, "Properties found on the products of the active model.", this);
                picker->setAttribute(Qt::WA_DeleteOnClose);
                connect(picker, &property_selection::PropertyPicker::picked, this,
                        [pset_guard, property_guard](const QString& p, const QString& q) {
                            if (pset_guard) pset_guard->setText(p);
                            if (property_guard) property_guard->setText(q);
                        });
                picker->open();
            });
        });
        layout->addWidget(pset, 2);
        layout->addWidget(new QLabel(".", row));
        layout->addWidget(property, 2);
        layout->addWidget(browse);
        break;
    }
    case FilterAction::Type::Isolate:
    case FilterAction::Type::AddToView:
    case FilterAction::Type::RemoveFromView:
        break;
    }
    layout->addStretch(1);

    auto tool = [row](const QString& text, const QString& tip) {
        auto* button = new QToolButton(row);
        button->setText(text);
        button->setToolTip(tip);
        return button;
    };
    auto* up = tool("↑", "Run earlier");
    auto* down = tool("↓", "Run later");
    auto* remove = tool("✕", "Remove the action");
    up->setEnabled(index > 0);
    down->setEnabled(index + 1 < static_cast<int>(layer_.actions.size()));
    // Queued: the row (and the button) is rebuilt by these slots.
    connect(up, &QToolButton::clicked, this, [this, index]() {
        QMetaObject::invokeMethod(this, [this, index]() { moveAction(index, -1); }, Qt::QueuedConnection);
    });
    connect(down, &QToolButton::clicked, this, [this, index]() {
        QMetaObject::invokeMethod(this, [this, index]() { moveAction(index, +1); }, Qt::QueuedConnection);
    });
    connect(remove, &QToolButton::clicked, this, [this, index]() {
        QMetaObject::invokeMethod(this, [this, index]() { removeAction(index); }, Qt::QueuedConnection);
    });
    layout->addWidget(up);
    layout->addWidget(down);
    layout->addWidget(remove);
    return row;
}

void LayerDialog::addAction(FilterAction::Type type) {
    FilterAction action;
    action.type = type;
    // Sensible defaults so a new action is never a no-op.
    if (type == FilterAction::Type::Colour) action.colour = QColor(220, 50, 47);
    if (type == FilterAction::Type::Transparency) action.transparency_percent = 50;
    layer_.actions.push_back(action);
    rebuildActions();
}

void LayerDialog::moveAction(int index, int direction) {
    const int target = index + direction;
    if (index < 0 || target < 0 || target >= static_cast<int>(layer_.actions.size())) return;
    std::swap(layer_.actions[static_cast<size_t>(index)], layer_.actions[static_cast<size_t>(target)]);
    rebuildActions();
}

void LayerDialog::removeAction(int index) {
    if (index < 0 || index >= static_cast<int>(layer_.actions.size())) return;
    layer_.actions.erase(layer_.actions.begin() + index);
    rebuildActions();
}

void LayerDialog::accept() {
    updateType();
    if (!layer_.is_and) {
        layer_.fields = selection_editor_->fields();
    } else {
        layer_.fields = {};
    }
    if (index_ == 0 && layer_.is_and) {
        services::popup::information(this, "The first layer cannot be an AND layer.");
        return;
    }
    if (const QString error = validationError(layer_); !error.isEmpty()) {
        services::popup::information(this, QString("Layer %1: %2.").arg(index_ + 1).arg(error));
        return;
    }
    QDialog::accept();
}

} // namespace bonsaiviewer::modules::filters
