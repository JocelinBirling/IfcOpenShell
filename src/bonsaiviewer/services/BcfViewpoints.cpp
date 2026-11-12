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
 ********************************************************************************/

#include "BcfViewpoints.h"

#include "ActiveView.h"
#include "AppearanceService.h"
#include "CategoryOverrideFilter.h"
#include "SelectionService.h"
#include "../ElementRegistry.h"
#include "../SessionState.h"
#include "../../ifcviewer/ViewportWindow.h"

#include <QColor>
#include <QDir>
#include <QFile>
#include <QMap>
#include <QUuid>

#include <Eigen/Dense>

#include <algorithm>
#include <cmath>
#include <limits>

namespace bonsaiviewer::services {

namespace bcf = bonsaiviewer::ports::bcf;

namespace {

constexpr double kDegToRad = 3.14159265358979323846 / 180.0;

// Model -> render matrix of the first loaded model (identity when none).
Eigen::Matrix4d modelToRender(SessionState& session, ViewportWindow& viewport) {
    Eigen::Matrix4d matrix = Eigen::Matrix4d::Identity();
    const auto models = session.sessionModelIds();
    if (!models.empty()) viewport.modelToWorld(models.front(), matrix);
    return matrix;
}

bcf::Vector3 toBcf(const Eigen::Vector3d& v) {
    return {v.x(), v.y(), v.z()};
}

Eigen::Vector3d fromBcf(const bcf::Vector3& v) {
    return {v.x, v.y, v.z};
}

Eigen::Vector3d transformPoint(const Eigen::Matrix4d& m, const Eigen::Vector3d& p) {
    return (m * p.homogeneous()).head<3>();
}

Eigen::Vector3d transformDirection(const Eigen::Matrix4d& m, const Eigen::Vector3d& d) {
    const Eigen::Vector3d r = m.topLeftCorner<3, 3>() * d;
    return r.norm() > 0.0 ? Eigen::Vector3d(r.normalized()) : r;
}

QString hexColour(std::uint32_t rgb8) {
    return QString("%1%2%3")
        .arg(rgb8 & 0xFFu, 2, 16, QChar('0'))
        .arg((rgb8 >> 8u) & 0xFFu, 2, 16, QChar('0'))
        .arg((rgb8 >> 16u) & 0xFFu, 2, 16, QChar('0'))
        .toUpper();
}

// "RRGGBB" or "AARRGGBB".
QColor colourOf(const QString& hex) {
    const QString rgb = hex.size() == 8 ? hex.mid(2) : hex;
    return QColor(QStringLiteral("#") + rgb);
}

} // namespace

BcfViewpoints::BcfViewpoints(SessionState& session, ViewportWindow& viewport, SelectionService& selection,
                             ActiveView& view, AppearanceService& appearance, CategoryOverrideFilter& categories,
                             QObject* parent)
    : QObject(parent), session_(session), viewport_(viewport), selection_(selection), view_(view),
      appearance_(appearance), categories_(categories) {
    connect(&viewport_, &ViewportWindow::imageCaptured, this, [this](const QString& path, bool ok) {
        if (path != snapshot_path_) return;
        QByteArray png;
        if (ok) {
            QFile file(path);
            if (file.open(QIODevice::ReadOnly)) png = file.readAll();
        }
        QFile::remove(path);
        snapshot_path_.clear();
        auto pending = std::move(pending_snapshots_);
        pending_snapshots_.clear();
        for (const auto& done : pending) done(png);
    });
}

void BcfViewpoints::refreshGuids() const {
    std::size_t count = 0;
    std::vector<std::vector<BasicElementInfo>> models;
    for (std::uint32_t model : session_.sessionModelIds()) {
        models.push_back(session_.elementRegistry()->basicElementInfoForModel(model));
        count += models.back().size();
    }
    if (count == indexed_elements_ && !id_by_guid_.isEmpty()) return;
    id_by_guid_.clear();
    guid_by_id_.clear();
    for (const auto& elements : models) {
        for (const auto& info : elements) {
            if (info.guid.isEmpty()) continue;
            id_by_guid_.insert(info.guid, info.object_id);
            guid_by_id_.insert(info.object_id, info.guid);
        }
    }
    indexed_elements_ = count;
}

std::vector<std::uint32_t> BcfViewpoints::objectIds(const std::vector<bcf::Component>& components) const {
    refreshGuids();
    std::vector<std::uint32_t> ids;
    ids.reserve(components.size());
    for (const auto& component : components) {
        const auto found = id_by_guid_.constFind(component.ifc_guid);
        if (found != id_by_guid_.constEnd()) ids.push_back(found.value());
    }
    return ids;
}

std::vector<bcf::Component> BcfViewpoints::components(const std::vector<std::uint32_t>& object_ids) const {
    refreshGuids();
    std::vector<bcf::Component> result;
    result.reserve(object_ids.size());
    for (std::uint32_t id : object_ids) {
        const auto found = guid_by_id_.constFind(id);
        if (found == guid_by_id_.constEnd()) continue;
        bcf::Component component;
        component.ifc_guid = found.value();
        component.originating_system = QStringLiteral("BonsaiViewer");
        result.push_back(component);
    }
    return result;
}

std::vector<bcf::Component> BcfViewpoints::selectedComponents() const {
    return components(selection_.selectionIdsAsVector());
}

std::pair<QString, QString> BcfViewpoints::describe(const QString& ifc_guid) const {
    refreshGuids();
    const auto found = id_by_guid_.constFind(ifc_guid);
    if (found == id_by_guid_.constEnd()) return {};
    const auto info = session_.elementRegistry()->findBasicElementInfo(found.value());
    if (!info) return {};
    return {info->name, info->type};
}

std::vector<std::uint32_t> BcfViewpoints::relevantIds(const bcf::VisualizationInfo& info) const {
    if (!info.selection.empty()) return objectIds(info.selection);
    std::vector<bcf::Component> coloured;
    for (const auto& coloring : info.coloring) {
        coloured.insert(coloured.end(), coloring.components.begin(), coloring.components.end());
    }
    if (!coloured.empty()) return objectIds(coloured);
    if (!info.default_visibility) return objectIds(info.exceptions);
    return {};
}

bcf::VisualizationInfo BcfViewpoints::capture() const {
    bcf::VisualizationInfo info;
    info.guid = QUuid::createUuid().toString(QUuid::WithoutBraces).toLower();
    const Eigen::Matrix4d to_render = modelToRender(session_, viewport_);
    const Eigen::Matrix4d to_model = to_render.inverse();

    // Camera: the orbit camera (target, distance, yaw, pitch; Z up, no roll).
    const auto state = viewport_.cameraState();
    const double yaw = state.yaw * kDegToRad;
    const double pitch = state.pitch * kDegToRad;
    const Eigen::Vector3d offset(std::cos(pitch) * std::cos(yaw), std::cos(pitch) * std::sin(yaw), std::sin(pitch));
    const Eigen::Vector3d target = state.target.cast<double>();
    const Eigen::Vector3d eye = target + state.distance * offset;
    const Eigen::Vector3d up(-std::sin(pitch) * std::cos(yaw), -std::sin(pitch) * std::sin(yaw), std::cos(pitch));
    bcf::Camera& camera = info.camera;
    const double fov = viewport_.fieldOfViewDeg();
    camera.kind = viewport_.projectionOrtho() ? bcf::Camera::Kind::Orthogonal : bcf::Camera::Kind::Perspective;
    camera.view_point = toBcf(transformPoint(to_model, eye));
    camera.direction = toBcf(transformDirection(to_model, -offset));
    camera.up_vector = toBcf(transformDirection(to_model, up));
    camera.field_of_view = fov;
    camera.view_to_world_scale = 2.0 * state.distance * std::tan(fov * kDegToRad / 2.0);
    if (viewport_.height() > 0) camera.aspect_ratio = double(viewport_.width()) / double(viewport_.height());

    info.selection = components(selection_.selectionIdsAsVector());

    // Visibility: the shorter list of exceptions.
    const auto visible = view_.visibleIds();
    const auto hidden = view_.hiddenIds();
    info.default_visibility = hidden.size() <= visible.size();
    info.exceptions = components(info.default_visibility ? hidden : visible);
    bcf::ViewSetupHints hints;
    hints.spaces_visible = categories_.isCategoryEnabled("IfcSpace");
    hints.openings_visible = categories_.isCategoryEnabled("IfcOpeningElement");
    info.view_setup_hints = hints;

    // Colours: the colour each element shows (highest override layer).
    QMap<std::uint32_t, std::vector<std::uint32_t>> by_colour;
    for (const auto& [id, state_of] : appearance_.snapshot()) {
        for (int layer = 0; layer < AppearanceService::kLayerCount; ++layer) {
            if (!state_of.layer_set[layer]) continue;
            by_colour[state_of.layer_rgb8[layer] & 0xFFFFFFu].push_back(id);
            break;
        }
    }
    for (auto it = by_colour.constBegin(); it != by_colour.constEnd(); ++it) {
        bcf::Coloring coloring;
        coloring.color = hexColour(it.key());
        coloring.components = components(it.value());
        if (!coloring.components.empty()) info.coloring.push_back(std::move(coloring));
    }

    for (const auto& plane : viewport_.sectionPlanes()) {
        bcf::ClippingPlane clipping;
        clipping.location = toBcf(transformPoint(to_model, plane.origin.cast<double>()));
        clipping.direction = toBcf(transformDirection(to_model, plane.n.cast<double>()));
        info.clipping_planes.push_back(clipping);
    }
    return info;
}

void BcfViewpoints::captureSnapshot(std::function<void(const QByteArray&)> done) {
    pending_snapshots_.push_back(std::move(done));
    if (!snapshot_path_.isEmpty()) return; // a capture is already on its way
    snapshot_path_ = QDir::temp().filePath(
        QString("bonsaiviewer-bcf-%1.png").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
    viewport_.captureNextFrameToPng(snapshot_path_.toStdString(), /*quit_after=*/false);
}

void BcfViewpoints::apply(const bcf::VisualizationInfo& info, Parts parts) {
    const Eigen::Matrix4d to_render = modelToRender(session_, viewport_);
    const bcf::Camera& camera = info.camera;
    if (parts.camera && camera.kind != bcf::Camera::Kind::None) {
        const Eigen::Vector3d eye = transformPoint(to_render, fromBcf(camera.view_point));
        Eigen::Vector3d direction = transformDirection(to_render, fromBcf(camera.direction));
        if (direction.norm() < 1e-9) direction = Eigen::Vector3d(0, 1, 0);
        const double fov = viewport_.fieldOfViewDeg();
        // The orbit target sits in front of the eye: at the view's scale in
        // orthographic, at the current distance in perspective.
        double distance = viewport_.cameraState().distance;
        const bool ortho = camera.kind == bcf::Camera::Kind::Orthogonal;
        if (ortho) distance = camera.view_to_world_scale / (2.0 * std::tan(fov * kDegToRad / 2.0));
        distance = std::max(distance, 0.5);
        const Eigen::Vector3d target = eye + distance * direction;
        const double pitch = std::asin(std::clamp(-direction.z(), -1.0, 1.0)) / kDegToRad;
        const double yaw = std::atan2(-direction.y(), -direction.x()) / kDegToRad;
        if (viewport_.projectionOrtho() != ortho) viewport_.toggleProjection();
        viewport_.setCamera(float(target.x()), float(target.y()), float(target.z()), float(distance), float(yaw),
                            float(pitch));
    }
    if (parts.visibility) {
        const auto exceptions = objectIds(info.exceptions);
        if (info.default_visibility) {
            view_.showAll();
            if (!exceptions.empty()) view_.remove(exceptions);
        } else {
            view_.isolate(exceptions);
        }
    }
    if (parts.colours) {
        appearance_.clearOverrides(); // the forced colours (activated filters) stay
        for (const auto& coloring : info.coloring) {
            const auto ids = objectIds(coloring.components);
            if (!ids.empty()) {
                appearance_.setLayerColour(ids, colourOf(coloring.color), AppearanceService::Layer::FilterAction);
            }
        }
    }
    if (parts.selection) selection_.setSelection(objectIds(info.selection), SelectionSource::Service);
    if (parts.clipping) {
        viewport_.clearSectionPlanes();
        for (const auto& plane : info.clipping_planes) {
            const Eigen::Vector3d point = transformPoint(to_render, fromBcf(plane.location));
            const Eigen::Vector3d normal = transformDirection(to_render, fromBcf(plane.direction));
            if (normal.norm() < 1e-9) continue;
            viewport_.addSectionPlaneFacing(point.cast<float>(), normal.cast<float>());
        }
    }
}

bcf::VisualizationInfo BcfViewpoints::focus(const Focus& focus) {
    std::vector<std::uint32_t> objects = focus.sources;
    objects.insert(objects.end(), focus.targets.begin(), focus.targets.end());
    std::vector<std::uint32_t> visible = objects;
    visible.insert(visible.end(), focus.context.begin(), focus.context.end());
    if (!visible.empty()) view_.isolate(visible);
    zoomTo(objects);
    selection_.setSelection(objects, SelectionSource::Service);

    bcf::VisualizationInfo info = capture(); // the camera of that view
    info.selection = components(objects);
    info.default_visibility = false;
    info.exceptions = components(visible);
    info.coloring.clear();
    const auto colour = [&info, this](const std::vector<std::uint32_t>& ids, const QString& hex) {
        bcf::Coloring coloring;
        coloring.color = hex;
        coloring.components = components(ids);
        if (!coloring.components.empty()) info.coloring.push_back(std::move(coloring));
    };
    colour(focus.sources, QStringLiteral("FFE6194B"));
    colour(focus.targets, QStringLiteral("FFF58231"));
    const Eigen::Matrix4d to_model = modelToRender(session_, viewport_).inverse();
    for (const auto& [start, end] : focus.lines) {
        info.lines.push_back(bcf::Line{toBcf(transformPoint(to_model, start)), toBcf(transformPoint(to_model, end))});
    }
    info.clipping_planes.clear();
    return info;
}

void BcfViewpoints::isolate(const std::vector<std::uint32_t>& ids) {
    if (!ids.empty()) view_.isolate(ids);
}

void BcfViewpoints::zoomTo(const std::vector<std::uint32_t>& ids) {
    Eigen::Vector3f low = Eigen::Vector3f::Constant(std::numeric_limits<float>::max());
    Eigen::Vector3f high = Eigen::Vector3f::Constant(std::numeric_limits<float>::lowest());
    bool any = false;
    for (std::uint32_t id : ids) {
        Eigen::Vector3f mn, mx;
        if (!viewport_.computeObjectAabb(id, mn, mx)) continue;
        low = low.cwiseMin(mn);
        high = high.cwiseMax(mx);
        any = true;
    }
    if (!any) return;
    const auto state = viewport_.cameraState();
    const Eigen::Vector3f centre = (low + high) / 2.0f;
    const float radius = std::max((high - low).norm() / 2.0f, 0.5f);
    const float fov = viewport_.fieldOfViewDeg() * float(kDegToRad);
    const float distance = 1.2f * radius / std::sin(fov / 2.0f);
    viewport_.setCamera(centre.x(), centre.y(), centre.z(), distance, state.yaw, state.pitch);
}

} // namespace bonsaiviewer::services
