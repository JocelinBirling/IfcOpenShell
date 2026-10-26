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
 ********************************************************************************/

#include "View.h"

#include "Panel.h"

#include "../../ElementRegistry.h"
#include "../../SessionState.h"
#include "../../services/SelectionService.h"
#include "../../../ifcviewer/SceneLoader.h"

#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QRegularExpression>
#include <QSet>

#include <functional>

namespace bonsaiviewer::modules::text_digger {

namespace {

// One STEP record: "#223=IFCWALL(#2,#3,'Text');", possibly on several lines.
struct Record {
    int line = 0;
    QString text;
};

// Streams the records of a STEP text file. `visit` returns false to stop.
// Lines outside the records (header, footer) are skipped.
void forEachRecord(const QString& path, const std::function<bool(const Record&)>& visit) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return;

    Record current;
    bool open = false;
    int number = 0;
    while (!file.atEnd()) {
        const QString line = QString::fromUtf8(file.readLine()).trimmed();
        ++number;
        if (line.startsWith('#')) {
            if (open && !visit(current)) return;
            current = Record{number, line};
            open = true;
        } else if (open) {
            current.text += ' ' + line;
        } else {
            continue;
        }
        if (current.text.endsWith(';')) {
            if (!visit(current)) return;
            open = false;
        }
    }
    if (open) visit(current);
}

// The id of the record's declaration ("#223=" -> 223), or -1 when the text is not one.
int declaredId(const QString& text) {
    static const QRegularExpression declaration(QStringLiteral(R"(^#(\d+)=)"));
    const auto match = declaration.match(text);
    return match.hasMatch() ? match.captured(1).toInt() : -1;
}

// The text with its hashtags as links: "ifcdecl" for the declaration hashtag at
// the start of the line, "ifcref" for the others. Each link carries the file
// index and the number, read back by TextDiggerView::follow.
QString linkify(const QString& text, int file) {
    static const QRegularExpression hashtag(QStringLiteral(R"(#(\d+))"));
    const int declared = declaredId(text);
    QString html;
    qsizetype last = 0;
    auto it = hashtag.globalMatch(text);
    while (it.hasNext()) {
        const auto match = it.next();
        const int number = match.captured(1).toInt();
        const bool is_declaration = match.capturedStart() == 0 && number == declared;
        html += text.mid(last, match.capturedStart() - last).toHtmlEscaped();
        html += QStringLiteral("<a href=\"%1:%2:%3\">#%3</a>")
                    .arg(is_declaration ? QStringLiteral("ifcdecl") : QStringLiteral("ifcref"))
                    .arg(file)
                    .arg(number);
        last = match.capturedEnd();
    }
    html += text.mid(last).toHtmlEscaped();
    return html;
}

} // namespace

TextDiggerView::TextDiggerView(TextDiggerPanel* panel,
                               bonsaiviewer::SessionState& session,
                               bonsaiviewer::services::SelectionService& selection,
                               QObject* parent)
    : QObject(parent), panel_(panel), session_(session), selection_(selection)
{
    connect(panel_, &TextDiggerPanel::inspectRequested, this, &TextDiggerView::inspect);
    connect(panel_, &TextDiggerPanel::resetRequested, this, &TextDiggerView::reset);
    connect(panel_, &TextDiggerPanel::hashtagClicked, this, &TextDiggerView::follow);
    // The lines of a closed project are stale.
    connect(&session_, &bonsaiviewer::SessionState::projectReset, this, [this]() { reset(); });
    connect(&session_, &bonsaiviewer::SessionState::projectOpened, this, [this](const QString&) { reset(); });
    render();
}

void TextDiggerView::inspect() {
    const auto ids = selection_.selectionIdsAsVector();
    if (ids.empty()) {
        message_ = "Select elements, then press Inspect.";
        render();
        return;
    }

    // The wanted declaration ids, per IFC text file.
    QMap<QString, QSet<int>> wanted;
    int unavailable = 0;
    for (std::uint32_t object_id : ids) {
        const auto info = session_.elementRegistry()->findBasicElementInfo(object_id);
        if (!info || info->ifc_id <= 0) continue;
        const QString path = filePathOf(info->session_model_id);
        if (path.isEmpty() || !QFileInfo::exists(path)) {
            ++unavailable;
            continue;
        }
        wanted[path].insert(info->ifc_id);
    }

    for (auto it = wanted.cbegin(); it != wanted.cend(); ++it) {
        const int file = fileIndex(it.key());
        QSet<int> pending = it.value();
        forEachRecord(it.key(), [this, file, &pending](const Record& record) {
            if (pending.remove(declaredId(record.text)) > 0) {
                lines_[{file, record.line}] = record.text;
            }
            return !pending.isEmpty();  // stop once every element is found
        });
    }

    message_ = unavailable > 0
                   ? QString("The IFC text file is not available for %1 element(s).").arg(unavailable)
                   : QString();
    render();
}

void TextDiggerView::reset() {
    lines_.clear();
    files_.clear();
    message_.clear();
    render();
}

void TextDiggerView::follow(const QString& link) {
    // "ifcref:<file>:<number>" or "ifcdecl:<file>:<number>"
    const QStringList parts = link.split(':');
    if (parts.size() != 3) return;
    const int file = parts[1].toInt();
    const int number = parts[2].toInt();
    if (file < 0 || file >= files_.size()) return;
    const QString& path = files_[file];

    int added = 0;
    if (parts[0] == QStringLiteral("ifcref")) {
        // The line that defines the hashtag.
        forEachRecord(path, [this, file, number, &added](const Record& record) {
            if (declaredId(record.text) != number) return true;
            lines_[{file, record.line}] = record.text;
            ++added;
            return false;
        });
        message_ = added > 0 ? QString() : QString("No line defines #%1.").arg(number);
    } else if (parts[0] == QStringLiteral("ifcdecl")) {
        // The lines that reference the hashtag.
        const QRegularExpression reference(QStringLiteral("#%1(?!\\d)").arg(number));
        forEachRecord(path, [this, file, number, reference, &added](const Record& record) {
            if (declaredId(record.text) != number && record.text.contains(reference)) {
                lines_[{file, record.line}] = record.text;
                ++added;
            }
            return true;
        });
        message_ = added > 0 ? QString() : QString("No line references #%1.").arg(number);
    }
    render();
}

void TextDiggerView::render() {
    if (lines_.empty()) {
        panel_->setHtml(QString());
        panel_->setStatus(message_.isEmpty() ? QStringLiteral("Nothing displayed. Select elements and press Inspect.")
                                             : message_);
        return;
    }
    QString html;
    for (const auto& [key, text] : lines_) {
        html += QStringLiteral("<p style=\"margin:0 0 6px 0; font-family: monospace;\">%1</p>")
                    .arg(linkify(text, key.first));
    }
    panel_->setHtml(html);
    const QString count = QStringLiteral("%1 line(s)").arg(static_cast<int>(lines_.size()));
    panel_->setStatus(message_.isEmpty() ? count : count + " — " + message_);
}

int TextDiggerView::fileIndex(const QString& path) {
    int index = files_.indexOf(path);
    if (index < 0) {
        files_.append(path);
        index = static_cast<int>(files_.size()) - 1;
    }
    return index;
}

QString TextDiggerView::filePathOf(std::uint32_t session_model_id) const {
    auto* loader = session_.loader();
    return (loader != nullptr && session_model_id != 0) ? loader->filePath(session_model_id) : QString();
}

} // namespace bonsaiviewer::modules::text_digger
