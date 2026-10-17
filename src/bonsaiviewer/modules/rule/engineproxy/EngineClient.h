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

#ifndef IFCINTERFACE_MODULES_RULE_ENGINEPROXY_ENGINE_CLIENT_H
#define IFCINTERFACE_MODULES_RULE_ENGINEPROXY_ENGINE_CLIENT_H

#include "Messages.h"

#include <QJsonArray>
#include <QObject>
#include <QString>
#include <QStringList>

class QProcess;

namespace bonsaiviewer::modules::rule::engineproxy {

// The C++ side of the rule engine IPC (SPEC_IfcClash_Plus_Integration.md
// sections 2 to 4): one persistent Python worker process (ifcclash_plus
// ipc.py) speaking JSON Lines, driven asynchronously through Qt signals.
//
// One job at a time: requestRun() while a run is in flight is refused
// (returns false). The engine's answers arrive as signals; the caller
// keeps the mapping between the model indexes of the results and its own
// session model ids (it sent list_ifc_path in that same order).
class EngineClient : public QObject {
    Q_OBJECT
public:
    explicit EngineClient(QObject* parent = nullptr);
    ~EngineClient() override;

    bool isProcessRunning() const;
    bool isJobRunning() const { return job_running_; }

    // Starts the engine process with the interpreter and script configured
    // in AppSettings. Returns false (and fills `error`) when the engine is
    // not configured or the process fails to start. Idempotent: a running
    // process is reused.
    bool ensureStarted(QString* error = nullptr);

    // Stops the engine (end of session). A running job is dropped.
    void shutdown();

    bool requestCatalogue();
    bool requestRun(const QJsonObject& rule_file, const QStringList& ifc_paths, bool preview);
    bool requestSelect(const QJsonObject& select, const QStringList& ifc_paths);

    // True when the engine paths are configured.
    static bool isConfigured();

signals:
    // Lifecycle
    void engineReady(const QString& engine_version);
    void engineFailed(const QString& error);
    void engineLog(const QString& level, const QString& message);

    // Catalogue reply: the full registry of rules and facets.
    void catalogueReady(const QJsonArray& rules, const QJsonArray& facets);

    // Run lifecycle
    void runStarted();
    void progress(int percent, const QString& message);
    void ruleResult(const QString& node_id, const QString& rule_type,
                    const std::vector<EngineResult>& results);
    void displayItemsReady(const QString& node_id, const QString& rule_type,
                           const std::vector<DisplayItem>& items);
    void runFinished(bool cancelled, const QString& error);

    // Select reply
    void selectFinished(const std::vector<ElementRef>& elements);

private:
    void sendRequest(const QJsonObject& request);
    void handleLine(const QByteArray& line);
    void handleEvent(const QJsonObject& message);
    void handleReply(const QJsonObject& message);
    void onProcessDied();

    QProcess* process_ = nullptr;
    QByteArray read_buffer_;
    QString pending_request_id_; // the select/catalogue reply correlation
    bool job_running_ = false;
};

} // namespace bonsaiviewer::modules::rule::engineproxy

#endif
