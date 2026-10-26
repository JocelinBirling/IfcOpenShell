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

#ifndef IFCINTERFACE_MODULES_FILTERS_CREATIONDIALOG_H
#define IFCINTERFACE_MODULES_FILTERS_CREATIONDIALOG_H

#include "FacetPages.h"

#include "../../components/Dialog.h"
#include "../../services/FilterStore.h"
#include "../../services/IdsBridge.h"

#include <vector>

class QListWidget;
class QCheckBox;
class QComboBox;
class QDialogButtonBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QStackedWidget;
class QTimer;
class QToolButton;

namespace bonsaiviewer::modules::filters {

// B06 — the Filter Creation Window: name, description and the layers,
// each with its selection (an IDS specification, or an AND marker) and its
// action list.
//
// The selection of a layer is a list of IDS facets: exactly one entity
// facet plus any number of property facets. Selecting a facet in the list
// shows the configuration page of its type (FacetPages), whose lists come
// from the schema and the active model. A live preview counts the
// elements the layer selects in the active model and can isolate them in
// the view.
//
// The dialog edits facets, not XML; on accept it asks the IdsBridge to
// serialise every selection layer to canonical IDS XML (built by
// IfcTester) and only then emits documentReady.
class FilterCreationDialog : public components::Dialog {
    Q_OBJECT

public:
    // The layer as edited here; is_and layers have no facets.
    struct EditLayer {
        bool is_and = false;
        bonsaiviewer::services::IdsBridge::SelectionFields fields;
        std::vector<bonsaiviewer::services::FilterAction> actions;
    };

    // `document` carries the id (Modify) or none (New); `layers` holds the
    // pre-parsed facets of each layer for Modify, or is empty.
    FilterCreationDialog(const bonsaiviewer::services::FilterDocument& document,
                         const std::vector<EditLayer>& layers,
                         bonsaiviewer::services::IdsBridge& ids_bridge,
                         IfcPathProvider ifc_path,
                         QWidget* parent = nullptr);

signals:
    // Emitted once every selection layer has been serialised to IDS XML.
    void documentReady(const bonsaiviewer::services::FilterDocument& document);
    // "Isolate in the view" of the preview: EXPRESS ids of the active model.
    void isolateRequested(const std::vector<int>& express_ids);

private:
    void accept() override;
    bool validate();
    void syncLayerList();
    void syncActionListRow(int row);
    void loadLayer(int index);
    void updateSelectionEnabled();
    void addLayer();
    void removeLayer(int index);
    void moveLayer(int index, int direction);
    void addAction();
    void removeAction(int index);
    void loadActionParameters(int row);
    void browseAutoColourProperty();
    void buildNextLayer(size_t index);
    bonsaiviewer::services::FilterAction* currentAction();

    // Facets of the current layer.
    void syncFacetList();
    void selectFacet(int index);
    void showFacet(int index);
    void addFacet(bonsaiviewer::services::IdsFacet::Type type);
    void removeFacet(int index);
    void storeFacetFromPage();
    bonsaiviewer::services::IdsFacet layerEntity() const;
    void updateAddFacetMenu();

    // Live preview.
    void schedulePreview();
    void runPreview();

    bonsaiviewer::services::FilterDocument document_;
    std::vector<EditLayer> layers_;
    bonsaiviewer::services::IdsBridge& ids_bridge_;
    IfcPathProvider ifc_path_;

    QListWidget* layer_list_ = nullptr;
    QListWidget* action_list_ = nullptr;
    QGroupBox* selection_box_ = nullptr;
    QComboBox* selection_type_combo_ = nullptr; // IDS specification / AND
    QStackedWidget* selection_stack_ = nullptr;

    QListWidget* facet_list_ = nullptr;
    QToolButton* add_facet_button_ = nullptr;
    QPushButton* remove_facet_button_ = nullptr;
    QStackedWidget* facet_pages_ = nullptr;
    EntityFacetPage* entity_page_ = nullptr;
    PropertyFacetPage* property_page_ = nullptr;
    int current_facet_ = -1;

    QLabel* preview_label_ = nullptr;
    QPushButton* preview_button_ = nullptr;
    QPushButton* isolate_button_ = nullptr;
    QTimer* preview_timer_ = nullptr;
    int preview_generation_ = 0;
    std::vector<int> preview_ids_;

    QComboBox* action_type_combo_ = nullptr;
    QPushButton* colour_button_ = nullptr;
    QSpinBox* transparency_spin_ = nullptr;
    QLineEdit* auto_pset_edit_ = nullptr;
    QLineEdit* auto_property_edit_ = nullptr;
    QWidget* action_params_ = nullptr;
    QDialogButtonBox* buttons_ = nullptr;
    int current_layer_ = -1;
    // True while the editor widgets are being filled from the model, so
    // their change signals do not write back.
    bool loading_ = false;
    std::vector<QString> built_xml_;
};

} // namespace bonsaiviewer::modules::filters

#endif // IFCINTERFACE_MODULES_FILTERS_CREATIONDIALOG_H
