// This file was generated with the assistance of an AI coding tool.
/********************************************************************************
 *                                                                              *
 * This file is part of Bonsai.                                                 *
 *                                                                              *
 * Bonsai is free software: you can redistribute it and/or modify               *
 * it under the terms of the License, or (at your option) any later            *
 * version.                                                                     *
 *                                                                              *
 ********************************************************************************/

#include "EngineClient.h"

#include "AppSettings.h"

#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QUuid>

namespace bonsaiviewer::modules::rule::engineproxy {

EngineClient::EngineClient(QObject* parent) : QObject(parent) {}

EngineClient::~EngineClient() {
    shutdown();
}

bool EngineClient::isProcessRunning() const {
    return process_ && process_->state() != QProcess::NotRunning;
}

bool EngineClient::isConfigured() {
    return !AppSettings::instance().ruleEngineInterpreter().isEmpty()
        && !AppSettings::instance().ruleEngineScript().isEmpty();
}

bool EngineClient::ensureStarted(QString* error) {
    if (isProcessRunning()) return true;

    const QString interpreter = AppSettings::instance().ruleEngineInterpreter();
    const QString script = AppSettings::instance().ruleEngineScript();
    if (interpreter.isEmpty() || script.isEmpty()) {
        if (error) *error = QStringLiteral(
            "The rule engine is not configured: set the interpreter and the "
            "engine script paths in Settings > General (Rule Engine).");
        return false;
    }

    if (!process_) {
        process_ = new QProcess(this);
        connect(process_, &QProcess::readyReadStandardOutput, this, [this]() {
            read_buffer_.append(process_->readAllStandardOutput());
            int newline = 0;
            while ((newline = read_buffer_.indexOf('\n')) >= 0) {
                const QByteArray line = read_buffer_.left(newline);
                read_buffer_.remove(0, newline + 1);
                handleLine(line);
            }
        });
        // The engine's stderr carries its own logs (tracebacks, warnings).
        connect(process_, &QProcess::readyReadStandardError, this, [this]() {
            const QString text = QString::fromUtf8(process_->readAllStandardError());
            emit engineLog(QStringLiteral("stderr"), text.trimmed());
        });
        connect(process_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
            onProcessDied();
        });
        connect(process_, &QProcess::finished, this, [this](int, QProcess::ExitStatus) {
            onProcessDied();
        });
    }

    // The engine runs as a script: the ifcclash_plus directory must be the
    // interpreter's sys.path[0], which QProcess gives us for free by using
    // the script's directory as the working directory.
    const QString working_dir = QFileInfo(script).absolutePath();
    process_->setWorkingDirectory(working_dir);
    process_->start(interpreter, QStringList{script});
    if (!process_->waitForStarted(10000)) {
        if (error) *error = QStringLiteral("The rule engine process failed to start: %1")
                                 .arg(process_->errorString());
        emit engineFailed(error ? *error : QStringLiteral("process start failed"));
        return false;
    }
    return true;
}

void EngineClient::shutdown() {
    if (!process_) return;
    if (isProcessRunning()) {
        // A courteous shutdown request; the process dies on EOF anyway.
        sendRequest({{"id", QUuid::createUuid().toString(QUuid::WithoutBraces)},
                     {"type", QStringLiteral("shutdown")}});
        process_->waitForFinished(2000);
        if (isProcessRunning()) process_->kill();
    }
    job_running_ = false;
}

void EngineClient::sendRequest(const QJsonObject& request) {
    if (!isProcessRunning()) return;
    const QByteArray data = QJsonDocument(request).toJson(QJsonDocument::Compact);
    process_->write(data + '\n');
}

bool EngineClient::requestCatalogue() {
    QString error;
    if (!ensureStarted(&error)) {
        emit engineFailed(error);
        return false;
    }
    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    pending_request_id_ = id;
    sendRequest({{"id", id}, {"type", QStringLiteral("catalogue")}});
    return true;
}

