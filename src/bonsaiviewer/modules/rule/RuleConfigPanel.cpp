// This file was generated with the assistance of an AI coding tool.
/********************************************************************************
 *                                                                              *
 * This file is part of Bonsai.                                                 *
 *                                                                              *
 * Bonsai is free software: you can redistribute it and/or modify               *
 * it under the terms of the GNU General Public License as published by         *
 * the Free Software Foundation, either version 3.0 of the License, or           *
 * (at your option) any later version.                                          *
 *                                                                              *
 * Bonsai is distributed in the hope that it will be useful,                     *
 * but WITHOUT ANY WARRANTY; without the implied warranty of                     *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                 *
 * GNU General Public License for more details.                                 *
 *                                                                              *
 * You should have received a copy of the GNU General Public License            *
 * along with this program. If not, see <http://www.gnu.org/licenses/>.         *
 ********************************************************************************/

#include "RuleConfigPanel.h"

#include "Catalogue.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTextEdit>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

#include <functional>

namespace bonsaiviewer::modules::rule {

namespace {

// Editor for an IDS facet property value: a simple text field by default,
// or — through the "Complex" checkbox — a restriction chosen in a combo
// (Enumeration / Pattern / Bounds / Length), per SPEC_GeometryRule_UI.md 5.
//
// Storage in the instance's value map:
// - simple: a QString;
// - complex: QVariantMap { "restriction": <kind>, "values": QStringList }
//   (Enumeration holds one entry per text field; the other kinds hold the
//   single text at [0]).
class FacetValueEditor : public QWidget {
public:
    using ChangeFn = std::function<void(const QVariant&)>;

    FacetValueEditor(const QVariant& initial, bool editable, ChangeFn on_change,
                     QWidget* parent = nullptr)
        : QWidget(parent), on_change_(std::move(on_change)) {
        auto* outer = new QVBoxLayout(this);
        outer->setContentsMargins(0, 0, 0, 0);
        outer->setSpacing(2);

        auto* head = new QHBoxLayout();
        head->setContentsMargins(0, 0, 0, 0);
        simple_ = new QLineEdit(this);
        simple_->setText(initial.toString());
        simple_->setEnabled(editable);
        connect(simple_, &QLineEdit::textEdited, this, [this](const QString& text) {
            if (on_change_) on_change_(text);
        });
        head->addWidget(simple_, 1);

        complex_box_ = new QCheckBox("Complex", this);
        complex_box_->setEnabled(editable);
        head->addWidget(complex_box_);
        outer->addLayout(head);

        complex_holder_ = new QWidget(this);
        auto* holder_layout = new QVBoxLayout(complex_holder_);
        holder_layout->setContentsMargins(0, 0, 0, 0);
        holder_layout->setSpacing(2);
        outer->addWidget(complex_holder_);

        connect(complex_box_, &QCheckBox::toggled, this, [this, editable](bool checked) {
            buildComplexUi(checked ? restriction_ : QString(), editable);
            if (!on_change_) return;
            if (!checked) on_change_(currentSimpleText());
            else on_change_(currentComplexVariant());
        });

        // Restore the complex state from the stored variant, if any.
        if (initial.userType() == QMetaType::QVariantMap) {
            const QVariantMap map = initial.toMap();
            restriction_ = map.value("restriction").toString();
            values_ = map.value("values").toStringList();
            {
                const QSignalBlocker blocker(complex_box_);
                complex_box_->setChecked(true);
            }
            buildComplexUi(restriction_, editable);
        } else {
            values_.clear();
        }
    }

private:
    QString currentSimpleText() const { return simple_->text(); }

    QVariant currentComplexVariant() const {
        if (restriction_.isEmpty()) return currentSimpleText();
        QVariantMap map;
        map.insert("restriction", restriction_);
        map.insert("values", values_);
        return map;
    }

