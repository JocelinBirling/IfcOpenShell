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

#ifndef IFCINTERFACE_MODULES_BCF_AUTHOR_H
#define IFCINTERFACE_MODULES_BCF_AUTHOR_H

#include <QSettings>
#include <QString>
#include <QStringList>

#include <utility>
#include <vector>

namespace bonsaiviewer::modules::bcf {

// BonsaiViewer10: the BCF settings (Settings > BCF). The author of the BCF
// topics and comments (an e-mail address, as BCF 3.0 expects).
inline QString author() {
    return QSettings().value(QStringLiteral("bcf/author")).toString().trimmed();
}

inline void setAuthor(const QString& value) {
    QSettings().setValue(QStringLiteral("bcf/author"), value.trimmed());
}

// The priority of a topic made from clash results, by criticity: the first
// lines rank highest (Settings > BCF). Each line: "criticity = priority".
using CriticityPriorities = std::vector<std::pair<QString, QString>>;

inline CriticityPriorities parseCriticityPriorities(const QString& text) {
    CriticityPriorities table;
    for (const QString& line : text.split('\n')) {
        const int equal = line.indexOf('=');
        if (equal <= 0) continue;
        const QString criticity = line.left(equal).trimmed();
        const QString priority = line.mid(equal + 1).trimmed();
        if (!criticity.isEmpty() && !priority.isEmpty()) table.emplace_back(criticity, priority);
    }
    return table;
}

inline QString criticityPrioritiesText() {
    return QSettings()
        .value(QStringLiteral("bcf/criticity_priorities"), QStringLiteral("High = Critical\nMedium = Normal\nLow = Minor"))
        .toString();
}

inline CriticityPriorities criticityPriorities() {
    return parseCriticityPriorities(criticityPrioritiesText());
}

inline void setCriticityPrioritiesText(const QString& text) {
    QStringList lines;
    for (const auto& [criticity, priority] : parseCriticityPriorities(text)) lines << criticity + " = " + priority;
    QSettings().setValue(QStringLiteral("bcf/criticity_priorities"), lines.join('\n'));
}

} // namespace bonsaiviewer::modules::bcf

#endif // IFCINTERFACE_MODULES_BCF_AUTHOR_H
