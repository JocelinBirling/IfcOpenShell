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

#ifndef IFCINTERFACE_MODULES_BCF_TOPICDIALOG_H
#define IFCINTERFACE_MODULES_BCF_TOPICDIALOG_H

#include "../../ports/bcf/Bcf.h"

#include <QDialog>
#include <QString>

class QCheckBox;
class QComboBox;
class QDateTimeEdit;
class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPlainTextEdit;
class QTableWidget;

namespace bonsaiviewer::modules::bcf {

class BcfView;

// BonsaiViewer10 — the page of one topic (non-modal, one per topic): every
// field of the topic, editable (Apply / OK); its image (edited in the
// system's image editor); its document references (opened with a click);
// its related topics, both ways (a double click navigates); its comments,
// each with its image and text. The first comment captures the active
// view and its image; the next ones only their text, unless "Add with the
// View" is used. A comment's right click: Isolate, Zoom, Retake the Image,
// Refresh the Data, Edit the Image. A double click applies its viewpoint.
// The element lists of a viewpoint (selection, visibility, colours) open
// in their own windows.
class TopicDialog : public QDialog {
    Q_OBJECT

public:
    TopicDialog(BcfView& view, const QString& topic_guid, QWidget* parent = nullptr);

    // Fills the page from the topic (after a change made elsewhere). The
    // fields keep their edits until Apply.
    void reload();

private:
    ports::bcf::Topic* topic();
    // The viewpoint of the selected comment, else the topic's main one.
    ports::bcf::Viewpoint* currentViewpoint();
    ports::bcf::Viewpoint* viewpoint(const QString& guid);
    ports::bcf::Comment* comment(const QString& guid);
    void apply();
    void changed();

    void addComment(bool with_view);
    void captureInto(const QString& viewpoint_guid, bool image, bool data);
    QString newViewpoint();
    void showCommentMenu(const QPoint& pos);
    void applyComment(QListWidgetItem* item);
    void editImage(const QString& viewpoint_guid);

    void openDocument(int row);
    void addLink();
    void addFile();
    void removeDocument();
    void addRelated();
    void removeRelated();
    void editElements(int kind);

    BcfView& view_;
    QString guid_;
    bool filling_ = false;
    // A field was edited and not applied yet: reload() leaves the fields alone.
    bool fields_edited_ = false;

    QLabel* image_ = nullptr;
    QLabel* info_ = nullptr;
    QLineEdit* title_ = nullptr;
    QComboBox* type_ = nullptr;
    QComboBox* status_ = nullptr;
    QComboBox* priority_ = nullptr;
    QLineEdit* labels_ = nullptr;
    QComboBox* assigned_ = nullptr;
    QComboBox* stage_ = nullptr;
    QCheckBox* has_due_ = nullptr;
    QDateTimeEdit* due_ = nullptr;
    QPlainTextEdit* description_ = nullptr;
    QPlainTextEdit* links_ = nullptr;
    QTableWidget* documents_ = nullptr;
    QListWidget* related_to_ = nullptr;
    QListWidget* related_by_ = nullptr;
    QListWidget* comments_ = nullptr;
    QPlainTextEdit* new_comment_ = nullptr;
};

} // namespace bonsaiviewer::modules::bcf

#endif // IFCINTERFACE_MODULES_BCF_TOPICDIALOG_H
