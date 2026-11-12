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

#ifndef IFCINTERFACE_SERVICES_TREEINDEX_H
#define IFCINTERFACE_SERVICES_TREEINDEX_H

#include <QHash>
#include <QString>

#include <cstdint>
#include <vector>

namespace bonsaiviewer::services::tree {

// BonsaiViewer32 — the node table behind one tree of the Tree Explorer. A
// builder adds the nodes (folders, elements, messages), then finish()
// computes what every tree shares: the number of distinct elements below a
// folder, the "×N" mark of an element listed in several places, the unique
// key of each node. Qt Core only, so it is unit tested on its own.
struct IndexNode {
    enum class Kind { Folder, Element, Message };
    Kind kind = Kind::Element;
    // Unique among its siblings; the node key is the path of the ids.
    QString id;
    QString label;
    QString icon;
    QString tooltip;
    // The IFC entity behind the node (Show properties), 0 when none.
    std::uint32_t model = 0;
    int ifc_id = 0;
    // The viewport object of the node (elements, spaces...), 0 when none.
    std::uint32_t object_id = 0;
    // Open when first shown.
    bool expanded = false;

    // Filled by TreeIndex.
    int parent = -1;
    std::vector<int> children;
    QString key;
    int count = 0;         // Folder: distinct elements below
    int multiplicity = 1;  // Element: how many times it is listed
};

class TreeIndex {
public:
    // Adds a node under `parent` (-1: a root) and returns its index.
    int add(int parent, IndexNode node);
    // Sorts the children (natural order, folders and elements mixed by
    // label) unless `sort` is false, then computes the keys, counts and
    // multiplicities. Call once, after the last add.
    void finish(bool sort = true);

    int size() const { return static_cast<int>(nodes_.size()); }
    const IndexNode& node(int index) const { return nodes_[index]; }
    const std::vector<int>& roots() const { return roots_; }
    // -1 when no node has this key.
    int find(const QString& key) const;

    // The label shown: "Name (N)" for a folder, "Name ×N" for an element
    // listed N > 1 times.
    QString displayLabel(int index) const;

    // Every object id at or below a node, each once.
    std::vector<std::uint32_t> objectIdsBelow(int index) const;

    // The search: true for the nodes whose label contains `text` (case
    // insensitive), their ancestors and their descendants. Empty text shows
    // everything.
    std::vector<char> match(const QString& text, int* matches = nullptr) const;

private:
    // The identity of an element for the counts: its object id, else its
    // model and IFC id.
    static std::uint64_t identity(const IndexNode& node);

    std::vector<IndexNode> nodes_;
    std::vector<int> roots_;
    QHash<QString, int> by_key_;
};

} // namespace bonsaiviewer::services::tree

#endif
