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
 * You should have received a copy of the GNU General Public License            *
 * along with this program. If not, see <http://www.gnu.org/licenses/>.         *
 *                                                                              *
 ********************************************************************************/

#include "RulePanels.h"

#include "Editors.h"

#include "../../services/clash/RuleCatalogue.h"
#include "../../services/clash/RuleJson.h"
#include "../../services/IdsBridge.h"
#include "../filters/SelectionEditor.h"

#include <QFormLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QTimer>
#include <QVBoxLayout>

namespace bonsaiviewer::modules::rule {

namespace {

constexpr int kPayloadRole = Qt::UserRole + 1;

QString facetHelp(const QString& name) {
    if (name == QLatin1String("Entity")) return "The IFC class (and predefined type) of the elements.";
    if (name == QLatin1String("Attribute")) return "An attribute of the elements (Name, Description, Tag...).";
    if (name == QLatin1String("Property")) return "A property of a property set or quantity set.";
    if (name == QLatin1String("Classification")) return "A classification reference (system and code).";
    if (name == QLatin1String("PartOf")) return "The element is part of another one (aggregate, container, group...).";
    return "The material of the elements.";
}

} // namespace

// --- RuleLibraryPanel ------------------------------------------------------------------

RuleLibraryPanel::RuleLibraryPanel(QWidget* parent) : QTreeWidget(parent) {
    setHeaderHidden(true);
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragOnly);
    setSelectionMode(QAbstractItemView::SingleSelection);
    const auto section = [this](const QString& title, const QString& tip) {
        auto* item = new QTreeWidgetItem(this);
        item->setText(0, title);
        item->setToolTip(0, tip);
        QFont bold = item->font(0);
        bold.setBold(true);
        item->setFont(0, bold);
        item->setFlags(Qt::ItemIsEnabled);
        return item;
    };
    const std::vector<std::pair<QString, QString>> families{
        {"OneObjectRule", "One object: each element of the source is checked alone (volume, area, OBB size...)."},
        {"TwoObjectRule", "Two objects: each pair source / target is checked (intersection, clearance, above...)."},
        {"ComplexRule", "Complex: the elements of the source are checked together (alignment)."}};
    for (const auto& [family, tip] : families) {
        auto* parent_item = section(family == "OneObjectRule" ? "One Object Rules" : family == "TwoObjectRule" ? "Two Object Rules" : "Complex Rules", tip);
        for (const clash::RuleSpec& spec : clash::RuleCatalogue::instance().rules()) {
            if (spec.family != family) continue;
            auto* item = new QTreeWidgetItem(parent_item);
            item->setText(0, spec.label);
            item->setToolTip(0, QString("<b>%1</b><br>%2<br><i>Drag onto the Rule tree.</i>").arg(spec.label, spec.doc.toHtmlEscaped()));
            item->setData(0, kPayloadRole, "rule:" + spec.type);
            item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled);
        }
        parent_item->setExpanded(true);
    }
    auto* facets = section("IDS Facets", "The IDS facets select the elements of a slot: drop them on source, target or context.");
    for (const QString& name : facetTypeNames()) {
        auto* item = new QTreeWidgetItem(facets);
        item->setText(0, name == "PartOf" ? QStringLiteral("Part Of") : name);
        item->setToolTip(0, QString("<b>%1 facet</b><br>%2<br><i>Drag onto a slot of a rule.</i>").arg(name, facetHelp(name)));
        item->setData(0, kPayloadRole, "facet:" + name);
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled);
    }
    facets->setExpanded(true);
    connect(this, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem* item, QTreeWidgetItem*) {
        const QString payload = item ? item->data(0, kPayloadRole).toString() : QString();
        if (payload.startsWith("rule:")) emit ruleTypeSelected(payload.mid(5));
    });
}

QStringList RuleLibraryPanel::mimeTypes() const {
    return {kLibraryMime};
}

QMimeData* RuleLibraryPanel::mimeData(const QList<QTreeWidgetItem*>& items) const {
    auto* mime = new QMimeData;
    if (!items.isEmpty()) mime->setData(kLibraryMime, items.front()->data(0, kPayloadRole).toString().toUtf8());
    return mime;
}

// --- RuleConfigPanel ---------------------------------------------------------------------

namespace {
QString facetsKey(const clash::Select* select) {
    if (!select || select->kind != clash::Select::Kind::Facet) return {};
    QJsonArray array;
    for (const auto& facet : select->applicability) array.append(clash::json::toJson(facet));
    return QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact));
}
} // namespace

void RuleConfigPanel::setFacetContext(bonsaiviewer::services::IdsBridge* bridge, filters::IfcPathProvider ifc_path) {
    bridge_ = bridge;
    ifc_path_ = std::move(ifc_path);
}

bool RuleConfigPanel::showsSlot(const std::optional<TreeRef>& ref) const {
    if (!ref || !shown_ || !slot_editor_) return false;
    const auto slotKind = [](TreeRef::Kind kind) { return kind == TreeRef::Kind::Slot || kind == TreeRef::Kind::Facet; };
    if (!slotKind(ref->kind) || !slotKind(shown_->kind)) return false;
    if (ref->rule != shown_->rule || ref->slot != shown_->slot) return false;
    return facetsKey(ref->select()) == written_;
}

