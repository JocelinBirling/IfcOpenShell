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

#include "CreationDialog.h"

#include "../property_selection/PropertyPicker.h"

#include "../../components/Dialog.h"
#include "../../services/PopupService.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QGuiApplication>
#include <QScreen>
#include <QSplitter>
#include <QStandardItemModel>
#include <QStackedWidget>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>

namespace bonsaiviewer::modules::filters {

using bonsaiviewer::services::FilterAction;
using bonsaiviewer::services::FilterDocument;
using bonsaiviewer::services::FilterLayer;
using bonsaiviewer::services::IdsBridge;
using bonsaiviewer::services::IdsFacet;
using bonsaiviewer::services::IdsValue;

namespace {

// The action parameter pages of the stacked editor, by action type.
enum ParamPage {
    kNoParamPage = 0,
    kColourPage,
    kTransparencyPage,
    kAutoColourPage,
};

// The two kinds of selection of a layer (combo index = stack page).
enum SelectionPage {
    kSpecificationPage = 0,
    kAndPage,
};

ParamPage pageFor(FilterAction::Type type) {
    switch (type) {
    case FilterAction::Type::Colour: return kColourPage;
    case FilterAction::Type::Transparency: return kTransparencyPage;
    case FilterAction::Type::AutoColourByProperty: return kAutoColourPage;
    case FilterAction::Type::Isolate:
    case FilterAction::Type::AddToView:
    case FilterAction::Type::RemoveFromView:
        break;
    }
    return kNoParamPage;
}

QString actionLabel(const FilterAction& action) {
    switch (action.type) {
    case FilterAction::Type::Isolate: return "Isolate in the view";
    case FilterAction::Type::AddToView: return "Add to the view";
    case FilterAction::Type::RemoveFromView: return "Remove from the view";
    case FilterAction::Type::Colour: return QString("Apply Color %1").arg(action.colour.name());
    case FilterAction::Type::Transparency:
        return QString("Transparency %1%").arg(action.transparency_percent);
    case FilterAction::Type::AutoColourByProperty:
        return QString("Automatic Color by %1.%2").arg(action.pset, action.property);
    }
    return {};
}

QString layerLabel(int index, const FilterCreationDialog::EditLayer& layer) {
    QString label = QString("%1. ").arg(index + 1);
    if (layer.is_and) {
        label += "\u21b3 same elements as above";
    } else {
        QStringList parts;
        for (const auto& facet : layer.fields.facets) parts << facet.summary();
        label += parts.isEmpty() ? QStringLiteral("(empty selection)") : parts.join(" & ");
    }
    if (!layer.actions.empty()) {
        QStringList actions;
        for (const auto& action : layer.actions) actions << actionLabel(action);
        label += "  →  " + actions.join(", ");
    }
    return label;
}

FilterCreationDialog::EditLayer defaultLayer(bool is_and) {
    FilterCreationDialog::EditLayer layer;
    layer.is_and = is_and;
    if (!is_and) {
        layer.fields = SelectionEditor::defaultFields();
    }
    return layer;
}

void setColourSwatch(QPushButton* button, const QColor& colour) {
    if (colour.isValid()) {
        button->setText(colour.name());
        button->setStyleSheet(QString("QPushButton { background-color: %1; color: %2; }")
                                  .arg(colour.name(),
                                       colour.lightnessF() > 0.55 ? "black" : "white"));
    } else {
        button->setText("Colour…");
        button->setStyleSheet({});
    }
}


} // namespace

FilterCreationDialog::FilterCreationDialog(const FilterDocument& document,
                                           const std::vector<EditLayer>& layers,
                                           IdsBridge& ids_bridge,
                                           IfcPathProvider ifc_path,
                                           QWidget* parent)
    : components::Dialog(parent),
      document_(document),
      layers_(layers),
      ids_bridge_(ids_bridge),
      ifc_path_(std::move(ifc_path))
{
    setWindowTitle(document_.id.isEmpty() ? "New Filter" : "Modify Filter");
    setModal(true);
    // Fit the screen: the selection needs room, but laptops are short.
    setMinimumSize(820, 560);
    if (const QScreen* screen = parent != nullptr ? parent->screen() : QGuiApplication::primaryScreen()) {
        const QRect available = screen->availableGeometry();
        resize(std::min(1240, available.width() * 9 / 10), std::min(880, available.height() * 9 / 10));
    }

    auto* body = new QWidget(this);
    auto* body_layout = new QVBoxLayout(body);
    body_layout->setContentsMargins(0, 0, 0, 0);
    body_layout->setSpacing(8);

    // Name and description.
    auto* identity = new QFormLayout();
    auto* name_edit = new QLineEdit(document_.name, this);
    auto* description_edit = new QLineEdit(document_.description, this);
    identity->addRow("Name", name_edit);
    identity->addRow("Description", description_edit);
    body_layout->addLayout(identity);
    connect(name_edit, &QLineEdit::textChanged, this,
            [this](const QString& text) { document_.name = text; });
    connect(description_edit, &QLineEdit::textChanged, this,
            [this](const QString& text) { document_.description = text; });

    // Layout: the layers in a column on the left; on the right, the editor
    // of the selected layer (selection above, actions below, resizable).
    auto* main_split = new QSplitter(Qt::Horizontal, body);
    body_layout->addWidget(main_split, 1);

    // Layer list with ordering buttons. The layers run from top to bottom.
    auto* layers_box = new QGroupBox("Layers (top to bottom)", main_split);
    auto* layers_layout = new QVBoxLayout(layers_box);
    layer_list_ = new QListWidget(layers_box);
    layer_list_->setWordWrap(true);
    layer_list_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    layers_layout->addWidget(layer_list_, 1);
    auto* layer_buttons = new QHBoxLayout();
    auto* add_layer_button = new QPushButton("Add", layers_box);
    add_layer_button->setToolTip("Add a layer under the selected one");
    auto* remove_layer_button = new QPushButton("Remove", layers_box);
    auto* layer_up_button = new QPushButton("\u2191", layers_box);
    layer_up_button->setToolTip("Move the layer up");
    auto* layer_down_button = new QPushButton("\u2193", layers_box);
    layer_down_button->setToolTip("Move the layer down");
    for (auto* button : {layer_up_button, layer_down_button}) button->setFixedWidth(32);
    for (auto* button : {add_layer_button, remove_layer_button, layer_up_button, layer_down_button}) {
        layer_buttons->addWidget(button);
    }
    layer_buttons->addStretch(1);
    layers_layout->addLayout(layer_buttons);
    main_split->addWidget(layers_box);

    auto* editor_split = new QSplitter(Qt::Vertical, main_split);
    main_split->addWidget(editor_split);
    main_split->setStretchFactor(0, 1);
    main_split->setStretchFactor(1, 4);
    main_split->setSizes({220, 980});


    // The selection of a layer is either an IDS specification (facet list
    // on the left, the page of the selected facet on the right) or AND
    // (the same elements as the layer above); the live preview is below.
    selection_box_ = new QGroupBox("Selection", editor_split);
    auto* selection_layout = new QVBoxLayout(selection_box_);
    auto* type_row = new QHBoxLayout();
    selection_type_combo_ = new QComboBox(selection_box_);
    selection_type_combo_->addItem("IDS specification");
    selection_type_combo_->addItem("AND \u2014 same elements as the layer above");
    selection_type_combo_->setToolTip("AND: this layer's actions apply to the elements of the "
                                      "layer above, without a selection of its own.");
    type_row->addWidget(new QLabel("Type", selection_box_));
    type_row->addWidget(selection_type_combo_);
    type_row->addStretch(1);
    selection_layout->addLayout(type_row);
    selection_stack_ = new QStackedWidget(selection_box_);
    selection_layout->addWidget(selection_stack_, 1);
    selection_editor_ = new SelectionEditor(ids_bridge_, ifc_path_, selection_stack_);
    selection_stack_->addWidget(selection_editor_); // kSpecificationPage
    auto* and_page = new QLabel("This layer has no selection of its own: its actions apply to the "
                                "same elements as the layer above.", selection_stack_);
    and_page->setAlignment(Qt::AlignCenter);
    and_page->setWordWrap(true);
    and_page->setProperty("textRole", "secondary");
    selection_stack_->addWidget(and_page); // kAndPage
    editor_split->addWidget(selection_box_);

    // The layer's actions: the list (with its buttons) and the parameters
    // of the selected action.
    auto* actions_box = new QGroupBox("Actions of the layer", editor_split);
    auto* actions_layout = new QHBoxLayout(actions_box);
    auto* action_list_side = new QVBoxLayout();
    action_list_ = new QListWidget(actions_box);
    action_list_->setMinimumHeight(48);
    action_list_side->addWidget(action_list_, 1);
    auto* action_buttons = new QHBoxLayout();
    auto* add_action_button = new QPushButton("Add", actions_box);
    add_action_button->setToolTip("Add an action to this layer");
    auto* remove_action_button = new QPushButton("Remove", actions_box);
    action_buttons->addWidget(add_action_button);
    action_buttons->addWidget(remove_action_button);
    action_buttons->addStretch(1);
    action_list_side->addLayout(action_buttons);
    actions_layout->addLayout(action_list_side, 2);

    action_params_ = new QWidget(actions_box);
    auto* params_form = new QFormLayout(action_params_);
    params_form->setContentsMargins(0, 0, 0, 0);
    action_type_combo_ = new QComboBox(action_params_);
    // Same order as FilterAction::Type.
    action_type_combo_->addItems({"Isolate in the view", "Add to the view", "Remove from the view",
                                  "Apply Color", "Change the transparency",
                                  "Automatic Color"});
    params_form->addRow("Action", action_type_combo_);

    auto* params_stack = new QStackedWidget(action_params_);
    params_stack->addWidget(new QWidget(params_stack)); // kNoParamPage
    {
        auto* page = new QWidget(params_stack);
        auto* form = new QFormLayout(page);
        form->setContentsMargins(0, 0, 0, 0);
        colour_button_ = new QPushButton("Colour…", page);
        colour_button_->setFixedWidth(100);
        form->addRow("Colour", colour_button_);
        params_stack->addWidget(page); // kColourPage
    }
    {
        auto* page = new QWidget(params_stack);
        auto* form = new QFormLayout(page);
        form->setContentsMargins(0, 0, 0, 0);
        transparency_spin_ = new QSpinBox(page);
        transparency_spin_->setRange(0, 100);
        transparency_spin_->setSuffix("%");
        form->addRow("Transparency", transparency_spin_);
        params_stack->addWidget(page); // kTransparencyPage
    }
    {
        auto* page = new QWidget(params_stack);
        auto* form = new QFormLayout(page);
        form->setContentsMargins(0, 0, 0, 0);
        auto_pset_edit_ = new QLineEdit(page);
        auto_pset_edit_->setPlaceholderText("Pset_WallCommon");
        auto_property_edit_ = new QLineEdit(page);
        auto_property_edit_->setPlaceholderText("FireRating");
        auto* browse = new QPushButton("Browse properties\u2026", page);
        browse->setToolTip("Search the properties of the model and double-click one to use it");
        form->addRow("Property set", auto_pset_edit_);
        form->addRow("Property", auto_property_edit_);
        form->addRow("", browse);
        params_stack->addWidget(page); // kAutoColourPage
        connect(browse, &QPushButton::clicked, this, &FilterCreationDialog::browseAutoColourProperty);
    }
    params_form->addRow(params_stack);
    actions_layout->addWidget(action_params_, 3);
    editor_split->addWidget(actions_box);
    editor_split->setStretchFactor(0, 4);
    editor_split->setStretchFactor(1, 1);
    editor_split->setChildrenCollapsible(false);
    main_split->setChildrenCollapsible(false);

    addBodyWidget(body);

    buttons_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons_, &QDialogButtonBox::accepted, this, &FilterCreationDialog::accept);
    connect(buttons_, &QDialogButtonBox::rejected, this, &QDialog::reject);
    addFooterWidget(buttons_);

