// This file was generated with the assistance of an AI coding tool.
/********************************************************************************
 *                                                                              *
 * This file is part of Bonsai.                                                 *
 *                                                                              *
 * Bonsai is free software: you can redistribute it and/or modify               *
 * it under the terms of the License, or (at your option) any later            *
 * version.                                                                     *
 *                                                                              *
 ********************************************************************************/

#ifndef IFCINTERFACE_MODULES_RULE_ENGINEPROXY_MESSAGES_H
#define IFCINTERFACE_MODULES_RULE_ENGINEPROXY_MESSAGES_H

#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <array>
#include <cmath>
#include <vector>

namespace bonsaiviewer::modules::rule::engineproxy {

// An IFC element as referenced across the process boundary: the model index
// (position in the run's list_ifc_path) joined against the viewer's session
// model ids, plus the STEP id and display fields.
struct ElementRef {
    int model = -1;
    int step_id = 0;
    QString global_id;
    QString name;
    QString ifc_type;

    bool isValid() const { return step_id > 0; }
};

// One clash result of a run (the engine's ClashResult, section 7.2 of the
// integration spec). `info` keeps the raw ClashResultInfo blocks: the
// consumers render the blocks they know and fall back to raw JSON.
struct EngineResult {
    QString id;
    QString type;      // OneObjectResult | TwoObjectsResult | ComplexResult
    QString rule;      // the rule display name the engine recorded
    QString rule_type; // the engine class (OBB_Above, ...)
    QString node_id;   // the tree node that produced the result
    bool status = true;
    QStringList criticity;
    QStringList actor;

    bool has_source = false;
    bool has_target = false;
    ElementRef source;
    ElementRef target;

    // ComplexResult (a Must-check group): the elements of its two sets.
    std::vector<ElementRef> sources;
    std::vector<ElementRef> targets;
    std::vector<ElementRef> context;

    // The nominal measure (the first one the rule posed), when any.
    QString measure_name;
    double measure_value = std::nan("");

    QJsonObject info;
};

// One drawing item of the display contract (section 8): the engine's
// DisplayItem converted to a JSON shape the viewer renders as overlays.
struct DisplayItem {
    QString category; // axis | face | obb | zone | context | segment | point

    // The shape, discriminated by `kind`:
    //   edge     — a single segment (start -> end)
    //   face     — a polygon (corners)
    //   axis     — a centered direction arrow
    //   obb      — a box (center + axes + half sizes)
    //   segments — a polyline list (each segment: two points)
    QString kind;
    std::array<float, 3> start{};
    std::array<float, 3> end{};
    std::vector<std::array<float, 3>> corners;
    std::array<float, 3> center{};
    std::array<float, 3> direction{};
    float length = 0.0f;
    std::array<float, 3> obb_center{};
    std::array<std::array<float, 3>, 3> obb_axes{};
    std::array<float, 3> obb_half_sizes{};
    std::vector<std::array<float, 6>> segments;

    std::array<float, 3> color{{1.0f, 0.0f, 0.0f}};
    float transparency = 0.6f;
    bool has_element = false;
    ElementRef element;
};

// Parse helpers shared by EngineClient and the run wiring.
ElementRef elementRefFromJson(const QJsonObject& object);
EngineResult engineResultFromJson(const QJsonObject& object);
DisplayItem displayItemFromJson(const QJsonObject& object);

} // namespace bonsaiviewer::modules::rule::engineproxy

#endif
