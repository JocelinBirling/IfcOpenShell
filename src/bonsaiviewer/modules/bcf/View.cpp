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

#include "View.h"

#include "Author.h"
#include "Panel.h"
#include "TopicDialog.h"

#include "../../components/FileDialogPaths.h"
#include "../../ports/bcf/BcfXml.h"
#include "../../services/BcfViewpoints.h"
#include "../../services/PopupService.h"

#include <QDateTime>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QInputDialog>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPixmap>
#include <QTreeWidget>

#include <algorithm>
#include <set>

namespace bonsaiviewer::modules::bcf {

namespace bcf = bonsaiviewer::ports::bcf;

namespace {

constexpr int kGuidRole = Qt::UserRole;
const QString kFullSearch = QStringLiteral("-fs");

QString shortDate(const QString& xs_date) {
    const QDateTime date = QDateTime::fromString(xs_date, Qt::ISODateWithMs);
    if (!date.isValid()) return xs_date;
    return date.toLocalTime().toString("yyyy-MM-dd HH:mm");
}

// Every field of the topic, for the full search.
QString fullText(const bcf::Topic& topic) {
    QStringList parts{topic.guid, topic.title, topic.topic_type, topic.topic_status, topic.priority,
                      topic.labels.join(' '), topic.creation_author, topic.modified_author, topic.assigned_to,
                      topic.stage, topic.description, topic.reference_links.join(' '),
                      topic.creation_date, topic.modified_date, topic.due_date, topic.server_assigned_id};
    for (const auto& reference : topic.document_references) parts << reference.description << reference.url;
    for (const auto& comment : topic.comments) parts << comment.author << comment.text << comment.date;
    if (topic.bim_snippet) parts << topic.bim_snippet->snippet_type << topic.bim_snippet->reference;
    for (const auto& file : topic.header_files) parts << file.filename << file.reference;
    return parts.join('\n');
}

} // namespace

BcfView::BcfView(BcfPanel* panel, services::BcfViewpoints& viewpoints, QObject* parent)
    : QObject(parent), panel_(panel), viewpoints_(viewpoints) {
    connect(panel_, &BcfPanel::loadRequested, this, &BcfView::load);
    connect(panel_, &BcfPanel::saveRequested, this, &BcfView::save);
    connect(panel_, &BcfPanel::newTopicRequested, this, &BcfView::newTopic);
    connect(panel_, &BcfPanel::searchChanged, this, &BcfView::refresh);
    connect(panel_, &BcfPanel::modeChanged, this, &BcfView::refresh);
    // Double click: zoom on the objects of the topic and isolate them.
    connect(panel_->tree(), &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem* item) {
        const QStringList guids{item->data(0, kGuidRole).toString()};
        isolate(guids);
        zoom(guids);
    });
    connect(panel_->thumbnails(), &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item) {
        const QStringList guids{item->data(kGuidRole).toString()};
        isolate(guids);
        zoom(guids);
    });
    connect(panel_->tree(), &QWidget::customContextMenuRequested, this,
            [this](const QPoint& pos) { showMenu(panel_->tree()->viewport()->mapToGlobal(pos)); });
    connect(panel_->thumbnails(), &QWidget::customContextMenuRequested, this,
            [this](const QPoint& pos) { showMenu(panel_->thumbnails()->viewport()->mapToGlobal(pos)); });
    refresh();
}

bcf::Topic* BcfView::topic(const QString& guid) {
    if (!bcf_) return nullptr;
    for (auto& entry : bcf_->topics) {
        if (entry.guid == guid) return &entry;
    }
    return nullptr;
}

const bcf::Viewpoint* BcfView::mainViewpoint(const bcf::Topic& topic) const {
    const bcf::Viewpoint* best = nullptr;
    for (const auto& viewpoint : topic.viewpoints) {
        if (!best || (viewpoint.index && (!best->index || *viewpoint.index < *best->index))) best = &viewpoint;
    }
    return best;
}

