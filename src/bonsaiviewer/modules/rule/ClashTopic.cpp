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

#include "ClashTopic.h"

#include "ResultStore.h"

#include <QDateTime>
#include <QFileInfo>
#include <QUrl>

#include <set>

namespace bonsaiviewer::modules::rule::topic {

namespace {

QString side(const QString& name, const QString& cls, const QString& guid, const QString& model) {
    QStringList parts{name.isEmpty() ? QStringLiteral("(unnamed)") : name};
    if (!cls.isEmpty()) parts << QString("[%1]").arg(cls);
    if (!guid.isEmpty()) parts << guid;
    QString text = parts.join(' ');
    if (!model.isEmpty()) text += " — " + model;
    return text;
}

QString measures(const ResultRow& row) {
    if (!row.record || row.record->measures.empty()) return row.measure;
    QStringList parts;
    for (const auto& [measure, value] : row.record->measures.items()) {
        const QString unit = measureUnit(measure);
        parts << QString("%1 = %2%3")
                     .arg(QString::fromLatin1(ifcclash::measure_name(measure)))
                     .arg(value, 0, 'g', 4)
                     .arg(unit.isEmpty() ? QString() : " " + unit);
    }
    return parts.join("; ");
}

void addOnce(QStringList& list, const QString& value) {
    if (!value.isEmpty() && !list.contains(value)) list << value;
}

} // namespace

QString title(const QString& pattern, const std::vector<const ResultRow*>& rows) {
    if (rows.empty()) return pattern;
    const ResultRow& first = *rows.front();
    QString text = pattern;
    text.replace("{rule}", first.rule_label);
    text.replace("{source}", first.source_name);
    text.replace("{target}", first.target_name);
    text.replace("{measure}", first.measure);
    text.replace("{n}", QString::number(rows.size()));
    return text.simplified();
}

QString describe(const ResultRow& row) {
    QStringList lines;
    lines << QString("Rule: %1%2").arg(row.rule_label, row.rule_type.isEmpty() ? QString() : QString(" (%1)").arg(row.rule_type));
    if (row.complex) {
        lines << QString("Group: %1 — %2 source(s), %3 target(s), %4")
                     .arg(row.group.isEmpty() ? QStringLiteral("(no name)") : row.group)
                     .arg(row.sources.size())
                     .arg(row.targets.size())
                     .arg(row.status ? "passes" : "fails");
    } else {
        lines << "Source: " + side(row.source_name, row.source_class, row.source_guid, row.source_model_name);
        if (!row.target_guid.isEmpty() || !row.target_name.isEmpty()) {
            lines << "Target: " + side(row.target_name, row.target_class, row.target_guid, row.target_model_name);
        }
        const QString measured = measures(row);
        if (!measured.isEmpty()) lines << "Measure: " + measured;
        if (!row.group.isEmpty()) lines << "Rule group: " + row.group;
    }
    if (!row.status && !row.complex) lines << "Excepted: an exception of the rule invalidates this result";
    if (!row.criticity.isEmpty()) lines << "Criticity: " + row.criticity.join(", ");
    if (!row.actor.isEmpty()) lines << "Actor: " + row.actor.join(", ");
    if (!row.manual_group.isEmpty()) lines << "Manual group: " + row.manual_group;
    lines << "Review: " + store::reviewName(row.review);
    lines << "Result key: " + row.key;
    return lines.join('\n');
}

QString description(const std::vector<const ResultRow*>& rows) {
    if (rows.empty()) return {};
    if (rows.size() == 1) return describe(*rows.front());
    QStringList rules;
    for (const ResultRow* row : rows) addOnce(rules, row->rule_label);
    QStringList parts{QString("%1 clash results (%2).").arg(rows.size()).arg(rules.join(", "))};
    int number = 1;
    for (const ResultRow* row : rows) parts << QString("Result %1\n%2").arg(number++).arg(describe(*row));
    return parts.join("\n\n");
}

QStringList labels(const std::vector<const ResultRow*>& rows) {
    QStringList out{QStringLiteral("Clash")};
    for (const ResultRow* row : rows) addOnce(out, row->rule_label);
    for (const ResultRow* row : rows) {
        for (const QString& value : row->criticity) addOnce(out, value);
    }
    for (const ResultRow* row : rows) {
        for (const QString& value : row->actor) addOnce(out, value);
    }
    return out;
}

QString priority(const std::vector<const ResultRow*>& rows, const PriorityTable& table) {
    for (const auto& [criticity, value] : table) {
        for (const ResultRow* row : rows) {
            if (row->criticity.contains(criticity, Qt::CaseInsensitive)) return value;
        }
    }
    return {};
}

std::vector<ports::bcf::HeaderFile> headerFiles(const std::vector<const ResultRow*>& rows,
                                                const std::vector<ModelFile>& models) {
    std::set<QString> used;
    for (const ResultRow* row : rows) {
        for (const auto* list : {&row->sources, &row->targets}) {
            for (const ResultElement& element : *list) used.insert(element.model_id);
        }
    }
    std::vector<ports::bcf::HeaderFile> files;
    for (const ModelFile& model : models) {
        if (!used.count(model.model_id) || model.path.isEmpty()) continue;
        const QFileInfo info(model.path);
        ports::bcf::HeaderFile file;
        file.filename = info.fileName();
        file.ifc_project = model.ifc_project;
        file.is_external = true;
        file.reference = QUrl::fromLocalFile(info.absoluteFilePath()).toString();
        if (info.exists()) file.date = info.lastModified().toUTC().toString(Qt::ISODate);
        files.push_back(std::move(file));
    }
    return files;
}

} // namespace bonsaiviewer::modules::rule::topic