    // Rebuilds the editor under the checkbox according to `restriction`:
    // a combo to pick the kind, then Enumeration's +/- text field stack or
    // a single text field for Pattern / Bounds / Length.
    void buildComplexUi(const QString& restriction, bool editable) {
        delete combo_;
        delete enum_area_;
        delete single_;
        combo_ = nullptr;
        enum_area_ = nullptr;
        single_ = nullptr;
        if (restriction.isEmpty()) {
            values_.clear();
            return;
        }

        combo_ = new QComboBox(complex_holder_);
        combo_->setEnabled(editable);
        for (const QString& kind : {"Enumeration", "Pattern", "Bounds", "Length"}) {
            combo_->addItem(kind);
        }
        combo_->setCurrentText(restriction);
        complex_holder_->layout()->addWidget(combo_);

        if (restriction == "Enumeration") {
            enum_area_ = new QWidget(complex_holder_);
            auto* layout = new QVBoxLayout(enum_area_);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->setSpacing(2);
            complex_holder_->layout()->addWidget(enum_area_);

            for (int i = 0; i < std::max<int>(1, values_.size()); ++i) {
                addEnumField(i < values_.size() ? values_[i] : QString(), editable);
            }

            auto* buttons = new QHBoxLayout();
            auto* plus = new QPushButton("+", enum_area_);
            auto* minus = new QPushButton("-", enum_area_);
            plus->setEnabled(editable);
            minus->setEnabled(editable);
            plus->setFixedWidth(28);
            minus->setFixedWidth(28);
            connect(plus, &QPushButton::clicked, this, [this, editable]() {
                addEnumField(QString(), editable);
                if (on_change_) on_change_(currentComplexVariant());
            });
            connect(minus, &QPushButton::clicked, this, [this]() {
                if (enum_fields_.size() <= 1) return;
                delete enum_fields_.takeLast();
                syncEnumValues();
                if (on_change_) on_change_(currentComplexVariant());
            });
            buttons->addWidget(plus);
            buttons->addWidget(minus);
            buttons->addStretch(1);
            layout->addLayout(buttons);
        } else {
            single_ = new QLineEdit(complex_holder_);
            single_->setEnabled(editable);
            if (!values_.isEmpty()) single_->setText(values_.front());
            connect(single_, &QLineEdit::textEdited, this, [this](const QString& text) {
                values_ = QStringList{text};
                if (on_change_) on_change_(currentComplexVariant());
            });
            complex_holder_->layout()->addWidget(single_);
        }

        connect(combo_, &QComboBox::currentTextChanged, this,
                [this, editable](const QString& kind) {
            restriction_ = kind;
            values_.clear();
            buildComplexUi(kind, editable);
            if (on_change_) on_change_(currentComplexVariant());
        });
    }

    void addEnumField(const QString& text, bool editable) {
        auto* line = new QLineEdit(enum_area_);
        line->setText(text);
        line->setEnabled(editable);
        static_cast<QVBoxLayout*>(enum_area_->layout())->addWidget(line);
        enum_fields_.append(line);
        connect(line, &QLineEdit::textEdited, this, [this](const QString&) {
            syncEnumValues();
            if (on_change_) on_change_(currentComplexVariant());
        });
    }

    void syncEnumValues() {
        QStringList values;
        for (const QLineEdit* field : enum_fields_) values.append(field->text());
        values_ = values;
    }

    QLineEdit* simple_ = nullptr;
    QCheckBox* complex_box_ = nullptr;
    QWidget* complex_holder_ = nullptr;
    QComboBox* combo_ = nullptr;
    QWidget* enum_area_ = nullptr;
    QLineEdit* single_ = nullptr;
    QList<QLineEdit*> enum_fields_;

