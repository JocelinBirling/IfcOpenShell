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

#include "BcfXml.h"

#include "Zip.h"

#include <QDateTime>
#include <functional>
#include <QMap>
#include <QRegularExpression>
#include <QSet>
#include <QUuid>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace bonsaiviewer::ports::bcf {

QString newGuid() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces).toLower();
}

QString now() {
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
}

namespace {

// ------------------------------------------------------------- XML input

// A light element tree (the Python port parses with xsdata into the model
// classes; here the tree is read once, then mapped). Local names only:
// BCF files are written without a target namespace.
struct Node {
    QString name;
    QMap<QString, QString> attributes;
    QString text;
    std::vector<Node> children;

    const Node* child(const QString& child_name) const {
        for (const Node& node : children) {
            if (node.name == child_name) return &node;
        }
        return nullptr;
    }
    std::vector<const Node*> all(const QString& child_name) const {
        std::vector<const Node*> found;
        for (const Node& node : children) {
            if (node.name == child_name) found.push_back(&node);
        }
        return found;
    }
    QString childText(const QString& child_name) const {
        const Node* node = child(child_name);
        return node ? node->text : QString();
    }
    QString attribute(const QString& attribute_name) const {
        return attributes.value(attribute_name);
    }
};

std::optional<Node> parse(const QByteArray& xml, QString* error) {
    QXmlStreamReader reader(xml);
    std::vector<Node> stack;
    std::optional<Node> root;
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement()) {
            Node node;
            node.name = reader.name().toString();
            for (const QXmlStreamAttribute& attribute : reader.attributes()) {
                node.attributes.insert(attribute.name().toString(), attribute.value().toString());
            }
            stack.push_back(std::move(node));
        } else if (reader.isCharacters() && !stack.empty()) {
            stack.back().text += reader.text();
        } else if (reader.isEndElement() && !stack.empty()) {
            Node node = std::move(stack.back());
            stack.pop_back();
            if (!node.children.empty()) node.text = node.text.trimmed();
            if (stack.empty()) root = std::move(node);
            else stack.back().children.push_back(std::move(node));
        }
    }
    if (reader.hasError() || !root) {
        if (error) *error = reader.hasError() ? reader.errorString() : QStringLiteral("empty XML document");
        return std::nullopt;
    }
    return root;
}

bool toBool(const QString& text, bool fallback) {
    const QString value = text.trimmed().toLower();
    if (value == QLatin1String("true") || value == QLatin1String("1")) return true;
    if (value == QLatin1String("false") || value == QLatin1String("0")) return false;
    return fallback;
}

std::optional<int> toInt(const QString& text) {
    bool ok = false;
    const int value = text.trimmed().toInt(&ok);
    return ok ? std::optional<int>(value) : std::nullopt;
}

double toDouble(const QString& text, double fallback = 0.0) {
    bool ok = false;
    const double value = text.trimmed().toDouble(&ok);
    return ok ? value : fallback;
}

Vector3 readVector(const Node* node) {
    Vector3 vector;
    if (!node) return vector;
    vector.x = toDouble(node->childText("X"));
    vector.y = toDouble(node->childText("Y"));
    vector.z = toDouble(node->childText("Z"));
    return vector;
}

Component readComponent(const Node& node) {
    Component component;
    component.ifc_guid = node.attribute("IfcGuid");
    component.originating_system = node.childText("OriginatingSystem");
    component.authoring_tool_id = node.childText("AuthoringToolId");
    return component;
}

std::vector<Component> readComponents(const Node* parent) {
    std::vector<Component> components;
    if (!parent) return components;
    for (const Node* node : parent->all("Component")) components.push_back(readComponent(*node));
    return components;
}

// A list either wrapped (3.0: <Labels><Label/></Labels>) or repeated (2.1: <Labels/><Labels/>).
QStringList readList(const Node& parent, const QString& wrapper, const QString& item) {
    QStringList values;
    const auto wrappers = parent.all(wrapper);
    if (wrappers.size() == 1 && !wrappers.front()->children.empty()) {
        for (const Node* node : wrappers.front()->all(item)) values.append(node->text);
        return values;
    }
    for (const Node* node : wrappers) {
        if (node->children.empty() && !node->text.isEmpty()) values.append(node->text);
    }
    return values;
}

// -------------------------------------------------------------- visinfo

