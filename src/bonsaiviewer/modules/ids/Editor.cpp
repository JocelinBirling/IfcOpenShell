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

#include "Editor.h"

#include "../filters/SelectionEditor.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSaveFile>
#include <QScrollArea>
#include <QSettings>
#include <QSignalBlocker>
#include <QSplitter>
#include <QTabWidget>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <algorithm>
#include <map>

namespace bonsaiviewer::modules::ids {

using bonsaiviewer::services::IdsBridge;
using bonsaiviewer::services::IdsFacet;

namespace {

constexpr auto kLastDirectoryKey = "ids/lastDirectory";
const QStringList kVersions = {"IFC2X3", "IFC4", "IFC4X3_ADD2"};
constexpr int kShownFailures = 300; // failure rows listed per requirement

// Results tree item data: the specification, requirement and failure indexes.
constexpr int kSpecRole = Qt::UserRole;
constexpr int kRequirementRole = Qt::UserRole + 1;
constexpr int kFailureRole = Qt::UserRole + 2;

enum Tab { kSpecificationTab = 0, kApplicabilityTab, kRequirementsTab, kResultsTab };

QPlainTextEdit* shortTextEdit(QWidget* parent) {
    auto* edit = new QPlainTextEdit(parent);
    edit->setTabChangesFocus(true);
    edit->setFixedHeight(edit->fontMetrics().lineSpacing() * 3 + 12);
    return edit;
}

QString mark(bool status) {
    return status ? QStringLiteral("✔ ") : QStringLiteral("✘ ");
}

} // namespace

IdsEditor::IdsEditor(IdsBridge& bridge, filters::IfcPathProvider ifc_path, QWidget* parent)
    : QWidget(parent),
      bridge_(bridge),
      ifc_path_(std::move(ifc_path))
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);

    // File toolbar and Run.
    auto* toolbar = new QHBoxLayout();
    auto* new_button = new QPushButton("New", this);
    auto* open_button = new QPushButton("Open…", this);
    auto* save_button = new QPushButton("Save", this);
    auto* save_as_button = new QPushButton("Save As…", this);
    auto* info_button = new QPushButton("Information…", this);
    info_button->setToolTip("Title, author, version, date, purpose… of the IDS file");
    run_button_ = new QPushButton("Run", this);
    run_button_->setToolTip("Check the loaded models against the IDS");
    file_label_ = new QLabel(this);
    file_label_->setProperty("textRole", "secondary");
    for (QPushButton* button : {new_button, open_button, save_button, save_as_button, info_button}) {
        toolbar->addWidget(button);
    }
    toolbar->addWidget(file_label_, 1);
    run_status_ = new QLabel(this);
    run_status_->setProperty("textRole", "secondary");
    toolbar->addWidget(run_status_);
    toolbar->addWidget(run_button_);
    layout->addLayout(toolbar);

    auto* splitter = new QSplitter(Qt::Horizontal, this);

    // Specifications.
    auto* specs_side = new QWidget(splitter);
    auto* specs_layout = new QVBoxLayout(specs_side);
    specs_layout->setContentsMargins(0, 0, 0, 0);
    auto* spec_buttons = new QHBoxLayout();
    spec_buttons->addWidget(new QLabel("Specifications", specs_side), 1);
    auto* add_spec_button = new QPushButton("+ Add", specs_side);
    duplicate_spec_button_ = new QPushButton("Duplicate", specs_side);
    remove_spec_button_ = new QPushButton("Remove", specs_side);
    spec_buttons->addWidget(add_spec_button);
    spec_buttons->addWidget(duplicate_spec_button_);
    spec_buttons->addWidget(remove_spec_button_);
    specs_layout->addLayout(spec_buttons);
    spec_list_ = new QListWidget(specs_side);
    specs_layout->addWidget(spec_list_, 1);

    // The selected specification.
    tabs_ = new QTabWidget(splitter);
    spec_page_ = new QWidget();
    auto* form = new QFormLayout(spec_page_);
    name_edit_ = new QLineEdit(spec_page_);
    form->addRow("Name", name_edit_);
    identifier_edit_ = new QLineEdit(spec_page_);
    identifier_edit_->setPlaceholderText("optional, e.g. SPEC-01");
    form->addRow("Identifier", identifier_edit_);
    description_edit_ = shortTextEdit(spec_page_);
    description_edit_->setPlaceholderText("Why this information is required");
    form->addRow("Description", description_edit_);
    instructions_edit_ = shortTextEdit(spec_page_);
    instructions_edit_->setPlaceholderText("How to provide it, for the author of the model");
    form->addRow("Instructions", instructions_edit_);
    auto* versions = new QWidget(spec_page_);
    auto* versions_layout = new QHBoxLayout(versions);
    versions_layout->setContentsMargins(0, 0, 0, 0);
    for (const QString& version : kVersions) {
        auto* check = new QCheckBox(version, versions);
        versions_layout->addWidget(check);
        version_checks_.push_back(check);
    }
    versions_layout->addStretch(1);
    form->addRow("IFC versions", versions);
    usage_combo_ = new QComboBox(spec_page_);
    usage_combo_->addItem("Required", "required");
    usage_combo_->addItem("Optional", "optional");
    usage_combo_->addItem("Prohibited", "prohibited");
    form->addRow("Applicability", usage_combo_);
    usage_help_ = new QLabel(spec_page_);
    usage_help_->setProperty("textRole", "secondary");
    usage_help_->setWordWrap(true);
    form->addRow("", usage_help_);
    facets_note_ = new QLabel(spec_page_);
    facets_note_->setWordWrap(true);
    facets_note_->hide();
    form->addRow("", facets_note_);
    auto* spec_scroll = new QScrollArea(tabs_);
    spec_scroll->setWidgetResizable(true);
    spec_scroll->setFrameShape(QFrame::NoFrame);
    spec_scroll->setWidget(spec_page_);
    tabs_->addTab(spec_scroll, "Specification");

    applicability_editor_ = new filters::SelectionEditor(bridge_, ifc_path_, tabs_,
                                                         filters::SelectionEditor::Mode::Clause);
    applicability_editor_->setTitle("Applicability: the elements the specification applies to");
    tabs_->addTab(applicability_editor_, "Applicability");
    requirements_editor_ = new filters::SelectionEditor(bridge_, ifc_path_, tabs_,
                                                        filters::SelectionEditor::Mode::Clause);
    requirements_editor_->setTitle("Requirements: what those elements must have");
    requirements_editor_->setEntitySource([this]() {
        for (const IdsFacet& facet : applicability_editor_->fields().facets) {
            if (facet.type == IdsFacet::Type::Entity) return facet;
        }
        return IdsFacet();
    });
    tabs_->addTab(requirements_editor_, "Requirements");

    auto* results_page = new QWidget(tabs_);
    auto* results_layout = new QVBoxLayout(results_page);
    results_layout->setContentsMargins(0, 4, 0, 0);
    auto* colour_row = new QHBoxLayout();
    colour_check_ = new QCheckBox("Colour the objects: passing green, failing red", results_page);
    colour_check_->setToolTip("When checked, double-click a requirement to isolate its elements and colour "
                              "them: passing green, failing red. Unchecking removes the colours.");
    colour_row->addWidget(colour_check_);
    colour_row->addStretch(1);
    results_layout->addLayout(colour_row);
    results_tree_ = new QTreeWidget(results_page);
    results_tree_->setHeaderLabels({"Result", "Details"});
    results_tree_->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    results_tree_->setContextMenuPolicy(Qt::CustomContextMenu);
    results_tree_->setToolTip("Click a failing element to isolate it; right-click for more");
    results_layout->addWidget(results_tree_, 1);
    tabs_->addTab(results_page, "Results");

    splitter->addWidget(specs_side);
    splitter->addWidget(tabs_);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({220, 800});
    layout->addWidget(splitter, 1);

    connect(new_button, &QPushButton::clicked, this, &IdsEditor::newDocument);
    connect(open_button, &QPushButton::clicked, this, &IdsEditor::openDocument);
    connect(save_button, &QPushButton::clicked, this, [this]() { saveDocument(false); });
    connect(save_as_button, &QPushButton::clicked, this, [this]() { saveDocument(true); });
    connect(info_button, &QPushButton::clicked, this, &IdsEditor::editInformation);
    connect(run_button_, &QPushButton::clicked, this, [this]() {
        storeSpecification();
        emit runRequested();
    });
    connect(add_spec_button, &QPushButton::clicked, this, [this]() { addSpecification(false); });
    connect(duplicate_spec_button_, &QPushButton::clicked, this, [this]() { addSpecification(true); });
    connect(remove_spec_button_, &QPushButton::clicked, this, &IdsEditor::removeSpecification);
    connect(spec_list_, &QListWidget::currentRowChanged, this, [this](int row) {
        storeSpecification();
        showSpecification(row);
    });

    const auto edited = [this]() {
        if (loading_) return;
        markModified();
        if (current_ >= 0 && current_ < spec_list_->count()) {
            const QString name = name_edit_->text().trimmed();
            spec_list_->item(current_)->setText(name.isEmpty() ? QString("Unnamed") : name);
        }
    };
    connect(name_edit_, &QLineEdit::textChanged, this, edited);
    connect(identifier_edit_, &QLineEdit::textChanged, this, edited);
    connect(description_edit_, &QPlainTextEdit::textChanged, this, edited);
    connect(instructions_edit_, &QPlainTextEdit::textChanged, this, edited);
    for (QCheckBox* check : version_checks_) connect(check, &QCheckBox::toggled, this, edited);
    connect(usage_combo_, &QComboBox::currentIndexChanged, this, [this, edited](int) {
        updateUsageHelp();
        edited();
    });
    connect(applicability_editor_, &filters::SelectionEditor::changed, this, edited);
    connect(requirements_editor_, &filters::SelectionEditor::changed, this, edited);

    connect(results_tree_, &QTreeWidget::customContextMenuRequested, this, &IdsEditor::showResultMenu);
    connect(colour_check_, &QCheckBox::toggled, this, [this](bool checked) {
        if (!checked) emit clearColoursRequested();
    });
    connect(results_tree_, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem* item, int) {
        requirementDoubleClicked(item);
    });
    connect(results_tree_, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem* item, int) {
        resultActivated(item);
    });

    setDocument(IdsDocument(), QString());
}

