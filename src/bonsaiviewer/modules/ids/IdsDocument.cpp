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

#include "IdsDocument.h"

#include "../../services/IdsEngine.h"

#include <QJsonArray>
#include <QJsonDocument>

namespace bonsaiviewer::modules::ids {

namespace engine = bonsaiviewer::services::ids_engine;
namespace it = bonsaiviewer::ports::ifctester;

namespace {

std::optional<std::string> optionalText(const QString& text) {
    return text.trimmed().isEmpty() ? std::nullopt : std::optional<std::string>(text.trimmed().toStdString());
}

QString text(const std::optional<std::string>& value) {
    return value ? QString::fromStdString(*value) : QString();
}

std::vector<it::Facet> toFacets(const std::vector<bonsaiviewer::services::IdsFacet>& facets) {
    std::vector<it::Facet> result;
    int index = 0;
    for (const auto& facet : facets) result.push_back(engine::toFacet(facet, ++index));
    return result;
}

} // namespace

QByteArray SpecificationEdit::facetsSignature() const {
    QJsonArray json;
    for (const auto* clause : {&applicability, &requirements}) {
        QJsonArray facets;
        for (const auto& facet : *clause) facets.append(facet.toJson());
        json.append(facets);
    }
    return QJsonDocument(json).toJson(QJsonDocument::Compact);
}

const QStringList& IdsDocument::infoKeys() {
    static const QStringList keys = {"title", "copyright", "version", "description",
                                     "author", "date", "purpose", "milestone"};
    return keys;
}

QString IdsDocument::infoValue(const QString& key) const {
    for (const auto& [k, v] : info) {
        if (k == key) return v;
    }
    return {};
}

void IdsDocument::setInfo(const QString& key, const QString& value) {
    for (int i = 0; i < info.size(); ++i) {
        if (info[i].first != key) continue;
        if (value.trimmed().isEmpty()) info.removeAt(i);
        else info[i].second = value.trimmed();
        return;
    }
    if (!value.trimmed().isEmpty()) info.append({key, value.trimmed()});
}

IdsDocument IdsDocument::fromXml(const QByteArray& xml) {
    std::string error;
    const auto ids = it::idsFromXml(xml, &error);
    if (!ids) throw engine::UserError("the file is not an IDS document: " + error);
    IdsDocument document;
    document.info.clear();
    for (const auto& [key, value] : ids->info) {
        document.info.append({QString::fromStdString(key), QString::fromStdString(value)});
    }
    for (const it::Specification& spec : ids->specifications) {
        SpecificationEdit edit;
        edit.name = QString::fromStdString(spec.name);
        edit.identifier = text(spec.identifier);
        edit.description = text(spec.description);
        edit.instructions = text(spec.instructions);
        edit.ifc_versions.clear();
        for (const auto& version : spec.ifcVersion) edit.ifc_versions << QString::fromStdString(version);
        edit.usage = QString::fromStdString(it::cardinalityName(spec.get_usage()));
        auto original = std::make_shared<SpecificationEdit::Original>();
        original->applicability = spec.applicability;
        original->requirements = spec.requirements;
        try {
            for (const it::Facet& facet : spec.applicability) edit.applicability.push_back(engine::fromFacet(facet));
            for (const it::Facet& facet : spec.requirements) edit.requirements.push_back(engine::fromFacet(facet));
            original->signature = edit.facetsSignature();
        } catch (const std::exception& e) {
            edit.applicability.clear();
            edit.requirements.clear();
            original->editable = false;
            original->reason = QString::fromStdString(e.what());
        }
        edit.original = std::move(original);
        document.specifications.push_back(std::move(edit));
    }
    return document;
}

it::Ids IdsDocument::toIds() const {
    it::Ids ids;
    ids.info.clear();
    for (const QString& key : infoKeys()) {
        const QString value = infoValue(key);
        if (!value.isEmpty()) ids.setInfo(key.toStdString(), value.toStdString());
    }
    if (!ids.infoValue("title")) ids.setInfo("title", "Untitled");
    for (const SpecificationEdit& edit : specifications) {
        const QString name = edit.name.trimmed().isEmpty() ? QString("Unnamed") : edit.name.trimmed();
        const auto fail = [&](const QString& message) {
            throw engine::UserError(QString("specification \"%1\": %2").arg(name, message).toStdString());
        };
        if (edit.ifc_versions.isEmpty()) fail("choose at least one IFC version");
        it::Specification spec;
        spec.name = name.toStdString();
        spec.identifier = optionalText(edit.identifier);
        spec.description = optionalText(edit.description);
        spec.instructions = optionalText(edit.instructions);
        spec.ifcVersion.clear();
        for (const QString& version : edit.ifc_versions) spec.ifcVersion.push_back(version.toStdString());
        spec.set_usage(it::cardinalityFromName(edit.usage.toStdString()).value_or(it::Cardinality::Required));
        if (edit.original && (!edit.original->editable || edit.facetsSignature() == edit.original->signature)) {
            spec.applicability = edit.original->applicability; // unchanged since read
            spec.requirements = edit.original->requirements;
        } else {
            if (edit.applicability.empty()) fail("the applicability needs at least one facet");
            try {
                spec.applicability = toFacets(edit.applicability);
                spec.requirements = toFacets(edit.requirements);
            } catch (const engine::UserError& e) {
                fail(e.what());
            }
        }
        spec.requirement_outcomes.assign(spec.requirements.size(), it::FacetOutcome());
        ids.specifications.push_back(std::move(spec));
    }
    return ids;
}

QByteArray IdsDocument::toXml() const {
    return it::idsToXml(toIds());
}

} // namespace bonsaiviewer::modules::ids
