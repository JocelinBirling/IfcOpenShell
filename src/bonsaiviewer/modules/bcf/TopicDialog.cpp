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

#include "TopicDialog.h"

#include "ElementListDialog.h"
#include "ImageEditor.h"
#include "View.h"

#include "../../services/BcfViewpoints.h"
#include "../../services/PopupService.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QTableWidget>
#include <QUrl>
#include <QUuid>
#include <QVBoxLayout>

#include <algorithm>

namespace bonsaiviewer::modules::bcf {

namespace bcf = bonsaiviewer::ports::bcf;

namespace {

constexpr int kGuidRole = Qt::UserRole;

QComboBox* editableCombo(const QStringList& values, QWidget* parent) {
    auto* combo = new QComboBox(parent);
    combo->setEditable(true);
    combo->addItem(QString());
    combo->addItems(values);
    return combo;
}

void setComboText(QComboBox* combo, const QString& text) {
    const int index = combo->findText(text);
    if (index >= 0) combo->setCurrentIndex(index);
    else combo->setEditText(text);
}

QString localDate(const QString& xs_date) {
    const QDateTime date = QDateTime::fromString(xs_date, Qt::ISODateWithMs);
    return date.isValid() ? date.toLocalTime().toString("yyyy-MM-dd HH:mm") : xs_date;
}

QPixmap pixmapOf(const QByteArray& png, const QSize& size) {
    QPixmap image;
    if (png.isEmpty() || !image.loadFromData(png)) return {};
    return image.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

// Writes `data` to a temporary file named `filename` and opens it.
void openData(const QByteArray& data, const QString& filename) {
    const QString folder = QDir::temp().filePath("bonsaiviewer-bcf-" + QUuid::createUuid().toString(QUuid::WithoutBraces));
    QDir().mkpath(folder);
    const QString path = QDir(folder).filePath(QFileInfo(filename).fileName().isEmpty() ? "document" : QFileInfo(filename).fileName());
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) file.write(data);
    file.close();
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

} // namespace

TopicDialog::TopicDialog(BcfView& view, const QString& topic_guid, QWidget* parent)
    : QDialog(parent), view_(view), guid_(topic_guid) {
    setWindowFlag(Qt::Window);
    setModal(false);
    resize(760, 860);
    const bcf::Extensions extensions = view_.file() ? view_.file()->extensions : bcf::Extensions{};

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    auto* page = new QWidget(scroll);
    auto* layout = new QVBoxLayout(page);

    // Image of the topic (its main viewpoint).
    auto* image_row = new QHBoxLayout();
    image_ = new QLabel(page);
    image_->setFixedSize(320, 240);
    image_->setAlignment(Qt::AlignCenter);
    image_->setFrameShape(QFrame::StyledPanel);
    auto* image_buttons = new QVBoxLayout();
    auto* go_to_view = new QPushButton("Go to the View", page);
    go_to_view->setToolTip("Camera, selection, colours and visibility of the topic's viewpoint");
    auto* edit_image = new QPushButton("Edit the Image…", page);
    edit_image->setToolTip("Open the image in the system's image editor; the saved image comes back into the BCF");
    image_buttons->addWidget(go_to_view);
    image_buttons->addWidget(edit_image);
    for (const auto& [label, kind] : {std::pair<const char*, int>{"Selection…", 0}, {"Visibility…", 1}, {"Colours…", 2}}) {
        auto* button = new QPushButton(label, page);
        button->setToolTip("The elements of the viewpoint (the selected comment's, else the topic's)");
        connect(button, &QPushButton::clicked, this, [this, kind = kind]() { editElements(kind); });
        image_buttons->addWidget(button);
    }
    image_buttons->addStretch(1);
    image_row->addWidget(image_);
    image_row->addLayout(image_buttons, 1);
    layout->addLayout(image_row);
    info_ = new QLabel(page);
    info_->setProperty("textRole", "secondary");
    info_->setWordWrap(true);
    layout->addWidget(info_);

    // The fields.
    auto* form = new QFormLayout();
    title_ = new QLineEdit(page);
    type_ = editableCombo(extensions.topic_types, page);
    status_ = editableCombo(extensions.topic_statuses, page);
    priority_ = editableCombo(extensions.priorities, page);
    labels_ = new QLineEdit(page);
    labels_->setPlaceholderText(extensions.topic_labels.isEmpty() ? "Comma separated"
                                                                  : "Comma separated: " + extensions.topic_labels.join(", "));
    assigned_ = editableCombo(extensions.users, page);
    stage_ = editableCombo(extensions.stages, page);
    auto* due_row = new QHBoxLayout();
    has_due_ = new QCheckBox(page);
    due_ = new QDateTimeEdit(page);
    due_->setCalendarPopup(true);
    due_row->addWidget(has_due_);
    due_row->addWidget(due_, 1);
    connect(has_due_, &QCheckBox::toggled, due_, &QWidget::setEnabled);
    description_ = new QPlainTextEdit(page);
    description_->setFixedHeight(90);
    links_ = new QPlainTextEdit(page);
    links_->setFixedHeight(50);
    links_->setPlaceholderText("One link per line");
    form->addRow("Title", title_);
    form->addRow("Type", type_);
    form->addRow("Status", status_);
    form->addRow("Priority", priority_);
    form->addRow("Labels", labels_);
    form->addRow("Assigned To", assigned_);
    form->addRow("Stage", stage_);
    form->addRow("Due Date", due_row);
    form->addRow("Description", description_);
    form->addRow("Reference Links", links_);
    layout->addLayout(form);

    // Document references.
    auto* documents_box = new QGroupBox("Document References", page);
    auto* documents_layout = new QVBoxLayout(documents_box);
    documents_ = new QTableWidget(0, 2, documents_box);
    documents_->setHorizontalHeaderLabels({"Description", "Link"});
    documents_->horizontalHeader()->setStretchLastSection(true);
    documents_->setSelectionBehavior(QAbstractItemView::SelectRows);
    documents_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    documents_->setFixedHeight(110);
    documents_->setToolTip("Double click: open the document (web link, file next to the BCF, or stored in the BCF)");
    auto* documents_buttons = new QHBoxLayout();
    auto* open_document = new QPushButton("Open", documents_box);
    auto* add_link = new QPushButton("Add a Link…", documents_box);
    auto* add_file = new QPushButton("Add a File…", documents_box);
    add_file->setToolTip("Store a file in the BCF (BCF 3.0 documents)");
    auto* remove_document = new QPushButton("Remove", documents_box);
    for (auto* button : {open_document, add_link, add_file, remove_document}) documents_buttons->addWidget(button);
    documents_buttons->addStretch(1);
    documents_layout->addWidget(documents_);
    documents_layout->addLayout(documents_buttons);
    layout->addWidget(documents_box);

    // Related topics.
    auto* related_box = new QGroupBox("Related Topics", page);
    auto* related_layout = new QHBoxLayout(related_box);
    auto* to_column = new QVBoxLayout();
    to_column->addWidget(new QLabel("This topic relates to", related_box));
    related_to_ = new QListWidget(related_box);
    related_to_->setFixedHeight(90);
    to_column->addWidget(related_to_);
    auto* related_buttons = new QHBoxLayout();
    auto* add_related = new QPushButton("Add…", related_box);
    auto* remove_related = new QPushButton("Remove", related_box);
    related_buttons->addWidget(add_related);
    related_buttons->addWidget(remove_related);
    related_buttons->addStretch(1);
    to_column->addLayout(related_buttons);
    auto* by_column = new QVBoxLayout();
    by_column->addWidget(new QLabel("Topics relating to this one", related_box));
    related_by_ = new QListWidget(related_box);
    related_by_->setFixedHeight(90);
    by_column->addWidget(related_by_);
    by_column->addStretch(1);
    related_layout->addLayout(to_column, 1);
    related_layout->addLayout(by_column, 1);
    layout->addWidget(related_box);

    // Comments.
    auto* comments_box = new QGroupBox("Comments", page);
    auto* comments_layout = new QVBoxLayout(comments_box);
    comments_ = new QListWidget(comments_box);
    comments_->setIconSize(QSize(120, 90));
    comments_->setWordWrap(true);
    comments_->setMinimumHeight(220);
    comments_->setContextMenuPolicy(Qt::CustomContextMenu);
    comments_->setToolTip("Double click: go to the comment's view. Right click: its functions.");
    new_comment_ = new QPlainTextEdit(comments_box);
    new_comment_->setPlaceholderText("New comment");
    new_comment_->setFixedHeight(60);
    auto* comment_buttons = new QHBoxLayout();
    auto* add_comment = new QPushButton("Add the Comment", comments_box);
    add_comment->setToolTip("The first comment of a topic captures the active view and its image; the next ones only "
                            "their text");
    auto* add_with_view = new QPushButton("Add with the View", comments_box);
    add_with_view->setToolTip("Add the comment with the active view and its image");
    comment_buttons->addWidget(add_comment);
    comment_buttons->addWidget(add_with_view);
    comment_buttons->addStretch(1);
    comments_layout->addWidget(comments_);
    comments_layout->addWidget(new_comment_);
    comments_layout->addLayout(comment_buttons);
    layout->addWidget(comments_box);

    scroll->setWidget(page);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Close, this);
    auto* outer = new QVBoxLayout(this);
    outer->addWidget(scroll, 1);
    outer->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        apply();
        close();
    });
    connect(buttons->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &TopicDialog::apply);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::close);
    connect(go_to_view, &QPushButton::clicked, this, [this]() {
        if (bcf::Topic* entry = topic()) {
            if (const bcf::Viewpoint* main = view_.mainViewpoint(*entry); main && main->info) {
                view_.viewpoints().apply(*main->info);
            }
        }
    });
    connect(edit_image, &QPushButton::clicked, this, [this]() {
        if (bcf::Topic* entry = topic()) {
            if (const bcf::Viewpoint* main = view_.mainViewpoint(*entry)) editImage(main->guid);
        }
    });
    connect(documents_, &QTableWidget::cellDoubleClicked, this, [this](int row, int) { openDocument(row); });
    connect(open_document, &QPushButton::clicked, this, [this]() { openDocument(documents_->currentRow()); });
    connect(add_link, &QPushButton::clicked, this, &TopicDialog::addLink);
    connect(add_file, &QPushButton::clicked, this, &TopicDialog::addFile);
    connect(remove_document, &QPushButton::clicked, this, &TopicDialog::removeDocument);
    connect(add_related, &QPushButton::clicked, this, &TopicDialog::addRelated);
    connect(remove_related, &QPushButton::clicked, this, &TopicDialog::removeRelated);
    for (QListWidget* list : {related_to_, related_by_}) {
        connect(list, &QListWidget::itemDoubleClicked, this,
                [this](QListWidgetItem* item) { view_.openTopic(item->data(kGuidRole).toString()); });
    }
    connect(comments_, &QWidget::customContextMenuRequested, this, &TopicDialog::showCommentMenu);
    connect(comments_, &QListWidget::itemDoubleClicked, this, &TopicDialog::applyComment);
    connect(add_comment, &QPushButton::clicked, this, [this]() {
        const bcf::Topic* entry = topic();
        addComment(entry && entry->comments.empty());
    });
    connect(add_with_view, &QPushButton::clicked, this, [this]() { addComment(true); });
    const auto edited = [this]() {
        if (!filling_) fields_edited_ = true;
    };
    connect(title_, &QLineEdit::textEdited, this, edited);
    connect(labels_, &QLineEdit::textEdited, this, edited);
    for (QComboBox* combo : {type_, status_, priority_, assigned_, stage_}) {
        connect(combo, &QComboBox::currentTextChanged, this, edited);
    }
    connect(has_due_, &QCheckBox::toggled, this, edited);
    connect(due_, &QDateTimeEdit::dateTimeChanged, this, edited);
    connect(description_, &QPlainTextEdit::textChanged, this, edited);
    connect(links_, &QPlainTextEdit::textChanged, this, edited);
    reload();
}

