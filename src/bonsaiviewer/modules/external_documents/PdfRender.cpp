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

#include "PdfRender.h"

#include <algorithm>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTemporaryDir>

namespace bonsaiviewer::modules::external_documents {

namespace {

// 1 PDF point in meters (the MediaBox unit); the inverse converts meters
// back to points for the DPI computation below.
constexpr double kPointToMeter = 25.4 / 72000.0;

// Target texture resolution: about 2048 pixels per meter of plan (~52 dpi),
// enough for a drawing read at model scale without multi-megapixel textures.
constexpr double kPixelsPerMeter = 2048.0;

// Resolves the Poppler rasterization tool: pdftoppm preferred, pdftocairo as
// fallback. Empty when neither is on the PATH.
QString rasterizerTool() {
    QString tool = QStandardPaths::findExecutable(QStringLiteral("pdftoppm"));
    if (!tool.isEmpty()) {
        return tool;
    }
    return QStandardPaths::findExecutable(QStringLiteral("pdftocairo"));
}

} // namespace

bool pdfRasterizerAvailable() {
    return !rasterizerTool().isEmpty();
}

bool pdfPageSizePoints(const QString& pdf_path, int page,
                       double& width_pt, double& height_pt) {
    width_pt = height_pt = 0.0;
    if (page < 0) return false;
    if (QFileInfo(pdf_path).suffix().toLower() != QLatin1String("pdf")) return false;
    if (!QFileInfo::exists(pdf_path)) return false;
    const QString pdfinfo = QStandardPaths::findExecutable(QStringLiteral("pdfinfo"));
    if (pdfinfo.isEmpty()) return false;
    // "Page    1 size:  612 x 792 pts (letter)"
    QProcess process;
    process.start(pdfinfo, {QStringLiteral("-f"), QString::number(page + 1),
                             QStringLiteral("-l"), QString::number(page + 1),
                             pdf_path});
    if (!process.waitForFinished(15000)) {
        process.kill();
        process.waitForFinished(2000);
        return false;
    }
    if (process.exitCode() != 0) return false;
    const QRegularExpression re(QStringLiteral(
        "Page\\s+%1\\s+size:\\s+([0-9.]+)\\s*x\\s*([0-9.]+)\\s*pts").arg(page + 1));
    const auto match = re.match(QString::fromUtf8(process.readAllStandardOutput()));
    if (!match.hasMatch()) return false;
    width_pt = match.captured(1).toDouble();
    height_pt = match.captured(2).toDouble();
    return width_pt > 0.0 && height_pt > 0.0;
}

QImage rasterizePdfPage(const QString& pdf_path, int page, double native_width_m, double native_height_m, int max_pixels) {
    const QString tool = rasterizerTool();
    if (tool.isEmpty()) {
        return {};
    }
    if (QFileInfo(pdf_path).suffix().toLower() != QLatin1String("pdf")) {
        return {};
    }
    if (page < 0) {
        return {};
    }
    if (!QFileInfo::exists(pdf_path)) {
        return {};
    }

    // DPI so the long side of the page lands on ~kPixelsPerMeter px/m,
    // capped at max_pixels: pixel_long_side = long_side_pt * dpi / 72.
    const double long_side_m = std::max(native_width_m, native_height_m);
    double dpi = kPixelsPerMeter * 0.0254; // px/m * m/in = px/in = dpi
    if (long_side_m > 0.0) {
        const double long_side_pt = long_side_m / kPointToMeter;
        const double dpi_cap = double(max_pixels) * 72.0 / long_side_pt;
        dpi = std::min(dpi, dpi_cap);
    }
    dpi = std::max(36.0, std::min(dpi, 600.0));

    QTemporaryDir dir;
    if (!dir.isValid()) {
        return {};
    }
    const QString prefix = dir.filePath(QStringLiteral("page"));

    // Both tools share the same flags for a single PNG page; -singlefile
    // makes the output exactly <prefix>.png with no page-number suffix.
    const QStringList args = {
        QStringLiteral("-png"),
        QStringLiteral("-r"),
        QString::number(dpi, 'f', 2),
        QStringLiteral("-f"),
        QString::number(page + 1), // CLI pages are 1-based
        QStringLiteral("-l"),
        QString::number(page + 1),
        QStringLiteral("-singlefile"),
        pdf_path,
        prefix,
    };
    QProcess process;
    process.start(tool, args);
    if (!process.waitForFinished(30000)) {
        process.kill();
        process.waitForFinished(2000);
        return {};
    }
    if (process.exitCode() != 0) {
        return {};
    }

    QImage image(prefix + QStringLiteral(".png"));
    if (image.isNull()) {
        return {};
    }
    // Straight-alpha RGBA8 for the wgpu upload + alpha blend in the renderer.
    return image.convertToFormat(QImage::Format_RGBA8888);
}

} // namespace bonsaiviewer::modules::external_documents
