// This file was generated with the assistance of an AI coding tool.
/********************************************************************************
 *                                                                              *
 * This file is part of Bonsai.                                                 *
 *                                                                              *
 * Bonsai is free software: you can redistribute it and/or modify               *
 * it under the terms of the GNU General Public License as published by         *
 * the Free Software Foundation, either version 3.0 of the License, or          *
 * (at your option) any later version.                                          *
 *                                                                              *
 * Bonsai is distributed in the hope that it will be useful,                    *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of               *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                 *
 * GNU General Public License for more details.                                 *
 *                                                                              *
 * You should have received a copy of the GNU General Public License            *
 * along with this program. If not, see <http://www.gnu.org/licenses/>.         *
 *                                                                              *
 ********************************************************************************/

// BonsaiViewer09 (TI09) — PORT of ifcclash_plus/orchestrator.py,
// pipeline.py and the run logic of RuleClass.py / booleanrule.py, on the
// IfcClash_Plus C++ engine:
//
//   run_rule = selections (IDS facets on the engine's files, or the output
//              of another rule) -> engine (the _triang twin of the rule)
//              -> pipeline: Exception -> Criticity -> Actor
//                           -> (Grouping | Must) -> Select
//
// Folders run their content in order when they are active (check box) and
// their activation rule holds. Only the nodes of the tree report their
// outcome; the rules run for a cascade, an activation or an exception are
// run on their own copy and report nothing.
//
// Runs in a worker thread: it reads only the engine's own files.

#ifndef BONSAIVIEWER_SERVICES_CLASH_ORCHESTRATOR_H
#define BONSAIVIEWER_SERVICES_CLASH_ORCHESTRATOR_H

#include "RuleModel.h"

#include "core/Control.h"
#include "core/ElementKey.h"
#include "core/Result.h"

#include <QString>
#include <QStringList>

#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace ifcclash { class Session; }

namespace bonsaiviewer::services::clash {

using ElementKey = ifcclash::ElementKey;
using Elements = std::vector<ElementKey>;

struct ClashResult {
    QString id;
    bool status = true; // false: invalidated by an exception (or a failing Must group)
    // One / two objects: source (and target). A Must group: sources / targets.
    bool complex = false;
    ElementKey source;
    std::optional<ElementKey> target;
    Elements sources;
    Elements targets;
    QStringList criticity;
    QStringList actor;
    std::shared_ptr<const ifcclash::ResultRecord> record; // the info blocks (null for a group)
};

struct ResultGroup {
    QString label;
    std::vector<int> results; // indexes in the outcome results
    Elements sources;
    Elements targets;
};

// What a tree node reports at the end of its run.
struct RuleOutcome {
    QString node_id;
    QString rule_type;
    QString label;
    std::vector<ClashResult> results;
    std::vector<ResultGroup> groups;
    QStringList messages;     // warnings of the engine (elements without geometry...)
    Elements not_checked;     // elements the engine could not test
    QString error;            // the rule failed (the run goes on)
    QString skipped;          // why a folder or rule did not run
    size_t source_count = 0;
    size_t target_count = 0;
    double seconds = 0.0;
};

class Orchestrator {
public:
    struct Callbacks {
        std::function<void(const QString& node_id, const QString& label)> started;
        std::function<void(const RuleOutcome& outcome)> finished;
        std::function<void(int done, int total)> progress;
    };

    Orchestrator(ifcclash::Session& session, size_t file_count, Callbacks callbacks,
                 const ifcclash::CancelToken& cancel);

    // Runs the nodes in tree order (a whole file, a folder, a rule or a
    // multiple selection). A folder in the list runs its content.
    void run(const std::vector<RuleNode>& nodes);

    // The elements of a select (the "show the selection" of a facet slot).
    Elements select(const Select& select);

private:
    struct RuleRun {
        std::vector<ClashResult> results;
        std::vector<ResultGroup> groups;
        Elements sources;
        Elements targets;
        Elements selection; // the Select step
        QStringList messages;
        Elements not_checked;
    };

    void runNodes(const std::vector<RuleNode>& nodes);
    void runFolder(const RuleFolder& folder);
    void reportRule(const RuleCheck& rule);
    bool folderActive(const RuleFolder& folder);

    RuleRun runRule(const RuleCheck& rule, const QString& state, const QString& element_to_pass = "source",
                    bool element_state_to_pass = true);
    ifcclash::RunResult runEngine(const RuleCheck& rule, const Elements& sources, const Elements& targets,
                                  const Elements& context);
    void applyException(const RuleCheck& rule, std::vector<ClashResult>& results);
    std::vector<bool> evaluateTree(const BooleanNode& node, const std::vector<ifcclash::KeyPair>& pairs);
    bool evaluateTree(const BooleanNode& node);
    bool relationException(const QString& relation, const ElementKey& a, const ElementKey& b);
    void applyLabels(const std::vector<Select>& selects, std::vector<ClashResult>& results, bool criticity);
    std::vector<ResultGroup> group(const RuleCheck& rule, const std::vector<ClashResult>& results);
    void applyMust(const RuleCheck& rule, RuleRun& run);
    Elements selectionOf(const RuleCheck& rule, const RuleRun& run, const QString& element_to_pass,
                         bool element_state_to_pass) const;

    ifcclash::Session& session_;
    size_t file_count_;
    Callbacks callbacks_;
    const ifcclash::CancelToken& cancel_;
    int done_ = 0;
    int total_ = 0;
};

// The rules of the nodes, in tree order (folders walked), for the progress.
int countRules(const std::vector<RuleNode>& nodes);

// "TARGET>=2" with SOURCE / TARGET replaced: arithmetic, comparisons (= is
// ==), and / or / not. Throws std::runtime_error on a malformed operation.
bool evaluateOperation(const QString& operation, double source, double target);

} // namespace bonsaiviewer::services::clash

#endif // BONSAIVIEWER_SERVICES_CLASH_ORCHESTRATOR_H
