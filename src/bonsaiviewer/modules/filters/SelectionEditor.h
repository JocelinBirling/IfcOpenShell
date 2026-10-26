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
// any number of property facets, all of which must pass), the page of the
// selected facet, and a live preview counting the elements selected in the
// active model, with "Isolate in the view".
class SelectionEditor : public QWidget {
    Q_OBJECT

public:
    SelectionEditor(bonsaiviewer::services::IdsBridge& bridge,
                    IfcPathProvider ifc_path,
                    QWidget* parent = nullptr);

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
    void updateAddFacetMenu();
    bonsaiviewer::services::IdsFacet entityFacet() const;
    void runPreview();

    bonsaiviewer::services::IdsBridge& bridge_;
    IfcPathProvider ifc_path_;
    bonsaiviewer::services::IdsBridge::SelectionFields fields_;
    int current_facet_ = -1;

    QListWidget* facet_list_ = nullptr;
    QToolButton* add_facet_button_ = nullptr;
    QPushButton* remove_facet_button_ = nullptr;
    QStackedWidget* facet_pages_ = nullptr;
    EntityFacetPage* entity_page_ = nullptr;
    PropertyFacetPage* property_page_ = nullptr;

    QLabel* preview_label_ = nullptr;
    QPushButton* preview_button_ = nullptr;
    QPushButton* isolate_button_ = nullptr;
    QTimer* preview_timer_ = nullptr;
    int preview_generation_ = 0;
    std::vector<int> preview_ids_;
};

} // namespace bonsaiviewer::modules::filters

#endif // IFCINTERFACE_MODULES_FILTERS_SELECTIONEDITOR_H
