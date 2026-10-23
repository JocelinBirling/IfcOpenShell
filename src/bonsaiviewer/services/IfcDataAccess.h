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

#ifndef IFCINTERFACE_SERVICES_IFCDATAACCESS_H
#define IFCINTERFACE_SERVICES_IFCDATAACCESS_H

#include <QObject>
#include <QHash>
#include <QString>

#include "../ifcparse/express.h"
#include "pset.h"

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace bonsaiviewer {
class ElementRegistry;
struct BasicElementInfo;
}

// B05 — IfcDataAccess + PropertyIndex (see 00_Technical_Implementation/
// BonsaiViewerTI00). IfcDataAccess is the uniform read path to element
// data (basic info, direct attributes, Psets, quantities) on top of
// ElementRegistry and the pset helpers. PropertyIndex is the per-model
// searchable index of the cheap fields (Name, IfcClass); the expensive
// property values stay on demand through IfcDataAccess. Consumers: the
// search bar (TI03), filter rules (TI06), the table (TI07), select by
// property (TI14), the class tree (TI12).

namespace bonsaiviewer::services {

class IfcDataAccess {
public:
    explicit IfcDataAccess(ElementRegistry& registry);

    std::optional<BasicElementInfo> basicInfo(std::uint32_t object_id) const;
    std::optional<QString> name(std::uint32_t object_id) const;
    std::optional<QString> ifcClass(std::uint32_t object_id) const;
    std::optional<express::base> entity(std::uint32_t object_id) const;

    // Pset_* and Qto_* sets, same shape as the properties panel: set name
    // to property name to value. Empty when the entity is unavailable.
    element_properties propertySets(std::uint32_t object_id) const;
    element_properties quantitySets(std::uint32_t object_id) const;
    std::vector<std::pair<std::string, std::string>> scalarAttributes(std::uint32_t object_id) const;

private:
    ElementRegistry& registry_;
};

class PropertyIndex : public QObject {
    Q_OBJECT

public:
    explicit PropertyIndex(ElementRegistry& registry, QObject* parent = nullptr);

    // (Re)index one model from the element registry's basic info. Call on
    // modelGeometryReady; the basic fields are cheap to harvest.
    void indexModel(std::uint32_t session_model_id);
    void removeModel(std::uint32_t session_model_id);

    // Substring search over the indexed fields (Name, IfcClass),
    // case-insensitive. Results across all indexed models, deduplicated.
    std::vector<std::uint32_t> search(const QString& needle) const;

    // Exact (case-insensitive) match of Name or IfcClass.
    std::vector<std::uint32_t> idsWithFieldValue(const QString& value) const;

private:
    struct ModelIndex {
        // object_id -> (Name, IfcClass)
        std::unordered_map<std::uint32_t, std::pair<QString, QString>> fields;
    };

    ElementRegistry& registry_;
    QHash<std::uint32_t, ModelIndex> models_;
};

} // namespace bonsaiviewer::services

#endif