RuleConfigPanel::RuleConfigPanel(RuleDocument& document, QWidget* parent) : QWidget(parent), document_(document) {
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(4, 4, 4, 4);
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto* host = new QWidget(scroll);
    body_ = new QVBoxLayout(host);
    body_->setContentsMargins(0, 0, 0, 0);
    body_->addStretch(1);
    scroll->setWidget(host);
    outer->addWidget(scroll);
    clear();
    // A slot or facet editor points into the facet list of the slot: shown
    // again after a change of the document (a facet added, moved, removed).
    connect(&document_, &RuleDocument::changed, this, [this]() {
        if (!shown_ || (shown_->kind != TreeRef::Kind::Slot && shown_->kind != TreeRef::Kind::Facet)) return;
        // The IDS editor's own writes keep it as it is (no rebuild while typing).
        if (showsSlot(shown_)) return;
        QTimer::singleShot(0, this, [this]() { showRef(shown_); });
    });
    connect(&document_, &RuleDocument::opened, this, [this]() { showRef(std::nullopt); });
}

void RuleConfigPanel::clear() {
    if (content_) {
        content_->deleteLater();
        content_ = nullptr;
    }
    content_ = new QWidget(this);
    body_->insertWidget(0, content_);
}

void RuleConfigPanel::showLibraryRule(const QString& type) {
    shown_.reset();
    clear();
    const clash::RuleSpec* spec = clash::RuleCatalogue::instance().find(type);
    if (!spec) return;
    auto* layout = new QVBoxLayout(content_);
    layout->setContentsMargins(0, 0, 0, 0);
    auto* title = new QLabel(QString("<b>%1</b> — %2").arg(spec->label, clash::familyBadge(spec->family)), content_);
    auto* doc = new QLabel(spec->doc, content_);
    doc->setWordWrap(true);
    layout->addWidget(title);
    layout->addWidget(doc);
    QStringList params;
    for (const clash::ParamSpec& p : spec->params) {
        params << QString("<b>%1</b>%2: %3").arg(p.name, p.unit.isEmpty() ? QString() : " (" + p.unit + ")",
                                p.doc.toHtmlEscaped().replace('\n', QStringLiteral("<br>")));
    }
    auto* list = new QLabel(params.join("<br>"), content_);
    list->setWordWrap(true);
    layout->addWidget(list);
    auto* slots_label = new QLabel("Slots: " + spec->slot_names.join(", "), content_);
    slots_label->setProperty("textRole", "secondary");
    layout->addWidget(slots_label);
}

void RuleConfigPanel::showRef(const std::optional<TreeRef>& requested) {
    const std::optional<TreeRef> ref = requested; // `requested` may be shown_ itself
    if (showsSlot(ref)) {
        shown_ = ref; // the same slot (the tree rebuilt after an edit): the editor stays
        return;
    }
    shown_ = ref;
    slot_editor_.clear();
    written_.clear();
    clear();
    if (!ref) return;
    auto* layout = new QVBoxLayout(content_);
    layout->setContentsMargins(0, 0, 0, 0);
    const auto labelField = [this, layout](QString* label, QString* description) {
        auto* form = new QFormLayout();
        auto* name = new QLineEdit(*label, content_);
        auto* text = new QPlainTextEdit(*description, content_);
        text->setMaximumHeight(60);
        name->setToolTip("<b>Name</b><br>The name shown in the tree and in the results.");
        connect(name, &QLineEdit::editingFinished, this, [this, name, label]() {
            if (*label == name->text()) return;
            *label = name->text();
            document_.touch();
        });
        connect(text, &QPlainTextEdit::textChanged, this, [text, description]() { *description = text->toPlainText(); });
        form->addRow("Name", name);
        form->addRow("Description", text);
        layout->addLayout(form);
    };

    switch (ref->kind) {
    case TreeRef::Kind::File:
        layout->addWidget(new QLabel("<b>Rule File</b>", content_));
        labelField(&document_.file().viewer.label, &document_.file().viewer.description);
        break;
    case TreeRef::Kind::Folder: {
        layout->addWidget(new QLabel("<b>Folder</b>", content_));
        labelField(&ref->folder->viewer.label, &ref->folder->viewer.description);
        auto* edit = new QPushButton("Activation…", content_);
        edit->setToolTip("<b>Activation</b><br>The rule (or the facets) that must hold for the folder to run.");
        connect(edit, &QPushButton::clicked, this, [this, folder = ref->folder]() { emit editFolderRequested(folder); });
        layout->addWidget(edit);
        break;
    }
    case TreeRef::Kind::Rule: {
        const clash::RuleSpec* spec = clash::RuleCatalogue::instance().find(ref->rule->type);
        layout->addWidget(new QLabel(QString("<b>%1</b> — %2").arg(spec ? spec->label : ref->rule->type,
                                                                   spec ? clash::familyBadge(spec->family) : QString("unknown")),
                                     content_));
        if (spec) {
            auto* doc = new QLabel(spec->doc, content_);
            doc->setWordWrap(true);
            doc->setProperty("textRole", "secondary");
            layout->addWidget(doc);
        }
        labelField(&ref->rule->viewer.label, &ref->rule->viewer.description);
        auto* params = new ParamForm(content_);
        params->setRule(spec, &ref->rule->params);
        connect(params, &ParamForm::edited, &document_, &RuleDocument::touch);
        layout->addWidget(new QLabel("<b>Parameters</b>", content_));
        layout->addWidget(params);
        auto* edit = new QPushButton("Exceptions, Must, Criticity…", content_);
        edit->setToolTip("<b>Edit Rule</b><br>The exceptions (two-object rules), the Must check, the grouping, "
                         "the criticity and the actor of the results.");
        connect(edit, &QPushButton::clicked, this, [this, rule = ref->rule]() { emit editRuleRequested(rule); });
        layout->addWidget(edit);
        break;
    }
    case TreeRef::Kind::Slot:
    case TreeRef::Kind::Facet:
        showSlot(*ref, layout);
        break;
    case TreeRef::Kind::Link: {
        clash::Select* select = ref->select();
        const auto linked = select && select->rule ? document_.treeRule(select->rule->id) : nullptr;
        auto* text = new QLabel(QString("<b>Cascade</b><br>The slot <b>%1</b> takes the %2 of <b>%3</b>.")
                                    .arg(ref->slot,
                                         select && select->element_state_to_pass ? select->element_to_pass + "s of its results"
                                                                                : (select ? select->element_to_pass : QString()) +
                                                                                      "s without result",
                                         linked ? RuleDocument::labelOf(*linked) : QString("?")),
                                content_);
        text->setWordWrap(true);
        layout->addWidget(text);
        break;
    }
    }
}

