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

#ifndef BONSAIVIEWER_MODULES_EXTERNAL_DOCUMENTS_PDFRENDER_H
#define BONSAIVIEWER_MODULES_EXTERNAL_DOCUMENTS_PDFRENDER_H

#include <QImage>
#include <QString>

namespace bonsaiviewer::modules::external_documents {

// BonsaiViewer36 phase 1 — rasterise one page of a PDF into a QImage so the
// page can be drawn on the plan quad in the 3D view.
//
// The rasterizer is Poppler through its command-line tools (pdftoppm, else
// pdftocairo), looked up on the PATH: no link-time dependency, and the
// feature degrades gracefully where the tools are absent — a null QImage is
// returned and the caller falls back to the plain translucent quad. Linking
// a PDF library (Poppler/MuPDF/Qt Pdf) directly is the phase-1 open subject
// of the spec; the CLI keeps phase 1 dependency-free.
//
// `native_width_m` / `native_height_m` are the page size in meters (from
// PlanOverlay::nativeSize); they only steer the rasterization DPI so the
// image resolution follows the plan's real-world size, capped at
// `max_pixels` on the long side. `page` is 0-based.
QImage rasterizePdfPage(const QString& pdf_path, int page, double native_width_m, double native_height_m, int max_pixels = 4096);

// True when a PDF rasterizer (Poppler CLI) is available on this machine.
bool pdfRasterizerAvailable();

// The size of one page in PDF points (1 pt = 1/72 in), read with Poppler's
// `pdfinfo` — authoritative even for PDFs whose page objects are compressed
// (object streams), where a plain-text MediaBox scan fails. `page` is
// 0-based. Returns false when pdfinfo is unavailable or the page is out of
// range.
bool pdfPageSizePoints(const QString& pdf_path, int page,
                       double& width_pt, double& height_pt);

} // namespace bonsaiviewer::modules::external_documents

#endif // BONSAIVIEWER_MODULES_EXTERNAL_DOCUMENTS_PDFRENDER_H
