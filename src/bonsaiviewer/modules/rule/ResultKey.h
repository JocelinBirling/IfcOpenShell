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

#ifndef IFCINTERFACE_MODULES_RULE_RESULTKEY_H
#define IFCINTERFACE_MODULES_RULE_RESULTKEY_H

#include <QString>

#include <vector>

namespace bonsaiviewer::modules::rule {

// An element of a result as it is saved: the federation's model id (stable,
// unlike the session model ids), the GlobalId, the STEP id.
struct ResultElement {
    QString model_id;
    QString global_id;
    int step_id = 0;

    bool operator==(const ResultElement& other) const {
        return model_id == other.model_id && global_id == other.global_id && step_id == other.step_id;
    }
};

// The key of a result, the same from one run to the next and after a reload
// (the engine's result ids are new at each run):
//   one or two objects: "<rule id>|<model>:<GlobalId>|<model>:<GlobalId>" (no
//   target: empty last part);
//   a Must group (complex): "<rule id>|<group>|<sha1 of the sources>|<sha1 of
//   the targets>", the elements sorted first.
// An element without GlobalId is written "<model>:#<STEP id>".
QString resultKey(const QString& rule_id, bool complex, const QString& group,
                  const std::vector<ResultElement>& sources, const std::vector<ResultElement>& targets);

} // namespace bonsaiviewer::modules::rule

#endif // IFCINTERFACE_MODULES_RULE_RESULTKEY_H