IdsDocument IdsEditor::document() {
    storeSpecification();
    return document_;
}

// ------------------------------------------------------------------- files

void IdsEditor::setDocument(const IdsDocument& document, const QString& path) {
    document_ = document;
    path_ = path;
    modified_ = false;
    results_.clear();
    results_tree_->clear();
    run_status_->clear();
    current_ = -1;
    syncSpecificationList();
    selectSpecification(document_.specifications.empty() ? -1 : 0);
    updateTitle();
}

bool IdsEditor::confirmDiscard() {
    if (!modified_) return true;
    const auto answer = QMessageBox::question(this, "IDS", "Save the changes of the IDS file?",
                                              QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (answer == QMessageBox::Cancel) return false;
    return answer == QMessageBox::Discard || saveDocument(false);
}

void IdsEditor::newDocument() {
    storeSpecification();
    if (!confirmDiscard()) return;
    setDocument(IdsDocument(), QString());
    addSpecification(false);
    modified_ = false;
    updateTitle();
}

void IdsEditor::openDocument() {
    storeSpecification();
    if (!confirmDiscard()) return;
    QSettings settings;
    const QString path = QFileDialog::getOpenFileName(this, "Open an IDS file",
                                                      settings.value(kLastDirectoryKey).toString(),
                                                      "IDS (*.ids *.xml);;All files (*)");
    if (path.isEmpty()) return;
    settings.setValue(kLastDirectoryKey, QFileInfo(path).absolutePath());
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Open IDS", QString("Cannot read %1.").arg(path));
        return;
    }
    try {
        setDocument(IdsDocument::fromXml(file.readAll()), path);
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Open IDS", QString("Cannot open %1:\n%2").arg(QFileInfo(path).fileName(), e.what()));
    }
}

