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

#include "RuleCatalogue.h"

#include "api/Engine.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

namespace bonsaiviewer::services::clash {

namespace {

const QString kTriangSuffix = QStringLiteral("_triang");

} // namespace

RuleCatalogue::RuleCatalogue() {
    const QJsonObject catalogue = QJsonDocument::fromJson(QByteArray::fromStdString(ifcclash::catalogue_json())).object();
    engine_version_ = catalogue.value("engine_version").toString();
    for (const QJsonValue& value : catalogue.value("rules").toArray()) {
        const QJsonObject rule = value.toObject();
        const QString engine_type = rule.value("type").toString();
        if (!engine_type.endsWith(kTriangSuffix)) continue; // the triangle rules only (TI09)
        RuleSpec spec;
        spec.engine_type = engine_type;
        spec.type = engine_type.left(engine_type.size() - kTriangSuffix.size());
        spec.label = rule.value("label").toString(spec.type);
        spec.label.remove(QStringLiteral(" (triangles)"));
        spec.family = rule.value("family").toString();
        spec.doc = rule.value("doc").toString();
        spec.doc.replace(QRegularExpression(QStringLiteral("^(\\w+) on the triangles: ")), QString());
        for (const QJsonValue& slot : rule.value("slots").toArray()) spec.slot_names << slot.toString();
        for (const QJsonValue& param_value : rule.value("params").toArray()) {
            const QJsonObject param = param_value.toObject();
            ParamSpec p;
            p.name = param.value("name").toString();
            p.kind = param.value("kind").toString();
            p.doc = param.value("doc").toString();
            p.unit = param.value("unit").toString();
            p.default_value = param.value("default");
            for (const QJsonValue& v : param.value("values").toArray()) p.values << v.toString();
            p.nullable = param.value("nullable").toBool(false);
            if (param.contains("min")) p.min = param.value("min").toDouble();
            if (param.contains("max")) p.max = param.value("max").toDouble();
            spec.params.push_back(std::move(p));
        }
        rules_.push_back(std::move(spec));
    }
}

const RuleCatalogue& RuleCatalogue::instance() {
    static const RuleCatalogue catalogue;
    return catalogue;
}

const RuleSpec* RuleCatalogue::find(const QString& type) const {
    for (const RuleSpec& spec : rules_) {
        if (spec.type == type || spec.engine_type == type) return &spec;
    }
    return nullptr;
}

QString familyBadge(const QString& family) {
    if (family == QLatin1String("OneObjectRule")) return QStringLiteral("1 object");
    if (family == QLatin1String("TwoObjectRule")) return QStringLiteral("2 objects");
    return QStringLiteral("complex");
}

} // namespace bonsaiviewer::services::clash
