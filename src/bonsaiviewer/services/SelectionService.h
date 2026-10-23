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

#ifndef IFCINTERFACE_SERVICES_SELECTIONSERVICE_H
#define IFCINTERFACE_SERVICES_SELECTIONSERVICE_H

#include <QObject>

#include <cstdint>
#include <unordered_set>
#include <vector>

class ViewportWindow;
namespace bonsaiviewer { class SessionState; }

// B01 — SelectionService (see 00_Technical_Implementation/
// BonsaiViewerTI00). Multi-object selection with operation modes and a
// source tag. The viewport keeps its own SelectionState and stays the
// authority for viewport-originated picks; this service is the shell-side
// facade for set-based selection from other sources (tree TI12, table
// TI07, search TI03) and broadcasts every change with its source.

namespace bonsaiviewer::services {

enum class SelectionSource {
    Viewport,
    Tree,
    Table,
    Search,
    Service,
};

enum class SelectionMode {
    Replace,
    Add,
    Remove,
};

class SelectionService : public QObject {
    Q_OBJECT

public:
    SelectionService(SessionState& session, ViewportWindow& viewport, QObject* parent = nullptr);

    // Set-based selection for non-viewport sources. Replace swaps the whole
    // selection; Add / Remove mutate the current set.
    void setSelection(const std::vector<std::uint32_t>& ids,
                      SelectionSource source,
                      SelectionMode mode = SelectionMode::Replace);

    // Wired to ViewportWindow::objectPicked: the viewport already mutated its
    // own selection, the service mirrors the active object into the session
    // (so the Properties panel keeps working) and broadcasts.
    void onViewportPicked(std::uint32_t object_id);

    const std::unordered_set<std::uint32_t>& selectionIds() const;
    std::vector<std::uint32_t> selectionIdsAsVector() const;
    std::uint32_t activeObjectId() const;

signals:
    void selectionChanged(const std::vector<std::uint32_t>& ids,
                          bonsaiviewer::services::SelectionSource source);

private:
    SessionState& session_;
    ViewportWindow& viewport_;
};

} // namespace bonsaiviewer::services

#endif
