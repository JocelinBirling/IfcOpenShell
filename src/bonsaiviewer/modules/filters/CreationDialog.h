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

#include "LayerDialog.h"

#include "../../components/Dialog.h"
#include "../../services/FilterStore.h"
#include "../../services/IdsBridge.h"

#include <vector>

class QDialogButtonBox;
class QLabel;
class QTableWidget;

namespace bonsaiviewer::modules::filters {

// B06 — the Filter Creation Window (New / Modify Filter): name,
// description and the layers in a summary table, one row per layer (type,
// selection in one line, actions). A layer is edited in its own window
// (LayerDialog): double-click or Edit. The problems of the filter are
// listed under the table as they appear.
//
// On accept the dialog asks the IdsBridge to serialise every selection
// layer to canonical IDS XML and only then emits documentReady.
class FilterCreationDialog : public components::Dialog {
    Q_OBJECT

public:
    using EditLayer = FilterEditLayer;

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
    // "Isolate in the view" of a layer's preview: EXPRESS ids of the active model.
    void isolateRequested(const std::vector<int>& express_ids);

private:
    void accept() override;
    // Blocking problems (the filter cannot be saved), then warnings.
    QStringList problems(bool* blocking = nullptr) const;
    void syncTable(int select);
    int currentLayer() const;
    // Opens the layer window; true when the user accepted it.
    bool editLayer(int index, EditLayer& layer);
    void addLayer();
    void duplicateLayer();
    void removeLayer();
    void moveLayer(int direction);
    void buildNextLayer(size_t index);

    bonsaiviewer::services::FilterDocument document_;
    std::vector<EditLayer> layers_;
    bonsaiviewer::services::IdsBridge& ids_bridge_;
    IfcPathProvider ifc_path_;

    QTableWidget* table_ = nullptr;
    QLabel* problems_label_ = nullptr;
    QDialogButtonBox* buttons_ = nullptr;
    std::vector<QString> built_xml_;
};

} // namespace bonsaiviewer::modules::filters

#endif // IFCINTERFACE_MODULES_FILTERS_CREATIONDIALOG_H
