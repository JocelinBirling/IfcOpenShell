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

// PORT of ifcclash_plus/orchestrator.py, pipeline.py, RuleClass.py and
// booleanrule.py — see Orchestrator.h.

#include "Orchestrator.h"

#include "RuleCatalogue.h"
#include "RuleJson.h"

#include "../../ports/ifcopenshell_util/Attr.h"
#include "../../ports/ifcopenshell_util/Element.h"
#include "../../ports/ifctester/PyValue.h"
#include "../../../ifcparse/file.h"

#include "api/Engine.h"
#include "pset.h" // helpers: get_pset

#include <QElapsedTimer>
#include <QJsonDocument>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <map>
#include <set>
#include <stdexcept>

namespace bonsaiviewer::services::clash {

namespace util = bonsaiviewer::ports::ifcopenshell_util;
namespace it = bonsaiviewer::ports::ifctester;

namespace {

class QuietProgress final : public ifcclash::ProgressSink {};

using KeyId = std::pair<uint32_t, int>;
KeyId idOf(const ElementKey& key) { return {key.file_index, key.entity_id}; }

QString guidOf(const express::base& element) {
    return QString::fromStdString(util::forwardString(element, "GlobalId").value_or(std::string()));
}

ifcclash::Json toEngineJson(const QJsonValue& value) {
    const QByteArray text = QJsonDocument(QJsonObject{{"v", value}}).toJson(QJsonDocument::Compact);
    return ifcclash::Json::parse(text.toStdString())["v"];
}

std::vector<ifcclash::FileSelection> fileSelections(const Elements& elements) {
    std::map<uint32_t, std::vector<int>> by_file;
    for (const ElementKey& key : elements) by_file[key.file_index].push_back(key.entity_id);
    std::vector<ifcclash::FileSelection> selections;
    for (auto& [file, ids] : by_file) selections.push_back({file, {}, std::move(ids)});
    return selections;
}

// BooleanLeaf._evaluate_number_of_results
bool modeHolds(const QString& mode, const QString& value, size_t count) {
    if (mode == QLatin1String("have_result")) return count > 0;
    if (mode == QLatin1String("have_no_result")) return count == 0;
    bool ok = false;
    const long long n = value.toLongLong(&ok);
    if (!ok) throw std::runtime_error("the mode '" + mode.toStdString() + "' needs a number");
    const auto c = static_cast<long long>(count);
    if (mode == QLatin1String("have_more")) return c > n;
    if (mode == QLatin1String("have_more_or_equals")) return c >= n;
    if (mode == QLatin1String("have_less")) return c < n;
    if (mode == QLatin1String("have_less_or_equals")) return c <= n;
    if (mode == QLatin1String("equals")) return c == n;
    throw std::runtime_error("unknown boolean mode: " + mode.toStdString());
}

std::optional<double> numberOf(const property_value& value) {
    if (const auto* i = value.get_if<std::int64_t>()) return static_cast<double>(*i);
    if (const auto* d = value.get_if<double>()) return *d;
    if (const auto* text = value.get_if<std::string>()) {
        try {
            size_t used = 0;
            const double d = std::stod(*text, &used);
            if (used == text->size()) return d;
        } catch (...) {
        }
    }
    return std::nullopt;
}

// --- the Must operation (AbsoluteOrRelativeChecking.eval_check) ----------------

class Expression {
public:
    explicit Expression(std::string text) : text_(std::move(text)) {}

