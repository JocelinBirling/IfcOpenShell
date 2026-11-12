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

#ifndef IFCINTERFACE_MODULES_RULE_RULEPANELS_H
#define IFCINTERFACE_MODULES_RULE_RULEPANELS_H

#include "RuleTree.h"
#include "../filters/FacetPages.h" // IfcPathProvider

#include <QPointer>

#include <QTreeWidget>
#include <QWidget>

#include <optional>

namespace bonsaiviewer::services { class IdsBridge; }
namespace bonsaiviewer::modules::filters { class SelectionEditor; }

class QLabel;
class QVBoxLayout;

namespace bonsaiviewer::modules::rule {

class FacetListEditor;
class FacetEditor;
class ParamForm;

// BonsaiViewer09 (TI09) — the Rule Library of the GeometryRule workspace:
// one section per family of the engine catalogue (one object, two
// objects, complex) and the IDS facets, dragged onto the Rule tree.
class RuleLibraryPanel : public QTreeWidget {
    Q_OBJECT

public:
    explicit RuleLibraryPanel(QWidget* parent = nullptr);

signals:
    void ruleTypeSelected(const QString& type);

protected:
    QMimeData* mimeData(const QList<QTreeWidgetItem*>& items) const override;
    QStringList mimeTypes() const override;
};

// Rule Config: what the selected node of the Rule tree holds — the
// parameters of a rule, the facets of a slot or one facet, the name of a
// folder or of the file; the documentation of a library rule.
class RuleConfigPanel : public QWidget {
    Q_OBJECT

public:
    RuleConfigPanel(RuleDocument& document, QWidget* parent = nullptr);

    void showRef(const std::optional<TreeRef>& ref);
    void showLibraryRule(const QString& type);
    // The IDS selection editor of the slots (as the Edit window): the bridge
    // and the active model's IFC. Without it, the slots are shown read only.
    void setFacetContext(bonsaiviewer::services::IdsBridge* bridge, filters::IfcPathProvider ifc_path);

signals:
    void editRuleRequested(std::shared_ptr<clash::RuleCheck> rule);
    void editFolderRequested(std::shared_ptr<clash::RuleFolder> folder);
    // "Isolate in the view" of the IDS editor's preview (EXPRESS ids of the active model).
    void isolateRequested(const std::vector<int>& express_ids);

private:
    void clear();
    void showSlot(const TreeRef& ref, class QVBoxLayout* layout);
    // Whether `ref` is the slot the IDS editor shows, unchanged since its last write.
    bool showsSlot(const std::optional<TreeRef>& ref) const;

    RuleDocument& document_;
    std::optional<TreeRef> shown_;
    QVBoxLayout* body_ = nullptr;
    QWidget* content_ = nullptr;
    bonsaiviewer::services::IdsBridge* bridge_ = nullptr;
    filters::IfcPathProvider ifc_path_;
    QPointer<filters::SelectionEditor> slot_editor_;
    QString written_; // the slot's facets as last written by the editor (JSON)
};

} // namespace bonsaiviewer::modules::rule

#endif // IFCINTERFACE_MODULES_RULE_RULEPANELS_H
