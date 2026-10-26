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

#ifndef IFCINTERFACE_MODULES_FILTERS_VALUEEDITOR_H
#define IFCINTERFACE_MODULES_FILTERS_VALUEEDITOR_H

#include "../../services/IdsBridge.h"

#include <QList>
#include <QPair>
#include <QWidget>

class QButtonGroup;
class QCheckBox;
class QComboBox;
class QCompleter;
class QLabel;
class QLineEdit;
class QListWidget;
class QStackedWidget;
class QStringListModel;
class QToolButton;

namespace bonsaiviewer::modules::filters {

// B06 — the standard editor of an IDS field. Every facet field (class,
// PredefinedType, property set, property, value) uses it, so all fields
// offer the same methods, in the same place:
//
//   Value   a simple value (the default); empty = no constraint
//   List    xs:enumeration — checkable values, pre-filled from the model
//           (with element counts) or the schema, free additions, paste
//   Pattern xs:pattern — "starts with / contains / ends with" presets or a
//           free XSD expression, with a test field
//   Range   min / max bounds, each included or not (numbers)
//   Length  number of characters: exact, minimum and/or maximum
//
// A row of buttons switches the method; what was typed is carried over
// (a value becomes the first list entry, the pattern text...). Each field
// only shows the methods that make sense for it.
class ValueEditor : public QWidget {
    Q_OBJECT

public:
    enum Method {
        AllowSimple = 0x02,
        AllowEnumeration = 0x04,
        AllowPattern = 0x08,
        AllowRange = 0x10,
        AllowLength = 0x20,
        // Names (class, property set, property, PredefinedType).
        AllowText = AllowSimple | AllowEnumeration | AllowPattern | AllowLength,
        AllowAll = AllowText | AllowRange,
    };

    explicit ValueEditor(int allowed_methods, QWidget* parent = nullptr);

    void setValue(const bonsaiviewer::services::IdsValue& value);
    // An empty simple value reads back as Kind::Any (no constraint).
    bonsaiviewer::services::IdsValue value() const;

    // Values found in the model, with their element counts; `missing` is
    // the number of elements without any value (-1 = unknown). They are
    // listed in the List method and offered in the Value method.
    void setModelValues(const QList<QPair<QString, int>>& values, int missing = -1);
    // Short known lists (e.g. the PredefinedType enumeration): listed too.
    void setSuggestions(const QStringList& values);
    // Long known lists (e.g. the schema classes): completion only.
    void setCompletions(const QStringList& values);
    void clearSuggestions();
    // The XSD base of a range: double or integer.
    void setRangeBase(const QString& base);

    // The "Load from model" button, wired by the owner.
    void setReloadVisible(bool visible);
    void setPlaceholder(const QString& text);

signals:
    void changed();
    void reloadRequested();

private:
    using Kind = bonsaiviewer::services::IdsValue::Kind;

    void buildSimplePage();
    void buildEnumerationPage();
    void buildPatternPage();
    void buildRangePage();
    void buildLengthPage();
    void switchTo(Kind kind);
    void showPage(Kind kind);
    Kind currentKind() const;
    QString typedText(Kind kind) const;
    void addEnumerationValue(const QString& value, bool checked);
    void rebuildEnumerationList(const QStringList& checked);
    QStringList checkedEnumerationValues() const;
    void refreshSimpleItems();
    void updatePatternPreview();
    QString currentPattern() const;
    void emitChanged();

    int allowed_methods_ = 0;
    bool loading_ = false;
    Kind kind_ = Kind::Simple;
    QButtonGroup* method_buttons_ = nullptr;
    QToolButton* reload_button_ = nullptr;
    QStackedWidget* pages_ = nullptr;
    QStringListModel* completion_model_ = nullptr;

    // Value
    QComboBox* simple_combo_ = nullptr;

    // List
    QListWidget* enumeration_list_ = nullptr;
    QLineEdit* enumeration_add_edit_ = nullptr;
    QLabel* enumeration_info_ = nullptr;

    // Pattern
    QComboBox* pattern_preset_ = nullptr;
    QLineEdit* pattern_text_ = nullptr;
    QLabel* pattern_preview_ = nullptr;
    QLineEdit* pattern_test_ = nullptr;
    QLabel* pattern_test_result_ = nullptr;

    // Range
    QLineEdit* range_min_ = nullptr;
    QCheckBox* range_min_inclusive_ = nullptr;
    QLineEdit* range_max_ = nullptr;
    QCheckBox* range_max_inclusive_ = nullptr;
    QComboBox* range_base_ = nullptr;

    // Length
    QLineEdit* length_min_ = nullptr;
    QLineEdit* length_max_ = nullptr;

    QList<QPair<QString, int>> model_values_;
    QStringList suggestions_;
    QStringList completions_;
    int missing_ = -1;
};

} // namespace bonsaiviewer::modules::filters

#endif // IFCINTERFACE_MODULES_FILTERS_VALUEEDITOR_H
