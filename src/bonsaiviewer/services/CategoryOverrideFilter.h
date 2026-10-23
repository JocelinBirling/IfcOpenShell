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

#ifndef IFCINTERFACE_SERVICES_CATEGORYOVERRIDEFILTER_H
#define IFCINTERFACE_SERVICES_CATEGORYOVERRIDEFILTER_H

#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>

#include <cstdint>
#include <vector>

class ViewportWindow;
namespace bonsaiviewer { class SessionState; }

// B03 — CategoryOverrideFilter (see 00_Technical_Implementation/
// BonsaiViewerTI00). Global per-IFC-class toggles (IfcSpace,
// IfcOpeningElement, IfcZone, ...). A disabled category is never shown,
// whatever the path: the filter is re-applied after every ActiveView
// mutation and after each model geometry becomes ready, so Show All /
// isolations cannot bring the objects back. The disabled set is persisted
// through PersistenceService (B14) and restored at startup.

namespace bonsaiviewer::services {

class CategoryOverrideFilter : public QObject {
    Q_OBJECT

public:
    CategoryOverrideFilter(SessionState& session, ViewportWindow& viewport, QObject* parent = nullptr);

    // Absent classes are enabled by default.
    bool isCategoryEnabled(const QString& ifc_class) const;
    void setCategoryEnabled(const QString& ifc_class, bool enabled);
    QStringList disabledCategories() const;

    // True when the object's IFC class is enabled (unknown objects pass).
    bool isObjectAllowed(std::uint32_t object_id) const;

    // Hide every object of a disabled category. Cheap when nothing is
    // disabled. Called by ActiveView after its mutations and on
    // modelGeometryReady.
    void applyToViewport();

signals:
    void categoryChanged(const QString& ifc_class, bool enabled);

private:
    std::vector<std::uint32_t> objectIdsForCategory(const QString& ifc_class) const;
    void load();
    void save();

    SessionState& session_;
    ViewportWindow& viewport_;
    QHash<QString, bool> disabled_;
};

} // namespace bonsaiviewer::services

#endif
