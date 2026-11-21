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

#include "Dialogs.h"

#include "../property_selection/PropertyField.h"
#include "../../SessionState.h"
#include "../../services/IfcDataAccess.h"
#include "../../services/PropertyCatalog.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QTableWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

#include <algorithm>

namespace bonsaiviewer::modules::external_documents {

// --- JoinDialog ---------------------------------------------------------------

JoinDialog::JoinDialog(const ExternalDocument& document, const TableData& data,
                       SessionState& session, services::IfcDataAccess& data_access,
                       services::PropertyCatalog& catalog, QWidget* parent)
    : QDialog(parent), document_(document), data_(data),
      session_(session), data_access_(data_access), catalog_(catalog)
{
    setWindowTitle(QStringLiteral("Join — %1").arg(document.name));
    config_ = document.join;
    if (config_.table_key_column.isEmpty() && !data.columns.isEmpty()) {
        config_.table_key_column = data.columns.first();
    }
    if (config_.sheet.isEmpty() && !data.sheets.isEmpty()) {
        config_.sheet = data.sheets.first();
    }

    auto* layout = new QVBoxLayout(this);

    auto* form = new QFormLayout();
    model_key_combo_ = new QComboBox(this);
    model_key_combo_->addItems({QStringLiteral("GlobalId"), QStringLiteral("Name"),
                                QStringLiteral("Class"), QStringLiteral("Property")});
    model_key_combo_->setCurrentText(config_.model_key);
    form->addRow(QStringLiteral("Model key"), model_key_combo_);

    property_field_ = new property_selection::PropertyField(catalog_, this);
    property_field_->setValue(config_.property_pset, config_.property_name);
    property_field_->setVisible(config_.model_key == QLatin1String("Property"));
    form->addRow(QStringLiteral("Property"), property_field_);

    key_column_combo_ = new QComboBox(this);
    key_column_combo_->addItems(data.columns);
    key_column_combo_->setCurrentText(config_.table_key_column);
    form->addRow(QStringLiteral("Table key column"), key_column_combo_);

    sheet_combo_ = new QComboBox(this);
    sheet_combo_->addItems(data.sheets);
    sheet_combo_->setCurrentText(data.sheet);
    sheet_combo_->setVisible(!data.sheets.isEmpty());
    form->addRow(QStringLiteral("Sheet"), sheet_combo_);

    connect(model_key_combo_, &QComboBox::currentTextChanged, this, [this](const QString& key) {
        property_field_->setVisible(key == QLatin1String("Property"));
    });

    // Get Lucky: proposals on the first 10 rows, best first.
    auto* lucky_row = new QHBoxLayout();
    auto* lucky_button = new QPushButton(QStringLiteral("Get Lucky"), this);
    guess_combo_ = new QComboBox(this);
    guess_combo_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    lucky_row->addWidget(lucky_button);
    lucky_row->addWidget(guess_combo_, 1);
    form->addRow(QStringLiteral("Auto-detect"), lucky_row);
    connect(lucky_button, &QPushButton::clicked, this, [this]() {
        guesses_ = guessJoins(data_, session_, data_access_, &catalog_);
        guess_combo_->clear();
        for (const JoinGuess& guess : guesses_) {
            QString label = QStringLiteral("%1 → %2 (%3/10)").arg(guess.table_key_column, guess.model_key).arg(guess.matches);
            if (guess.model_key == QLatin1String("Property")) {
                label = QStringLiteral("%1 → %2.%3 (%4/10)")
                            .arg(guess.table_key_column, guess.property_pset, guess.property_name).arg(guess.matches);
            }
            guess_combo_->addItem(label);
        }
        if (!guesses_.isEmpty()) guess_combo_->setCurrentIndex(0);
    });
    connect(guess_combo_, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (index >= 0 && index < guesses_.size()) applyGuess(index);
    });

    layout->addLayout(form);

    preview_ = new QTableWidget(this);
    preview_->setColumnCount(data.columns.size());
    preview_->setHorizontalHeaderLabels(data.columns);
    preview_->verticalHeader()->setVisible(false);
    preview_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(new QLabel(QStringLiteral("Preview — first 10 rows, key column highlighted:"), this));
    layout->addWidget(preview_);
    rebuildPreview();
    connect(key_column_combo_, &QComboBox::currentTextChanged, this, [this]() { rebuildPreview(); });

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        config_.model_key = model_key_combo_->currentText();
        if (config_.model_key == QLatin1String("Property")) {
            config_.property_pset = property_field_->propertySet();
            config_.property_name = property_field_->propertyName();
        }
        config_.table_key_column = key_column_combo_->currentText();
        if (sheet_combo_->isVisible()) config_.sheet = sheet_combo_->currentText();
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);

    resize(760, 480);
}