    connect(selection_editor_, &SelectionEditor::isolateRequested, this, &FilterCreationDialog::isolateRequested);
    connect(selection_editor_, &SelectionEditor::changed, this, [this]() {
        storeSelection();
        syncLayerList();
    });

    // Layer list wiring.
    connect(add_layer_button, &QPushButton::clicked, this, [this]() { addLayer(); });
    connect(remove_layer_button, &QPushButton::clicked, this, [this]() {
        if (current_layer_ >= 0) removeLayer(current_layer_);
    });
    connect(layer_up_button, &QPushButton::clicked, this, [this]() {
        if (current_layer_ > 0) moveLayer(current_layer_, -1);
    });
    connect(layer_down_button, &QPushButton::clicked, this, [this]() {
        if (current_layer_ >= 0 && current_layer_ < static_cast<int>(layers_.size()) - 1) {
            moveLayer(current_layer_, +1);
        }
    });
    connect(layer_list_, &QListWidget::currentRowChanged, this, [this](int row) {
        storeSelection();
        loadLayer(row);
    });
    connect(selection_type_combo_, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (loading_ || current_layer_ < 0) return;
        const bool own_selection = index == kSpecificationPage;
        layers_[current_layer_].is_and = !own_selection;
        if (own_selection && layers_[current_layer_].fields.facets.empty()) {
            layers_[current_layer_].fields = SelectionEditor::defaultFields();
        }
        if (own_selection) selection_editor_->setFields(layers_[current_layer_].fields);
        updateSelectionEnabled();
        syncLayerList();
    });

    // Action wiring.
    connect(add_action_button, &QPushButton::clicked, this, [this]() { addAction(); });
    connect(remove_action_button, &QPushButton::clicked, this, [this]() {
        const int row = action_list_->currentRow();
        if (row >= 0 && current_layer_ >= 0) removeAction(row);
    });
    connect(action_type_combo_, &QComboBox::currentIndexChanged, this, [this, params_stack](int index) {
        params_stack->setCurrentIndex(pageFor(static_cast<FilterAction::Type>(index)));
        if (loading_) return;
        FilterAction* action = currentAction();
        if (action == nullptr) return;
        action->type = static_cast<FilterAction::Type>(index);
        // Sensible defaults so a freshly switched action is never a no-op.
        if (action->type == FilterAction::Type::Colour && !action->colour.isValid()) {
            action->colour = QColor(220, 50, 47);
        }
        if (action->type == FilterAction::Type::Transparency && action->transparency_percent == 0) {
            action->transparency_percent = 50;
        }
        loadActionParameters(action_list_->currentRow());
        syncActionListRow(action_list_->currentRow());
        syncLayerList();
    });
    connect(colour_button_, &QPushButton::clicked, this, [this]() {
        FilterAction* action = currentAction();
        if (action == nullptr || action->type != FilterAction::Type::Colour) return;
        const QColor picked = QColorDialog::getColor(action->colour, this, "Pick a colour");
        if (picked.isValid()) {
            action->colour = picked;
            setColourSwatch(colour_button_, picked);
            syncActionListRow(action_list_->currentRow());
            syncLayerList();
        }
    });
    connect(transparency_spin_, &QSpinBox::valueChanged, this, [this](int value) {
        if (loading_) return;
        FilterAction* action = currentAction();
        if (action == nullptr || action->type != FilterAction::Type::Transparency) return;
        action->transparency_percent = value;
        syncActionListRow(action_list_->currentRow());
        syncLayerList();
    });
    connect(auto_pset_edit_, &QLineEdit::textChanged, this, [this](const QString& text) {
        if (loading_) return;
        FilterAction* action = currentAction();
        if (action == nullptr || action->type != FilterAction::Type::AutoColourByProperty) return;
        action->pset = text.trimmed();
        syncActionListRow(action_list_->currentRow());
        syncLayerList();
    });
    connect(auto_property_edit_, &QLineEdit::textChanged, this, [this](const QString& text) {
        if (loading_) return;
        FilterAction* action = currentAction();
        if (action == nullptr || action->type != FilterAction::Type::AutoColourByProperty) return;
        action->property = text.trimmed();
        syncActionListRow(action_list_->currentRow());
        syncLayerList();
    });
    connect(action_list_, &QListWidget::currentRowChanged, this, [this](int row) {
        loadActionParameters(row);
    });

    if (layers_.empty()) {
        EditLayer layer = defaultLayer(false);
        FilterAction action;
        action.type = FilterAction::Type::Isolate;
        layer.actions.push_back(action);
        layers_.push_back(layer);
    }
    syncLayerList();
    {
        const QSignalBlocker blocker(layer_list_);
        layer_list_->setCurrentRow(0);
    }
    loadLayer(0);
}

