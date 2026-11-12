// This file was generated with the assistance of an AI coding tool.
/********************************************************************************
 *                                                                              *
 * This file is part of Bonsai.                                                 *
 *                                                                              *
 * Bonsai is free software: you can redistribute it and/or modify               *
 * it under the terms of the GNU General Public License as published by         *
 * the Free Software Foundation, either version 3.0 of the License, or          *
 * (at your option) any later version.                                           *
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

#include "TreeIndex.h"

#include <QCollator>
#include <QSet>

#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace bonsaiviewer::services::tree {

int TreeIndex::add(int parent, IndexNode node) {
    const int index = static_cast<int>(nodes_.size());
    node.parent = parent;
    node.children.clear();
    nodes_.push_back(std::move(node));
    if (parent < 0) {
        roots_.push_back(index);
    } else {
        nodes_[parent].children.push_back(index);
    }
    return index;
}

std::uint64_t TreeIndex::identity(const IndexNode& node) {
    if (node.object_id != 0) return node.object_id;
    return (std::uint64_t(1) << 63) | (std::uint64_t(node.model) << 32) | std::uint32_t(node.ifc_id);
}

void TreeIndex::finish(bool sort) {
    if (sort) {
        QCollator collator;
        collator.setNumericMode(true);
        collator.setCaseSensitivity(Qt::CaseInsensitive);
        const auto less = [this, &collator](int a, int b) {
            // Messages (loading, empty) stay first.
            const bool ma = nodes_[a].kind == IndexNode::Kind::Message;
            const bool mb = nodes_[b].kind == IndexNode::Kind::Message;
            if (ma != mb) return ma;
            return collator.compare(nodes_[a].label, nodes_[b].label) < 0;
        };
        std::stable_sort(roots_.begin(), roots_.end(), less);
        for (IndexNode& node : nodes_) std::stable_sort(node.children.begin(), node.children.end(), less);
    }

    // Keys: the path of the ids, made unique among the siblings.
    by_key_.clear();
    const auto assignKeys = [this](const std::vector<int>& siblings, const QString& prefix) {
        QSet<QString> used;
        for (int index : siblings) {
            IndexNode& node = nodes_[index];
            QString id = node.id.isEmpty() ? QString::number(index) : node.id;
            if (used.contains(id)) {
                int n = 2;
                while (used.contains(id + QStringLiteral("#%1").arg(n))) ++n;
                id += QStringLiteral("#%1").arg(n);
            }
            used.insert(id);
            node.key = prefix.isEmpty() ? id : prefix + QLatin1Char('/') + id;
            by_key_.insert(node.key, index);
        }
    };
    assignKeys(roots_, QString());
    // Parents are added before their children, so one pass in index order works.
    for (int index = 0; index < size(); ++index) {
        assignKeys(nodes_[index].children, nodes_[index].key);
    }

    // Multiplicity: the number of element nodes of the same identity.
    std::unordered_map<std::uint64_t, int> listed;
    for (const IndexNode& node : nodes_) {
        if (node.kind == IndexNode::Kind::Element) ++listed[identity(node)];
    }
    for (IndexNode& node : nodes_) {
        node.multiplicity = node.kind == IndexNode::Kind::Element ? listed[identity(node)] : 1;
    }

    // Counts: distinct elements below each folder, children before parents.
    // The largest child set is moved (merge) into the parent's instead of
    // being copied, so the total work stays close to O(total identities)
    // rather than O(sum of subtree sizes).
    std::vector<std::unordered_set<std::uint64_t>> below(nodes_.size());
    for (int index = size() - 1; index >= 0; --index) {
        IndexNode& node = nodes_[index];
        if (node.kind == IndexNode::Kind::Element) below[index].insert(identity(node));
        int biggest_child = -1;
        for (int child : node.children) {
            if (biggest_child < 0 || below[child].size() > below[biggest_child].size()) {
                biggest_child = child;
            }
        }
        if (biggest_child >= 0) below[index].merge(below[biggest_child]);
        for (int child : node.children) {
            if (child == biggest_child) continue;
            below[index].insert(below[child].begin(), below[child].end());
            below[child].clear();
        }
        node.count = static_cast<int>(below[index].size());
    }
}

int TreeIndex::find(const QString& key) const {
    return by_key_.value(key, -1);
}

QString TreeIndex::displayLabel(int index) const {
    const IndexNode& node = nodes_[index];
    if (node.kind == IndexNode::Kind::Folder) return QStringLiteral("%1 (%2)").arg(node.label).arg(node.count);
    if (node.kind == IndexNode::Kind::Element && node.multiplicity > 1) {
        return node.label + QString::fromUtf8(" ×%1").arg(node.multiplicity);
    }
    return node.label;
}

std::vector<std::uint32_t> TreeIndex::objectIdsBelow(int index) const {
    std::vector<std::uint32_t> ids;
    std::unordered_set<std::uint32_t> seen;
    std::vector<int> stack{index};
    while (!stack.empty()) {
        const int current = stack.back();
        stack.pop_back();
        const IndexNode& node = nodes_[current];
        if (node.object_id != 0 && seen.insert(node.object_id).second) ids.push_back(node.object_id);
        stack.insert(stack.end(), node.children.begin(), node.children.end());
    }
    return ids;
}

std::vector<char> TreeIndex::match(const QString& text, int* matches) const {
    const QString needle = text.trimmed();
    std::vector<char> shown(nodes_.size(), needle.isEmpty() ? 1 : 0);
    if (matches) *matches = 0;
    if (needle.isEmpty()) return shown;
    for (int index = 0; index < size(); ++index) {
        if (!nodes_[index].label.contains(needle, Qt::CaseInsensitive)) continue;
        if (matches) ++*matches;
        // The node, its ancestors...
        for (int up = index; up >= 0 && !shown[up]; up = nodes_[up].parent) shown[up] = 1;
        shown[index] = 1;
        // ...and everything below it.
        std::vector<int> stack(nodes_[index].children.begin(), nodes_[index].children.end());
        while (!stack.empty()) {
            const int current = stack.back();
            stack.pop_back();
            shown[current] = 1;
            stack.insert(stack.end(), nodes_[current].children.begin(), nodes_[current].children.end());
        }
    }
    return shown;
}

} // namespace bonsaiviewer::services::tree