bool IdsEditor::saveDocument(bool ask_path) {
    storeSpecification();
    QByteArray xml;
    try {
        xml = document_.toXml();
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Save IDS", QString("The IDS cannot be written: %1.").arg(e.what()));
        return false;
    }
    QString path = path_;
    if (ask_path || path.isEmpty()) {
        QSettings settings;
        const QString title = document_.infoValue("title");
        const QString suggested = QDir(settings.value(kLastDirectoryKey).toString())
                                      .filePath((title.isEmpty() ? QString("Untitled") : title) + ".ids");
        path = QFileDialog::getSaveFileName(this, "Save the IDS file", path.isEmpty() ? suggested : path,
                                            "IDS (*.ids)");
        if (path.isEmpty()) return false;
        if (QFileInfo(path).suffix().isEmpty()) path += ".ids";
        settings.setValue(kLastDirectoryKey, QFileInfo(path).absolutePath());
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(xml) != xml.size() || !file.commit()) {
        QMessageBox::warning(this, "Save IDS", QString("Cannot write %1.").arg(path));
        return false;
    }
    path_ = path;
    modified_ = false;
    updateTitle();
    return true;
}

void IdsEditor::editInformation() {
    QDialog dialog(this);
    dialog.setWindowTitle("IDS information");
    auto* form = new QFormLayout(&dialog);
    const std::map<QString, QString> placeholders = {
        {"version", "X.Y (Y: minor change, X: models may pass or fail differently)"},
        {"author", "e-mail address"},
        {"date", "YYYY-MM-DD"},
        {"purpose", "e.g. coordination, cost estimation"},
        {"milestone", "e.g. Schematic Design, Construction"},
    };
    std::map<QString, QLineEdit*> edits;
    for (const QString& key : IdsDocument::infoKeys()) {
        auto* edit = new QLineEdit(document_.infoValue(key), &dialog);
        const auto placeholder = placeholders.find(key);
        if (placeholder != placeholders.end()) edit->setPlaceholderText(placeholder->second);
        edit->setMinimumWidth(360);
        form->addRow(key.left(1).toUpper() + key.mid(1), edit);
        edits[key] = edit;
    }
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted) return;
    for (const auto& [key, edit] : edits) document_.setInfo(key, edit->text());
    if (document_.infoValue("title").isEmpty()) document_.setInfo("title", "Untitled");
    markModified();
}

