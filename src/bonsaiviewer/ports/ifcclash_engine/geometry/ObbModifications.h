// This file was generated with the assistance of an AI coding tool.
// IfcClash_Plus engine (BonsaiViewer copy) — the list_of_modifications of
// OBB_Custom / OBB_Custom_triang: one strict parser, shared by both rules,
// and the zones it builds from the OBB of the source.
#pragma once

#include "Obb.h"

#include <Bnd_OBB.hxx>

#include <string>
#include <vector>

namespace ifcclash::geometry {

enum class ObbOp { ExpandSides, DetachTop, DetachBottom, ExtendUp, ExtendDown, NewObb };

// One line of the list: an operation and its value (none for NewObb).
struct ObbStep {
    ObbOp op = ObbOp::ExtendUp;
    ObbChange change;
};

// The operation names, in the catalogue order: expand_sides,
// detach_top_by_extrude, detach_bottom_by_extrude, extend_up, extend_down,
// NEW_OBB.
const std::vector<std::string>& obb_op_names();
const std::string& obb_op_name(ObbOp op);

// One line, "name:value" or "NEW_OBB": the name is compared exactly (spaces
// around trimmed); the value is a number with a decimal point, optionally
// followed by '%' (then kept as a ratio: 50% -> 0.5); negative values are
// allowed (they shrink a zone). `line` (1-based) only feeds the message.
// Throws ConfigError "line N: ...".
ObbStep parse_obb_step(const std::string& text, size_t line);

// The whole list: every line parsed, then the structure checked (NEW_OBB
// separates two zones: never first, last, or twice in a row). An empty list
// is valid: one zone, the OBB of the source. Throws ConfigError
// "<rule>: line N: ...".
std::vector<ObbStep> parse_obb_modifications(const std::vector<std::string>& lines, const std::string& rule);

// The zones of the parsed list, from the OBB of the source: the steps apply
// in order to the current zone; NEW_OBB closes it and starts the next one
// from the source OBB again.
std::vector<Bnd_OBB> obb_zones(const Bnd_OBB& base, const std::vector<ObbStep>& steps);

// The documentation of list_of_modifications (the catalogue doc of the
// parameter, shown as its tooltip), shared by OBB_Custom and
// OBB_Custom_triang; one line per '\n'.
std::string obb_modifications_doc();

}  // namespace ifcclash::geometry
