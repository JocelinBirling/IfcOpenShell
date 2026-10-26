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
 ********************************************************************************/

#include "View.h"

#include "../models/Commands.h"
#include "../../SessionState.h"
#include "../../../ifcviewer/Federation.h"
#include "../../../ifcviewer/SceneLoader.h"
#include "../../../ifcviewer/ViewportWindow.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QMessageBox>
#include <QPushButton>

namespace bonsaiviewer::modules::easy_load {

namespace {

bool isIfc(const QString& path) {
    return QFileInfo(path).suffix().compare(QStringLiteral("ifc"), Qt::CaseInsensitive) == 0;
}

// Canonical path, so that the same file reached by two spellings compares equal.
QString canonical(const QString& path) {
    const QFileInfo info(path);
    const QString resolved = info.canonicalFilePath();
    return resolved.isEmpty() ? info.absoluteFilePath() : resolved;
}

// Folder of `file` relative to `root`, with "/" separators; "." when it is `root` itself.
QString relativeFolder(const QString& root, const QString& file) {
    return QDir(root).relativeFilePath(QFileInfo(file).absolutePath());
}

// A dropped folder and the IFC files found inside it.
struct FolderDrop {
    QString root;
    QStringList files;
};

} // namespace

EasyLoadView::EasyLoadView(bonsaiviewer::SessionState& session, ViewportWindow& viewport,
                           QWidget* host, QObject* parent)
    : QObject(parent), session_(session), viewport_(viewport), host_(host)
{
}

std::optional<QString> EasyLoadView::loadedModelIdFor(const QString& path) const {
    auto* loader = session_.loader();
    if (loader == nullptr) return std::nullopt;
    const QString wanted = canonical(path);
    for (std::uint32_t session_model_id : session_.sessionModelIds()) {
        const QString loaded = loader->filePath(session_model_id);
        if (loaded.isEmpty() || canonical(loaded) != wanted) continue;
        const QString model_id = session_.modelIdForSessionModelId(session_model_id);
        if (!model_id.isEmpty()) return model_id;
    }
    return std::nullopt;
}

std::optional<QString> EasyLoadView::newestModelIdFor(const QString& path) const {
    const auto& models = session_.federation()->models();
    for (auto it = models.rbegin(); it != models.rend(); ++it) {
        if (it->source_path == path) return it->id;
    }
    return std::nullopt;
}

QHash<QString, QString> EasyLoadView::createFolderTree(const QString& root, const QStringList& files) {
    auto* federation = session_.federation();
    QHash<QString, QString> group_of;  // relative folder -> group id
    for (const QString& file : files) {
        const QString folder = relativeFolder(root, file);
        if (folder == QLatin1String(".")) continue;

        QString key;
        QString parent;  // empty = root of the Models panel
        for (const QString& part : folder.split(QLatin1Char('/'))) {
            key = key.isEmpty() ? part : key + QLatin1Char('/') + part;
            if (!group_of.contains(key)) {
                group_of.insert(key, federation->addGroup(part, parent));
            }
            parent = group_of.value(key);
        }
    }
    return group_of;
}

void EasyLoadView::handleDrop(const QList<QUrl>& urls) {
    const QString title = QStringLiteral("Bonsai Viewer");

    // 1. Collect the IFC files. Dropped files and folders: a folder asks for one choice
    //    between opening its models only or also recreating its sub-folders as groups.
    QStringList plain;
    QList<FolderDrop> trees;
    for (const QUrl& url : urls) {
        const QString local = url.toLocalFile();
        if (local.isEmpty()) continue;

        if (QFileInfo(local).isDir()) {
            QStringList found;
            QDirIterator it(local, QDir::Files, QDirIterator::Subdirectories);
            while (it.hasNext()) {
                const QString path = it.next();
                if (isIfc(path)) found << path;
            }
            if (found.isEmpty()) continue;  // a folder without IFC files is ignored

            QMessageBox box(QMessageBox::Question, title,
                QStringLiteral("Do you want to open the %1 models found in this folder?").arg(found.size()),
                QMessageBox::Cancel, host_);
            QPushButton* open_only = box.addButton(QStringLiteral("Open only"), QMessageBox::AcceptRole);
            QPushButton* with_tree = box.addButton(QStringLiteral("Open and create folder tree"), QMessageBox::AcceptRole);
            box.setDefaultButton(with_tree);
            box.exec();

            if (box.clickedButton() == open_only) {
                plain << found;
            } else if (box.clickedButton() == with_tree) {
                trees.append(FolderDrop{local, found});
            }
        } else if (isIfc(local)) {
            plain << local;
        }
        // Any other dropped file is silently ignored.
    }
    if (plain.isEmpty() && trees.isEmpty()) return;

    // 2. Plain files: a file already loaded is reloaded only when the user agrees.
    int added = 0;
    int moved = 0;
    for (const QString& path : plain) {
        if (const auto loaded = loadedModelIdFor(path)) {
            const auto answer = QMessageBox::question(
                host_, title,
                QStringLiteral("'%1' is already loaded. Do you want to reload it?").arg(QFileInfo(path).fileName()));
            if (answer != QMessageBox::Yes) continue;
            modules::models::commands::removeModel(session_, viewport_, *host_, *loaded);
        }
        if (modules::models::commands::addModelFromPath(session_, path)) ++added;
    }

    // 3. Folder trees: a loaded model is moved into its group, never reloaded.
    for (const FolderDrop& drop : trees) {
        const QHash<QString, QString> group_of = createFolderTree(drop.root, drop.files);
        auto* federation = session_.federation();
        for (const QString& path : drop.files) {
            const QString group_id = group_of.value(relativeFolder(drop.root, path));
            if (const auto loaded = loadedModelIdFor(path)) {
                federation->setModelGroup(*loaded, group_id);
                ++moved;
                continue;
            }
            if (modules::models::commands::addModelFromPath(session_, path)) {
                ++added;
                if (const auto id = newestModelIdFor(path)) {
                    federation->setModelGroup(*id, group_id);
                }
            }
        }
    }
    if (!trees.isEmpty()) session_.notifyFederationChanged();

    session_.setStatusMessage(QStringLiteral("Models"),
        QStringLiteral("%1 model(s) added, %2 moved by drop").arg(added).arg(moved));
}

} // namespace bonsaiviewer::modules::easy_load
