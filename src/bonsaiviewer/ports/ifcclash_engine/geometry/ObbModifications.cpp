// This file was generated with the assistance of an AI coding tool.
#include "ObbModifications.h"

#include "../core/Errors.h"

#include <charconv>
#include <cmath>

namespace ifcclash::geometry {

namespace {

std::string trim(const std::string& s) {
    const auto first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1);
}

[[noreturn]] void fail(size_t line, const std::string& why) {
    throw ConfigError("line " + std::to_string(line) + ": " + why);
}

std::string expected_names() {
    std::string out;
    for (const auto& name : obb_op_names()) out += (out.empty() ? "" : ", ") + name;
    return out;
}

}  // namespace

const std::vector<std::string>& obb_op_names() {
    static const std::vector<std::string> names{"expand_sides", "detach_top_by_extrude", "detach_bottom_by_extrude",
                                                "extend_up",    "extend_down",           "NEW_OBB"};
    return names;
}

const std::string& obb_op_name(ObbOp op) {
    return obb_op_names().at(static_cast<size_t>(op));
}

ObbStep parse_obb_step(const std::string& text, size_t line) {
    const std::string entry = trim(text);
    if (entry.empty()) fail(line, "empty line");
    const auto colon = entry.find(':');
    const std::string name = trim(entry.substr(0, colon));
    const auto& names = obb_op_names();
    size_t index = 0;
    while (index < names.size() && names[index] != name) ++index;
    if (index == names.size()) fail(line, "unknown operation '" + name + "' (expected: " + expected_names() + ")");
    ObbStep step;
    step.op = static_cast<ObbOp>(index);
    if (step.op == ObbOp::NewObb) {
        if (colon != std::string::npos) fail(line, "NEW_OBB takes no value");
        return step;
    }
    std::string value = colon == std::string::npos ? std::string() : trim(entry.substr(colon + 1));
    if (value.find(':') != std::string::npos) fail(line, "one ':' expected in '" + entry + "'");
    if (value.empty()) fail(line, "'" + name + "' needs a value (e.g. " + name + ":0.5 or " + name + ":50%)");
    const std::string written = value;
    if (value.back() == '%') {
        step.change.percent = true;
        value = trim(value.substr(0, value.size() - 1));
    }
    if (value.find(',') != std::string::npos) fail(line, "invalid value '" + written + "': use a decimal point (0.5)");
    if (!value.empty() && value.front() == '+') value.erase(0, 1);
    double number = 0.0;
    const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), number);
    if (value.empty() || error != std::errc() || end != value.data() + value.size() || !std::isfinite(number)) {
        fail(line, "invalid value '" + written + "'");
    }
    step.change.value = step.change.percent ? number / 100.0 : number;
    return step;
}

std::vector<ObbStep> parse_obb_modifications(const std::vector<std::string>& lines, const std::string& rule) {
    std::vector<ObbStep> steps;
    try {
        for (size_t i = 0; i < lines.size(); ++i) steps.push_back(parse_obb_step(lines[i], i + 1));
        for (size_t i = 0; i < steps.size(); ++i) {
            if (steps[i].op != ObbOp::NewObb) continue;
            if (i == 0 || i + 1 == steps.size() || steps[i - 1].op == ObbOp::NewObb) {
                fail(i + 1, "NEW_OBB separates two zones: it cannot be first, last or doubled");
            }
        }
    } catch (const ConfigError& e) {
        throw ConfigError(rule + ": " + e.what());
    }
    return steps;
}

std::vector<Bnd_OBB> obb_zones(const Bnd_OBB& base, const std::vector<ObbStep>& steps) {
    Bnd_OBB obb = base;
    std::vector<Bnd_OBB> out;
    for (const ObbStep& step : steps) {
        switch (step.op) {
        case ObbOp::ExpandSides:
            // Python assigns the None returned by expand_sides (and then
            // fails): the intended in-place expansion is applied here.
            expand_sides(obb, step.change);
            break;
        case ObbOp::DetachTop: obb = detach_by_extrude(obb, step.change, true); break;
        case ObbOp::DetachBottom: obb = detach_by_extrude(obb, step.change, false); break;
        case ObbOp::ExtendUp: obb = extend_vertically(obb, step.change, true); break;
        case ObbOp::ExtendDown: obb = extend_vertically(obb, step.change, false); break;
        case ObbOp::NewObb:
            // Python recomputes the OBB of the (already meshed) shape: the
            // same box as `base`.
            out.push_back(obb);
            obb = base;
            break;
        }
    }
    out.push_back(obb);
    return out;
}

std::string obb_modifications_doc() {
    return "The detection zones around the source, one step per line, applied in order to the zone; the zone "
           "starts as the OBB of the source.\n"
           "expand_sides:v - the two horizontal widths are set to the height of the zone + v (as IfcClash_Plus)\n"
           "expand_sides:p% - the two horizontal widths grow by p/2 % (50% gives +25%)\n"
           "detach_top_by_extrude:v - the zone becomes a slab of height v on its top (p%: of its height)\n"
           "detach_bottom_by_extrude:v - the same, below it\n"
           "extend_up:v - the zone grows upward by v (p%: of its height)\n"
           "extend_down:v - the zone grows downward by v (p%: of its height)\n"
           "NEW_OBB - closes the zone; the next steps build a new zone from the OBB of the source (never first, "
           "last or twice in a row)\n"
           "Values in metres with a decimal point (0.5), or a percentage (50%); a negative value shrinks the zone.\n"
           "Empty: one zone, the OBB of the source.\n"
           "Example: extend_up:0.5 / NEW_OBB / detach_bottom_by_extrude:20%";
}

}  // namespace ifcclash::geometry
