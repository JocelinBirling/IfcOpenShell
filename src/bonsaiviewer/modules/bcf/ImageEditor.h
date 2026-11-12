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

#ifndef IFCINTERFACE_MODULES_BCF_IMAGEEDITOR_H
#define IFCINTERFACE_MODULES_BCF_IMAGEEDITOR_H

#include <QByteArray>
#include <QObject>
#include <QString>

#include <functional>

class QFileSystemWatcher;
class QProcess;

namespace bonsaiviewer::modules::bcf {

// BonsaiViewer10 "Modify the image": opens an image of the BCF in the
// system's image editor (Paint on Windows, Preview on macOS, the default
// application elsewhere) and hands the image back each time the editor
// saves it. The image goes through a temporary file, removed with this
// object. Windows and macOS: the image is read back when the editor closes;
// elsewhere (the launcher returns at once), each time the file changes.
class ImageEditor : public QObject {
    Q_OBJECT

public:
    ImageEditor(const QByteArray& image, std::function<void(const QByteArray&)> changed, QObject* parent);
    ~ImageEditor() override;

    // False when no editor could be started.
    bool open();

private:
    void readBack();

    QString path_;
    QByteArray last_;
    std::function<void(const QByteArray&)> changed_;
    QFileSystemWatcher* watcher_ = nullptr;
    QProcess* process_ = nullptr;
};

} // namespace bonsaiviewer::modules::bcf

#endif // IFCINTERFACE_MODULES_BCF_IMAGEEDITOR_H
