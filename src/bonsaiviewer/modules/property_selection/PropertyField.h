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

#ifndef IFCINTERFACE_MODULES_PROPERTY_SELECTION_PROPERTYFIELD_H
#define IFCINTERFACE_MODULES_PROPERTY_SELECTION_PROPERTYFIELD_H

#include <QWidget>

class QCompleter;
class QLineEdit;
class QPushButton;
class QStandardItemModel;

namespace bonsaiviewer::services { class PropertyCatalog; }

namespace bonsaiviewer::modules::property_selection {

// BonsaiViewer26 — the standard property field: a Pset input and a Property
// input side by side, plus "Browse…". Both inputs complete on the property
// sets of all the loaded models (PropertyCatalog), matching anywhere in the
// name. Choosing a property fills its property set too; a filled Pset input
// restricts the property proposals to that set. Free typing stays allowed.
class PropertyField : public QWidget {
    Q_OBJECT

public:
    explicit PropertyField(bonsaiviewer::services::PropertyCatalog& catalog, QWidget* parent = nullptr);

    QString propertySet() const;
    QString propertyName() const;
    void setValue(const QString& pset, const QString& property);

signals:
    void changed();

private:
    void rebuildPsetModel();
    void rebuildPropertyModel();
    void browse();

    bonsaiviewer::services::PropertyCatalog& catalog_;
    QLineEdit* pset_edit_ = nullptr;
    QLineEdit* property_edit_ = nullptr;
    QPushButton* browse_button_ = nullptr;
    QStandardItemModel* pset_model_ = nullptr;
    QStandardItemModel* property_model_ = nullptr;
    QCompleter* pset_completer_ = nullptr;
    QCompleter* property_completer_ = nullptr;
};

} // namespace bonsaiviewer::modules::property_selection

#endif // IFCINTERFACE_MODULES_PROPERTY_SELECTION_PROPERTYFIELD_H