void JoinDialog::applyGuess(int index) {
    const JoinGuess& guess = guesses_.at(index);
    key_column_combo_->setCurrentText(guess.table_key_column);
    model_key_combo_->setCurrentText(guess.model_key);
    if (guess.model_key == QLatin1String("Property")) {
        property_field_->setValue(guess.property_pset, guess.property_name);
    }
    rebuildPreview();
}

void JoinDialog::rebuildPreview() {
    if (preview_ == nullptr) return;
    const int key_column = data_.columns.indexOf(key_column_combo_->currentText());
    const int rows = std::min<int>(10, data_.rows.size());
    preview_->setRowCount(rows);
    for (int row = 0; row < rows; ++row) {
        const QStringList& values = data_.rows.at(row);
        for (int column = 0; column < data_.columns.size(); ++column) {
            auto* item = new QTableWidgetItem(column < values.size() ? values.at(column) : QString());
            if (column == key_column) {
                item->setBackground(QApplication::palette().color(QPalette::Highlight));
                item->setForeground(QApplication::palette().color(QPalette::HighlightedText));
            }
            preview_->setItem(row, column, item);
        }
    }
    preview_->resizeColumnsToContents();
}

// --- PlanDialog ---------------------------------------------------------------

PlanDialog::PlanDialog(const ExternalDocument& document, QWidget* parent)
    : QDialog(parent), config_(document.plan), source_path_(document.path)
{
    setWindowTitle(QStringLiteral("Plan Configuration — %1").arg(document.name));

    auto* layout = new QVBoxLayout(this);
    auto* form = new QFormLayout();

    auto* source_row = new QHBoxLayout();
    auto* source_label = new QLabel(QFileInfo(source_path_).fileName(), this);
    source_label->setToolTip(source_path_);
    auto* change_button = new QPushButton(QStringLiteral("Change Source…"), this);
    connect(change_button, &QPushButton::clicked, this, [this, source_label]() {
        const QString path = QFileDialog::getOpenFileName(
            this, QStringLiteral("Change plan source"), source_path_,
            QStringLiteral("2D plans (*.pdf *.png *.jpg *.jpeg *.svg *.dxf *.dwg);;All Files (*)"));
        if (!path.isEmpty()) {
            source_path_ = path; // the current transform is kept
            source_label->setText(QFileInfo(path).fileName());
            source_label->setToolTip(path);
        }
    });
    source_row->addWidget(source_label, 1);
    source_row->addWidget(change_button);
    form->addRow(QStringLiteral("Source"), source_row);

    height_spin_ = new QDoubleSpinBox(this);
    height_spin_->setRange(-1000.0, 1000.0);
    height_spin_->setDecimals(3);
    height_spin_->setSingleStep(0.1);
    height_spin_->setSuffix(QStringLiteral(" m"));
    height_spin_->setValue(config_.height_z);
    form->addRow(QStringLiteral("Height (Z)"), height_spin_);

    rot_x_spin_ = new QDoubleSpinBox(this);
    rot_y_spin_ = new QDoubleSpinBox(this);
    rot_z_spin_ = new QDoubleSpinBox(this);
    for (QDoubleSpinBox* spin : {rot_x_spin_, rot_y_spin_, rot_z_spin_}) {
        spin->setRange(-360.0, 360.0);
        spin->setDecimals(1);
        spin->setSuffix(QStringLiteral("°"));
    }
    rot_x_spin_->setValue(config_.rot_x);
    rot_y_spin_->setValue(config_.rot_y);
    rot_z_spin_->setValue(config_.rot_z);
    auto* rot_row = new QHBoxLayout();
    rot_row->addWidget(rot_x_spin_);
    rot_row->addWidget(rot_y_spin_);
    rot_row->addWidget(rot_z_spin_);
    form->addRow(QStringLiteral("Rotation X/Y/Z"), rot_row);

    scale_spin_ = new QDoubleSpinBox(this);
    scale_spin_->setRange(0.000001, 1000000.0);
    scale_spin_->setDecimals(6);
    scale_spin_->setValue(config_.scale);
    form->addRow(QStringLiteral("Scale"), scale_spin_);

    trans_x_spin_ = new QDoubleSpinBox(this);
    trans_y_spin_ = new QDoubleSpinBox(this);
    trans_z_spin_ = new QDoubleSpinBox(this);
    for (QDoubleSpinBox* spin : {trans_x_spin_, trans_y_spin_, trans_z_spin_}) {
        spin->setRange(-100000.0, 100000.0);
        spin->setDecimals(3);
        spin->setSingleStep(0.1);
        spin->setSuffix(QStringLiteral(" m"));
    }
    trans_x_spin_->setValue(config_.trans_x);
    trans_y_spin_->setValue(config_.trans_y);
    trans_z_spin_->setValue(config_.trans_z);
    auto* trans_row = new QHBoxLayout();
    trans_row->addWidget(trans_x_spin_);
    trans_row->addWidget(trans_y_spin_);
    trans_row->addWidget(trans_z_spin_);
    form->addRow(QStringLiteral("Translation X/Y/Z"), trans_row);

    trans_plan_check_ = new QCheckBox(QStringLiteral("Relative to the plan axes (following its rotations)"), this);
    trans_plan_check_->setChecked(config_.trans_relative_to_plan);
    form->addRow(QString(), trans_plan_check_);

    const bool is_pdf = QFileInfo(source_path_).suffix().toLower() == QLatin1String("pdf");
    page_spin_ = new QSpinBox(this);
    page_spin_->setRange(0, 999);
    page_spin_->setValue(config_.page);
    page_spin_->setVisible(is_pdf);
    form->addRow(QStringLiteral("Page (PDF)"), page_spin_);

    auto* opacity_row = new QHBoxLayout();
    opacity_slider_ = new QSlider(Qt::Horizontal, this);
    opacity_slider_->setRange(0, 100);
    opacity_slider_->setValue(int(config_.opacity * 100.0));
    opacity_label_ = new QLabel(QStringLiteral("%1 %").arg(opacity_slider_->value()), this);
    connect(opacity_slider_, &QSlider::valueChanged, this, [this](int value) {
        opacity_label_->setText(QStringLiteral("%1 %").arg(value));
    });
    opacity_row->addWidget(opacity_slider_, 1);
    opacity_row->addWidget(opacity_label_);
    form->addRow(QStringLiteral("Opacity"), opacity_row);

    layout->addLayout(form);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("Apply"));
    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        config_.height_z = height_spin_->value();
        config_.rot_x = rot_x_spin_->value();
        config_.rot_y = rot_y_spin_->value();
        config_.rot_z = rot_z_spin_->value();
        config_.scale = scale_spin_->value();
        config_.trans_x = trans_x_spin_->value();
        config_.trans_y = trans_y_spin_->value();
        config_.trans_z = trans_z_spin_->value();
        config_.trans_relative_to_plan = trans_plan_check_->isChecked();
        if (page_spin_->isVisible()) config_.page = page_spin_->value();
        config_.opacity = opacity_slider_->value() / 100.0;
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);

    resize(520, 420);
}

