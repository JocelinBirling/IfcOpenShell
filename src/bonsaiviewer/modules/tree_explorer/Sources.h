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

#ifndef IFCINTERFACE_MODULES_TREE_EXPLORER_SOURCES_H
#define IFCINTERFACE_MODULES_TREE_EXPLORER_SOURCES_H

#include "IndexedSource.h"

#include <QString>
#include <QStringList>

namespace bonsaiviewer::modules::tree_explorer {

// The trees of the Tree Explorer (BonsaiViewer32), by id.
struct TreeKind {
    QString id;
    QString title;
};
const QList<TreeKind>& treeKinds();
// The data source of a tree; nullptr for an unknown id.
IndexedSource* makeSource(const QString& tree_id, const Context& context, QObject* parent);

// Models: federation groups > models.
class ModelsSource : public IndexedSource {
    Q_OBJECT
public:
    explicit ModelsSource(const Context& context, QObject* parent = nullptr);
    bool supportsScope() const override { return false; }

protected:
    void build(bonsaiviewer::services::tree::TreeIndex& index, const QList<std::uint32_t>& models) override;
};

// Classes: IFC class > instances, from the element table (.ifcview), as the
// Ifc Tree panel (BonsaiViewer12).
class ClassesSource : public IndexedSource {
    Q_OBJECT
public:
    using IndexedSource::IndexedSource;

protected:
    void build(bonsaiviewer::services::tree::TreeIndex& index, const QList<std::uint32_t>& models) override;
};

} // namespace bonsaiviewer::modules::tree_explorer

#endif
