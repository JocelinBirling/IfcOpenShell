// This file was generated with the assistance of an AI coding tool.
// IfcClash_Plus engine (BonsaiViewer copy) — the list_of_modifications of
// OBB_Custom: the strict parser and the zones (target ifcclash_engine_tests).
#include "core/Errors.h"
#include "geometry/Obb.h"
#include "geometry/ObbModifications.h"
#include "registry/Registry.h"

#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>

#include <cmath>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

using namespace ifcclash;
using namespace ifcclash::geometry;

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

bool near(double a, double b) { return std::abs(a - b) < 1e-9; }

// The message of the ConfigError `f` raises ("" when it does not raise).
std::string error_of(const std::function<void()>& f) {
    try {
        f();
    } catch (const ConfigError& e) {
        return e.what();
    }
    return {};
}

bool contains(const std::string& text, const std::string& part) { return text.find(part) != std::string::npos; }

// An axis-aligned 2 x 2 x 2 OBB at (0, 0, 0).
Bnd_OBB cube() { return make_obb(gp_Pnt(0, 0, 0), gp_Dir(1, 0, 0), gp_Dir(0, 1, 0), gp_Dir(0, 0, 1), 1, 1, 1); }

// The former loop of OBB_Custom (before BonsaiViewer's parser): the reference
// of the non-regression test, on valid lists.
std::vector<Bnd_OBB> former_zones(const Bnd_OBB& base, const std::vector<std::string>& list) {
    const auto value = [](const std::string& m) -> ObbChange {
        std::string v = m.substr(m.find(':') + 1);
        if (!v.empty() && v.back() == '%') {
            v.pop_back();
            return {std::stod(v) / 100.0, true};
        }
        return {std::stod(v), false};
    };
    Bnd_OBB obb = base;
    std::vector<Bnd_OBB> out;
    for (const auto& m : list) {
        const auto has = [&](const char* name) { return m.find(name) != std::string::npos; };
        if (has("expand_sides")) expand_sides(obb, value(m));
        else if (has("detach_top_by_extrude")) obb = detach_by_extrude(obb, value(m), true);
        else if (has("detach_bottom_by_extrude")) obb = detach_by_extrude(obb, value(m), false);
        else if (has("extend_up")) obb = extend_vertically(obb, value(m), true);
        else if (has("extend_down")) obb = extend_vertically(obb, value(m), false);
        else if (has("NEW_OBB")) {
            out.push_back(obb);
            obb = base;
        }
    }
    out.push_back(obb);
    return out;
}

bool same(const Bnd_OBB& a, const Bnd_OBB& b) {
    const auto ca = obb_corners(a), cb = obb_corners(b);
    for (size_t i = 0; i < ca.size(); ++i) {
        if (!near(ca[i].X(), cb[i].X()) || !near(ca[i].Y(), cb[i].Y()) || !near(ca[i].Z(), cb[i].Z())) return false;
    }
    return true;
}

void test_parse() {
    // Accepted.
    auto s = parse_obb_step("extend_up:0.5", 1);
    check(s.op == ObbOp::ExtendUp && near(s.change.value, 0.5) && !s.change.percent, "extend_up:0.5");
    s = parse_obb_step(" extend_up : 0.5 ", 1);
    check(s.op == ObbOp::ExtendUp && near(s.change.value, 0.5), "spaces around are trimmed");
    s = parse_obb_step("extend_up:50%", 1);
    check(s.change.percent && near(s.change.value, 0.5), "extend_up:50% is the ratio 0.5");
    s = parse_obb_step("extend_up:-0.2", 1);
    check(near(s.change.value, -0.2), "a negative value is allowed (it shrinks the zone)");
    s = parse_obb_step("NEW_OBB", 1);
    check(s.op == ObbOp::NewObb, "NEW_OBB");
    // Refused, with the line and the cause.
    const auto refused = [](const std::string& entry, const std::vector<std::string>& parts) {
        const std::string message = error_of([&]() { parse_obb_step(entry, 1); });
        bool ok = !message.empty() && contains(message, "line 1");
        for (const auto& part : parts) ok = ok && contains(message, part);
        check(ok, "'" + entry + "' refused with " + (parts.empty() ? "" : parts.front()) + " (got: " + message + ")");
    };
    refused("detach_top:0.5", {"unknown operation 'detach_top'", "expected: expand_sides, detach_top_by_extrude"});
    refused("extend_upp:0.5", {"unknown operation 'extend_upp'"});
    refused("extend_up", {"'extend_up' needs a value", "extend_up:0.5 or extend_up:50%"});
    refused("extend_up:0,5", {"invalid value '0,5'", "decimal point"});
    refused("extend_up:abc", {"invalid value 'abc'"});
    refused("extend_up:0.5:x", {"one ':' expected"});
    refused("NEW_OBB:1", {"NEW_OBB takes no value"});
    refused("", {"empty line"});
}

