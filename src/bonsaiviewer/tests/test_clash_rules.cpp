// This file was generated with the assistance of an AI coding tool.
// Unit tests of the clash rules on the Duplex model, ported from the
// IfcClash_Plus Python suite (tests/test_Rule_OneObject.py and
// tests/test_Rule_TwoObjects.py, which run the same rules on
// Ifc2x3_Duplex_Architecture.ifc) — target bonsaiviewer_clash_rule_tests.
//
// Each case keeps the Python expected count; a rule that runs through the
// C++ engine must flag the same number of results.
#include "ports/ifcclash_engine/api/Engine.h"

#include "../ifcparse/file.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

using namespace ifcclash;

namespace {

int failures = 0, checks = 0;

void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) {
        ++failures;
        std::cerr << "FAILED: " << what << "\n";
    }
}

const std::filesystem::path kDuplex =
    std::filesystem::path(BONSAIVIEWER_TEST_DATA_DIR) / "Ifc2x3_Duplex_Architecture.ifc";

// All the entity ids of an IFC class in the session's file.
std::vector<int> idsOf(Session& session, const char* ifc_class) {
    std::vector<int> ids;
    if (ifcopenshell::file* file = session.ifc_file(0)) {
        for (const auto& instance : file->instances_by_type(ifc_class)) ids.push_back(instance.id());
    }
    return ids;
}

FileSelection select(Session& session, const char* ifc_class) {
    return FileSelection{0, {}, idsOf(session, ifc_class)};
}

SessionOptions quietOptions() {
    SessionOptions o;
    o.geometry_cache = CachePolicy::Off;  // convert from the IFC, no .ifcview probe
    return o;
}

// Runs one rule and returns the number of flagged results.
size_t runCount(Session& session, const std::string& rule, const Json& params,
                const FileSelection& source, const FileSelection& target = {},
                const FileSelection& context = {}) {
    RunRequest r;
    r.rule_type = rule;
    r.params = params;
    r.selections.source = {source};
    if (!target.entity_ids.empty()) r.selections.target = {target};
    if (!context.entity_ids.empty()) r.selections.context = {context};
    return session.run(r).results.size();
}

}  // namespace

int main() {
    if (!std::filesystem::exists(kDuplex)) {
        std::cerr << "missing test model: " << kDuplex << "\n";
        return 1;
    }
    Session session({kDuplex}, quietOptions());
    check(session.file_count() == 1, "the session opens the Duplex");

    const FileSelection walls = select(session, "IfcWallStandardCase");
    const FileSelection furniture = select(session, "IfcFurnishingElement");
    const FileSelection doors = select(session, "IfcDoor");
    const FileSelection slabs = select(session, "IfcSlab");
    check(!walls.entity_ids.empty() && !furniture.entity_ids.empty() && !doors.entity_ids.empty() &&
              !slabs.entity_ids.empty(),
          "the Duplex classes resolve to entities");

    // --- one-object rules (test_Rule_OneObject.py) -------------------------
    check(runCount(session, "Volume", Json{{"volume_min", 16.0}, {"volume_max", 17.0}}, walls) == 2,
          "Volume(walls, 16..17) == 2  [test_volume_rule]");
    check(runCount(session, "Area", Json{{"volume_min", 0.0}, {"volume_max", 1.0}}, walls) == 8,
          "Area(walls, 0..1) == 8  [test_area_rule]");
    check(runCount(session, "TopOrBottomSurface",
                   Json{{"surface_min", 5.0}, {"surface_max", 10.0}, {"top_or_bot_method", "Top"}}, walls) == 8,
          "TopOrBottomSurface(walls, Top, 5..10) == 8  [test_top_surface_rule]");

    // --- two-object rules (test_Rule_TwoObjects.py) -------------------------
    check(runCount(session, "Intersection", Json{{"tolerance", 0.01}}, walls, furniture) == 1,
          "Intersection(walls x furniture, 0.01) == 1  [test_intersection_rule]");
    check(runCount(session, "Collision", Json{{"allow_touching", false}}, doors, slabs) == 24,
          "Collision(doors x slabs) == 24  [test_collision_rule]");
    check(runCount(session, "Clearance", Json{{"clearance", 0.5}}, walls, furniture) == 152,
          "Clearance(walls x furniture, 0.5) == 152  [test_clearance_rule]");

    // Ray_Check with a context (walls + slabs, as in test_ray_check_rule).
    FileSelection wallsAndSlabs{0, {}, walls.entity_ids};
    wallsAndSlabs.entity_ids.insert(wallsAndSlabs.entity_ids.end(), slabs.entity_ids.begin(), slabs.entity_ids.end());
    check(runCount(session, "Ray_Check", Json{{"max_ray_length", 5.0}}, doors, furniture, wallsAndSlabs) == 21,
          "Ray_Check(doors x furniture, ctx walls+slabs, 5) == 21  [test_ray_check_rule]");

    std::cout << (checks - failures) << "/" << checks << " checks passed\n";
    return failures == 0 ? 0 : 1;
}
