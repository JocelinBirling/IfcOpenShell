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

#ifndef BONSAIVIEWER_SERVICES_CLASH_OBJECTCLASHINDEX_H
#define BONSAIVIEWER_SERVICES_CLASH_OBJECTCLASHINDEX_H

#include <QObject>
#include <QString>

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace bonsaiviewer::services::clash {

// BonsaiViewer09 (TI09) — the clash rules of the session, per object: the
// results the object is in, and the rules that checked it without result.
// Published by the clash view (after every run, review or group change),
// read by the Properties panel (its "Clash" section), which sends the
// actions back (show the result, isolate it, review it).
class ObjectClashIndex : public QObject {
    Q_OBJECT

public:
    struct Entry {
        enum class Role { Source, Target, Tested, NotChecked };
        QString node_id;
        QString rule_label;
        QString rule_type;
        Role role = Role::Tested;
        QString other;   // the other element of the pair
        QString measure;
        QString review;  // Valid, Non Determined, Invalid
        QString group;   // the manual group
        bool excepted = false;
        int row = -1;    // the result in the Result panel (-1: no result)
    };

    explicit ObjectClashIndex(QObject* parent = nullptr) : QObject(parent) {}

    // The entries of every object (viewer object id), in rule order.
    void publish(std::unordered_map<std::uint32_t, std::vector<Entry>> entries, int rules) {
        entries_ = std::move(entries);
        rules_ = rules;
        emit changed();
    }
    const std::vector<Entry>* entriesOf(std::uint32_t object_id) const {
        const auto found = entries_.find(object_id);
        return found == entries_.end() ? nullptr : &found->second;
    }
    // The number of rules run in the session (0: the section says so).
    int rules() const { return rules_; }

signals:
    void changed();
    // From the Properties panel (row: the result in the Result panel).
    void showResultRequested(int row);
    void isolateRequested(int row);
    void reviewRequested(int row, const QString& review);

private:
    std::unordered_map<std::uint32_t, std::vector<Entry>> entries_;
    int rules_ = 0;
};

} // namespace bonsaiviewer::services::clash

#endif // BONSAIVIEWER_SERVICES_CLASH_OBJECTCLASHINDEX_H
