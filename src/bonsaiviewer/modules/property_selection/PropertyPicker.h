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

#include <functional>

class QLineEdit;
class QLabel;
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

    using PropertyMap = bonsaiviewer::services::PropertyCatalog::PropertyMap;
    using SampleMap = bonsaiviewer::services::PropertyCatalog::SampleMap;
    // Reads the properties of a narrower set of elements and hands them over
    // (now or later, from the IDS engine).
    using ScopedLoader = std::function<void(std::function<void(const PropertyMap&, const SampleMap&)>)>;

    // Adds the toggle "Only <label>": the list shows the properties of those
    // elements only (the ones about to be selected, the rows of a table...).
    // Checked from the start.
    void setScopedSource(const QString& label, ScopedLoader loader);

signals:
    void picked(const QString& pset, const QString& property);

private:
    void fill(const PropertyMap& psets, const SampleMap& samples);
    void showScoped(bool scoped);
    void applyFilter(const QString& text);
    void pickCurrent();

    PropertyMap all_psets_;
    SampleMap all_samples_;
    PropertyMap scoped_psets_;
    SampleMap scoped_samples_;
    bool scoped_loaded_ = false;
    ScopedLoader scoped_loader_;
    QString scope_text_;
    QString scoped_label_;
    QLabel* scope_label_ = nullptr;
    QPushButton* scope_button_ = nullptr;

    QLineEdit* search_edit_ = nullptr;
    QTreeWidget* tree_ = nullptr;
    QPushButton* use_button_ = nullptr;
};

} // namespace bonsaiviewer::modules::property_selection

#endif // IFCINTERFACE_MODULES_PROPERTY_SELECTION_PROPERTYPICKER_H
