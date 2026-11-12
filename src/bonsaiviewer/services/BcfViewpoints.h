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

#ifndef IFCINTERFACE_SERVICES_BCFVIEWPOINTS_H
#define IFCINTERFACE_SERVICES_BCFVIEWPOINTS_H

#include "../ports/bcf/Bcf.h"

#include <QByteArray>
#include <QHash>
#include <QObject>

#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

class ViewportWindow;
namespace bonsaiviewer { class SessionState; }

namespace bonsaiviewer::services {

class ActiveView;
class AppearanceService;
class CategoryOverrideFilter;
class SelectionService;

// BonsaiViewer10 — the bridge between a BCF viewpoint and the viewer: the
// active view as a viewpoint (camera, selection, visibility, colours,
// clipping planes, view setup hints), a viewpoint applied to the viewer,
// the snapshot of the 3D view, and the components <-> elements by GlobalId.
//
// Coordinates: BCF uses the model's coordinates (metres). The viewer may
// move the models (federation false origin); the first loaded model's
// model -> render matrix converts both ways. The viewer's camera has no
// roll: the up vector is not applied.
class BcfViewpoints : public QObject {
    Q_OBJECT

public:
    BcfViewpoints(SessionState& session, ViewportWindow& viewport, SelectionService& selection,
                  ActiveView& view, AppearanceService& appearance, CategoryOverrideFilter& categories,
                  QObject* parent = nullptr);

    // The active view as a visualization info (new guid).
    ports::bcf::VisualizationInfo capture() const;
    // The next frame of the 3D view as PNG bytes (empty on failure).
    void captureSnapshot(std::function<void(const QByteArray&)> done);

    struct Parts {
        bool camera = true;
        bool selection = true;
        bool visibility = true;
        bool colours = true;
        bool clipping = true;
    };
    void apply(const ports::bcf::VisualizationInfo& info, Parts parts);
    void apply(const ports::bcf::VisualizationInfo& info) { apply(info, Parts{}); }

    // The loaded elements of the components (unknown GlobalIds are left out).
    std::vector<std::uint32_t> objectIds(const std::vector<ports::bcf::Component>& components) const;
    std::vector<ports::bcf::Component> components(const std::vector<std::uint32_t>& object_ids) const;
    // The selected elements of the viewer as components.
    std::vector<ports::bcf::Component> selectedComponents() const;
    // Name and IFC class of a loaded element ({} when it is not loaded).
    std::pair<QString, QString> describe(const QString& ifc_guid) const;
    // The viewpoint's relevant elements: its selection, else its coloured
    // elements, else its visible exceptions.
    std::vector<std::uint32_t> relevantIds(const ports::bcf::VisualizationInfo& info) const;

    void isolate(const std::vector<std::uint32_t>& ids);
    // Frames the elements, keeping the direction of the view.
    void zoomTo(const std::vector<std::uint32_t>& ids);

private:
    void refreshGuids() const;

    SessionState& session_;
    ViewportWindow& viewport_;
    SelectionService& selection_;
    ActiveView& view_;
    AppearanceService& appearance_;
    CategoryOverrideFilter& categories_;
    // GlobalId <-> object id over every loaded model, rebuilt when the
    // number of loaded elements changes.
    mutable QHash<QString, std::uint32_t> id_by_guid_;
    mutable QHash<std::uint32_t, QString> guid_by_id_;
    mutable std::size_t indexed_elements_ = 0;
    std::vector<std::function<void(const QByteArray&)>> pending_snapshots_;
    QString snapshot_path_;
};

} // namespace bonsaiviewer::services

#endif // IFCINTERFACE_SERVICES_BCFVIEWPOINTS_H
