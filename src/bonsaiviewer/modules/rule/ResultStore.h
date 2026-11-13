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

#ifndef IFCINTERFACE_MODULES_RULE_RESULTSTORE_H
#define IFCINTERFACE_MODULES_RULE_RESULTSTORE_H

#include "ResultKey.h"
#include "Results.h"

#include <QJsonObject>
#include <QString>
#include <QStringList>

#include <map>
#include <utility>
#include <vector>

namespace bonsaiviewer::modules::rule::store {

// The clash section of a federation (extensions["ifcclash_plus"]): the rule
// file, the last run of each rule (its parameters at that time, the elements
// it checked) and the results with their review, manual group and topic. The
// results point at the elements by model id + GlobalId + STEP id; the viewer
// resolves them once the models are loaded.

inline constexpr int kFormat = 1;
inline constexpr const char* kExtensionKey = "ifcclash_plus";

struct SavedModel {
    QString model_id;
    QString name;
    QString path;
    qint64 mtime = 0; // ms since the epoch, the IFC file at the run
    qint64 size = 0;
};

struct SavedRun {
    QString id;
    QString started; // xs:dateTime, UTC
    double seconds = 0.0;
    std::vector<SavedModel> models;
};

// One rule (or folder) of a run.
struct SavedNode {
    QString node_id;
    QString run_id;
    QString label;
    QString type;     // empty: a folder
    QJsonObject rule; // the rule as it ran (RuleJson::toJson)
    QString text;     // the result text of the rule tree ("3 results", "error"…)
    std::vector<ResultElement> sources, targets, not_checked;
};

struct SavedClash {
    QJsonObject rule_file; // RuleJson::toJson(RuleFile), empty: none
    QString rule_file_path;
    std::vector<SavedRun> runs;
    std::vector<SavedNode> nodes;
    // The rows as saved: what the panel shows, without the viewer ids
    // (source_model, target_model, *_objects), filled at the resolution.
    std::vector<ResultRow> results;
    // The set-aside rows (Must verdicts, excepted pairs): saved like the
    // results (review, manual group, topic), shown only in set-aside mode
    // and never numbered.
    std::vector<ResultRow> set_aside;
    std::vector<std::pair<QString, QString>> messages; // node id (empty: the run), text
    QString bcf_file; // the BCF file of the topics of the results (empty: not saved yet)
    // The short numbers of the results (resultNumberText): result key -> number,
    // kept for the results gone too, so that a number is never given twice.
    std::map<QString, int> numbers;
    int next_number = 1;
};

QString reviewName(ReviewStatus status);
ReviewStatus reviewFromName(const QString& name);

QJsonObject toJson(const SavedClash& clash);
// Unknown keys are ignored; a result whose record cannot be read is kept
// without its record, with a warning.
SavedClash fromJson(const QJsonObject& json, QStringList* warnings = nullptr);

} // namespace bonsaiviewer::modules::rule::store

#endif // IFCINTERFACE_MODULES_RULE_RESULTSTORE_H