FilterAction* FilterCreationDialog::currentAction() {
    if (current_layer_ < 0 || current_layer_ >= static_cast<int>(layers_.size())) return nullptr;
    auto& actions = layers_[current_layer_].actions;
    const int row = action_list_->currentRow();
    if (row < 0 || row >= static_cast<int>(actions.size())) return nullptr;
    return &actions[row];
}

void FilterCreationDialog::syncLayerList() {
    // Update the labels in place: rebuilding the list would steal the
    // focus from the field being typed into.
    const QSignalBlocker blocker(layer_list_);
    while (layer_list_->count() > static_cast<int>(layers_.size())) {
        delete layer_list_->takeItem(layer_list_->count() - 1);
    }
    for (int i = 0; i < static_cast<int>(layers_.size()); ++i) {
        const QString label = layerLabel(i, layers_[i]);
        if (i < layer_list_->count()) {
            layer_list_->item(i)->setText(label);
        } else {
            layer_list_->addItem(label);
        }
    }
}

void FilterCreationDialog::syncActionListRow(int row) {
    if (current_layer_ < 0) return;
    if (row >= 0 && row < action_list_->count()) {
        action_list_->item(row)->setText(actionLabel(layers_[current_layer_].actions[row]));
    }
}

void FilterCreationDialog::loadLayer(int index) {
    const bool valid = index >= 0 && index < static_cast<int>(layers_.size());
    current_layer_ = valid ? index : -1;
    selection_editor_->setFields(valid ? layers_[index].fields : IdsBridge::SelectionFields{});

    {
        const QSignalBlocker blocker(action_list_);
        action_list_->clear();
        if (valid) {
            for (const auto& action : layers_[index].actions) {
                action_list_->addItem(actionLabel(action));
            }
        }
    }
    if (action_list_->count() > 0) {
        action_list_->setCurrentRow(0);
    }
    loadActionParameters(action_list_->currentRow());
    updateSelectionEnabled();
}

