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

#ifndef IFCINTERFACE_MODULES_BCF_ELEMENTLISTDIALOG_H
#define IFCINTERFACE_MODULES_BCF_ELEMENTLISTDIALOG_H

#include "../../ports/bcf/Bcf.h"

#include <QDialog>

class QCheckBox;
class QTableWidget;

namespace bonsaiviewer::services { class BcfViewpoints; }

namespace bonsaiviewer::modules::bcf {

// BonsaiViewer10 — one element list of a viewpoint, in its own window: the
// selection, the visibility (the default visibility and its exceptions) or
// the colours. Elements are removed, or added from the viewer's selection;
// a colour can be changed. OK keeps the edited list (info()).
class ElementListDialog : public QDialog {
    Q_OBJECT

public:
    enum class Kind { Selection = 0, Visibility = 1, Colours = 2 };

    ElementListDialog(Kind kind, const ports::bcf::VisualizationInfo& info, services::BcfViewpoints& viewpoints,
                      QWidget* parent = nullptr);

    const ports::bcf::VisualizationInfo& info() const { return info_; }

private:
    void fill();
    void removeRows();
    void addSelection();
    void changeColour();

    Kind kind_;
    ports::bcf::VisualizationInfo info_;
    services::BcfViewpoints& viewpoints_;
    QTableWidget* table_ = nullptr;
    QCheckBox* default_visible_ = nullptr;
};

} // namespace bonsaiviewer::modules::bcf

#endif // IFCINTERFACE_MODULES_BCF_ELEMENTLISTDIALOG_H