    double parse() {
        const double value = orExpr();
        skip();
        if (pos_ != text_.size()) fail();
        return value;
    }

private:
    [[noreturn]] void fail() const {
        throw std::runtime_error("malformed operation: " + text_);
    }
    void skip() {
        while (pos_ < text_.size() && std::isspace(static_cast<unsigned char>(text_[pos_]))) ++pos_;
    }
    bool take(const char* token) {
        skip();
        const size_t n = std::strlen(token);
        if (text_.compare(pos_, n, token) != 0) return false;
        // A word operator must not run into an identifier.
        if (std::isalpha(static_cast<unsigned char>(token[0])) && pos_ + n < text_.size() &&
            std::isalnum(static_cast<unsigned char>(text_[pos_ + n]))) {
            return false;
        }
        pos_ += n;
        return true;
    }
    double orExpr() {
        double value = andExpr();
        while (take("or")) {
            const double rhs = andExpr();
            value = (value != 0.0 || rhs != 0.0) ? 1.0 : 0.0;
        }
        return value;
    }
    double andExpr() {
        double value = notExpr();
        while (take("and")) {
            const double rhs = notExpr();
            value = (value != 0.0 && rhs != 0.0) ? 1.0 : 0.0;
        }
        return value;
    }
    double notExpr() {
        if (take("not")) return notExpr() != 0.0 ? 0.0 : 1.0;
        return comparison();
    }
    double comparison() {
        double value = sum();
        constexpr double kEpsilon = 1e-9;
        while (true) {
            if (take("<=")) value = value <= sum() + kEpsilon ? 1.0 : 0.0;
            else if (take(">=")) value = value + kEpsilon >= sum() ? 1.0 : 0.0;
            else if (take("==")) value = std::abs(value - sum()) <= kEpsilon ? 1.0 : 0.0;
            else if (take("!=")) value = std::abs(value - sum()) > kEpsilon ? 1.0 : 0.0;
            else if (take("<")) value = value < sum() - kEpsilon ? 1.0 : 0.0;
            else if (take(">")) value = value > sum() + kEpsilon ? 1.0 : 0.0;
            else if (take("=")) value = std::abs(value - sum()) <= kEpsilon ? 1.0 : 0.0;
            else return value;
        }
    }
    double sum() {
        double value = product();
        while (true) {
            if (take("+")) value += product();
            else if (take("-")) value -= product();
            else return value;
        }
    }
    double product() {
        double value = unary();
        while (true) {
            if (take("*")) value *= unary();
            else if (take("/")) {
                const double d = unary();
                if (d == 0.0) throw std::runtime_error("division by zero in the operation: " + text_);
                value /= d;
            } else {
                return value;
            }
        }
    }
    double unary() {
        if (take("-")) return -unary();
        if (take("+")) return unary();
        if (take("(")) {
            const double value = orExpr();
            if (!take(")")) fail();
            return value;
        }
        if (take("True")) return 1.0;
        if (take("False")) return 0.0;
        skip();
        size_t used = 0;
        double value = 0.0;
        try {
            value = std::stod(text_.substr(pos_), &used);
        } catch (...) {
            fail();
        }
        pos_ += used;
        return value;
    }

