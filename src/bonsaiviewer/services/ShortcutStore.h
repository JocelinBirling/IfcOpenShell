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

#ifndef IFCINTERFACE_SERVICES_SHORTCUTSTORE_H
#define IFCINTERFACE_SERVICES_SHORTCUTSTORE_H

#include <QHash>
#include <QKeySequence>
#include <QObject>
#include <QString>

#include <vector>

// BonsaiViewer31 — Keybindings. The list of the viewer's functions, their
// default shortcut and the shortcut each one currently uses. Bindings are
// stored in the user's settings (QSettings, group "shortcuts") and can be
// exported to / imported from a JSON file. A shortcut is stored as typed
// (portable text), so an export is tied to one keyboard layout.
//
// Ids are stable: they name the function in the settings and in the files.

namespace bonsaiviewer::services {

class ShortcutStore : public QObject {
    Q_OBJECT

public:
    struct Definition {
        QString id;
        QString label;
        QString default_key; // portable text; empty when the function has none
        // Functions of one scope must not share a key. "fly": the fly-mode
        // move keys, active only in fly mode, so they may reuse S, A...
        QString scope;
    };

    static ShortcutStore& instance();

    // A key handled directly by the 3D view or a panel: shown in the
    // Keybindings tab for reference, not editable.
    struct FixedShortcut {
        QString label;
        QString keys;
    };

    const std::vector<Definition>& definitions() const { return definitions_; }
    static const std::vector<FixedShortcut>& fixedShortcuts();
    const Definition* definition(const QString& id) const;

    // The shortcut of a function (empty when it has none).
    QKeySequence sequence(const QString& id) const;
    // The id of another function already using `sequence`, or an empty string.
    QString conflictingId(const QString& id, const QKeySequence& sequence) const;

    // Gives `sequence` to `id`. Returns false, and changes nothing, when
    // another function already uses it. An empty sequence removes the shortcut.
    bool assign(const QString& id, const QKeySequence& sequence);
    // Puts back the default shortcut. Returns false, and changes nothing, when
    // another function already uses that default.
    bool reset(const QString& id);
    void resetAll();

    // Writes every binding to `path`. Returns false with `error` set on failure.
    bool exportToFile(const QString& path, QString* error) const;
    // Replaces every binding with the ones of `path`: a function missing from
    // the file loses its shortcut. A file whose bindings collide is refused and
    // nothing changes. Returns false with `error` set on failure.
    bool importFromFile(const QString& path, QString* error);

signals:
    void changed();

private:
    ShortcutStore();
    void load();
    void save() const;

    std::vector<Definition> definitions_;
    QHash<QString, QKeySequence> sequences_;
};

} // namespace bonsaiviewer::services

#endif // IFCINTERFACE_SERVICES_SHORTCUTSTORE_H