VisualizationInfo readVisualizationInfo(const Node& root) {
    VisualizationInfo info;
    info.guid = root.attribute("Guid");
    if (const Node* components = root.child("Components")) {
        info.selection = readComponents(components->child("Selection"));
        const Node* visibility = components->child("Visibility");
        if (visibility) {
            info.default_visibility = toBool(visibility->attribute("DefaultVisibility"), false);
            info.exceptions = readComponents(visibility->child("Exceptions"));
        }
        // 2.1: in Components; 3.0: in Visibility.
        const Node* hints = components->child("ViewSetupHints");
        if (!hints && visibility) hints = visibility->child("ViewSetupHints");
        if (hints) {
            ViewSetupHints value;
            value.spaces_visible = toBool(hints->attribute("SpacesVisible"), false);
            value.space_boundaries_visible = toBool(hints->attribute("SpaceBoundariesVisible"), false);
            value.openings_visible = toBool(hints->attribute("OpeningsVisible"), false);
            info.view_setup_hints = value;
        }
        if (const Node* coloring = components->child("Coloring")) {
            for (const Node* color : coloring->all("Color")) {
                Coloring entry;
                entry.color = color->attribute("Color");
                // 3.0: Color/Components/Component; 2.1: Color/Component.
                const Node* list = color->child("Components");
                entry.components = readComponents(list ? list : color);
                info.coloring.push_back(std::move(entry));
            }
        }
    }
    const auto readCamera = [&info](const Node* node, Camera::Kind kind) {
        if (!node) return;
        Camera& camera = info.camera;
        camera.kind = kind;
        camera.view_point = readVector(node->child("CameraViewPoint"));
        camera.direction = readVector(node->child("CameraDirection"));
        camera.up_vector = readVector(node->child("CameraUpVector"));
        if (kind == Camera::Kind::Perspective) camera.field_of_view = toDouble(node->childText("FieldOfView"), 60.0);
        if (kind == Camera::Kind::Orthogonal) {
            camera.view_to_world_scale = toDouble(node->childText("ViewToWorldScale"), 1.0);
        }
        if (node->child("AspectRatio")) camera.aspect_ratio = toDouble(node->childText("AspectRatio"), 1.0);
    };
    readCamera(root.child("PerspectiveCamera"), Camera::Kind::Perspective);
    readCamera(root.child("OrthogonalCamera"), Camera::Kind::Orthogonal);
    if (const Node* lines = root.child("Lines")) {
        for (const Node* node : lines->all("Line")) {
            info.lines.push_back({readVector(node->child("StartPoint")), readVector(node->child("EndPoint"))});
        }
    }
    if (const Node* planes = root.child("ClippingPlanes")) {
        for (const Node* node : planes->all("ClippingPlane")) {
            info.clipping_planes.push_back({readVector(node->child("Location")), readVector(node->child("Direction"))});
        }
    }
    // 3.0: Bitmaps/Bitmap with Format; 2.1: repeated Bitmap with a Bitmap child (the format).
    std::vector<const Node*> bitmaps;
    if (const Node* wrapper = root.child("Bitmaps")) bitmaps = wrapper->all("Bitmap");
    else bitmaps = root.all("Bitmap");
    for (const Node* node : bitmaps) {
        Bitmap bitmap;
        bitmap.format = (node->child("Format") ? node->childText("Format") : node->childText("Bitmap")).toLower();
        bitmap.reference = node->childText("Reference");
        bitmap.location = readVector(node->child("Location"));
        bitmap.normal = readVector(node->child("Normal"));
        bitmap.up = readVector(node->child("Up"));
        bitmap.height = toDouble(node->childText("Height"));
        info.bitmaps.push_back(std::move(bitmap));
    }
    return info;
}

// --------------------------------------------------------------- markup

Comment readComment(const Node& node) {
    Comment comment;
    comment.guid = node.attribute("Guid");
    comment.date = node.childText("Date");
    comment.author = node.childText("Author");
    comment.text = node.childText("Comment");
    if (const Node* viewpoint = node.child("Viewpoint")) comment.viewpoint_guid = viewpoint->attribute("Guid");
    comment.modified_date = node.childText("ModifiedDate");
    comment.modified_author = node.childText("ModifiedAuthor");
    return comment;
}

Viewpoint readViewpointEntry(const Node& node) {
    Viewpoint viewpoint;
    viewpoint.guid = node.attribute("Guid");
    viewpoint.viewpoint_file = node.childText("Viewpoint");
    viewpoint.snapshot_file = node.childText("Snapshot");
    viewpoint.index = toInt(node.childText("Index"));
    return viewpoint;
}

