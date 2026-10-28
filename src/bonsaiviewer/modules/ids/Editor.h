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

#ifndef IFCINTERFACE_MODULES_IDS_EDITOR_H
#define IFCINTERFACE_MODULES_IDS_EDITOR_H

#include "IdsDocument.h"

#include "../filters/FacetPages.h" // IfcPathProvider

#include <QWidget>

#include <vector>

class QCheckBox;
class QColor;
class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPlainTextEdit;
class QPushButton;
class QTabWidget;
class QTreeWidget;
class QTreeWidgetItem;

namespace bonsaiviewer::modules::filters { class SelectionEditor; }

namespace bonsaiviewer::modules::ids {

// BonsaiViewer24 — the content of the IDS panel: the file toolbar (New,
// Open, Save, Save As, Information, Run), the list of specifications and,
// for the selected one, its fields, its applicability and its requirements
// (the facet editor of the filters, in clause mode), and the results of the
// last run. IdsPanelView runs the document and isolates the results.
class IdsEditor : public QWidget {
    Q_OBJECT

public:
    IdsEditor(bonsaiviewer::services::IdsBridge& bridge,
              filters::IfcPathProvider ifc_path,
              QWidget* parent = nullptr);

    // The document, with the pending edits of the open specification.
    IdsDocument document();

    void setRunning(bool running);
    void setResults(const std::vector<SpecificationResult>& results);
    void setRunError(const QString& error);
    bool colorProperties() const;
    bool filterProperties() const;

    // Asks to save the pending changes; false when the user cancels.
    bool confirmDiscard();

signals:
    void runRequested();
    void isolateRequested(const std::vector<ElementRef>& elements);
    // Colour the passing elements green and the failing ones red (the
    // previous IDS colours are removed first); clear removes them.
    void colourRequested(const std::vector<ElementRef>& passed, const std::vector<ElementRef>& failed);
    void clearColoursRequested();
    // "Color Properties": colour the rows of the Properties panel by the
    // property and attribute requirements of the last run.
    void colorPropertiesToggled(bool enabled);
    // "Filter Properties": show only the rows the IDS checks.
    void filterPropertiesToggled(bool enabled);

private:
    void newDocument();
    void openDocument();
    bool saveDocument(bool ask_path);
    void editInformation();
    void setDocument(const IdsDocument& document, const QString& path);

    void syncSpecificationList();
    void selectSpecification(int index);
    void showSpecification(int index);
    void storeSpecification();
    void addSpecification(bool duplicate);
    void removeSpecification();
    void markModified();
    void updateTitle();
    void updateUsageHelp();

    void showResultMenu(const QPoint& position);
    void resultActivated(QTreeWidgetItem* item);
    std::vector<ElementRef> resultElements(QTreeWidgetItem* item, const QString& which) const;
    void colourItem(QTreeWidgetItem* item);
    void requirementDoubleClicked(QTreeWidgetItem* item);

    bonsaiviewer::services::IdsBridge& bridge_;
    filters::IfcPathProvider ifc_path_;
    IdsDocument document_;
    QString path_;
    bool modified_ = false;
    bool loading_ = false;
    int current_ = -1;
    std::vector<SpecificationResult> results_;

    QLabel* file_label_ = nullptr;
    QPushButton* run_button_ = nullptr;
    QPushButton* color_properties_button_ = nullptr;
    QPushButton* filter_properties_button_ = nullptr;
    QLabel* run_status_ = nullptr;
    QListWidget* spec_list_ = nullptr;
    QPushButton* remove_spec_button_ = nullptr;
    QPushButton* duplicate_spec_button_ = nullptr;
    QTabWidget* tabs_ = nullptr;
    QWidget* spec_page_ = nullptr;
    QLineEdit* name_edit_ = nullptr;
    QLineEdit* identifier_edit_ = nullptr;
    QPlainTextEdit* description_edit_ = nullptr;
    QPlainTextEdit* instructions_edit_ = nullptr;
    std::vector<QCheckBox*> version_checks_;
    QComboBox* usage_combo_ = nullptr;
    QLabel* usage_help_ = nullptr;
    QLabel* facets_note_ = nullptr; // shown when the facets are read-only
    bool facets_editable_ = true;
    filters::SelectionEditor* applicability_editor_ = nullptr;
    filters::SelectionEditor* requirements_editor_ = nullptr;
    QTreeWidget* results_tree_ = nullptr;
    QPushButton* colour_check_ = nullptr; // Color Objects
};

} // namespace bonsaiviewer::modules::ids

#endif // IFCINTERFACE_MODULES_IDS_EDITOR_H
