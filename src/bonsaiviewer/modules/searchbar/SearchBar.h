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

#ifndef IFCINTERFACE_MODULES_SEARCHBAR_H
#define IFCINTERFACE_MODULES_SEARCHBAR_H

#include <QWidget>

class QLineEdit;
class QToolButton;

namespace bonsaiviewer::services {
class ActionPipeline;
class QueryEngine;
}

// TI03 — Search Bar (see 00_Technical_Implementation/
// BonsaiViewerTI03). QLineEdit + magnifier button in the left corner
// of the top band. Enter or the magnifier runs the query: B06 parses
// the prefix grammar and evaluates it against B05, the result set is
// routed through B08 (default isolate, -add, -rm), and a "No results"
// popup (B17) leaves the view unchanged. The prefix cheat sheet shows
// on hover.

namespace bonsaiviewer::modules::searchbar {

class SearchBar : public QWidget {
    Q_OBJECT

public:
    SearchBar(bonsaiviewer::services::QueryEngine* engine,
              bonsaiviewer::services::ActionPipeline* pipeline,
              QWidget* parent = nullptr);

private:
    void runSearch();

    bonsaiviewer::services::QueryEngine* engine_ = nullptr;
    bonsaiviewer::services::ActionPipeline* pipeline_ = nullptr;
    QLineEdit* edit_ = nullptr;
    QToolButton* magnifier_ = nullptr;
};

} // namespace bonsaiviewer::modules::searchbar

#endif
