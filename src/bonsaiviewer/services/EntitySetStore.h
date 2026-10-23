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

#ifndef IFCINTERFACE_SERVICES_ENTITYSETSTORE_H
#define IFCINTERFACE_SERVICES_ENTITYSETSTORE_H

#include <QObject>

#include <array>
#include <cstdint>
#include <vector>

namespace bonsaiviewer { class SessionState; }

// B07 — EntitySetStore (see 00_Technical_Implementation/
// BonsaiViewerTI00). Named element sets with session lifetime: the
// current Box and its three slots. Set algebra: replace / add / remove
// on the current set, with ids kept valid against the element registry.
// Everything is purged when a model set changes invalidates ids or when
// the project is reset (spec: session only, purged when a model closes).

namespace bonsaiviewer::services {

class EntitySetStore : public QObject {
    Q_OBJECT

public:
    static constexpr int kSlotCount = 3;

    explicit EntitySetStore(bonsaiviewer::SessionState& session,
                            QObject* parent = nullptr);

    void replaceCurrent(const std::vector<std::uint32_t>& ids);
    void addToCurrent(const std::vector<std::uint32_t>& ids);
    void removeFromCurrent(const std::vector<std::uint32_t>& ids);
    const std::vector<std::uint32_t>& current() const { return current_; }

    // Slots are 1-based (1..kSlotCount) to match the spec's wording.
    void saveSlot(int slot);
    const std::vector<std::uint32_t>& slot(int slot) const;

    // Drop the ids of one model (call on model removal). Also available
    // is the automatic purge: modelsChanged / projectReset are wired in
    // the constructor.
    void purgeModel(std::uint32_t session_model_id);
    void purgeInvalidIds();

signals:
    void currentChanged();
    void slotChanged(int slot);

private:
    static void intersectValid(bonsaiviewer::SessionState& session,
                               std::vector<std::uint32_t>& ids);

    bonsaiviewer::SessionState& session_;
    std::vector<std::uint32_t> current_;
    std::array<std::vector<std::uint32_t>, kSlotCount> slots_;
};

} // namespace bonsaiviewer::services

#endif