Topic readMarkup(const Node& markup) {
    Topic topic;
    if (const Node* header = markup.child("Header")) {
        // 3.0: Header/Files/File; 2.1: Header/File.
        const Node* files = header->child("Files");
        for (const Node* node : (files ? files : header)->all("File")) {
            HeaderFile file;
            file.ifc_project = node->attribute("IfcProject");
            file.ifc_spatial_structure_element = node->attribute("IfcSpatialStructureElement");
            const QString external = node->attributes.contains("IsExternal") ? node->attribute("IsExternal")
                                                                              : node->attribute("isExternal");
            file.is_external = toBool(external, true);
            file.filename = node->childText("Filename");
            file.date = node->childText("Date");
            file.reference = node->childText("Reference");
            topic.header_files.push_back(std::move(file));
        }
    }
    const Node* node = markup.child("Topic");
    if (!node) return topic;
    topic.guid = node->attribute("Guid");
    topic.server_assigned_id = node->attribute("ServerAssignedId");
    topic.topic_type = node->attribute("TopicType");
    topic.topic_status = node->attribute("TopicStatus");
    if (const Node* links = node->child("ReferenceLinks")) {
        for (const Node* link : links->all("ReferenceLink")) topic.reference_links.append(link->text);
    } else {
        for (const Node* link : node->all("ReferenceLink")) topic.reference_links.append(link->text);
    }
    topic.title = node->childText("Title");
    topic.priority = node->childText("Priority");
    topic.index = toInt(node->childText("Index"));
    topic.labels = readList(*node, "Labels", "Label");
    topic.creation_date = node->childText("CreationDate");
    topic.creation_author = node->childText("CreationAuthor");
    topic.modified_date = node->childText("ModifiedDate");
    topic.modified_author = node->childText("ModifiedAuthor");
    topic.due_date = node->childText("DueDate");
    topic.assigned_to = node->childText("AssignedTo");
    topic.stage = node->childText("Stage");
    topic.description = node->childText("Description");
    if (const Node* snippet = node->child("BimSnippet")) {
        BimSnippet value;
        value.snippet_type = snippet->attribute("SnippetType");
        const QString external = snippet->attributes.contains("IsExternal") ? snippet->attribute("IsExternal")
                                                                           : snippet->attribute("isExternal");
        value.is_external = toBool(external, false);
        value.reference = snippet->childText("Reference");
        value.reference_schema = snippet->childText("ReferenceSchema");
        topic.bim_snippet = value;
    }
    if (const Node* references = node->child("DocumentReferences")) {
        for (const Node* reference : references->all("DocumentReference")) {
            DocumentReference value;
            value.guid = reference->attribute("Guid");
            value.document_guid = reference->childText("DocumentGuid");
            value.url = reference->childText("Url");
            value.is_external = value.document_guid.isEmpty();
            value.description = reference->childText("Description");
            topic.document_references.push_back(std::move(value));
        }
    } else {
        for (const Node* reference : node->all("DocumentReference")) {
            DocumentReference value;
            value.guid = reference->attribute("Guid");
            value.url = reference->childText("ReferencedDocument");
            value.is_external = toBool(reference->attribute("isExternal"), false);
            value.description = reference->childText("Description");
            topic.document_references.push_back(std::move(value));
        }
    }
    if (const Node* related = node->child("RelatedTopics")) {
        for (const Node* item : related->all("RelatedTopic")) topic.related_topics.append(item->attribute("Guid"));
    } else {
        for (const Node* item : node->all("RelatedTopic")) topic.related_topics.append(item->attribute("Guid"));
    }
    // Comments and viewpoints: 3.0 in the Topic, 2.1 in the Markup.
    if (const Node* comments = node->child("Comments")) {
        for (const Node* comment : comments->all("Comment")) topic.comments.push_back(readComment(*comment));
    } else {
        for (const Node* comment : markup.all("Comment")) topic.comments.push_back(readComment(*comment));
    }
    if (const Node* viewpoints = node->child("Viewpoints"); viewpoints && !viewpoints->children.empty()) {
        for (const Node* item : viewpoints->all("ViewPoint")) topic.viewpoints.push_back(readViewpointEntry(*item));
    } else {
        for (const Node* item : markup.all("Viewpoints")) topic.viewpoints.push_back(readViewpointEntry(*item));
    }
    return topic;
}

