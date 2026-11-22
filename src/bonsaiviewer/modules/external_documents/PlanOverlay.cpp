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

#include "PlanOverlay.h"

#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>

#include <cmath>

namespace bonsaiviewer::modules::external_documents {

namespace {

constexpr double kPi = 3.14159265358979323846;

double degToRad(double deg) { return deg * kPi / 180.0; }

} // namespace

void nativeSize(const QString& path, int page, double& width_m, double& height_m,
                double fallback_w, double fallback_h) {
    width_m = fallback_w;
    height_m = fallback_h;
    if (QFileInfo(path).suffix().toLower() != QLatin1String("pdf")) return;

    // The MediaBox of a page is plain text in the (usually uncompressed)
    // page object: /MediaBox [ x0 y0 x1 y1 ]. Scan the first 256 KiB; the
    // n-th MediaBox belongs to the n-th page object in file order, which is
    // the common case for simple PDFs.
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return;
    const QByteArray head = file.read(256 * 1024);

    QRegularExpression media_box(QStringLiteral("/MediaBox\\s*\\[\\s*(-?[0-9.]+)\\s+(-?[0-9.]+)\\s+(-?[0-9.]+)\\s+(-?[0-9.]+)\\s*\\]"));
    auto it = media_box.globalMatch(QString::fromLatin1(head));
    int index = 0;
    while (it.hasNext()) {
        if (index == page) {
            const QRegularExpressionMatch match = it.next();
            const double x0 = match.captured(1).toDouble();
            const double y0 = match.captured(2).toDouble();
            const double x1 = match.captured(3).toDouble();
            const double y1 = match.captured(4).toDouble();
            width_m = std::fabs(x1 - x0) * kPointToMeter;
            height_m = std::fabs(y1 - y0) * kPointToMeter;
            return;
        }
        it.next();
        ++index;
    }
}

PlanGeometry planQuad(const QString& source_path, const PlanConfig& config) {
    double native_w = 0.0, native_h = 0.0;
    nativeSize(source_path, config.page, native_w, native_h);
    return planQuad(source_path, config, native_w, native_h);
}

PlanGeometry planQuad(const QString& source_path, const PlanConfig& config,
                      double native_w, double native_h) {
    (void)source_path;
    PlanGeometry geometry;
    const double w = native_w * config.scale;
    const double h = native_h * config.scale;
    geometry.width_m = w;
    geometry.height_m = h;

    // Plan-local corners, centered on the origin, Z = 0.
    const double corners[4][2] = {
        {-w / 2.0, -h / 2.0}, {w / 2.0, -h / 2.0}, {w / 2.0, h / 2.0}, {-w / 2.0, h / 2.0},
    };

    const double rx = degToRad(config.rot_x);
    const double ry = degToRad(config.rot_y);
    const double rz = degToRad(config.rot_z);
    const double cx = std::cos(rx), sx = std::sin(rx);
    const double cy = std::cos(ry), sy = std::sin(ry);
    const double cz = std::cos(rz), sz = std::sin(rz);

    auto transform = [&](double x, double y, double& ox, double& oy, double& oz) {
        // Translation relative to the plan axes is applied in the plan's local
        // frame, before the rotations; the model-axes translation and the
        // height are applied after.
        double lx = x, ly = y, lz = 0.0;
        if (config.trans_relative_to_plan) {
            lx += config.trans_x;
            ly += config.trans_y;
            lz += config.trans_z;
        }
        // R = Rz * Ry * Rx
        double y1 = cy * ly - sy * lz;
        double z1 = sy * ly + cy * lz;
        double x2 = cx * lx + sx * z1;
        double z2 = -sx * lx + cx * z1;
        double x3 = cz * x2 - sz * y1;
        double y3 = sz * x2 + cz * y1;
        ox = x3;
        oy = y3;
        oz = z2;
        if (!config.trans_relative_to_plan) {
            ox += config.trans_x;
            oy += config.trans_y;
            oz += config.trans_z;
        }
        oz += config.height_z;
    };

    double world[4][3];
    for (int i = 0; i < 4; ++i) {
        transform(corners[i][0], corners[i][1], world[i][0], world[i][1], world[i][2]);
        for (int k = 0; k < 3; ++k) geometry.corners[i][k] = float(world[i][k]);
    }
    // Two triangles: (0,1,2) and (0,2,3).
    const int triangles[2][3] = {{0, 1, 2}, {0, 2, 3}};
    geometry.quad_triangles.reserve(18);
    for (const auto& tri : triangles) {
        for (int k = 0; k < 3; ++k) {
            const int v = tri[k];
            geometry.quad_triangles.push_back(float(world[v][0]));
            geometry.quad_triangles.push_back(float(world[v][1]));
            geometry.quad_triangles.push_back(float(world[v][2]));
        }
    }
    for (int k = 0; k < 3; ++k) {
        geometry.center[k] = float((world[0][k] + world[2][k]) / 2.0);
    }
    return geometry;
}

} // namespace bonsaiviewer::modules::external_documents
