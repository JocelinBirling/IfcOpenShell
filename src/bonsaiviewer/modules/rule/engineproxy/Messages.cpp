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

#include "Messages.h"

#include <QJsonArray>
#include <QJsonValue>

namespace bonsaiviewer::modules::rule::engineproxy {

namespace {

std::array<float, 3> point3FromJson(const QJsonArray& array) {
    std::array<float, 3> point{};
    for (int i = 0; i < 3 && i < array.size(); ++i) point[i] = static_cast<float>(array[i].toDouble());
    return point;
}

} // namespace

ElementRef elementRefFromJson(const QJsonObject& object) {
    ElementRef ref;
    ref.model = object.value("model").toInt(-1);
    ref.step_id = object.value("step_id").toInt();
    ref.global_id = object.value("global_id").toString();
    ref.name = object.value("name").toString();
    ref.ifc_type = object.value("ifc_type").toString();
    return ref;
}

EngineResult engineResultFromJson(const QJsonObject& object) {
    EngineResult result;
    result.id = object.value("id").toString();
    result.type = object.value("type").toString();
    result.rule = object.value("rule").toString();
    result.rule_type = object.value("rule_type").toString();
    result.node_id = object.value("node_id").toString();
    result.status = object.value("status").toBool(true);
    for (const QJsonValue& value : object.value("criticity").toArray()) {
        result.criticity.append(value.toString());
    }
    for (const QJsonValue& value : object.value("actor").toArray()) {
        result.actor.append(value.toString());
    }

    if (object.contains("source")) {
        result.has_source = true;
        result.source = elementRefFromJson(object.value("source").toObject());
    }
    if (object.contains("target")) {
        result.has_target = true;
        result.target = elementRefFromJson(object.value("target").toObject());
    }
    for (const QJsonValue& value : object.value("sources").toArray()) {
        result.sources.push_back(elementRefFromJson(value.toObject()));
    }
    for (const QJsonValue& value : object.value("targets").toArray()) {
        result.targets.push_back(elementRefFromJson(value.toObject()));
    }
    for (const QJsonValue& value : object.value("context").toArray()) {
        result.context.push_back(elementRefFromJson(value.toObject()));
    }

    // The nominal measure: the first entry of the engine's "measure" block.
    const QJsonObject info = object.value("info").toObject();
    result.info = info;
    const QJsonObject measure = info.value("measure").toObject();
    if (!measure.isEmpty()) {
        result.measure_name = measure.begin().key();
        const QJsonObject entry = measure.begin().value().toObject();
        if (entry.contains("value")) result.measure_value = entry.value("value").toDouble();
    }
    return result;
}

DisplayItem displayItemFromJson(const QJsonObject& object) {
    DisplayItem item;
    item.category = object.value("category").toString();
    const QJsonObject shape = object.value("shape").toObject();
    item.kind = shape.value("kind").toString();

    if (item.kind == QStringLiteral("edge")) {
        item.start = point3FromJson(shape.value("start").toArray());
        item.end = point3FromJson(shape.value("end").toArray());
        item.segments.push_back({item.start[0], item.start[1], item.start[2],
                                 item.end[0], item.end[1], item.end[2]});
    } else if (item.kind == QStringLiteral("face")) {
        for (const QJsonValue& corner : shape.value("corners").toArray()) {
            item.corners.push_back(point3FromJson(corner.toArray()));
        }
    } else if (item.kind == QStringLiteral("axis")) {
        item.center = point3FromJson(shape.value("center").toArray());
        item.direction = point3FromJson(shape.value("direction").toArray());
        item.length = static_cast<float>(shape.value("length").toDouble());
    } else if (item.kind == QStringLiteral("obb")) {
        item.obb_center = point3FromJson(shape.value("center").toArray());
        int axis_index = 0;
        for (const QJsonValue& axis : shape.value("axes").toArray()) {
            if (axis_index < 3) item.obb_axes[axis_index++] = point3FromJson(axis.toArray());
        }
        item.obb_half_sizes = point3FromJson(shape.value("half_sizes").toArray());
    } else if (item.kind == QStringLiteral("segments")) {
        for (const QJsonValue& segment : shape.value("segments").toArray()) {
            const QJsonArray pair = segment.toArray();
            if (pair.size() != 2) continue;
            const std::array<float, 3> a = point3FromJson(pair[0].toArray());
            const std::array<float, 3> b = point3FromJson(pair[1].toArray());
            item.segments.push_back({a[0], a[1], a[2], b[0], b[1], b[2]});
        }
    }

    const QJsonArray color = object.value("color").toArray();
    for (int i = 0; i < 3 && i < color.size(); ++i) {
        item.color[i] = static_cast<float>(color[i].toDouble());
    }
    item.transparency = static_cast<float>(object.value("transparency").toDouble(0.6));
    if (object.contains("element")) {
        item.has_element = true;
        item.element = elementRefFromJson(object.value("element").toObject());
    }
    return item;
}

} // namespace bonsaiviewer::modules::rule::engineproxy