    std::string text_;
    size_t pos_ = 0;
};

} // namespace

bool evaluateOperation(const QString& operation, double source, double target) {
    QString text = operation;
    text.replace("SOURCE", QString::number(source, 'g', 17));
    text.replace("TARGET", QString::number(target, 'g', 17));
    return Expression(text.toStdString()).parse() != 0.0;
}

int countRules(const std::vector<RuleNode>& nodes) {
    int count = 0;
    for (const RuleNode& node : nodes) {
        if (const auto* folder = std::get_if<std::shared_ptr<RuleFolder>>(&node)) count += countRules((*folder)->contains);
        else ++count;
    }
    return count;
}

Orchestrator::Orchestrator(ifcclash::Session& session, size_t file_count, Callbacks callbacks,
                           const ifcclash::CancelToken& cancel)
    : session_(session), file_count_(file_count), callbacks_(std::move(callbacks)), cancel_(cancel) {}

void Orchestrator::run(const std::vector<RuleNode>& nodes) {
    done_ = 0;
    total_ = countRules(nodes);
    runNodes(nodes);
}

void Orchestrator::runNodes(const std::vector<RuleNode>& nodes) {
    for (const RuleNode& node : nodes) {
        if (cancel_.cancelled()) return;
        if (const auto* folder = std::get_if<std::shared_ptr<RuleFolder>>(&node)) runFolder(**folder);
        else reportRule(*std::get<std::shared_ptr<RuleCheck>>(node));
    }
}

void Orchestrator::runFolder(const RuleFolder& folder) {
    QString skipped;
    if (!folder.active) {
        skipped = "the folder is deactivated";
    } else {
        try {
            if (!folderActive(folder)) skipped = "the activation rule of the folder is not met";
        } catch (const std::exception& e) {
            skipped = QString("the activation rule failed: %1").arg(e.what());
        }
    }
    if (skipped.isEmpty()) {
        runNodes(folder.contains);
        return;
    }
    RuleOutcome outcome;
    outcome.node_id = folder.id;
    outcome.label = folder.viewer.label;
    outcome.skipped = skipped;
    if (callbacks_.finished) callbacks_.finished(outcome);
    done_ += countRules(folder.contains);
    if (callbacks_.progress) callbacks_.progress(done_, total_);
}

// RuleFolder.check_Activation_Rule
bool Orchestrator::folderActive(const RuleFolder& folder) {
    if (folder.activation_tree) return evaluateTree(*folder.activation_tree);
    if (!folder.activation_select.empty()) return !select(folder.activation_select).empty();
    return true;
}

void Orchestrator::reportRule(const RuleCheck& rule) {
    RuleOutcome outcome;
    outcome.node_id = rule.id;
    outcome.rule_type = rule.type;
    outcome.label = rule.viewer.label.isEmpty() ? rule.type : rule.viewer.label;
    if (callbacks_.started) callbacks_.started(outcome.node_id, outcome.label);
    QElapsedTimer timer;
    timer.start();
    try {
        RuleRun run = runRule(rule, rule.state == QLatin1String("Select") ? QStringLiteral("Select") : QStringLiteral("Final"));
        outcome.results = std::move(run.results);
        outcome.groups = std::move(run.groups);
        outcome.messages = std::move(run.messages);
        outcome.not_checked = std::move(run.not_checked);
        outcome.source_count = run.sources.size();
        outcome.target_count = run.targets.size();
        outcome.sources = std::move(run.sources);
        outcome.targets = std::move(run.targets);
    } catch (const ifcclash::Cancelled&) {
        outcome.error = "cancelled";
    } catch (const std::exception& e) {
        outcome.error = QString::fromUtf8(e.what());
    }
    outcome.seconds = timer.elapsed() / 1000.0;
    if (callbacks_.finished) callbacks_.finished(outcome);
    ++done_;
    if (callbacks_.progress) callbacks_.progress(done_, total_);
}

// SelectFacet.run + create_list_of_element, SelectRule.run
Elements Orchestrator::select(const Select& select) {
    Elements elements;
    if (select.kind == Select::Kind::Rule) {
        if (!select.rule) return elements;
        return runRule(*select.rule, "Select", select.element_to_pass, select.element_state_to_pass).selection;
    }
    if (select.kind != Select::Kind::Facet || select.applicability.empty()) return elements;
    for (uint32_t index = 0; index < file_count_; ++index) {
        ifcopenshell::file* file = session_.ifc_file(index);
        if (file == nullptr) continue;
        std::optional<std::vector<express::base>> current;
        for (it::Facet facet : select.applicability) {
            // IDS entity names are case-insensitive: the port compares uppercase.
            if (facet.type == it::FacetType::Entity && facet.name.isSimple()) {
                std::transform(facet.name.simple.begin(), facet.name.simple.end(), facet.name.simple.begin(),
                               [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
            }
            if (!current) {
                if (facet.type == it::FacetType::Entity) {
                    current = facet.filter(*file, nullptr);
                } else {
                    const std::vector<express::base> products = file->instances_by_type("IfcProduct");
                    current = facet.filter(*file, &products);
                }
            } else {
                current = facet.filter(*file, &*current);
            }
        }
        for (const express::base& element : current.value_or(std::vector<express::base>{})) {
            elements.push_back({index, static_cast<int>(element.id()), guidOf(element).toStdString()});
        }
    }
    return elements;
}

ifcclash::RunResult Orchestrator::runEngine(const RuleCheck& rule, const Elements& sources, const Elements& targets,
                                            const Elements& context) {
    const RuleSpec* spec = RuleCatalogue::instance().find(rule.type);
    if (spec == nullptr) throw std::runtime_error("unknown rule type: " + rule.type.toStdString());
    ifcclash::RunRequest request;
    request.rule_type = spec->engine_type.toStdString();
    request.params = ifcclash::Json::object();
    for (const ParamSpec& param : spec->params) {
        if (rule.params.contains(param.name)) request.params[param.name.toStdString()] = toEngineJson(rule.params.value(param.name));
    }
    request.selections.source = fileSelections(sources);
    if (spec->isTwoObject()) request.selections.target = fileSelections(targets);
    if (spec->hasSlot("context")) request.selections.context = fileSelections(context);
    QuietProgress quiet;
    return session_.run(request, cancel_, quiet);
}

Orchestrator::RuleRun Orchestrator::runRule(const RuleCheck& rule, const QString& state,
                                            const QString& element_to_pass, bool element_state_to_pass) {
    cancel_.throw_if_cancelled();
    const RuleSpec* spec = RuleCatalogue::instance().find(rule.type);
    if (spec == nullptr) throw std::runtime_error("unknown rule type: " + rule.type.toStdString());

    RuleRun run;
    run.sources = select(rule.source);
    if (spec->isTwoObject()) run.targets = select(rule.target);
    const Elements context = spec->hasSlot("context") ? select(rule.context) : Elements{};

    const ifcclash::RunResult raw = runEngine(rule, run.sources, run.targets, context);
    std::set<QString> seen_messages;
    for (const ifcclash::Diagnostic& d : raw.diagnostics.entries) {
        if (d.severity == ifcclash::Severity::Info) continue;
        if (d.element) run.not_checked.push_back(*d.element);
        const QString message = QString::fromStdString(d.message);
        if (seen_messages.insert(message).second) run.messages << message;
    }
    for (const ifcclash::ResultRecord& record : raw.results) {
        ClashResult result;
        result.id = json::newId();
        result.status = record.status;
        result.source = record.source;
        result.target = record.target;
        result.record = std::make_shared<const ifcclash::ResultRecord>(record);
        run.results.push_back(std::move(result));
    }

    // The pipeline (ResultPipeline.apply).
    if (spec->isTwoObject()) applyException(rule, run.results);
    if (state == QLatin1String("Final")) {
        applyLabels(rule.criticity, run.results, true);
        applyLabels(rule.actor, run.results, false);
        if (rule.must) applyMust(rule, run);
        else run.groups = group(rule, run.results);
    }
    if (state == QLatin1String("Select")) run.selection = selectionOf(rule, run, element_to_pass, element_state_to_pass);
    return run;
}

// --- exceptions (RuleCheck.run_exception, BooleanLeaf.evaluate_result) -----------

void Orchestrator::applyException(const RuleCheck& rule, std::vector<ClashResult>& results) {
    if (rule.exception.empty() || results.empty()) return;
    std::vector<bool> excepted(results.size(), false);
    if (!rule.exception.relation.isEmpty()) {
        for (size_t i = 0; i < results.size(); ++i) {
            if (results[i].target) excepted[i] = relationException(rule.exception.relation, results[i].source, *results[i].target);
        }
    } else {
        std::vector<ifcclash::KeyPair> pairs;
        std::vector<size_t> indexes;
        for (size_t i = 0; i < results.size(); ++i) {
            if (!results[i].target) continue;
            pairs.emplace_back(results[i].source, *results[i].target);
            indexes.push_back(i);
        }
        const std::vector<bool> holds = evaluateTree(*rule.exception.tree, pairs);
        for (size_t j = 0; j < indexes.size(); ++j) excepted[indexes[j]] = holds[j];
    }
    if (rule.exception.timing == QLatin1String("before")) {
        // The excepted pairs leave the run.
        std::vector<ClashResult> kept;
        for (size_t i = 0; i < results.size(); ++i) {
            if (!excepted[i]) kept.push_back(std::move(results[i]));
        }
        results = std::move(kept);
    } else {
        for (size_t i = 0; i < results.size(); ++i) {
            if (excepted[i]) results[i].status = false;
        }
    }
}

std::vector<bool> Orchestrator::evaluateTree(const BooleanNode& node, const std::vector<ifcclash::KeyPair>& pairs) {
    switch (node.kind) {
    case BooleanNode::Kind::And:
    case BooleanNode::Kind::Or: {
        std::vector<bool> combined(pairs.size(), node.kind == BooleanNode::Kind::And);
        for (const auto& child : node.children) {
            const std::vector<bool> values = evaluateTree(*child, pairs);
            for (size_t i = 0; i < pairs.size(); ++i) {
                combined[i] = node.kind == BooleanNode::Kind::And ? (combined[i] && values[i]) : (combined[i] || values[i]);
            }
        }
        return combined;
    }
    case BooleanNode::Kind::Not: {
        if (node.children.empty()) return std::vector<bool>(pairs.size(), true);
        std::vector<bool> values = evaluateTree(*node.children.front(), pairs);
        for (size_t i = 0; i < values.size(); ++i) values[i] = !values[i];
        return values;
    }
    case BooleanNode::Kind::Leaf:
        break;
    }
    if (!node.rule) throw std::runtime_error("an exception leaf without rule");
    const RuleSpec* spec = RuleCatalogue::instance().find(node.rule->type);
    if (spec == nullptr) throw std::runtime_error("unknown rule type: " + node.rule->type.toStdString());

    // The leaf rule runs on the elements of each pair: in one engine call.
    std::vector<size_t> counts(pairs.size(), 0);
    if (spec->isTwoObject()) {
        ifcclash::RunRequest request;
        request.rule_type = spec->engine_type.toStdString();
        request.params = ifcclash::Json::object();
        for (const ParamSpec& param : spec->params) {
            if (node.rule->params.contains(param.name)) {
                request.params[param.name.toStdString()] = toEngineJson(node.rule->params.value(param.name));
            }
        }
        const ifcclash::RunResult raw = session_.evaluate_pairs(request, pairs);
        std::map<std::pair<KeyId, KeyId>, size_t> index;
        for (size_t i = 0; i < pairs.size(); ++i) index.emplace(std::make_pair(idOf(pairs[i].first), idOf(pairs[i].second)), i);
        for (const ifcclash::ResultRecord& record : raw.results) {
            if (!record.target) continue;
            auto found = index.find({idOf(record.source), idOf(*record.target)});
            if (found == index.end()) found = index.find({idOf(*record.target), idOf(record.source)});
            if (found != index.end()) ++counts[found->second];
        }
    } else {
        Elements sources;
        std::set<KeyId> seen;
        for (const auto& pair : pairs) {
            if (seen.insert(idOf(pair.first)).second) sources.push_back(pair.first);
        }
        const ifcclash::RunResult raw = runEngine(*node.rule, sources, {}, {});
        std::map<KeyId, size_t> per_source;
        for (const ifcclash::ResultRecord& record : raw.results) ++per_source[idOf(record.source)];
        for (size_t i = 0; i < pairs.size(); ++i) counts[i] = per_source[idOf(pairs[i].first)];
    }
    std::vector<bool> holds(pairs.size());
    for (size_t i = 0; i < pairs.size(); ++i) holds[i] = modeHolds(node.mode, node.value, counts[i]);
    return holds;
}

// BooleanRule.evaluate after its rules ran (folder activation).
bool Orchestrator::evaluateTree(const BooleanNode& node) {
    switch (node.kind) {
    case BooleanNode::Kind::And:
        return std::all_of(node.children.begin(), node.children.end(), [this](const auto& c) { return evaluateTree(*c); });
    case BooleanNode::Kind::Or:
        return std::any_of(node.children.begin(), node.children.end(), [this](const auto& c) { return evaluateTree(*c); });
    case BooleanNode::Kind::Not:
        return node.children.empty() || !evaluateTree(*node.children.front());
    case BooleanNode::Kind::Leaf:
        break;
    }
    if (!node.rule) throw std::runtime_error("a boolean leaf without rule");
    return modeHolds(node.mode, node.value, runRule(*node.rule, "Final").results.size());
}

bool Orchestrator::relationException(const QString& relation, const ElementKey& a, const ElementKey& b) {
    const auto entity = [this](const ElementKey& key) -> express::base {
        ifcopenshell::file* file = session_.ifc_file(key.file_index);
        if (file == nullptr) return {};
        try {
            return file->instance_by_id(key.entity_id);
        } catch (...) {
            return {};
        }
    };
    const express::base ea = entity(a);
    const express::base eb = entity(b);
    if (!ea || !eb) return false;
    if (relation == QLatin1String("Same_IfcSystem")) {
        // The IfcSystems of an element: IfcRelAssignsToGroup through HasAssignments.
        const auto systems = [](const express::base& element) {
            std::set<QString> result;
            for (const express::base& assignment : util::inverse(element, "HasAssignments")) {
                if (!util::isA(assignment, "IfcRelAssignsToGroup")) continue;
                const express::base group = util::forwardEntity(assignment, "RelatingGroup");
                if (group && util::isA(group, "IfcSystem")) result.insert(guidOf(group));
            }
            return result;
        };
        const auto sa = systems(ea);
        const auto sb = systems(eb);
        return std::any_of(sa.begin(), sa.end(), [&sb](const QString& g) { return !g.isEmpty() && sb.count(g); });
    }
    const char* container_class = relation == QLatin1String("Same_Building")  ? "IfcBuilding"
                                  : relation == QLatin1String("Same_Storey") ? "IfcBuildingStorey"
                                  : relation == QLatin1String("Same_Space")  ? "IfcSpace"
                                                                              : nullptr;
    if (container_class == nullptr) throw std::runtime_error("unknown exception: " + relation.toStdString());
    const QString ca = guidOf(util::get_container(ea, false, container_class));
    const QString cb = guidOf(util::get_container(eb, false, container_class));
    return !ca.isEmpty() && ca == cb;
}

// --- criticity, actor (run_criticity, run_actor) ----------------------------------

void Orchestrator::applyLabels(const std::vector<Select>& selects, std::vector<ClashResult>& results, bool criticity) {
    for (const Select& one : selects) {
        if (one.empty() || one.classification_name.isEmpty()) continue;
        std::set<KeyId> members;
        for (const ElementKey& key : select(one)) members.insert(idOf(key));
        for (ClashResult& result : results) {
            const bool hit = members.count(idOf(result.source)) || (result.target && members.count(idOf(*result.target)));
            if (!hit) continue;
            QStringList& labels = criticity ? result.criticity : result.actor;
            if (!labels.contains(one.classification_name)) labels << one.classification_name;
        }
    }
}

// --- grouping (run_grouping) --------------------------------------------------------

std::vector<ResultGroup> Orchestrator::group(const RuleCheck& rule, const std::vector<ClashResult>& results) {
    std::vector<ResultGroup> groups;
    const Grouping& grouping = rule.grouping;
    if (grouping.kind == Grouping::Kind::None) return groups;

    const auto entity = [this](const ElementKey& key) -> express::base {
        ifcopenshell::file* file = session_.ifc_file(key.file_index);
        if (file == nullptr) return {};
        try {
            return file->instance_by_id(key.entity_id);
        } catch (...) {
            return {};
        }
    };
    std::map<QString, size_t> index;
    const auto groupFor = [&](const QString& label) -> ResultGroup& {
        auto found = index.find(label);
        if (found == index.end()) {
            found = index.emplace(label, groups.size()).first;
            groups.push_back(ResultGroup{label, {}, {}, {}});
        }
        return groups[found->second];
    };
    const auto add = [](ResultGroup& g, int result, const ElementKey* source, const ElementKey* target) {
        if (std::find(g.results.begin(), g.results.end(), result) == g.results.end()) g.results.push_back(result);
        const auto insert = [](Elements& set, const ElementKey& key) {
            if (std::none_of(set.begin(), set.end(), [&key](const ElementKey& k) { return k == key; })) set.push_back(key);
        };
        if (source) insert(g.sources, *source);
        if (target) insert(g.targets, *target);
    };

    if (grouping.kind == Grouping::Kind::SelectFacet) {
        // The Grouping set of spec 09 (e.g. the spaces): one group per element
        // of the set, empty ones included, holding the results whose source or
        // target lies in it (spatial container or aggregate chain).
        const Elements set = select(grouping.select);
        std::map<KeyId, size_t> by_element;
        for (const ElementKey& key : set) {
            const express::base e = entity(key);
            QString label = QString::fromStdString(util::forwardString(e, "Name").value_or(""));
            if (label.isEmpty()) label = QString::fromStdString(key.global_id);
            by_element.emplace(idOf(key), groups.size());
            groups.push_back(ResultGroup{label, {}, {}, {}});
        }
        const auto holder = [&](const ElementKey& key) -> std::optional<size_t> {
            express::base current = entity(key);
            for (int depth = 0; current && depth < 32; ++depth) {
                const auto found = by_element.find({key.file_index, static_cast<int>(current.id())});
                if (found != by_element.end() && depth > 0) return found->second;
                express::base next = util::get_container(current, true);
                if (!next) next = util::get_aggregate(current);
                if (!next) next = util::get_container(current);
                current = next;
            }
            return std::nullopt;
        };
        for (size_t i = 0; i < results.size(); ++i) {
            if (const auto g = holder(results[i].source)) add(groups[*g], static_cast<int>(i), &results[i].source, nullptr);
            if (results[i].target) {
                if (const auto g = holder(*results[i].target)) add(groups[*g], static_cast<int>(i), nullptr, &*results[i].target);
            }
        }
        return groups;
    }

    const auto keyOf = [&](const ElementKey& key) -> QString {
        const express::base e = entity(key);
        if (grouping.kind == Grouping::Kind::Name) {
            if (grouping.name == QLatin1String("ENTITY")) return QString::fromStdString(util::className(e));
            return QString::fromStdString(key.global_id);
        }
        if (!grouping.facet) return {};
        const it::Facet& facet = *grouping.facet;
        if (facet.type == it::FacetType::Property) {
            const auto value = get_pset(e, facet.propertySet.simple, facet.baseName.simple);
            return value ? QString::fromStdString(it::pyStr(*value)) : QStringLiteral("None");
        }
        if (facet.type == it::FacetType::Attribute) {
            const auto value = util::forward(e, facet.name.simple);
            if (!value) return QStringLiteral("None");
            const auto text = util::forwardString(e, facet.name.simple);
            if (text) return QString::fromStdString(*text);
            if (value->type() == ifcopenshell::Argument_DOUBLE) return QString::number(static_cast<double>(*value));
            if (value->type() == ifcopenshell::Argument_INT) return QString::number(static_cast<int64_t>(*value));
            if (value->type() == ifcopenshell::Argument_BOOL) return static_cast<bool>(*value) ? "True" : "False";
            return QStringLiteral("…");
        }
        return {}; // PartOf, Material, Classification, CLOSENESS: not grouped (as in Python)
    };

    const bool by_object = grouping.kind == Grouping::Kind::Name &&
                           (grouping.name == QLatin1String("SOURCE") || grouping.name == QLatin1String("TARGET"));
    const bool supported = by_object || (grouping.kind == Grouping::Kind::Name && grouping.name == QLatin1String("ENTITY")) ||
                           (grouping.kind == Grouping::Kind::Facet && grouping.facet &&
                            (grouping.facet->type == it::FacetType::Property || grouping.facet->type == it::FacetType::Attribute));
    if (!supported) return groups;
    for (size_t i = 0; i < results.size(); ++i) {
        const ClashResult& r = results[i];
        if (by_object) {
            const ElementKey* object = grouping.name == QLatin1String("SOURCE") ? &r.source : (r.target ? &*r.target : nullptr);
            if (!object) continue;
            add(groupFor(QString::fromStdString(object->global_id)), static_cast<int>(i), &r.source, r.target ? &*r.target : nullptr);
            continue;
        }
        add(groupFor(keyOf(r.source)), static_cast<int>(i), &r.source, nullptr);
        if (r.target) add(groupFor(keyOf(*r.target)), static_cast<int>(i), nullptr, &*r.target);
    }
    return groups;
}

// --- Must (AbsoluteOrRelativeChecking.run) --------------------------------------------

void Orchestrator::applyMust(const RuleCheck& rule, RuleRun& run) {
    const AbsOrRelCheck& must = *rule.must;
    if (!must.count_excepted) {
        // The pairs an exception invalidated stay out of the groups.
        run.results.erase(std::remove_if(run.results.begin(), run.results.end(), [](const ClashResult& r) { return !r.status; }),
                          run.results.end());
    }
    std::vector<ResultGroup> groups = group(rule, run.results);
    if (groups.empty()) {
        // group_by_source: one group per source object.
        std::map<KeyId, size_t> index;
        for (size_t i = 0; i < run.results.size(); ++i) {
            const ClashResult& r = run.results[i];
            auto found = index.find(idOf(r.source));
            if (found == index.end()) {
                found = index.emplace(idOf(r.source), groups.size()).first;
                groups.push_back(ResultGroup{QString::fromStdString(r.source.global_id), {}, {r.source}, {}});
            }
            ResultGroup& g = groups[found->second];
            g.results.push_back(static_cast<int>(i));
            if (r.target && std::none_of(g.targets.begin(), g.targets.end(), [&r](const ElementKey& k) { return k == *r.target; })) {
                g.targets.push_back(*r.target);
            }
        }
    }
    const bool quantity = must.check_type.contains("Quantity");
    const auto measure = [&](const Elements& set) -> double {
        if (!quantity) return static_cast<double>(set.size());
        if (must.quantity_pset.isEmpty() || must.quantity_prop.isEmpty()) return 0.0;
        double total = 0.0;
        for (const ElementKey& key : set) {
            ifcopenshell::file* file = session_.ifc_file(key.file_index);
            if (file == nullptr) continue;
            try {
                const auto value = get_pset(file->instance_by_id(key.entity_id), must.quantity_pset.toStdString(),
                                            must.quantity_prop.toStdString());
                if (value) total += numberOf(*value).value_or(0.0);
            } catch (...) {
            }
        }
        return total;
    };
    std::vector<ClashResult> verdicts;
    for (size_t g = 0; g < groups.size(); ++g) {
        ClashResult verdict;
        verdict.id = json::newId();
        verdict.complex = true;
        verdict.sources = groups[g].sources;
        verdict.targets = groups[g].targets;
        if (!verdict.sources.empty()) verdict.source = verdict.sources.front();
        verdict.status = evaluateOperation(must.operation, measure(groups[g].sources), measure(groups[g].targets));
        groups[g].results = {static_cast<int>(g)};
        verdicts.push_back(std::move(verdict));
    }
    run.results = std::move(verdicts);
    run.groups = std::move(groups);
}

// --- the Select step (pipeline.selection_of) --------------------------------------------

Elements Orchestrator::selectionOf(const RuleCheck& rule, const RuleRun& run, const QString& element_to_pass,
                                   bool element_state_to_pass) const {
    const RuleSpec* spec = RuleCatalogue::instance().find(rule.type);
    const bool target = spec && spec->isTwoObject() && element_to_pass == QLatin1String("target");
    Elements selection;
    std::set<KeyId> added;
    if (element_state_to_pass) {
        for (const ClashResult& r : run.results) {
            if (!r.status) continue;
            const ElementKey* element = target ? (r.target ? &*r.target : nullptr) : &r.source;
            if (element && added.insert(idOf(*element)).second) selection.push_back(*element);
        }
        return selection;
    }
    std::set<KeyId> in_results;
    for (const ClashResult& r : run.results) {
        const ElementKey* element = target ? (r.target ? &*r.target : nullptr) : &r.source;
        if (element) in_results.insert(idOf(*element));
    }
    for (const ElementKey& key : target ? run.targets : run.sources) {
        if (!in_results.count(idOf(key)) && added.insert(idOf(key)).second) selection.push_back(key);
    }
    return selection;
}

} // namespace bonsaiviewer::services::clash