void IdsEditor::markModified() {
    if (modified_) return;
    modified_ = true;
    updateTitle();
}

void IdsEditor::updateTitle() {
    const QString name = path_.isEmpty() ? QString("New IDS") : QFileInfo(path_).fileName();
    file_label_->setText(QString("%1 — %2%3").arg(name, document_.infoValue("title"), modified_ ? " *" : ""));
}

// ---------------------------------------------------------- specifications

void IdsEditor::syncSpecificationList() {
    loading_ = true;
    {
        const QSignalBlocker blocker(spec_list_);
        spec_list_->clear();
        for (const SpecificationEdit& spec : document_.specifications) {
            spec_list_->addItem(spec.name.trimmed().isEmpty() ? QString("Unnamed") : spec.name);
        }
    }
    loading_ = false;
}

void IdsEditor::selectSpecification(int index) {
    {
        const QSignalBlocker blocker(spec_list_);
        spec_list_->setCurrentRow(index);
    }
    showSpecification(index);
}

void IdsEditor::showSpecification(int index) {
    current_ = -1;
    const bool valid = index >= 0 && index < static_cast<int>(document_.specifications.size());
    tabs_->setEnabled(valid);
    remove_spec_button_->setEnabled(valid);
    duplicate_spec_button_->setEnabled(valid);
    loading_ = true;
    const SpecificationEdit spec = valid ? document_.specifications[index] : SpecificationEdit();
    name_edit_->setText(valid ? spec.name : QString());
    identifier_edit_->setText(spec.identifier);
    description_edit_->setPlainText(spec.description);
    instructions_edit_->setPlainText(spec.instructions);
    for (QCheckBox* check : version_checks_) check->setChecked(spec.ifc_versions.contains(check->text()));
    const int usage = usage_combo_->findData(spec.usage);
    usage_combo_->setCurrentIndex(usage < 0 ? 0 : usage);
    applicability_editor_->setFields({spec.applicability});
    requirements_editor_->setFields({spec.requirements});
    facets_editable_ = spec.facetsEditable();
    tabs_->setTabEnabled(kApplicabilityTab, facets_editable_);
    facets_note_->setVisible(!facets_editable_);
    if (!facets_editable_) {
        facets_note_->setText(QString("⚠ The facets of this specification use IDS options the editor cannot show "
                                      "(%1). They are kept as they are, and run.")
                                  .arg(spec.original->reason));
    }
    loading_ = false;
    updateUsageHelp();
    current_ = valid ? index : -1;
}

