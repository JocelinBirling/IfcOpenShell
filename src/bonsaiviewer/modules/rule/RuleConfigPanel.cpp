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
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QSignalBlocker>
#include <QTextEdit>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

namespace bonsaiviewer::modules::rule {

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
    const RuleDefinition* definition = findRuleDefinition(rule_name);
    if (!definition) {
        rebuild(rule_name, nullptr, nullptr);
        return;
    }
    // Library rules are reference documentation: visible, not editable.
    rebuild(definition->name, definition, nullptr);
}

void RuleConfigPanel::showInstance(QTreeWidgetItem* item) {
    if (!item) {
        rebuild(tr("No selection"), nullptr, nullptr);
        return;
    }
    const RuleDefinition* definition = nullptr;
    const QString key = item->data(0, kRuleKeyRole).toString();
    if (!key.isEmpty()) definition = findRuleDefinition(key);
    rebuild(item->text(0), definition, item);
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
            form->addRow(property.name, editor);
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
