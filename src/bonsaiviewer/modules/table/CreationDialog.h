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

#ifndef IFCINTERFACE_MODULES_TABLE_CREATIONDIALOG_H
#define IFCINTERFACE_MODULES_TABLE_CREATIONDIALOG_H

#include "Table.h"

#include "../../components/Dialog.h"
#include "../filters/FacetPages.h"

#include <vector>

class QListWidget;

namespace bonsaiviewer::modules::filters { class SelectionEditor; }

namespace bonsaiviewer::modules::table {

// TI07 — the Table Creation Window: name, description, the IDS selection of
// the elements (the same editor as the filter layers) and the grouping
// (TI07 part C: the checked columns are the group keys).
class TableCreationDialog : public components::Dialog {
    Q_OBJECT

public:
    TableCreationDialog(const TableDefinition& definition,
                        bonsaiviewer::services::IdsBridge& bridge,
                        filters::IfcPathProvider ifc_path,
                        QWidget* parent = nullptr);

    TableDefinition definition() const { return definition_; }

signals:
    void isolateRequested(const std::vector<int>& express_ids);

public slots:
    void accept() override;

private:
    TableDefinition definition_;
    filters::SelectionEditor* selection_editor_ = nullptr;
    QListWidget* grouping_list_ = nullptr;
};

} // namespace bonsaiviewer::modules::table

#endif // IFCINTERFACE_MODULES_TABLE_CREATIONDIALOG_H
