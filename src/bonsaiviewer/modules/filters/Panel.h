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
 ********************************************************************************/

#ifndef IFCINTERFACE_MODULES_FILTERS_PANEL_H
#define IFCINTERFACE_MODULES_FILTERS_PANEL_H

#include "../../components/Panel.h"

class QLabel;
class QToolButton;
class QTreeWidget;

namespace bonsaiviewer::modules::filters {

// B06 — the Filters panel: a toolbar (add filter / add folder / run /
// run on the box / import / export) above a B12 tree of folders and
// filters. The tree widget itself is populated by the TreePanelWidget in
// the view; the panel is only the shell and forwards the toolbar clicks
// as signals.
class FiltersPanel : public components::Panel {
    Q_OBJECT

public:
    explicit FiltersPanel(QWidget* parent = nullptr);

    QTreeWidget* treeWidget() const { return tree_; }

    // The key (Qt::UserRole of column 0) of the selected node, or empty.
    QString currentKey() const;

    // Run buttons follow the selection: only a filter can be run.
    void setRunEnabled(bool enabled);
    // The hint shown while the tree has no filter and no folder.
    void setEmptyHintVisible(bool visible);

signals:
    void addFilterRequested();
    void addFolderRequested();
    void runRequested();
    void runOnBoxRequested();
    void importRequested();
    void exportRequested();

private:
    QTreeWidget* tree_ = nullptr;
    QLabel* empty_hint_ = nullptr;
    QToolButton* run_button_ = nullptr;
    QToolButton* run_box_button_ = nullptr;
};

} // namespace bonsaiviewer::modules::filters

#endif // IFCINTERFACE_MODULES_FILTERS_PANEL_H