// --- ReportDialog -------------------------------------------------------------

ReportDialog::ReportDialog(const QString& document_name, const TableData& data, const MatchReport& report,
                           QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Match Report — %1").arg(document_name));
    auto* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(QStringLiteral("%1 matched row(s), %2 unmatched, %3 duplicate, %4 unmatched object(s)")
                                      .arg(report.matched_rows).arg(report.unmatched_rows)
                                      .arg(report.duplicate_rows).arg(report.unmatched_objects), this));

    auto addSection = [&](const QString& title, const QStringList& headers,
                          const QList<QStringList>& rows) {
        auto* box = new QGroupBox(title, this);
        auto* box_layout = new QVBoxLayout(box);
        auto* tree = new QTreeWidget(box);
        tree->setHeaderLabels(headers);
        tree->setRootIsDecorated(false);
        for (const QStringList& row : rows) {
            auto* item = new QTreeWidgetItem(row);
            tree->addTopLevelItem(item);
        }
        tree->resizeColumnToContents(0);
        box_layout->addWidget(tree);
        layout->addWidget(box);
        return tree;
    };

    addSection(QStringLiteral("Unmatched rows (no loaded object)"), data.columns, report.unmatched_row_values);
    addSection(QStringLiteral("Duplicate rows (key already matched by an earlier row)"), data.columns,
               report.duplicate_row_values);

    auto* box = new QGroupBox(QStringLiteral("Unmatched objects (no row in the file)"), this);
    auto* box_layout = new QVBoxLayout(box);
    objects_tree_ = new QTreeWidget(box);
    objects_tree_->setHeaderLabels({QStringLiteral("Model"), QStringLiteral("Class"), QStringLiteral("Name"),
                                    QStringLiteral("GlobalId"), QStringLiteral("Key")});
    objects_tree_->setRootIsDecorated(false);
    for (const UnmatchedObjectInfo& info : report.unmatched_object_values) {
        auto* item = new QTreeWidgetItem({info.model, info.ifc_class, info.name, info.global_id, info.key});
        item->setData(0, Qt::UserRole, info.object_id);
        objects_tree_->addTopLevelItem(item);
    }
    connect(objects_tree_, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem* item) {
        const std::uint32_t object_id = item->data(0, Qt::UserRole).toUInt();
        if (object_id != 0) emit objectSelected(object_id);
    });
    box_layout->addWidget(objects_tree_);
    layout->addWidget(box);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    layout->addWidget(buttons);
    resize(720, 560);
}

} // namespace bonsaiviewer::modules::external_documents
