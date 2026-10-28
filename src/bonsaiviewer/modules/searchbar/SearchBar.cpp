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

#include "SearchBar.h"

#include "../../services/ActionPipeline.h"
#include "../../services/QueryEngine.h"
#include "../../services/PopupService.h"

#include <QHBoxLayout>
#include <QLineEdit>
#include <QToolButton>

namespace bonsaiviewer::modules::searchbar {

namespace {

// The cheat sheet of the spec's "advanced search commands", shown when
// hovering the field.
const char* kCheatSheet =
    "<b>Search</b> — substring, case-insensitive. Prefixes can be combined:"
    "<table cellpadding='2'>"
    "<tr><td><code>-fs</code></td><td>full search: walk every property value</td></tr>"
    "<tr><td><code>-prop \"Pset.name\"</code></td><td>search one property only</td></tr>"
    "<tr><td><code>-case</code></td><td>case-sensitive search</td></tr>"
    "<tr><td><code>-view</code></td><td>search only in the active view</td></tr>"
    "<tr><td><code>-box</code></td><td>search only in the active box</td></tr>"
    "<tr><td><code>-add</code></td><td>add the results to the view</td></tr>"
    "<tr><td><code>-rm</code></td><td>remove the results from the view</td></tr>"
    "</table>"
    "Without a prefix the results are isolated in the view.";

} // namespace

SearchBar::SearchBar(bonsaiviewer::services::QueryEngine* engine,
                     bonsaiviewer::services::ActionPipeline* pipeline,
                     QWidget* parent)
    : QWidget(parent), engine_(engine), pipeline_(pipeline) {
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(2);

    edit_ = new QLineEdit(this);
    edit_->setPlaceholderText(QStringLiteral("Search..."));
    edit_->setClearButtonEnabled(true);
    edit_->setToolTip(QString::fromUtf8(kCheatSheet));
    // Compact band: half the former width so the groups keep their room.
    // Spec 05: the search field is 130 x 30 px.
    edit_->setFixedSize(130, 30);
    layout->addWidget(edit_);

    magnifier_ = new QToolButton(this);
    magnifier_->setText(QStringLiteral("Search"));
    magnifier_->setIcon(QIcon(QStringLiteral(":/icons/search.svg")));
    magnifier_->setToolTip(QString::fromUtf8(kCheatSheet));
    layout->addWidget(magnifier_, 0);

    connect(edit_, &QLineEdit::returnPressed, this, &SearchBar::runSearch);
    connect(magnifier_, &QToolButton::clicked, this, &SearchBar::runSearch);
}

void SearchBar::focusSearch() {
    edit_->setFocus(Qt::ShortcutFocusReason);
    edit_->selectAll();
}

void SearchBar::runSearch() {
    if (!engine_ || !pipeline_) return;
    const QString text = edit_->text().trimmed();
    if (text.isEmpty()) return;

    const bonsaiviewer::services::QueryEngine::Query query =
        bonsaiviewer::services::QueryEngine::parse(text);
    const std::vector<std::uint32_t> ids = engine_->evaluate(query);
    if (ids.empty()) {
        // Spec: a « No results » popup leaves the view unchanged.
        bonsaiviewer::services::popup::information(this, QStringLiteral("No results"));
        return;
    }
    using Action = bonsaiviewer::services::ActionPipeline::Action;
    switch (query.action) {
        case bonsaiviewer::services::QueryEngine::Action::Isolate:
            pipeline_->run(ids, Action::Isolate);
            break;
        case bonsaiviewer::services::QueryEngine::Action::Add:
            pipeline_->run(ids, Action::AddToView);
            break;
        case bonsaiviewer::services::QueryEngine::Action::Remove:
            pipeline_->run(ids, Action::RemoveFromView);
            break;
    }
}

} // namespace bonsaiviewer::modules::searchbar
