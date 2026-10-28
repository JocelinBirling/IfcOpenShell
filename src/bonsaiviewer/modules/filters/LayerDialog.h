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

#ifndef IFCINTERFACE_MODULES_FILTERS_LAYERDIALOG_H
#define IFCINTERFACE_MODULES_FILTERS_LAYERDIALOG_H

#include "FacetPages.h"
#include "SelectionEditor.h"

#include "../../components/Dialog.h"
#include "../../services/FilterStore.h"
#include "../../services/IdsBridge.h"

#include <vector>

class QRadioButton;
class QStackedWidget;
class QVBoxLayout;

namespace bonsaiviewer::modules::filters {

// One filter layer as edited: AND layers have no facets.
struct FilterEditLayer {
    bool is_and = false;
    bonsaiviewer::services::IdsBridge::SelectionFields fields;
    std::vector<bonsaiviewer::services::FilterAction> actions;
};

// B06 — the window of one filter layer, opened from Modify Filter. Three
// steps from top to bottom: ① the type (a selection of its own, or AND:
// the elements of the layers above), ② the selection (facets and live
// preview, SelectionEditor), ③ the actions, run in their order, each one a
// row with its parameters. Cancel drops the changes of this layer only.
class LayerDialog : public components::Dialog {
    Q_OBJECT

public:
    // `index` is the layer's position (0-based): the first layer cannot be AND.
    LayerDialog(int index, const FilterEditLayer& layer,
                bonsaiviewer::services::IdsBridge& ids_bridge,
                IfcPathProvider ifc_path,
                QWidget* parent = nullptr);

    FilterEditLayer layer() const { return layer_; }

    static FilterEditLayer defaultLayer(bool is_and);
    static QString actionLabel(const bonsaiviewer::services::FilterAction& action);
    // Empty when the layer is complete, else what is missing.
    static QString validationError(const FilterEditLayer& layer);

signals:
    // "Isolate in the view" of the preview: EXPRESS ids of the active model.
    void isolateRequested(const std::vector<int>& express_ids);

private:
    void accept() override;
    void updateType();
    void rebuildActions();
    void addAction(bonsaiviewer::services::FilterAction::Type type);
    void moveAction(int index, int direction);
    void removeAction(int index);
    QWidget* actionRow(int index);

    int index_ = 0;
    FilterEditLayer layer_;
    bonsaiviewer::services::IdsBridge& ids_bridge_;
    IfcPathProvider ifc_path_;

    QRadioButton* own_selection_ = nullptr;
    QRadioButton* and_selection_ = nullptr;
    QStackedWidget* selection_stack_ = nullptr;
    SelectionEditor* selection_editor_ = nullptr;
    QWidget* actions_host_ = nullptr;
    QVBoxLayout* actions_layout_ = nullptr;
};

} // namespace bonsaiviewer::modules::filters

#endif // IFCINTERFACE_MODULES_FILTERS_LAYERDIALOG_H
