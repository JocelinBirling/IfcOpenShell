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

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    QTreeWidgetItem* createRuleItem(const QString& name);
    void addSlot(QTreeWidgetItem* rule_item, const QString& slot_name);

private:
    int file_counter_ = 0;
    int folder_counter_ = 0;
};

class RulePanel : public QWidget {
    Q_OBJECT
public:
    explicit RulePanel(QWidget* parent = nullptr);

signals:
    // Emitted when the tree selection changes; carries the selected node
    // (RuleFile, folder or rule instance) or null when nothing is selected.
    void ruleNodeSelected(QTreeWidgetItem* item);

private:
    RuleTreeWidget* tree_ = nullptr;
};

} // namespace bonsaiviewer::modules::rule

#endif
