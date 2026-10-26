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

#ifndef IFCINTERFACE_MODULES_FILTERS_FACETPAGES_H
#define IFCINTERFACE_MODULES_FILTERS_FACETPAGES_H

#include "../../services/IdsBridge.h"

#include <QStringList>
#include <QWidget>

#include <functional>
#include <vector>

class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;
class QTimer;

namespace bonsaiviewer::modules::filters {

class ValueEditor;

// B06 — the configuration pages of the Filter Creation Window, one per
// IDS facet type. Every IDS field of a page is a ValueEditor (Value by
// default, List / Pattern / Range / Length on demand). The lists come from
// the IFC schema and, when a model is loaded, from the model itself
// (IdsBridge introspection). Without a model the pages stay usable as free
// entry.

// The IFC file of the active model, or empty.
using IfcPathProvider = std::function<QString()>;

class EntityFacetPage : public QWidget {
    Q_OBJECT

public:
    EntityFacetPage(bonsaiviewer::services::IdsBridge& bridge,
                    IfcPathProvider ifc_path,
                    QWidget* parent = nullptr);

    void setFacet(const bonsaiviewer::services::IdsFacet& facet);
    bonsaiviewer::services::IdsFacet facet() const;

signals:
    void changed();

private:
    void loadClasses();
    void requestClassInfo();
    void updateClassStatus();

    bonsaiviewer::services::IdsBridge& bridge_;
    IfcPathProvider ifc_path_;
    bool loading_ = false;
    QString classes_path_; // the model the class list was loaded for
    bool classes_loaded_ = false;
    QStringList known_classes_;
    int info_generation_ = 0;
    QString info_warning_; // from the last class info (non blocking)

    ValueEditor* class_editor_ = nullptr;
    QLabel* class_status_ = nullptr;
    QCheckBox* subtypes_check_ = nullptr;
    QLabel* covered_label_ = nullptr;
    QLabel* count_label_ = nullptr;
    ValueEditor* predefined_editor_ = nullptr;
    QLabel* predefined_note_ = nullptr;
    QTimer* info_timer_ = nullptr;
};

class PropertyFacetPage : public QWidget {
    Q_OBJECT

public:
    // `entity` returns the entity facet of the layer being edited: the
    // model lists only cover the elements that facet selects.
    PropertyFacetPage(bonsaiviewer::services::IdsBridge& bridge,
                      IfcPathProvider ifc_path,
                      std::function<bonsaiviewer::services::IdsFacet()> entity,
                      QWidget* parent = nullptr);

    void setFacet(const bonsaiviewer::services::IdsFacet& facet);
    bonsaiviewer::services::IdsFacet facet() const;

signals:
    void changed();

private:
    void loadProperties();
    void updatePropertySuggestions();
    void loadValues();
    void updateCardinalityHelp();
    void openPicker();

    bonsaiviewer::services::IdsBridge& bridge_;
    IfcPathProvider ifc_path_;
    std::function<bonsaiviewer::services::IdsFacet()> entity_;
    bool loading_ = false;
    int properties_generation_ = 0;
    int values_generation_ = 0;
    bonsaiviewer::services::IdsBridge::PropertyMap psets_;
    bonsaiviewer::services::IdsBridge::SampleMap samples_;

    QPushButton* browse_button_ = nullptr;
    ValueEditor* pset_editor_ = nullptr;
    ValueEditor* property_editor_ = nullptr;
    ValueEditor* value_editor_ = nullptr;
    QComboBox* cardinality_combo_ = nullptr;
    QLabel* cardinality_help_ = nullptr;
    QLabel* status_label_ = nullptr;
    QTimer* values_timer_ = nullptr;
};

// The facet types offered by the "Add facet" menu. The ones not built yet
// are listed disabled so the IDS structure stays visible.
struct FacetTypeEntry {
    QString label;
    bool available;
    bonsaiviewer::services::IdsFacet::Type type;
};
const std::vector<FacetTypeEntry>& facetTypes();

} // namespace bonsaiviewer::modules::filters

#endif // IFCINTERFACE_MODULES_FILTERS_FACETPAGES_H
