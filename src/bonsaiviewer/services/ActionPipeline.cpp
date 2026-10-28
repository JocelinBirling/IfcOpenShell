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

#include <algorithm>
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
    appearance_.setOpacity(ids, 100 - std::clamp(percent, 0, 100));
}

void ActionPipeline::clearAppearance() {
    appearance_.clearOverrides();
}

void ActionPipeline::forcedColour(const std::vector<std::uint32_t>& ids, const QColor& colour) {
    appearance_.setLayerColour(ids, colour, AppearanceService::Layer::ForcedFilter);
}

void ActionPipeline::forcedTransparency(const std::vector<std::uint32_t>& ids, int percent) {
    appearance_.setForcedOpacity(ids, 100 - std::clamp(percent, 0, 100));
}

void ActionPipeline::clearForced() {
    appearance_.clearForced();
}

QColor ActionPipeline::generatedColour(int index) {
    // Golden-ratio hue stepping: distinct, stable, and spread however many
    // groups the property produces.
    const double hue = std::fmod(0.61803398875 * double(index), 1.0);
    return QColor::fromHsvF(hue, 0.65, 0.95);
}

ActionPipeline::ElementValues ActionPipeline::readValues(const std::vector<std::uint32_t>& ids,
                                                         const std::string& pset,
                                                         const std::string& property) const {
    // Property sets first, then quantity sets, as the Properties panel reads them.
    auto find = [&pset, &property](const element_properties& sets) -> std::optional<std::string> {
        const auto set = sets.find(pset);
        if (set == sets.end()) return std::nullopt;
        const auto value = set->second.find(property);
        if (value == set->second.end() || property == "id") return std::nullopt;
        std::ostringstream formatted;
        formatted << value->second;
        return formatted.str();
    };
    ElementValues values;
    values.reserve(ids.size());
    for (std::uint32_t id : ids) {
        std::optional<std::string> value = find(data_access_.propertySets(id));
        if (!value) value = find(data_access_.quantitySets(id));
        values.emplace_back(id, std::move(value));
    }
    return values;
}

std::unordered_map<std::uint32_t, QColor> ActionPipeline::autoColourByProperty(
    const std::vector<std::uint32_t>& ids, const std::string& pset, const std::string& property) {
    return autoColourByValues(readValues(ids, pset, property));
}

std::unordered_map<std::uint32_t, QColor> ActionPipeline::autoColourByValues(const ElementValues& values,
                                                                             bool forced) {
    const auto layer = forced ? AppearanceService::Layer::ForcedFilter : AppearanceService::Layer::FilterAction;
    std::map<std::string, std::vector<std::uint32_t>> groups;
    std::vector<std::uint32_t> without_value;
    for (const auto& [id, value] : values) {
        if (value) {
            groups[*value].push_back(id);
        } else {
            without_value.push_back(id);
        }
    }
    std::unordered_map<std::uint32_t, QColor> colours;
    int index = 0;
    for (const auto& [value, group] : groups) {
        static_cast<void>(value);
        const QColor colour = generatedColour(index);
        appearance_.setLayerColour(group, colour, layer);
        for (std::uint32_t id : group) colours[id] = colour;
        ++index;
    }
    if (!without_value.empty() && !forced) {
        const QColor grey(128, 128, 128);
        appearance_.setLayerColour(without_value, grey, AppearanceService::Layer::FilterAction);
        for (std::uint32_t id : without_value) colours[id] = grey;
    }
    return colours;
}

} // namespace bonsaiviewer::services
