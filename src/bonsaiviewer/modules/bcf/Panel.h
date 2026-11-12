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

#ifndef IFCINTERFACE_MODULES_BCF_PANEL_H
#define IFCINTERFACE_MODULES_BCF_PANEL_H

#include "../../components/Panel.h"

class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QStackedWidget;
class QToolButton;
class QTreeWidget;

namespace bonsaiviewer::modules::bcf {

// BonsaiViewer10 — the BCF panel (the former "Issues" panel): a toolbar
// (Load, Save, New Topic), the search bar, the display mode, and the list
// of the topics as a table, thumbnails or a tree of related topics. Only
// the shell: BcfView drives it.
class BcfPanel : public components::Panel {
    Q_OBJECT

public:
    enum class Mode { Table = 0, Thumbnails = 1, Tree = 2 };

    explicit BcfPanel(QWidget* parent = nullptr);

    // Table and Tree modes.
    QTreeWidget* tree() const { return tree_; }
    QListWidget* thumbnails() const { return thumbnails_; }
    Mode mode() const;
    QString searchText() const;
    void setSummary(const QString& text);
    void setFileActionsEnabled(bool enabled);

signals:
    void loadRequested();
    void saveRequested();
    void newTopicRequested();
    void searchChanged();
    void modeChanged();

private:
    QToolButton* save_button_ = nullptr;
    QToolButton* new_topic_button_ = nullptr;
    QLineEdit* search_ = nullptr;
    QComboBox* mode_ = nullptr;
    QLabel* summary_ = nullptr;
    QStackedWidget* stack_ = nullptr;
    QTreeWidget* tree_ = nullptr;
    QListWidget* thumbnails_ = nullptr;
};

} // namespace bonsaiviewer::modules::bcf

#endif // IFCINTERFACE_MODULES_BCF_PANEL_H
