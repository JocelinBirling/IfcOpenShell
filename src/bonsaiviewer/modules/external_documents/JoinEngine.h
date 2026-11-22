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

#ifndef BONSAIVIEWER_MODULES_EXTERNAL_DOCUMENTS_JOINENGINE_H
#define BONSAIVIEWER_MODULES_EXTERNAL_DOCUMENTS_JOINENGINE_H

#include "Types.h"
#include "TableFile.h"

#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>

#include <cstdint>
#include <utility>
#include <vector>

namespace bonsaiviewer {
class SessionState;
namespace services {
class IfcDataAccess;
class PropertyCatalog;
}
} // namespace bonsaiviewer

namespace bonsaiviewer::modules::external_documents {

// BonsaiViewer35 — the join between an external table and the loaded models:
// one column of the file matched against one identifier of the objects
// (GlobalId, Name, Class or a property). One object per row: with duplicate
// key values only the first matching row is kept, the others are listed in
// the report.

struct UnmatchedObjectInfo {
    std::uint32_t object_id = 0;
    QString model;
    QString ifc_class;
    QString name;
    QString global_id;
    QString key;
};

struct MatchReport {
    int matched_rows = 0;
    int unmatched_rows = 0;
    int duplicate_rows = 0;
    int unmatched_objects = 0;
    // The file's columns for the unmatched and duplicate rows.
    QList<QStringList> unmatched_row_values;
    QList<QStringList> duplicate_row_values;
    QList<UnmatchedObjectInfo> unmatched_object_values;
};

struct JoinResult {
    // Row index (in TableData::rows) -> joined object id. Only the first
    // row of a duplicated key is present.
    QHash<int, std::uint32_t> row_to_object;
    MatchReport report;
};

// The key of one object for the configured model key.
QString keyOfObject(std::uint32_t object_id, const JoinConfig& join,
                    SessionState& session, services::IfcDataAccess& data_access);

// The model's display name (its file name by default) for a session model —
// modelIdForSessionModelId returns the internal GUID id, not a label.
QString modelDisplayName(const SessionState& session, std::uint32_t session_model_id);

// Joins `data` on `join.table_key_column`. The key column is matched by
// name; when it disappeared from the file the result is empty and
// `key_column_missing` is true.
JoinResult joinTable(const TableData& data, const JoinConfig& join,
                     SessionState& session, services::IfcDataAccess& data_access,
                     bool* key_column_missing = nullptr);

// "Get Lucky": reads the first 10 rows of the file and proposes the
// (key column, model key) pairs that match, best first. A column matches a
// model key when a majority of its 10 values (>= 8, tolerating empty cells)
// match distinct loaded objects. Property candidates come from the property
// catalog's sample values (BonsaiViewer26).
struct JoinGuess {
    QString table_key_column;
    QString model_key; // GlobalId | Name | Class | Property
    QString property_pset;
    QString property_name;
    int matches = 0; // of the 10 sampled rows
};
QList<JoinGuess> guessJoins(const TableData& data,
                            SessionState& session,
                            services::IfcDataAccess& data_access,
                            services::PropertyCatalog* catalog);

} // namespace bonsaiviewer::modules::external_documents

#endif // BONSAIVIEWER_MODULES_EXTERNAL_DOCUMENTS_JOINENGINE_H
