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

#include <QComboBox>
#include <QFormLayout>
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
        params << QString("<b>%1</b>%2: %3").arg(p.name, p.unit.isEmpty() ? QString() : " (" + p.unit + ")", p.doc.toHtmlEscaped());
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
    shown_ = ref;
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
        auto* state = new QComboBox(content_);
        state->addItems({"Final", "Select"});
        state->setCurrentText(ref->rule->state == QLatin1String("Select") ? "Select" : "Final");
        state->setToolTip("<b>State</b><br>Final: the results are managed (criticity, actor, grouping or Must). "
                          "Select: the rule only produces a selection for the rules it feeds.");
        connect(state, &QComboBox::currentTextChanged, this, [this, rule = ref->rule](const QString& text) {
            rule->state = text;
            document_.touch();
        });
        auto* state_form = new QFormLayout();
        state_form->addRow("State", state);
        layout->addLayout(state_form);
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
    case TreeRef::Kind::Facet: {
        clash::Select* select = ref->select();
        layout->addWidget(new QLabel(QString("<b>%1 — %2</b>").arg(RuleDocument::labelOf(*ref->rule), ref->slot), content_));
        if (!select || select->kind != clash::Select::Kind::Facet) {
            auto* hint = new QLabel("Drop IDS facets from the Rule Library on the slot, or a rule of the tree to "
                                    "take its elements (cascade).", content_);
            hint->setWordWrap(true);
            layout->addWidget(hint);
            break;
        }
        if (ref->kind == TreeRef::Kind::Facet && ref->facet >= 0 && ref->facet < static_cast<int>(select->applicability.size())) {
            auto* editor = new FacetEditor(content_);
            editor->setFacet(&select->applicability[static_cast<size_t>(ref->facet)]);
            connect(editor, &FacetEditor::edited, &document_, &RuleDocument::touch);
            layout->addWidget(editor);
        } else {
            auto* editor = new FacetListEditor(content_);
            editor->setFacets(&select->applicability);
            connect(editor, &FacetListEditor::edited, &document_, &RuleDocument::touch);
            layout->addWidget(editor);
        }
        break;
    }
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

} // namespace bonsaiviewer::modules::rule