bcf::Topic* TopicDialog::topic() {
    return view_.topic(guid_);
}

bcf::Viewpoint* TopicDialog::viewpoint(const QString& guid) {
    bcf::Topic* entry = topic();
    if (!entry || guid.isEmpty()) return nullptr;
    for (auto& candidate : entry->viewpoints) {
        if (candidate.guid == guid) return &candidate;
    }
    return nullptr;
}

bcf::Comment* TopicDialog::comment(const QString& guid) {
    bcf::Topic* entry = topic();
    if (!entry) return nullptr;
    for (auto& candidate : entry->comments) {
        if (candidate.guid == guid) return &candidate;
    }
    return nullptr;
}

bcf::Viewpoint* TopicDialog::currentViewpoint() {
    if (const QListWidgetItem* item = comments_->currentItem()) {
        if (const bcf::Comment* selected = comment(item->data(kGuidRole).toString())) {
            if (bcf::Viewpoint* found = viewpoint(selected->viewpoint_guid)) return found;
        }
    }
    bcf::Topic* entry = topic();
    if (!entry) return nullptr;
    return const_cast<bcf::Viewpoint*>(view_.mainViewpoint(*entry));
}

void TopicDialog::reload() {
    const bcf::Topic* entry = topic();
    if (!entry) return;
    filling_ = true;
    setWindowTitle(QString("Topic — %1").arg(entry->title));
    const bcf::Viewpoint* main = view_.mainViewpoint(*entry);
    const QPixmap image = main ? pixmapOf(main->snapshot, image_->size()) : QPixmap();
    if (image.isNull()) image_->setText("No image");
    else image_->setPixmap(image);
    QString info = QString("Created %1 by %2").arg(localDate(entry->creation_date), entry->creation_author);
    if (!entry->modified_date.isEmpty()) {
        info += QString(" — modified %1 by %2").arg(localDate(entry->modified_date), entry->modified_author);
    }
    if (view_.isDeleted(guid_)) info += " — deleted at the next Save";
    info_->setText(info);
    if (!fields_edited_) {
        title_->setText(entry->title);
        setComboText(type_, entry->topic_type);
        setComboText(status_, entry->topic_status);
        setComboText(priority_, entry->priority);
        labels_->setText(entry->labels.join(", "));
        setComboText(assigned_, entry->assigned_to);
        setComboText(stage_, entry->stage);
        const QDateTime due = QDateTime::fromString(entry->due_date, Qt::ISODateWithMs);
        has_due_->setChecked(due.isValid());
        due_->setEnabled(due.isValid());
        due_->setDateTime(due.isValid() ? due.toLocalTime() : QDateTime::currentDateTime());
        description_->setPlainText(entry->description);
        links_->setPlainText(entry->reference_links.join('\n'));
    }

    documents_->setRowCount(0);
    for (const auto& reference : entry->document_references) {
        const int row = documents_->rowCount();
        documents_->insertRow(row);
        QString link = reference.url;
        if (!reference.document_guid.isEmpty() && view_.file()) {
            for (const auto& document : view_.file()->documents) {
                if (document.guid == reference.document_guid) link = document.filename + "  (in the BCF)";
            }
        }
        documents_->setItem(row, 0, new QTableWidgetItem(reference.description));
        documents_->setItem(row, 1, new QTableWidgetItem(link));
    }

    related_to_->clear();
    related_by_->clear();
    for (const QString& related : entry->related_topics) {
        const bcf::Topic* other = view_.topic(related);
        auto* item = new QListWidgetItem(other ? other->title : related + "  (not in this file)", related_to_);
        item->setData(kGuidRole, related);
    }
    if (view_.file()) {
        for (const auto& other : view_.file()->topics) {
            if (other.related_topics.contains(guid_)) {
                auto* item = new QListWidgetItem(other.title, related_by_);
                item->setData(kGuidRole, other.guid);
            }
        }
    }

    const QString current = comments_->currentItem() ? comments_->currentItem()->data(kGuidRole).toString() : QString();
    comments_->clear();
    for (const auto& entry_comment : entry->comments) {
        QString text = QString("%1 — %2").arg(entry_comment.author, localDate(entry_comment.date));
        if (!entry_comment.modified_date.isEmpty()) text += QString(" (modified %1)").arg(localDate(entry_comment.modified_date));
        text += '\n' + entry_comment.text;
        auto* item = new QListWidgetItem(text, comments_);
        item->setData(kGuidRole, entry_comment.guid);
        if (const bcf::Viewpoint* own = viewpoint(entry_comment.viewpoint_guid)) {
            const QPixmap thumbnail = pixmapOf(own->snapshot, comments_->iconSize());
            if (!thumbnail.isNull()) item->setIcon(QIcon(thumbnail));
        }
        if (entry_comment.guid == current) comments_->setCurrentItem(item);
    }
    filling_ = false;
}

