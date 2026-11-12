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

#include "ClashTopicDialog.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

namespace bonsaiviewer::modules::rule {

namespace {

const QString kOnePattern = QStringLiteral("{id} {rule} — {source} / {target}");
const QString kGroupPattern = QStringLiteral("{rule} — {n} clashes");
const QString kFromCriticity = QStringLiteral("(from the criticity)");

// An editable list: the values of the BCF file, `preferred` selected (added when missing).
QComboBox* valueBox(const QStringList& values, const QString& preferred, QWidget* parent) {
    auto* box = new QComboBox(parent);
    box->setEditable(true);
    box->addItems(values);
    if (!preferred.isEmpty() && box->findText(preferred) < 0) box->insertItem(0, preferred);
    box->setCurrentText(preferred.isEmpty() ? values.value(0) : preferred);
    return box;
}

QStringList splitLabels(const QString& text) {
    QStringList labels;
    for (const QString& part : text.split(',')) {
        const QString label = part.trimmed();
        if (!label.isEmpty() && !labels.contains(label)) labels << label;
    }
    return labels;
}

} // namespace

ClashTopicDialog::ClashTopicDialog(std::vector<const ResultRow*> rows, bool per_result,
                                   const ports::bcf::Extensions& extensions, const topic::PriorityTable& priorities,
                                   bool has_context, const QString& bcf_name, QWidget* parent)
    : QDialog(parent), rows_(std::move(rows)), priorities_(priorities), bcf_name_(bcf_name) {
    setWindowTitle("Create a BCF Topic");
    resize(640, 720);
    auto* layout = new QVBoxLayout(this);

    auto* mode = new QGroupBox("Topics", this);
    auto* mode_layout = new QVBoxLayout(mode);
    one_ = new QRadioButton("One topic for the selected results", mode);
    each_ = new QRadioButton("One topic per result", mode);
    auto* group = new QButtonGroup(this);
    group->addButton(one_);
    group->addButton(each_);
    (per_result && rows_.size() > 1 ? each_ : one_)->setChecked(true);
    each_->setEnabled(rows_.size() > 1);
    mode_layout->addWidget(one_);
    mode_layout->addWidget(each_);
    summary_ = new QLabel(mode);
    summary_->setProperty("textRole", "secondary");
    summary_->setWordWrap(true);
    mode_layout->addWidget(summary_);
    layout->addWidget(mode);

    auto* fields = new QGroupBox("Fields", this);
    auto* form = new QFormLayout(fields);
    title_ = new QLineEdit(fields);
    title_->setToolTip("{rule}, {source}, {target}, {measure} and {n} (the number of results) are replaced by the "
                       "values of the first result of each topic; {id} by the IDs of its results (R-0042).");
    form->addRow("Title", title_);
    type_ = valueBox(extensions.topic_types, "Clash", fields);
    form->addRow("Type", type_);
    status_ = valueBox(extensions.topic_statuses, extensions.topic_statuses.contains("Open") ? "Open" : extensions.topic_statuses.value(0, "Open"), fields);
    form->addRow("Status", status_);
    priority_ = new QComboBox(fields);
    priority_->setEditable(true);
    priority_->addItem(kFromCriticity);
    priority_->addItems(extensions.priorities);
    for (const auto& [criticity, value] : priorities_) {
        if (priority_->findText(value) < 0) priority_->addItem(value);
    }
    priority_->setToolTip("From the criticity: the table of Settings > BCF, the highest criticity of each topic wins.");
    form->addRow("Priority", priority_);
    labels_label_ = new QLabel("Labels", fields);
    labels_ = new QLineEdit(fields);
    labels_->setToolTip("Separated by commas.");
    form->addRow(labels_label_, labels_);
    assigned_ = valueBox(extensions.users, QString(), fields);
    assigned_->insertItem(0, QString());
    assigned_->setCurrentIndex(0);
    form->addRow("Assigned to", assigned_);
    stage_ = valueBox(extensions.stages, QString(), fields);
    stage_->insertItem(0, QString());
    stage_->setCurrentIndex(0);
    form->addRow("Stage", stage_);
    auto* due_row = new QWidget(fields);
    auto* due_layout = new QHBoxLayout(due_row);
    due_layout->setContentsMargins(0, 0, 0, 0);
    has_due_ = new QCheckBox(due_row);
    due_ = new QDateEdit(QDate::currentDate().addDays(14), due_row);
    due_->setCalendarPopup(true);
    due_->setEnabled(false);
    connect(has_due_, &QCheckBox::toggled, due_, &QWidget::setEnabled);
    due_layout->addWidget(has_due_);
    due_layout->addWidget(due_, 1);
    form->addRow("Due date", due_row);
    description_ = new QPlainTextEdit(fields);
    description_->setMinimumHeight(150);
    form->addRow("Description", description_);
    comment_ = new QPlainTextEdit(fields);
    comment_->setPlaceholderText("Optional: the first comment of each topic.");
    comment_->setMaximumHeight(70);
    form->addRow("Comment", comment_);
    layout->addWidget(fields, 1);

    auto* view = new QGroupBox("Viewpoints", this);
    auto* view_layout = new QVBoxLayout(view);
    snapshot_ = new QCheckBox("Take the snapshots", view);
    snapshot_->setChecked(true);
    snapshot_->setToolTip("Each viewpoint gets the image of the 3D view: the elements of the results alone, framed.");
    lines_ = new QCheckBox("Draw the closest points as lines", view);
    lines_->setChecked(true);
    context_ = new QCheckBox("Show the saved context", view);
    context_->setEnabled(has_context);
    context_->setToolTip(has_context ? "The context saved in the ribbon (Save Context) stays visible around the results."
                                     : "No context saved (CONTEXT VIEW > Save Context).");
    view_layout->addWidget(snapshot_);
    view_layout->addWidget(lines_);
    view_layout->addWidget(context_);
    layout->addWidget(view);

    mark_valid_ = new QCheckBox("Mark the results as Valid", this);
    mark_valid_->setChecked(true);
    layout->addWidget(mark_valid_);
    int invalid = 0, excepted = 0;
    for (const ResultRow* row : rows_) {
        if (row->review == ReviewStatus::Invalid) ++invalid;
        if (!row->status && !row->complex) ++excepted;
    }
    if (invalid > 0 || excepted > 0) {
        auto* warning = new QLabel(QString("Among the results: %1 reviewed Invalid, %2 excepted by the rule.")
                                       .arg(invalid)
                                       .arg(excepted),
                                   this);
        warning->setStyleSheet("color: #B8860B;");
        layout->addWidget(warning);
    }

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)->setText("Create");
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);

    generated_ = topic::description(rows_);
    connect(one_, &QRadioButton::toggled, this, &ClashTopicDialog::modeChanged);
    modeChanged();
}

