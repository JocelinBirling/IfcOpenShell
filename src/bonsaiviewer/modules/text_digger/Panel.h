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

#ifndef IFCINTERFACE_MODULES_TEXTDIGGER_PANEL_H
#define IFCINTERFACE_MODULES_TEXTDIGGER_PANEL_H

#include "../../components/Panel.h"

#include <QString>

class QLabel;
class QPushButton;
class QTextBrowser;

namespace bonsaiviewer::modules::text_digger {

// BonsaiViewer16 — the Ifc Text Digger panel (bottom, in its own tab). A
// toolbar (Reset at the top left, Inspect), a status line and the STEP lines
// as rich text. The hashtags are links: TextDiggerView follows them.
class TextDiggerPanel : public components::Panel {
    Q_OBJECT

public:
    explicit TextDiggerPanel(QWidget* parent = nullptr);

    void setHtml(const QString& html);
    void setStatus(const QString& text);

signals:
    void inspectRequested();
    void resetRequested();
    // The text of a clicked link: "ifcref:<file>:<number>" or "ifcdecl:<file>:<number>".
    void hashtagClicked(const QString& link);

private:
    QPushButton* reset_button_ = nullptr;
    QPushButton* inspect_button_ = nullptr;
    QLabel* status_label_ = nullptr;
    QTextBrowser* browser_ = nullptr;
};

} // namespace bonsaiviewer::modules::text_digger

#endif // IFCINTERFACE_MODULES_TEXTDIGGER_PANEL_H
