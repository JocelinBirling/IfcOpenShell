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

#ifndef IFCINTERFACE_MODULES_RULE_RULEDOCUMENT_H
#define IFCINTERFACE_MODULES_RULE_RULEDOCUMENT_H

#include "../../services/clash/RuleModel.h"

#include <QObject>
#include <QString>
#include <QStringList>

#include <memory>
#include <optional>
#include <vector>

namespace bonsaiviewer::modules::rule {

namespace clash = bonsaiviewer::services::clash;

// BonsaiViewer09 (TI09) — the open rule file of the GeometryRule workspace,
// shared by the Rule tree, Rule Config, the editing window and the runs.
// One file at a time (New replaces it, as in the IfcViewer_Clash branch).
//
// Cascades: a slot filled from another rule of the tree holds a SelectRule
// with a copy of that rule under the same id — the copy is the by-value
// form of serialization.py. refreshLinks() puts the current version of the
// tree rule in every such copy before a run and a save.
class RuleDocument : public QObject {
    Q_OBJECT

public:
    explicit RuleDocument(QObject* parent = nullptr);

    clash::RuleFile& file() { return file_; }
    const clash::RuleFile& file() const { return file_; }
    bool isOpen() const { return open_; }
    QString path() const { return path_; }
    bool isModified() const { return modified_; }

    void newFile();
    void setFile(clash::RuleFile file, const QString& path);
    void setPath(const QString& path);
    void markSaved();
    // A change of the content: emits changed() (the tree rebuilds).
    void touch();

    // The rules of the tree (folders walked, not the copies in the slots).
    std::vector<std::shared_ptr<clash::RuleCheck>> treeRules() const;
    std::shared_ptr<clash::RuleCheck> treeRule(const QString& id) const;
    std::shared_ptr<clash::RuleFolder> folder(const QString& id) const;
    // The container (folder, or null for the file) and position of a node.
    std::vector<clash::RuleNode>* containerOf(const QString& id, size_t* index = nullptr);
    std::vector<clash::RuleNode>* contents(const QString& folder_id); // null id: the file
    // Removes a node from its container; returns it.
    std::optional<clash::RuleNode> take(const QString& id);

    // Cascades: the tree rules a rule takes elements from, and the ones
    // that take elements from it.
    QStringList fedBy(const clash::RuleCheck& rule) const;
    QStringList feeds(const clash::RuleCheck& rule) const;
    // Whether linking `from` into a slot of `into` would loop.
    bool wouldLoop(const QString& from_id, const QString& into_id) const;
    void refreshLinks();

    static QString labelOf(const clash::RuleCheck& rule);
    static QString labelOf(const clash::RuleFolder& folder);

signals:
    void changed();
    void opened();

private:
    clash::RuleFile file_;
    bool open_ = false;
    bool modified_ = false;
    QString path_;
};

} // namespace bonsaiviewer::modules::rule

#endif // IFCINTERFACE_MODULES_RULE_RULEDOCUMENT_H