void IdsEditor::storeSpecification() {
    if (current_ < 0 || current_ >= static_cast<int>(document_.specifications.size())) return;
    SpecificationEdit& spec = document_.specifications[current_];
    spec.name = name_edit_->text().trimmed();
    spec.identifier = identifier_edit_->text().trimmed();
    spec.description = description_edit_->toPlainText().trimmed();
    spec.instructions = instructions_edit_->toPlainText().trimmed();
    spec.ifc_versions.clear();
    for (QCheckBox* check : version_checks_) {
        if (check->isChecked()) spec.ifc_versions << check->text();
    }
    spec.usage = usage_combo_->currentData().toString();
    if (!spec.facetsEditable()) return; // kept as read
    spec.applicability = applicability_editor_->fields().facets;
    spec.requirements = requirements_editor_->fields().facets;
}

void IdsEditor::addSpecification(bool duplicate) {
    storeSpecification();
    SpecificationEdit spec;
    if (duplicate && current_ >= 0) {
        spec = document_.specifications[current_];
        spec.name += " (copy)";
        spec.identifier.clear();
    } else {
        spec.name = QString("Specification %1").arg(document_.specifications.size() + 1);
        spec.applicability.push_back(IdsFacet::entity("IfcWall"));
    }
    document_.specifications.push_back(spec);
    syncSpecificationList();
    selectSpecification(static_cast<int>(document_.specifications.size()) - 1);
    tabs_->setCurrentIndex(kSpecificationTab);
    name_edit_->setFocus();
    name_edit_->selectAll();
    markModified();
}

void IdsEditor::removeSpecification() {
    if (current_ < 0) return;
    const int index = current_;
    current_ = -1; // do not store the page back into the removed specification
    document_.specifications.erase(document_.specifications.begin() + index);
    syncSpecificationList();
    selectSpecification(std::min(index, static_cast<int>(document_.specifications.size()) - 1));
    markModified();
}

void IdsEditor::updateUsageHelp() {
    const QString usage = usage_combo_->currentData().toString();
    if (usage == "optional") {
        usage_help_->setText("Matching elements may exist; when they do, they must meet the requirements.");
    } else if (usage == "prohibited") {
        usage_help_->setText("No matching element may exist; the requirements are ignored.");
    } else {
        usage_help_->setText("At least one matching element must exist, and each must meet the requirements.");
    }
    tabs_->setTabEnabled(kRequirementsTab, usage != "prohibited" && facets_editable_);
}

