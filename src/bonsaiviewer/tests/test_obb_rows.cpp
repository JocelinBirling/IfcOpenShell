// This file was generated with the assistance of an AI coding tool.
// BonsaiViewer09 (TI09) — the model of the OBB_Custom steps editor:
// describeStep, the JSON round trip, the entries kept as text (target
// bonsaiviewer_obb_tests).
#include "modules/rule/ObbModifications.h"

#include <iostream>

using namespace bonsaiviewer::modules::rule;

namespace {

int failures = 0, checks = 0;

void check(bool ok, const QString& what) {
    ++checks;
    if (!ok) {
        ++failures;
        std::cerr << "FAILED: " << what.toStdString() << "\n";
    }
}

void expect(const QString& op, double value, bool percent, const QString& text) {
    const QString got = obb::describeStep(op, value, percent);
    check(got == text, QString("describeStep(%1, %2%3): '%4', expected '%5'").arg(op).arg(value).arg(percent ? "%" : "", got, text));
}

} // namespace

int main() {
    // describeStep: each operation, in metres and in %.
    expect("expand_sides", 0.4, false, "the two horizontal widths are set to the height of the zone + 0.4 m");
    expect("expand_sides", 20, true, "the two horizontal widths grow by 10 % (half of 20 %)");
    expect("detach_top_by_extrude", 0.5, false, "the zone becomes a slab of 0.5 m on top of it");
    expect("detach_top_by_extrude", 50, true, "the zone becomes a slab of 50 % of its height on top of it");
    expect("detach_bottom_by_extrude", 0.3, false, "the zone becomes a slab of 0.3 m below it");
    expect("detach_bottom_by_extrude", 25, true, "the zone becomes a slab of 25 % of its height below it");
    expect("extend_up", 1, false, "the zone grows 1 m upward");
    expect("extend_up", 50, true, "the zone grows 50 % of its height upward");
    expect("extend_down", 0.7, false, "the zone grows 0.7 m downward");
    expect("extend_down", 10, true, "the zone grows 10 % of its height downward");
    expect("extend_up", -0.2, false, "the zone shrinks by 0.2 m from its top");

    // Round trip JSON -> rows -> JSON on a valid list: the same list.
    const QJsonArray valid{"extend_up:0.5", "expand_sides:20%", "NEW_OBB", "detach_bottom_by_extrude:0.07", "extend_down:12.5%"};
    const auto rows = obb::rowsFromJson(valid);
    check(rows.size() == 5 && rows[2].kind == obb::Row::Kind::NewZone && rows[1].percent && rows[1].value == 20,
          "a valid list is read as steps and a separator");
    check(obb::rowsToJson(rows) == valid, "round trip: the same list");
    const auto valid_check = obb::check(rows);
    check(valid_check.message.isEmpty() && valid_check.bad_rows.empty() && valid_check.zones == 2, "a valid list: 2 zones");

    // An entry the engine refuses is kept as text, with its message.
    const QJsonArray old{"extend_up:0.5", "extend_upp:1", "extend_down:0,5"};
    const auto read = obb::rowsFromJson(old);
    check(read.size() == 3 && read[1].kind == obb::Row::Kind::Raw && read[1].raw == "extend_upp:1" &&
              read[1].error.contains("unknown operation 'extend_upp'"),
          "an unknown entry stays as text with the message of the engine");
    check(read[2].kind == obb::Row::Kind::Raw && read[2].error.contains("decimal point"), "a comma: kept as text");
    check(obb::rowsToJson(read) == old, "the entries read as text are written back unchanged");
    const auto old_check = obb::check(read);
    check(old_check.bad_rows == std::vector<int>{1, 2} && old_check.message.contains("line 2"),
          "the refused rows and the first message of the engine");

    // A separator first, last or doubled: flagged before the run.
    obb::Row step, zone;
    zone.kind = obb::Row::Kind::NewZone;
    const auto bad = obb::check({zone, step, zone, zone, step, zone});
    check(bad.bad_rows == std::vector<int>{0, 3, 5} && bad.message.contains("NEW_OBB separates"), "misplaced separators");

    std::cout << checks - failures << " / " << checks << " checks passed\n";
    return failures == 0 ? 0 : 1;
}
