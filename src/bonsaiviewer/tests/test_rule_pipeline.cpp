// This file was generated with the assistance of an AI coding tool.
// The pipeline of the rules (services/clash/Orchestrator): exceptions and
// Must checks, run on the Duplex Architecture of tests/data — target
// bonsaiviewer_rule_pipeline_tests.
//
// The cases start from Collision(doors x slabs): 24 pairs, every door on
// the slabs of its own storey; and Clearance(doors x slabs, 1 m): 66 pairs,
// 46 on the same storey, the others with the slab of the storey above or
// below. The expected counts were checked on the model with IfcOpenShell
// (storeys, properties, the pairs of each door).
#include "services/clash/Orchestrator.h"
#include "services/clash/RuleJson.h"

#include "api/Engine.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QString>

#include <filesystem>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace clash = bonsaiviewer::services::clash;

namespace {

int failures = 0, checks = 0;

void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) {
        ++failures;
        std::cerr << "FAILED: " << what << "\n";
    }
}

struct Outcome {
    std::string error;
    size_t results = 0;   // kept pairs, or Must verdicts
    size_t set_aside = 0; // excepted pairs, or the pairs a Must gathers
    size_t groups = 0;
    size_t failing = 0;   // Must verdicts with status false
    std::set<std::string> failing_labels;
};

ifcclash::Session& session() {
    static ifcclash::SessionOptions options = [] {
        ifcclash::SessionOptions o;
        o.geometry_cache = ifcclash::CachePolicy::Off;
        return o;
    }();
    static const std::filesystem::path duplex =
        std::filesystem::path(BONSAIVIEWER_TEST_DATA_DIR) / "Ifc2x3_Duplex_Architecture.ifc";
    static ifcclash::Session instance({duplex}, options);
    return instance;
}

Outcome run(const QJsonObject& rule_json) {
    Outcome outcome;
    ifcclash::CancelToken cancel;
    clash::Orchestrator::Callbacks callbacks;
    callbacks.finished = [&outcome](const clash::RuleOutcome& o) {
        outcome.error = o.error.toStdString();
        outcome.results = o.results.size();
        outcome.set_aside = o.set_aside.size();
        outcome.groups = o.groups.size();
        for (size_t g = 0; g < o.groups.size() && g < o.results.size(); ++g) {
            if (o.results[g].complex && !o.results[g].status) {
                ++outcome.failing;
                outcome.failing_labels.insert(o.groups[g].label.toStdString());
            }
        }
    };
    clash::Orchestrator orchestrator(session(), 1, callbacks, cancel);
    orchestrator.run({clash::json::ruleFromJson(rule_json)});
    return outcome;
}

QJsonObject entity(const char* name) {
    return {{"type", "SelectFacet"}, {"applicability", QJsonArray{QJsonObject{{"type", "Entity"}, {"name", name}}}}};
}

// Collision(doors x slabs): 24 pairs, 10 doors on two slabs, 4 on one.
QJsonObject doorsOnSlabs() {
    return {{"type", "Collision"},
            {"id", "doors-slabs"},
            {"allow_touching", false},
            {"select_source", entity("IFCDOOR")},
            {"select_target", entity("IFCSLAB")}};
}

// Clearance(doors x slabs, 1 m): 66 pairs, 46 on the door's storey; each
// door has 1 to 3 pairs with another storey (10 doors only one).
QJsonObject doorsNearSlabs() {
    return {{"type", "Clearance"},
            {"id", "doors-near-slabs"},
            {"clearance", 1.0},
            {"select_source", entity("IFCDOOR")},
            {"select_target", entity("IFCSLAB")}};
}

QJsonObject withException(QJsonObject rule, const QJsonValue& exception, const QString& timing = "after") {
    rule["select_exception"] = exception;
    if (timing != QLatin1String("after")) rule["viewer"] = QJsonObject{{"exception_timing", timing}};
    return rule;
}

// The passage doors (one object, 1.9 to 2.3 m high), as a leaf: holds for
// a pair whose door is one.
QJsonObject passageDoor(const char* mode = "have_result", const QJsonValue& value = QJsonValue()) {
    return {{"type", "BooleanLeaf"},
            {"mode", mode},
            {"value", value},
            {"rule", QJsonObject{{"type", "ObbHigh"}, {"length_min", 1.9}, {"length_max", 2.3}}}};
}

