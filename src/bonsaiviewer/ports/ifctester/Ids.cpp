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
 * Bonsai is distributed in the hope that it will be useful,                    *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of               *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                 *
 * GNU General Public License for more details.                                 *
 *                                                                              *
 * You should have received a copy of the GNU General Public License            *
 * along with this program. If not, see <http://www.gnu.org/licenses/>.         *
 *                                                                              *
 ********************************************************************************/

// PORT — see Ids.h.

#include "Ids.h"

#include "../../../ifcparse/file.h"
#include "../../../ifcparse/schema.h"

#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <algorithm>

namespace bonsaiviewer::ports::ifctester {

namespace {

const char* kIdsNamespace = "http://standards.buildingsmart.org/IDS";
const char* kInfoKeys[] = {"title", "copyright", "version", "description", "author", "date", "purpose", "milestone"};

std::string str(const QStringView text) {
    return text.toString().toStdString();
}

QString q(const std::string& text) {
    return QString::fromStdString(text);
}

std::optional<std::string> attribute(const QXmlStreamReader& reader, const char* name) {
    const auto attributes = reader.attributes();
    for (const auto& a : attributes) {
        if (a.name() == QLatin1String(name)) return str(a.value());
    }
    return std::nullopt;
}

// ------------------------------------------------------------------ reading

// <xs:restriction base="xs:string"><xs:enumeration value="A"/>...</xs:restriction>
Restriction readRestriction(QXmlStreamReader& reader) {
    Restriction restriction;
    const auto base = attribute(reader, "base").value_or("xs:string");
    restriction.base = base.size() > 3 ? base.substr(3) : base;  // ids_dict.get("@base", "xs:string")[3:]
    while (reader.readNextStartElement()) {
        const std::string key = str(reader.name());
        if (key == "annotation") {
            reader.skipCurrentElement();
            continue;
        }
        if (const auto value = attribute(reader, "value")) restriction.add(key, *value);
        reader.skipCurrentElement();
    }
    return restriction;
}

// A facet parameter: <simpleValue>…</simpleValue> or <xs:restriction>…; an
// empty element is None.
Param readParam(QXmlStreamReader& reader) {
    Param param;
    while (reader.readNextStartElement()) {
        const auto name = reader.name();
        if (name == QLatin1String("simpleValue")) {
            param = Param::text(reader.readElementText().toStdString());
        } else if (name == QLatin1String("restriction")) {
            const Restriction restriction = readRestriction(reader);
            if (!param.isRestriction()) param = Param::of(restriction);  // the first one, as Python can only take one
        } else {
            reader.skipCurrentElement();
        }
    }
    return param;
}

void readParamsInto(QXmlStreamReader& reader, Facet& facet) {
    while (reader.readNextStartElement()) {
        const std::string name = str(reader.name());
        if (name == "name") facet.name = readParam(reader);
        else if (name == "predefinedType") facet.predefinedType = readParam(reader);
        else if (name == "value") facet.value = readParam(reader);
        else if (name == "system") facet.system = readParam(reader);
        else if (name == "propertySet") facet.propertySet = readParam(reader);
        else if (name == "baseName") facet.baseName = readParam(reader);
        else if (name == "entity") readParamsInto(reader, facet);  // partOf > entity > name, predefinedType
        else reader.skipCurrentElement();
    }
}

// Facet.parse(xml): the constructor defaults, then every attribute / element found.
Facet readFacet(QXmlStreamReader& reader, FacetType type) {
    Facet facet = Facet::make(type);
    facet.cardinality = cardinalityFromName(attribute(reader, "cardinality").value_or("required"))
                            .value_or(Cardinality::Required);
    facet.relation = attribute(reader, "relation");
    facet.dataType = attribute(reader, "dataType");
    facet.uri = attribute(reader, "uri");
    facet.instructions = attribute(reader, "instructions");
    readParamsInto(reader, facet);
    return facet;
}

std::vector<Facet> readClause(QXmlStreamReader& reader) {
    std::vector<Facet> facets;
    while (reader.readNextStartElement()) {
        if (const auto type = facetTypeFromName(str(reader.name()))) {
            facets.push_back(readFacet(reader, *type));
        } else {
            reader.skipCurrentElement();
        }
    }
    return facets;
}

std::optional<int> occurs(const std::optional<std::string>& text) {
    if (!text) return std::nullopt;
    bool ok = false;
    const int value = q(*text).trimmed().toInt(&ok);
    return ok ? std::optional<int>(value) : std::nullopt;
}

Specification readSpecification(QXmlStreamReader& reader) {
    Specification spec;
    spec.name = attribute(reader, "name").value_or("");
    spec.identifier = attribute(reader, "identifier");
    spec.description = attribute(reader, "description").value_or("");
    spec.instructions = attribute(reader, "instructions").value_or("");
    spec.ifcVersion.clear();
    for (const QString& version : q(attribute(reader, "ifcVersion").value_or("")).split(' ', Qt::SkipEmptyParts)) {
        spec.ifcVersion.push_back(version.toStdString());
    }
    while (reader.readNextStartElement()) {
        const auto name = reader.name();
        if (name == QLatin1String("applicability")) {
            // xs:occurs defaults (applied by xmlschema): minOccurs 1, maxOccurs 1.
            spec.minOccurs = occurs(attribute(reader, "minOccurs")).value_or(1);
            const auto max_text = attribute(reader, "maxOccurs").value_or("1");
            spec.maxOccurs = max_text != "unbounded" ? occurs(max_text) : std::nullopt;
            spec.applicability = readClause(reader);
        } else if (name == QLatin1String("requirements")) {
            spec.requirements = readClause(reader);
        } else {
            reader.skipCurrentElement();
        }
    }
    spec.requirement_outcomes.assign(spec.requirements.size(), FacetOutcome());
    return spec;
}

void readInfo(QXmlStreamReader& reader, Ids& ids) {
    while (reader.readNextStartElement()) {
        const std::string key = str(reader.name());
        const std::string value = reader.readElementText(QXmlStreamReader::IncludeChildElements).toStdString();
        if (std::find(std::begin(kInfoKeys), std::end(kInfoKeys), key) != std::end(kInfoKeys) && !value.empty()) {
            ids.setInfo(key, value);
        }
    }
}

// ------------------------------------------------------------------ writing

// Facet.to_ids_value(parameter)
void writeParam(QXmlStreamWriter& writer, const char* element, const Param& param) {
    if (param.isNone()) return;
    writer.writeStartElement(element);
    if (param.isSimple()) {
        writer.writeTextElement("simpleValue", q(param.simple));
    } else {
        const Restriction& restriction = param.restriction;
        writer.writeStartElement("xs:restriction");
        writer.writeAttribute("base", q("xs:" + restriction.base));
        for (const auto& [constraint, values] : restriction.options) {
            for (const std::string& value : values) {
                writer.writeEmptyElement(q("xs:" + constraint));
                writer.writeAttribute("value", q(value));
            }
        }
        writer.writeEndElement();
    }
    writer.writeEndElement();
}

// Facet.asdict(clause_type), encoded.
void writeFacet(QXmlStreamWriter& writer, const Facet& facet, Clause clause) {
    writer.writeStartElement(q(facetTypeName(facet.type)));
    const bool applicability = clause == Clause::Applicability;
    if (facet.type == FacetType::Property && facet.dataType) writer.writeAttribute("dataType", q(*facet.dataType).toUpper());
    if (facet.type == FacetType::PartOf && facet.relation) writer.writeAttribute("relation", q(*facet.relation));
    if (!applicability) {
        if (facet.uri) writer.writeAttribute("uri", q(*facet.uri));
        if (facet.type != FacetType::Entity) writer.writeAttribute("cardinality", q(cardinalityName(facet.cardinality)));
        if (facet.instructions) writer.writeAttribute("instructions", q(*facet.instructions));
    }
    switch (facet.type) {
    case FacetType::Entity:
        writeParam(writer, "name", facet.name);
        writeParam(writer, "predefinedType", facet.predefinedType);
        break;
    case FacetType::PartOf:
        if (!facet.name.isNone() || !facet.predefinedType.isNone()) {
            writer.writeStartElement("entity");
            writeParam(writer, "name", facet.name);
            writeParam(writer, "predefinedType", facet.predefinedType);
            writer.writeEndElement();
        }
        break;
    case FacetType::Classification:
        writeParam(writer, "value", facet.value);
        writeParam(writer, "system", facet.system);
        break;
    case FacetType::Attribute:
        writeParam(writer, "name", facet.name);
        writeParam(writer, "value", facet.value);
        break;
    case FacetType::Property:
        writeParam(writer, "propertySet", facet.propertySet);
        writeParam(writer, "baseName", facet.baseName);
        writeParam(writer, "value", facet.value);
        break;
    case FacetType::Material:
        writeParam(writer, "value", facet.value);
        break;
    }
    writer.writeEndElement();
}

// The facets of a clause in the XSD order: entity, partOf, classification,
// attribute, property, material.
void writeClause(QXmlStreamWriter& writer, const std::vector<Facet>& facets, Clause clause) {
    for (FacetType type : {FacetType::Entity, FacetType::PartOf, FacetType::Classification, FacetType::Attribute,
                           FacetType::Property, FacetType::Material}) {
        for (const Facet& facet : facets) {
            if (facet.type == type) writeFacet(writer, facet, clause);
        }
    }
}

void writeNamespaces(QXmlStreamWriter& writer) {
    writer.writeAttribute("xmlns", kIdsNamespace);
    writer.writeAttribute("xmlns:xs", "http://www.w3.org/2001/XMLSchema");
    writer.writeAttribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
}

} // namespace

// ------------------------------------------------------------ Specification

void Specification::reset_status() {
    applicable_entities.clear();
    passed_entities.clear();
    failed_entities.clear();
    requirement_outcomes.assign(requirements.size(), FacetOutcome());
    status.reset();
}

bool Specification::check_ifc_version(ifcopenshell::file& file) {
    const std::string schema = file.schema() ? file.schema()->name() : std::string();
    is_ifc_version = std::find(ifcVersion.begin(), ifcVersion.end(), schema) != ifcVersion.end();
    return *is_ifc_version;
}

// def validate(self, ifc_file, should_filter_version=False)
void Specification::validate(ifcopenshell::file& file, bool should_filter_version) {
    if (should_filter_version && !is_ifc_version.value_or(false)) return;
    if (requirement_outcomes.size() != requirements.size()) requirement_outcomes.assign(requirements.size(), FacetOutcome());

    // Broadphase filter of applicability.
    std::optional<std::vector<Entity>> elements;
    for (const Facet& facet : applicability) {
        elements = facet.filter(file, elements ? &*elements : nullptr);
    }

    for (const Entity& element : elements.value_or(std::vector<Entity>())) {
        bool is_applicable = true;
        for (const Facet& facet : applicability) {
            if (facet.type == FacetType::Entity) continue;
            if (!facet(element)) {
                is_applicable = false;
                break;
            }
        }
        if (!is_applicable) continue;
        applicable_entities.push_back(element);
        if (prohibited()) continue;  // Requirements are skipped for prohibited applicability
        for (std::size_t i = 0; i < requirements.size(); ++i) {
            const Result result = requirements[i](element);
            if (result) {
                passed_entities.insert(element);
                requirement_outcomes[i].passed_entities.insert(element);
            } else {
                failed_entities.insert(element);
                requirement_outcomes[i].failures.push_back({element, result.reason});
            }
        }
    }

    status = true;
    for (FacetOutcome& outcome : requirement_outcomes) {
        outcome.status = outcome.failures.empty();
        if (!*outcome.status) status = false;
    }
    if (minOccurs != 0) {  // Required specification
        if (applicable_entities.empty()) {
            status = false;
            for (FacetOutcome& outcome : requirement_outcomes) outcome.status = false;
        }
    } else if (prohibited()) {  // Prohibited specification
        if (!applicable_entities.empty()) status = false;
    }
}

Cardinality Specification::get_usage() const {
    if (minOccurs != 0) return Cardinality::Required;
    if (!prohibited()) return Cardinality::Optional;
    return Cardinality::Prohibited;
}

void Specification::set_usage(Cardinality usage) {
    if (usage == Cardinality::Optional) {
        minOccurs = 0;
        maxOccurs.reset();
    } else if (usage == Cardinality::Prohibited) {
        minOccurs = 0;
        maxOccurs = 0;
    } else {
        minOccurs = 1;
        maxOccurs.reset();
    }
}

// ---------------------------------------------------------------------- Ids

std::optional<std::string> Ids::infoValue(const std::string& key) const {
    for (const auto& [k, v] : info) {
        if (k == key) return v;
    }
    return std::nullopt;
}

void Ids::setInfo(const std::string& key, const std::string& value) {
    info.erase(std::remove_if(info.begin(), info.end(), [&](const auto& kv) { return kv.first == key; }), info.end());
    if (value.empty()) return;
    // Keep the XSD order of the info elements.
    const auto rank = [](const std::string& k) {
        return std::find(std::begin(kInfoKeys), std::end(kInfoKeys), k) - std::begin(kInfoKeys);
    };
    const auto at = std::find_if(info.begin(), info.end(), [&](const auto& kv) { return rank(kv.first) > rank(key); });
    info.insert(at, {key, value});
}

void Ids::validate(ifcopenshell::file& file, bool should_filter_version) {
    for (Specification& specification : specifications) {
        specification.reset_status();
        specification.check_ifc_version(file);
        specification.validate(file, should_filter_version);
    }
}

std::optional<Ids> idsFromXml(const QByteArray& xml, std::string* error) {
    QXmlStreamReader reader(xml);
    if (!reader.readNextStartElement() || reader.name() != QLatin1String("ids")) {
        if (error) *error = reader.hasError() ? reader.errorString().toStdString() : "The XML is not an IDS document";
        return std::nullopt;
    }
    Ids ids;
    ids.info.clear();
    while (reader.readNextStartElement()) {
        const auto name = reader.name();
        if (name == QLatin1String("info")) {
            readInfo(reader, ids);
        } else if (name == QLatin1String("specifications")) {
            while (reader.readNextStartElement()) {
                if (reader.name() == QLatin1String("specification")) {
                    ids.specifications.push_back(readSpecification(reader));
                } else {
                    reader.skipCurrentElement();
                }
            }
        } else {
            reader.skipCurrentElement();
        }
    }
    if (reader.hasError()) {
        if (error) *error = reader.errorString().toStdString();
        return std::nullopt;
    }
    if (!ids.infoValue("title")) ids.setInfo("title", "Untitled");
    return ids;
}

QByteArray idsToXml(const Ids& ids) {
    QByteArray xml;
    QXmlStreamWriter writer(&xml);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeStartElement("ids");
    writeNamespaces(writer);
    writer.writeAttribute("xsi:schemaLocation",
                          "http://standards.buildingsmart.org/IDS http://standards.buildingsmart.org/IDS/1.0/ids.xsd");
    writer.writeStartElement("info");
    for (const auto& [key, value] : ids.info) writer.writeTextElement(q(key), q(value));
    writer.writeEndElement();
    writer.writeStartElement("specifications");
    for (const Specification& spec : ids.specifications) {
        writer.writeStartElement("specification");
        writer.writeAttribute("name", q(spec.name));
        std::string versions;
        for (const std::string& version : spec.ifcVersion) versions += (versions.empty() ? "" : " ") + version;
        writer.writeAttribute("ifcVersion", q(versions));
        if (spec.identifier) writer.writeAttribute("identifier", q(*spec.identifier));
        if (spec.description && !spec.description->empty()) writer.writeAttribute("description", q(*spec.description));
        if (spec.instructions && !spec.instructions->empty()) writer.writeAttribute("instructions", q(*spec.instructions));
        writer.writeStartElement("applicability");
        writer.writeAttribute("minOccurs", QString::number(spec.minOccurs));
        writer.writeAttribute("maxOccurs", spec.maxOccurs ? QString::number(*spec.maxOccurs) : QString("unbounded"));
        writeClause(writer, spec.applicability, Clause::Applicability);
        writer.writeEndElement();
        writer.writeStartElement("requirements");
        writeClause(writer, spec.requirements, Clause::Requirement);
        writer.writeEndElement();
        writer.writeEndElement();
    }
    writer.writeEndElement();
    writer.writeEndElement();
    writer.writeEndDocument();
    return xml;
}

QByteArray facetsToXml(const std::vector<Facet>& facets, Clause clause) {
    QByteArray xml;
    QXmlStreamWriter writer(&xml);
    writer.setAutoFormatting(true);
    writer.writeStartElement("facets");
    writeNamespaces(writer);
    for (const Facet& facet : facets) writeFacet(writer, facet, clause);  // the given order, not the XSD one
    writer.writeEndElement();
    return xml;
}

std::optional<std::vector<Facet>> facetsFromXml(const QByteArray& xml) {
    QXmlStreamReader reader(xml);
    if (!reader.readNextStartElement()) return std::nullopt;
    const std::string root = str(reader.name());
    if (const auto type = facetTypeFromName(root)) return std::vector<Facet>{readFacet(reader, *type)};
    if (root == "facets") {
        std::vector<Facet> facets = readClause(reader);
        if (reader.hasError() || facets.empty()) return std::nullopt;
        return facets;
    }
    if (root == "ids") {
        const auto ids = idsFromXml(xml);
        if (!ids || ids->specifications.empty()) return std::nullopt;
        std::vector<Facet> facets = ids->specifications.front().applicability;
        const auto& requirements = ids->specifications.front().requirements;
        facets.insert(facets.end(), requirements.begin(), requirements.end());
        if (facets.empty()) return std::nullopt;
        return facets;
    }
    return std::nullopt;
}

} // namespace bonsaiviewer::ports::ifctester
