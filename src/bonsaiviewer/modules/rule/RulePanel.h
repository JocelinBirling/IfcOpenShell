// This file was generated with the assistance of an AI coding tool.
/********************************************************************************
 *                                                                              *
 * This file is part of Bonsai.                                                 *
 *                                                                              *
 * Bonsai is free software: you can redistribute it and/or modify               *
 * it under the terms of the GNU General Public License as published by         *
 * the Free Software Foundation, either version 3.0 of the License, or           *
 * (at your option) any later version.                                          *
 *                                                                              *
 * Bonsai is distributed in the hope that it will be useful,                     *
 * but WITHOUT ANY WARRANTY; without the implied warranty of                     *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                 *
 * GNU General Public License for more details.                                 *
 *                                                                              *
 * You should have received a copy of the GNU General Public License            *
 * along with this program. If not, see <http://www.gnu.org/licenses/>.         *
 ********************************************************************************/

#ifndef IFCINTERFACE_MODULES_RULE_PANEL_H
#define IFCINTERFACE_MODULES_RULE_PANEL_H

#include "Catalogue.h"

#include <QTreeWidget>
#include <QVariantMap>
#include <QWidget>

class QTreeWidgetItem;

namespace bonsaiviewer::modules::rule {

class RuleTreeWidget : public QTreeWidget {
    Q_OBJECT
public:
    explicit RuleTreeWidget(QWidget* parent = nullptr);

    static RuleNodeType nodeType(const QTreeWidgetItem* item);

    QTreeWidgetItem* createRuleFile();
    QTreeWidgetItem* createFolder(QTreeWidgetItem* parent_item = nullptr);
    void addRule(const QString& name, QTreeWidgetItem* container);

    // Load support (RuleFileJson): builds tree nodes carrying an explicit
    // engine node id and instance values.
    QTreeWidgetItem* createRuleFileWithId(const QString& label, const QString& node_id);
    QTreeWidgetItem* createFolderWithId(QTreeWidgetItem* parent_item, const QString& label,
                                       const QString& node_id, const QString& description);
    // A rule instance with its values; container falls back to the open
    // RuleFile. Returns null for an unknown catalogue key.
    QTreeWidgetItem* addRuleInstance(const QString& key, const QVariantMap& values,
                                     const QString& node_id, const QString& description,
                                     QTreeWidgetItem* container);
    // Fills a placeholder slot in place with a rule/facet instance and
    // returns it (the slot becomes a Rule node, its label underlined).
    QTreeWidgetItem* fillSlot(QTreeWidgetItem* slot_item, const QString& key,
                              const QVariantMap& values, const QString& node_id,
                              const QString& description, const QString& display_label);

    // True when the item is an instance of an IDS facet definition.
    static bool isFacetItem(const QTreeWidgetItem* item);

    // The single RuleFile of the tree, if any (the tree always keeps at
    // most one open: New RuleFile replaces the previous).
    QTreeWidgetItem* openRuleFile() const;

signals:
    // Context-menu / double-click intents; MainWindow owns the run
    // machinery and the viewport wiring.
    void ruleRunRequested(QTreeWidgetItem* item);       // File, Folder or Rule
    void displayInputRequested(QTreeWidgetItem* item);  // Rule
    void facetSelectionRequested(QTreeWidgetItem* item); // Facet
    void ruleDoubleClicked(QTreeWidgetItem* item);     // Rule or Facet

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    QTreeWidgetItem* createRuleItem(const QString& name);
    void addSlot(QTreeWidgetItem* rule_item, const QString& slot_name);
    QTreeWidgetItem* cloneTreeItem(const QTreeWidgetItem* source);
    // The single RuleFile of the tree, if any.
    QTreeWidgetItem* ruleFileItem() const;

private:
    int file_counter_ = 0;
    int folder_counter_ = 0;
    QTreeWidgetItem* clipboard_ = nullptr;  // deep copy, owned by the tree
};

class RulePanel : public QWidget {
    Q_OBJECT
public:
    explicit RulePanel(QWidget* parent = nullptr);

    // The rule tree; MainWindow walks it to build the engine's rule specs.
    RuleTreeWidget* tree() const { return tree_; }

signals:
    // Emitted when the tree selection changes; carries the selected node
    // (RuleFile, folder or rule instance) or null when nothing is selected.
    void ruleNodeSelected(QTreeWidgetItem* item);
    // Save/Load of the open RuleFile (integration spec section 9).
    void saveRequested();
    void loadRequested();

private:
    RuleTreeWidget* tree_ = nullptr;
};

} // namespace bonsaiviewer::modules::rule

#endif
