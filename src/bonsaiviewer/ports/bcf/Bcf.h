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

#ifndef BONSAIVIEWER_PORTS_BCF_BCF_H
#define BONSAIVIEWER_PORTS_BCF_BCF_H

// C++ port of IfcOpenShell's bcf library (src/bcf, BCF-XML 2.1 and 3.0).
// The Python library has one model per version (bcf.v2.model, bcf.v3.model)
// and a version-agnostic layer (bcf.agnostic); this port keeps a single
// version-agnostic model, read from and written to either version by
// BcfXml. Dates are kept as their xs:dateTime text so they round-trip as
// read. The files of the archive the model does not describe (BIM
// snippets, header reference files, v2 internal documents, bitmaps...) are
// kept as bytes and written back.

#include <QByteArray>
#include <QString>
#include <QStringList>

#include <map>
#include <optional>
#include <vector>

namespace bonsaiviewer::ports::bcf {

// --------------------------------------------------------------- visinfo

struct Vector3 {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

struct Component {
    QString ifc_guid;
    QString originating_system;
    QString authoring_tool_id;
};

struct ViewSetupHints {
    bool spaces_visible = false;
    bool space_boundaries_visible = false;
    bool openings_visible = false;
};

struct Coloring {
    QString color; // "RRGGBB" or "AARRGGBB" (hex)
    std::vector<Component> components;
};

struct Camera {
    enum class Kind { None, Perspective, Orthogonal };
    Kind kind = Kind::None;
    Vector3 view_point;
    Vector3 direction;
    Vector3 up_vector;
    double field_of_view = 60.0;       // Perspective: vertical, degrees
    double view_to_world_scale = 1.0;  // Orthogonal: visible height, metres
    std::optional<double> aspect_ratio; // 3.0 only (w / h)
};

struct Line {
    Vector3 start;
    Vector3 end;
};

struct ClippingPlane {
    Vector3 location;
    Vector3 direction;
};

struct Bitmap {
    QString format; // png / jpg
    QString reference;
    Vector3 location;
    Vector3 normal;
    Vector3 up;
    double height = 0.0;
};

struct VisualizationInfo {
    QString guid;
    std::vector<Component> selection;
    bool default_visibility = false;
    std::vector<Component> exceptions; // the components that differ from default_visibility
    std::optional<ViewSetupHints> view_setup_hints;
    std::vector<Coloring> coloring;
    Camera camera;
    std::vector<Line> lines;
    std::vector<ClippingPlane> clipping_planes;
    std::vector<Bitmap> bitmaps;
};

// ---------------------------------------------------------------- markup

// One viewpoint of a topic: the markup entry, its visualization info, its
// snapshot (PNG / JPG bytes).
struct Viewpoint {
    QString guid;
    QString viewpoint_file; // "<guid>.bcfv" (empty: no visualization info)
    QString snapshot_file;  // "<guid>.png" (empty: no snapshot)
    std::optional<int> index;
    std::optional<VisualizationInfo> info;
    QByteArray snapshot;
};

struct Comment {
    QString guid;
    QString date;
    QString author;
    QString text;
    QString viewpoint_guid; // empty: no viewpoint
    QString modified_date;
    QString modified_author;
};

// A document of the topic. 3.0: an internal document (document_guid, in
// the documents of the BcfFile) or an external URL. 2.1: a path or URL in
// `url`, external or not (a relative path is relative to the topic folder).
struct DocumentReference {
    QString guid;
    QString document_guid;
    QString url;
    bool is_external = false;
    QString description;
};

struct HeaderFile {
    QString ifc_project;
    QString ifc_spatial_structure_element;
    bool is_external = true;
    QString filename;
    QString date;
    QString reference;
};

struct BimSnippet {
    QString snippet_type;
    bool is_external = false;
    QString reference;
    QString reference_schema;
};

struct Topic {
    QString guid;
    QString server_assigned_id;
    QString topic_type;
    QString topic_status;
    QStringList reference_links;
    QString title;
    QString priority;
    std::optional<int> index;
    QStringList labels;
    QString creation_date;
    QString creation_author;
    QString modified_date;
    QString modified_author;
    QString due_date;
    QString assigned_to;
    QString stage;
    QString description;
    std::optional<BimSnippet> bim_snippet;
    std::vector<DocumentReference> document_references;
    QStringList related_topics; // topic guids
    std::vector<Comment> comments;
    std::vector<Viewpoint> viewpoints;
    std::vector<HeaderFile> header_files;
};

// ------------------------------------------------------------------- file

struct Project {
    QString project_id;
    QString name;
};

// The allowed values (3.0 extensions.xml; 2.1 extension schema).
struct Extensions {
    QStringList topic_types;
    QStringList topic_statuses;
    QStringList priorities;
    QStringList topic_labels;
    QStringList users;
    QStringList snippet_types;
    QStringList stages;
};

// 3.0 documents.xml: a document stored in the archive (documents/<guid>).
struct Document {
    QString guid;
    QString filename;
    QString description;
    QByteArray data;
};

struct BcfFile {
    QString version = "3.0"; // as read: "2.1" or "3.0"
    std::optional<Project> project;
    Extensions extensions;
    std::vector<Document> documents;
    std::vector<Topic> topics;
    // The archive entries not described by the model (path -> bytes),
    // written back as they are.
    std::map<QString, QByteArray> other_files;
};

// A new random GUID in the BCF form (lower case, no braces).
QString newGuid();
// The current date and time as xs:dateTime, UTC ("2026-10-28T15:47:00Z").
QString now();

} // namespace bonsaiviewer::ports::bcf

#endif // BONSAIVIEWER_PORTS_BCF_BCF_H
