// This file was generated with the assistance of an AI coding tool.
/********************************************************************************
 *                                                                              *
 * This file is part of Bonsai.                                                 *
 *                                                                              *
 * Bonsai is free software: you can redistribute it and/or modify               *
 * it under the terms of the GNU General Public License as published by         *
 * the Free Software Foundation, either version 3.0 of the License, or          *
 * (at your option) any later version.                                          *
 *                                                                              *
 ********************************************************************************/

#ifndef IFCINTERFACE_MODULES_RULE_RULEEXAMPLES_H
#define IFCINTERFACE_MODULES_RULE_RULEEXAMPLES_H

#include "../../services/clash/RuleModel.h"

namespace bonsaiviewer::modules::rule {

// BonsaiViewer09 — the example rule file opened at start, while the Rules
// panel holds no rule: an "Examples" folder whose rules give results on
// the Duplex sample models (Ifc2x3_Duplex_Architecture, _MEP, _Mechanical,
// _Plumbing, Electrical). One sub-folder per set of models to load.
services::clash::RuleFile exampleRuleFile();

} // namespace bonsaiviewer::modules::rule

#endif // IFCINTERFACE_MODULES_RULE_RULEEXAMPLES_H
