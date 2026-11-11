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

// PORT of ifcclash_plus/serialization.py — see RuleJson.h.

#include "RuleJson.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QUuid>

#include <cstring>
#include <set>

namespace bonsaiviewer::services::clash::json {

namespace it = bonsaiviewer::ports::ifctester;

namespace {

QString s(const std::string& text) { return QString::fromStdString(text); }
std::string toStd(const QString& text) { return text.toStdString(); }

QJsonValue optionalString(const std::optional<std::string>& value) {
    return value ? QJsonValue(s(*value)) : QJsonValue(QJsonValue::Null);
}

std::optional<std::string> optionalString(const QJsonValue& value) {
    if (value.isNull() || value.isUndefined()) return std::nullopt;
    if (value.isString()) return toStd(value.toString());
    if (value.isDouble()) return toStd(QString::number(value.toDouble(), 'g', 15));
    if (value.isBool()) return value.toBool() ? std::string("True") : std::string("False");
    return std::nullopt;
}

// --- facet parameters (_serialize_facet_param) -------------------------------

QJsonValue paramToJson(const it::Param& param) {
    if (param.isNone()) return QJsonValue::Null;
    if (param.isSimple()) return s(param.simple);
    const it::Restriction& r = param.restriction;
    const auto values = [&r](const char* key) {
        QJsonArray array;
        if (const auto* list = r.option(key)) {
            for (const std::string& v : *list) array.append(s(v));
        }
        return array;
    };
    const auto first = [&r](const char* key) -> QString {
        const auto* list = r.option(key);
        return list && !list->empty() ? s(list->front()) : QString();
    };
    if (r.option("enumeration")) return QJsonObject{{"restriction", "Enumeration"}, {"values", values("enumeration")}};
    if (r.option("pattern")) return QJsonObject{{"restriction", "Pattern"}, {"values", values("pattern")}};
    const QString minimum = !first("minInclusive").isEmpty() ? first("minInclusive") : first("minExclusive");
    const QString maximum = !first("maxInclusive").isEmpty() ? first("maxInclusive") : first("maxExclusive");
    if (!minimum.isEmpty() || !maximum.isEmpty()) {
        return QJsonObject{{"restriction", "Bounds"}, {"values", QJsonArray{minimum, maximum}}};
    }
    if (!first("length").isEmpty()) {
        return QJsonObject{{"restriction", "Length"}, {"values", QJsonArray{first("length"), first("length")}}};
    }
    if (r.option("minLength") || r.option("maxLength")) {
        return QJsonObject{{"restriction", "Length"}, {"values", QJsonArray{first("minLength"), first("maxLength")}}};
    }
    QJsonObject raw;
    for (const auto& [key, list] : r.options) {
        QJsonArray array;
        for (const std::string& v : list) array.append(s(v));
        raw[s(key)] = array;
    }
    return QJsonObject{{"restriction", "Raw"}, {"values", raw}};
}

it::Param paramFromJson(const QJsonValue& value) {
    if (value.isNull() || value.isUndefined()) return it::Param::none();
    if (!value.isObject()) {
        const auto text = optionalString(value);
        return text ? it::Param::text(*text) : it::Param::none();
    }
    const QJsonObject block = value.toObject();
    if (!block.contains("restriction")) return it::Param::none();
    const QString kind = block.value("restriction").toString();
    it::Restriction r;
    if (kind == QLatin1String("Raw")) {
        const QJsonObject raw = block.value("values").toObject();
        for (auto i = raw.begin(); i != raw.end(); ++i) {
            const QJsonValue v = i.value();
            if (v.isArray()) {
                for (const QJsonValue& item : v.toArray()) r.add(toStd(i.key()), optionalString(item).value_or(""));
            } else {
                r.add(toStd(i.key()), optionalString(v).value_or(""));
            }
        }
        return it::Param::of(r);
    }
    std::vector<std::string> values;
    for (const QJsonValue& v : block.value("values").toArray()) values.push_back(optionalString(v).value_or(""));
    if (kind == QLatin1String("Enumeration")) {
        for (const auto& v : values) r.add("enumeration", v);
    } else if (kind == QLatin1String("Pattern")) {
        if (values.empty()) values.push_back("");
        for (const auto& v : values) r.add("pattern", v);
    } else if (kind == QLatin1String("Bounds")) {
        if (values.size() > 0 && !values[0].empty()) r.add("minInclusive", values[0]);
        if (values.size() > 1 && !values[1].empty()) r.add("maxInclusive", values[1]);
    } else if (kind == QLatin1String("Length")) {
        if (values.size() > 0 && !values[0].empty()) r.add("minLength", values[0]);
        if (values.size() > 1 && !values[1].empty()) r.add("maxLength", values[1]);
    }
    return it::Param::of(r);
}

QJsonObject viewerToJson(const ViewerMeta& meta) {
    QJsonObject viewer;
    if (!meta.label.isEmpty()) viewer["label"] = meta.label;
    if (!meta.description.isEmpty()) viewer["description"] = meta.description;
    return viewer;
}

ViewerMeta viewerFromJson(const QJsonObject& node) {
    const QJsonObject viewer = node.value("viewer").toObject();
    return {viewer.value("label").toString(), viewer.value("description").toString()};
}

QJsonValue groupingToJson(const Grouping& grouping) {
    switch (grouping.kind) {
    case Grouping::Kind::None: return QJsonValue::Null;
    case Grouping::Kind::Name: return grouping.name;
    case Grouping::Kind::Facet: return grouping.facet ? QJsonValue(toJson(*grouping.facet)) : QJsonValue(QJsonValue::Null);
    case Grouping::Kind::SelectFacet: return toJson(grouping.select);
    }
    return QJsonValue::Null;
}

Grouping groupingFromJson(const QJsonValue& value) {
    Grouping grouping;
    if (value.isString()) {
        grouping.kind = Grouping::Kind::Name;
        grouping.name = value.toString();
    } else if (value.isObject()) {
        const QJsonObject object = value.toObject();
        if (object.value("type").toString() == QLatin1String("SelectFacet")) {
            grouping.kind = Grouping::Kind::SelectFacet;
            grouping.select = selectFromJson(object);
        } else {
            grouping.kind = Grouping::Kind::Facet;
            grouping.facet = facetFromJson(object);
        }
    }
    return grouping;
}

QJsonValue mustToJson(const std::optional<AbsOrRelCheck>& must) {
    if (!must) return QJsonValue::Null;
    return QJsonObject{{"class", "AbsoluteOrRelativeChecking"},
                       {"check_type", must->check_type},
                       {"quantity_pset", must->quantity_pset.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(must->quantity_pset)},
                       {"quantity_prop", must->quantity_prop.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(must->quantity_prop)},
                       {"operation", must->operation.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(must->operation)}};
}

std::optional<AbsOrRelCheck> mustFromJson(const QJsonValue& value) {
    if (!value.isObject()) return std::nullopt;
    const QJsonObject data = value.toObject();
    const QString cls = data.value("class").toString();
    if (cls != QLatin1String("AbsoluteOrRelativeChecking") && cls != QLatin1String("AbsoluteChecking") &&
        cls != QLatin1String("RelativeChecking")) {
        throw FormatError("unknown check class: " + toStd(cls));
    }
    AbsOrRelCheck check;
    check.check_type = data.value("check_type").toString("Absolute_Number");
    check.quantity_pset = data.value("quantity_pset").toString();
    check.quantity_prop = data.value("quantity_prop").toString();
    check.operation = data.value("operation").toString();
    // The old format (deserialize_abs_or_rel_check): the comparison split
    // into focus / operation / aim or source_operation / operation / target_operation.
    if (!check.operation.isEmpty() && !check.operation.contains("SOURCE") && !check.operation.contains("TARGET")) {
        if (check.check_type.startsWith("Relative")) {
            check.operation = "SOURCE" + data.value("source_operation").toString() + check.operation + "TARGET" +
                              data.value("target_operation").toString();
        } else {
            const QString measured = data.value("focus").toString("source") == QLatin1String("source") ? "TARGET" : "SOURCE";
            check.operation = measured + check.operation + data.value("aim").toVariant().toString();
        }
    }
    return check;
}

// The keys a rule object carries besides its type-specific parameters.
const std::set<QString>& commonRuleKeys() {
    static const std::set<QString> keys{"type", "id", "state", "select_grouping", "select_criticity", "select_actor",
                                        "select_source", "select_target", "select_context", "select_exception",
                                        "abs_or_rel_check", "viewer"};
    return keys;
}

} // namespace

QString newId() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

// --- facets --------------------------------------------------------------------

QJsonObject toJson(const it::Facet& facet) {
    const QJsonValue instructions = optionalString(facet.instructions);
    const QString cardinality = s(it::cardinalityName(facet.cardinality));
    switch (facet.type) {
    case it::FacetType::Entity:
        return {{"type", "Entity"}, {"name", paramToJson(facet.name)},
                {"predefined_type", paramToJson(facet.predefinedType)}, {"instructions", instructions}};
    case it::FacetType::Property:
        return {{"type", "Property"}, {"property_set", paramToJson(facet.propertySet)},
                {"base_name", paramToJson(facet.baseName)}, {"value", paramToJson(facet.value)},
                {"data_type", optionalString(facet.dataType)}, {"cardinality", cardinality},
                {"instructions", instructions}};
    case it::FacetType::Attribute:
        return {{"type", "Attribute"}, {"name", paramToJson(facet.name)}, {"value", paramToJson(facet.value)},
                {"cardinality", cardinality}, {"instructions", instructions}};
    case it::FacetType::Classification:
        return {{"type", "Classification"}, {"value", paramToJson(facet.value)}, {"system", paramToJson(facet.system)},
                {"cardinality", cardinality}, {"instructions", instructions}};
    case it::FacetType::PartOf:
        return {{"type", "PartOf"}, {"name", paramToJson(facet.name)},
                {"predefined_type", paramToJson(facet.predefinedType)}, {"relation", optionalString(facet.relation)},
                {"cardinality", cardinality}, {"instructions", instructions}};
    case it::FacetType::Material:
        return {{"type", "Material"}, {"value", paramToJson(facet.value)}, {"cardinality", cardinality},
                {"instructions", instructions}};
    }
    throw FormatError("unsupported facet type");
}

it::Facet facetFromJson(const QJsonObject& data) {
    const QString type = data.value("type").toString();
    const auto cardinality = [&data]() {
        return it::cardinalityFromName(toStd(data.value("cardinality").toString("required"))).value_or(it::Cardinality::Required);
    };
    const auto param_or = [&data](const char* key, const char* fallback) {
        return data.contains(key) ? paramFromJson(data.value(key)) : it::Param::text(fallback);
    };
    it::Facet facet;
    if (type == QLatin1String("Entity")) {
        facet = it::Facet::make(it::FacetType::Entity);
        facet.name = param_or("name", "IFCWALL");
        facet.predefinedType = paramFromJson(data.value("predefined_type"));
    } else if (type == QLatin1String("Property")) {
        facet = it::Facet::make(it::FacetType::Property);
        facet.propertySet = param_or("property_set", "Property_Set");
        facet.baseName = param_or("base_name", "PropertyName");
        facet.value = paramFromJson(data.value("value"));
        facet.dataType = optionalString(data.value("data_type"));
        facet.cardinality = cardinality();
    } else if (type == QLatin1String("Attribute")) {
        facet = it::Facet::make(it::FacetType::Attribute);
        facet.name = param_or("name", "Name");
        facet.value = paramFromJson(data.value("value"));
        facet.cardinality = cardinality();
    } else if (type == QLatin1String("Classification")) {
        facet = it::Facet::make(it::FacetType::Classification);
        facet.value = paramFromJson(data.value("value"));
        facet.system = paramFromJson(data.value("system"));
        facet.cardinality = cardinality();
    } else if (type == QLatin1String("PartOf")) {
        facet = it::Facet::make(it::FacetType::PartOf);
        facet.name = param_or("name", "IFCWALL");
        facet.predefinedType = paramFromJson(data.value("predefined_type"));
        facet.relation = optionalString(data.value("relation"));
        facet.cardinality = cardinality();
    } else if (type == QLatin1String("Material")) {
        facet = it::Facet::make(it::FacetType::Material);
        facet.value = paramFromJson(data.value("value"));
        facet.cardinality = cardinality();
    } else {
        throw FormatError("unknown facet type: " + toStd(type));
    }
    facet.instructions = optionalString(data.value("instructions"));
    return facet;
}

// --- selects --------------------------------------------------------------------

QJsonValue toJson(const Select& select) {
    switch (select.kind) {
    case Select::Kind::None:
        return QJsonValue::Null;
    case Select::Kind::Facet: {
        QJsonArray facets;
        for (const it::Facet& facet : select.applicability) facets.append(toJson(facet));
        return QJsonObject{{"type", "SelectFacet"},
                           {"classification_type", select.classification_type},
                           {"classification_name", select.classification_name},
                           {"applicability", facets}};
    }
    case Select::Kind::Rule:
        return QJsonObject{{"type", "SelectRule"},
                           {"action_type", select.action_type},
                           {"element_to_pass", select.element_to_pass},
                           {"element_state_to_pass", select.element_state_to_pass},
                           {"rule", select.rule ? QJsonValue(toJson(*select.rule)) : QJsonValue(QJsonValue::Null)}};
    }
    return QJsonValue::Null;
}

Select selectFromJson(const QJsonValue& value) {
    Select select;
    if (!value.isObject()) return select;
    const QJsonObject data = value.toObject();
    const QString type = data.value("type").toString();
    if (type == QLatin1String("SelectFacet")) {
        select.kind = Select::Kind::Facet;
        select.classification_type = data.value("classification_type").toString("Facet");
        select.classification_name = data.value("classification_name").toString();
        for (const QJsonValue& facet : data.value("applicability").toArray()) {
            select.applicability.push_back(facetFromJson(facet.toObject()));
        }
    } else if (type == QLatin1String("SelectRule")) {
        select.kind = Select::Kind::Rule;
        select.action_type = data.value("action_type").toInt(1);
        select.element_to_pass = data.value("element_to_pass").toString("source");
        select.element_state_to_pass = data.value("element_state_to_pass").toBool(true);
        if (data.value("rule").isObject()) select.rule = ruleFromJson(data.value("rule").toObject());
    }
    return select;
}

// --- boolean rules ----------------------------------------------------------------

QJsonValue toJson(const BooleanNode* node) {
    if (node == nullptr) return QJsonValue::Null;
    switch (node->kind) {
    case BooleanNode::Kind::Leaf:
        return QJsonObject{{"type", "BooleanLeaf"},
                           {"mode", node->mode},
                           {"value", node->value.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(node->value)},
                           {"rule", node->rule ? QJsonValue(toJson(*node->rule)) : QJsonValue(QJsonValue::Null)}};
    case BooleanNode::Kind::And:
    case BooleanNode::Kind::Or: {
        QJsonArray children;
        for (const auto& child : node->children) children.append(toJson(child.get()));
        return QJsonObject{{"type", node->kind == BooleanNode::Kind::And ? "AndRule" : "OrRule"}, {"children", children}};
    }
    case BooleanNode::Kind::Not:
        return QJsonObject{{"type", "NotRule"},
                           {"child", node->children.empty() ? QJsonValue(QJsonValue::Null) : toJson(node->children.front().get())}};
    }
    return QJsonValue::Null;
}

std::shared_ptr<BooleanNode> booleanFromJson(const QJsonValue& value) {
    if (!value.isObject()) return nullptr;
    const QJsonObject data = value.toObject();
    const QString type = data.value("type").toString();
    auto node = std::make_shared<BooleanNode>();
    if (type == QLatin1String("BooleanLeaf")) {
        node->kind = BooleanNode::Kind::Leaf;
        node->mode = data.value("mode").toString("have_result");
        node->value = optionalString(data.value("value")) ? s(*optionalString(data.value("value"))) : QString();
        if (data.value("rule").isObject()) node->rule = ruleFromJson(data.value("rule").toObject());
    } else if (type == QLatin1String("AndRule") || type == QLatin1String("OrRule")) {
        node->kind = type == QLatin1String("AndRule") ? BooleanNode::Kind::And : BooleanNode::Kind::Or;
        for (const QJsonValue& child : data.value("children").toArray()) {
            if (auto c = booleanFromJson(child)) node->children.push_back(std::move(c));
        }
    } else if (type == QLatin1String("NotRule")) {
        node->kind = BooleanNode::Kind::Not;
        if (auto c = booleanFromJson(data.value("child"))) node->children.push_back(std::move(c));
    } else {
        throw FormatError("unknown boolean rule type: " + toStd(type));
    }
    return node;
}

// --- rules ----------------------------------------------------------------------

QJsonObject toJson(const RuleCheck& rule) {
    QJsonObject data{{"type", rule.type}, {"id", rule.id.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(rule.id)},
                     {"state", rule.state}, {"select_grouping", groupingToJson(rule.grouping)}};
    QJsonArray criticity;
    for (const Select& select : rule.criticity) criticity.append(toJson(select));
    QJsonArray actor;
    for (const Select& select : rule.actor) actor.append(toJson(select));
    data["select_criticity"] = criticity;
    data["select_actor"] = actor;
    for (auto i = rule.params.begin(); i != rule.params.end(); ++i) {
        if (!commonRuleKeys().count(i.key())) data[i.key()] = i.value();
    }
    if (!rule.context.empty()) data["select_context"] = toJson(rule.context);
    if (!rule.source.empty()) data["select_source"] = toJson(rule.source);
    if (!rule.target.empty()) data["select_target"] = toJson(rule.target);
    if (!rule.exception.relation.isEmpty()) data["select_exception"] = rule.exception.relation;
    else if (rule.exception.tree) data["select_exception"] = toJson(rule.exception.tree.get());
    if (rule.must) data["abs_or_rel_check"] = mustToJson(rule.must);
    QJsonObject viewer = viewerToJson(rule.viewer);
    if (!rule.exception.empty() && rule.exception.timing != QLatin1String("after")) viewer["exception_timing"] = rule.exception.timing;
    if (!viewer.isEmpty()) data["viewer"] = viewer;
    return data;
}

std::shared_ptr<RuleCheck> ruleFromJson(const QJsonObject& data) {
    auto rule = std::make_shared<RuleCheck>();
    rule->type = data.value("type").toString();
    if (rule->type.isEmpty()) throw FormatError("a rule without type");
    rule->id = data.value("id").toString();
    rule->state = data.value("state").toString("Final");
    rule->grouping = groupingFromJson(data.value("select_grouping"));
    for (const QJsonValue& v : data.value("select_criticity").toArray()) rule->criticity.push_back(selectFromJson(v));
    for (const QJsonValue& v : data.value("select_actor").toArray()) rule->actor.push_back(selectFromJson(v));
    rule->source = selectFromJson(data.value("select_source"));
    rule->target = selectFromJson(data.value("select_target"));
    rule->context = selectFromJson(data.value("select_context"));
    const QJsonValue exception = data.value("select_exception");
    if (exception.isString()) rule->exception.relation = exception.toString();
    else if (exception.isObject()) rule->exception.tree = booleanFromJson(exception);
    rule->must = mustFromJson(data.value("abs_or_rel_check"));
    rule->viewer = viewerFromJson(data);
    rule->exception.timing = data.value("viewer").toObject().value("exception_timing").toString("after");
    for (auto i = data.begin(); i != data.end(); ++i) {
        if (!commonRuleKeys().count(i.key())) rule->params[i.key()] = i.value();
    }
    return rule;
}

// --- folders and files ----------------------------------------------------------

QJsonObject toJson(const RuleFolder& folder) {
    QJsonArray contains;
    for (const RuleNode& node : folder.contains) {
        if (const auto* f = std::get_if<std::shared_ptr<RuleFolder>>(&node)) contains.append(toJson(**f));
        else contains.append(toJson(*std::get<std::shared_ptr<RuleCheck>>(node)));
    }
    QJsonValue activation = QJsonValue::Null;
    if (folder.activation_tree) activation = toJson(folder.activation_tree.get());
    else if (!folder.activation_select.empty()) activation = toJson(folder.activation_select);
    QJsonObject data{{"type", "RuleFolder"}, {"id", folder.id}, {"activation_case", folder.activation_case},
                     {"activation_rule", activation}, {"contains", contains}};
    QJsonObject viewer = viewerToJson(folder.viewer);
    if (!folder.active) viewer["active"] = false;
    if (!viewer.isEmpty()) data["viewer"] = viewer;
    return data;
}

RuleNode nodeFromJson(const QJsonObject& data) {
    if (data.value("type").toString() != QLatin1String("RuleFolder")) return ruleFromJson(data);
    auto folder = std::make_shared<RuleFolder>();
    folder->id = data.value("id").toString();
    folder->activation_case = data.value("activation_case").toString("ALLTRUE");
    const QJsonValue activation = data.value("activation_rule");
    if (activation.isObject()) {
        const QString type = activation.toObject().value("type").toString();
        if (type == "BooleanLeaf" || type == "AndRule" || type == "OrRule" || type == "NotRule") {
            folder->activation_tree = booleanFromJson(activation);
        } else {
            folder->activation_select = selectFromJson(activation);
        }
    }
    for (const QJsonValue& child : data.value("contains").toArray()) folder->contains.push_back(nodeFromJson(child.toObject()));
    folder->viewer = viewerFromJson(data);
    folder->active = data.value("viewer").toObject().value("active").toBool(true);
    return folder;
}

QJsonObject toJson(const RuleFile& file) {
    QJsonArray contains;
    for (const RuleNode& node : file.contains) {
        if (const auto* f = std::get_if<std::shared_ptr<RuleFolder>>(&node)) contains.append(toJson(**f));
        else contains.append(toJson(*std::get<std::shared_ptr<RuleCheck>>(node)));
    }
    // No model path: a rule file is reused on any set of models (the
    // viewer runs it on the loaded ones). The Python CLI takes them as
    // arguments ("list_ifc_path" defaults to []).
    QJsonObject data{{"type", "RuleFile"}, {"id", file.id},
                     {"path_to_save", file.path_to_save.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(file.path_to_save)},
                     {"contains", contains}};
    const QJsonObject viewer = viewerToJson(file.viewer);
    if (!viewer.isEmpty()) data["viewer"] = viewer;
    return data;
}

RuleFile fileFromJson(const QJsonObject& data) {
    if (data.value("type").toString("RuleFile") != QLatin1String("RuleFile")) throw FormatError("not a RuleFile");
    RuleFile file;
    file.id = data.value("id").toString();
    for (const QJsonValue& path : data.value("list_ifc_path").toArray()) file.list_ifc_path.push_back(path.toString());
    file.path_to_save = data.value("path_to_save").toString();
    for (const QJsonValue& child : data.value("contains").toArray()) file.contains.push_back(nodeFromJson(child.toObject()));
    file.viewer = viewerFromJson(data);
    return file;
}

QJsonObject parseDocument(const QByteArray& text) {
    // Replace the Python number tokens outside the strings.
    QByteArray clean;
    clean.reserve(text.size());
    bool in_string = false;
    for (qsizetype i = 0; i < text.size(); ++i) {
        const char c = text[i];
        if (in_string) {
            clean.append(c);
            if (c == '\\' && i + 1 < text.size()) clean.append(text[++i]);
            else if (c == '"') in_string = false;
            continue;
        }
        if (c == '"') {
            in_string = true;
            clean.append(c);
            continue;
        }
        const auto starts = [&](const char* token) { return text.mid(i, qsizetype(std::strlen(token))) == token; };
        if (starts("-Infinity")) { clean.append("-1e308"); i += 8; continue; }
        if (starts("Infinity")) { clean.append("1e308"); i += 7; continue; }
        if (starts("NaN")) { clean.append("null"); i += 2; continue; }
        clean.append(c);
    }
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(clean, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        throw FormatError("not a rule file: " + error.errorString().toStdString());
    }
    return document.object();
}

std::shared_ptr<RuleCheck> cloneRule(const RuleCheck& rule) {
    return ruleFromJson(toJson(rule));
}

} // namespace bonsaiviewer::services::clash::json