// Extension values of a 2.1 extension schema: the enumerations of its simple types.
Extensions readExtensionSchema(const Node& root) {
    Extensions extensions;
    std::function<void(const Node&)> walk = [&](const Node& node) {
        if (node.name == QLatin1String("simpleType")) {
            QStringList values;
            std::function<void(const Node&)> collect = [&](const Node& child) {
                if (child.name == QLatin1String("enumeration")) values.append(child.attribute("value"));
                for (const Node& grandchild : child.children) collect(grandchild);
            };
            collect(node);
            const QString type = node.attribute("name");
            if (type == QLatin1String("TopicType")) extensions.topic_types = values;
            else if (type == QLatin1String("TopicStatus")) extensions.topic_statuses = values;
            else if (type == QLatin1String("Priority")) extensions.priorities = values;
            else if (type == QLatin1String("TopicLabel")) extensions.topic_labels = values;
            else if (type == QLatin1String("UserIdType")) extensions.users = values;
            else if (type == QLatin1String("SnippetType")) extensions.snippet_types = values;
            else if (type == QLatin1String("Stage")) extensions.stages = values;
            return;
        }
        for (const Node& child : node.children) walk(child);
    };
    walk(root);
    return extensions;
}

Extensions readExtensions(const Node& root) {
    Extensions extensions;
    extensions.topic_types = readList(root, "TopicTypes", "TopicType");
    extensions.topic_statuses = readList(root, "TopicStatuses", "TopicStatus");
    extensions.priorities = readList(root, "Priorities", "Priority");
    extensions.topic_labels = readList(root, "TopicLabels", "TopicLabel");
    extensions.users = readList(root, "Users", "User");
    extensions.snippet_types = readList(root, "SnippetTypes", "SnippetType");
    extensions.stages = readList(root, "Stages", "Stage");
    return extensions;
}

// ------------------------------------------------------------ XML output

QString number(double value) {
    return QString::number(value, 'g', 17);
}

class Writer {
public:
    Writer() : xml_(&bytes_) {
        xml_.setAutoFormatting(true);
        xml_.setAutoFormattingIndent(2);
        xml_.writeStartDocument();
    }
    QXmlStreamWriter& xml() { return xml_; }
    void start(const QString& name) { xml_.writeStartElement(name); }
    void end() { xml_.writeEndElement(); }
    void attribute(const QString& name, const QString& value) { xml_.writeAttribute(name, value); }
    // An element with text, left out when the text is empty.
    void optional(const QString& name, const QString& value) {
        if (!value.isEmpty()) xml_.writeTextElement(name, value);
    }
    void text(const QString& name, const QString& value) { xml_.writeTextElement(name, value); }
    void vector(const QString& name, const Vector3& value) {
        start(name);
        text("X", number(value.x));
        text("Y", number(value.y));
        text("Z", number(value.z));
        end();
    }
    QByteArray finish() {
        xml_.writeEndDocument();
        return bytes_;
    }

private:
    QByteArray bytes_;
    QXmlStreamWriter xml_;
};

void writeComponent(Writer& out, const Component& component) {
    out.start("Component");
    if (!component.ifc_guid.isEmpty()) out.attribute("IfcGuid", component.ifc_guid);
    out.optional("OriginatingSystem", component.originating_system);
    out.optional("AuthoringToolId", component.authoring_tool_id);
    out.end();
}

void writeHints(Writer& out, const ViewSetupHints& hints) {
    out.start("ViewSetupHints");
    out.attribute("SpacesVisible", hints.spaces_visible ? "true" : "false");
    out.attribute("SpaceBoundariesVisible", hints.space_boundaries_visible ? "true" : "false");
    out.attribute("OpeningsVisible", hints.openings_visible ? "true" : "false");
    out.end();
}

