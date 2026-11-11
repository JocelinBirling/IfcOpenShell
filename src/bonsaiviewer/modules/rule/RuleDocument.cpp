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

#include "RuleDocument.h"

#include "../../services/clash/RuleCatalogue.h"
#include "../../services/clash/RuleJson.h"

#include <functional>
#include <set>

namespace bonsaiviewer::modules::rule {

namespace {

using clash::RuleCheck;
using clash::RuleFolder;
using clash::RuleNode;

void walk(const std::vector<RuleNode>& nodes, const std::function<void(const RuleNode&)>& visit) {
    for (const RuleNode& node : nodes) {
        visit(node);
        if (const auto* folder = std::get_if<std::shared_ptr<RuleFolder>>(&node)) walk((*folder)->contains, visit);
    }
}

QString nodeId(const RuleNode& node) {
    if (const auto* folder = std::get_if<std::shared_ptr<RuleFolder>>(&node)) return (*folder)->id;
    return std::get<std::shared_ptr<RuleCheck>>(node)->id;
}

std::vector<clash::Select*> slotSelects(RuleCheck& rule) {
    return {&rule.source, &rule.target, &rule.context};
}

} // namespace

RuleDocument::RuleDocument(QObject* parent) : QObject(parent) {}

void RuleDocument::newFile() {
    file_ = clash::RuleFile{};
    file_.id = clash::json::newId();
    file_.viewer.label = QStringLiteral("Rule File");
    open_ = true;
    modified_ = false;
    path_.clear();
    emit opened();
    emit changed();
}

void RuleDocument::setFile(clash::RuleFile file, const QString& path) {
    file_ = std::move(file);
    if (file_.id.isEmpty()) file_.id = clash::json::newId();
    // Nodes without id (a hand-written file) get one: the results and the
    // cascades are tracked by id.
    walk(file_.contains, [](const RuleNode& node) {
        if (const auto* folder = std::get_if<std::shared_ptr<RuleFolder>>(&node)) {
            if ((*folder)->id.isEmpty()) (*folder)->id = clash::json::newId();
        } else {
            auto& rule = std::get<std::shared_ptr<RuleCheck>>(node);
            if (rule->id.isEmpty()) rule->id = clash::json::newId();
        }
    });
    open_ = true;
    modified_ = false;
    path_ = path;
    emit opened();
    emit changed();
}

void RuleDocument::setPath(const QString& path) {
    path_ = path;
}

void RuleDocument::markSaved() {
    modified_ = false;
    emit changed();
}

void RuleDocument::touch() {
    modified_ = true;
    emit changed();
}

std::vector<std::shared_ptr<RuleCheck>> RuleDocument::treeRules() const {
    std::vector<std::shared_ptr<RuleCheck>> rules;
    walk(file_.contains, [&rules](const RuleNode& node) {
        if (const auto* rule = std::get_if<std::shared_ptr<RuleCheck>>(&node)) rules.push_back(*rule);
    });
    return rules;
}

std::shared_ptr<RuleCheck> RuleDocument::treeRule(const QString& id) const {
    for (const auto& rule : treeRules()) {
        if (rule->id == id) return rule;
    }
    return nullptr;
}

std::shared_ptr<RuleFolder> RuleDocument::folder(const QString& id) const {
    std::shared_ptr<RuleFolder> found;
    walk(file_.contains, [&found, &id](const RuleNode& node) {
        if (const auto* folder = std::get_if<std::shared_ptr<RuleFolder>>(&node)) {
            if ((*folder)->id == id) found = *folder;
        }
    });
    return found;
}

std::vector<RuleNode>* RuleDocument::containerOf(const QString& id, size_t* index) {
    std::function<std::vector<RuleNode>*(std::vector<RuleNode>&)> search = [&](std::vector<RuleNode>& nodes) -> std::vector<RuleNode>* {
        for (size_t i = 0; i < nodes.size(); ++i) {
            if (nodeId(nodes[i]) == id) {
                if (index) *index = i;
                return &nodes;
            }
            if (auto* folder = std::get_if<std::shared_ptr<RuleFolder>>(&nodes[i])) {
                if (auto* found = search((*folder)->contains)) return found;
            }
        }
        return nullptr;
    };
    return search(file_.contains);
}

std::vector<RuleNode>* RuleDocument::contents(const QString& folder_id) {
    if (folder_id.isEmpty()) return &file_.contains;
    const auto f = folder(folder_id);
    return f ? &f->contains : nullptr;
}

std::optional<RuleNode> RuleDocument::take(const QString& id) {
    size_t index = 0;
    std::vector<RuleNode>* container = containerOf(id, &index);
    if (container == nullptr) return std::nullopt;
    RuleNode node = (*container)[index];
    container->erase(container->begin() + static_cast<std::ptrdiff_t>(index));
    return node;
}

QStringList RuleDocument::fedBy(const RuleCheck& rule) const {
    QStringList labels;
    for (const clash::Select* select : {&rule.source, &rule.target, &rule.context}) {
        if (select->kind != clash::Select::Kind::Rule || !select->rule) continue;
        if (const auto source = treeRule(select->rule->id)) labels << labelOf(*source);
    }
    return labels;
}

QStringList RuleDocument::feeds(const RuleCheck& rule) const {
    QStringList labels;
    for (const auto& other : treeRules()) {
        if (other->id == rule.id) continue;
        for (const clash::Select* select : {&other->source, &other->target, &other->context}) {
            if (select->kind == clash::Select::Kind::Rule && select->rule && select->rule->id == rule.id) {
                labels << labelOf(*other);
                break;
            }
        }
    }
    return labels;
}

bool RuleDocument::wouldLoop(const QString& from_id, const QString& into_id) const {
    // A loop when `into` already feeds `from`, directly or not.
    std::set<QString> seen;
    std::function<bool(const QString&)> reaches = [&](const QString& id) {
        if (id == into_id) return true;
        if (!seen.insert(id).second) return false;
        const auto rule = treeRule(id);
        if (!rule) return false;
        for (const clash::Select* select : {&rule->source, &rule->target, &rule->context}) {
            if (select->kind == clash::Select::Kind::Rule && select->rule && reaches(select->rule->id)) return true;
        }
        return false;
    };
    return reaches(from_id);
}

void RuleDocument::refreshLinks() {
    // One pass per tree rule: a chain of n cascades is up to date after n
    // passes (a loop is refused when the link is made).
    const auto rules = treeRules();
    std::function<void(RuleCheck&, int)> refresh = [&](RuleCheck& rule, int depth) {
        if (depth > 16) return;
        for (clash::Select* select : slotSelects(rule)) {
            if (select->kind != clash::Select::Kind::Rule || !select->rule) continue;
            if (const auto source = treeRule(select->rule->id); source && source.get() != &rule) {
                select->rule = clash::json::cloneRule(*source);
            } else {
                refresh(*select->rule, depth + 1); // a rule nested by value
            }
        }
    };
    for (size_t pass = 0; pass < rules.size(); ++pass) {
        for (const auto& rule : rules) refresh(*rule, 0);
    }
}

QString RuleDocument::labelOf(const RuleCheck& rule) {
    if (!rule.viewer.label.isEmpty()) return rule.viewer.label;
    const auto* spec = clash::RuleCatalogue::instance().find(rule.type);
    return spec ? spec->label : rule.type;
}

QString RuleDocument::labelOf(const RuleFolder& folder) {
    return folder.viewer.label.isEmpty() ? QStringLiteral("Folder") : folder.viewer.label;
}

} // namespace bonsaiviewer::modules::rule
