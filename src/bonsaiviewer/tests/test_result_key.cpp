// This file was generated with the assistance of an AI coding tool.
// The stable key of the clash results (target bonsaiviewer_result_key_tests).
#include "modules/rule/ResultKey.h"

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

} // namespace

int main() {
    const ResultElement wall{"m1", "2O2Fr$t4X7Zf8NOew3FLOH", 12};
    const ResultElement duct{"m2", "1hOSvn6df7F8_7GcBWlRGQ", 345};
    const ResultElement copy{"m3", "2O2Fr$t4X7Zf8NOew3FLOH", 12}; // the same element in a copied model

    check(resultKey("r1", false, QString(), {wall}, {duct}) == "r1|m1:2O2Fr$t4X7Zf8NOew3FLOH|m2:1hOSvn6df7F8_7GcBWlRGQ",
          "a pair: rule, source, target");
    check(resultKey("r1", false, QString(), {wall}, {}) == "r1|m1:2O2Fr$t4X7Zf8NOew3FLOH|", "one object: no target");
    check(resultKey("r1", false, QString(), {wall}, {duct}) != resultKey("r1", false, QString(), {copy}, {duct}),
          "the model id tells two copies of an element apart");
    check(resultKey("r1", false, QString(), {wall}, {duct}) != resultKey("r1", false, QString(), {duct}, {wall}),
          "source and target are not swapped");
    check(resultKey("r1", false, QString(), {ResultElement{"m1", QString(), 99}}, {}) == "r1|m1:#99|",
          "no GlobalId: the STEP id");
    check(resultKey("r1", false, "g", {wall}, {duct}) == resultKey("r1", false, QString(), {wall}, {duct}),
          "a pair does not depend on its group");

    // A Must group: the order of the elements does not matter, the group does.
    const QString group = resultKey("r2", true, "Room 1", {wall, duct}, {copy});
    check(group == resultKey("r2", true, "Room 1", {duct, wall}, {copy}), "a Must group: the order is free");
    check(group != resultKey("r2", true, "Room 2", {wall, duct}, {copy}), "a Must group: the group counts");
    check(group != resultKey("r2", true, "Room 1", {wall}, {copy}), "a Must group: its elements count");
    check(group.startsWith("r2|Room 1|") && group.count('|') == 3, "a Must group: rule|group|sources|targets");

    std::cout << checks - failures << "/" << checks << " checks passed\n";
    return failures == 0 ? 0 : 1;
}