QByteArray writeInfo(const VisualizationInfo& info, bool v3) {
    Writer out;
    out.start("VisualizationInfo");
    out.attribute("Guid", info.guid);
    out.start("Components");
    if (!v3 && info.view_setup_hints) writeHints(out, *info.view_setup_hints);
    if (!info.selection.empty()) {
        out.start("Selection");
        for (const Component& component : info.selection) writeComponent(out, component);
        out.end();
    }
    out.start("Visibility");
    out.attribute("DefaultVisibility", info.default_visibility ? "true" : "false");
    if (v3 && info.view_setup_hints) writeHints(out, *info.view_setup_hints);
    if (!info.exceptions.empty()) {
        out.start("Exceptions");
        for (const Component& component : info.exceptions) writeComponent(out, component);
        out.end();
    }
    out.end(); // Visibility
    if (!info.coloring.empty()) {
        out.start("Coloring");
        for (const Coloring& coloring : info.coloring) {
            if (coloring.components.empty()) continue;
            out.start("Color");
            out.attribute("Color", coloring.color);
            if (v3) out.start("Components");
            for (const Component& component : coloring.components) writeComponent(out, component);
            if (v3) out.end();
            out.end();
        }
        out.end();
    }
    out.end(); // Components
    const Camera& camera = info.camera;
    if (camera.kind != Camera::Kind::None) {
        const bool perspective = camera.kind == Camera::Kind::Perspective;
        out.start(perspective ? "PerspectiveCamera" : "OrthogonalCamera");
        out.vector("CameraViewPoint", camera.view_point);
        out.vector("CameraDirection", camera.direction);
        out.vector("CameraUpVector", camera.up_vector);
        if (perspective) out.text("FieldOfView", number(camera.field_of_view));
        else out.text("ViewToWorldScale", number(camera.view_to_world_scale));
        if (v3) out.text("AspectRatio", number(camera.aspect_ratio.value_or(1.0)));
        out.end();
    }
    if (!info.lines.empty()) {
        out.start("Lines");
        for (const Line& line : info.lines) {
            out.start("Line");
            out.vector("StartPoint", line.start);
            out.vector("EndPoint", line.end);
            out.end();
        }
        out.end();
    }
    if (!info.clipping_planes.empty()) {
        out.start("ClippingPlanes");
        for (const ClippingPlane& plane : info.clipping_planes) {
            out.start("ClippingPlane");
            out.vector("Location", plane.location);
            out.vector("Direction", plane.direction);
            out.end();
        }
        out.end();
    }
    if (!info.bitmaps.empty()) {
        if (v3) out.start("Bitmaps");
        for (const Bitmap& bitmap : info.bitmaps) {
            out.start("Bitmap");
            out.text(v3 ? "Format" : "Bitmap", v3 ? bitmap.format.toLower() : bitmap.format.toUpper());
            out.text("Reference", bitmap.reference);
            out.vector("Location", bitmap.location);
            out.vector("Normal", bitmap.normal);
            out.vector("Up", bitmap.up);
            out.text("Height", number(bitmap.height));
            out.end();
        }
        if (v3) out.end();
    }
    out.end(); // VisualizationInfo
    return out.finish();
}

void writeComment(Writer& out, const Comment& comment, bool v3) {
    out.start("Comment");
    out.attribute("Guid", comment.guid);
    out.text("Date", comment.date);
    out.text("Author", comment.author);
    if (v3) out.optional("Comment", comment.text);
    else out.text("Comment", comment.text); // required in 2.1
    if (!comment.viewpoint_guid.isEmpty()) {
        out.start("Viewpoint");
        out.attribute("Guid", comment.viewpoint_guid);
        out.end();
    }
    out.optional("ModifiedDate", comment.modified_date);
    out.optional("ModifiedAuthor", comment.modified_author);
    out.end();
}

void writeViewpointEntry(Writer& out, const Viewpoint& viewpoint, const QString& element) {
    out.start(element);
    out.attribute("Guid", viewpoint.guid);
    out.optional("Viewpoint", viewpoint.viewpoint_file);
    out.optional("Snapshot", viewpoint.snapshot_file);
    if (viewpoint.index) out.text("Index", QString::number(*viewpoint.index));
    out.end();
}

