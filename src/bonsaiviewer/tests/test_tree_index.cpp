// This file was generated with the assistance of an AI coding tool.
// The node table of the Tree Explorer (target bonsaiviewer_tree_index_tests).
#include "services/TreeIndex.h"

#include <algorithm>
#include <iostream>

using namespace bonsaiviewer::services::tree;

namespace {

int failures = 0, checks = 0;

void check(bool ok, const QString& what) {
    ++checks;
    if (!ok) {
        ++failures;
        std::cerr << "FAILED: " << what.toStdString() << "\n";
    }
}

IndexNode folder(const QString& id, const QString& label) {
    IndexNode node;
    node.kind = IndexNode::Kind::Folder;
    node.id = id;
    node.label = label;
    return node;
}

IndexNode element(std::uint32_t object_id, const QString& label, int ifc_id = 0) {
    IndexNode node;
    node.kind = IndexNode::Kind::Element;
    node.id = QString::number(object_id ? object_id : ifc_id);
    node.label = label;
    node.object_id = object_id;
    node.model = 1;
    node.ifc_id = ifc_id;
    return node;
}

} // namespace

int main() {
    // Two systems sharing a pipe, a sub-system, and an element without geometry.
    TreeIndex index;
    const int heating = index.add(-1, folder("s1", "Heating"));
    const int water = index.add(-1, folder("s2", "Water"));
    const int loop = index.add(heating, folder("s3", "Loop 10"));
    const int loop2 = index.add(heating, folder("s4", "Loop 9"));
    index.add(heating, element(10, "Boiler"));
    const int pipe_a = index.add(loop, element(11, "Pipe"));
    index.add(loop, element(11, "Pipe"));  // listed twice in the same folder: counted once
    index.add(loop2, element(12, "Valve"));
    const int pipe_b = index.add(water, element(11, "Pipe"));
    index.add(water, element(0, "Sensor", 99));
    IndexNode message;
    message.kind = IndexNode::Kind::Message;
    message.label = "Model B: IFC data loading";
    const int loading = index.add(-1, message);
    index.finish();

    check(index.node(heating).count == 3, "a folder counts the distinct elements below it");
    check(index.node(loop).count == 1, "an element twice in a folder is counted once");
    check(index.node(water).count == 2, "an element without geometry is counted by its IFC id");
    check(index.node(pipe_a).multiplicity == 3 && index.node(pipe_b).multiplicity == 3, "×N over the whole tree");
    check(index.displayLabel(heating) == "Heating (3)", "folder label with its count");
    check(index.displayLabel(pipe_b) == QString::fromUtf8("Pipe ×3"), "element label with ×N");
    check(index.displayLabel(index.find("s2/99")) == "Sensor", "no mark for an element listed once");
    check(index.displayLabel(loading) == "Model B: IFC data loading", "messages have no count");

    // Order: messages first, then natural order ("Loop 9" before "Loop 10").
    check(index.roots().front() == loading, "messages first");
    check(index.node(heating).children[1] == loop2 && index.node(heating).children[2] == loop,
          "natural order of the labels");

    // Keys: paths, unique among siblings.
    check(index.node(loop).key == "s1/s3", "the key is the path of the ids");
    check(index.find("s1/s3/11") == pipe_a && index.find("s1/s3/11#2") >= 0, "duplicate siblings get a suffix");
    check(index.find("nope") == -1, "unknown key");

    auto ids = index.objectIdsBelow(heating);
    std::sort(ids.begin(), ids.end());
    check(ids == std::vector<std::uint32_t>{10, 11, 12}, "the object ids below a folder, each once");

    // Search: the matches, their parents and their children.
    int matches = 0;
    auto shown = index.match("valve", &matches);
    check(matches == 1, "one match");
    check(shown[index.find("s1/s4/12")] && shown[loop2] && shown[heating], "a match keeps its parents");
    check(!shown[water] && !shown[loop], "the rest is hidden");
    shown = index.match("loop 9");
    check(shown[index.find("s1/s4/12")], "a matching folder shows its children");
    shown = index.match("  ");
    check(std::all_of(shown.begin(), shown.end(), [](char c) { return c != 0; }), "empty search shows all");

    std::cout << checks - failures << "/" << checks << " checks passed\n";
    return failures == 0 ? 0 : 1;
}
