// This file was generated with the assistance of an AI coding tool.
// The clash section of a federation: the runs, the rules as they ran, the
// results with their review and group, written and read back
// (target bonsaiviewer_result_store_tests).
#include "modules/rule/ResultStore.h"

#include <QJsonArray>

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

store::SavedClash sample() {
    store::SavedClash clash;
    clash.rule_file = QJsonObject{{"type", "RuleFile"}, {"id", "file1"}, {"contains", QJsonArray{}}};
    clash.rule_file_path = "rules/clash.json";
    store::SavedRun run;
    run.id = "run1";
    run.started = "2026-11-12T15:00:00Z";
    run.seconds = 4.5;
    run.models.push_back(store::SavedModel{"m1", "Archi.ifc", "/models/Archi.ifc", 1762959600000, 123456});
    clash.runs.push_back(run);
    store::SavedNode node;
    node.node_id = "r1";
    node.run_id = "run1";
    node.label = "Duct vs Wall";
    node.type = "Intersection";
    node.rule = QJsonObject{{"type", "Intersection"}, {"id", "r1"}, {"tolerance", 0.01}};
    node.text = "1 result";
    node.sources = {ResultElement{"m1", "2O2Fr$t4X7Zf8NOew3FLOH", 12}};
    node.targets = {ResultElement{"m2", "1hOSvn6df7F8_7GcBWlRGQ", 345}};
    node.not_checked = {ResultElement{"m2", "3x", 7}};
    clash.nodes.push_back(node);

    ResultRow row;
    row.node_id = "r1";
    row.rule_label = "Duct vs Wall";
    row.rule_type = "Intersection";
    row.source_name = "Wall 1";
    row.source_guid = "2O2Fr$t4X7Zf8NOew3FLOH";
    row.source_class = "IfcWall";
    row.source_model_name = "Archi.ifc";
    row.source_ifc_id = 12;
    row.target_name = "Duct 3";
    row.target_guid = "1hOSvn6df7F8_7GcBWlRGQ";
    row.target_class = "IfcDuctSegment";
    row.target_model_name = "HVAC.ifc";
    row.target_ifc_id = 345;
    row.measure = "volume = 0.003 m³";
    row.criticity = {"High"};
    row.actor = {"HVAC"};
    row.group = "Level 1";
    row.manual_group = "To check on site";
    row.status = false;
    row.review = ReviewStatus::Valid;
    row.sources = node.sources;
    row.targets = node.targets;
    row.key = resultKey("r1", false, row.group, row.sources, row.targets);
    auto record = std::make_shared<ifcclash::ResultRecord>();
    record->source = ifcclash::ElementKey{0, 12, "2O2Fr$t4X7Zf8NOew3FLOH"};
    record->target = ifcclash::ElementKey{1, 345, "1hOSvn6df7F8_7GcBWlRGQ"};
    record->status = false;
    record->measures.set(ifcclash::Measure::Volume, 0.003);
    record->points.push_back({{0, 0, 0}, {1, 1, 1}, std::nullopt});
    row.record = record;
    row.topic_guid = "5e0d4c2a-1b3c-4d5e-8f90-123456789abc";
    clash.results.push_back(row);
    clash.bcf_file = "issues.bcf";
    clash.numbers = {{row.key, 1}, {"gone|a|b", 2}};
    clash.next_number = 3;
    clash.messages.emplace_back("r1", "Duct vs Wall: 1 element without geometry");
    return clash;
}

} // namespace

int main() {
    const store::SavedClash clash = sample();
    const QJsonObject json = store::toJson(clash);
    check(json.value("format").toInt() == store::kFormat, "the format is written");

    QStringList warnings;
    const store::SavedClash read = store::fromJson(json, &warnings);
    check(warnings.isEmpty(), "no warning on a sample written by this version");
    check(store::toJson(read) == json, "round trip: the same JSON");

    check(read.rule_file == clash.rule_file && read.rule_file_path == "rules/clash.json", "the rule file is kept");
    check(read.runs.size() == 1 && read.runs[0].models.size() == 1 && read.runs[0].models[0].mtime == 1762959600000 &&
              read.runs[0].models[0].size == 123456,
          "the run and the date and size of its models");
    check(read.nodes.size() == 1 && read.nodes[0].rule.value("tolerance").toDouble() == 0.01 &&
              read.nodes[0].not_checked.size() == 1 && read.nodes[0].text == "1 result",
          "the rule as it ran and the elements it did not check");

    const ResultRow& row = read.results.at(0);
    check(row.key == clash.results[0].key && row.review == ReviewStatus::Valid && row.manual_group == "To check on site",
          "the key, the review and the manual group of a result");
    check(!row.status && row.criticity == QStringList{"High"} && row.target_class == "IfcDuctSegment",
          "the status, the criticity, the classes");
    check(row.record && row.record->measures.get(ifcclash::Measure::Volume) == 0.003 && row.record->points.size() == 1,
          "the record of a result: its measures and points");
    check(row.source_objects.empty() && row.source_model == 0, "the viewer ids are not saved");
    check(read.messages.size() == 1 && read.messages[0].first == "r1", "the warnings of the run");
    check(row.topic_guid == "5e0d4c2a-1b3c-4d5e-8f90-123456789abc" && read.bcf_file == "issues.bcf",
          "the topic of a result and the BCF file");

    check(read.numbers == clash.numbers && read.next_number == 3,
          "the short numbers of the results, those of results gone too");
    QJsonObject lost_next = json;
    lost_next.remove("next_result_number");
    check(store::fromJson(lost_next).next_number == 3, "without the next number: after the highest one");

    // A broken record: the result is kept, without its details, with a warning.
    QJsonObject broken = json;
    QJsonArray results = broken.value("results").toArray();
    QJsonObject first = results[0].toObject();
    QJsonObject record_json = first.value("record").toObject();
    QJsonObject info = record_json.value("info").toObject();
    info["points"] = QJsonArray{QJsonObject{{"source", QJsonArray{1, 2}}, {"target", QJsonArray{1, 2, 3}}}};
    record_json["info"] = info;
    first["record"] = record_json;
    results[0] = first;
    broken["results"] = results;
    warnings.clear();
    const store::SavedClash partial = store::fromJson(broken, &warnings);
    check(partial.results.size() == 1 && !partial.results[0].record && warnings.size() == 1,
          "a malformed record: the result stays, without its details, one warning");

    // A newer format: read, with a warning.
    QJsonObject newer = json;
    newer["format"] = store::kFormat + 1;
    warnings.clear();
    store::fromJson(newer, &warnings);
    check(warnings.size() == 1, "a newer format gives a warning");

    std::cout << checks - failures << "/" << checks << " checks passed\n";
    return failures == 0 ? 0 : 1;
}
