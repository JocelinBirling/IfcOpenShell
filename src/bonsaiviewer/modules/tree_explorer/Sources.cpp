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
        {QStringLiteral("classes"), QStringLiteral("Classes")},
    };
    return kinds;
}

IndexedSource* makeSource(const QString& tree_id, const Context& context, QObject* parent) {
    if (tree_id == QLatin1String("models")) return new ModelsSource(context, parent);
    if (tree_id == QLatin1String("classes")) return new ClassesSource(context, parent);
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

} // namespace bonsaiviewer::modules::tree_explorer