QJsonObject notNode(const QJsonObject& child) {
    return {{"type", "NotRule"}, {"child", child}};
}

QJsonObject both(const char* type, const QJsonObject& a, const QJsonObject& b) {
    return {{"type", type}, {"children", QJsonArray{a, b}}};
}

QJsonObject withMust(QJsonObject rule, const QJsonValue& grouping, const QString& check_type, const QString& operation,
                     const QString& pset = {}, const QString& prop = {}) {
    rule["select_grouping"] = grouping;
    rule["abs_or_rel_check"] = QJsonObject{{"class", "AbsoluteOrRelativeChecking"},
                                           {"check_type", check_type},
                                           {"quantity_pset", pset.isEmpty() ? QJsonValue() : QJsonValue(pset)},
                                           {"quantity_prop", prop.isEmpty() ? QJsonValue() : QJsonValue(prop)},
                                           {"operation", operation}};
    return rule;
}

void testOperations() {
    using clash::evaluateOperation;
    check(evaluateOperation("TARGET>=2", 0, 2) && !evaluateOperation("TARGET>=2", 5, 1), "TARGET>=2");
    check(evaluateOperation("SOURCE=1", 1, 0) && evaluateOperation("SOURCE==1", 1, 0), "= is ==");
    check(evaluateOperation("TARGET>=SOURCE*2", 2, 4) && !evaluateOperation("TARGET>=SOURCE*2", 2, 3),
          "relative: TARGET >= SOURCE * 2");
    check(evaluateOperation("SOURCE+TARGET<5", 2, 2) && !evaluateOperation("SOURCE+TARGET<5", 2, 3), "a sum");
    check(evaluateOperation("SOURCE>0 and TARGET>0", 1, 1) && !evaluateOperation("SOURCE>0 and TARGET>0", 1, 0), "and");
    check(evaluateOperation("SOURCE>0 or TARGET>0", 0, 1) && !evaluateOperation("SOURCE>0 or TARGET>0", 0, 0), "or");
    check(evaluateOperation("not TARGET<1", 0, 1) && !evaluateOperation("not TARGET<1", 0, 0), "not");
    check(evaluateOperation("SOURCE!=3", 2, 0) && !evaluateOperation("SOURCE!=3", 3, 0), "!=");
    bool thrown = false;
    try {
        evaluateOperation("TARGET>=", 0, 0);
    } catch (const std::runtime_error&) {
        thrown = true;
    }
    check(thrown, "a malformed operation throws");
}

} // namespace