void FilterCreationDialog::storeSelection() {
    if (current_layer_ < 0 || current_layer_ >= static_cast<int>(layers_.size())) return;
    if (layers_[current_layer_].is_and) return; // the editor holds no selection of this layer
    layers_[current_layer_].fields = selection_editor_->fields();
}

void FilterCreationDialog::updateSelectionEnabled() {
    const bool valid = current_layer_ >= 0;
    const bool is_and = valid && layers_[current_layer_].is_and;
    loading_ = true;
    selection_box_->setEnabled(valid);
    // The first layer has nothing above it to reuse.
    if (auto* model = qobject_cast<QStandardItemModel*>(selection_type_combo_->model())) {
        model->item(kAndPage)->setEnabled(valid && (current_layer_ > 0 || is_and));
    }
    selection_type_combo_->setCurrentIndex(is_and ? kAndPage : kSpecificationPage);
    selection_stack_->setCurrentIndex(is_and ? kAndPage : kSpecificationPage);
    loading_ = false;
}

// ---------------------------------------------------------------------------
// Actions
// ---------------------------------------------------------------------------

void FilterCreationDialog::loadActionParameters(int row) {
    FilterAction* action = (row >= 0) ? currentAction() : nullptr;
    loading_ = true;
    action_params_->setEnabled(action != nullptr);
    if (action != nullptr) {
        action_type_combo_->setCurrentIndex(static_cast<int>(action->type));
        setColourSwatch(colour_button_, action->colour);
        transparency_spin_->setValue(action->transparency_percent);
        auto_pset_edit_->setText(action->pset);
        auto_property_edit_->setText(action->property);
    }
    loading_ = false;
}

