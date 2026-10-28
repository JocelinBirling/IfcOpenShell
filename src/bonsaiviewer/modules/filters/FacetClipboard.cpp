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

#include "FacetClipboard.h"

#include "../../services/IdsEngine.h"

#include <QApplication>
#include <QClipboard>
#include <QMessageBox>

namespace bonsaiviewer::modules::filters::facet_clipboard {

namespace engine = bonsaiviewer::services::ids_engine;
using bonsaiviewer::services::IdsFacet;

bool copy(QWidget* parent, const std::vector<IdsFacet>& facets) {
    if (facets.empty()) return false;
    try {
        QApplication::clipboard()->setText(engine::facetsToClipboard(facets));
        return true;
    } catch (const std::exception& e) {
        QMessageBox::warning(parent, "Copy facets", QString("Cannot copy: %1.").arg(e.what()));
        return false;
    }
}

std::vector<IdsFacet> paste(QWidget* parent) {
    try {
        return engine::facetsFromClipboard(QApplication::clipboard()->text());
    } catch (const std::exception& e) {
        QMessageBox::information(parent, "Paste facets", QString("Nothing to paste: %1.").arg(e.what()));
        return {};
    }
}

bool canPaste() {
    const QString text = QApplication::clipboard()->text().trimmed();
    if (!text.startsWith('<') && !text.startsWith("<?xml")) return false;
    for (const char* tag : {"<facets", "<ids", "<entity", "<property", "<attribute", "<classification",
                            "<material", "<partOf"}) {
        if (text.contains(QLatin1String(tag))) return true;
    }
    return false;
}

} // namespace bonsaiviewer::modules::filters::facet_clipboard
