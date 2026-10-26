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

#ifndef IFCINTERFACE_SERVICES_FILTERSTORE_H
#define IFCINTERFACE_SERVICES_FILTERSTORE_H

#include <QColor>
#include <QObject>
#include <QString>

#include <optional>
#include <vector>

namespace bonsaiviewer::services {

// B06 filters — the document model. One filter is a JSON envelope (name,
// description, layers, actions) whose selections are standard IDS XML
// documents built by IfcTester; the envelope itself is proprietary. The
// whole tree (folders + filters) is one PersistenceService document, and a
// single filter exports to / imports from a standalone envelope file so
// rules can be shared between people.

struct FilterAction {
    enum class Type {
        Isolate,
        AddToView,
        RemoveFromView,
        Colour,
        Transparency,
        AutoColourByProperty,
    };
    Type type = Type::Isolate;
    QColor colour;                // Colour
    int transparency_percent = 0; // Transparency
    QString pset;                 // AutoColourByProperty
    QString property;            // AutoColourByProperty
};

struct FilterLayer {
    // An AND layer reuses the selection of the layer above instead of
    // carrying its own specification.
    bool is_and = false;
    QString ids_xml; // the layer's IDS specification; empty on AND layers
    std::vector<FilterAction> actions;
};

struct FilterDocument {
    QString id; // stable within the store (QUuid)
    QString name;
    QString description;
    QString folder_id; // empty = tree root
    std::vector<FilterLayer> layers;
};

struct FilterFolder {
    QString id;
    QString name;
    QString parent_id; // empty = tree root
};

class FilterStore : public QObject {
    Q_OBJECT

public:
    explicit FilterStore(QObject* parent = nullptr);

    const std::vector<FilterFolder>& folders() const { return folders_; }
    std::vector<FilterDocument> filtersInFolder(const QString& folder_id) const;

    std::optional<FilterDocument> filter(const QString& id) const;
    std::optional<FilterFolder> folder(const QString& id) const;
    bool isFolder(const QString& id) const;
    bool folderIsEmpty(const QString& id) const;

    // Mutations persist immediately (B14) and emit changed().
    QString addFolder(const QString& name, const QString& parent_id);
    void renameFolder(const QString& id, const QString& name);
    // Deletes the folder's whole subtree (its folders and filters).
    void deleteFolder(const QString& id);
    QString addFilter(const FilterDocument& document, const QString& folder_id);
    void updateFilter(const FilterDocument& document);
    void deleteFilter(const QString& id);
    // Drag and drop; empty new_parent is the root. No-ops on cycles.
    void moveNode(const QString& id, const QString& new_parent);

    // Share: write one filter as a standalone envelope file, read one back
    // (new id, stored at the tree root). Empty return means failure.
    bool exportFilter(const QString& filter_id, const QString& file_path) const;
    QString importFilter(const QString& file_path, QString& error);

    // Share the whole tree (folders + filters) as one library file.
    bool exportLibrary(const QString& file_path) const;
    // Import either a single filter envelope or a library into `folder_id`
    // (empty = root). Folders and filters get fresh ids so nothing clashes
    // with the existing tree. Returns the number of filters imported, or
    // -1 with `error` set.
    int importFile(const QString& file_path, const QString& folder_id, QString& error);

signals:
    void changed();

private:
    static FilterDocument documentFromJson(const class QJsonObject& json);
    static QJsonObject documentToJson(const FilterDocument& document);
    static FilterLayer layerFromJson(const QJsonObject& json);
    static QJsonObject layerToJson(const FilterLayer& layer);
    static FilterAction actionFromJson(const QJsonObject& json);
    static QJsonObject actionToJson(const FilterAction& action);

    void load();
    void save() const;
    bool folderIsInSubtree(const QString& candidate_id, const QString& root_id) const;

    std::vector<FilterFolder> folders_;
    std::vector<FilterDocument> filters_;
};

} // namespace bonsaiviewer::services

#endif // IFCINTERFACE_SERVICES_FILTERSTORE_H
