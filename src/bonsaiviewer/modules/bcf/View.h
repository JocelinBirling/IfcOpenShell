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

#ifndef IFCINTERFACE_MODULES_BCF_VIEW_H
#define IFCINTERFACE_MODULES_BCF_VIEW_H

#include "../../ports/bcf/Bcf.h"

#include <QMap>
#include <QObject>
#include <QPointer>
#include <QSet>
#include <QString>

#include <optional>

namespace bonsaiviewer::services { class BcfViewpoints; }

namespace bonsaiviewer::modules::bcf {

class BcfPanel;
class TopicDialog;

// BonsaiViewer10 — drives the BCF panel: loads and saves the BCF file
// (ports/bcf), lists and searches the topics, creates topics from the
// active view, and runs the topic actions (zoom, isolate, modify, delete).
// A deleted topic stays listed, struck out, until the next Save removes it.
class BcfView : public QObject {
    Q_OBJECT

public:
    BcfView(BcfPanel* panel, services::BcfViewpoints& viewpoints, QObject* parent = nullptr);

    // The BCF data the topic pages edit.
    ports::bcf::Topic* topic(const QString& guid);
    const ports::bcf::BcfFile* file() const { return bcf_ ? &*bcf_ : nullptr; }
    ports::bcf::BcfFile* file() { return bcf_ ? &*bcf_ : nullptr; }
    QString filePath() const { return path_; }
    services::BcfViewpoints& viewpoints() { return viewpoints_; }
    bool isDeleted(const QString& guid) const { return deleted_.contains(guid); }

    // The author of new topics and comments (Settings > BCF), asked once if unset.
    QString ensureAuthor();
    // A topic changed (title, comments...): the list follows, the file is unsaved.
    void topicChanged(const QString& guid);
    void openTopic(const QString& guid);
    // The viewpoint shown for a topic: the lowest index, else the first.
    const ports::bcf::Viewpoint* mainViewpoint(const ports::bcf::Topic& topic) const;

private:
    void load();
    void save();
    void newTopic();
    void refresh();
    bool matches(const ports::bcf::Topic& topic, const QString& search) const;
    QStringList selectedGuids() const;
    void showMenu(const QPoint& global_pos);
    void zoom(const QStringList& guids);
    void isolate(const QStringList& guids);
    void setDeleted(const QStringList& guids, bool deleted);
    void updateSummary();
    bool confirmDiscard();

    BcfPanel* panel_ = nullptr;
    services::BcfViewpoints& viewpoints_;
    std::optional<ports::bcf::BcfFile> bcf_;
    QString path_;
    QSet<QString> deleted_;
    bool dirty_ = false;
    QMap<QString, QPointer<TopicDialog>> pages_;
};

} // namespace bonsaiviewer::modules::bcf

#endif // IFCINTERFACE_MODULES_BCF_VIEW_H
