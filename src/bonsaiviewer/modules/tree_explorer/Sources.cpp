// This file was generated with the assistance of an AI coding tool.
/********************************************************************************
 *                                                                              *
 * This file is part of Bonsai.                                                 *
 *                                                                              *
 * Bonsai is free software: you can redistribute it and/or modify               *
 * it under the terms of the GNU General Public License as published by         *
 * the Free Software Foundation, either version 3.0 of the License, or          *
 * (at your option) any later version.                                           *
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

#include "Sources.h"

#include "../../ElementRegistry.h"
#include "../../SessionState.h"
#include "../../../ifcviewer/Federation.h"
#include "../../../ifcviewer/SceneLoader.h"

#include <QHash>

namespace bonsaiviewer::modules::tree_explorer {

using bonsaiviewer::services::tree::IndexNode;
using bonsaiviewer::services::tree::TreeIndex;

namespace {

const QString kFolderIcon = QStringLiteral(":/icons/folder.svg");
const QString kElementIcon = QStringLiteral(":/icons/cube.svg");

} // namespace

const QList<TreeKind>& treeKinds() {
    static const QList<TreeKind> kinds{
        {QStringLiteral("models"), QStringLiteral("Models")},
        {QStringLiteral("spatial"), QStringLiteral("Spatial")},
        {QStringLiteral("classes"), QStringLiteral("Classes")},
        {QStringLiteral("types"), QStringLiteral("Types")},
        {QStringLiteral("systems"), QStringLiteral("Systems")},
        {QStringLiteral("zones"), QStringLiteral("Zones")},
        {QStringLiteral("groups"), QStringLiteral("Groups")},
        {QStringLiteral("materials"), QStringLiteral("Materials")},
    };
    return kinds;
}

IndexedSource* makeSource(const QString& tree_id, const Context& context, QObject* parent) {
    if (tree_id == QLatin1String("models")) return new ModelsSource(context, parent);
    if (tree_id == QLatin1String("classes")) return new ClassesSource(context, parent);
    if (tree_id == QLatin1String("spatial")) {
        return new IfcRelationSource(context, ifc_trees::buildSpatial, true,
                                     QStringLiteral("No spatial structure in the models."), parent);
    }
    if (tree_id == QLatin1String("types")) {
        return new IfcRelationSource(context, ifc_trees::buildTypes, false, QStringLiteral("No types in the models."),
                                     parent);
    }
    const auto groups = [&](ifc_trees::GroupKind kind, const QString& empty) {
        return new IfcRelationSource(
            context,
            [kind](TreeIndex& index, const ifc_trees::ModelInput& input, int parent) {
                ifc_trees::buildGroups(index, input, parent, kind);
            },
            false, empty, parent);
    };
    if (tree_id == QLatin1String("systems")) {
        return groups(ifc_trees::GroupKind::Systems, QStringLiteral("No systems in the models."));
    }
    if (tree_id == QLatin1String("zones")) return groups(ifc_trees::GroupKind::Zones, QStringLiteral("No zones in the models."));
    if (tree_id == QLatin1String("groups")) {
        return groups(ifc_trees::GroupKind::Groups, QStringLiteral("No groups in the models."));
    }
    if (tree_id == QLatin1String("materials")) {
        return new IfcRelationSource(context, ifc_trees::buildMaterials, false,
                                     QStringLiteral("No materials in the models."), parent);
    }
    return nullptr;
}

// ---- Models -----------------------------------------------------------------

ModelsSource::ModelsSource(const Context& context, QObject* parent)
    : IndexedSource(context, parent)
{
    connect(&context_.session, &SessionState::federationChanged, this, [this]() { scheduleRebuild(); });
}

void ModelsSource::build(TreeIndex& index, const QList<std::uint32_t>&) {
    const Federation* federation = context_.session.federation();
    QHash<QString, int> group_nodes;
    // Groups, parents before children.
    for (const Federation::Group* group : federation->allGroups()) {
        IndexNode node;
        node.kind = IndexNode::Kind::Folder;
        node.id = QStringLiteral("g:") + group->id;
        node.label = group->display_name.isEmpty() ? QStringLiteral("Group") : group->display_name;
        node.icon = kFolderIcon;
        node.expanded = true;
        const int parent = group->parent ? group_nodes.value(group->parent->id, -1) : -1;
        group_nodes.insert(group->id, index.add(parent, node));
    }
    for (const Federation::Model& model : federation->models()) {
        const std::uint32_t session_model_id = context_.session.sessionModelIdForModelId(model.id);
        const QString name = model.display_name.isEmpty() ? model.id : model.display_name;
        IndexNode node = session_model_id != 0 ? modelNode(session_model_id, name) : messageNode(name + " (not loaded)");
        if (session_model_id != 0) {
            // A leaf: the model stands for all its objects.
            node.kind = IndexNode::Kind::Element;
            const auto elements = context_.session.elementRegistry()->basicElementInfoForModel(session_model_id);
            node.tooltip = QStringLiteral("%1: %2 elements").arg(name).arg(elements.size());
        }
        node.id = QStringLiteral("m:") + model.id;
        index.add(group_nodes.value(model.group_id, -1), node);
    }
}

// ---- Classes ----------------------------------------------------------------

void ClassesSource::build(TreeIndex& index, const QList<std::uint32_t>& models) {
    QHash<QString, int> class_nodes;
    for (std::uint32_t model : models) {
        for (const auto& info : context_.session.elementRegistry()->basicElementInfoForModel(model)) {
            if (info.type.isEmpty()) continue;
            int folder = class_nodes.value(info.type, -1);
            if (folder < 0) {
                IndexNode node;
                node.kind = IndexNode::Kind::Folder;
                node.id = info.type;
                node.label = info.type;
                node.icon = kFolderIcon;
                folder = index.add(-1, node);
                class_nodes.insert(info.type, folder);
            }
            IndexNode node;
            node.kind = IndexNode::Kind::Element;
            node.id = QString::number(info.object_id);
            // Instances without a name show their GlobalId.
            node.label = info.name.trimmed().isEmpty() ? info.guid : info.name;
            node.icon = kElementIcon;
            node.model = model;
            node.ifc_id = info.ifc_id;
            node.object_id = info.object_id;
            index.add(folder, node);
        }
    }
}

// ---- IFC relations ----------------------------------------------------------

IfcRelationSource::IfcRelationSource(const Context& context, Builder builder, bool always_model_level,
                                     const QString& empty_text, QObject* parent)
    : IndexedSource(context, parent), builder_(std::move(builder)), always_model_level_(always_model_level),
      empty_text_(empty_text)
{
}

void IfcRelationSource::build(TreeIndex& index, const QList<std::uint32_t>& models) {
    SceneLoader* loader = context_.session.loader();
    const bool model_level = always_model_level_ || models.size() > 1;
    for (std::uint32_t model : models) {
        const QString name = modelName(model);
        ifcopenshell::file* file = loader ? loader->ifcFile(model) : nullptr;
        if (file == nullptr) {
            // The IFC data source opens after the geometry (.ifcview hit).
            index.add(-1, messageNode(name + QStringLiteral(": IFC data loading…")));
            continue;
        }
        int parent = -1;
        if (model_level) {
            IndexNode node = modelNode(model, name);
            node.expanded = true;
            // Outside the Spatial tree, the model folder stands for the elements below only.
            if (!always_model_level_) node.model = 0;
            parent = index.add(-1, node);
        }
        ifc_trees::ModelInput input;
        input.model = model;
        input.file = file;
        for (const auto& info : context_.session.elementRegistry()->basicElementInfoForModel(model)) {
            if (info.ifc_id != 0) input.object_by_ifc.insert(info.ifc_id, info.object_id);
        }
        try {
            builder_(index, input, parent);
        } catch (const std::exception& e) {
            index.add(parent, messageNode(QStringLiteral("%1 cannot be read: %2").arg(name, QString::fromUtf8(e.what()))));
        }
    }
}

} // namespace bonsaiviewer::modules::tree_explorer
