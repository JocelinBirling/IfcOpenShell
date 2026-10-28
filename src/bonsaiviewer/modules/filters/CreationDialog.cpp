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

#include "../../services/PopupService.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPixmap>
#include <QPointer>
#include <QPushButton>
#include <QScreen>
#include <QTableWidget>
#include <QVBoxLayout>

#include <algorithm>

namespace bonsaiviewer::modules::filters {

using bonsaiviewer::services::FilterAction;
using bonsaiviewer::services::FilterDocument;
using bonsaiviewer::services::FilterLayer;
using bonsaiviewer::services::IdsBridge;

namespace {

enum Column { kNumber = 0, kType, kSelection, kActions, kColumnCount };

QString selectionSummary(int index, const FilterCreationDialog::EditLayer& layer) {
    if (layer.is_and) {
        return index > 1 ? QString("(reuses layers 1–%1)").arg(index) : QString("(reuses layer 1)");
    }
    QStringList parts;
    for (const auto& facet : layer.fields.facets) parts << facet.summary();
    return parts.isEmpty() ? QStringLiteral("(empty selection)") : parts.join(" · ");
}

QString actionsSummary(const FilterCreationDialog::EditLayer& layer) {
    QStringList parts;
    for (const auto& action : layer.actions) parts << LayerDialog::actionLabel(action);
    return parts.isEmpty() ? QStringLiteral("—") : parts.join(", ");
}

// The colour of the layer's first Colour action, as a small swatch.
QIcon actionsIcon(const FilterCreationDialog::EditLayer& layer) {
    for (const auto& action : layer.actions) {
        if (action.type != FilterAction::Type::Colour || !action.colour.isValid()) continue;
        QPixmap swatch(12, 12);
        swatch.fill(action.colour);
        return QIcon(swatch);
    }
    return {};
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
    setMinimumSize(720, 420);
    if (const QScreen* screen = parent != nullptr ? parent->screen() : QGuiApplication::primaryScreen()) {
        const QRect available = screen->availableGeometry();
        resize(std::min(980, available.width() * 9 / 10), std::min(620, available.height() * 9 / 10));
    }

    auto* body = new QWidget(this);
    auto* body_layout = new QVBoxLayout(body);
    body_layout->setContentsMargins(0, 0, 0, 0);
    body_layout->setSpacing(8);

    // Name and description on one line.
    auto* identity = new QHBoxLayout();
    auto* name_edit = new QLineEdit(document_.name, body);
    auto* description_edit = new QLineEdit(document_.description, body);
    identity->addWidget(new QLabel("Name", body));
    identity->addWidget(name_edit, 1);
    identity->addWidget(new QLabel("Description", body));
    identity->addWidget(description_edit, 2);
    body_layout->addLayout(identity);
    connect(name_edit, &QLineEdit::textChanged, this, [this](const QString& text) {
        document_.name = text;
        syncTable(currentLayer());
    });
    connect(description_edit, &QLineEdit::textChanged, this,
            [this](const QString& text) { document_.description = text; });

    // The layers, one row each, run from top to bottom.
    table_ = new QTableWidget(0, kColumnCount, body);
    table_->setHorizontalHeaderLabels({"#", "Type", "Selection", "Actions"});
    table_->verticalHeader()->setVisible(false);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setWordWrap(true);
    auto* header = table_->horizontalHeader();
    header->setSectionResizeMode(kNumber, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(kType, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(kSelection, QHeaderView::Stretch);
    header->setSectionResizeMode(kActions, QHeaderView::Stretch);
    table_->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    body_layout->addWidget(table_, 1);

    auto* row_buttons = new QHBoxLayout();
    auto button = [body, row_buttons](const QString& text, const QString& tip) {
        auto* b = new QPushButton(text, body);
        b->setToolTip(tip);
        row_buttons->addWidget(b);
        return b;
    };
    auto* add = button("+ Add layer", "Add a layer under the selected one");
    auto* edit = button("Edit…", "Edit the selected layer (or double-click it)");
    auto* duplicate = button("Duplicate", "Copy the selected layer under it");
    auto* remove = button("Remove", "Remove the selected layer");
    auto* up = button("↑", "Move the layer up");
    auto* down = button("↓", "Move the layer down");
    up->setFixedWidth(32);
    down->setFixedWidth(32);
    row_buttons->addStretch(1);
    body_layout->addLayout(row_buttons);

    problems_label_ = new QLabel(body);
    problems_label_->setWordWrap(true);
    problems_label_->setProperty("textRole", "secondary");
    body_layout->addWidget(problems_label_);

    addBodyWidget(body);
    buttons_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons_, &QDialogButtonBox::accepted, this, &FilterCreationDialog::accept);
    connect(buttons_, &QDialogButtonBox::rejected, this, &QDialog::reject);
    addFooterWidget(buttons_);

    connect(add, &QPushButton::clicked, this, &FilterCreationDialog::addLayer);
    connect(edit, &QPushButton::clicked, this, [this]() {
        const int index = currentLayer();
        if (index < 0) return;
        EditLayer layer = layers_[static_cast<size_t>(index)];
        if (editLayer(index, layer)) {
            layers_[static_cast<size_t>(index)] = layer;
            syncTable(index);
        }
    });
    connect(table_, &QTableWidget::cellDoubleClicked, edit, [edit](int, int) { edit->click(); });
    connect(duplicate, &QPushButton::clicked, this, &FilterCreationDialog::duplicateLayer);
    connect(remove, &QPushButton::clicked, this, &FilterCreationDialog::removeLayer);
    connect(up, &QPushButton::clicked, this, [this]() { moveLayer(-1); });
    connect(down, &QPushButton::clicked, this, [this]() { moveLayer(+1); });

    if (layers_.empty()) {
        EditLayer layer = LayerDialog::defaultLayer(false);
        FilterAction action;
        action.type = FilterAction::Type::Isolate;
        layer.actions.push_back(action);
        layers_.push_back(layer);
    }
    syncTable(0);
}

int FilterCreationDialog::currentLayer() const {
    const int row = table_->currentRow();
    return row >= 0 && row < static_cast<int>(layers_.size()) ? row : -1;
}

void FilterCreationDialog::syncTable(int select) {
    table_->setRowCount(static_cast<int>(layers_.size()));
    for (int i = 0; i < static_cast<int>(layers_.size()); ++i) {
        const EditLayer& layer = layers_[static_cast<size_t>(i)];
        auto set = [this, i](int column, const QString& text) {
            auto* item = table_->item(i, column);
            if (item == nullptr) {
                item = new QTableWidgetItem();
                table_->setItem(i, column, item);
            }
            item->setText(text);
            return item;
        };
        set(kNumber, QString::number(i + 1));
        set(kType, layer.is_and ? "AND" : "Select");
        set(kSelection, selectionSummary(i, layer));
        set(kActions, actionsSummary(layer))->setIcon(actionsIcon(layer));
    }
    if (select >= 0 && select < table_->rowCount()) table_->selectRow(select);

    bool blocking = false;
    const QStringList list = problems(&blocking);
    problems_label_->setText(list.isEmpty() ? QString() : "⚠ " + list.join("\n⚠ "));
    problems_label_->setVisible(!list.isEmpty());
}

QStringList FilterCreationDialog::problems(bool* blocking) const {
    QStringList list;
    bool block = false;
    if (document_.name.trimmed().isEmpty()) {
        list << "The filter needs a name.";
        block = true;
    }
    if (layers_.empty()) {
        list << "The filter needs at least one layer.";
        block = true;
    } else if (layers_.front().is_and) {
        list << "Layer 1: the first layer cannot be an AND layer.";
        block = true;
    }
    bool any_action = false;
    for (int i = 0; i < static_cast<int>(layers_.size()); ++i) {
        const EditLayer& layer = layers_[static_cast<size_t>(i)];
        any_action = any_action || !layer.actions.empty();
        if (const QString error = LayerDialog::validationError(layer); !error.isEmpty()) {
            list << QString("Layer %1: %2.").arg(i + 1).arg(error);
            block = true;
        }
    }
    if (!layers_.empty() && !any_action) {
        list << "No layer has an action: the filter would do nothing.";
        block = true;
    }
    // Non blocking: selection layers whose elements no later action uses.
    QStringList unused;
    for (int i = 0; i < static_cast<int>(layers_.size()); ++i) {
        if (!layers_[static_cast<size_t>(i)].actions.empty()) {
            unused.clear();
        } else if (!layers_[static_cast<size_t>(i)].is_and) {
            unused << QString::number(i + 1);
        }
    }
    if (!unused.isEmpty()) {
        list << QString("Layer(s) %1: no action and no action layer after them, they do nothing.")
                    .arg(unused.join(", "));
    }
    if (blocking != nullptr) *blocking = block;
    return list;
}

bool FilterCreationDialog::editLayer(int index, EditLayer& layer) {
    LayerDialog dialog(index, layer, ids_bridge_, ifc_path_, this);
    connect(&dialog, &LayerDialog::isolateRequested, this, &FilterCreationDialog::isolateRequested);
    if (dialog.exec() != QDialog::Accepted) return false;
    layer = dialog.layer();
    return true;
}

void FilterCreationDialog::addLayer() {
    const int at = currentLayer() >= 0 ? currentLayer() + 1 : static_cast<int>(layers_.size());
    EditLayer layer = LayerDialog::defaultLayer(false);
    if (!editLayer(at, layer)) return;
    layers_.insert(layers_.begin() + at, layer);
    syncTable(at);
}

void FilterCreationDialog::duplicateLayer() {
    const int index = currentLayer();
    if (index < 0) return;
    layers_.insert(layers_.begin() + index + 1, layers_[static_cast<size_t>(index)]);
    syncTable(index + 1);
}

void FilterCreationDialog::removeLayer() {
    const int index = currentLayer();
    if (index < 0) return;
    layers_.erase(layers_.begin() + index);
    syncTable(std::min(index, static_cast<int>(layers_.size()) - 1));
}

void FilterCreationDialog::moveLayer(int direction) {
    const int index = currentLayer();
    const int target = index + direction;
    if (index < 0 || target < 0 || target >= static_cast<int>(layers_.size())) return;
    std::swap(layers_[static_cast<size_t>(index)], layers_[static_cast<size_t>(target)]);
    syncTable(target);
}

void FilterCreationDialog::accept() {
    bool blocking = false;
    const QStringList list = problems(&blocking);
    if (blocking) {
        services::popup::information(this, list.join("\n"));
        return;
    }
    // Serialise every selection layer to canonical IDS XML through the
    // bridge; AND layers carry no XML.
    built_xml_.assign(layers_.size(), QString());
    buttons_->setEnabled(false);
    buildNextLayer(0);
}

void FilterCreationDialog::buildNextLayer(size_t index) {
    while (index < layers_.size() && layers_[index].is_and) ++index;
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
    QPointer<FilterCreationDialog> guard(this);
    ids_bridge_.build(spec_name, layers_[index].fields,
                      [this, guard, index](bool ok, const QString& ids_xml, const QString& error) {
        if (guard.isNull()) return;
        if (!ok) {
            buttons_->setEnabled(true);
            table_->selectRow(static_cast<int>(index));
            services::popup::information(this, QString("Layer %1: %2").arg(index + 1)
                .arg(error.isEmpty() ? "the IDS engine failed" : error));
            return;
        }
        built_xml_[index] = ids_xml;
        buildNextLayer(index + 1);
    });
}

} // namespace bonsaiviewer::modules::filters
