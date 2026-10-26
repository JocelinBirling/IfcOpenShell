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

#ifndef IFCINTERFACE_MODULES_PROPERTY_SELECTION_PROPERTYPICKER_H
#define IFCINTERFACE_MODULES_PROPERTY_SELECTION_PROPERTYPICKER_H

#include "../../services/PropertyCatalog.h"

#include <QDialog>

class QLineEdit;
class QPushButton;
class QTreeWidget;

namespace bonsaiviewer::modules::property_selection {

// BonsaiViewer26 — the property picker ("Browse…" of the property field):
// the property sets laid out like the Properties panel (one group per set,
// a property per row with its element count and a few of its values). A
// search field filters sets, properties and values; double-clicking a
// property (or "Use") picks it.
class PropertyPicker : public QDialog {
    Q_OBJECT

public:
    PropertyPicker(const bonsaiviewer::services::PropertyCatalog::PropertyMap& psets,
                   const bonsaiviewer::services::PropertyCatalog::SampleMap& samples,
                   const QString& scope,
                   QWidget* parent = nullptr);

signals:
    void picked(const QString& pset, const QString& property);

private:
    void applyFilter(const QString& text);
    void pickCurrent();

    QLineEdit* search_edit_ = nullptr;
    QTreeWidget* tree_ = nullptr;
    QPushButton* use_button_ = nullptr;
};

} // namespace bonsaiviewer::modules::property_selection

#endif // IFCINTERFACE_MODULES_PROPERTY_SELECTION_PROPERTYPICKER_H
