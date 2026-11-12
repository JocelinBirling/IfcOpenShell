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

#include "ImageEditor.h"

#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileSystemWatcher>
#include <QProcess>
#include <QTimer>
#include <QUrl>
#include <QUuid>

namespace bonsaiviewer::modules::bcf {

ImageEditor::ImageEditor(const QByteArray& image, std::function<void(const QByteArray&)> changed, QObject* parent)
    : QObject(parent), last_(image), changed_(std::move(changed)) {
    path_ = QDir::temp().filePath(
        QString("bonsaiviewer-bcf-image-%1.png").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
    QFile file(path_);
    if (file.open(QIODevice::WriteOnly)) file.write(image);
}

ImageEditor::~ImageEditor() {
    QFile::remove(path_);
}

bool ImageEditor::open() {
#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
    process_ = new QProcess(this);
    connect(process_, &QProcess::finished, this, [this](int, QProcess::ExitStatus) { readBack(); });
#if defined(Q_OS_WIN)
    process_->start(QStringLiteral("mspaint.exe"), {QDir::toNativeSeparators(path_)});
#else
    process_->start(QStringLiteral("open"), {QStringLiteral("-W"), QStringLiteral("-a"), QStringLiteral("Preview"), path_});
#endif
    return process_->waitForStarted(5000);
#else
    // The launcher returns at once: follow the file instead.
    watcher_ = new QFileSystemWatcher({path_}, this);
    connect(watcher_, &QFileSystemWatcher::fileChanged, this, [this]() {
        // Editors often replace the file: watch it again, read once written.
        QTimer::singleShot(300, this, [this]() {
            if (!watcher_->files().contains(path_) && QFile::exists(path_)) watcher_->addPath(path_);
            readBack();
        });
    });
    return QDesktopServices::openUrl(QUrl::fromLocalFile(path_));
#endif
}

void ImageEditor::readBack() {
    QFile file(path_);
    if (!file.open(QIODevice::ReadOnly)) return;
    const QByteArray image = file.readAll();
    if (image.isEmpty() || image == last_) return;
    last_ = image;
    if (changed_) changed_(image);
}

} // namespace bonsaiviewer::modules::bcf
