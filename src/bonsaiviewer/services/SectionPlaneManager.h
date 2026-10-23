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

#ifndef IFCINTERFACE_SERVICES_SECTIONPLANEMANAGER_H
#define IFCINTERFACE_SERVICES_SECTIONPLANEMANAGER_H

#include <QObject>

#include <cstdint>

class ViewportWindow;

// B11 — SectionPlaneManager (see 00_Technical_Implementation/
// BonsaiViewerTI00). Multiple section planes (origin + normal), all
// cutting simultaneously, with the two global toggles of the spec: hide
// the gizmos (the cuts stay) and suspend (gizmos and cuts both stop).
// The viewport stays the authority for the plane list and the drag
// interaction; the shell-side right-click "Delete plane" menu (TI02,
// via B15) resolves the plane under the cursor with hitTest.

namespace bonsaiviewer::services {

class SectionPlaneManager : public QObject {
    Q_OBJECT

public:
    explicit SectionPlaneManager(ViewportWindow& viewport, QObject* parent = nullptr);

    // The viewport flips the normal toward the camera so the first cut
    // reveals the clicked surface; false when the plane cap is reached.
    bool addPlane(const float point[3], const float normal[3]);
    void removePlaneAt(int index);
    void clear();
    int count() const;

    void setGizmosHidden(bool hidden);
    bool gizmosHidden() const;
    void setSuspended(bool suspended);
    bool suspended() const;

    // Plane gizmo under (x, y) logical pixels, -1 for none.
    int hitTest(int x, int y) const;

signals:
    void planesChanged();

private:
    ViewportWindow& viewport_;
};

} // namespace bonsaiviewer::services

#endif
