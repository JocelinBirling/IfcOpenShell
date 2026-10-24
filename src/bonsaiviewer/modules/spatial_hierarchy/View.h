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

#ifndef IFCINTERFACE_PANELS_SPATIALHIERARCHYPANELVIEW_H
#define IFCINTERFACE_PANELS_SPATIALHIERARCHYPANELVIEW_H

#include "Types.h"

#include <QObject>

namespace bonsaiviewer { class SessionState; }
class ViewportWindow;

namespace bonsaiviewer::modules::spatial_hierarchy {

class SpatialHierarchyPanel;

class SpatialHierarchyPanelView : public QObject {
    Q_OBJECT
public:
    explicit SpatialHierarchyPanelView(SpatialHierarchyPanel* widget,
                                       bonsaiviewer::SessionState* session_state,
                                       ViewportWindow* viewport,
                                       QObject* parent = nullptr);

private:
    void rebuild();  // re-derive nodes_ from the loaded models' IFC spatial structure
    void reload();   // push nodes_ into the tree widget
    TreeNode* findNode(const NodePath& path);
    // Push one node's visibility to the viewport: every element of the
    // node's spatial subtree (contained elements included) is shown or
    // hidden. No-op without a viewport or a live IFC source.
    void applyVisibility(const TreeNode& node);

    SpatialHierarchyPanel* widget_ = nullptr;
    bonsaiviewer::SessionState* session_state_ = nullptr;
    ViewportWindow* viewport_ = nullptr;
    QList<TreeNode> nodes_;
};

} // namespace bonsaiviewer::modules::spatial_hierarchy

#endif
