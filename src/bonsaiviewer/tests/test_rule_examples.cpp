// This file was generated with the assistance of an AI coding tool.
// The example rules of the Rules panel (modules/rule/RuleExamples.cpp) run
// through the viewer's Orchestrator on the Duplex Architecture of
// tests/data — target bonsaiviewer_rule_example_tests.
//
// With IFC paths as arguments, runs every example on those models instead,
// with the viewer's options (.ifcview read), and prints the counts (no
// check): to try the MEP examples on the other Duplex models.
#include "modules/rule/RuleExamples.h"
#include "services/clash/Orchestrator.h"
#include "services/clash/RuleJson.h"

#include "api/Engine.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QString>

#include <filesystem>
#include <iostream>
#include <map>
#include <set>
#include <string>
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

struct Count {
    size_t results = 0;
    size_t set_aside = 0;
    size_t failing_groups = 0; // Must verdicts (results) with status false
    std::set<int> failing_elements; // their grouping-set elements (entity ids)
    size_t groups = 0;
    std::string error;
};

// as_viewer: the options of the viewer (ClashView::startWork): the .ifcview
// next to an IFC is read when there is one.
std::map<std::string, Count> runNodes(const std::vector<std::filesystem::path>& models,
                                      const std::vector<clash::RuleNode>& nodes, bool as_viewer = false) {
    ifcclash::SessionOptions options;
    if (!as_viewer) options.geometry_cache = ifcclash::CachePolicy::Off;
    ifcclash::Session session(models, options);
    ifcclash::CancelToken cancel;
    std::map<std::string, Count> counts;
    clash::Orchestrator::Callbacks callbacks;
    callbacks.finished = [&counts](const clash::RuleOutcome& outcome) {
        if (outcome.rule_type.isEmpty()) return; // a skipped folder
        Count& count = counts[outcome.label.toStdString()];
        count.results = outcome.results.size();
        count.set_aside = outcome.set_aside.size();
        count.groups = outcome.groups.size();
        for (const auto& r : outcome.results) {
            if (!r.complex || r.status) continue;
            ++count.failing_groups;
            if (r.group_element) count.failing_elements.insert(r.group_element->entity_id);
        }
        count.error = outcome.error.toStdString();
    };
    clash::Orchestrator orchestrator(session, models.size(), callbacks, cancel);
    orchestrator.run(nodes);
    return counts;
}

std::map<std::string, Count> runExamples(const std::vector<std::filesystem::path>& models, bool as_viewer = false) {
    return runNodes(models, bonsaiviewer::modules::rule::exampleRuleFile().contains, as_viewer);
}

QJsonObject entity(const char* name) {
    return {{"type", "SelectFacet"},
            {"applicability", QJsonArray{QJsonObject{{"type", "Entity"}, {"name", name}}}}};
}

// The furniture of Level 2 stands in the 50 cm band above the Level 1
// ceilings; a Ray Check exception with the slabs as context sets those
// pairs aside. With the doors as context nothing blocks the rays.
Count ceilingsWithException(const std::filesystem::path& model, const char* context) {
    const QJsonObject leaf{{"type", "BooleanLeaf"},
                           {"mode", "have_no_result"},
                           {"rule", QJsonObject{{"type", "Ray_Check"},
                                                {"max_ray_length", 20.0},
                                                {"select_context", entity(context)}}}};
    const auto rule = clash::json::ruleFromJson(QJsonObject{{"type", "OBB_Above"},
                                                            {"id", "ceilings"},
                                                            {"tolerance", 0.5},
                                                            {"select_source", entity("IFCCOVERING")},
                                                            {"select_target", entity("IFCFURNISHINGELEMENT")},
                                                            {"select_exception", leaf}});
    const auto counts = runNodes({model}, {rule});
    return counts.empty() ? Count{} : counts.begin()->second;
}

void print(const std::map<std::string, Count>& counts) {
    for (const auto& [label, count] : counts) {
        std::cout << label << ": " << count.results << " results, " << count.set_aside << " set aside";
        if (count.groups) std::cout << ", " << count.groups << " groups (" << count.failing_groups << " failing)";
        if (!count.error.empty()) std::cout << " — error: " << count.error;
        std::cout << "\n";
    }
}

} // namespace

int main(int argc, char** argv) {
    if (argc > 1) {
        std::vector<std::filesystem::path> models(argv + 1, argv + argc);
        print(runExamples(models, true));
        return 0;
    }

    const auto duplex = std::filesystem::path(BONSAIVIEWER_TEST_DATA_DIR) / "Ifc2x3_Duplex_Architecture.ifc";
    const auto counts = runExamples({duplex});
    print(counts);

    check(counts.size() == 12, "every example rule reports an outcome");
    for (const auto& [label, count] : counts) check(count.error.empty(), label + " runs without error");

    const auto results = [&counts](const char* label) {
        const auto found = counts.find(label);
        return found == counts.end() ? Count{} : found->second;
    };
    // The Architecture examples give results on the Architecture model.
    check(results("1. Passage doors (one object)").results == 12, "12 passage doors");
    check(results("2. Walls through the furniture (Intersection)").results == 1, "1 wall through the furniture");
    check(results("3. Furniture on the slabs (OBB Above)").results > 0, "furniture on the slabs");
    check(results("4. Doors and windows under the beams (OBB Below)").results > 0, "doors and windows under beams");
    check(results("5. Doors in view of the windows (Ray Check)").results > 0, "doors in view of the windows");
    const Count in_front = results("6. Furniture in front of the doors (with an exception)");
    check(in_front.results == 1 && in_front.set_aside == 10, "the exception keeps 1 pair of 11, sets 10 aside");
    // The Must: one group per space of the model (21), some fail.
    const Count must = results("12. Every space is furnished or equipped (Must)");
    check(must.results == 21 && must.groups == 21, "one Must verdict per space: the results");
    check(must.set_aside == 66, "the 66 (space, furniture) intersections set aside");
    // The failing verdicts are the spaces no furniture intersects: hallways,
    // B104, utilities, stair, B105, roof. The foyers pass: the kitchen
    // counter tops reach into them.
    check(must.failing_elements == std::set<int>{819, 1928, 2412, 3197, 3325, 3456, 3586, 3707},
          "the 8 failing verdicts carry the spaces without furniture");
    // An exception whose rule has a context (Ray Check): the context is used.
    const Count blocked = ceilingsWithException(duplex, "IFCSLAB");
    const Count free = ceilingsWithException(duplex, "IFCDOOR");
    check(blocked.error.empty() && free.error.empty(), "the Ray Check exception runs");
    check(free.results > 0 && free.set_aside == 0, "no slab as context: every pair kept");
    check(blocked.set_aside > 0 && blocked.results + blocked.set_aside == free.results,
          "the slabs as context: the pairs across a slab set aside");
    // The MEP examples find nothing on the Architecture model, without error.
    check(results("7. Long ducts and pipes (one object)").results == 0, "no duct in the Architecture model");

    std::cout << checks - failures << "/" << checks << " checks passed\n";
    return failures == 0 ? 0 : 1;
}