void TopicDialog::apply() {
    bcf::Topic* entry = topic();
    if (!entry) return;
    entry->title = title_->text().trimmed();
    entry->topic_type = type_->currentText().trimmed();
    entry->topic_status = status_->currentText().trimmed();
    entry->priority = priority_->currentText().trimmed();
    QStringList labels;
    for (const QString& label : labels_->text().split(',')) {
        if (!label.trimmed().isEmpty()) labels << label.trimmed();
    }
    entry->labels = labels;
    entry->assigned_to = assigned_->currentText().trimmed();
    entry->stage = stage_->currentText().trimmed();
    entry->due_date = has_due_->isChecked() ? due_->dateTime().toUTC().toString(Qt::ISODate) : QString();
    entry->description = description_->toPlainText();
    QStringList links;
    for (const QString& link : links_->toPlainText().split('\n')) {
        if (!link.trimmed().isEmpty()) links << link.trimmed();
    }
    entry->reference_links = links;
    fields_edited_ = false;
    changed();
}

// The topic was modified: modification date and author, then the list.
void TopicDialog::changed() {
    if (bcf::Topic* entry = topic()) {
        entry->modified_date = bcf::now();
        const QString author = view_.ensureAuthor();
        if (!author.isEmpty()) entry->modified_author = author;
    }
    view_.topicChanged(guid_);
}

