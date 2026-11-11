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

#ifndef IFCINTERFACE_MODULES_RULE_RULETREE_H
#define IFCINTERFACE_MODULES_RULE_RULETREE_H

#include "RuleDocument.h"

#include <QHash>
#include <QMap>
#include <QSet>
#include <QTreeWidget>

#include <memory>

namespace bonsaiviewer::modules::rule {

// What a tree item shows.
struct TreeRef {
    enum class Kind { File, Folder, Rule, Slot, Facet, Link };
    Kind kind = Kind::File;
    std::shared_ptr<clash::RuleFolder> folder;
    std::shared_ptr<clash::RuleCheck> rule;  // the rule, or the owner of the slot / facet / link
    bool nested = false;                     // a rule nested by value in a slot
    QString slot;                            // source, target, context
    int facet = -1;

    clash::Select* select() const; // the slot's select (Slot, Facet, Link)
};

// BonsaiViewer09 (TI09) — the Rule tree of the IfcViewer_Clash branch on
// the rule model: RuleFile > folders (check box: active) > rules (type
// badge, cascade badge, results) > slots (source / target / context) >
// facets, a rule nested in the slot, or a link to another rule of the tree.
// Rules and IDS facets are dropped from the Rule Library; rules and folders
// move by drag and drop; a rule dragged on a slot of another rule feeds it.
class RuleTreeWidget : public QTreeWidget {
    Q_OBJECT

public:
    RuleTreeWidget(RuleDocument& document, QWidget* parent = nullptr);

    void rebuild();
    std::optional<TreeRef> refOf(QTreeWidgetItem* item) const;
    std::optional<TreeRef> currentRef() const;
    // The nodes to run for the selection (rules and folders, tree order);
    // empty when nothing runnable is selected.
    std::vector<clash::RuleNode> selectedNodes() const;
    // "12 results", "error"... per node id, shown in the Results column.
    void setResultTexts(const QMap<QString, QString>& texts);

    // Creation helpers (toolbar, menus, drops).
    std::shared_ptr<clash::RuleCheck> createRule(const QString& type) const;
    void addRule(const QString& type, const QString& folder_id, int index = -1);
    void addFolder(const QString& parent_folder_id);
    // The folder that new nodes go into for the current item (empty: the file).
    QString targetFolderForCurrent() const;

signals:
    void refSelected(const std::optional<TreeRef>& ref);
    void runRequested(const std::vector<clash::RuleNode>& nodes);
    void editRuleRequested(std::shared_ptr<clash::RuleCheck> rule);
    void editFolderRequested(std::shared_ptr<clash::RuleFolder> folder);
    void selectionRequested(const clash::Select& select, const QString& title);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    QMimeData* mimeData(const QList<QTreeWidgetItem*>& items) const override;

private:
    QTreeWidgetItem* addNode(QTreeWidgetItem* parent, const clash::RuleNode& node);
    QTreeWidgetItem* addRuleItem(QTreeWidgetItem* parent, const std::shared_ptr<clash::RuleCheck>& rule, bool nested,
                                 const QString& key);
    QString keyOf(QTreeWidgetItem* item) const;
    void dropLibraryItem(const QString& payload, QTreeWidgetItem* target);
    void dropTreeItem(QTreeWidgetItem* dragged, QTreeWidgetItem* target);
    void addRuleMenu(QMenu* menu, const QString& folder_id);
    void removeRef(const TreeRef& ref);

    RuleDocument& document_;
    QHash<QTreeWidgetItem*, TreeRef> refs_;
    QHash<QTreeWidgetItem*, QString> keys_;
    QSet<QString> collapsed_;
    QMap<QString, QString> result_texts_;
    std::shared_ptr<clash::RuleCheck> clipboard_;
};

// The Rule panel: the spec 09 toolbar (Save, Load, New, Create Rule,
// Create Folder, Run) above the tree.
class RulePanel : public QWidget {
    Q_OBJECT

public:
    RulePanel(RuleDocument& document, QWidget* parent = nullptr);
    RuleTreeWidget* tree() const { return tree_; }

signals:
    void saveRequested();
    void loadRequested();

private:
    RuleDocument& document_;
    RuleTreeWidget* tree_ = nullptr;
};

// The MIME type of the Rule Library drags: "rule:<type>" or "facet:<Type>".
inline const char* kLibraryMime = "application/x-bonsai-clash-library";
inline const char* kTreeMime = "application/x-bonsai-clash-tree";

} // namespace bonsaiviewer::modules::rule

#endif // IFCINTERFACE_MODULES_RULE_RULETREE_H
