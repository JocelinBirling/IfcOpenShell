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

#include "TableFile.h"

#include "../../ports/bcf/Zip.h"

#include <QFile>
#include <QFileInfo>
#include <QLocale>
#include <QTextStream>
#include <QXmlStreamReader>

#include <map>

namespace bonsaiviewer::modules::external_documents {

namespace {

// --- CSV ---------------------------------------------------------------------

// Parses one CSV record stream; handles quoted fields with embedded
// separators, quotes and newlines. `out` gets one QStringList per record.
void parseCsv(const QString& text, QChar separator, QList<QStringList>& out) {
    QStringList record;
    QString field;
    bool in_quotes = false;
    bool field_started = false;

    auto endField = [&]() {
        record.append(field);
        field.clear();
        field_started = false;
    };
    auto endRecord = [&]() {
        endField();
        if (!(record.size() == 1 && record.front().isEmpty())) out.append(record);
        record.clear();
    };

    const qsizetype n = text.size();
    for (qsizetype i = 0; i < n; ++i) {
        const QChar c = text.at(i);
        if (in_quotes) {
            if (c == QLatin1Char('"')) {
                if (i + 1 < n && text.at(i + 1) == QLatin1Char('"')) {
                    field.append(QLatin1Char('"'));
                    ++i;
                } else {
                    in_quotes = false;
                }
            } else {
                field.append(c);
            }
        } else if (c == QLatin1Char('"') && !field_started) {
            in_quotes = true;
            field_started = true;
        } else if (c == separator) {
            endField();
        } else if (c == QLatin1Char('\n') || c == QLatin1Char('\r')) {
            if (c == QLatin1Char('\r') && i + 1 < n && text.at(i + 1) == QLatin1Char('\n')) ++i;
            endRecord();
        } else {
            field.append(c);
            field_started = true;
        }
    }
    if (field_started || !field.isEmpty() || !record.isEmpty()) endRecord();
}

QString escapeCsvField(const QString& field, QChar separator) {
    if (field.contains(separator) || field.contains(QLatin1Char('"')) ||
        field.contains(QLatin1Char('\n')) || field.contains(QLatin1Char('\r'))) {
        QString escaped = field;
        escaped.replace(QStringLiteral("\""), QStringLiteral("\"\""));
        return QLatin1Char('"') + escaped + QLatin1Char('"');
    }
    return field;
}

// --- XLSX --------------------------------------------------------------------

// sharedStrings.xml: one <si> per shared string; <t> children are
// concatenated (rich text runs).
QStringList readSharedStrings(const QByteArray& xml) {
    QStringList strings;
    QXmlStreamReader reader(xml);
    QString current;
    bool in_si = false;
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement()) {
            if (reader.name() == QLatin1String("si")) {
                in_si = true;
                current.clear();
            } else if (in_si && reader.name() == QLatin1String("t")) {
                current.append(reader.readElementText());
            }
        } else if (reader.isEndElement()) {
            if (reader.name() == QLatin1String("si")) {
                strings.append(current);
                in_si = false;
            }
        }
    }
    return strings;
}

// workbook.xml + workbook.xml.rels: sheet name -> worksheet path.
std::map<QString, QString> readWorkbookSheets(const QByteArray& workbook_xml,
                                              const QByteArray& rels_xml) {
    std::map<QString, QString> name_to_target; // rId -> target
    QXmlStreamReader rels(rels_xml);
    while (!rels.atEnd()) {
        rels.readNext();
        if (rels.isStartElement() && rels.name() == QLatin1String("Relationship")) {
            const QString id = rels.attributes().value(QLatin1String("Id")).toString();
            QString target = rels.attributes().value(QLatin1String("Target")).toString();
            if (target.startsWith(QLatin1Char('/'))) target.remove(0, 1);
            else if (!target.startsWith(QLatin1String("xl/"))) target.prepend(QLatin1String("xl/"));
            name_to_target[id] = target;
        }
    }

    std::map<QString, QString> sheets; // name -> worksheet path
    QXmlStreamReader reader(workbook_xml);
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement() && reader.name() == QLatin1String("sheet")) {
            const QString name = reader.attributes().value(QLatin1String("name")).toString();
            const QString rid = reader.attributes().value(QLatin1String("r:id")).toString();
            const auto it = name_to_target.find(rid);
            if (it != name_to_target.end()) sheets[name] = it->second;
        }
    }
    return sheets;
}

// Column letters of a cell reference ("BC12" -> 54, 0-based).
int columnOfReference(const QString& ref) {
    int column = 0;
    for (const QChar c : ref) {
        if (!c.isLetter()) break;
        column = column * 26 + (c.toUpper().unicode() - 'A' + 1);
    }
    return column - 1;
}

// One worksheet: rows of cells. Shared-string cells (t="s") are resolved
// through `shared`; inline strings (t="inlineStr"/<is><t>) are read
// directly; numbers are kept as their text.
QList<QStringList> readSheet(const QByteArray& xml, const QStringList& shared,
                             qint64& total_rows) {
    QList<QStringList> rows;
    QStringList current;
    QString cell_text;
    QString cell_ref;
    QString cell_type;
    bool in_cell = false;
    bool in_text = false;
    bool row_open = false;

    auto flushRow = [&]() {
        rows.append(current);
        current.clear();
    };

    QXmlStreamReader reader(xml);
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement()) {
            const QStringView name = reader.name();
            if (name == QLatin1String("row")) {
                if (row_open) flushRow();
                row_open = true;
                current.clear();
            } else if (name == QLatin1String("c")) {
                cell_ref = reader.attributes().value(QLatin1String("r")).toString();
                cell_type = reader.attributes().value(QLatin1String("t")).toString();
                cell_text.clear();
                in_cell = true;
            } else if (in_cell && (name == QLatin1String("v") || name == QLatin1String("t"))) {
                in_text = true;
            }
        } else if (reader.isEndElement()) {
            const QStringView name = reader.name();
            if (name == QLatin1String("row")) {
                flushRow();
                row_open = false;
                ++total_rows;
            } else if (name == QLatin1String("c")) {
                QString value = cell_text;
                if (cell_type == QLatin1String("s")) {
                    const int index = cell_text.toInt();
                    if (index >= 0 && index < shared.size()) value = shared.at(index);
                }
                const int column = columnOfReference(cell_ref);
                while (current.size() <= column) current.append(QString());
                current[column] = value;
                in_cell = false;
            } else if (in_cell && (name == QLatin1String("v") || name == QLatin1String("t"))) {
                in_text = false;
            }
        } else if (reader.isCharacters() && in_cell && in_text) {
            cell_text.append(reader.text());
        }
    }
    if (row_open) flushRow();
    return rows;
}

} // namespace

