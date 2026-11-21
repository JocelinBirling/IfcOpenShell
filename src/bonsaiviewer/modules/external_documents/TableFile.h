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

#ifndef BONSAIVIEWER_MODULES_EXTERNAL_DOCUMENTS_TABLEFILE_H
#define BONSAIVIEWER_MODULES_EXTERNAL_DOCUMENTS_TABLEFILE_H

#include <QList>
#include <QString>
#include <QStringList>

namespace bonsaiviewer::modules::external_documents {

// BonsaiViewer35 — reading and writing the external table files.
//
// CSV follows the CSV export rule of BonsaiViewer07 in reverse: UTF-8, the
// field separator is ';' when the system decimal separator is ',', otherwise
// ',', and the first row gives the column names.
//
// .xlsx files are read as what they are — a ZIP of XML — through the BCF zip
// port (Qt private QZipReader) and QXmlStreamReader: workbook sheets, shared
// strings and the sheet cells. No external spreadsheet library.

// At most this many rows are loaded into the grid (BonsaiViewer35, large
// files); `total_rows` still reports the real row count of the file.
inline constexpr int kMaxLoadedRows = 10000;

struct TableData {
    QStringList columns;
    QList<QStringList> rows; // capped at kMaxLoadedRows
    qint64 total_rows = 0;
    QStringList sheets;      // .xlsx only
    QString sheet;           // the sheet that was read
    bool ok = false;
    QString error;
};

// The separator rule of BonsaiViewer07: ';' when the system decimal
// separator is ',', otherwise ','.
QChar csvSeparator();

// Reads a .csv file (UTF-8, quoted fields supported).
TableData readCsvFile(const QString& path);

// Lists the sheets of a .xlsx file without reading any cell data.
QStringList xlsxSheets(const QString& path);

// Reads one sheet of a .xlsx file; `sheet` empty reads the first sheet.
TableData readXlsxFile(const QString& path, const QString& sheet = QString());

// Dispatches on the extension (.csv / .xlsx).
TableData readTableFile(const QString& path, const QString& sheet = QString());

// Writes all the rows, same format rule as BonsaiViewer07. False on failure.
bool writeCsvFile(const QString& path, const QStringList& columns, const QList<QStringList>& rows);

} // namespace bonsaiviewer::modules::external_documents

#endif // BONSAIVIEWER_MODULES_EXTERNAL_DOCUMENTS_TABLEFILE_H