QString TopicDialog::newViewpoint() {
    bcf::Topic* entry = topic();
    if (!entry) return {};
    bcf::Viewpoint created;
    created.guid = bcf::newGuid();
    int index = 0;
    for (const auto& existing : entry->viewpoints) index = std::max(index, existing.index.value_or(0) + 1);
    created.index = index;
    entry->viewpoints.push_back(created);
    return created.guid;
}

// The active view into a viewpoint: its data (camera, components) and / or its image.
void TopicDialog::captureInto(const QString& viewpoint_guid, bool image, bool data) {
    if (bcf::Viewpoint* target = viewpoint(viewpoint_guid); target && data) {
        target->info = view_.viewpoints().capture();
        target->info->guid = viewpoint_guid;
    }
    if (image) {
        BcfView* bcf_view = &view_;
        const QString topic_guid = guid_;
        view_.viewpoints().captureSnapshot([bcf_view, topic_guid, viewpoint_guid](const QByteArray& png) {
            bcf::Topic* entry = bcf_view->topic(topic_guid);
            if (!entry || png.isEmpty()) return;
            for (auto& candidate : entry->viewpoints) {
                if (candidate.guid == viewpoint_guid) candidate.snapshot = png;
            }
            bcf_view->topicChanged(topic_guid);
        });
    }
}

