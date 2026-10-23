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

#ifndef IFCINTERFACE_SERVICES_STEPTEXTSERVICE_H
#define IFCINTERFACE_SERVICES_STEPTEXTSERVICE_H

#include <QHash>
#include <QObject>
#include <QString>

#include <cstdint>
#include <map>
#include <vector>

namespace bonsaiviewer { class SessionState; }

// B16 — StepTextService (see 00_Technical_Implementation/
// BonsaiViewerTI00). Raw STEP line access per entity: the raw line
// (#0223=IFCWALL(#2,#3,Text)), hashtag parsing, and the
// declaration-to-usages navigation. Lines are re-read from the parsed
// file on demand (the memory-safe default of TI16); the usage index is
// built lazily per model on the first usagesOf call, from the lines
// themselves, and always returns ascending order.

namespace bonsaiviewer::services {

class StepTextService : public QObject {
    Q_OBJECT

public:
    explicit StepTextService(bonsaiviewer::SessionState& session, QObject* parent = nullptr);

    // "#0223=IFCWALL(#2,#3,Text)" for the IFC entity id, from the model's
    // live data source. Empty when the source is not available yet.
    QString lineFor(std::uint32_t session_model_id, int ifc_id);
    // Same, resolving the object id through the element registry first.
    QString lineForObject(std::uint32_t object_id);

    // Entity ids that reference #ifc_id, ascending. The index is built
    // once per model on first use.
    std::vector<int> usagesOf(std::uint32_t session_model_id, int ifc_id);

    // The hashtag ids referenced by a line (without the declaration id),
    // ascending, deduplicated.
    static std::vector<int> parseHashtags(const QString& line);

    // True when the model's data source is ready for queries.
    bool modelAvailable(std::uint32_t session_model_id) const;

private:
    void buildUsageIndex(std::uint32_t session_model_id);
    void dropModel(std::uint32_t session_model_id);

    bonsaiviewer::SessionState& session_;
    // model id -> (referenced id -> sorted user ids)
    QHash<std::uint32_t, std::map<int, std::vector<int>>> usage_indexes_;
};

} // namespace bonsaiviewer::services

#endif