QByteArray writeMarkup(const Topic& topic, bool v3) {
    Writer out;
    out.start("Markup");
    if (!topic.header_files.empty()) {
        out.start("Header");
        if (v3) out.start("Files");
        for (const HeaderFile& file : topic.header_files) {
            out.start("File");
            if (!file.ifc_project.isEmpty()) out.attribute("IfcProject", file.ifc_project);
            if (!file.ifc_spatial_structure_element.isEmpty()) {
                out.attribute("IfcSpatialStructureElement", file.ifc_spatial_structure_element);
            }
            out.attribute(v3 ? "IsExternal" : "isExternal", file.is_external ? "true" : "false");
            out.optional("Filename", file.filename);
            out.optional("Date", file.date);
            out.optional("Reference", file.reference);
            out.end();
        }
        if (v3) out.end();
        out.end();
    }
    out.start("Topic");
    out.attribute("Guid", topic.guid);
    if (v3 && !topic.server_assigned_id.isEmpty()) out.attribute("ServerAssignedId", topic.server_assigned_id);
    // Required (non-empty) in 3.0.
    const QString type = topic.topic_type.isEmpty() && v3 ? QStringLiteral("Issue") : topic.topic_type;
    const QString status = topic.topic_status.isEmpty() && v3 ? QStringLiteral("Open") : topic.topic_status;
    if (!type.isEmpty()) out.attribute("TopicType", type);
    if (!status.isEmpty()) out.attribute("TopicStatus", status);
    if (v3 && !topic.reference_links.isEmpty()) {
        out.start("ReferenceLinks");
        for (const QString& link : topic.reference_links) out.text("ReferenceLink", link);
        out.end();
    } else if (!v3) {
        for (const QString& link : topic.reference_links) out.text("ReferenceLink", link);
    }
    out.text("Title", topic.title);
    out.optional("Priority", topic.priority);
    if (topic.index) out.text("Index", QString::number(*topic.index));
    if (v3 && !topic.labels.isEmpty()) {
        out.start("Labels");
        for (const QString& label : topic.labels) out.text("Label", label);
        out.end();
    } else if (!v3) {
        for (const QString& label : topic.labels) out.text("Labels", label);
    }
    out.text("CreationDate", topic.creation_date);
    out.text("CreationAuthor", topic.creation_author);
    out.optional("ModifiedDate", topic.modified_date);
    out.optional("ModifiedAuthor", topic.modified_author);
    out.optional("DueDate", topic.due_date);
    out.optional("AssignedTo", topic.assigned_to);
    out.optional("Stage", topic.stage);
    out.optional("Description", topic.description);
    if (topic.bim_snippet) {
        const BimSnippet& snippet = *topic.bim_snippet;
        out.start("BimSnippet");
        out.attribute("SnippetType", snippet.snippet_type);
        if (snippet.is_external) out.attribute(v3 ? "IsExternal" : "isExternal", "true");
        out.text("Reference", snippet.reference);
        out.text("ReferenceSchema", snippet.reference_schema);
        out.end();
    }
    if (v3) {
        if (!topic.document_references.empty()) {
            out.start("DocumentReferences");
            for (const DocumentReference& reference : topic.document_references) {
                out.start("DocumentReference");
                out.attribute("Guid", reference.guid.isEmpty() ? newGuid() : reference.guid);
                if (!reference.document_guid.isEmpty()) out.text("DocumentGuid", reference.document_guid);
                else out.optional("Url", reference.url);
                out.optional("Description", reference.description);
                out.end();
            }
            out.end();
        }
        if (!topic.related_topics.isEmpty()) {
            out.start("RelatedTopics");
            for (const QString& guid : topic.related_topics) {
                out.start("RelatedTopic");
                out.attribute("Guid", guid);
                out.end();
            }
            out.end();
        }
        if (!topic.comments.empty()) {
            out.start("Comments");
            for (const Comment& comment : topic.comments) writeComment(out, comment, true);
            out.end();
        }
        if (!topic.viewpoints.empty()) {
            out.start("Viewpoints");
            for (const Viewpoint& viewpoint : topic.viewpoints) writeViewpointEntry(out, viewpoint, "ViewPoint");
            out.end();
        }
        out.end(); // Topic
    } else {
        for (const DocumentReference& reference : topic.document_references) {
            out.start("DocumentReference");
            if (!reference.guid.isEmpty()) out.attribute("Guid", reference.guid);
            // An internal 3.0 document is written in the documents folder of the archive.
            const bool internal = !reference.document_guid.isEmpty();
            if (reference.is_external && !internal) out.attribute("isExternal", "true");
            out.text("ReferencedDocument",
                     internal ? QString("../documents/%1").arg(reference.document_guid) : reference.url);
            out.optional("Description", reference.description);
            out.end();
        }
        for (const QString& guid : topic.related_topics) {
            out.start("RelatedTopic");
            out.attribute("Guid", guid);
            out.end();
        }
        out.end(); // Topic
        for (const Comment& comment : topic.comments) writeComment(out, comment, false);
        for (const Viewpoint& viewpoint : topic.viewpoints) writeViewpointEntry(out, viewpoint, "Viewpoints");
    }
    out.end(); // Markup
    return out.finish();
}

QByteArray writeExtensions(const Extensions& extensions) {
    Writer out;
    out.start("Extensions");
    const auto list = [&out](const QString& wrapper, const QString& item, const QStringList& values) {
        if (values.isEmpty()) return;
        out.start(wrapper);
        for (const QString& value : values) out.text(item, value);
        out.end();
    };
    list("TopicTypes", "TopicType", extensions.topic_types);
    list("TopicStatuses", "TopicStatus", extensions.topic_statuses);
    list("Priorities", "Priority", extensions.priorities);
    list("TopicLabels", "TopicLabel", extensions.topic_labels);
    list("Users", "User", extensions.users);
    list("SnippetTypes", "SnippetType", extensions.snippet_types);
    list("Stages", "Stage", extensions.stages);
    out.end();
    return out.finish();
}

