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
 * Bonsai is distributed in the hope that it will be useful,                    *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of               *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                 *
 * GNU General Public License for more details.                                 *
 *                                                                              *
 * You should have received a copy of the GNU General Public License            *
 * along with this program. If not, see <http://www.gnu.org/licenses/>.         *
 *                                                                              *
 ********************************************************************************/

// BonsaiViewer09 (TI09) — the rule model of IfcClash_Plus, ported from
// ifcclash_plus/RuleClass.py and booleanrule.py: a RuleFile holds folders
// and rules; a rule takes its elements from selects (IDS facets, or the
// output of another rule), and carries its management configuration
// (exception, criticity, actor, grouping, Must check).
//
// The structures hold configuration only. A run (Orchestrator) keeps its
// results apart, so the same model can be run again or saved meanwhile.

#ifndef BONSAIVIEWER_SERVICES_CLASH_RULEMODEL_H
#define BONSAIVIEWER_SERVICES_CLASH_RULEMODEL_H

#include "../../ports/ifctester/Facets.h"

#include <QJsonObject>
#include <QString>

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace bonsaiviewer::services::clash {

struct RuleCheck;
struct BooleanNode;

// What the viewer adds to a node of the rule file, under the "viewer" key
// (ignored by the Python CLI, so the file stays readable by both).
struct ViewerMeta {
    QString label;
    QString description;
};

// Select / SelectFacet / SelectRule.
struct Select {
    enum class Kind { None, Facet, Rule };
    Kind kind = Kind::None;
    // SelectFacet: the applicability facets, AND-combined in order.
    QString classification_type = "Facet";
    QString classification_name; // the label of a criticity / actor select
    std::vector<bonsaiviewer::ports::ifctester::Facet> applicability;
    // SelectRule: the rule whose output feeds the slot (cascade).
    int action_type = 1;
    QString element_to_pass = "source"; // "source" or "target"
    bool element_state_to_pass = true;  // true: passing elements; false: the complement
    std::shared_ptr<RuleCheck> rule;

    bool empty() const { return kind == Kind::None; }
};

// BooleanLeaf / AndRule / OrRule / NotRule (booleanrule.py).
struct BooleanNode {
    enum class Kind { Leaf, And, Or, Not };
    Kind kind = Kind::Leaf;
    // Leaf: the rule seen as a boolean, by its number of results.
    QString mode = "have_result"; // have_result, have_no_result, have_more, have_less,
                                  // have_more_or_equals, have_less_or_equals, equals
    QString value;                // the number of the quantity modes
    std::shared_ptr<RuleCheck> rule;
    // And / Or: the children; Not: one child.
    std::vector<std::shared_ptr<BooleanNode>> children;
};

// The select_exception of a two-object rule: a relation name or a boolean
// tree evaluated pair by pair.
struct RuleException {
    QString relation; // "Same_IfcSystem", "Same_Building", "Same_Storey", "Same_Space"
    std::shared_ptr<BooleanNode> tree;
    // Spec 09 timing (viewer extension): "after" keeps the excepted results
    // with status false; "before" removes the excepted pairs from the run.
    QString timing = "after";

    bool empty() const { return relation.isEmpty() && !tree; }
};

// select_grouping: a method name, a facet (Property / Attribute...) or a
// SelectFacet.
struct Grouping {
    enum class Kind { None, Name, Facet, SelectFacet };
    Kind kind = Kind::None;
    QString name; // ENTITY, SOURCE, TARGET, CLOSENESS
    std::optional<bonsaiviewer::ports::ifctester::Facet> facet;
    Select select;
};

// AbsoluteOrRelativeChecking (the Must rule).
struct AbsOrRelCheck {
    QString check_type = "Absolute_Number"; // Absolute_Number, Absolute_Quantity, Relative_Number, Relative_Quantity
    QString quantity_pset;
    QString quantity_prop;
    QString operation; // with the SOURCE and TARGET keywords, e.g. "TARGET>=2"
    // Viewer extension: whether the pairs an exception invalidated (timing
    // "after") count in the groups. The Python counts them.
    bool count_excepted = true;
};

struct RuleCheck {
    QString type;            // the rule type of the file ("Above"); the engine runs "Above_triang"
    QString id;              // the node id (uuid), kept through Save / Load
    QString state = "Final"; // Final, Select (Display_* are run as Final)
    // The type-specific parameters, by their catalogue / serialization keys
    // (every key of the rule object the viewer does not model itself, so a
    // rule saved by the CLI keeps its unknown keys).
    QJsonObject params;
    Select source;
    Select target;
    Select context; // "select_context" of the rules with a context slot
    Grouping grouping;
    std::vector<Select> criticity; // SelectFacets with a classification_name
    std::vector<Select> actor;
    RuleException exception;
    std::optional<AbsOrRelCheck> must;
    ViewerMeta viewer;
};

struct RuleFolder;
using RuleNode = std::variant<std::shared_ptr<RuleFolder>, std::shared_ptr<RuleCheck>>;

struct RuleFolder {
    QString id;
    QString activation_case = "ALLTRUE";
    // activation_rule: a SelectFacet (run when it selects something) or a
    // boolean tree (its rules run first, then the tree is evaluated).
    Select activation_select;
    std::shared_ptr<BooleanNode> activation_tree;
    std::vector<RuleNode> contains;
    ViewerMeta viewer;
    bool active = true; // "Active" of the folder Edit window (viewer extension)
};

struct RuleFile {
    QString id;
    std::vector<QString> list_ifc_path;
    QString path_to_save;
    std::vector<RuleNode> contains;
    ViewerMeta viewer;
};

} // namespace bonsaiviewer::services::clash

#endif // BONSAIVIEWER_SERVICES_CLASH_RULEMODEL_H