void FilterCreationDialog::addLayer() {
    storeSelection();
    // Insert after the selected layer.
    const int at = current_layer_ >= 0 ? current_layer_ + 1 : static_cast<int>(layers_.size());
    layers_.insert(layers_.begin() + at, defaultLayer(false));
    current_layer_ = -1; // indices shifted: nothing to store on the row change
    syncLayerList();
    layer_list_->setCurrentRow(at);
}

void FilterCreationDialog::removeLayer(int index) {
    if (index < 0 || index >= static_cast<int>(layers_.size())) return;
    layers_.erase(layers_.begin() + index);
    current_layer_ = -1;
    syncLayerList();
    if (!layers_.empty()) {
        const int next = std::min(index, static_cast<int>(layers_.size()) - 1);
        {
            const QSignalBlocker blocker(layer_list_);
            layer_list_->setCurrentRow(next);
        }
        loadLayer(next);
    } else {
        loadLayer(-1);
    }
}

void FilterCreationDialog::moveLayer(int index, int direction) {
    const int target = index + direction;
    if (index < 0 || target < 0 || target >= static_cast<int>(layers_.size())) return;
    storeSelection();
    std::swap(layers_[index], layers_[target]);
    current_layer_ = -1;
    syncLayerList();
    {
        const QSignalBlocker blocker(layer_list_);
        layer_list_->setCurrentRow(target);
    }
    loadLayer(target);
}