// 2.1: the extension values as a schema redefining the markup types.
QByteArray writeExtensionSchema(const Extensions& extensions) {
    Writer out;
    out.xml().writeDefaultNamespace("http://www.w3.org/2001/XMLSchema");
    out.start("schema");
    out.start("redefine");
    out.attribute("schemaLocation", "markup.xsd");
    const auto type = [&out](const QString& name, const QStringList& values) {
        out.start("simpleType");
        out.attribute("name", name);
        out.start("restriction");
        out.attribute("base", name);
        for (const QString& value : values) {
            out.start("enumeration");
            out.attribute("value", value);
            out.end();
        }
        out.end();
        out.end();
    };
    type("TopicType", extensions.topic_types);
    type("TopicStatus", extensions.topic_statuses);
    type("TopicLabel", extensions.topic_labels);
    type("SnippetType", extensions.snippet_types);
    type("Priority", extensions.priorities);
    type("UserIdType", extensions.users);
    type("Stage", extensions.stages);
    out.end();
    out.end();
    return out.finish();
}

const QRegularExpression& guidFolder() {
    static const QRegularExpression pattern(
        "^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$");
    return pattern;
}

} // namespace

// ------------------------------------------------------------------ API

std::optional<VisualizationInfo> parseVisualizationInfo(const QByteArray& xml, QString* error) {
    const auto root = parse(xml, error);
    if (!root) return std::nullopt;
    return readVisualizationInfo(*root);
}

QByteArray writeVisualizationInfo(const VisualizationInfo& info, const QString& version) {
    return writeInfo(info, version.startsWith('3'));
}

std::optional<BcfFile> fromEntries(const std::map<QString, QByteArray>& entries, QString* error) {
    BcfFile file;
    QSet<QString> used;
    const auto read = [&](const QString& name) -> std::optional<Node> {
        const auto found = entries.find(name);
        if (found == entries.end()) return std::nullopt;
        used.insert(name);
        QString parse_error;
        auto node = parse(found->second, &parse_error);
        if (!node && error) *error = QString("%1: %2").arg(name, parse_error);
        return node;
    };

    const auto version = read("bcf.version");
    if (!version) {
        if (error && error->isEmpty()) *error = QStringLiteral("not a BCF file: bcf.version is missing");
        return std::nullopt;
    }
    const QString version_id = version->attribute("VersionId");
    file.version = version_id.startsWith('3') ? QStringLiteral("3.0") : QStringLiteral("2.1");
    const bool v3 = file.version == QLatin1String("3.0");

    if (const auto project = read("project.bcfp")) {
        if (const Node* node = project->child("Project")) {
            file.project = Project{node->attribute("ProjectId"), node->childText("Name")};
        }
        const QString schema = project->childText("ExtensionSchema");
        if (!v3 && !schema.isEmpty()) {
            if (const auto extensions = read(schema)) file.extensions = readExtensionSchema(*extensions);
        }
    }
    if (v3) {
        if (const auto extensions = read("extensions.xml")) file.extensions = readExtensions(*extensions);
        if (const auto documents = read("documents.xml")) {
            if (const Node* list = documents->child("Documents")) {
                for (const Node* node : list->all("Document")) {
                    Document document;
                    document.guid = node->attribute("Guid");
                    document.filename = node->childText("Filename");
                    document.description = node->childText("Description");
                    const QString path = QString("documents/%1").arg(document.guid);
                    if (const auto data = entries.find(path); data != entries.end()) {
                        document.data = data->second;
                        used.insert(path);
                    }
                    file.documents.push_back(std::move(document));
                }
            }
        }
    }

    for (const auto& [name, data] : entries) {
        const QStringList parts = name.split('/');
        if (parts.size() != 2 || parts[1] != QLatin1String("markup.bcf")) continue;
        const auto markup = read(name);
        if (!markup) return std::nullopt;
        Topic topic = readMarkup(*markup);
        const QString folder = parts[0];
        if (topic.guid.isEmpty()) topic.guid = folder;
        for (Viewpoint& viewpoint : topic.viewpoints) {
            if (!viewpoint.viewpoint_file.isEmpty()) {
                if (const auto info = read(folder + '/' + viewpoint.viewpoint_file)) {
                    viewpoint.info = readVisualizationInfo(*info);
                }
            }
            if (!viewpoint.snapshot_file.isEmpty()) {
                const QString path = folder + '/' + viewpoint.snapshot_file;
                if (const auto snapshot = entries.find(path); snapshot != entries.end()) {
                    viewpoint.snapshot = snapshot->second;
                    used.insert(path);
                }
            }
        }
        // The topic folder may differ from the topic guid: keep its other files with it.
        if (folder != topic.guid) {
            for (const auto& [other, bytes] : entries) {
                if (!used.contains(other) && other.startsWith(folder + '/')) {
                    file.other_files[topic.guid + other.mid(folder.size())] = bytes;
                    used.insert(other);
                }
            }
        }
        file.topics.push_back(std::move(topic));
    }
    for (const auto& [name, data] : entries) {
        if (!used.contains(name)) file.other_files[name] = data;
    }
    return file;
}

