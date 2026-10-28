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

#ifndef IFCINTERFACE_MODULES_IDS_IDSDOCUMENT_H
#define IFCINTERFACE_MODULES_IDS_IDSDOCUMENT_H

#include "../../services/IdsBridge.h"

#include "../../ports/ifctester/Facets.h"

#include <QByteArray>
#include <QList>
#include <QPair>
#include <QString>
#include <QStringList>

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace bonsaiviewer::ports::ifctester { struct Ids; }

namespace bonsaiviewer::modules::ids {

// BonsaiViewer24 — the IDS document as edited by the IDS panel. The facets
// are the editor's (IdsFacet, shared with the filters and the tables); the
// IfcTester port (ports/ifctester) reads, writes and runs the document.

struct SpecificationEdit {
    QString name = "Unnamed";
    QString identifier;
    QString description;
    QString instructions;
    QStringList ifc_versions = {"IFC2X3", "IFC4", "IFC4X3_ADD2"};
    QString usage = "required"; // required / optional / prohibited (applicability cardinality)
    std::vector<bonsaiviewer::services::IdsFacet> applicability;
    std::vector<bonsaiviewer::services::IdsFacet> requirements;

    // The facets as read from the file. The editor normalises what it shows
    // (class and PredefinedType case, one pattern per field...), so while
    // the facets are not edited, toIds() writes these back unchanged: an IDS
    // that is only opened, run or saved keeps its exact meaning. A
    // specification the editor cannot show (e.g. a totalDigits restriction)
    // keeps them for good and its facets are read-only.
    struct Original {
        std::vector<bonsaiviewer::ports::ifctester::Facet> applicability;
        std::vector<bonsaiviewer::ports::ifctester::Facet> requirements;
        QByteArray signature; // the editor facets right after reading
        bool editable = true;
        QString reason;       // why it is not editable
    };
    std::shared_ptr<const Original> original;

    // The editor facets as JSON, to tell an edit from the loaded state.
    QByteArray facetsSignature() const;
    bool facetsEditable() const { return !original || original->editable; }
};

struct IdsDocument {
    // title, copyright, version, description, author, date, purpose,
    // milestone: only the ones set.
    QList<QPair<QString, QString>> info = {{"title", "Untitled"}};
    std::vector<SpecificationEdit> specifications;

    static const QStringList& infoKeys();
    QString infoValue(const QString& key) const;
    void setInfo(const QString& key, const QString& value);

    // Throw ids_engine::UserError with a message for the user.
    static IdsDocument fromXml(const QByteArray& xml);
    QByteArray toXml() const;
    bonsaiviewer::ports::ifctester::Ids toIds() const;
};

// One element of a loaded model.
struct ElementRef {
    std::uint32_t session_model_id = 0;
    int ifc_id = 0;
};

struct FailureRow {
    ElementRef element;
    QString label;  // "model — #12 IfcWall 'Name'"
    QString reason;
};

struct RequirementResult {
    QString text;   // the requirement in words (Facet.to_string)
    std::optional<bonsaiviewer::ports::ifctester::Facet> facet; // the requirement itself
    bool status = true;
    int passed = 0;
    std::vector<FailureRow> failures;
};

// The result of one specification on all the loaded models.
struct SpecificationResult {
    QString name;
    bool status = true;
    QString note; // why the status is what it is, when not obvious
    std::vector<ElementRef> applicable;
    std::vector<ElementRef> passed;
    std::vector<ElementRef> failed;
    int models_checked = 0;
    int models_other_schema = 0;
    std::vector<RequirementResult> requirements;
};

} // namespace bonsaiviewer::modules::ids

#endif // IFCINTERFACE_MODULES_IDS_IDSDOCUMENT_H
