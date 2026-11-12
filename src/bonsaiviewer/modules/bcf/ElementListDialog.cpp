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

#include "ElementListDialog.h"

#include "../../services/BcfViewpoints.h"
#include "../../services/PopupService.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QSet>
#include <QTableWidget>
#include <QVBoxLayout>

#include <algorithm>
#include <set>

namespace bonsaiviewer::modules::bcf {

namespace bcf = bonsaiviewer::ports::bcf;

namespace {

constexpr int kGuidRole = Qt::UserRole;
constexpr int kColourRole = Qt::UserRole + 1;

QColor colourOf(const QString& hex) {
    return QColor(QStringLiteral("#") + (hex.size() == 8 ? hex.mid(2) : hex));
}

QString hexOf(const QColor& colour) {
    return colour.name(QColor::HexRgb).mid(1).toUpper();
}

} // namespace

ElementListDialog::ElementListDialog(Kind kind, const bcf::VisualizationInfo& info,
                                     services::BcfViewpoints& viewpoints, QWidget* parent)
    : QDialog(parent), kind_(kind), info_(info), viewpoints_(viewpoints) {
    static const char* const titles[] = {"Selection", "Visibility", "Colours"};
    setWindowTitle(QString("Viewpoint — %1").arg(titles[int(kind)]));
    resize(620, 480);
    auto* layout = new QVBoxLayout(this);
    if (kind_ == Kind::Visibility) {
        default_visible_ = new QCheckBox("Visible by default (the list holds the hidden elements)", this);
        default_visible_->setToolTip("Checked: everything is shown except the listed elements.\n"
                                     "Unchecked: only the listed elements are shown.");
        default_visible_->setChecked(info_.default_visibility);
        connect(default_visible_, &QCheckBox::toggled, this, [this](bool checked) { info_.default_visibility = checked; });
        layout->addWidget(default_visible_);
    }
    table_ = new QTableWidget(0, kind_ == Kind::Colours ? 4 : 3, this);
    QStringList headers{"GlobalId", "Name", "Class"};
    if (kind_ == Kind::Colours) headers.prepend("Colour");
    table_->setHorizontalHeaderLabels(headers);
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->verticalHeader()->setVisible(false);
    layout->addWidget(table_, 1);
    auto* note = new QLabel("Elements of models that are not loaded keep their GlobalId, without a name.", this);
    note->setProperty("textRole", "secondary");
    layout->addWidget(note);

    auto* row = new QHBoxLayout();
    auto* remove = new QPushButton("Remove", this);
    auto* add = new QPushButton(kind_ == Kind::Colours ? "Add the Selected Elements…" : "Add the Selected Elements", this);
    add->setToolTip("Add the elements selected in the viewer");
    row->addWidget(remove);
    row->addWidget(add);
    if (kind_ == Kind::Colours) {
        auto* colour = new QPushButton("Change the Colour…", this);
        row->addWidget(colour);
        connect(colour, &QPushButton::clicked, this, &ElementListDialog::changeColour);
    }
    row->addStretch(1);
    layout->addLayout(row);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(remove, &QPushButton::clicked, this, &ElementListDialog::removeRows);
    connect(add, &QPushButton::clicked, this, &ElementListDialog::addSelection);
    fill();
}

void ElementListDialog::fill() {
    table_->setRowCount(0);
    const auto addRow = [this](const bcf::Component& component, const QString& colour) {
        const int row = table_->rowCount();
        table_->insertRow(row);
        int column = 0;
        if (kind_ == Kind::Colours) {
            auto* swatch = new QTableWidgetItem(colour);
            swatch->setBackground(colourOf(colour));
            swatch->setData(kColourRole, colour);
            table_->setItem(row, column++, swatch);
        }
        const auto [name, ifc_class] = viewpoints_.describe(component.ifc_guid);
        auto* guid = new QTableWidgetItem(component.ifc_guid);
        guid->setData(kGuidRole, component.ifc_guid);
        guid->setData(kColourRole, colour);
        table_->setItem(row, column++, guid);
        table_->setItem(row, column++, new QTableWidgetItem(name));
        table_->setItem(row, column, new QTableWidgetItem(ifc_class));
    };
    switch (kind_) {
    case Kind::Selection:
        for (const auto& component : info_.selection) addRow(component, {});
        break;
    case Kind::Visibility:
        for (const auto& component : info_.exceptions) addRow(component, {});
        break;
    case Kind::Colours:
        for (const auto& coloring : info_.coloring) {
            for (const auto& component : coloring.components) addRow(component, coloring.color);
        }
        break;
    }
}

// The rows' (GlobalId, colour) pairs.
void ElementListDialog::removeRows() {
    const int guid_column = kind_ == Kind::Colours ? 1 : 0;
    std::set<std::pair<QString, QString>> removed;
    for (const auto* item : table_->selectedItems()) {
        const auto* guid = table_->item(item->row(), guid_column);
        removed.insert({guid->data(kGuidRole).toString(), guid->data(kColourRole).toString()});
    }
    const auto drop = [&removed](std::vector<bcf::Component>& list, const QString& colour) {
        list.erase(std::remove_if(list.begin(), list.end(),
                                  [&](const bcf::Component& c) { return removed.count({c.ifc_guid, colour}) != 0; }),
                   list.end());
    };
    switch (kind_) {
    case Kind::Selection: drop(info_.selection, {}); break;
    case Kind::Visibility: drop(info_.exceptions, {}); break;
    case Kind::Colours:
        for (auto& coloring : info_.coloring) drop(coloring.components, coloring.color);
        info_.coloring.erase(std::remove_if(info_.coloring.begin(), info_.coloring.end(),
                                            [](const bcf::Coloring& c) { return c.components.empty(); }),
                             info_.coloring.end());
        break;
    }
    fill();
}

void ElementListDialog::addSelection() {
    const auto selected = viewpoints_.selectedComponents();
    if (selected.empty()) {
        services::popup::information(this, "Select elements in the viewer first.");
        return;
    }
    const auto merge = [](std::vector<bcf::Component>& list, const std::vector<bcf::Component>& added) {
        QSet<QString> present;
        for (const auto& component : list) present.insert(component.ifc_guid);
        for (const auto& component : added) {
            if (!present.contains(component.ifc_guid)) list.push_back(component);
        }
    };
    switch (kind_) {
    case Kind::Selection: merge(info_.selection, selected); break;
    case Kind::Visibility: merge(info_.exceptions, selected); break;
    case Kind::Colours: {
        const QColor colour = QColorDialog::getColor(Qt::red, this, "Colour of the selected elements");
        if (!colour.isValid()) return;
        // An element has one colour: it leaves its former group.
        QSet<QString> moved;
        for (const auto& component : selected) moved.insert(component.ifc_guid);
        for (auto& coloring : info_.coloring) {
            coloring.components.erase(std::remove_if(coloring.components.begin(), coloring.components.end(),
                                                     [&](const bcf::Component& c) { return moved.contains(c.ifc_guid); }),
                                      coloring.components.end());
        }
        const QString hex = hexOf(colour);
        auto group = std::find_if(info_.coloring.begin(), info_.coloring.end(),
                                  [&hex](const bcf::Coloring& c) { return c.color.right(6).compare(hex, Qt::CaseInsensitive) == 0; });
        if (group == info_.coloring.end()) {
            info_.coloring.push_back({hex, {}});
            group = std::prev(info_.coloring.end());
        }
        merge(group->components, selected);
        info_.coloring.erase(std::remove_if(info_.coloring.begin(), info_.coloring.end(),
                                            [](const bcf::Coloring& c) { return c.components.empty(); }),
                             info_.coloring.end());
        break;
    }
    }
    fill();
}

// The colour of the groups of the selected rows.
void ElementListDialog::changeColour() {
    QSet<QString> colours;
    for (const auto* item : table_->selectedItems()) {
        colours.insert(table_->item(item->row(), 1)->data(kColourRole).toString());
    }
    if (colours.isEmpty()) return;
    const QColor colour = QColorDialog::getColor(colourOf(*colours.begin()), this, "Colour");
    if (!colour.isValid()) return;
    for (auto& coloring : info_.coloring) {
        if (colours.contains(coloring.color)) coloring.color = hexOf(colour);
    }
    fill();
}

} // namespace bonsaiviewer::modules::bcf