void ClashTopicDialog::modeChanged() {
    const bool each = each_->isChecked();
    const size_t topics = each ? rows_.size() : 1;
    title_->setText(each || rows_.size() == 1 ? kOnePattern : kGroupPattern);
    labels_label_->setText(each ? "Extra labels" : "Labels");
    labels_->setText(each ? QString() : topic::labels(rows_).join(", "));
    labels_->setPlaceholderText(each ? "Added to the labels of each topic (Clash, rule, criticity, actor)" : QString());
    if (each) {
        description_->setPlainText(topic::describe(*rows_.front()));
        description_->setReadOnly(true);
        description_->setToolTip("The description of the first topic; each topic describes its own result.");
    } else {
        description_->setPlainText(generated_);
        description_->setReadOnly(false);
        description_->setToolTip("Generated from the results; it can be edited.");
    }
    summary_->setText(QString("%1 result(s) → %2 topic(s) in %3").arg(rows_.size()).arg(topics).arg(bcf_name_));
}

ClashTopicOptions ClashTopicDialog::options() const {
    ClashTopicOptions options;
    options.per_result = each_->isChecked();
    options.title = title_->text().trimmed();
    options.type = type_->currentText().trimmed();
    options.status = status_->currentText().trimmed();
    options.priority = priority_->currentText() == kFromCriticity ? QString() : priority_->currentText().trimmed();
    options.labels = splitLabels(labels_->text());
    options.assigned_to = assigned_->currentText().trimmed();
    options.stage = stage_->currentText().trimmed();
    if (has_due_->isChecked()) options.due_date = due_->date().toString(Qt::ISODate);
    if (!options.per_result && description_->toPlainText() != generated_) options.description = description_->toPlainText();
    options.comment = comment_->toPlainText().trimmed();
    options.mark_valid = mark_valid_->isChecked();
    options.context = context_->isEnabled() && context_->isChecked();
    options.snapshot = snapshot_->isChecked();
    options.lines = lines_->isChecked();
    return options;
}

} // namespace bonsaiviewer::modules::rule
