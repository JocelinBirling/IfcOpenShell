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

#include "CategoryOverrideFilter.h"

#include "../ElementRegistry.h"
#include "../SessionState.h"
#include "../ifcviewer/ViewportWindow.h"
#include "PersistenceService.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace bonsaiviewer::services {

namespace {
constexpr const char* kDocumentName = "category-overrides.json";
}

CategoryOverrideFilter::CategoryOverrideFilter(SessionState& session,
                                               ViewportWindow& viewport,
                                               QObject* parent)
    : QObject(parent), session_(session), viewport_(viewport) {
    load();
}

bool CategoryOverrideFilter::isCategoryEnabled(const QString& ifc_class) const {
    return !disabled_.contains(ifc_class);
}

void CategoryOverrideFilter::setCategoryEnabled(const QString& ifc_class, bool enabled) {
    const bool was_disabled = disabled_.contains(ifc_class);
    if (enabled && was_disabled) {
        disabled_.remove(ifc_class);
        save();
        emit categoryChanged(ifc_class, true);
        // The category returns to the view: unhide its objects so the
        // toggle-off is reversible without a Show All.
        viewport_.setObjectsVisible(objectIdsForCategory(ifc_class), true);
        session_.notifyVisibilityChanged();
    } else if (!enabled && !was_disabled) {
        disabled_.insert(ifc_class, true);
        save();
        emit categoryChanged(ifc_class, false);
        applyToViewport();
        session_.notifyVisibilityChanged();
    }
}

QStringList CategoryOverrideFilter::disabledCategories() const {
    QStringList result;
    for (auto it = disabled_.keyBegin(); it != disabled_.keyEnd(); ++it) {
        result.append(*it);
    }
    result.sort();
    return result;
}

bool CategoryOverrideFilter::isObjectAllowed(std::uint32_t object_id) const {
    const auto info = session_.elementRegistry()->findBasicElementInfo(object_id);
    if (!info) {
        return true;
    }
    return isCategoryEnabled(info->type);
}

void CategoryOverrideFilter::applyToViewport() {
    for (const QString& ifc_class : disabledCategories()) {
        viewport_.setObjectsVisible(objectIdsForCategory(ifc_class), false);
    }
}

std::vector<std::uint32_t> CategoryOverrideFilter::objectIdsForCategory(const QString& ifc_class) const {
    std::vector<std::uint32_t> ids;
    for (std::uint32_t session_model_id : session_.sessionModelIds()) {
        for (const auto& info : session_.elementRegistry()->basicElementInfoForModel(session_model_id)) {
            if (info.type == ifc_class) {
                ids.push_back(info.object_id);
            }
        }
    }
    return ids;
}

void CategoryOverrideFilter::load() {
    const auto document = PersistenceService::instance().loadDocument(QLatin1String(kDocumentName));
    if (!document || !document->isObject()) {
        return;
    }
    for (const auto& value : document->object().value(QStringLiteral("disabled")).toArray()) {
        if (value.isString()) {
            disabled_.insert(value.toString(), true);
        }
    }
}

void CategoryOverrideFilter::save() {
    QJsonArray disabled;
    for (const QString& ifc_class : disabledCategories()) {
        disabled.append(ifc_class);
    }
    QJsonObject root;
    root.insert(QStringLiteral("disabled"), disabled);
    PersistenceService::instance().saveDocument(QLatin1String(kDocumentName), QJsonDocument(root));
}

} // namespace bonsaiviewer::services