void TopicDialog::addComment(bool with_view) {
    const QString text = new_comment_->toPlainText().trimmed();
    if (text.isEmpty() && !with_view) return;
    const QString author = view_.ensureAuthor();
    if (author.isEmpty()) return;
    bcf::Topic* entry = topic();
    if (!entry) return;
    bcf::Comment created;
    created.guid = bcf::newGuid();
    created.date = bcf::now();
    created.author = author;
    created.text = text;
    if (with_view) {
        created.viewpoint_guid = newViewpoint();
        entry = topic(); // newViewpoint() may have moved the topics
    }
    entry->comments.push_back(created);
    new_comment_->clear();
    if (with_view) captureInto(created.viewpoint_guid, true, true);
    changed();
}

void TopicDialog::showCommentMenu(const QPoint& pos) {
    QListWidgetItem* item = comments_->itemAt(pos);
    if (!item) return;
    comments_->setCurrentItem(item);
    const QString comment_guid = item->data(kGuidRole).toString();
    const bcf::Comment* selected = comment(comment_guid);
    if (!selected) return;
    const QString viewpoint_guid = selected->viewpoint_guid;
    const bcf::Viewpoint* own = viewpoint(viewpoint_guid);
    QMenu menu(this);
    auto* isolate = menu.addAction("Isolate the Objects", this, [this, viewpoint_guid]() {
        if (const bcf::Viewpoint* found = viewpoint(viewpoint_guid); found && found->info) {
            view_.viewpoints().isolate(view_.viewpoints().relevantIds(*found->info));
        }
    });
    auto* zoom = menu.addAction("Zoom on the Objects", this, [this, viewpoint_guid]() {
        if (const bcf::Viewpoint* found = viewpoint(viewpoint_guid); found && found->info) {
            view_.viewpoints().zoomTo(view_.viewpoints().relevantIds(*found->info));
        }
    });
    isolate->setEnabled(own && own->info);
    zoom->setEnabled(own && own->info);
    menu.addSeparator();
    menu.addAction("Retake the Image", this, [this, comment_guid]() {
        // Captures the active view: image and viewpoint of the comment.
        bcf::Comment* target = comment(comment_guid);
        if (!target) return;
        QString vp = target->viewpoint_guid;
        if (!viewpoint(vp)) {
            vp = newViewpoint();
            if (bcf::Comment* again = comment(comment_guid)) again->viewpoint_guid = vp;
        }
        captureInto(vp, true, true);
        if (bcf::Comment* again = comment(comment_guid)) again->modified_date = bcf::now();
        changed();
    });
    auto* refresh = menu.addAction("Refresh the Data", this, [this, viewpoint_guid]() {
        // The viewpoint's data from the active view; the image is kept.
        captureInto(viewpoint_guid, false, true);
        changed();
    });
    refresh->setEnabled(own != nullptr);
    refresh->setToolTip("Camera, selection, visibility and colours from the active view; the image is kept");
    auto* edit = menu.addAction("Edit the Image…", this, [this, viewpoint_guid]() { editImage(viewpoint_guid); });
    edit->setEnabled(own && !own->snapshot.isEmpty());
    menu.addSeparator();
    menu.addAction("Edit the Text…", this, [this, comment_guid]() {
        bcf::Comment* target = comment(comment_guid);
        if (!target) return;
        bool ok = false;
        const QString text = QInputDialog::getMultiLineText(this, "Comment", "Text:", target->text, &ok);
        if (!ok) return;
        target->text = text;
        target->modified_date = bcf::now();
        target->modified_author = view_.ensureAuthor();
        changed();
    });
    menu.addAction("Delete the Comment", this, [this, comment_guid]() {
        bcf::Topic* entry = topic();
        if (!entry || !services::popup::confirmation(this, "Delete this comment?")) return;
        entry->comments.erase(std::remove_if(entry->comments.begin(), entry->comments.end(),
                                             [&comment_guid](const bcf::Comment& c) { return c.guid == comment_guid; }),
                              entry->comments.end());
        changed();
    });
    menu.exec(comments_->viewport()->mapToGlobal(pos));
}

