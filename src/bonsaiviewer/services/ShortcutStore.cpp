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

#include "ShortcutStore.h"

#include <QFile>
#include <QIODevice>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSettings>

namespace bonsaiviewer::services {

namespace {

const char* kFileFormat = "bonsai-shortcuts";
constexpr int kFileVersion = 1;
const char* kSettingsGroup = "shortcuts";

QString toText(const QKeySequence& sequence) {
    return sequence.toString(QKeySequence::PortableText);
}

QKeySequence fromText(const QString& text) {
    return QKeySequence::fromString(text, QKeySequence::PortableText);
}

} // namespace

ShortcutStore& ShortcutStore::instance() {
    static ShortcutStore store;
    return store;
}

ShortcutStore::ShortcutStore() {
    definitions_ = {
        {"search.focus", "Search Bar", "Ctrl+Space"},
        {"view.hide", "Hide", "H"},
        {"view.show", "Show", "S"},
        {"view.show_all", "Show All", "Alt+S"},
        {"view.invert", "Invert View", "Ctrl+I"},
        {"view.restore", "Restore View", "Ctrl+R"},
        {"view.undo", "Undo View", "Ctrl+Z"},
        {"section.plane", "Section Plane", "Ctrl+S"},
        {"section.remove", "Remove Section", ""},
        {"section.hide", "Hide Section", ""},
        {"section.suspend", "Suspend Section", "Ctrl+D"},
        {"select.all", "Select Everything", "Ctrl+A"},
        {"view.toggle_visibility", "Toggle Visibility", ""},
        {"color.paint", "Color", "C"},
        {"color.remove", "Remove Color", ""},
        {"color.transparency", "Transparency", "T"},
        {"box.set", "Set Box", "Ctrl+B"},
        {"box.add", "Add to Box", "Ctrl+N"},
        {"box.remove", "Remove From Box", "Ctrl+,"},
        {"box.isolate", "Isolate Box", "Ctrl+J"},
        {"box.hide", "Hide Box", "Ctrl+K"},
        {"view.fit", "Fit View", "Ctrl+F"},
        {"view.perpendicular", "Perpendicular View", "Ctrl+P"},
        // Not in the default list of BonsaiViewer31: the shortcuts the viewer
        // already had, kept so that these functions stay reachable.
        {"view.isolate_selection", "Isolate Selection", "Shift+H"},
        {"tool.measure_distance", "Measure Distance", "Ctrl+Shift+L"},
        {"tool.measure_area", "Measure Area", "Ctrl+Shift+A"},
        {"tool.measure_volume", "Measure Volume", "Ctrl+Shift+V"},
        {"camera.fly", "Fly Mode", "Shift+F"},
    };
    load();
}

const ShortcutStore::Definition* ShortcutStore::definition(const QString& id) const {
    for (const auto& def : definitions_) {
        if (def.id == id) return &def;
    }
    return nullptr;
}

QKeySequence ShortcutStore::sequence(const QString& id) const {
    return sequences_.value(id);
}

QString ShortcutStore::conflictingId(const QString& id, const QKeySequence& sequence) const {
    if (sequence.isEmpty()) return {};
    for (const auto& def : definitions_) {
        if (def.id != id && sequences_.value(def.id) == sequence) {
            return def.id;
        }
    }
    return {};
}

bool ShortcutStore::assign(const QString& id, const QKeySequence& sequence) {
    if (definition(id) == nullptr) return false;
    if (!conflictingId(id, sequence).isEmpty()) return false;
    sequences_.insert(id, sequence);
    save();
    emit changed();
    return true;
}

bool ShortcutStore::reset(const QString& id) {
    const auto* def = definition(id);
    if (def == nullptr) return false;
    const QKeySequence default_sequence = fromText(def->default_key);
    if (!conflictingId(id, default_sequence).isEmpty()) return false;
    sequences_.insert(id, default_sequence);
    save();
    emit changed();
    return true;
}

void ShortcutStore::resetAll() {
    for (const auto& def : definitions_) {
        sequences_.insert(def.id, fromText(def.default_key));
    }
    save();
    emit changed();
}

void ShortcutStore::load() {
    QSettings settings;
    settings.beginGroup(kSettingsGroup);
    for (const auto& def : definitions_) {
        const QString stored = settings.contains(def.id)
            ? settings.value(def.id).toString()
            : def.default_key;
        sequences_.insert(def.id, fromText(stored));
    }
    settings.endGroup();
}

void ShortcutStore::save() const {
    QSettings settings;
    settings.beginGroup(kSettingsGroup);
    for (const auto& def : definitions_) {
        settings.setValue(def.id, toText(sequences_.value(def.id)));
    }
    settings.endGroup();
}

bool ShortcutStore::exportToFile(const QString& path, QString* error) const {
    QJsonObject shortcuts;
    for (const auto& def : definitions_) {
        shortcuts.insert(def.id, toText(sequences_.value(def.id)));
    }
    QJsonObject root;
    root.insert("format", QString::fromUtf8(kFileFormat));
    root.insert("version", kFileVersion);
    root.insert("shortcuts", shortcuts);

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error != nullptr) *error = file.errorString();
        return false;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}

bool ShortcutStore::importFromFile(const QString& path, QString* error) {
    const auto fail = [error](const QString& message) -> bool {
        if (error != nullptr) *error = message;
        return false;
    };

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return fail(file.errorString());
    }
    QJsonParseError parse_error;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parse_error);
    if (parse_error.error != QJsonParseError::NoError || !document.isObject()) {
        return fail(QStringLiteral("the file is not a valid JSON document"));
    }
    const QJsonObject root = document.object();
    if (root.value("format").toString() != QString::fromUtf8(kFileFormat)) {
        return fail(QStringLiteral("the file is not a Bonsai shortcut file"));
    }

    const QJsonObject shortcuts = root.value("shortcuts").toObject();
    QHash<QString, QKeySequence> imported;
    for (const auto& def : definitions_) {
        imported.insert(def.id, fromText(shortcuts.value(def.id).toString()));
    }

    // A file whose own bindings collide cannot be applied.
    for (const auto& def : definitions_) {
        const QKeySequence sequence = imported.value(def.id);
        if (sequence.isEmpty()) continue;
        for (const auto& other : definitions_) {
            if (other.id != def.id && imported.value(other.id) == sequence) {
                return fail(QStringLiteral("%1 and %2 use the same shortcut")
                                .arg(def.label, other.label));
            }
        }
    }

    sequences_ = imported;
    save();
    emit changed();
    return true;
}

} // namespace bonsaiviewer::services
