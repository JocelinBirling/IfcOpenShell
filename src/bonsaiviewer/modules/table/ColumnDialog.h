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

#ifndef IFCINTERFACE_MODULES_TABLE_COLUMNDIALOG_H
#define IFCINTERFACE_MODULES_TABLE_COLUMNDIALOG_H

#include "Table.h"

#include "../../components/Dialog.h"

class QCheckBox;
class QComboBox;
class QLineEdit;

namespace bonsaiviewer::services { class PropertyCatalog; }
namespace bonsaiviewer::modules::property_selection { class PropertyField; }

namespace bonsaiviewer::modules::table {

// TI07 — the Add / Modify Column window: the kind of column, its name,
// for a property, the standard property field (BonsaiViewer26) and
// AutoConvertTextToNumeric, and how the values of a group are combined
// (TI07 part C).
class ColumnDialog : public components::Dialog {
    Q_OBJECT

public:
    ColumnDialog(const TableColumn& column,
                 bonsaiviewer::services::PropertyCatalog& catalog,
                 bool is_new,
                 QWidget* parent = nullptr);

    TableColumn column() const;

public slots:
    void accept() override;

private:
    void updateKind();

    QComboBox* kind_combo_ = nullptr;
    QLineEdit* title_edit_ = nullptr;
    property_selection::PropertyField* property_field_ = nullptr;
    QCheckBox* auto_numeric_check_ = nullptr;
    QComboBox* aggregation_combo_ = nullptr;
    // The group key is set in the Creation window, not here: kept as is.
    bool group_key_ = false;
};

} // namespace bonsaiviewer::modules::table

#endif // IFCINTERFACE_MODULES_TABLE_COLUMNDIALOG_H
