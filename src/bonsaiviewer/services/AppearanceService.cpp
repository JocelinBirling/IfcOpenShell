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

#include "AppearanceService.h"

#include "../ElementRegistry.h"
#include "../SessionState.h"
#include "../ifcviewer/ViewportWindow.h"
#include "UndoStack.h"

#include <algorithm>

namespace bonsaiviewer::services {

namespace {

constexpr std::uint32_t kAlphaOnlyFlag = 1u;

std::uint32_t packRgba8(std::uint32_t r, std::uint32_t g, std::uint32_t b, std::uint32_t a) {
    return (a << 24) | (b << 16) | (g << 8) | r;
}

} // namespace

AppearanceService::AppearanceService(bonsaiviewer::SessionState& session,
                                     ViewportWindow& viewport,
                                     QObject* parent)
    : QObject(parent), session_(session), viewport_(viewport) {
    // Overrides do not survive a (re)load: the baked geometry comes back,
    // so any stored RGB/alpha would be stale (TI02: reset on model reload).
    connect(&session_, &bonsaiviewer::SessionState::modelGeometryReady, this,
            [this](std::uint32_t session_model_id) {
                purgeModel(session_model_id);
            });
    connect(&session_, &bonsaiviewer::SessionState::projectReset, this,
            [this]() { clearAll(); });
}

void AppearanceService::setColour(const std::vector<std::uint32_t>& ids,
                                  const QColor& colour, Layer layer) {
    if (undo_) undo_->record();
    setLayerColour(ids, colour, layer);
}

void AppearanceService::setLayerColour(const std::vector<std::uint32_t>& ids,
                                       const QColor& colour, Layer layer) {
    const std::uint32_t rgb8 =
        packRgba8(colour.red(), colour.green(), colour.blue(), 255);
    for (std::uint32_t id : ids) {
        ObjectState& state = states_[id];
        state.layer_set[static_cast<int>(layer)] = true;
        state.layer_rgb8[static_cast<int>(layer)] = rgb8;
    }
    applyToViewport(ids);
}

void AppearanceService::removeColour(const std::vector<std::uint32_t>& ids) {
    if (undo_) undo_->record();
    for (std::uint32_t id : ids) {
        auto it = states_.find(id);
        if (it == states_.end()) continue;
        it->second.layer_set[static_cast<int>(Layer::UserPaint)] = false;
        // Keep the state entry: the opacity byte may still be live.
    }
    applyToViewport(ids);
}

void AppearanceService::clearLayer(Layer layer) {
    std::vector<std::uint32_t> ids;
    for (auto& [id, state] : states_) {
        if (!state.layer_set[static_cast<int>(layer)]) continue;
        state.layer_set[static_cast<int>(layer)] = false;
        ids.push_back(id);
    }
    applyToViewport(ids);
}

void AppearanceService::setOpacity(const std::vector<std::uint32_t>& ids, int percent) {
    if (undo_) undo_->record();
    const std::uint8_t alpha = static_cast<std::uint8_t>(
        std::clamp(percent, 0, 100) * 255 / 100);
    std::vector<std::uint32_t> changed;
    changed.reserve(ids.size());
    for (std::uint32_t id : ids) {
        if (forced_opacity_.count(id)) continue; // an activated filter imposes it
        states_[id].opacity = alpha;
        changed.push_back(id);
    }
    applyToViewport(changed);
}

void AppearanceService::setForcedOpacity(const std::vector<std::uint32_t>& ids, int percent) {
    const std::uint8_t alpha = static_cast<std::uint8_t>(
        std::clamp(percent, 0, 100) * 255 / 100);
    for (std::uint32_t id : ids) {
        forced_opacity_[id] = alpha;
        states_[id].opacity = alpha;
    }
    applyToViewport(ids);
}

void AppearanceService::clearForced() {
    std::vector<std::uint32_t> ids;
    constexpr int forced = static_cast<int>(Layer::ForcedFilter);
    for (auto& [id, state] : states_) {
        if (!state.layer_set[forced]) continue;
        state.layer_set[forced] = false;
        ids.push_back(id);
    }
    for (const auto& [id, alpha] : forced_opacity_) {
        states_[id].opacity = 255;
        ids.push_back(id);
    }
    forced_opacity_.clear();
    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    applyToViewport(ids);
}

void AppearanceService::restore(const Snapshot& snapshot) {
    // Every object that has an override now, or had one in the snapshot,
    // must be re-applied to the viewport.
    std::vector<std::uint32_t> ids;
    ids.reserve(states_.size() + snapshot.size());
    for (const auto& [id, state] : states_) ids.push_back(id);
    for (const auto& [id, state] : snapshot) ids.push_back(id);
    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    states_ = snapshot;
    applyToViewport(ids);
}

void AppearanceService::clearOverrides() {
    // The forced colours and opacities (activated filters) are kept.
    constexpr int forced = static_cast<int>(Layer::ForcedFilter);
    std::unordered_map<std::uint32_t, ObjectState> kept;
    for (const auto& [id, state] : states_) {
        const auto opacity = forced_opacity_.find(id);
        if (!state.layer_set[forced] && opacity == forced_opacity_.end()) continue;
        ObjectState& keep = kept[id];
        keep.layer_set[forced] = state.layer_set[forced];
        keep.layer_rgb8[forced] = state.layer_rgb8[forced];
        if (opacity != forced_opacity_.end()) keep.opacity = opacity->second;
    }
    states_ = std::move(kept);
    viewport_.clearObjectColors();
    std::vector<std::uint32_t> ids;
    ids.reserve(states_.size());
    for (const auto& [id, state] : states_) ids.push_back(id);
    applyToViewport(ids);
    emit appearanceChanged({});
}

void AppearanceService::clearAll() {
    forced_opacity_.clear();
    states_.clear();
    viewport_.clearObjectColors();
    emit appearanceChanged({});
}

void AppearanceService::applyToViewport(const std::vector<std::uint32_t>& ids) {
    if (ids.empty()) return;
    std::vector<std::uint32_t> restore;
    std::vector<std::uint32_t> opaque;
    std::vector<std::uint32_t> opaque_rgb;
    std::vector<std::uint32_t> alpha_only;
    std::vector<std::uint32_t> alpha_only_rgba;
    for (std::uint32_t id : ids) {
        auto it = states_.find(id);
        if (it == states_.end()) {
            restore.push_back(id);
            continue;
        }
        const ObjectState& state = it->second;
        int effective = -1;
        for (int i = 0; i < kLayerCount; ++i) {
            if (state.layer_set[i]) { effective = i; break; }
        }
        if (effective < 0 && state.opacity == 255) {
            restore.push_back(id);
        } else if (effective < 0) {
            // No colour layer: the baked RGB stays, the alpha byte carries the opacity.
            alpha_only.push_back(id);
            alpha_only_rgba.push_back(packRgba8(0, 0, 0, state.opacity));
        } else {
            opaque.push_back(id);
            opaque_rgb.push_back(packRgba8(state.layer_rgb8[effective] & 0xFFu,
                                           (state.layer_rgb8[effective] >> 8u) & 0xFFu,
                                           (state.layer_rgb8[effective] >> 16u) & 0xFFu,
                                           state.opacity));
        }
    }
    // Group by the override value: one viewport call per distinct colour
    // keeps the re-upload count at one buffer write per model per colour.
    if (!restore.empty()) {
        viewport_.setObjectsColor(restore, 0);
    }
    std::unordered_map<std::uint32_t, std::vector<std::uint32_t>> by_colour;
    for (std::size_t i = 0; i < opaque.size(); ++i) {
        by_colour[opaque_rgb[i]].push_back(opaque[i]);
    }
    for (const auto& [rgba8, object_ids] : by_colour) {
        viewport_.setObjectsColor(object_ids, rgba8);
    }
    // The alpha byte is the object's own opacity: group by it, as for colours.
    std::unordered_map<std::uint32_t, std::vector<std::uint32_t>> by_alpha;
    for (std::size_t i = 0; i < alpha_only.size(); ++i) {
        by_alpha[alpha_only_rgba[i]].push_back(alpha_only[i]);
    }
    for (const auto& [rgba8, object_ids] : by_alpha) {
        viewport_.setObjectsColor(object_ids, rgba8, kAlphaOnlyFlag);
    }
    emit appearanceChanged(ids);
}

void AppearanceService::purgeModel(std::uint32_t session_model_id) {
    std::vector<std::uint32_t> ids;
    for (const auto& info : session_.elementRegistry()->basicElementInfoForModel(session_model_id)) {
        forced_opacity_.erase(info.object_id);
        auto it = states_.find(info.object_id);
        if (it != states_.end()) {
            states_.erase(it);
            ids.push_back(info.object_id);
        }
    }
    if (!ids.empty()) {
        viewport_.setObjectsColor(ids, 0);
    }
}

} // namespace bonsaiviewer::services
