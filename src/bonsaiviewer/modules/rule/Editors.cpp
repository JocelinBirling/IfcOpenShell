// This file was generated with the assistance of an AI coding tool.
/********************************************************************************
 *                                                                              *
 * This file is part of Bonsai.                                                 *
 *                                                                              *
 * Bonsai is free software: you can redistribute it and/or modify               *
 * it under the terms of the GNU General Public License as published by         *
 * the Free Software Foundation, either version 3.0 of the License, or          *
 * (at your option) any later version.                                          *
 *                                                                              *
 * Bonsai is distributed in the hope that it will be useful,                    *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of               *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                 *
 * GNU General Public License for more details.                                 *
 *                                                                              *
 * You should have received a copy of the GNU General Public License            *
 * along with this program. If not, see <http://www.gnu.org/licenses/>.         *
 *                                                                              *
 ********************************************************************************/

#include "Editors.h"

#include "../../services/IdsEngine.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QDoubleValidator>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QLocale>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QVBoxLayout>

#include <cmath>
#include <memory>

namespace bonsaiviewer::modules::rule {

namespace {

QString s(const std::string& text) { return QString::fromStdString(text); }

void clearForm(QFormLayout* form) {
    while (form->rowCount() > 0) form->removeRow(0);
}

QString tooltipOf(const clash::ParamSpec& p) {
    QString tip = QString("<b>%1</b>").arg(p.name);
    if (!p.doc.isEmpty()) tip += "<br>" + p.doc.toHtmlEscaped();
    if (!p.unit.isEmpty()) tip += QString("<br>Unit: %1").arg(p.unit);
    if (p.nullable) tip += "<br>Empty: no value.";
    return tip;
}

QString numberText(const QJsonValue& value) {
    if (!value.isDouble()) return {};
    const double d = value.toDouble();
    if (d >= 1e300) return QStringLiteral("inf");
    if (d <= -1e300) return QStringLiteral("-inf");
    return QString::number(d, 'g', 12);
}

QJsonValue numberValue(const QString& text, bool nullable) {
    const QString t = text.trimmed();
    if (t.isEmpty()) return nullable ? QJsonValue(QJsonValue::Null) : QJsonValue(0.0);
    if (t.compare("inf", Qt::CaseInsensitive) == 0) return 1e308;
    if (t.compare("-inf", Qt::CaseInsensitive) == 0) return -1e308;
    bool ok = false;
    const double d = QLocale::c().toDouble(QString(t).replace(',', '.'), &ok);
    return ok ? QJsonValue(d) : (nullable ? QJsonValue(QJsonValue::Null) : QJsonValue(0.0));
}

} // namespace

const QStringList& facetTypeNames() {
    static const QStringList names{"Entity", "Attribute", "Property", "Classification", "PartOf", "Material"};
    return names;
}

QString paramText(const it::Param& param) {
    if (param.isNone()) return {};
    if (param.isSimple()) return s(param.simple);
    const it::Restriction& r = param.restriction;
    const auto first = [&r](const char* key) -> QString {
        const auto* list = r.option(key);
        return list && !list->empty() ? s(list->front()) : QString();
    };
    if (const auto* values = r.option("enumeration")) {
        QStringList items;
        for (const auto& v : *values) items << s(v);
        return "enum: " + items.join(" | ");
    }
    if (r.option("pattern")) return "pattern: " + first("pattern");
    const QString minimum = !first("minInclusive").isEmpty() ? first("minInclusive") : first("minExclusive");
    const QString maximum = !first("maxInclusive").isEmpty() ? first("maxInclusive") : first("maxExclusive");
    if (!minimum.isEmpty() || !maximum.isEmpty()) return QString("bounds: %1 .. %2").arg(minimum, maximum);
    if (!first("length").isEmpty()) return QString("length: %1 .. %1").arg(first("length"));
    if (r.option("minLength") || r.option("maxLength")) return QString("length: %1 .. %2").arg(first("minLength"), first("maxLength"));
    return s(r.toString());
}

it::Param paramFromText(const QString& raw) {
    const QString text = raw.trimmed();
    if (text.isEmpty()) return it::Param::none();
    const auto prefixed = [&text](const char* prefix, QString& rest) {
        const QString p = QString::fromLatin1(prefix);
        if (!text.startsWith(p, Qt::CaseInsensitive)) return false;
        rest = text.mid(p.size()).trimmed();
        return true;
    };
    const auto range = [](const QString& rest) {
        const QStringList parts = rest.split("..");
        return std::make_pair(parts.value(0).trimmed(), parts.value(1).trimmed());
    };
    QString rest;
    it::Restriction r;
    if (prefixed("enum:", rest)) {
        for (const QString& item : rest.split('|')) {
            if (!item.trimmed().isEmpty()) r.add("enumeration", item.trimmed().toStdString());
        }
        return it::Param::of(r);
    }
    if (prefixed("pattern:", rest)) {
        r.add("pattern", rest.toStdString());
        return it::Param::of(r);
    }
    if (prefixed("bounds:", rest)) {
        const auto [minimum, maximum] = range(rest);
        if (!minimum.isEmpty()) r.add("minInclusive", minimum.toStdString());
        if (!maximum.isEmpty()) r.add("maxInclusive", maximum.toStdString());
        return it::Param::of(r);
    }
    if (prefixed("length:", rest)) {
        const auto [minimum, maximum] = range(rest);
        if (!minimum.isEmpty()) r.add("minLength", minimum.toStdString());
        if (!maximum.isEmpty()) r.add("maxLength", maximum.toStdString());
        return it::Param::of(r);
    }
    return it::Param::text(text.toStdString());
}

QString facetSummary(const it::Facet& facet) {
    const auto p = [](const it::Param& param) { return paramText(param); };
    switch (facet.type) {
    case it::FacetType::Entity: {
        QString text = "Entity " + p(facet.name);
        if (!facet.predefinedType.isNone()) text += " / " + p(facet.predefinedType);
        return text;
    }
    case it::FacetType::Attribute:
        return "Attribute " + p(facet.name) + (facet.value.isNone() ? QString() : " = " + p(facet.value));
    case it::FacetType::Property:
        return "Property " + p(facet.propertySet) + "." + p(facet.baseName) +
               (facet.value.isNone() ? QString() : " = " + p(facet.value));
    case it::FacetType::Classification:
        return "Classification " + p(facet.system) + (facet.value.isNone() ? QString() : " " + p(facet.value));
    case it::FacetType::PartOf:
        return "Part of " + p(facet.name) + (facet.relation ? " (" + s(*facet.relation) + ")" : QString());
    case it::FacetType::Material:
        return "Material " + p(facet.value);
    }
    return {};
}

// --- ParamForm ----------------------------------------------------------------------

ParamForm::ParamForm(QWidget* parent) : QWidget(parent) {
    form_ = new QFormLayout(this);
    form_->setContentsMargins(0, 0, 0, 0);
}

void ParamForm::setRule(const clash::RuleSpec* spec, QJsonObject* params) {
    clearForm(form_);
    if (spec == nullptr || params == nullptr) return;
    for (const clash::ParamSpec& p : spec->params) {
        const QString name = p.name;
        const QJsonValue value = params->contains(name) ? params->value(name) : p.default_value;
        QWidget* editor = nullptr;
        const auto store = [this, params, name](const QJsonValue& v) {
            (*params)[name] = v;
            emit edited();
        };
        if (p.kind == QLatin1String("bool")) {
            auto* check = new QCheckBox(this);
            check->setChecked(value.toBool());
            connect(check, &QCheckBox::toggled, this, [store](bool on) { store(on); });
            editor = check;
        } else if (p.kind == QLatin1String("enum")) {
            auto* combo = new QComboBox(this);
            QStringList values = p.values;
            if (p.nullable && !values.contains(QString())) values.prepend(QString());
            combo->addItems(values);
            combo->setCurrentText(value.isNull() ? QString() : value.toString());
            connect(combo, &QComboBox::currentTextChanged, this, [store, nullable = p.nullable](const QString& text) {
                store(text.isEmpty() && nullable ? QJsonValue(QJsonValue::Null) : QJsonValue(text));
            });
            editor = combo;
        } else if (p.kind == QLatin1String("int") && !p.nullable) {
            auto* spin = new QSpinBox(this);
            spin->setRange(p.min ? int(*p.min) : -1000000, p.max ? int(*p.max) : 1000000);
            spin->setValue(value.toInt());
            connect(spin, &QSpinBox::valueChanged, this, [store](int v) { store(v); });
            editor = spin;
        } else if (p.kind == QLatin1String("float") || p.kind == QLatin1String("int")) {
            auto* line = new QLineEdit(numberText(value), this);
            line->setPlaceholderText(p.nullable ? QStringLiteral("(none)") : QString());
            connect(line, &QLineEdit::editingFinished, this, [line, store, nullable = p.nullable, integer = p.kind == QLatin1String("int")]() {
                QJsonValue v = numberValue(line->text(), nullable);
                if (integer && v.isDouble()) v = static_cast<int>(std::lround(v.toDouble()));
                store(v);
            });
            editor = line;
        } else if (p.kind == QLatin1String("direction")) {
            auto* row = new QWidget(this);
            auto* layout = new QHBoxLayout(row);
            layout->setContentsMargins(0, 0, 0, 0);
            const QJsonArray xyz = value.toArray();
            auto spins = std::make_shared<std::vector<QDoubleSpinBox*>>();
            for (int i = 0; i < 3; ++i) {
                auto* spin = new QDoubleSpinBox(row);
                spin->setRange(-1e6, 1e6);
                spin->setDecimals(4);
                spin->setValue(xyz.at(i).toDouble());
                layout->addWidget(spin);
                spins->push_back(spin);
            }
            for (QDoubleSpinBox* spin : *spins) {
                connect(spin, &QDoubleSpinBox::valueChanged, this, [spins, store](double) {
                    store(QJsonArray{(*spins)[0]->value(), (*spins)[1]->value(), (*spins)[2]->value()});
                });
            }
            editor = row;
        } else if (p.kind == QLatin1String("string_list")) {
            auto* text = new QPlainTextEdit(this);
            QStringList lines;
            for (const QJsonValue& v : value.toArray()) lines << v.toString();
            text->setPlainText(lines.join('\n'));
            text->setMaximumHeight(80);
            connect(text, &QPlainTextEdit::textChanged, this, [text, store]() {
                QJsonArray array;
                for (const QString& line : text->toPlainText().split('\n', Qt::SkipEmptyParts)) array.append(line.trimmed());
                store(array);
            });
            editor = text;
        } else {
            // string / object: a JSON value or plain text.
            QString shown;
            if (value.isString()) shown = value.toString();
            else if (!value.isNull() && !value.isUndefined()) {
                shown = QString::fromUtf8(QJsonDocument(QJsonArray{value}).toJson(QJsonDocument::Compact));
                shown = shown.mid(1, shown.size() - 2);
            }
            auto* line = new QLineEdit(shown, this);
            line->setPlaceholderText(p.kind == QLatin1String("object") ? QStringLiteral("text or JSON") : QString());
            connect(line, &QLineEdit::editingFinished, this, [line, store, object = p.kind == QLatin1String("object")]() {
                const QString text = line->text().trimmed();
                if (text.isEmpty()) {
                    store(QJsonValue::Null);
                    return;
                }
                if (object) {
                    const QJsonDocument doc = QJsonDocument::fromJson(("[" + text + "]").toUtf8());
                    if (doc.isArray() && doc.array().size() == 1) {
                        store(doc.array().at(0));
                        return;
                    }
                }
                store(text);
            });
            editor = line;
        }
        editor->setToolTip(tooltipOf(p));
        auto* label = new QLabel(p.unit.isEmpty() ? p.name : QString("%1 (%2)").arg(p.name, p.unit), this);
        label->setToolTip(tooltipOf(p));
        form_->addRow(label, editor);
    }
}

// --- FacetEditor ------------------------------------------------------------------------

FacetEditor::FacetEditor(QWidget* parent) : QWidget(parent) {
    form_ = new QFormLayout(this);
    form_->setContentsMargins(0, 0, 0, 0);
}

void FacetEditor::setFacet(it::Facet* facet) {
    clearForm(form_);
    if (facet == nullptr) return;
    const QString help = "Plain text: the value. Or <i>enum: a | b</i>, <i>pattern: regex</i>, "
                         "<i>bounds: min .. max</i>, <i>length: min .. max</i>. Empty: no value.";
    const auto addParam = [this, &help](const QString& label, it::Param* param) {
        auto* line = new QLineEdit(paramText(*param), this);
        line->setToolTip(QString("<b>%1</b><br>%2").arg(label, help));
        connect(line, &QLineEdit::editingFinished, this, [this, line, param]() {
            *param = paramFromText(line->text());
            emit edited();
        });
        form_->addRow(label, line);
    };
    const auto addOptional = [this](const QString& label, std::optional<std::string>* value) {
        auto* line = new QLineEdit(value->has_value() ? s(**value) : QString(), this);
        connect(line, &QLineEdit::editingFinished, this, [this, line, value]() {
            const QString text = line->text().trimmed();
            *value = text.isEmpty() ? std::nullopt : std::optional<std::string>(text.toStdString());
            emit edited();
        });
        form_->addRow(label, line);
    };
    const auto addCardinality = [this, facet]() {
        auto* combo = new QComboBox(this);
        combo->addItems({"required", "optional", "prohibited"});
        combo->setCurrentText(s(it::cardinalityName(facet->cardinality)));
        combo->setToolTip("<b>Cardinality</b><br>required: the element must match; prohibited: it must not.");
        connect(combo, &QComboBox::currentTextChanged, this, [this, facet](const QString& text) {
            facet->cardinality = it::cardinalityFromName(text.toStdString()).value_or(it::Cardinality::Required);
            emit edited();
        });
        form_->addRow("Cardinality", combo);
    };
    form_->addRow(new QLabel(QString("<b>%1 facet</b>").arg(s(it::facetTypeName(facet->type))), this));
    switch (facet->type) {
    case it::FacetType::Entity:
        addParam("Name", &facet->name);
        addParam("Predefined type", &facet->predefinedType);
        break;
    case it::FacetType::Attribute:
        addParam("Name", &facet->name);
        addParam("Value", &facet->value);
        addCardinality();
        break;
    case it::FacetType::Property:
        addParam("Property set", &facet->propertySet);
        addParam("Property", &facet->baseName);
        addParam("Value", &facet->value);
        addOptional("Data type", &facet->dataType);
        addCardinality();
        break;
    case it::FacetType::Classification:
        addParam("System", &facet->system);
        addParam("Value", &facet->value);
        addCardinality();
        break;
    case it::FacetType::PartOf:
        addParam("Name", &facet->name);
        addParam("Predefined type", &facet->predefinedType);
        addOptional("Relation", &facet->relation);
        addCardinality();
        break;
    case it::FacetType::Material:
        addParam("Value", &facet->value);
        addCardinality();
        break;
    }
}

// --- FacetListEditor ----------------------------------------------------------------------

FacetListEditor::FacetListEditor(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    list_ = new QListWidget(this);
    list_->setMaximumHeight(110);
    list_->setToolTip("The facets select the elements: each facet keeps the elements of the previous ones that match it.");
    auto* buttons = new QHBoxLayout();
    auto* add = new QPushButton("Add Facet", this);
    auto* remove = new QPushButton("Remove", this);
    add->setToolTip("<b>Add Facet</b><br>An IDS facet that the elements must match.");
    remove->setToolTip("<b>Remove</b><br>Remove the selected facet.");
    auto* menu = new QMenu(add);
    for (const QString& name : facetTypeNames()) {
        menu->addAction(name, this, [this, name]() {
            if (!facets_) return;
            const std::string key = name == QLatin1String("PartOf") ? std::string("partOf") : name.toLower().toStdString();
            auto type = it::facetTypeFromName(key);
            facets_->push_back(it::Facet::make(type.value_or(it::FacetType::Entity)));
            refreshList(static_cast<int>(facets_->size()) - 1);
            emit edited();
        });
    }
    add->setMenu(menu);
    buttons->addWidget(add);
    buttons->addWidget(remove);
    buttons->addStretch(1);
    editor_ = new FacetEditor(this);
    layout->addWidget(list_);
    layout->addLayout(buttons);
    layout->addWidget(editor_);
    connect(remove, &QPushButton::clicked, this, [this]() {
        const int row = list_->currentRow();
        if (!facets_ || row < 0 || row >= static_cast<int>(facets_->size())) return;
        facets_->erase(facets_->begin() + row);
        refreshList(std::min(row, static_cast<int>(facets_->size()) - 1));
        emit edited();
    });
    connect(list_, &QListWidget::currentRowChanged, this, [this](int row) {
        editor_->setFacet(facets_ && row >= 0 && row < static_cast<int>(facets_->size()) ? &(*facets_)[row] : nullptr);
    });
    connect(editor_, &FacetEditor::edited, this, [this]() {
        const int row = list_->currentRow();
        if (facets_ && row >= 0 && row < static_cast<int>(facets_->size())) list_->item(row)->setText(facetSummary((*facets_)[row]));
        emit edited();
    });
}

void FacetListEditor::setFacets(std::vector<it::Facet>* facets) {
    facets_ = facets;
    refreshList(facets && !facets->empty() ? 0 : -1);
}

void FacetListEditor::refreshList(int current) {
    const QSignalBlocker blocker(list_);
    list_->clear();
    if (facets_) {
        for (const it::Facet& facet : *facets_) list_->addItem(facetSummary(facet));
    }
    list_->setCurrentRow(current);
    editor_->setFacet(facets_ && current >= 0 && current < static_cast<int>(facets_->size()) ? &(*facets_)[current] : nullptr);
}

bonsaiviewer::services::IdsBridge::SelectionFields idsFields(const std::vector<it::Facet>& facets) {
    bonsaiviewer::services::IdsBridge::SelectionFields fields;
    for (const auto& facet : facets) {
        try {
            fields.facets.push_back(bonsaiviewer::services::ids_engine::fromFacet(facet));
        } catch (const std::exception&) {
        }
    }
    return fields;
}

std::vector<it::Facet> engineFacets(const bonsaiviewer::services::IdsBridge::SelectionFields& fields,
                                    ifcopenshell::file* file, QString* error) {
    std::vector<it::Facet> facets;
    for (size_t i = 0; i < fields.facets.size(); ++i) {
        try {
            facets.push_back(bonsaiviewer::services::ids_engine::toFacet(fields.facets[i], static_cast<int>(i), file));
        } catch (const std::exception& e) {
            if (error) {
                *error = QString("facet %1 (%2): %3")
                             .arg(i + 1)
                             .arg(bonsaiviewer::services::IdsFacet::typeLabel(fields.facets[i].type),
                                  QString::fromStdString(e.what()));
            }
            return {};
        }
    }
    return facets;
}

} // namespace bonsaiviewer::modules::rule