int main() {
    testOperations();

    const Outcome base = run(doorsOnSlabs());
    check(base.error.empty() && base.results == 24 && base.set_aside == 0, "Collision(doors x slabs): 24 pairs");
    const Outcome near = run(doorsNearSlabs());
    check(near.error.empty() && near.results == 66 && near.set_aside == 0, "Clearance(doors x slabs, 1 m): 66 pairs");

    // --- exceptions by relation: the excepted pairs are set aside --------------------
    const Outcome building = run(withException(doorsOnSlabs(), "Same_Building"));
    check(building.results == 0 && building.set_aside == 24, "Same_Building: one building, every pair excepted");
    const Outcome storey = run(withException(doorsNearSlabs(), "Same_Storey"));
    check(storey.results == 20 && storey.set_aside == 46, "Same_Storey: the 46 pairs of one storey excepted");
    const Outcome storey_before = run(withException(doorsNearSlabs(), "Same_Storey", "before"));
    check(storey_before.results == 20 && storey_before.set_aside == 46, "Same_Storey before: the same pairs");
    const Outcome space = run(withException(doorsOnSlabs(), "Same_Space"));
    check(space.error.empty() && space.results == 24 && space.set_aside == 0,
          "Same_Space: the doors and slabs are in no space, nothing excepted");

    // --- exceptions by a boolean tree, evaluated pair by pair -----------------------
    // 12 passage doors (1.9 to 2.3 m) of 14: the two glass doors (2.42 m)
    // are not, each on two slabs: 4 pairs.
    const Outcome leaf = run(withException(doorsOnSlabs(), passageDoor()));
    check(leaf.results == 4 && leaf.set_aside == 20, "leaf have_result: the pairs of the passage doors excepted");
    const Outcome negation = run(withException(doorsOnSlabs(), notNode(passageDoor())));
    check(negation.results == 20 && negation.set_aside == 4, "NOT: the complement");
    const Outcome less = run(withException(doorsOnSlabs(), passageDoor("have_less", "1")));
    check(less.results == 20 && less.set_aside == 4, "have_less 1: as NOT have_result");
    const Outcome either = run(withException(doorsOnSlabs(), both("OrRule", passageDoor(), notNode(passageDoor()))));
    check(either.results == 0 && either.set_aside == 24, "A OR NOT A: every pair excepted");
    const Outcome neither = run(withException(doorsOnSlabs(), both("AndRule", passageDoor(), notNode(passageDoor()))));
    check(neither.results == 24 && neither.set_aside == 0, "A AND NOT A: no pair excepted");

    // --- Must: one verdict per group, the pairs set aside ----------------------------
    const Outcome by_entity = run(withMust(doorsOnSlabs(), "ENTITY", "Absolute_Number", "SOURCE>=1"));
    check(by_entity.results == 2 && by_entity.groups == 2 && by_entity.set_aside == 24 &&
              by_entity.failing_labels == std::set<std::string>{"IfcSlab"},
          "grouping ENTITY: IfcDoor passes, IfcSlab (no source) fails");
    const Outcome by_door = run(withMust(doorsOnSlabs(), "SOURCE", "Absolute_Number", "TARGET>=2"));
    check(by_door.results == 14 && by_door.groups == 14 && by_door.failing == 4,
          "grouping SOURCE: one verdict per door, the 4 doors on one slab fail");
    const Outcome relative = run(withMust(doorsOnSlabs(), "SOURCE", "Relative_Number", "TARGET>=SOURCE*2"));
    check(relative.results == 14 && relative.failing == 4, "relative: TARGET >= SOURCE * 2");
    const Outcome quantity =
        run(withMust(doorsOnSlabs(), "SOURCE", "Absolute_Quantity", "SOURCE>2.1", "PSet_Revit_Other", "Head Height"));
    check(quantity.results == 14 && quantity.failing == 12,
          "quantity: the 12 doors with a head height of 2.01 / 2.032 m fail");
    const QJsonObject reference{{"type", "Property"}, {"property_set", "Pset_DoorCommon"}, {"base_name", "Reference"}};
    const Outcome by_reference = run(withMust(doorsOnSlabs(), reference, "Absolute_Number", "SOURCE>=1"));
    check(by_reference.results == 5 && by_reference.failing_labels == std::set<std::string>{"None"},
          "grouping by property: 4 door types pass, the slabs (None) fail");

    // The pairs an exception sets aside: counted in the groups by default
    // (as the Python), not when "count excepted" is off or the exception
    // runs before. TARGET >= 2: 10 doors have one pair on another storey.
    QJsonObject counted =
        withMust(withException(doorsNearSlabs(), "Same_Storey"), "SOURCE", "Absolute_Number", "TARGET>=2");
    const Outcome count_excepted = run(counted);
    check(count_excepted.results == 14 && count_excepted.failing == 0, "the excepted pairs counted: every door passes");
    counted["viewer"] = QJsonObject{{"must_count_excepted", false}};
    const Outcome not_counted = run(counted);
    check(not_counted.results == 14 && not_counted.failing == 10, "not counted: the 10 doors left with one pair fail");
    const Outcome before = run(withMust(withException(doorsNearSlabs(), "Same_Storey", "before"), "SOURCE",
                                        "Absolute_Number", "TARGET>=2"));
    check(before.results == 14 && before.failing == 10, "exception before: the excepted pairs are not counted");
    for (const Outcome* o : {&building, &storey, &leaf, &by_entity, &by_door, &quantity, &count_excepted, &before}) {
        check(o->error.empty(), "runs without error: " + o->error);
    }

    std::cout << checks - failures << "/" << checks << " checks passed\n";
    return failures == 0 ? 0 : 1;
}
