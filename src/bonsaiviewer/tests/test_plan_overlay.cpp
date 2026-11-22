// This file was generated with the assistance of an AI coding tool.
// BonsaiViewer36 — the 2D-plan geometry (target bonsaiviewer_plan_tests):
// MediaBox parsing, the world quad with its corners and UVs, and the PDF
// page rasterization used to draw the page on the quad in the 3D view.
#include "modules/external_documents/PdfRender.h"
#include "modules/external_documents/PlanOverlay.h"

#include <cmath>
#include <iostream>
#include <QCoreApplication>
#include <QFile>
#include <QStandardPaths>
#include <QTemporaryDir>

using namespace bonsaiviewer::modules::external_documents;

namespace {

int failures = 0, checks = 0;

void check(bool ok, const QString& what) {
    ++checks;
    if (!ok) {
        ++failures;
        std::cerr << "FAILED: " << what.toStdString() << "\n";
    }
}

bool near(double a, double b, double eps = 1e-6) { return std::fabs(a - b) <= eps; }

// A minimal one-page PDF, uncompressed page object with a MediaBox of
// [0 0 841.89 595.28] points (A4 landscape: 297 x 210 mm).
QByteArray minimalPdf() {
    return QByteArrayLiteral(
        "%PDF-1.4\n"
        "1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n"
        "2 0 obj\n<< /Type /Pages /Kids [3 0 R] /Count 1 >>\nendobj\n"
        "3 0 obj\n<< /Type /Page /Parent 2 0 R /MediaBox [0 0 841.89 595.28] "
        "/Contents 4 0 R /Resources << >> >>\nendobj\n"
        "4 0 obj\n<< /Length 44 >>\nstream\n"
        "0.5 0.5 1 RG 10 10 m 800 500 l S\n"
        "endstream\nendobj\n"
        "xref\n0 5\n"
        "0000000000 65535 f \n0000000009 00000 n \n0000000058 00000 n \n"
        "0000000115 00000 n \n0000000210 00000 n \n"
        "trailer\n<< /Size 5 /Root 1 0 R >>\nstartxref\n305\n%%EOF\n");
}

} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);

    QTemporaryDir dir;
    check(dir.isValid(), "temp dir");
    const QString pdf = dir.filePath(QStringLiteral("plan_a4.pdf"));
    {
        QFile f(pdf);
        check(f.open(QIODevice::WriteOnly), "write test pdf");
        f.write(minimalPdf());
    }

    // --- MediaBox -> native size (1 pt = 25.4/72000 m) ----------------------
    double w = 0.0, h = 0.0;
    nativeSize(pdf, 0, w, h);
    check(near(w, 0.297, 1e-3) && near(h, 0.210, 1e-3),
          QStringLiteral("nativeSize reads the MediaBox (%1 x %2 m)").arg(w).arg(h));
    // Fallback for a non-PDF path.
    nativeSize(dir.filePath(QStringLiteral("drawing.dxf")), 0, w, h);
    check(near(w, 10.0) && near(h, 10.0), "nativeSize fallback for non-PDF");

    // --- The world quad, default config -------------------------------------
    PlanConfig config; // visible, scale 1, no rotation, Z = 0
    const PlanGeometry g = planQuad(pdf, config);
    check(g.quad_triangles.size() == 18, "quad = 2 triangles = 18 floats");
    check(near(g.width_m, 0.297, 1e-3) && near(g.height_m, 0.210, 1e-3), "quad size");
    check(near(g.center[0], 0.0) && near(g.center[1], 0.0) && near(g.center[2], 0.0), "quad centered");
    // Corners: (-,-) (+,-) (+,+) (-,+) at Z = 0.
    check(near(g.corners[0][0], -g.width_m / 2, 1e-3) && near(g.corners[0][1], -g.height_m / 2, 1e-3) && near(g.corners[0][2], 0.0), "corner 0");
    check(near(g.corners[2][0], +g.width_m / 2, 1e-3) && near(g.corners[2][1], +g.height_m / 2, 1e-3), "corner 2");
    // UVs: the image's top row is the plan's +Y edge.
    check(near(g.uvs[0][0], 0.0) && near(g.uvs[0][1], 1.0), "uv corner 0 (bottom-left)");
    check(near(g.uvs[2][0], 1.0) && near(g.uvs[2][1], 0.0), "uv corner 2 (top-right)");
    // The outline reads the corners directly (4 edges, 24 floats, no OOB).
    static const int edges[4][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}};
    float outline[24];
    for (int e = 0; e < 4; ++e) {
        for (int k = 0; k < 2; ++k) {
            for (int c = 0; c < 3; ++c) {
                outline[e * 6 + k * 3 + c] = g.corners[edges[e][k]][c];
            }
        }
    }
    check(near(outline[0], g.corners[0][0]) && near(outline[23], g.corners[3][2]), "outline spans all corners");

    // --- Transform: elevation, rotation Z, scale -----------------------------
    config.height_z = 2.5;
    config.rot_z = 90.0;
    config.scale = 2.0;
    const PlanGeometry g2 = planQuad(pdf, config);
    check(near(g2.width_m, 2 * 0.297, 1e-3), "scale multiplies the native size");
    check(near(g2.corners[0][2], 2.5, 1e-3), "height Z applied");
    // 90 deg around Z: (-w/2, -h/2) -> (+h/2, -w/2) with w,h already scaled.
    check(near(g2.corners[0][0], +g2.height_m / 2, 1e-3) && near(g2.corners[0][1], -g2.width_m / 2, 1e-3),
          "rotation Z 90 deg maps the corner");

    // --- planQuad with an explicit native size (pdfinfo path) ---------------
    {
        PlanConfig fresh; // default scale 1
        const PlanGeometry g3 = planQuad(pdf, fresh, 2.0, 1.0);
        check(near(g3.width_m, 2.0) && near(g3.height_m, 1.0), "planQuad honors the explicit native size");
    }

    // --- PDF page size via pdfinfo (authoritative for compressed PDFs) ------
    if (QStandardPaths::findExecutable(QStringLiteral("pdfinfo")) != QString()) {
        double w_pt = 0.0, h_pt = 0.0;
        check(pdfPageSizePoints(pdf, 0, w_pt, h_pt), "pdfPageSizePoints reads page 1");
        check(near(w_pt, 841.89, 0.01) && near(h_pt, 595.28, 0.01),
              QStringLiteral("page size in points (%1 x %2)").arg(w_pt).arg(h_pt));
        check(near(w_pt * kPointToMeter, 0.297, 1e-3) && near(h_pt * kPointToMeter, 0.210, 1e-3),
              "points -> meters conversion");
        double dummy_w = 0.0, dummy_h = 0.0;
        check(!pdfPageSizePoints(pdf, 9, dummy_w, dummy_h), "out-of-range page -> false");
    }

    // --- PDF page rasterization ----------------------------------------------
    if (pdfRasterizerAvailable()) {
        const QImage page = rasterizePdfPage(pdf, 0, 0.297, 0.210);
        check(!page.isNull(), "rasterizePdfPage returns an image");
        if (!page.isNull()) {
            check(page.format() == QImage::Format_RGBA8888, "RGBA8888 for the wgpu upload");
            const double ar = double(page.width()) / double(page.height());
            check(near(ar, 0.297 / 0.210, 0.02), "rasterized aspect matches the page");
            // The page content (a blue stroke) is actually drawn, not a blank page.
            bool has_ink = false;
            for (int y = 0; y < page.height() && !has_ink; y += 3) {
                for (int x = 0; x < page.width(); x += 3) {
                    const QRgb p = page.pixel(x, y);
                    if (qRed(p) != 255 || qGreen(p) != 255 || qBlue(p) != 255) {
                        has_ink = true;
                        break;
                    }
                }
            }
            check(has_ink, "the page content is rendered (non-white pixels)");
        }
        check(rasterizePdfPage(pdf, 7, 0.297, 0.210).isNull(), "out-of-range page -> null image");
        check(rasterizePdfPage(dir.filePath(QStringLiteral("nope.pdf")), 0, 1.0, 1.0).isNull(),
              "missing file -> null image");
    } else {
        std::cout << "SKIP: no Poppler CLI (pdftoppm/pdftocairo) on the PATH, "
                     "rasterization checks skipped (the view falls back to the translucent quad)\n";
    }

    std::cout << checks - failures << "/" << checks << " checks passed\n";
    return failures == 0 ? 0 : 1;
}