    QString restriction_;
    QStringList values_;
    ChangeFn on_change_;
};

} // namespace

RuleConfigPanel::RuleConfigPanel(QWidget* parent) : QWidget(parent) {
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(4, 4, 4, 4);
    outer->setSpacing(4);
    body_layout_ = new QVBoxLayout();
    outer->addLayout(body_layout_);
    outer->addStretch(1);
    showInstance(nullptr);
}

void RuleConfigPanel::showDefinition(const QString& rule_name) {
    const RuleDefinition* definition = resolveRuleDefinition(rule_name);
    if (!definition) {
        rebuild(rule_name, nullptr, nullptr);
        return;
    }
    // Library rules are reference documentation: visible, not editable.
    rebuild(definition->label, definition, nullptr);
}

void RuleConfigPanel::showInstance(QTreeWidgetItem* item) {
    if (!item) {
        rebuild(tr("No selection"), nullptr, nullptr);
        return;
    }
    const RuleDefinition* definition = nullptr;
    const QString key = item->data(0, kRuleKeyRole).toString();
    if (!key.isEmpty()) definition = findRuleDefinition(key);
    // A slot-filled facet keeps the slot prefix in its label; the title
    // shows the bare name.
    QString title = item->text(0);
    const int split = title.indexOf(QStringLiteral(" : "));
    if (split >= 0) title = title.mid(split + 3);
    rebuild(title, definition, item);
}

void RuleConfigPanel::rebuild(const QString& title,
                              const RuleDefinition* definition,
                              QTreeWidgetItem* item) {
    // The whole form lives in one container so a rebuild destroys every
    // editor cleanly instead of leaving stale widgets drawn under the new ones.
    delete form_container_;
    form_container_ = new QWidget(this);
    auto* container_layout = new QVBoxLayout(form_container_);
    container_layout->setContentsMargins(0, 0, 0, 0);
    container_layout->setSpacing(4);

    auto* title_label = new QLabel(title, form_container_);
    title_label->setStyleSheet("font-weight: bold;");
    container_layout->addWidget(title_label);

    auto* form = new QFormLayout();
    form->setLabelAlignment(Qt::AlignRight);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    container_layout->addLayout(form);

    const bool editable = item != nullptr
        && static_cast<RuleNodeType>(item->data(0, Qt::UserRole).toInt()) != RuleNodeType::Slot;

    // A rule dropped into a slot keeps its role label as a text prefix
    // ("source : Overlap Check"); the editor only shows and edits the name.
    const QString slot_label = item ? item->data(0, kSlotNameRole).toString() : QString();
    const QString display_name = item ? item->text(0) : title;
    const QString bare_name = slot_label.isEmpty()
        ? display_name
        : display_name.mid(slot_label.length() + 3);

    // Name: the tree item text for instances, the catalogue name otherwise.
    auto* name_edit = new QLineEdit(form_container_);
    name_edit->setText(editable ? bare_name : title);
    name_edit->setEnabled(editable);
    form->addRow("Name", name_edit);

    // Description: stored on the item (initialized from the catalogue for
    // freshly dropped rules) or read from the catalogue definition.
    auto* description_edit = new QTextEdit(form_container_);
    description_edit->setAcceptRichText(false);
    {
        const QSignalBlocker blocker(description_edit);
        const QString description = definition
            ? definition->description
            : QString();
        description_edit->setPlainText(item ? item->data(0, kDescriptionRole).toString() : description);
    }
    description_edit->setEnabled(editable);
    form->addRow("Description", description_edit);

    QVariantMap values;
    if (item) {
        values = item->data(0, kRuleValuesRole).toMap();
    } else if (definition) {
        for (const auto& property : definition->properties) {
            values.insert(property.name, property.default_value);
        }
    }

    if (definition) {
        for (const auto& property : definition->properties) {
            QWidget* editor = nullptr;
            switch (property.type) {
            case RulePropertyType::Text: {
                // IDS facet value fields carry an optional complex
                // restriction (Enumeration / Pattern / Bounds / Length).
                // Instructions stay plain text — they are documentation.
                const bool is_facet_value = definition->family == "IDS Facet" &&
                                            property.name != "Instructions";
                if (is_facet_value) {
                    editor = new FacetValueEditor(values.value(property.name), editable,
                        [item, key = property.name](const QVariant& value) {
                            QVariantMap map = item->data(0, kRuleValuesRole).toMap();
                            map[key] = value;
                            item->setData(0, kRuleValuesRole, map);
                        });
                    break;
                }
                auto* line = new QLineEdit(form_container_);
                line->setText(values.value(property.name).toString());
                editor = line;
                if (editable) {
                    connect(line, &QLineEdit::textEdited, this,
                            [item, key = property.name](const QString& text) {
                        QVariantMap map = item->data(0, kRuleValuesRole).toMap();
                        map[key] = text;
                        item->setData(0, kRuleValuesRole, map);
                    });
                }
                break;
            }
            case RulePropertyType::Float: {
                auto* spin = new QDoubleSpinBox(form_container_);
                spin->setRange(-1000000.0, 1000000.0);
                spin->setDecimals(3);
                spin->setValue(values.value(property.name).toDouble());
                editor = spin;
                if (editable) {
                    connect(spin, &QDoubleSpinBox::valueChanged, this,
                            [item, key = property.name](double value) {
                        QVariantMap map = item->data(0, kRuleValuesRole).toMap();
                        map[key] = value;
                        item->setData(0, kRuleValuesRole, map);
                    });
                }
                break;
            }
            case RulePropertyType::Int: {
                if (property.nullable) {
                    // An empty field means "absent" (the engine reads None).
                    auto* line = new QLineEdit(form_container_);
                    const QVariant stored = values.value(property.name);
                    line->setText(stored.userType() == QMetaType::QString
                                      ? QString()
                                      : QString::number(stored.toInt()));
                    editor = line;
                    if (editable) {
                        connect(line, &QLineEdit::textEdited, this,
                                [item, key = property.name](const QString& text) {
                            QVariantMap map = item->data(0, kRuleValuesRole).toMap();
                            bool ok = false;
                            const int value = text.toInt(&ok);
                            map[key] = ok ? QVariant(value) : QVariant(QString());
                            item->setData(0, kRuleValuesRole, map);
                        });
                    }
                    break;
                }
                auto* spin = new QSpinBox(form_container_);
                spin->setRange(-1000000, 1000000);
                spin->setValue(values.value(property.name).toInt());
                editor = spin;
                if (editable) {
                    connect(spin, &QSpinBox::valueChanged, this,
                            [item, key = property.name](int value) {
                        QVariantMap map = item->data(0, kRuleValuesRole).toMap();
                        map[key] = value;
                        item->setData(0, kRuleValuesRole, map);
                    });
                }
                break;
            }
            case RulePropertyType::NullableFloat: {
                // An empty field means "absent" (the engine reads None).
                auto* line = new QLineEdit(form_container_);
                const QVariant stored = values.value(property.name);
                line->setText(stored.userType() == QMetaType::QString
                                  ? QString()
                                  : QString::number(stored.toDouble(), 'g', 6));
                editor = line;
                if (editable) {
                    connect(line, &QLineEdit::textEdited, this,
                            [item, key = property.name](const QString& text) {
                        QVariantMap map = item->data(0, kRuleValuesRole).toMap();
                        bool ok = false;
                        const double value = text.toDouble(&ok);
                        map[key] = ok ? QVariant(value) : QVariant(QString());
                        item->setData(0, kRuleValuesRole, map);
                    });
                }
                break;
            }
            case RulePropertyType::Enum: {
                auto* combo = new QComboBox(form_container_);
                combo->addItems(property.enum_values);
                combo->setCurrentText(values.value(property.name).toString());
                combo->setEditable(true); // a value outside the list stays legal
                editor = combo;
                if (editable) {
                    connect(combo, &QComboBox::currentTextChanged, this,
                            [item, key = property.name](const QString& text) {
                        QVariantMap map = item->data(0, kRuleValuesRole).toMap();
                        map[key] = text;
                        item->setData(0, kRuleValuesRole, map);
                    });
                }
                break;
            }
            case RulePropertyType::StringList: {
                // One value per line.
                auto* text_edit = new QTextEdit(form_container_);
                text_edit->setAcceptRichText(false);
                text_edit->setFixedHeight(72);
                text_edit->setPlainText(values.value(property.name).toStringList().join('\n'));
                editor = text_edit;
                if (editable) {
                    connect(text_edit, &QTextEdit::textChanged, this,
                            [item, key = property.name, text_edit]() {
                        QVariantMap map = item->data(0, kRuleValuesRole).toMap();
                        QStringList lines;
                        for (const QString& line : text_edit->toPlainText().split('\n')) {
                            if (!line.trimmed().isEmpty()) lines.append(line.trimmed());
                        }
                        map[key] = lines;
                        item->setData(0, kRuleValuesRole, map);
                    });
                }
                break;
            }
            case RulePropertyType::Direction: {
                auto* widget = new QWidget(form_container_);
                auto* row = new QHBoxLayout(widget);
                row->setContentsMargins(0, 0, 0, 0);
                const QVariantList direction = values.value(property.name).toList();
                for (int axis = 0; axis < 3; ++axis) {
                    auto* spin = new QDoubleSpinBox(widget);
                    spin->setRange(-1.0, 1.0);
                    spin->setDecimals(3);
                    spin->setSingleStep(0.1);
                    spin->setValue(axis < direction.size() ? direction[axis].toDouble() : 0.0);
                    row->addWidget(spin);
                    if (editable) {
                        connect(spin, &QDoubleSpinBox::valueChanged, this,
                                [item, key = property.name, widget](double) {
                            QVariantList updated;
                            const auto spins = widget->findChildren<QDoubleSpinBox*>();
                            for (const QDoubleSpinBox* axis_spin : spins) updated.append(axis_spin->value());
                            QVariantMap map = item->data(0, kRuleValuesRole).toMap();
                            map[key] = updated;
                            item->setData(0, kRuleValuesRole, map);
                        });
                    }
                }
                row->addStretch(1);
                editor = widget;
                break;
            }
            case RulePropertyType::Bool: {
                auto* check = new QCheckBox(form_container_);
                check->setChecked(values.value(property.name).toBool());
                editor = check;
                if (editable) {
                    connect(check, &QCheckBox::toggled, this,
                            [item, key = property.name](bool checked) {
                        QVariantMap map = item->data(0, kRuleValuesRole).toMap();
                        map[key] = checked;
                        item->setData(0, kRuleValuesRole, map);
                    });
                }
                break;
            }
            }
            editor->setEnabled(editable);
            QString label = property.name;
            if (!property.doc.isEmpty()) label += QStringLiteral("  (%1)").arg(property.doc);
            form->addRow(label, editor);
        }
    }

    if (editable) {
        connect(name_edit, &QLineEdit::textEdited, this,
                [item, slot_label](const QString& text) {
            item->setText(0, slot_label.isEmpty() ? text : QString("%1 : %2").arg(slot_label, text));
        });
        connect(description_edit, &QTextEdit::textChanged, this, [item, description_edit]() {
            item->setData(0, kDescriptionRole, description_edit->toPlainText());
        });
    }

    body_layout_->addWidget(form_container_);
}

} // namespace bonsaiviewer::modules::rule
