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

#include "JoinEngine.h"

#include "../../SessionState.h"
#include "../../ElementRegistry.h"
#include "../../services/IfcDataAccess.h"
#include "../../services/PropertyCatalog.h"

#include <QSet>

#include <algorithm>
#include <cmath>
#include <type_traits>
#include <variant>

namespace bonsaiviewer::modules::external_documents {

namespace {

// A property_value flattened to the string the table would hold.
QString propertyValueToString(const property_value& value) {
    return std::visit([](const auto& v) -> QString {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>) return {};
        else if constexpr (std::is_same_v<T, bool>) return v ? QStringLiteral("true") : QStringLiteral("false");
        else if constexpr (std::is_same_v<T, std::int64_t>) return QString::number(v);
        else if constexpr (std::is_same_v<T, double>) {
            QString text = QString::number(v, 'g', 15);
            return text;
        } else if constexpr (std::is_same_v<T, std::string>) return QString::fromStdString(v);
        else return QString(); // entities, lists, maps: not joinable
    }, value.value);
}

// Every loaded object with its basic info, once.
std::vector<BasicElementInfo> allElements(SessionState& session) {
    std::vector<BasicElementInfo> out;
    auto* registry = session.elementRegistry();
    if (registry == nullptr) return out;
    for (std::uint32_t session_model_id : session.sessionModelIds()) {
        for (const BasicElementInfo& info : registry->basicElementInfoForModel(session_model_id)) {
            out.push_back(info);
        }
    }
    return out;
}

} // namespace

QString keyOfObject(std::uint32_t object_id, const JoinConfig& join,
                    SessionState& session, services::IfcDataAccess& data_access) {
    auto* registry = session.elementRegistry();
    const auto info = registry != nullptr ? registry->findBasicElementInfo(object_id) : std::nullopt;
    if (join.model_key == QLatin1String("Name")) return info ? info->name : QString();
    if (join.model_key == QLatin1String("Class")) return info ? info->type : QString();
    if (join.model_key == QLatin1String("Property")) {
        const element_properties psets = data_access.propertySets(object_id);
        const auto pset_it = psets.find(join.property_pset.toStdString());
        if (pset_it == psets.end()) return {};
        const auto prop_it = pset_it->second.find(join.property_name.toStdString());
        if (prop_it == pset_it->second.end()) return {};
        return propertyValueToString(prop_it->second);
    }
    // GlobalId (default).
    return info ? info->guid : QString();
}

JoinResult joinTable(const TableData& data, const JoinConfig& join,
                     SessionState& session, services::IfcDataAccess& data_access,
                     bool* key_column_missing) {
    if (key_column_missing != nullptr) *key_column_missing = false;
    JoinResult result;
    if (!data.ok) return result;

    const int key_column = data.columns.indexOf(join.table_key_column);
    if (key_column < 0) {
        // The key column is matched by name; when it is not among the
        // first column (default) and vanished from a changed file, the
        // document goes to the "key column missing" state.
        if (!join.table_key_column.isEmpty() && data.columns.size() > 1) {
            if (key_column_missing != nullptr) *key_column_missing = true;
        } else if (data.columns.isEmpty()) {
            if (key_column_missing != nullptr) *key_column_missing = true;
        }
        return result;
    }

    // key value -> first object id carrying it.
    QHash<QString, std::uint32_t> key_to_object;
    QHash<QString, int> key_use_count;
    std::vector<std::pair<std::uint32_t, QString>> object_keys;
    const auto elements = allElements(session);
    object_keys.reserve(elements.size());
    for (const BasicElementInfo& info : elements) {
        const QString key = keyOfObject(info.object_id, join, session, data_access);
        object_keys.emplace_back(info.object_id, key);
        if (!key.isEmpty() && !key_to_object.contains(key)) {
            key_to_object.insert(key, info.object_id);
        }
    }

    QSet<QString> matched_keys;
    for (int row = 0; row < data.rows.size(); ++row) {
        const QStringList& values = data.rows.at(row);
        const QString key = key_column < values.size() ? values.at(key_column) : QString();
        if (key.isEmpty()) {
            ++result.report.unmatched_rows;
            result.report.unmatched_row_values.append(values);
            continue;
        }
        const auto it = key_to_object.find(key);
        if (it == key_to_object.end()) {
            ++result.report.unmatched_rows;
            result.report.unmatched_row_values.append(values);
            continue;
        }
        if (key_use_count.value(key, 0) > 0) {
            // Duplicate key: only the first matching row is kept.
            ++result.report.duplicate_rows;
            result.report.duplicate_row_values.append(values);
            continue;
        }
        key_use_count.insert(key, 1);
        matched_keys.insert(key);
        result.row_to_object.insert(row, it.value());
        ++result.report.matched_rows;
    }

    for (const auto& [object_id, key] : object_keys) {
        if (key.isEmpty() || matched_keys.contains(key)) continue;
        UnmatchedObjectInfo info;
        info.object_id = object_id;
        info.key = key;
        if (const auto basic = session.elementRegistry()->findBasicElementInfo(object_id)) {
            info.ifc_class = basic->type;
            info.name = basic->name;
            info.global_id = basic->guid;
            info.model = session.modelIdForSessionModelId(basic->session_model_id);
        }
        result.report.unmatched_object_values.append(info);
    }
    result.report.unmatched_objects = int(result.report.unmatched_object_values.size());
    return result;
}