QChar csvSeparator() {
    return QLocale().decimalPoint() == QLatin1Char(',') ? QLatin1Char(';') : QLatin1Char(',');
}

TableData readCsvFile(const QString& path) {
    TableData data;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        data.error = QStringLiteral("Cannot open %1").arg(path);
        return data;
    }
    // UTF-8, per the BonsaiViewer07 export rule.
    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    const QString text = stream.readAll();

    QList<QStringList> records;
    parseCsv(text, csvSeparator(), records);
    if (records.isEmpty()) {
        data.error = QStringLiteral("Empty file %1").arg(path);
        return data;
    }
    data.columns = records.takeFirst();
    data.rows = records.mid(0, kMaxLoadedRows);
    data.total_rows = records.size();
    data.ok = true;
    return data;
}

QStringList xlsxSheets(const QString& path) {
    QString error;
    const auto entries = ports::bcf::zip::read(path, &error);
    if (!entries) return {};
    const auto workbook = entries->find(QStringLiteral("xl/workbook.xml"));
    const auto rels = entries->find(QStringLiteral("xl/_rels/workbook.xml.rels"));
    if (workbook == entries->end() || rels == entries->end()) return {};
    QStringList names;
    for (const auto& [name, target] : readWorkbookSheets(workbook->second, rels->second)) {
        names.append(name);
    }
    return names;
}

TableData readXlsxFile(const QString& path, const QString& sheet) {
    TableData data;
    QString error;
    const auto entries = ports::bcf::zip::read(path, &error);
    if (!entries) {
        data.error = error.isEmpty() ? QStringLiteral("Not a readable .xlsx: %1").arg(path) : error;
        return data;
    }

    const auto workbook = entries->find(QStringLiteral("xl/workbook.xml"));
    const auto rels = entries->find(QStringLiteral("xl/_rels/workbook.xml.rels"));
    if (workbook == entries->end() || rels == entries->end()) {
        data.error = QStringLiteral("Missing workbook parts in %1").arg(path);
        return data;
    }
    const auto sheets = readWorkbookSheets(workbook->second, rels->second);
    for (const auto& [name, target] : sheets) data.sheets.append(name);
    if (sheets.empty()) {
        data.error = QStringLiteral("No sheet in %1").arg(path);
        return data;
    }

    // The requested sheet, else the first one.
    QString chosen_name;
    QString chosen_target;
    if (!sheet.isEmpty()) {
        const auto it = sheets.find(sheet);
        if (it != sheets.end()) {
            chosen_name = it->first;
            chosen_target = it->second;
        }
    }
    if (chosen_target.isEmpty()) {
        chosen_name = sheets.begin()->first;
        chosen_target = sheets.begin()->second;
    }
    data.sheet = chosen_name;

    QStringList shared;
    if (const auto it = entries->find(QStringLiteral("xl/sharedStrings.xml")); it != entries->end()) {
        shared = readSharedStrings(it->second);
    }
    const auto sheet_it = entries->find(chosen_target);
    if (sheet_it == entries->end()) {
        data.error = QStringLiteral("Missing sheet part %1 in %2").arg(chosen_target, path);
        return data;
    }

    qint64 total = 0;
    const QList<QStringList> all_rows = readSheet(sheet_it->second, shared, total);
    if (all_rows.isEmpty()) {
        data.error = QStringLiteral("Empty sheet %1 in %2").arg(chosen_name, path);
        return data;
    }
    data.columns = all_rows.first();
    const QList<QStringList> body = all_rows.mid(1);
    data.rows = body.mid(0, kMaxLoadedRows);
    data.total_rows = body.size();
    data.ok = true;
    return data;
}

TableData readTableFile(const QString& path, const QString& sheet) {
    const QString suffix = QFileInfo(path).suffix().toLower();
    if (suffix == QLatin1String("csv")) return readCsvFile(path);
    if (suffix == QLatin1String("xlsx")) return readXlsxFile(path, sheet);
    TableData data;
    data.error = QStringLiteral("Unsupported table file %1").arg(path);
    return data;
}

bool writeCsvFile(const QString& path, const QStringList& columns, const QList<QStringList>& rows) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    const QChar separator = csvSeparator();

    QStringList line;
    for (const QString& column : columns) line.append(escapeCsvField(column, separator));
    stream << line.join(separator) << QLatin1Char('\n');
    for (const QStringList& row : rows) {
        line.clear();
        for (int i = 0; i < columns.size(); ++i) {
            line.append(escapeCsvField(i < row.size() ? row.at(i) : QString(), separator));
        }
        stream << line.join(separator) << QLatin1Char('\n');
    }
    stream.flush();
    return stream.status() == QTextStream::Ok;
}

} // namespace bonsaiviewer::modules::external_documents