QString BcfView::ensureAuthor() {
    QString value = author();
    if (!value.isEmpty()) return value;
    bool ok = false;
    value = QInputDialog::getText(panel_, "BCF author",
                                  "Your e-mail address, written as the author of the topics and comments\n"
                                  "(it can be changed in Settings > BCF):",
                                  QLineEdit::Normal, QString(), &ok)
                .trimmed();
    if (!ok || value.isEmpty()) return {};
    setAuthor(value);
    return value;
}

bool BcfView::confirmDiscard() {
    return !dirty_ || services::popup::confirmation(panel_, "The BCF file has unsaved changes. Discard them?");
}

bool BcfView::confirmClose() {
    if (!bcf_ || !dirty_) return true;
    const QString name = path_.isEmpty() ? QStringLiteral("The new BCF file") : QFileInfo(path_).fileName();
    const auto answer = QMessageBox::question(panel_, "BCF", QString("%1 has unsaved changes. Save it?").arg(name),
                                              QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (answer == QMessageBox::Cancel) return false;
    if (answer == QMessageBox::Discard) return true;
    save();
    return !dirty_; // the save dialog cancelled: the viewer stays open
}

void BcfView::load() {
    if (!confirmDiscard()) return;
    QFileDialog dialog(panel_, "Load a BCF file");
    dialog.setNameFilters({"BCF files (*.bcf *.bcfzip)", "All files (*)"});
    dialog.setFileMode(QFileDialog::ExistingFile);
    components::acceptPastedPaths(dialog);
    if (dialog.exec() != QDialog::Accepted || dialog.selectedFiles().isEmpty()) return;
    const QString path = dialog.selectedFiles().front();
    QString error;
    auto loaded = bcf::load(path, &error);
    if (!loaded) {
        services::popup::information(panel_, QString("The BCF file cannot be read:\n%1").arg(error));
        return;
    }
    for (auto& page : pages_) {
        if (page) page->close();
    }
    pages_.clear();
    bcf_ = std::move(*loaded);
    path_ = path;
    deleted_.clear();
    dirty_ = false;
    refresh();
    emit topicsChanged();
}

void BcfView::save() {
    if (!bcf_) return;
    QFileDialog dialog(panel_, "Save the BCF file");
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    const QString v3 = "BCF 3.0 (*.bcf)";
    const QString v2 = "BCF 2.1 (*.bcf)";
    dialog.setNameFilters({v3, v2});
    dialog.selectNameFilter(bcf_->version.startsWith('2') ? v2 : v3);
    dialog.setDefaultSuffix("bcf");
    if (!path_.isEmpty()) dialog.selectFile(path_);
    components::acceptPastedPaths(dialog);
    if (dialog.exec() != QDialog::Accepted || dialog.selectedFiles().isEmpty()) return;
    const QString path = dialog.selectedFiles().front();
    const QString version = dialog.selectedNameFilter() == v2 ? QStringLiteral("2.1") : QStringLiteral("3.0");
    // The deleted topics leave the file now.
    bcf::BcfFile written = *bcf_;
    written.topics.erase(std::remove_if(written.topics.begin(), written.topics.end(),
                                        [this](const bcf::Topic& topic) { return deleted_.contains(topic.guid); }),
                         written.topics.end());
    QString error;
    if (!bcf::save(written, path, version, &error)) {
        services::popup::information(panel_, error);
        return;
    }
    bcf_ = std::move(written);
    bcf_->version = version;
    path_ = path;
    deleted_.clear();
    dirty_ = false;
    refresh();
    emit topicsChanged();
}

bcf::BcfFile& BcfView::ensureFile() {
    if (!bcf_) {
        bcf_ = bcf::BcfFile{};
        bcf_->project = bcf::Project{bcf::newGuid(), QString()};
    }
    return *bcf_;
}

QString BcfView::addTopic(bcf::Topic topic) {
    ensureFile();
    if (topic.guid.isEmpty()) topic.guid = bcf::newGuid();
    const QString guid = topic.guid;
    bcf_->topics.push_back(std::move(topic));
    topicChanged(guid);
    return guid;
}

void BcfView::newTopic() {
    const QString creator = ensureAuthor();
    if (creator.isEmpty()) return;
    bool ok = false;
    const QString title = QInputDialog::getText(panel_, "New topic", "Title:", QLineEdit::Normal, QString(), &ok).trimmed();
    if (!ok || title.isEmpty()) return;
    ensureFile();
    bcf::Topic topic;
    topic.guid = bcf::newGuid();
    topic.title = title;
    topic.topic_type = bcf_->extensions.topic_types.value(0, QStringLiteral("Issue"));
    topic.topic_status = bcf_->extensions.topic_statuses.value(0, QStringLiteral("Open"));
    topic.creation_date = bcf::now();
    topic.creation_author = creator;
    // The active view at the moment of the creation; its image follows.
    bcf::Viewpoint viewpoint;
    viewpoint.guid = bcf::newGuid();
    viewpoint.info = viewpoints_.capture();
    viewpoint.info->guid = viewpoint.guid;
    viewpoint.index = 0;
    topic.viewpoints.push_back(viewpoint);
    const QString viewpoint_guid = viewpoint.guid;
    const QString guid = addTopic(std::move(topic));
    viewpoints_.captureSnapshot([this, guid, viewpoint_guid](const QByteArray& png) {
        if (bcf::Topic* created = this->topic(guid)) {
            for (auto& entry : created->viewpoints) {
                if (entry.guid == viewpoint_guid) entry.snapshot = png;
            }
            topicChanged(guid);
        }
    });
    openTopic(guid);
}

void BcfView::topicChanged(const QString& guid) {
    dirty_ = true;
    refresh();
    if (auto page = pages_.value(guid)) page->reload();
    emit topicsChanged();
}

void BcfView::openTopic(const QString& guid) {
    if (!topic(guid)) return;
    QPointer<TopicDialog> page = pages_.value(guid);
    if (!page) {
        page = new TopicDialog(*this, guid, panel_);
        page->setAttribute(Qt::WA_DeleteOnClose);
        pages_.insert(guid, page);
    }
    page->show();
    page->raise();
    page->activateWindow();
}

bool BcfView::matches(const bcf::Topic& topic, const QString& search) const {
    const QString text = search.trimmed();
    if (text.isEmpty()) return true;
    if (text.startsWith(kFullSearch)) {
        const QString needle = text.mid(kFullSearch.size()).trimmed();
        return needle.isEmpty() || fullText(topic).contains(needle, Qt::CaseInsensitive);
    }
    return topic.title.contains(text, Qt::CaseInsensitive);
}

void BcfView::refresh() {
    QTreeWidget* tree = panel_->tree();
    QListWidget* thumbnails = panel_->thumbnails();
    const QStringList selected = selectedGuids();
    tree->setSortingEnabled(false);
    tree->clear();
    thumbnails->clear();
    panel_->setFileActionsEnabled(bcf_.has_value());
    if (!bcf_) {
        updateSummary();
        return;
    }
    const QString search = panel_->searchText();
    const bool tree_mode = panel_->mode() == BcfPanel::Mode::Tree;
    const auto fill = [this](QTreeWidgetItem* item, const bcf::Topic& topic) {
        item->setText(0, topic.title);
        item->setText(1, topic.topic_status);
        item->setText(2, topic.topic_type);
        item->setText(3, topic.priority);
        item->setText(4, topic.assigned_to);
        item->setText(5, shortDate(topic.modified_date.isEmpty() ? topic.creation_date : topic.modified_date));
        item->setText(6, QString::number(topic.comments.size()));
        item->setData(0, kGuidRole, topic.guid);
        item->setToolTip(0, topic.description);
        if (deleted_.contains(topic.guid)) {
            QFont font = item->font(0);
            font.setStrikeOut(true);
            for (int column = 0; column < 7; ++column) item->setFont(column, font);
            item->setText(0, topic.title + "  (deleted)");
        }
    };
    for (const auto& topic : bcf_->topics) {
        if (!matches(topic, search)) continue;
        auto* item = new QTreeWidgetItem(tree);
        fill(item, topic);
        if (tree_mode) {
            for (const QString& related : topic.related_topics) {
                if (const bcf::Topic* other = this->topic(related)) {
                    auto* child = new QTreeWidgetItem(item);
                    fill(child, *other);
                }
            }
        }
        if (selected.contains(topic.guid)) item->setSelected(true);

        auto* thumbnail = new QListWidgetItem(topic.title, thumbnails);
        thumbnail->setData(kGuidRole, topic.guid);
        thumbnail->setToolTip(topic.description);
        if (const bcf::Viewpoint* viewpoint = mainViewpoint(topic); viewpoint && !viewpoint->snapshot.isEmpty()) {
            QPixmap image;
            image.loadFromData(viewpoint->snapshot);
            thumbnail->setIcon(QIcon(image.scaled(160, 120, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
        }
        if (deleted_.contains(topic.guid)) thumbnail->setText(topic.title + "\n(deleted)");
    }
    if (tree_mode) tree->expandAll();
    tree->setSortingEnabled(true);
    updateSummary();
}

void BcfView::updateSummary() {
    if (!bcf_) {
        panel_->setSummary("No BCF file. Load one, or create a topic.");
        return;
    }
    const QString name = path_.isEmpty() ? QStringLiteral("New BCF") : QFileInfo(path_).fileName();
    QString text = QString("%1 (BCF %2) — %3 topic(s)").arg(name, bcf_->version).arg(bcf_->topics.size());
    if (!deleted_.isEmpty()) text += QString(", %1 deleted at the next Save").arg(deleted_.size());
    if (dirty_) text += " — not saved";
    panel_->setSummary(text);
}

QStringList BcfView::selectedGuids() const {
    QStringList guids;
    if (panel_->mode() == BcfPanel::Mode::Thumbnails) {
        for (const auto* item : panel_->thumbnails()->selectedItems()) guids << item->data(kGuidRole).toString();
    } else {
        for (const auto* item : panel_->tree()->selectedItems()) guids << item->data(0, kGuidRole).toString();
    }
    guids.removeDuplicates();
    return guids;
}

void BcfView::showMenu(const QPoint& global_pos) {
    const QStringList guids = selectedGuids();
    if (guids.isEmpty()) return;
    QMenu menu(panel_);
    menu.addAction("Zoom on the Objects", this, [this, guids]() { zoom(guids); });
    menu.addAction("Isolate the Objects", this, [this, guids]() { isolate(guids); });
    menu.addSeparator();
    auto* modify = menu.addAction("Modify the Topic…", this, [this, guids]() { openTopic(guids.front()); });
    modify->setEnabled(guids.size() == 1);
    const bool all_deleted = std::all_of(guids.begin(), guids.end(), [this](const QString& g) { return deleted_.contains(g); });
    if (all_deleted) {
        menu.addAction("Restore the Topic", this, [this, guids]() { setDeleted(guids, false); });
    } else {
        menu.addAction("Delete the Topic", this, [this, guids]() { setDeleted(guids, true); });
    }
    menu.exec(global_pos);
}

void BcfView::zoom(const QStringList& guids) {
    std::vector<std::uint32_t> ids;
    for (const QString& guid : guids) {
        const bcf::Topic* entry = topic(guid);
        const bcf::Viewpoint* viewpoint = entry ? mainViewpoint(*entry) : nullptr;
        if (!viewpoint || !viewpoint->info) continue;
        const auto relevant = viewpoints_.relevantIds(*viewpoint->info);
        ids.insert(ids.end(), relevant.begin(), relevant.end());
    }
    viewpoints_.zoomTo(ids);
}

void BcfView::isolate(const QStringList& guids) {
    std::vector<std::uint32_t> ids;
    for (const QString& guid : guids) {
        const bcf::Topic* entry = topic(guid);
        const bcf::Viewpoint* viewpoint = entry ? mainViewpoint(*entry) : nullptr;
        if (!viewpoint || !viewpoint->info) continue;
        const auto relevant = viewpoints_.relevantIds(*viewpoint->info);
        ids.insert(ids.end(), relevant.begin(), relevant.end());
    }
    viewpoints_.isolate(ids);
}

void BcfView::setDeleted(const QStringList& guids, bool deleted) {
    for (const QString& guid : guids) {
        if (deleted) deleted_.insert(guid);
        else deleted_.remove(guid);
    }
    dirty_ = true;
    refresh();
    emit topicsChanged();
}

} // namespace bonsaiviewer::modules::bcf
