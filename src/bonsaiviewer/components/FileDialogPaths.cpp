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

#include "FileDialogPaths.h"

#include <QComboBox>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QKeyEvent>
#include <QLineEdit>

namespace bonsaiviewer::components {

namespace {

// Watches the line edit of the "Look in" bar and takes the Enter key when the
// text is an existing absolute path.
class PastedPathFilter : public QObject {
public:
    PastedPathFilter(QFileDialog* dialog, QLineEdit* edit)
        : QObject(dialog), dialog_(dialog), edit_(edit)
    {
    }

    bool eventFilter(QObject* watched, QEvent* event) override {
        if (event->type() != QEvent::KeyPress) return QObject::eventFilter(watched, event);
        const auto* key = static_cast<QKeyEvent*>(event);
        if (key->key() != Qt::Key_Return && key->key() != Qt::Key_Enter) {
            return QObject::eventFilter(watched, event);
        }

        QString text = edit_->text().trimmed();
        if (text.size() >= 2 && text.startsWith('"') && text.endsWith('"')) {
            text = text.mid(1, text.size() - 2);  // a path copied as "C:\..." with its quotes
        }
        const QFileInfo info(text);
        if (!info.isAbsolute() || !info.exists()) return QObject::eventFilter(watched, event);

        dialog_->setDirectory(info.isDir() ? info.absoluteFilePath() : info.absolutePath());
        edit_->clear();
        return true;  // handled: the dialog must not accept or complete the text
    }

private:
    QFileDialog* dialog_ = nullptr;
    QLineEdit* edit_ = nullptr;
};

} // namespace

void acceptPastedPaths(QFileDialog& dialog) {
    // Qt's own name for the "Look in" combo box of its file dialog.
    auto* look_in = dialog.findChild<QComboBox*>(QStringLiteral("lookInCombo"));
    if (look_in == nullptr) return;

    // Editable, so that a pasted path can be typed in; the list of folders stays.
    look_in->setEditable(true);
    if (QLineEdit* edit = look_in->lineEdit()) {
        edit->installEventFilter(new PastedPathFilter(&dialog, edit));
    }
}

} // namespace bonsaiviewer::components
