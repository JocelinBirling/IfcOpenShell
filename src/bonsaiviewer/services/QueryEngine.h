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

#ifndef IFCINTERFACE_SERVICES_QUERYENGINE_H
#define IFCINTERFACE_SERVICES_QUERYENGINE_H

#include <QString>

#include <cstdint>
#include <vector>

namespace bonsaiviewer { class SessionState; }
namespace bonsaiviewer::services {
class ActiveView;
class EntitySetStore;
class IfcDataAccess;
class PropertyIndex;
}

// B06 — QueryEngine (see 00_Technical_Implementation/
// BonsaiViewerTI00). Parses the search-bar prefix grammar and evaluates
// the query against B05. Prefixes: -fs (walk every property value),
// -prop "Pset.name" (single property), -case (case-sensitive), -view /
// -box (scope: active view / the box instead of the whole model), -add /
// -rm (action on the result set). Results are element ids; the actions
// themselves run through B08 ActionPipeline (TI03 wiring).

namespace bonsaiviewer::services {

class QueryEngine {
public:
    enum class Scope {
        WholeModel,
        ActiveView,
        Box,
    };
    enum class Action {
        Isolate,
        Add,
        Remove,
    };

    struct Query {
        QString text;
        bool case_sensitive = false;
        bool full_search = false;
        // "Pset.name" for -prop; empty otherwise.
        QString property;
        Scope scope = Scope::WholeModel;
        Action action = Action::Isolate;
    };

    QueryEngine(bonsaiviewer::SessionState& session,
                PropertyIndex& index,
                IfcDataAccess& data_access);

    // Parse the prefix grammar; anything that is not a known prefix is
    // search text. Unknown prefixes are treated as text so a typo still
    // searches (and gets found) rather than silently returning nothing.
    static Query parse(const QString& input);

    // Evaluate against B05 and return the matching ids (deduplicated,
    // registry-valid).
    std::vector<std::uint32_t> evaluate(const Query& query) const;

    void setActiveView(ActiveView* active_view) { active_view_ = active_view; }
    void setEntitySetStore(EntitySetStore* store) { store_ = store; }

private:
    std::vector<std::uint32_t> scopeIds(const Query& query) const;

    bonsaiviewer::SessionState& session_;
    PropertyIndex& index_;
    IfcDataAccess& data_access_;
    ActiveView* active_view_ = nullptr;
    EntitySetStore* store_ = nullptr;
};

} // namespace bonsaiviewer::services

#endif