void TopicDialog::applyComment(QListWidgetItem* item) {
    const bcf::Comment* selected = comment(item->data(kGuidRole).toString());
    if (!selected) return;
    if (const bcf::Viewpoint* own = viewpoint(selected->viewpoint_guid); own && own->info) {
        view_.viewpoints().apply(*own->info);
    }
}

void TopicDialog::editImage(const QString& viewpoint_guid) {
    const bcf::Viewpoint* target = viewpoint(viewpoint_guid);
    if (!target || target->snapshot.isEmpty()) {
        services::popup::information(this, "This viewpoint has no image.");
        return;
    }
    BcfView* bcf_view = &view_;
    const QString topic_guid = guid_;
    auto* editor = new ImageEditor(target->snapshot, [bcf_view, topic_guid, viewpoint_guid](const QByteArray& png) {
        bcf::Topic* entry = bcf_view->topic(topic_guid);
        if (!entry) return;
        for (auto& candidate : entry->viewpoints) {
            if (candidate.guid == viewpoint_guid) candidate.snapshot = png;
        }
        bcf_view->topicChanged(topic_guid);
    }, this);
    if (!editor->open()) {
        services::popup::information(this, "No image editor could be started.");
        editor->deleteLater();
    }
}

void TopicDialog::openDocument(int row) {
    bcf::Topic* entry = topic();
    if (!entry || row < 0 || row >= static_cast<int>(entry->document_references.size())) return;
    const bcf::DocumentReference& reference = entry->document_references[static_cast<size_t>(row)];
    // Stored in the BCF (3.0 documents).
    if (!reference.document_guid.isEmpty() && view_.file()) {
        for (const auto& document : view_.file()->documents) {
            if (document.guid == reference.document_guid) {
                openData(document.data, document.filename);
                return;
            }
        }
        services::popup::information(this, "The document is not in the BCF file.");
        return;
    }
    const QString link = reference.url.trimmed();
    if (link.isEmpty()) return;
    // A web link: the default browser.
    if (link.startsWith("http", Qt::CaseInsensitive)) {
        QDesktopServices::openUrl(QUrl(link));
        return;
    }
    // A file of the archive (2.1: relative to the topic folder).
    if (view_.file()) {
        const QString inside = QDir::cleanPath(guid_ + '/' + link);
        const auto found = view_.file()->other_files.find(inside);
        if (found != view_.file()->other_files.end()) {
            openData(found->second, inside);
            return;
        }
    }
    // A file on disk: relative to the BCF file, or absolute.
    QString path = link;
    if (QFileInfo(link).isRelative() && !view_.filePath().isEmpty()) {
        path = QFileInfo(view_.filePath()).dir().absoluteFilePath(link);
    }
    if (!QFileInfo::exists(path)) {
        services::popup::information(this, QString("The document cannot be found:\n%1").arg(path));
        return;
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void TopicDialog::addLink() {
    bool ok = false;
    const QString link = QInputDialog::getText(this, "Document reference",
                                               "Web link, or path of a file (relative to the BCF file, or absolute):",
                                               QLineEdit::Normal, QString(), &ok)
                             .trimmed();
    if (!ok || link.isEmpty()) return;
    const QString description = QInputDialog::getText(this, "Document reference", "Description:").trimmed();
    bcf::Topic* entry = topic();
    if (!entry) return;
    bcf::DocumentReference reference;
    reference.guid = bcf::newGuid();
    reference.url = link;
    reference.is_external = true;
    reference.description = description;
    entry->document_references.push_back(reference);
    changed();
}

void TopicDialog::addFile() {
    const QString path = QFileDialog::getOpenFileName(this, "Store a document in the BCF");
    if (path.isEmpty() || !view_.file()) return;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        services::popup::information(this, QString("Cannot read %1").arg(path));
        return;
    }
    bcf::Document document;
    document.guid = bcf::newGuid();
    document.filename = QFileInfo(path).fileName();
    document.data = file.readAll();
    view_.file()->documents.push_back(document);
    bcf::Topic* entry = topic();
    if (!entry) return;
    bcf::DocumentReference reference;
    reference.guid = bcf::newGuid();
    reference.document_guid = document.guid;
    reference.description = document.filename;
    entry->document_references.push_back(reference);
    changed();
}

void TopicDialog::removeDocument() {
    bcf::Topic* entry = topic();
    const int row = documents_->currentRow();
    if (!entry || row < 0 || row >= static_cast<int>(entry->document_references.size())) return;
    entry->document_references.erase(entry->document_references.begin() + row);
    changed();
}

void TopicDialog::addRelated() {
    if (!view_.file()) return;
    QStringList titles;
    QStringList guids;
    for (const auto& other : view_.file()->topics) {
        if (other.guid == guid_) continue;
        titles << QString("%1  [%2]").arg(other.title, other.guid.left(8));
        guids << other.guid;
    }
    if (titles.isEmpty()) {
        services::popup::information(this, "There is no other topic in this file.");
        return;
    }
    bool ok = false;
    const QString chosen = QInputDialog::getItem(this, "Related topic", "This topic relates to:", titles, 0, false, &ok);
    if (!ok) return;
    const QString related = guids.value(titles.indexOf(chosen));
    bcf::Topic* entry = topic();
    if (!entry || related.isEmpty() || entry->related_topics.contains(related)) return;
    entry->related_topics << related;
    changed();
}

void TopicDialog::removeRelated() {
    bcf::Topic* entry = topic();
    const QListWidgetItem* item = related_to_->currentItem();
    if (!entry || !item) return;
    entry->related_topics.removeAll(item->data(kGuidRole).toString());
    changed();
}

void TopicDialog::editElements(int kind) {
    bcf::Viewpoint* target = currentViewpoint();
    if (!target || !target->info) {
        services::popup::information(this, "This topic has no viewpoint.");
        return;
    }
    const QString viewpoint_guid = target->guid;
    ElementListDialog dialog(static_cast<ElementListDialog::Kind>(kind), *target->info, view_.viewpoints(), this);
    if (dialog.exec() != QDialog::Accepted) return;
    if (bcf::Viewpoint* again = viewpoint(viewpoint_guid)) {
        again->info = dialog.info();
        changed();
    }
}

} // namespace bonsaiviewer::modules::bcf
