// This file was generated with the assistance of an AI coding tool.
// Unit tests of the clash rules on the Duplex model, ported from the
// IfcClash_Plus Python suite (tests/test_Rule_OneObject.py and
// tests/test_Rule_TwoObjects.py, which run the same rules on
// Ifc2x3_Duplex_Architecture.ifc) — target bonsaiviewer_clash_rule_tests.
//
// Each case keeps the Python expected count; a rule that runs through the
// C++ engine must flag the same number of results.
#include "ports/ifcclash_engine/api/Engine.h"

#include "../ifcgeom/serializer.h"
#include "../ifcparse/file.h"
#include "../serializers/document_serializer_plugin.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
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

// The Duplex converted into a RocksDB database, as the mass conversion does
// (rdb document serializer): <dir>/Duplex.ifc and <dir>/Duplex.rdb.
std::filesystem::path makeRdb(const std::filesystem::path& dir) {
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    const auto ifc = dir / "Duplex.ifc";
    const auto rdb = dir / "Duplex.rdb";
    std::filesystem::copy_file(kDuplex, ifc);
    ifcopenshell::serializers::document_serializer_context context;
    context.file = nullptr;
    context.input_filename = ifc.string();
    context.output_filename = rdb.string();
    context.stream = true;
    auto& registry = ifcopenshell::serializers::document_serializer_registry_instance();
    if (!registry.find("rdb")) return {};
    registry.create("rdb", context)->finalize();
    return rdb;
}

bool hasDiagnostic(const Session& session, const std::string& text) {
    for (const auto& d : session.diagnostics().entries) {
        if (d.message.find(text) != std::string::npos) return true;
    }
    return false;
}

// The rules on a model loaded from a .rdb (BonsaiViewer runs them on the
// database the loader opened): same entity ids and same results as the IFC.
void checkRdb(Session& ifc_session) {
    const auto dir = std::filesystem::temp_directory_path() / "bonsaiviewer_clash_rule_tests_rdb";
    const auto rdb = makeRdb(dir);
    check(!rdb.empty() && std::filesystem::is_directory(rdb), "the Duplex converts to a .rdb");
    if (rdb.empty()) return;

    // The viewer's own reader, as SceneLoader opens the data source.
    auto viewer_file = std::make_shared<ifcopenshell::file>(rdb.string(), ifcopenshell::FT_AUTODETECT, true);
    check(viewer_file->good(), "the .rdb opens read-only");
    {
        // One session adopts the viewer's file; two more open the .rdb
        // themselves at the same time: read-only, the database lock (taken
        // by a read-write open) does not stop the second one.
        Session adopted({rdb}, {viewer_file}, quietOptions());
        Session opened({rdb}, quietOptions());
        Session opened_too({rdb}, quietOptions());
        check(opened.file_count() == 1 && opened_too.file_count() == 1, "two sessions open the same .rdb at once");
        for (Session* session : {&adopted, &opened}) {
            check(idsOf(*session, "IfcWallStandardCase") == idsOf(ifc_session, "IfcWallStandardCase"),
                  "the .rdb keeps the entity ids of the IFC");
            const FileSelection walls = select(*session, "IfcWallStandardCase");
            const FileSelection furniture = select(*session, "IfcFurnishingElement");
            const FileSelection doors = select(*session, "IfcDoor");
            const FileSelection slabs = select(*session, "IfcSlab");
            check(runCount(*session, "Intersection", Json{{"tolerance", 0.01}}, walls, furniture) == 1,
                  "Intersection(walls x furniture, 0.01) == 1 on the .rdb");
            check(runCount(*session, "Collision", Json{{"allow_touching", false}}, doors, slabs) == 24,
                  "Collision(doors x slabs) == 24 on the .rdb");
        }
    }

    // The .ifcview of a .rdb (mtime freshness) is dated against the .ifc it
    // comes from, not the database written after it. Here an unreadable one:
    // the freshness passes, the format check rejects it.
    const auto now = std::filesystem::file_time_type::clock::now();
    const auto view = dir / "Duplex.ifcview";
    std::ofstream(view) << "not an ifcview";
    std::filesystem::last_write_time(dir / "Duplex.ifc", now - std::chrono::hours(2));
    std::filesystem::last_write_time(view, now - std::chrono::hours(1));
    std::filesystem::last_write_time(rdb, now);
    {
        SessionOptions options;
        options.ifcview_freshness = "mtime";
        Session session({rdb}, {viewer_file}, options);
        check(!hasDiagnostic(session, "older than the IFC") && hasDiagnostic(session, "unreadable"),
              "a .ifcview older than its .rdb but newer than its .ifc passes the freshness check");
    }
    std::filesystem::last_write_time(view, now - std::chrono::hours(3));
    {
        Session session({rdb}, {viewer_file}, SessionOptions{});
        check(hasDiagnostic(session, "older than the IFC"), "a .ifcview older than the .ifc of its .rdb is stale");
    }
    viewer_file.reset();
    std::filesystem::remove_all(dir);
}

}  // namespace

// bonsaiviewer_clash_rule_tests <model.rdb|model.ifc>...: Collision walls x
// slabs over real models, opened as BonsaiViewer opens them (read-only
// reader adopted by the session, .ifcview found next to each model).
// CLASH_RULE=Collision_triang takes the triangles from the .ifcview.
int runOnModels(const std::vector<std::filesystem::path>& paths) {
    const auto start = std::chrono::steady_clock::now();
    std::vector<std::shared_ptr<ifcopenshell::file>> files;
    for (const auto& path : paths) {
        files.push_back(std::make_shared<ifcopenshell::file>(path.string(), ifcopenshell::FT_AUTODETECT, true));
    }
    SessionOptions options;
    options.triangle_source = "auto";  // as ClashView
    Session session(paths, files, options);
    for (const auto& d : session.diagnostics().entries) std::cout << "diagnostic: " << d.message << "\n";
    RunRequest r;
    const char* rule = std::getenv("CLASH_RULE");
    r.rule_type = rule ? rule : "Collision";
    r.params = Json{{"allow_touching", false}};
    for (uint32_t i = 0; i < paths.size(); ++i) {
        FileSelection walls{i, {}, {}}, slabs{i, {}, {}};
        for (const auto& e : files[i]->instances_by_type("IfcWall")) walls.entity_ids.push_back(e.id());
        for (const auto& e : files[i]->instances_by_type("IfcSlab")) slabs.entity_ids.push_back(e.id());
        std::cout << paths[i].filename().string() << ": " << walls.entity_ids.size() << " walls, "
                  << slabs.entity_ids.size() << " slabs\n";
        r.selections.source.push_back(walls);
        r.selections.target.push_back(slabs);
    }
    const RunResult result = session.run(r);
    const auto ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
    std::cout << r.rule_type << "(walls x slabs): " << result.results.size() << " results, " << ms << " ms, "
              << session.ifc_conversions() << " elements triangulated from the source\n";
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc > 1) return runOnModels({argv + 1, argv + argc});
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

    try {
        checkRdb(session);
    } catch (const std::exception& e) {
        check(false, std::string("the rules on a .rdb: ") + e.what());
    }

    std::cout << (checks - failures) << "/" << checks << " checks passed\n";
    return failures == 0 ? 0 : 1;
}
