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

#include "ActionPipeline.h"

#include "ActiveView.h"
#include "AppearanceService.h"
#include "IfcDataAccess.h"

#include <cmath>
#include <sstream>

namespace bonsaiviewer::services {

ActionPipeline::ActionPipeline(ActiveView& active_view,
                               AppearanceService& appearance,
                               IfcDataAccess& data_access)
    : active_view_(active_view),
      appearance_(appearance),
      data_access_(data_access) {}

void ActionPipeline::run(const std::vector<std::uint32_t>& ids, Action action) {
    switch (action) {
        case Action::Isolate:
            active_view_.isolate(ids);
            break;
        case Action::AddToView:
            active_view_.add(ids);
            break;
        case Action::RemoveFromView:
            active_view_.remove(ids);
            break;
    }
}

void ActionPipeline::colour(const std::vector<std::uint32_t>& ids, const QColor& colour) {
    appearance_.setColour(ids, colour, AppearanceService::Layer::UserPaint);
}

void ActionPipeline::transparency(const std::vector<std::uint32_t>& ids, int percent) {
    appearance_.setOpacity(ids, percent);
}

QColor ActionPipeline::generatedColour(int index) {
    // Golden-ratio hue stepping: distinct, stable, and spread however many
    // groups the property produces.
    const double hue = std::fmod(0.61803398875 * double(index), 1.0);
    return QColor::fromHsvF(hue, 0.65, 0.95);
}

void ActionPipeline::autoColourByProperty(const std::vector<std::uint32_t>& ids,
                                          const std::string& pset,
                                          const std::string& property) {
    // Group by the property's value (string comparison of the formatted
    // value — TI14's typed-comparison refinement lands with that fiche).
    std::map<std::string, std::vector<std::uint32_t>> groups;
    std::vector<std::uint32_t> without_value;
    for (std::uint32_t id : ids) {
        bool found = false;
        for (const auto& [set_name, values] : data_access_.propertySets(id)) {
            if (set_name != pset) continue;
            const auto it = values.find(property);
            if (it != values.end()) {
                std::ostringstream formatted;
                formatted << it->second;
                groups[formatted.str()].push_back(id);
                found = true;
                break;
            }
        }
        if (!found) without_value.push_back(id);
    }
    int index = 0;
    for (const auto& [value, group] : groups) {
        static_cast<void>(value);
        appearance_.setLayerColour(group, generatedColour(index),
                                   AppearanceService::Layer::FilterAction);
        ++index;
    }
    if (!without_value.empty()) {
        appearance_.setLayerColour(without_value, QColor(128, 128, 128),
                                    AppearanceService::Layer::FilterAction);
    }
}

} // namespace bonsaiviewer::services