void test_structure() {
    const auto message = [](const std::vector<std::string>& list) {
        return error_of([&]() { parse_obb_modifications(list, "OBB_Custom"); });
    };
    std::string m = message({"NEW_OBB", "extend_up:1"});
    check(contains(m, "OBB_Custom: line 1") && contains(m, "NEW_OBB separates two zones"), "NEW_OBB first: " + m);
    m = message({"extend_up:1", "NEW_OBB"});
    check(contains(m, "line 2") && contains(m, "NEW_OBB separates"), "NEW_OBB last: " + m);
    m = message({"extend_up:1", "NEW_OBB", "NEW_OBB", "extend_down:1"});
    check(contains(m, "line 3") && contains(m, "NEW_OBB separates"), "NEW_OBB doubled: " + m);
    m = message({"extend_up:1", "detach_top:2"});
    check(contains(m, "OBB_Custom: line 2: unknown operation 'detach_top'"), "the line of the faulty entry: " + m);
    check(parse_obb_modifications({}, "OBB_Custom").empty(), "an empty list is valid");
    // Through the registry: refused when the rule is created, before any computation.
    for (const char* rule : {"OBB_Custom", "OBB_Custom_triang"}) {
        m = error_of([&]() {
            default_registry().find(rule).create(Json{{"list_of_modifications", {"extend_up:0.5", "extend_up:0,5"}}});
        });
        check(contains(m, std::string(rule) + ": line 2: invalid value '0,5'"), std::string(rule) + " refuses at creation: " + m);
    }
}

void test_zones() {
    const Bnd_OBB base = cube();
    auto zones = obb_zones(base, parse_obb_modifications({}, "test"));
    check(zones.size() == 1 && same(zones[0], base), "empty list: the zone is the OBB of the source");
    zones = obb_zones(base, parse_obb_modifications({"extend_up:1"}, "test"));
    check(zones.size() == 1 && near(zones[0].ZHSize(), 1.5) && near(zones[0].Center().Z(), 0.5),
          "extend_up:1: height 3, centre at z = 0.5");
    zones = obb_zones(base, parse_obb_modifications({"detach_top_by_extrude:50%"}, "test"));
    check(zones.size() == 1 && near(zones[0].ZHSize(), 0.5) && near(zones[0].Center().Z(), 1.5),
          "detach_top_by_extrude:50%: a 1 m slab centred at z = 1.5");
    zones = obb_zones(base, parse_obb_modifications({"extend_up:1", "NEW_OBB", "extend_down:1"}, "test"));
    check(zones.size() == 2 && near(zones[0].Center().Z(), 0.5) && near(zones[1].Center().Z(), -0.5) &&
              near(zones[1].ZHSize(), 1.5),
          "extend_up:1, NEW_OBB, extend_down:1: two zones, the second from the OBB of the source");
    // Non-regression: the zones of valid lists are those of the former loop.
    const std::vector<std::vector<std::string>> lists{
        {"expand_sides:0.4"},
        {"expand_sides:20%", "detach_top_by_extrude:0.5"},
        {"detach_bottom_by_extrude:0.3", "extend_down:25%"},
        {"extend_up:1", "NEW_OBB", "expand_sides:0.2", "detach_top_by_extrude:10%", "NEW_OBB", "extend_down:0.7"},
    };
    for (const auto& list : lists) {
        const auto now = obb_zones(base, parse_obb_modifications(list, "test"));
        const auto before = former_zones(base, list);
        bool ok = now.size() == before.size();
        for (size_t i = 0; ok && i < now.size(); ++i) ok = same(now[i], before[i]);
        check(ok, "non-regression: " + list.front() + "... (" + std::to_string(list.size()) + " lines)");
    }
}

void test_doc() {
    const std::string doc = obb_modifications_doc();
    for (const auto& name : obb_op_names()) check(contains(doc, name), "the doc explains " + name);
    check(contains(doc, "decimal point") && contains(doc, "negative") && contains(doc, "Empty"),
          "the doc gives the values, the negative values and the empty list");
}

}  // namespace

int main() {
    test_parse();
    test_structure();
    test_zones();
    test_doc();
    std::cout << checks - failures << " / " << checks << " checks passed\n";
    return failures == 0 ? 0 : 1;
}
