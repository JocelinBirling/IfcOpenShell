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

#ifndef IFCINTERFACE_MODULES_TABLE_TABLESTORE_H
#define IFCINTERFACE_MODULES_TABLE_TABLESTORE_H

#include "Table.h"

#include <QJsonObject>
#include <QObject>
#include <QString>

#include <optional>
#include <vector>

namespace bonsaiviewer::modules::table {

// TI07 phase 2 — the saved tables: folders and table configurations (name,
// description, IDS selection, columns) in one PersistenceService document
// (tables.json). A single table exports to a standalone file and imports
// back as a new table, so tables can be shared.
struct TableDocument {
    QString id;        // stable within the store (QUuid)
    QString folder_id; // empty = tree root
    TableDefinition definition;
};

struct TableFolder {
    QString id;
    QString name;
    QString parent_id; // empty = tree root
};

class TableStore : public QObject {
    Q_OBJECT

public:
    explicit TableStore(QObject* parent = nullptr);

    const std::vector<TableFolder>& folders() const { return folders_; }
    std::vector<TableDocument> tablesInFolder(const QString& folder_id) const;
    std::optional<TableDocument> table(const QString& id) const;
    std::optional<TableFolder> folder(const QString& id) const;
    bool isFolder(const QString& id) const;

    // Mutations persist immediately and emit changed().
    QString addFolder(const QString& name, const QString& parent_id);
    void renameFolder(const QString& id, const QString& name);
    // Deletes the folder's whole subtree (its folders and tables).
    void deleteFolder(const QString& id);
    // Adds a table (its id is ignored) and returns the new id.
    QString addTable(const TableDocument& document, const QString& folder_id);
    // Replaces the definition of a stored table; its folder is kept.
    void updateTable(const TableDocument& document);
    void deleteTable(const QString& id);
    // Drag and drop; empty new_parent is the root. No-ops on cycles.
    void moveNode(const QString& id, const QString& new_parent);

    // Share: one table as a standalone file. Import returns the new id, or
    // an empty string with `error` set.
    bool exportTable(const QString& id, const QString& file_path) const;
    QString importTable(const QString& file_path, const QString& folder_id, QString& error);

signals:
    void changed();

private:
    static QJsonObject tableToJson(const TableDocument& document);
    static TableDocument tableFromJson(const QJsonObject& json);

    void load();
    void seedExamples();
    void save() const;
    bool folderIsInSubtree(const QString& candidate_id, const QString& root_id) const;

    std::vector<TableFolder> folders_;
    std::vector<TableDocument> tables_;
};

} // namespace bonsaiviewer::modules::table

#endif // IFCINTERFACE_MODULES_TABLE_TABLESTORE_H
