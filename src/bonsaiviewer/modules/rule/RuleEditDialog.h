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

#ifndef IFCINTERFACE_MODULES_RULE_RULEEDITDIALOG_H
#define IFCINTERFACE_MODULES_RULE_RULEEDITDIALOG_H

#include "../../services/clash/RuleModel.h"
#include "../filters/FacetPages.h" // IfcPathProvider

#include <QDialog>
#include <QHash>
#include <QStringList>

#include <memory>
#include <vector>

class QComboBox;
class QLineEdit;
class QPlainTextEdit;
class QTreeWidget;
class QTreeWidgetItem;
class QVBoxLayout;

namespace bonsaiviewer::services { class IdsBridge; }
namespace bonsaiviewer::modules::filters { class SelectionEditor; }

namespace bonsaiviewer::modules::rule {

namespace clash = bonsaiviewer::services::clash;

class RuleDocument;

// A boolean rule tree (BooleanLeaf / And / Or / Not): the exception of a
// two-object rule (its leaves run on each pair, no selection of their own)
// or the activation of a folder (its leaves run on their own selections).
class BooleanEditor : public QWidget {
    Q_OBJECT

public:
    BooleanEditor(bool leaves_have_selections, QWidget* parent = nullptr);
    void setRoot(std::shared_ptr<clash::BooleanNode> root);
    std::shared_ptr<clash::BooleanNode> root() const { return root_; }

private:
    void rebuild(clash::BooleanNode* current);
    void add(std::shared_ptr<clash::BooleanNode> node);
    void showNode(clash::BooleanNode* node);
    clash::BooleanNode* selected() const;
    clash::BooleanNode* parentOf(clash::BooleanNode* node, clash::BooleanNode* within) const;

    bool with_selections_;
    std::shared_ptr<clash::BooleanNode> root_;
    QTreeWidget* tree_ = nullptr;
    QHash<QTreeWidgetItem*, clash::BooleanNode*> nodes_;
    QWidget* node_editor_ = nullptr;
    QVBoxLayout* node_layout_ = nullptr;
};

// BonsaiViewer09 (TI09) — the editing window of spec 09, anchored to a
// rule: its title, description, state and parameters; the source / target /
// context selections (the IDS selection editor, or the elements of another
// rule of the tree, which opens in its own editing window: cascade); the
// exception (two-object rules only), the Must check, the grouping, the
// criticity and the actor. Edits a copy; OK writes it back.
class RuleEditDialog : public QDialog {
    Q_OBJECT

public:
    // The IDS editor of the slots: the bridge and the active model's IFC.
    struct FacetContext {
        bonsaiviewer::services::IdsBridge& bridge;
        filters::IfcPathProvider ifc_path;
    };

    // `document` gives the rules of the tree a slot can take its elements
    // from (null: facets only).
    RuleEditDialog(std::shared_ptr<clash::RuleCheck> rule, RuleDocument* document, FacetContext context,
                   QWidget* parent = nullptr);
    // Writes the edited configuration into the rule.
    void apply();
    // Refuses OK while a slot's facets cannot be built.
    void accept() override;

signals:
    // The preview of a slot's IDS editor: "Isolate in the view" (EXPRESS
    // ids of the active model).
    void isolateRequested(const std::vector<int>& express_ids);
    // A rule of the tree was edited from a cascade (its own window).
    void ruleEdited();

private:
    // The edited state of one slot.
    struct SlotState {
        QString name;
        int mode = 0; // 0 empty, 1 IDS facets, 2 rule of the tree, 3 nested rule
        QComboBox* mode_combo = nullptr;
        filters::SelectionEditor* editor = nullptr;
        clash::Select original; // a nested rule: a copy, edited in its own window
        QComboBox* rule = nullptr;
        QComboBox* element = nullptr;
    };
    // Shows `select` in the slot's widgets (opening, paste).
    void loadSlot(SlotState& slot, const clash::Select& select);
    // The right-click menu of a slot: copy / paste its elements.
    void slotMenu(SlotState& slot, const QPoint& global_pos);
    QWidget* buildRulePage(bool two_object);
    QWidget* buildSelectionPage(const QStringList& slot_names, bool two_object);
    // The facets of a slot's IDS editor, in the engine's form.
    std::vector<bonsaiviewer::ports::ifctester::Facet> facetsOf(const SlotState& slot, QString* error) const;
    clash::Select selectOf(const SlotState& slot) const;
    // Opens a rule in its own editing window (a cascade); true when accepted.
    bool editNested(const std::shared_ptr<clash::RuleCheck>& rule);

    std::shared_ptr<clash::RuleCheck> rule_;
    std::shared_ptr<clash::RuleCheck> copy_;
    RuleDocument* document_ = nullptr;
    FacetContext context_;
    std::vector<std::unique_ptr<SlotState>> slots_;
    BooleanEditor* exception_tree_ = nullptr;
};

// The editing window of a folder: its activation (none, IDS facets, or a
// boolean rule tree).
class FolderEditDialog : public QDialog {
    Q_OBJECT

public:
    FolderEditDialog(std::shared_ptr<clash::RuleFolder> folder, QWidget* parent = nullptr);
    void apply();

private:
    std::shared_ptr<clash::RuleFolder> folder_;
    clash::RuleFolder copy_;
    BooleanEditor* tree_editor_ = nullptr;
    int mode_ = 0; // 0 none, 1 facets, 2 rule tree
};

} // namespace bonsaiviewer::modules::rule

#endif // IFCINTERFACE_MODULES_RULE_RULEEDITDIALOG_H
