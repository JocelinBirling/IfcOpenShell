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

#include "ResultKey.h"

#include <QCryptographicHash>
#include <QStringList>

namespace bonsaiviewer::modules::rule {

namespace {

QString token(const ResultElement& element) {
    const QString id = element.global_id.isEmpty() ? QString("#%1").arg(element.step_id) : element.global_id;
    return element.model_id + ':' + id;
}

QString digest(const std::vector<ResultElement>& elements) {
    QStringList tokens;
    for (const ResultElement& element : elements) tokens << token(element);
    tokens.sort();
    return QString::fromLatin1(QCryptographicHash::hash(tokens.join('\n').toUtf8(), QCryptographicHash::Sha1).toHex());
}

} // namespace

QString resultKey(const QString& rule_id, bool complex, const QString& group,
                  const std::vector<ResultElement>& sources, const std::vector<ResultElement>& targets) {
    if (complex) return QStringList{rule_id, group, digest(sources), digest(targets)}.join('|');
    return QStringList{rule_id, sources.empty() ? QString() : token(sources.front()),
                       targets.empty() ? QString() : token(targets.front())}
        .join('|');
}

} // namespace bonsaiviewer::modules::rule
