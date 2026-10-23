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

#include "QueryEngine.h"

#include "ActiveView.h"
#include "EntitySetStore.h"
#include "IfcDataAccess.h"

#include "../ElementRegistry.h"
#include "../SessionState.h"

#include <QRegularExpression>

#include <sstream>
#include <unordered_set>

namespace bonsaiviewer::services {

namespace {

// Split on whitespace, keeping "..." and '...' groups intact.
QStringList tokenize(const QString& input) {
    QStringList tokens;
    QString current;
    QChar quote;
    for (const QChar& c : input) {
        if (!quote.isNull()) {
            if (c == quote) quote = QChar();
            else current.append(c);
            continue;
        }
        if (c == '"' || c == '\'') {
            quote = c;
            continue;
        }
        if (c.isSpace()) {
            if (!current.isEmpty()) tokens.append(current);
            current.clear();
            continue;
        }
        current.append(c);
    }
    if (!current.isEmpty()) tokens.append(current);
    return tokens;
}

bool contains(const QString& haystack, const QString& needle, Qt::CaseSensitivity cs) {
    return haystack.contains(needle, cs);
}

} // namespace

QueryEngine::QueryEngine(bonsaiviewer::SessionState& session,
                          PropertyIndex& index,
                          IfcDataAccess& data_access)
    : session_(session), index_(index), data_access_(data_access) {}

QueryEngine::Query QueryEngine::parse(const QString& input) {
    Query query;
    QStringList words;
    for (const QString& token : tokenize(input)) {
        if (token == QLatin1String("-fs")) query.full_search = true;
        else if (token == QLatin1String("-case")) query.case_sensitive = true;
        else if (token == QLatin1String("-view")) query.scope = Scope::ActiveView;
        else if (token == QLatin1String("-box")) query.scope = Scope::Box;
        else if (token == QLatin1String("-add")) query.action = Action::Add;
        else if (token == QLatin1String("-rm")) query.action = Action::Remove;
        else if (token == QLatin1String("-prop")) words.append(QStringLiteral("\x01-prop\x01"));
        else words.append(token);
    }
    // The token following the -prop marker is the property name.
    for (int i = 0; i < words.size(); ++i) {
        if (words[i] == QLatin1String("\x01-prop\x01")) {
            if (i + 1 < words.size()) {
                query.property = words[i + 1];
                words.removeAt(i + 1);
            }
            words.removeAt(i);
            break;
        }
    }
    query.text = words.join(QLatin1Char(' '));
    return query;
}

std::vector<std::uint32_t> QueryEngine::evaluate(const Query& query) const {
    if (query.text.isEmpty()) return {};

    const auto cs = query.case_sensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
    const auto matches = [&query, cs](const QString& value) {
        return contains(value, query.text, cs);
    };

    std::vector<std::uint32_t> ids;
    const std::vector<std::uint32_t> scope = scopeIds(query);

    // The cheap path: substring over the indexed Name / IfcClass fields.
    // PropertyIndex::search is whole-session and case-insensitive, so it
    // only covers the default scope-less query; other modes walk B05.
    if (!query.case_sensitive && !query.full_search && query.property.isEmpty()
        && query.scope == Scope::WholeModel) {
        return index_.search(query.text);
    }

    std::unordered_set<std::uint32_t> seen;
    for (std::uint32_t id : scope) {
        bool matched = false;
        if (!query.property.isEmpty()) {
            // "Pset.name" restricts to one property.
            const int dot = query.property.indexOf(QLatin1Char('.'));
            const QString set_name = dot > 0 ? query.property.left(dot) : query.property;
            const QString prop_name = dot > 0 ? query.property.mid(dot + 1) : QString();
            for (const auto& [pset, values] : data_access_.propertySets(id)) {
                const QString set_key = QString::fromStdString(pset);
                if (!contains(set_key, set_name, Qt::CaseInsensitive)) continue;
                for (const auto& [key, value] : values) {
                    if (!prop_name.isEmpty()
                        && !contains(QString::fromStdString(key), prop_name, Qt::CaseInsensitive)) continue;
                    std::ostringstream formatted;
                    formatted << value;
                    if (matches(QString::fromStdString(formatted.str()))) { matched = true; break; }
                }
                if (matched) break;
            }
        } else {
            if (const auto name = data_access_.name(id)) {
                matched = matches(*name);
            }
            if (!matched) {
                if (const auto ifc_class = data_access_.ifcClass(id)) {
                    matched = matches(*ifc_class);
                }
            }
            if (!matched && query.full_search) {
                const auto walk = [&matches](const element_properties& sets) {
                    for (const auto& [pset, values] : sets) {
                        for (const auto& [key, value] : values) {
                            std::ostringstream formatted;
                            formatted << value;
                            if (matches(QString::fromStdString(formatted.str()))) return true;
                        }
                    }
                    return false;
                };
                matched = walk(data_access_.propertySets(id))
                       || walk(data_access_.quantitySets(id));
            }
        }
        if (matched && seen.insert(id).second) {
            ids.push_back(id);
        }
    }
    return ids;
}

std::vector<std::uint32_t> QueryEngine::scopeIds(const Query& query) const {
    std::vector<std::uint32_t> ids;
    switch (query.scope) {
        case Scope::ActiveView:
            ids = active_view_ ? active_view_->visibleIds()
                              : std::vector<std::uint32_t>{};
            break;
        case Scope::Box:
            ids = store_ ? store_->current() : std::vector<std::uint32_t>{};
            break;
        case Scope::WholeModel:
            for (std::uint32_t session_model_id : session_.sessionModelIds()) {
                for (const auto& info :
                     session_.elementRegistry()->basicElementInfoForModel(session_model_id)) {
                    ids.push_back(info.object_id);
                }
            }
            break;
    }
    return ids;
}

} // namespace bonsaiviewer::services