// ----------------------------------------------------------------- results

void IdsEditor::setRunning(bool running) {
    run_button_->setEnabled(!running);
    if (running) run_status_->setText("Running…");
}

void IdsEditor::setRunError(const QString& error) {
    setRunning(false);
    run_status_->setText("⚠ Not run");
    QMessageBox::warning(this, "Run IDS", error);
}

void IdsEditor::setResults(const std::vector<SpecificationResult>& results) {
    setRunning(false);
    results_ = results;
    results_tree_->clear();
    int passed = 0;
    for (int s = 0; s < static_cast<int>(results_.size()); ++s) {
        const SpecificationResult& result = results_[s];
        if (result.status) ++passed;
        auto* spec_item = new QTreeWidgetItem(results_tree_);
        spec_item->setText(0, mark(result.status) + result.name);
        QString details = QString("%1 applicable, %2 failed").arg(result.applicable.size()).arg(result.failed.size());
        if (!result.note.isEmpty()) details += " — " + result.note;
        if (result.models_other_schema > 0) {
            details += QString(" (%1 model(s) of another IFC version skipped)").arg(result.models_other_schema);
        }
        spec_item->setText(1, details);
        spec_item->setData(0, kSpecRole, s);
        spec_item->setData(0, kRequirementRole, -1);
        spec_item->setData(0, kFailureRole, -1);
        for (int r = 0; r < static_cast<int>(result.requirements.size()); ++r) {
            const RequirementResult& requirement = result.requirements[r];
            auto* requirement_item = new QTreeWidgetItem(spec_item);
            requirement_item->setText(0, mark(requirement.status) + requirement.text);
            requirement_item->setText(1, QString("%1 passed, %2 failed").arg(requirement.passed).arg(requirement.failures.size()));
            requirement_item->setData(0, kSpecRole, s);
            requirement_item->setData(0, kRequirementRole, r);
            requirement_item->setData(0, kFailureRole, -1);
            const int shown = std::min<int>(kShownFailures, static_cast<int>(requirement.failures.size()));
            for (int f = 0; f < shown; ++f) {
                auto* failure_item = new QTreeWidgetItem(requirement_item);
                failure_item->setText(0, requirement.failures[f].label);
                failure_item->setText(1, requirement.failures[f].reason);
                failure_item->setToolTip(1, requirement.failures[f].reason);
                failure_item->setData(0, kSpecRole, s);
                failure_item->setData(0, kRequirementRole, r);
                failure_item->setData(0, kFailureRole, f);
            }
            if (shown < static_cast<int>(requirement.failures.size())) {
                auto* more = new QTreeWidgetItem(requirement_item);
                more->setText(0, QString("… %1 more: right-click the requirement to isolate them all")
                                     .arg(requirement.failures.size() - shown));
            }
        }
        spec_item->setExpanded(!result.status);
    }
    run_status_->setText(QString("%1 / %2 specification(s) passed").arg(passed).arg(results_.size()));
    tabs_->setCurrentIndex(kResultsTab);
}

// With "Colour the objects" checked: a double-click on a requirement
// isolates the applicable elements of its specification and colours them,
// failing that requirement red, the others green.
void IdsEditor::requirementDoubleClicked(QTreeWidgetItem* item) {
    if (!colour_check_->isChecked() || item == nullptr || !item->data(0, kSpecRole).isValid()) return;
    const int s = item->data(0, kSpecRole).toInt();
    if (item->data(0, kRequirementRole).toInt() < 0 || item->data(0, kFailureRole).toInt() >= 0) return;
    if (s < 0 || s >= static_cast<int>(results_.size())) return;
    if (results_[s].applicable.empty()) return;
    emit isolateRequested(results_[s].applicable);
    colourItem(item);
}