void FilterCreationDialog::addAction() {
    if (current_layer_ < 0) return;
    FilterAction action;
    action.type = FilterAction::Type::Isolate;
    layers_[current_layer_].actions.push_back(action);
    action_list_->addItem(actionLabel(action));
    action_list_->setCurrentRow(action_list_->count() - 1);
    syncLayerList();
}

void FilterCreationDialog::removeAction(int index) {
    if (current_layer_ < 0) return;
    auto& actions = layers_[current_layer_].actions;
    if (index < 0 || index >= static_cast<int>(actions.size())) return;
    actions.erase(actions.begin() + index);
    {
        const QSignalBlocker blocker(action_list_);
        delete action_list_->takeItem(index);
    }
    loadActionParameters(action_list_->currentRow());
    syncLayerList();
}

void FilterCreationDialog::browseAutoColourProperty() {
    const QString path = ifc_path_ ? ifc_path_() : QString();
    if (path.isEmpty()) {
        services::popup::information(this, "No model loaded: type the property set and the property.");
        return;
    }
    // Every product of the model: the colour applies to whatever the layer
    // selected, so the picker is not limited to one class.
    IdsFacet all = IdsFacet::entity("IfcProduct");
    all.include_subtypes = true;
    QPointer<FilterCreationDialog> guard(this);
    ids_bridge_.properties(path, all, [this, guard](bool ok, const IdsBridge::PropertyMap& psets,
                                                    const IdsBridge::SampleMap& samples, const QString& error) {
        if (guard.isNull()) return;
        if (!ok) {
            services::popup::information(this, error);
            return;
        }
        auto* picker = new property_selection::PropertyPicker(psets, samples, "Properties found on the products of the active model.",
                                          this);
        picker->setAttribute(Qt::WA_DeleteOnClose);
        connect(picker, &property_selection::PropertyPicker::picked, this, [this](const QString& pset, const QString& property) {
            // The line edits write the action back (see their connections).
            auto_pset_edit_->setText(pset);
            auto_property_edit_->setText(property);
        });
        picker->open();
    });
}