void RuleConfigPanel::showSlot(const TreeRef& ref, QVBoxLayout* layout) {
    clash::Select* select = ref.select();
    layout->addWidget(new QLabel(QString("<b>%1 — %2</b>").arg(RuleDocument::labelOf(*ref.rule), ref.slot), content_));
    const auto hint = [this, layout](const QString& text) {
        auto* label = new QLabel(text, content_);
        label->setWordWrap(true);
        layout->addWidget(label);
    };
    if (!select) return;
    if (select->kind == clash::Select::Kind::Rule) {
        hint("The slot takes the elements of a rule (shown under the slot). Double click the rule to edit it.");
        return;
    }
    if (!bridge_) {
        hint(select->empty() ? "Empty slot." : QString("%1 facet(s).").arg(select->applicability.size()));
        return;
    }
    if (select->empty()) {
        hint("Empty slot: drop IDS facets from the Rule Library or a rule of the tree on it, or start an IDS "
             "selection here.");
        auto* start = new QPushButton("Select with IDS facets", content_);
        start->setToolTip("<b>Select with IDS facets</b><br>Fill the slot with an IDS selection (an entity facet "
                          "to start from), edited here as in the IDS panel.");
        connect(start, &QPushButton::clicked, this, [this, r = ref]() {
            clash::Select* target = r.select();
            if (!target) return;
            QString error;
            auto facets = engineFacets(filters::SelectionEditor::defaultFields(), nullptr, &error);
            if (facets.empty()) return;
            target->kind = clash::Select::Kind::Facet;
            target->applicability = std::move(facets);
            document_.touch(); // shown again with the editor
        });
        layout->addWidget(start, 0, Qt::AlignLeft);
        return;
    }
    // The IDS selection editor of the filters / IDS panel, with its preview.
    auto* editor = new filters::SelectionEditor(*bridge_, ifc_path_, content_);
    editor->setFields(idsFields(select->applicability));
    auto* status = new QLabel(content_);
    status->setWordWrap(true);
    status->setProperty("textRole", "secondary");
    status->hide();
    slot_editor_ = editor;
    written_ = facetsKey(select);
    connect(editor, &filters::SelectionEditor::isolateRequested, this, &RuleConfigPanel::isolateRequested);
    connect(editor, &filters::SelectionEditor::changed, this, [this, editor, status, r = ref]() {
        clash::Select* target = r.select();
        if (!target) return;
        const QString path = ifc_path_ ? ifc_path_() : QString();
        ifcopenshell::file* file = path.isEmpty() ? nullptr : bridge_->file(path);
        QString error;
        auto facets = engineFacets(editor->fields(), file, &error);
        if (!error.isEmpty()) {
            // Not written until it can be built (a class being typed...).
            status->setText("Not applied yet: " + error);
            status->show();
            return;
        }
        status->hide();
        if (facets.empty()) {
            *target = clash::Select{}; // all the facets removed: the slot is empty
        } else {
            target->kind = clash::Select::Kind::Facet;
            target->applicability = std::move(facets);
        }
        written_ = facetsKey(target);
        if (target->empty()) written_ = QStringLiteral("-"); // shown again (the empty-slot page)
        document_.touch();
    });
    layout->addWidget(editor, 1);
    layout->addWidget(status);
}

} // namespace bonsaiviewer::modules::rule