// One specification (its passing / failing elements) or one requirement
// (its failing elements red, the other applicable elements green).
void IdsEditor::colourItem(QTreeWidgetItem* item) {
    if (item == nullptr || !item->data(0, kSpecRole).isValid()) return;
    const int s = item->data(0, kSpecRole).toInt();
    const int r = item->data(0, kRequirementRole).toInt();
    if (s < 0 || s >= static_cast<int>(results_.size())) return;
    const SpecificationResult& result = results_[s];
    if (r < 0) {
        emit colourRequested(result.passed, result.failed);
        return;
    }
    const std::vector<ElementRef> failed = resultElements(item, "failed");
    std::vector<ElementRef> passed;
    for (const ElementRef& element : result.applicable) {
        const bool fails = std::any_of(failed.begin(), failed.end(), [&](const ElementRef& f) {
            return f.session_model_id == element.session_model_id && f.ifc_id == element.ifc_id;
        });
        if (!fails) passed.push_back(element);
    }
    emit colourRequested(passed, failed);
}

std::vector<ElementRef> IdsEditor::resultElements(QTreeWidgetItem* item, const QString& which) const {
    if (item == nullptr || !item->data(0, kSpecRole).isValid()) return {};
    const int s = item->data(0, kSpecRole).toInt();
    const int r = item->data(0, kRequirementRole).toInt();
    const int f = item->data(0, kFailureRole).toInt();
    if (s < 0 || s >= static_cast<int>(results_.size())) return {};
    const SpecificationResult& result = results_[s];
    if (r >= 0 && r < static_cast<int>(result.requirements.size())) {
        const auto& failures = result.requirements[r].failures;
        if (f >= 0 && f < static_cast<int>(failures.size())) return {failures[f].element};
        std::vector<ElementRef> elements;
        for (const FailureRow& failure : failures) elements.push_back(failure.element);
        return elements;
    }
    if (which == "passed") return result.passed;
    if (which == "failed") return result.failed;
    return result.applicable;
}

void IdsEditor::resultActivated(QTreeWidgetItem* item) {
    if (item == nullptr || item->data(0, kFailureRole).toInt() < 0) return;
    const auto elements = resultElements(item, {});
    if (!elements.empty()) emit isolateRequested(elements);
}

void IdsEditor::showResultMenu(const QPoint& position) {
    QTreeWidgetItem* item = results_tree_->itemAt(position);
    if (item == nullptr || !item->data(0, kSpecRole).isValid()) return;
    QMenu menu(this);
    std::vector<std::pair<QAction*, QString>> actions;
    if (item->data(0, kRequirementRole).toInt() >= 0) {
        const bool single = item->data(0, kFailureRole).toInt() >= 0;
        actions.push_back({menu.addAction(single ? "Isolate" : "Isolate the failing elements"), "failed"});
        if (!single) actions.push_back({menu.addAction("Colour this requirement (passing green, failing red)"), "colour"});
    } else {
        actions.push_back({menu.addAction("Isolate the applicable elements"), "applicable"});
        actions.push_back({menu.addAction("Isolate the passing elements"), "passed"});
        actions.push_back({menu.addAction("Isolate the failing elements"), "failed"});
        actions.push_back({menu.addAction("Colour this specification (passing green, failing red)"), "colour"});
    }
    menu.addSeparator();
    QAction* clear = menu.addAction("Remove the IDS colours");
    QAction* chosen = menu.exec(results_tree_->viewport()->mapToGlobal(position));
    if (chosen == clear) {
        emit clearColoursRequested();
        return;
    }
    for (const auto& [action, which] : actions) {
        if (action != chosen) continue;
        if (which == "colour") {
            colourItem(item);
            return;
        }
        const auto elements = resultElements(item, which);
        if (elements.empty()) {
            QMessageBox::information(this, "IDS results", "No element to isolate.");
        } else {
            emit isolateRequested(elements);
        }
    }
}

} // namespace bonsaiviewer::modules::ids
