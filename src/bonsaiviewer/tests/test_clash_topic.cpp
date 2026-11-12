// This file was generated with the assistance of an AI coding tool.
// The content of a BCF topic made from clash results: title, description,
// labels, priority, header files (target bonsaiviewer_clash_topic_tests).
#include "modules/rule/ClashTopic.h"

#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>

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

ResultRow pair(const QString& source, const QString& criticity, const QString& actor) {
    ResultRow row;
    row.node_id = "r1";
    row.rule_label = "Duct vs Wall";
    row.rule_type = "Intersection";
    row.source_name = source;
    row.source_class = "IfcWall";
    row.source_guid = "2O2Fr$t4X7Zf8NOew3FLOH";
    row.source_model_name = "Archi.ifc";
    row.target_name = "Duct 3";
    row.target_class = "IfcDuctSegment";
    row.target_guid = "1hOSvn6df7F8_7GcBWlRGQ";
    row.target_model_name = "HVAC.ifc";
    row.measure = "volume = 0.003 m³";
    if (!criticity.isEmpty()) row.criticity = {criticity};
    if (!actor.isEmpty()) row.actor = {actor};
    row.review = ReviewStatus::Valid;
    row.sources = {ResultElement{"m1", row.source_guid, 12}};
    row.targets = {ResultElement{"m2", row.target_guid, 345}};
    row.key = resultKey("r1", false, QString(), row.sources, row.targets);
    auto record = std::make_shared<ifcclash::ResultRecord>();
    record->measures.set(ifcclash::Measure::Volume, 0.003);
    record->measures.set(ifcclash::Measure::PenetrationDepth, 0.012);
    row.record = record;
    return row;
}

} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    const ResultRow a = pair("Wall 1", "Low", "HVAC");
    ResultRow b = pair("Wall 2", "High", "Structure");
    b.number = 42;

    check(topic::title("{rule} — {source} / {target}", {&a}) == "Duct vs Wall — Wall 1 / Duct 3", "title of one result");
    check(topic::title("{rule} — {n} clash(es)", {&a, &b}) == "Duct vs Wall — 2 clash(es)", "title of several results");
    check(topic::title("{id} {rule}", {&b}) == "R-0042 Duct vs Wall", "{id}: the short number");
    check(topic::title("{id} {rule}", {&a}) == "Duct vs Wall", "{id} of a result without number: nothing");
    check(topic::describe(b).startsWith("ID: R-0042\nRule: Duct vs Wall"), "the ID line first");

    const QString one = topic::describe(a);
    check(one.startsWith("Rule: Duct vs Wall (Intersection)\n"), "the rule first");
    check(one.contains("Source: Wall 1 [IfcWall] 2O2Fr$t4X7Zf8NOew3FLOH — Archi.ifc"), "the source, class, GlobalId, model");
    check(one.contains("Target: Duct 3 [IfcDuctSegment] 1hOSvn6df7F8_7GcBWlRGQ — HVAC.ifc"), "the target");
    check(one.contains("Measure: volume = 0.003 m³; penetration_depth = 0.012 m"), "every measure, with its unit");
    check(one.contains("Criticity: Low") && one.contains("Actor: HVAC") && one.contains("Review: Valid"),
          "criticity, actor, review");
    check(one.endsWith("Result key: " + a.key), "the last line is the result key");

    const QString both = topic::description({&a, &b});
    check(both.startsWith("2 clash results (Duct vs Wall).\n\nResult 1\nRule:"), "several: a count, then each result");
    check(both.count("Result key: ") == 2, "several: one key line per result");
    check(both.contains("\n\nResult R-0042\nID: R-0042\n"), "several: a result named by its ID");

    check(topic::labels({&a, &b}) == QStringList({"Clash", "Duct vs Wall", "Low", "High", "HVAC", "Structure"}),
          "labels: Clash, the rules, the criticities, the actors, each once");

    const topic::PriorityTable table{{"High", "Critical"}, {"Medium", "Normal"}, {"Low", "Minor"}};
    check(topic::priority({&a}, table) == "Minor", "the priority of the criticity");
    check(topic::priority({&a, &b}, table) == "Critical", "the highest criticity wins");
    check(topic::priority({&a}, {}).isEmpty(), "no table: no priority");

    QTemporaryDir dir;
    const QString path = dir.filePath("Archi.ifc");
    QFile file(path);
    file.open(QIODevice::WriteOnly);
    file.write("ISO-10303-21;");
    file.close();
    const auto headers = topic::headerFiles({&a}, {{"m0", dir.filePath("Other.ifc"), ""}, {"m1", path, "0proj"}});
    check(headers.size() == 1 && headers[0].filename == "Archi.ifc" && headers[0].ifc_project == "0proj" &&
              headers[0].reference.startsWith("file://") && !headers[0].date.isEmpty(),
          "one header file per model of the results (m2 has no path, m0 is not used)");

    std::cout << checks - failures << "/" << checks << " checks passed\n";
    return failures == 0 ? 0 : 1;
}
