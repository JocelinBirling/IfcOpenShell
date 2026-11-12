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

#ifndef IFCINTERFACE_MODULES_RULE_CLASHTOPIC_H
#define IFCINTERFACE_MODULES_RULE_CLASHTOPIC_H

#include "Results.h"

#include "../../ports/bcf/Bcf.h"

#include <QString>
#include <QStringList>

#include <utility>
#include <vector>

namespace bonsaiviewer::modules::rule::topic {

// The content of a BCF topic made from clash results: everything but the
// viewpoints (BcfViewpoints) — the title, the description, the labels, the
// priority, the IFC files of the header.

// Criticity -> priority, the first entries rank highest (Settings > BCF).
using PriorityTable = std::vector<std::pair<QString, QString>>;

// The title: {rule}, {source}, {target}, {measure}, {n} (the number of
// results) replaced by the values of the first result.
QString title(const QString& pattern, const std::vector<const ResultRow*>& rows);
// The text of one result: the rule, the source, the target, the measures,
// the criticity, the actor, the groups, the review, and a last line
// "Result key: <key>" (the link can be found again from the topic).
QString describe(const ResultRow& row);
// One result: its text; several: a line counting them, then each text.
QString description(const std::vector<const ResultRow*>& rows);
// "Clash", the rules, the criticities, the actors (each once).
QStringList labels(const std::vector<const ResultRow*>& rows);
// The priority of the highest criticity of the results (empty: none in the table).
QString priority(const std::vector<const ResultRow*>& rows, const PriorityTable& table);

// A loaded IFC model, for the header of the topic.
struct ModelFile {
    QString model_id;
    QString path;
    QString ifc_project; // GlobalId of the IfcProject, if known
};
// One header file per model the results point at, in the order of `models`.
std::vector<ports::bcf::HeaderFile> headerFiles(const std::vector<const ResultRow*>& rows,
                                                const std::vector<ModelFile>& models);

} // namespace bonsaiviewer::modules::rule::topic

#endif // IFCINTERFACE_MODULES_RULE_CLASHTOPIC_H