std::map<QString, QByteArray> toEntries(const BcfFile& file, const QString& version) {
    const bool v3 = version.startsWith('3');
    std::map<QString, QByteArray> entries;
    QSet<QString> topic_guids;
    for (const Topic& topic : file.topics) topic_guids.insert(topic.guid);

    // The files kept as they were, except those of deleted topics.
    for (const auto& [name, data] : file.other_files) {
        const QString folder = name.section('/', 0, 0);
        if (name.contains('/') && guidFolder().match(folder).hasMatch() && !topic_guids.contains(folder)) continue;
        entries[name] = data;
    }
    {
        Writer out;
        out.start("Version");
        out.attribute("VersionId", v3 ? "3.0" : "2.1");
        if (!v3) out.text("DetailedVersion", "2.1");
        out.end();
        entries["bcf.version"] = out.finish();
    }
    {
        Writer out;
        out.start(v3 ? "ProjectInfo" : "ProjectExtension");
        const Project project = file.project.value_or(Project{newGuid(), QString()});
        out.start("Project");
        out.attribute("ProjectId", project.project_id.isEmpty() ? newGuid() : project.project_id);
        out.optional("Name", project.name);
        out.end();
        if (!v3) out.text("ExtensionSchema", "extensions.xsd");
        out.end();
        entries["project.bcfp"] = out.finish();
    }
    if (v3) {
        entries["extensions.xml"] = writeExtensions(file.extensions);
        if (!file.documents.empty()) {
            Writer out;
            out.start("DocumentInfo");
            out.start("Documents");
            for (const Document& document : file.documents) {
                out.start("Document");
                out.attribute("Guid", document.guid);
                out.text("Filename", document.filename);
                out.optional("Description", document.description);
                out.end();
            }
            out.end();
            out.end();
            entries["documents.xml"] = out.finish();
        }
    } else {
        entries["extensions.xsd"] = writeExtensionSchema(file.extensions);
    }
    for (const Document& document : file.documents) entries["documents/" + document.guid] = document.data;

    for (const Topic& topic : file.topics) {
        Topic written = topic;
        for (Viewpoint& viewpoint : written.viewpoints) {
            if (viewpoint.guid.isEmpty()) viewpoint.guid = newGuid();
            if (viewpoint.info && viewpoint.viewpoint_file.isEmpty()) viewpoint.viewpoint_file = viewpoint.guid + ".bcfv";
            if (!viewpoint.snapshot.isEmpty() && viewpoint.snapshot_file.isEmpty()) {
                viewpoint.snapshot_file = viewpoint.guid + ".png";
            }
            if (viewpoint.info) {
                VisualizationInfo info = *viewpoint.info;
                if (info.guid.isEmpty()) info.guid = viewpoint.guid;
                entries[topic.guid + '/' + viewpoint.viewpoint_file] = writeInfo(info, v3);
            }
            if (!viewpoint.snapshot.isEmpty()) entries[topic.guid + '/' + viewpoint.snapshot_file] = viewpoint.snapshot;
        }
        entries[topic.guid + "/markup.bcf"] = writeMarkup(written, v3);
    }
    return entries;
}

std::optional<BcfFile> load(const QString& path, QString* error) {
    const auto entries = zip::read(path, error);
    if (!entries) return std::nullopt;
    return fromEntries(*entries, error);
}

bool save(const BcfFile& file, const QString& path, const QString& version, QString* error) {
    return zip::write(path, toEntries(file, version), error);
}

} // namespace bonsaiviewer::ports::bcf
