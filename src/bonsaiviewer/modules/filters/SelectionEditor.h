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

#ifndef IFCINTERFACE_MODULES_FILTERS_SELECTIONEDITOR_H
#define IFCINTERFACE_MODULES_FILTERS_SELECTIONEDITOR_H

#include "FacetPages.h"

#include "../../services/IdsBridge.h"

#include <QWidget>

#include <vector>

class QLabel;
class QListWidget;
class QPushButton;
class QStackedWidget;
class QTimer;
class QToolButton;

namespace bonsaiviewer::modules::filters {

// B06 — the editor of an IDS selection, shared by the filter layers (TI06)
// and the tables (TI07): the list of facets (exactly one entity facet plus
// any number of property, attribute, classification, material and partOf
// facets, all of which must pass), the page of the selected facet, and a
// live preview counting the elements selected in the active model, with
// "Isolate in the view". Facets are copied and pasted as IDS XML
// (FacetClipboard): Ctrl+C / Ctrl+V on the list, or its context menu.
class SelectionEditor : public QWidget {
    Q_OBJECT

public:
    // Selection: one entity facet plus other facets, with the live preview
    // (filters, tables). Clause: any facets, no preview — an applicability or
    // the requirements of an IDS specification (BonsaiViewer24).
    enum class Mode { Selection, Clause };

    SelectionEditor(bonsaiviewer::services::IdsBridge& bridge,
                    IfcPathProvider ifc_path,
                    QWidget* parent = nullptr,
                    Mode mode = Mode::Selection);

    // Clause mode: the title above the facet list, and the entity facet the
    // property lists follow when the clause has none (the applicability's).
    void setTitle(const QString& title);
    void setEntitySource(std::function<bonsaiviewer::services::IdsFacet()> source);

    void setFields(const bonsaiviewer::services::IdsBridge::SelectionFields& fields);
    // The edited facets, including the pending edits of the open page.
    bonsaiviewer::services::IdsBridge::SelectionFields fields();

    // A selection with one entity facet on IfcWall (and subtypes).
    static bonsaiviewer::services::IdsBridge::SelectionFields defaultFields();
    // Empty when the selection can be built, else what is missing.
    static QString validationError(const bonsaiviewer::services::IdsBridge::SelectionFields& fields);

    void schedulePreview();

signals:
    void changed();
    // "Isolate in the view" of the preview: EXPRESS ids of the active model.
    void isolateRequested(const std::vector<int>& express_ids);

private:
    void syncFacetList();
    void selectFacet(int index);
    void showFacet(int index);
    void storeFacetFromPage();
    void addFacet(bonsaiviewer::services::IdsFacet::Type type);
    void removeFacet(int index);
    void copyFacets(bool all);
    void pasteFacets();
    void showFacetMenu(const QPoint& position);
    void updateAddFacetMenu();
    bonsaiviewer::services::IdsFacet entityFacet() const;
    void runPreview();

    bonsaiviewer::services::IdsBridge& bridge_;
    IfcPathProvider ifc_path_;
    Mode mode_ = Mode::Selection;
    std::function<bonsaiviewer::services::IdsFacet()> entity_source_;
    QLabel* title_label_ = nullptr;
    bonsaiviewer::services::IdsBridge::SelectionFields fields_;
    int current_facet_ = -1;

    QListWidget* facet_list_ = nullptr;
    QToolButton* add_facet_button_ = nullptr;
    QPushButton* remove_facet_button_ = nullptr;
    QStackedWidget* facet_pages_ = nullptr;
    EntityFacetPage* entity_page_ = nullptr;
    PropertyFacetPage* property_page_ = nullptr;
    SimpleFacetPage* attribute_page_ = nullptr;
    SimpleFacetPage* classification_page_ = nullptr;
    SimpleFacetPage* material_page_ = nullptr;
    PartOfFacetPage* part_of_page_ = nullptr;

    QLabel* preview_label_ = nullptr;
    QPushButton* preview_button_ = nullptr;
    QPushButton* isolate_button_ = nullptr;
    QTimer* preview_timer_ = nullptr;
    int preview_generation_ = 0;
    std::vector<int> preview_ids_;
};

} // namespace bonsaiviewer::modules::filters

#endif // IFCINTERFACE_MODULES_FILTERS_SELECTIONEDITOR_H
