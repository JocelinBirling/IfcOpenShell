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

#include "RuleExamples.h"

#include "../../services/clash/RuleCatalogue.h"
#include "../../services/clash/RuleJson.h"

#include <QStringList>

namespace bonsaiviewer::modules::rule {

namespace {

namespace clash = bonsaiviewer::services::clash;
namespace it = bonsaiviewer::ports::ifctester;

using clash::RuleCheck;
using clash::RuleFolder;
using clash::Select;

// An entity facet on one class, or on several (enumeration). The Duplex
// models are IFC2X3: the entity facet does not take the subtypes, so the
// walls are IFCWALL and IFCWALLSTANDARDCASE.
Select classes(const QStringList& names) {
    it::Facet facet = it::Facet::make(it::FacetType::Entity);
    if (names.size() == 1) {
        facet.name = it::Param::text(names.first().toStdString());
    } else {
        it::Restriction restriction;
        for (const QString& name : names) restriction.add("enumeration", name.toStdString());
        facet.name = it::Param::of(restriction);
    }
    Select select;
    select.kind = Select::Kind::Facet;
    select.applicability.push_back(facet);
    return select;
}

// A rule with the catalogue defaults, then the given parameters.
std::shared_ptr<RuleCheck> rule(const QString& type, const QString& label, const QString& description,
                                const QJsonObject& params) {
    auto result = std::make_shared<RuleCheck>();
    result->type = type;
    result->id = clash::json::newId();
    if (const auto* spec = clash::RuleCatalogue::instance().find(type)) {
        for (const clash::ParamSpec& p : spec->params) result->params[p.name] = p.default_value;
    }
    for (auto i = params.begin(); i != params.end(); ++i) result->params[i.key()] = i.value();
    result->viewer.label = label;
    result->viewer.description = description;
    return result;
}

std::shared_ptr<RuleCheck> rule(const QString& type, const QString& label, const QString& description,
                                const QJsonObject& params, Select source, Select target = {}, Select context = {}) {
    auto result = rule(type, label, description, params);
    result->source = std::move(source);
    result->target = std::move(target);
    result->context = std::move(context);
    return result;
}

std::shared_ptr<RuleFolder> folder(const QString& label, const QString& description,
                                   std::vector<clash::RuleNode> contains) {
    auto result = std::make_shared<RuleFolder>();
    result->id = clash::json::newId();
    result->viewer.label = label;
    result->viewer.description = description;
    result->contains = std::move(contains);
    return result;
}

const QStringList kWalls{"IFCWALL", "IFCWALLSTANDARDCASE"};
const QStringList kDuctsAndPipes{"IFCFLOWSEGMENT", "IFCFLOWFITTING"};

std::shared_ptr<RuleFolder> architecture() {
    // The exception: the OBB band in front of the doors is generous, a pair
    // stays a result only if the furniture is really closer than 1 m.
    auto in_front = rule(
        "OBB_Front_And_Back", "6. Furniture in front of the doors (with an exception)",
        "Two-object rule with an exception. OBB Front And Back finds the furniture inside a 2 m band in front of "
        "and behind each door; the exception (Edit Rule > Exception) runs a Clearance of 1 m on each pair: when "
        "it finds nothing (have_no_result), the furniture is farther than 1 m and the pair is set aside. Most "
        "pairs are set aside, one remains.",
        {{"tolerance", 2.0}, {"direction_method", "Wide"}}, classes({"IFCDOOR"}), classes({"IFCFURNISHINGELEMENT"}));
    auto leaf = std::make_shared<clash::BooleanNode>();
    leaf->mode = "have_no_result";
    leaf->rule = rule("Clearance", "Clearance 1 m", {}, {{"clearance", 1.0}});
    in_front->exception.tree = leaf;

    return folder(
        "Architecture", "Load Ifc2x3_Duplex_Architecture.ifc.",
        {rule("ObbHigh", "1. Passage doors (one object)",
              "One-object rule: the doors whose oriented bounding box is between 1.9 m and 2.3 m high.",
              {{"length_min", 1.9}, {"length_max", 2.3}}, classes({"IFCDOOR"})),
         rule("Intersection", "2. Walls through the furniture (Intersection)",
              "Two-object rule: the furniture that intersects a wall by more than 1 cm.", {{"tolerance", 0.01}},
              classes(kWalls), classes({"IFCFURNISHINGELEMENT"})),
         rule("OBB_Above", "3. Furniture on the slabs (OBB Above)",
              "The furniture that stands in the 10 cm band above a slab (source: the slabs, target: the furniture).",
              {{"tolerance", 0.1}}, classes({"IFCSLAB"}), classes({"IFCFURNISHINGELEMENT"})),
         rule("OBB_Below", "4. Doors and windows under the beams (OBB Below)",
              "The doors and windows that reach the 50 cm band below a beam: the lintels.", {{"tolerance", 0.5}},
              classes({"IFCBEAM"}), classes({"IFCDOOR", "IFCWINDOW"})),
         rule("Ray_Check", "5. Doors in view of the windows (Ray Check)",
              "The door / window pairs less than 6 m apart whose centre-to-centre ray crosses no wall (the "
              "context).",
              {{"max_ray_length", 6.0}}, classes({"IFCDOOR"}), classes({"IFCWINDOW"}), classes(kWalls)),
         in_front});
}

std::shared_ptr<RuleFolder> mep() {
    return folder(
        "MEP",
        "Load one of Ifc2x3_Duplex_MEP.ifc, _Mechanical.ifc, _Plumbing.ifc, Ifc2s3_Duplex_Electrical.ifc (or "
        "several).",
        {rule("ObbLength", "7. Long ducts and pipes (one object)",
              "One-object rule: the duct and pipe segments longer than 3 m (MEP, Mechanical, Plumbing).",
              {{"length_min", 3.0}, {"length_max", 1000.0}, {"direction_method", "Wide"}},
              classes({"IFCFLOWSEGMENT"})),
         rule("OBB_Below", "8. Terminals under the ducts and pipes (OBB Below)",
              "The terminals (outlets, lights, sanitary) in the 50 cm band below a duct or pipe segment (MEP, "
              "Plumbing).",
              {{"tolerance", 0.5}}, classes({"IFCFLOWSEGMENT"}), classes({"IFCFLOWTERMINAL"})),
         rule("Ray_Check", "9. Controllers in view of the terminals (Ray Check)",
              "The controller / terminal pairs less than 3 m apart in direct view. With the Architecture model "
              "loaded too, the walls (context) block the rays.",
              {{"max_ray_length", 3.0}}, classes({"IFCFLOWCONTROLLER"}), classes({"IFCFLOWTERMINAL"}),
              classes(kWalls))});
}

std::shared_ptr<RuleFolder> architectureAndMep() {
    // The band above a ceiling also reaches the floor above: the exception
    // sets aside the pairs a slab separates (the Ray Check finds no free ray).
    auto in_ceilings = rule(
        "OBB_Above", "11. Terminals in the ceilings (OBB Above, with an exception)",
        "The terminals of the MEP model in the 50 cm band above a ceiling (IfcCovering) of the Architecture model. "
        "The band also reaches the terminals of the floor above: the exception runs a Ray Check on each pair with "
        "the slabs as context; when a slab blocks the ray (have_no_result), the terminal is on the other side of "
        "the floor and the pair is set aside.",
        {{"tolerance", 0.5}}, classes({"IFCCOVERING"}), classes({"IFCFLOWTERMINAL"}));
    auto no_slab_between = std::make_shared<clash::BooleanNode>();
    no_slab_between->mode = "have_no_result";
    no_slab_between->rule = rule("Ray_Check", "No slab between", {}, {{"max_ray_length", 20.0}});
    no_slab_between->rule->context = classes({"IFCSLAB"});
    in_ceilings->exception.tree = no_slab_between;

    return folder(
        "Architecture + MEP", "Load Ifc2x3_Duplex_Architecture.ifc and Ifc2x3_Duplex_MEP.ifc together.",
        {rule("Intersection", "10. Ducts and pipes through the architecture (Intersection)",
              "A clash between two models: the duct and pipe segments and fittings that intersect a beam, a door, a "
              "window or a piece of furniture.",
              {{"tolerance", 0.01}}, classes(kDuctsAndPipes),
              classes({"IFCBEAM", "IFCDOOR", "IFCWINDOW", "IFCFURNISHINGELEMENT"})),
         in_ceilings});
}

std::shared_ptr<RuleFolder> spaces() {
    // The Must: the pairs (space, element) grouped by their space; a space
    // that intersects no element fails.
    auto must = rule("Intersection", "12. Every space is furnished or equipped (Must)",
                     "A Must check on the spaces. The Intersection finds the furniture, terminals and equipment "
                     "inside each space (source: the spaces, target: the elements). The Grouping set is the spaces "
                     "(Edit Rule > Grouping): each space holds the elements it intersects. The Must (TARGET >= 1) "
                     "gives one result per space: the spaces with none (hallways, stairs...) fail, shown in red. "
                     "The (space, element) pairs are under the Set aside toggle of the Result panel. Works on the Architecture (furniture), MEP and "
                     "Mechanical (terminals, equipment) models, alone or together; the Plumbing and Electrical "
                     "models only have the roof space: load them with the Architecture model.",
                     {{"tolerance", 0.01}}, classes({"IFCSPACE"}),
                     classes({"IFCFURNISHINGELEMENT", "IFCFLOWTERMINAL", "IFCENERGYCONVERSIONDEVICE"}));
    must->grouping.kind = clash::Grouping::Kind::SelectFacet;
    must->grouping.select = classes({"IFCSPACE"});
    clash::AbsOrRelCheck check;
    check.check_type = "Absolute_Number";
    check.operation = "TARGET>=1";
    must->must = check;
    return folder("Spaces", "Load one or several Duplex models.", {must});
}

} // namespace

services::clash::RuleFile exampleRuleFile() {
    clash::RuleFile file;
    file.id = clash::json::newId();
    file.viewer.label = QStringLiteral("Rule File");
    file.contains.push_back(folder(
        "Examples",
        "Example rules for the Duplex sample models (IfcSampleFiles). Load the models a sub-folder names, then "
        "Run the sub-folder or a rule (right click).",
        {architecture(), mep(), architectureAndMep(), spaces()}));
    return file;
}

} // namespace bonsaiviewer::modules::rule
