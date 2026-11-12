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

namespace bonsaiviewer::modules::bcf {

// BonsaiViewer10: the author of the BCF topics and comments (an e-mail
// address, as BCF 3.0 expects), set in Settings > BCF.
inline QString author() {
    return QSettings().value(QStringLiteral("bcf/author")).toString().trimmed();
}

inline void setAuthor(const QString& value) {
    QSettings().setValue(QStringLiteral("bcf/author"), value.trimmed());
}

} // namespace bonsaiviewer::modules::bcf

#endif // IFCINTERFACE_MODULES_BCF_AUTHOR_H