QList<JoinGuess> guessJoins(const TableData& data,
                            SessionState& session,
                            services::IfcDataAccess& data_access,
                            services::PropertyCatalog* catalog) {
    QList<JoinGuess> guesses;
    if (!data.ok || data.columns.isEmpty()) return guesses;

    constexpr int kSampleRows = 10;
    constexpr int kMinMatches = 8; // majority of the 10 sampled rows
    const int sampled = std::min<int>(kSampleRows, data.rows.size());

    // Key sets of the loaded models for the cheap model keys.
    QSet<QString> global_ids, names, classes;
    for (const BasicElementInfo& info : allElements(session)) {
        if (!info.guid.isEmpty()) global_ids.insert(info.guid);
        if (!info.name.isEmpty()) names.insert(info.name);
        if (!info.type.isEmpty()) classes.insert(info.type);
    }

    // Sample values of the catalog's properties (BonsaiViewer26).
    QHash<QString, QSet<QString>> property_values; // "pset|property" -> values
    if (catalog != nullptr) {
        catalog->ensureBuilt();
        const auto& samples = catalog->samples();
        for (auto pset_it = samples.begin(); pset_it != samples.end(); ++pset_it) {
            for (auto prop_it = pset_it->begin(); prop_it != pset_it->end(); ++prop_it) {
                QSet<QString> values;
                for (const QString& sample : prop_it.value()) values.insert(sample);
                property_values.insert(pset_it.key() + QLatin1Char('|') + prop_it.key(), values);
            }
        }
    }

    for (int column = 0; column < data.columns.size(); ++column) {
        int gid = 0, nme = 0, cls = 0;
        QHash<QString, int> prop_matches; // "pset|property" -> matches
        int non_empty = 0;
        for (int row = 0; row < sampled; ++row) {
            const QStringList& values = data.rows.at(row);
            const QString value = column < values.size() ? values.at(column).trimmed() : QString();
            if (value.isEmpty()) continue; // tolerated empty cells
            ++non_empty;
            if (global_ids.contains(value)) ++gid;
            if (names.contains(value)) ++nme;
            if (classes.contains(value)) ++cls;
            for (auto it = property_values.begin(); it != property_values.end(); ++it) {
                if (it.value().contains(value)) prop_matches[it.key()] += 1;
            }
        }
        const QString column_name = data.columns.at(column);
        if (gid >= kMinMatches) guesses.append({column_name, QStringLiteral("GlobalId"), {}, {}, gid});
        if (nme >= kMinMatches) guesses.append({column_name, QStringLiteral("Name"), {}, {}, nme});
        if (cls >= kMinMatches) guesses.append({column_name, QStringLiteral("Class"), {}, {}, cls});
        for (auto it = prop_matches.begin(); it != prop_matches.end(); ++it) {
            if (it.value() < kMinMatches) continue;
            const QStringList parts = it.key().split(QLatin1Char('|'));
            if (parts.size() != 2) continue;
            guesses.append({column_name, QStringLiteral("Property"), parts.at(0), parts.at(1), it.value()});
        }
    }

    // Best first.
    std::sort(guesses.begin(), guesses.end(),
              [](const JoinGuess& a, const JoinGuess& b) { return a.matches > b.matches; });
    return guesses;
}

} // namespace bonsaiviewer::modules::external_documents
