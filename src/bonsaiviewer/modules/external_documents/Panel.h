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

#ifndef BONSAIVIEWER_MODULES_EXTERNAL_DOCUMENTS_PANEL_H
#define BONSAIVIEWER_MODULES_EXTERNAL_DOCUMENTS_PANEL_H

#include "../../components/Panel.h"

class QTreeWidget;
class QTreeWidgetItem;

namespace bonsaiviewer::modules::external_documents {

// BonsaiViewer35/36 — the External Documents panel: a toolbar (add folder,
// import document) above the tree of folders and documents. The tree itself
// is populated by ExternalDocumentsView; the panel is only the shell.
// Default position: bottom left of the Table tab, next to the Tables panel.
class ExternalDocumentsPanel : public components::Panel {
    Q_OBJECT

public:
    explicit ExternalDocumentsPanel(QWidget* parent = nullptr);

    QTreeWidget* treeWidget() const { return tree_; }

signals:
    void addFolderRequested();
    void importRequested();

private:
    QTreeWidget* tree_ = nullptr;
};

} // namespace bonsaiviewer::modules::external_documents

#endif // BONSAIVIEWER_MODULES_EXTERNAL_DOCUMENTS_PANEL_H
