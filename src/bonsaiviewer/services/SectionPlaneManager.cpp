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

#include "SectionPlaneManager.h"

#include "../ifcviewer/ViewportWindow.h"

#include <Eigen/Dense>

namespace bonsaiviewer::services {

SectionPlaneManager::SectionPlaneManager(ViewportWindow& viewport, QObject* parent)
    : QObject(parent), viewport_(viewport) {}

bool SectionPlaneManager::addPlane(const float point[3], const float normal[3]) {
    const Eigen::Vector3f p(point[0], point[1], point[2]);
    const Eigen::Vector3f n(normal[0], normal[1], normal[2]);
    if (!viewport_.addSectionPlaneAtSurface(p, n)) return false;
    emit planesChanged();
    return true;
}

void SectionPlaneManager::removePlaneAt(int index) {
    viewport_.removeSectionPlane(index);
    emit planesChanged();
}

void SectionPlaneManager::clear() {
    viewport_.clearSectionPlanes();
    emit planesChanged();
}

int SectionPlaneManager::count() const {
    return viewport_.sectionPlaneCount();
}

void SectionPlaneManager::setGizmosHidden(bool hidden) {
    viewport_.setSectionGizmosHidden(hidden);
}

bool SectionPlaneManager::gizmosHidden() const {
    return viewport_.sectionGizmosHidden();
}

void SectionPlaneManager::setSuspended(bool suspended) {
    viewport_.setSectionsSuspended(suspended);
}

bool SectionPlaneManager::suspended() const {
    return viewport_.sectionsSuspended();
}

int SectionPlaneManager::hitTest(int x, int y) const {
    return viewport_.hitTestSectionGizmo(x, y);
}

} // namespace bonsaiviewer::services