bool EngineClient::requestRun(const QJsonObject& rule_file, const QStringList& ifc_paths, bool preview) {
    if (job_running_) return false;
    QString error;
    if (!ensureStarted(&error)) {
        emit engineFailed(error);
        return false;
    }
    QJsonArray paths;
    for (const QString& path : ifc_paths) paths.append(path);
    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QJsonObject request{{"id", id},
                         {"type", preview ? QStringLiteral("preview") : QStringLiteral("run")},
                         {"list_ifc_path", paths},
                         {"rule_file", rule_file}};
    sendRequest(request);
    job_running_ = true;
    emit runStarted();
    return true;
}

bool EngineClient::requestSelect(const QJsonObject& select, const QStringList& ifc_paths) {
    QString error;
    if (!ensureStarted(&error)) {
        emit engineFailed(error);
        return false;
    }
    QJsonArray paths;
    for (const QString& path : ifc_paths) paths.append(path);
    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    pending_request_id_ = id;
    sendRequest({{"id", id},
                 {"type", QStringLiteral("select")},
                 {"list_ifc_path", paths},
                 {"select", select}});
    return true;
}

void EngineClient::handleLine(const QByteArray& line) {
    const QByteArray trimmed = line.trimmed();
    if (trimmed.isEmpty()) return;
    QJsonParseError parse_error;
    const QJsonDocument document = QJsonDocument::fromJson(trimmed, &parse_error);
    if (parse_error.error != QJsonParseError::NoError || !document.isObject()) {
        emit engineLog(QStringLiteral("error"),
                        QStringLiteral("Malformed engine message: %1").arg(QString::fromUtf8(trimmed)));
        return;
    }
    const QJsonObject message = document.object();
    const QString type = message.value("type").toString();
    if (type == QStringLiteral("ready")) {
        emit engineReady(message.value("version").toString());
    } else if (type == QStringLiteral("reply")) {
        handleReply(message);
    } else {
        handleEvent(message);
    }
}

void EngineClient::handleReply(const QJsonObject& message) {
    const QString to = message.value("reply_to").toString();
    if (message.contains("error")) {
        emit engineFailed(message.value("error").toString());
        return;
    }
    if (to == QStringLiteral("catalogue")) {
        const QJsonObject catalogue = message.value("catalogue").toObject();
        emit catalogueReady(catalogue.value("rules").toArray(),
                            catalogue.value("facets").toArray());
    } else if (to == QStringLiteral("select")) {
        std::vector<ElementRef> elements;
        for (const QJsonValue& value : message.value("elements").toArray()) {
            elements.push_back(elementRefFromJson(value.toObject()));
        }
        emit selectFinished(elements);
    }
}

void EngineClient::handleEvent(const QJsonObject& message) {
    const QString type = message.value("type").toString();
    if (type == QStringLiteral("progress")) {
        emit progress(message.value("percent").toInt(), message.value("message").toString());
    } else if (type == QStringLiteral("rule_start")) {
        emit progress(-1, QStringLiteral("Running %1...").arg(message.value("rule_type").toString()));
    } else if (type == QStringLiteral("rule_result")) {
        std::vector<EngineResult> results;
        for (const QJsonValue& value : message.value("results").toArray()) {
            results.push_back(engineResultFromJson(value.toObject()));
        }
        emit ruleResult(message.value("node_id").toString(),
                        message.value("rule_type").toString(), results);
    } else if (type == QStringLiteral("display")) {
        std::vector<DisplayItem> items;
        for (const QJsonValue& value : message.value("items").toArray()) {
            items.push_back(displayItemFromJson(value.toObject()));
        }
        emit displayItemsReady(message.value("node_id").toString(),
                               message.value("rule_type").toString(), items);
    } else if (type == QStringLiteral("run_done")) {
        const bool cancelled = message.value("cancelled").toBool();
        if (message.contains("error")) {
            emit engineFailed(message.value("error").toString());
        }
        job_running_ = false;
        emit runFinished(cancelled, message.value("error").toString());
    } else if (type == QStringLiteral("log")) {
        emit engineLog(message.value("level").toString(), message.value("message").toString());
    }
}

void EngineClient::onProcessDied() {
    if (!process_) return;
    if (job_running_) {
        job_running_ = false;
        emit runFinished(false, QStringLiteral("The rule engine process died during the run"));
    }
    emit engineFailed(QStringLiteral("The rule engine process stopped"));
    process_->deleteLater();
    process_ = nullptr;
}

} // namespace bonsaiviewer::modules::rule::engineproxy
