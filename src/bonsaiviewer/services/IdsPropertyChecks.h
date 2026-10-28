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

#ifndef IFCINTERFACE_SERVICES_IDSPROPERTYCHECKS_H
#define IFCINTERFACE_SERVICES_IDSPROPERTYCHECKS_H

#include "../ports/ifctester/Facets.h"

#include <QObject>
#include <QString>

#include <cstdint>
#include <map>
#include <utility>
#include <vector>

// B24 — "Color Properties" and "Filter Properties" of the IDS panel: the
// property and attribute requirements of the last IDS run, element by
// element, for the Properties panel (BonsaiViewer14) to colour its rows
// (green when the element meets the requirement, red when it fails it)
// and / or to show only the rows the IDS checks.

namespace bonsaiviewer::services {

class IdsPropertyChecks : public QObject {
    Q_OBJECT

public:
    struct Check {
        bonsaiviewer::ports::ifctester::Facet facet; // a property or attribute requirement
        bool passed = true;
        QString text; // "specification: requirement in words", and the failure reason
    };
    // (session model id, EXPRESS id) -> the checks of that element.
    using Checks = std::map<std::pair<std::uint32_t, int>, std::vector<Check>>;

    explicit IdsPropertyChecks(QObject* parent = nullptr) : QObject(parent) {}

    void setChecks(Checks checks, bool colour, bool filter) {
        checks_ = std::move(checks);
        colour_ = colour;
        filter_ = filter;
        emit changed();
    }
    void clear() {
        if (checks_.empty() && !colour_ && !filter_) return;
        checks_.clear();
        colour_ = filter_ = false;
        emit changed();
    }
    bool colour() const { return colour_; }
    bool filter() const { return filter_; }
    // Null when the element has no check.
    const std::vector<Check>* checksOf(std::uint32_t session_model_id, int ifc_id) const {
        const auto found = checks_.find({session_model_id, ifc_id});
        return found == checks_.end() ? nullptr : &found->second;
    }

signals:
    void changed();

private:
    Checks checks_;
    bool colour_ = false; // Color Properties
    bool filter_ = false; // Filter Properties
};

} // namespace bonsaiviewer::services

#endif // IFCINTERFACE_SERVICES_IDSPROPERTYCHECKS_H
