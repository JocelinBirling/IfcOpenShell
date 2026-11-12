// This file was generated with the assistance of an AI coding tool.
// IfcClash_Plus engine (BonsaiViewer copy) — the JSON round trip of a result
// record: to_json -> record_from_json -> to_json gives the same JSON
// (target ifcclash_result_json_tests).
#include "core/Errors.h"
#include "core/Result.h"

#include <iostream>
#include <string>

using namespace ifcclash;

namespace {

int failures = 0;
int checks = 0;

void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) {
        ++failures;
        std::cerr << "FAILED: " << what << "\n";
    }
}

ResultRecord fullRecord() {
    ResultRecord r;
    r.source = ElementKey{0, 12, "2O2Fr$t4X7Zf8NOew3FLOH"};
    r.target = ElementKey{1, 345, "1hOSvn6df7F8_7GcBWlRGQ"};
    r.status = false;
    r.flags.degraded = true;
    r.flags.fallback_source = true;
    r.measures.set(Measure::PenetrationDepth, 0.012);
    r.measures.set(Measure::Volume, 0.003);
    r.points.push_back({{0, 1, 2}, {3, 4, 5}, std::string("closest")});
    r.points.push_back({{6, 7, 8}, {9, 10, 11}, std::nullopt});
    FaceData face{{0, 0, 1}, 2.5, {{Point3{0, 0, 0}, Point3{1, 0, 0}, Point3{0, 1, 0}}}};
    r.faces.push_back({face, face});
    r.source_obbs.push_back({{1, 2, 3}, {Point3{1, 0, 0}, Point3{0, 1, 0}, Point3{0, 0, 1}}, {0.5, 0.5, 1}, std::string("zone")});
    r.target_obbs.push_back({{4, 5, 6}, {Point3{0, 1, 0}, Point3{-1, 0, 0}, Point3{0, 0, 1}}, {1, 1, 1}, std::nullopt});
    r.axes.push_back({{0, 0, 0}, {0, 0, 1}, 3.0, std::string("main")});
    r.volumes.intersection = 0.25;
    r.volumes.clearance_violated = 1.0;
    r.axis = AxisBlock{std::string("parallel"), false};
    RaycastBlock raycast;
    raycast.planned = 10;
    raycast.cast = 9;
    raycast.hit = 4;
    raycast.hit_segments.push_back({Point3{0, 0, 0}, Point3{1, 1, 1}});
    raycast.blocked_segments.push_back({Point3{2, 2, 2}, Point3{3, 3, 3}});
    r.raycast = raycast;
    r.zone = ZoneBlock{"push", "left", 0.9, 1.2, 2.1};
    return r;
}

std::string message_of(const Json& json) {
    try {
        record_from_json(json);
    } catch (const ConfigError& e) {
        return e.what();
    }
    return {};
}

}  // namespace

int main() {
    // Every block written, read back, written again: the same JSON.
    const Json full = to_json(fullRecord());
    const ResultRecord read = record_from_json(full);
    check(to_json(read) == full, "full record: round trip gives the same JSON");
    check(read.measures.items().front().first == Measure::PenetrationDepth,
          "the order of the measures is kept (the first is the nominal one)");
    check(read.source == fullRecord().source && read.source.global_id == "2O2Fr$t4X7Zf8NOew3FLOH",
          "the source key and its GlobalId are read back");
    check(read.flags.degraded && read.flags.fallback_source && !read.flags.gross_element, "the flags are read back");

    // A one-object record: no target.
    ResultRecord one;
    one.source = ElementKey{2, 7, "0abc"};
    one.measures.set(Measure::Area, 4.0);
    const Json one_json = to_json(one);
    const ResultRecord one_read = record_from_json(one_json);
    check(!one_read.target && to_json(one_read) == one_json, "one-object record: no target, same JSON");

    // An empty info and unknown keys: accepted.
    Json extra = one_json;
    extra["info"]["future_block"] = Json{{"x", 1}};
    check(to_json(record_from_json(extra)) == one_json, "unknown info keys are ignored");

    // A malformed block raises ConfigError, never a JSON exception.
    Json bad = full;
    bad["info"]["points"][0]["source"] = Json::array({1, 2});
    check(message_of(bad).find("point must be an array of 3") != std::string::npos, "a 2D point is refused");
    bad = full;
    bad["info"]["obbs"]["source"][0]["center"] = "here";
    check(!message_of(bad).empty(), "a string point is refused with a ConfigError");
    check(!message_of(Json::array()).empty(), "a record that is not an object is refused");

    std::cout << checks - failures << "/" << checks << " checks passed\n";
    return failures == 0 ? 0 : 1;
}
