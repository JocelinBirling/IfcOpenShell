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

#ifndef IFCINTERFACE_MODULES_TEXTDIGGER_VIEW_H
#define IFCINTERFACE_MODULES_TEXTDIGGER_VIEW_H

#include <QObject>
#include <QString>
#include <QStringList>

#include <cstdint>
#include <map>
#include <utility>

namespace bonsaiviewer { class SessionState; }
namespace bonsaiviewer::services { class SelectionService; }

namespace bonsaiviewer::modules::text_digger {

class TextDiggerPanel;

// BonsaiViewer16 — drives the Ifc Text Digger. Inspect shows the STEP line of
// each selected element, read from the IFC text file on demand. A click on a
// reference hashtag shows the line that defines it; a click on the declaration
// hashtag of a shown line shows the lines that reference it. Lines are kept
// sorted in ascending order (by file, then by line number).
class TextDiggerView : public QObject {
    Q_OBJECT

public:
    TextDiggerView(TextDiggerPanel* panel,
                   bonsaiviewer::SessionState& session,
                   bonsaiviewer::services::SelectionService& selection,
                   QObject* parent = nullptr);

private:
    void inspect();
    void reset();
    void follow(const QString& link);
    void render();

    // Index of a file in files_, added when new.
    int fileIndex(const QString& path);
    QString filePathOf(std::uint32_t session_model_id) const;

    TextDiggerPanel* panel_ = nullptr;
    bonsaiviewer::SessionState& session_;
    bonsaiviewer::services::SelectionService& selection_;
    // The IFC text files shown; a line refers to its file by index.
    QStringList files_;
    // (file index, line number) -> the STEP record text.
    std::map<std::pair<int, int>, QString> lines_;
    // The status of the last action when it is not just the line count.
    QString message_;
};

} // namespace bonsaiviewer::modules::text_digger

#endif // IFCINTERFACE_MODULES_TEXTDIGGER_VIEW_H
