// This file was generated with the assistance of an AI coding tool.
/********************************************************************************
 *                                                                              *
 * This file is part of Bonsai.                                                 *
 *                                                                              *
 * Bonsai is free software: you can redistribute it and/or modify               *
 * it under the terms of the GNU General Public License as published by         *
 * the Free Software Foundation, either version 3.0 of the License, or          *
 * (at your option) any later version.                                           *
 * Bonsai is distributed in the hope that it will be useful,                    *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the               *
 * GNU General Public License for more details.                                 *
 *                                                                              *
 * You should have received a copy of the GNU General Public License           *
 * along with this program. If not, see <http://www.gnu.org/licenses/>.         *
 ********************************************************************************/

#ifndef IFCINTERFACE_MODULES_RULE_RESULT_INFO_PANEL_H
#define IFCINTERFACE_MODULES_RULE_RESULT_INFO_PANEL_H

#include "ResultPanel.h"

#include <QWidget>

namespace bonsaiviewer::modules::rule {

// The "Result Info" panel of the Clash Filter workspace: the details of the
// result currently selected in the Result panel — the source and target
// names, and what the run carried (rule name, tolerance, models, verdict).
class ResultInfoPanel : public QWidget {
    Q_OBJECT
public:
    explicit ResultInfoPanel(QWidget* parent = nullptr);

    // Shows the result details; a null pointer clears the panel.
    void showResult(const ResultRow* row);
    void showResultByIndex(const std::vector<ResultRow>* rows, int index);

private:
    void clear();
    QWidget* body_ = nullptr;
};

} // namespace bonsaiviewer::modules::rule

#endif