// ---------------------------------------------------------------------------
// Accept
// ---------------------------------------------------------------------------

bool FilterCreationDialog::validate() {
    storeSelection();
    const auto fail = [this](int layer, const QString& message) {
        services::popup::information(this, message);
        if (layer >= 0) layer_list_->setCurrentRow(layer);
        return false;
    };
    if (document_.name.trimmed().isEmpty()) return fail(-1, "The filter needs a name.");
    if (layers_.empty()) return fail(-1, "The filter needs at least one layer.");
    if (layers_.front().is_and) {
        return fail(0, "The first layer cannot be an AND layer: there is no selection above it to reuse.");
    }
    bool any_action = false;
    for (int i = 0; i < static_cast<int>(layers_.size()); ++i) {
        const EditLayer& layer = layers_[i];
        any_action = any_action || !layer.actions.empty();
        if (layer.is_and) continue;
        if (const QString error = SelectionEditor::validationError(layer.fields); !error.isEmpty()) {
            return fail(i, QString("Layer %1: %2.").arg(i + 1).arg(error));
        }
        for (const auto& action : layer.actions) {
            if (action.type == FilterAction::Type::AutoColourByProperty &&
                (action.pset.isEmpty() || action.property.isEmpty())) {
                return fail(i, QString("Layer %1: the automatic colour needs a property set and a "
                                       "property.").arg(i + 1));
            }
        }
    }
    if (!any_action) return fail(-1, "No layer has an action: the filter would do nothing.");

    // Non blocking: selection layers whose elements no later action uses
    // (they wait for an action layer that never comes).
    QStringList unused;
    for (int i = 0; i < static_cast<int>(layers_.size()); ++i) {
        if (!layers_[i].actions.empty()) {
            unused.clear();
        } else if (!layers_[i].is_and) {
            unused << QString::number(i + 1);
        }
    }
    if (!unused.isEmpty()) {
        services::popup::information(
            this, QString("Layer(s) %1 have no action and no action layer follows them: they will "
                          "do nothing. The filter is saved anyway.").arg(unused.join(", ")));
    }
    return true;
}

void FilterCreationDialog::accept() {
    // Serialise every selection layer to canonical IDS XML through the
    // bridge (IfcTester builds the document); AND layers carry no XML.
    if (!validate()) return;
    built_xml_.assign(layers_.size(), QString());
    buttons_->setEnabled(false);
    buildNextLayer(0);
}

void FilterCreationDialog::buildNextLayer(size_t index) {
    while (index < layers_.size() && layers_[index].is_and) {
        ++index;
    }
    if (index >= layers_.size()) {
        FilterDocument document = document_;
        document.name = document.name.trimmed();
        document.layers.clear();
        for (size_t i = 0; i < layers_.size(); ++i) {
            FilterLayer layer;
            layer.is_and = layers_[i].is_and;
            layer.ids_xml = built_xml_[i];
            layer.actions = layers_[i].actions;
            document.layers.push_back(layer);
        }
        buttons_->setEnabled(true);
        emit documentReady(document);
        QDialog::accept();
        return;
    }
    // Name the specification after the filter and the layer, so the IDS
    // stays readable when opened in another tool.
    const QString spec_name = QString("%1 — layer %2").arg(document_.name.trimmed()).arg(index + 1);
    // The window may be closed while IfcTester builds the document.
    QPointer<FilterCreationDialog> guard(this);
    ids_bridge_.build(spec_name, layers_[index].fields,
                      [this, guard, index](bool ok, const QString& ids_xml, const QString& error) {
        if (guard.isNull()) return;
        if (!ok) {
            buttons_->setEnabled(true);
            layer_list_->setCurrentRow(static_cast<int>(index));
            services::popup::information(this, QString("Layer %1: %2").arg(index + 1)
                .arg(error.isEmpty() ? "the IDS engine failed" : error));
            return;
        }
        built_xml_[index] = ids_xml;
        buildNextLayer(index + 1);
    });
}

} // namespace bonsaiviewer::modules::filters
