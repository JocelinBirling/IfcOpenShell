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

#ifndef IFCINTERFACE_MODULES_RULE_CONFIG_PANEL_H
#define IFCINTERFACE_MODULES_RULE_CONFIG_PANEL_H

#include <QWidget>

class QTreeWidgetItem;
class QVBoxLayout;
class QWidget;

namespace bonsaiviewer::modules::rule {

struct RuleDefinition;

// Property editor for the current selection: a rule clicked in the Rule
// Library is shown read-only; a RuleFile / folder / rule instance selected
// in the Rule tree is shown editable and edits are written back to the item.
class RuleConfigPanel : public QWidget {
    Q_OBJECT
public:
    explicit RuleConfigPanel(QWidget* parent = nullptr);

public slots:
    void showDefinition(const QString& rule_name);
    void showInstance(QTreeWidgetItem* item);

private:
    void rebuild(const QString& title,
                 const RuleDefinition* definition,
                 QTreeWidgetItem* item);

    QVBoxLayout* body_layout_ = nullptr;
    QWidget* form_container_ = nullptr;
};

} // namespace bonsaiviewer::modules::rule

#endif
