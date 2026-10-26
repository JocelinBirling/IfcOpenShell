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

#ifndef IFCINTERFACE_MODULES_EASYLOAD_VIEW_H
#define IFCINTERFACE_MODULES_EASYLOAD_VIEW_H

#include <QHash>
#include <QList>
#include <QObject>
#include <QString>
#include <QUrl>

#include <optional>

class QWidget;
class ViewportWindow;
namespace bonsaiviewer { class SessionState; }

namespace bonsaiviewer::modules::easy_load {

// BonsaiViewer22 — drag and drop of IFC files and folders onto the 3D view.
// Dropped IFC files are added to the view; other files are ignored. A dropped
// folder is searched recursively, after a choice: open its models only, or also
// recreate its sub-folders as groups in the Models panel (the dropped folder
// itself is not a group). A file already loaded is added again only when the
// user agrees to reload it; in tree mode a loaded model is moved into its group.
class EasyLoadView : public QObject {
    Q_OBJECT

public:
    EasyLoadView(bonsaiviewer::SessionState& session, ViewportWindow& viewport,
                 QWidget* host, QObject* parent = nullptr);

    void handleDrop(const QList<QUrl>& urls);

private:
    // The model id of the loaded model whose file is `path`, when one is loaded.
    std::optional<QString> loadedModelIdFor(const QString& path) const;
    // The model id of the most recently added model whose source is `path`.
    std::optional<QString> newestModelIdFor(const QString& path) const;
    // Creates one group per sub-folder of `root` holding IFC files, parents first.
    // Returns the group id for each relative folder ("a/b"); files directly in
    // `root` have no entry and stay at the root of the Models panel.
    QHash<QString, QString> createFolderTree(const QString& root, const QStringList& files);

    bonsaiviewer::SessionState& session_;
    ViewportWindow& viewport_;
    QWidget* host_ = nullptr;
};

} // namespace bonsaiviewer::modules::easy_load

#endif // IFCINTERFACE_MODULES_EASYLOAD_VIEW_H
