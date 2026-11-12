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

#include "RuleEditDialog.h"

#include "Editors.h"
#include "RuleDocument.h"
#include "SlotClipboard.h"

#include "../../services/clash/RuleCatalogue.h"
#include "../../services/clash/Orchestrator.h"
#include "../../services/clash/RuleJson.h"
#include "../../services/IdsBridge.h"
#include "../../services/IdsEngine.h"
#include "../filters/SelectionEditor.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QRadioButton>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <QScrollArea>
#include <QStackedWidget>
#include <QStandardItemModel>
#include <QTabBar>
#include <QTabWidget>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <functional>
#include <tuple>

namespace bonsaiviewer::modules::rule {

namespace {

std::shared_ptr<clash::BooleanNode> cloneTree(const clash::BooleanNode* node) {
    return node ? clash::json::booleanFromJson(clash::json::toJson(node)) : nullptr;
}

QWidget* scrolled(QWidget* content) {
    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidget(content);
    return scroll;
}

QLabel* helpLabel(const QString& text, QWidget* parent) {
    auto* label = new QLabel(text, parent);
    label->setWordWrap(true);
    label->setProperty("textRole", "secondary");
    return label;
}

QString nodeText(const clash::BooleanNode& node) {
    switch (node.kind) {
    case clash::BooleanNode::Kind::And: return QStringLiteral("AND — every child holds");
    case clash::BooleanNode::Kind::Or: return QStringLiteral("OR — at least one child holds");
    case clash::BooleanNode::Kind::Not: return QStringLiteral("NOT — the child does not hold");
    case clash::BooleanNode::Kind::Leaf: break;
    }
    QString text = node.rule ? RuleDocument::labelOf(*node.rule) : QStringLiteral("(no rule)");
    text += " — " + node.mode;
    if (!node.value.isEmpty()) text += " " + node.value;
    return text;
}

// The labelled sets of the criticity or the actor: a label and the IDS
// facets of the elements it applies to.
class LabelSetsEditor : public QWidget {
public:
    LabelSetsEditor(std::vector<clash::Select>* sets, const QString& what, QWidget* parent) : QWidget(parent), sets_(sets) {
        auto* layout = new QVBoxLayout(this);
        layout->addWidget(helpLabel(QString("Each %1 is a label given to the results whose source or target matches "
                                            "its IDS facets.").arg(what), this));
        list_ = new QListWidget(this);
        list_->setMaximumHeight(100);
        auto* buttons = new QHBoxLayout();
        auto* add = new QPushButton("Add " + what, this);
        auto* remove = new QPushButton("Remove", this);
        add->setToolTip(QString("<b>Add %1</b><br>A new label and its facets.").arg(what));
        buttons->addWidget(add);
        buttons->addWidget(remove);
        buttons->addStretch(1);
        auto* form = new QFormLayout();
        label_ = new QLineEdit(this);
        label_->setToolTip("<b>Label</b><br>The text given to the matching results.");
        form->addRow("Label", label_);
        facets_ = new FacetListEditor(this);
        layout->addWidget(list_);
        layout->addLayout(buttons);
        layout->addLayout(form);
        layout->addWidget(facets_);
        layout->addStretch(1);
        connect(add, &QPushButton::clicked, this, [this, what]() {
            clash::Select select;
            select.kind = clash::Select::Kind::Facet;
            select.classification_type = what;
            select.classification_name = what + " " + QString::number(sets_->size() + 1);
            select.applicability.push_back(it::Facet::make(it::FacetType::Entity));
            sets_->push_back(select);
            refresh(static_cast<int>(sets_->size()) - 1);
        });
        connect(remove, &QPushButton::clicked, this, [this]() {
            const int row = list_->currentRow();
            if (row < 0 || row >= static_cast<int>(sets_->size())) return;
            sets_->erase(sets_->begin() + row);
            refresh(std::min(row, static_cast<int>(sets_->size()) - 1));
        });
        connect(list_, &QListWidget::currentRowChanged, this, [this](int row) { select(row); });
        connect(label_, &QLineEdit::textEdited, this, [this](const QString& text) {
            const int row = list_->currentRow();
            if (row < 0 || row >= static_cast<int>(sets_->size())) return;
            (*sets_)[static_cast<size_t>(row)].classification_name = text;
            list_->item(row)->setText(text);
        });
        refresh(sets_->empty() ? -1 : 0);
    }

private:
    void refresh(int current) {
        const QSignalBlocker blocker(list_);
        list_->clear();
        for (const clash::Select& s : *sets_) list_->addItem(s.classification_name);
        list_->setCurrentRow(current);
        select(current);
    }
    void select(int row) {
        const bool valid = row >= 0 && row < static_cast<int>(sets_->size());
        label_->setEnabled(valid);
        label_->setText(valid ? (*sets_)[static_cast<size_t>(row)].classification_name : QString());
        facets_->setEnabled(valid);
        facets_->setFacets(valid ? &(*sets_)[static_cast<size_t>(row)].applicability : nullptr);
    }

    std::vector<clash::Select>* sets_;
    QListWidget* list_ = nullptr;
    QLineEdit* label_ = nullptr;
    FacetListEditor* facets_ = nullptr;
};

} // namespace

// --- BooleanEditor ----------------------------------------------------------------------

BooleanEditor::BooleanEditor(bool leaves_have_selections, QWidget* parent)
    : QWidget(parent), with_selections_(leaves_have_selections) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    tree_ = new QTreeWidget(this);
    tree_->setHeaderHidden(true);
    tree_->setMinimumHeight(120);
    auto* buttons = new QHBoxLayout();
    const auto composite = [this](const QString& text, clash::BooleanNode::Kind kind, const QString& tip) {
        auto* b = new QPushButton(text, this);
        b->setToolTip(QString("<b>%1</b><br>%2").arg(text, tip));
        connect(b, &QPushButton::clicked, this, [this, kind]() {
            auto node = std::make_shared<clash::BooleanNode>();
            node->kind = kind;
            add(node);
        });
        return b;
    };
    auto* add_rule = new QPushButton("Add Rule", this);
    add_rule->setToolTip("<b>Add Rule</b><br>A rule seen as a boolean, by its number of results "
                         "(have_result, have_no_result, have_more...).");
    auto* menu = new QMenu(add_rule);
    menu->setToolTipsVisible(true);
    for (const clash::RuleSpec& spec : clash::RuleCatalogue::instance().rules()) {
        // An exception runs its rule on a pair: a one-object or two-object rule.
        if (!with_selections_ && spec.family == QLatin1String("ComplexRule")) continue;
        QAction* action = menu->addAction(spec.label, this, [this, type = spec.type]() {
            auto node = std::make_shared<clash::BooleanNode>();
            node->kind = clash::BooleanNode::Kind::Leaf;
            node->rule = std::make_shared<clash::RuleCheck>();
            node->rule->type = type;
            node->rule->id = clash::json::newId();
            if (const auto* s = clash::RuleCatalogue::instance().find(type)) {
                for (const clash::ParamSpec& p : s->params) node->rule->params[p.name] = p.default_value;
                node->rule->viewer.label = s->label;
            }
            add(node);
        });
        action->setToolTip(spec.doc);
    }
    add_rule->setMenu(menu);
    auto* remove = new QPushButton("Remove", this);
    remove->setToolTip("<b>Remove</b><br>Remove the selected node and its children.");
    buttons->addWidget(composite("AND", clash::BooleanNode::Kind::And, "Holds when every child holds."));
    buttons->addWidget(composite("OR", clash::BooleanNode::Kind::Or, "Holds when one child holds."));
    buttons->addWidget(composite("NOT", clash::BooleanNode::Kind::Not, "Holds when its child does not."));
    buttons->addWidget(add_rule);
    buttons->addWidget(remove);
    buttons->addStretch(1);
    node_editor_ = new QWidget(this);
    node_layout_ = new QVBoxLayout(node_editor_);
    node_layout_->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(tree_);
    layout->addLayout(buttons);
    layout->addWidget(node_editor_);
    connect(tree_, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem* item, QTreeWidgetItem*) {
        showNode(nodes_.value(item, nullptr));
    });
    connect(remove, &QPushButton::clicked, this, [this]() {
        clash::BooleanNode* node = selected();
        if (!node) return;
        if (node == root_.get()) {
            root_.reset();
        } else if (clash::BooleanNode* parent = parentOf(node, root_.get())) {
            auto& children = parent->children;
            children.erase(std::remove_if(children.begin(), children.end(), [node](const auto& c) { return c.get() == node; }),
                           children.end());
        }
        rebuild(nullptr);
    });
}

void BooleanEditor::setRoot(std::shared_ptr<clash::BooleanNode> root) {
    root_ = std::move(root);
    rebuild(root_.get());
}

clash::BooleanNode* BooleanEditor::selected() const {
    return nodes_.value(tree_->currentItem(), nullptr);
}

clash::BooleanNode* BooleanEditor::parentOf(clash::BooleanNode* node, clash::BooleanNode* within) const {
    if (!within) return nullptr;
    for (const auto& child : within->children) {
        if (child.get() == node) return within;
        if (auto* found = parentOf(node, child.get())) return found;
    }
    return nullptr;
}

void BooleanEditor::add(std::shared_ptr<clash::BooleanNode> node) {
    clash::BooleanNode* raw = node.get();
    clash::BooleanNode* at = selected();
    if (!root_) {
        root_ = node;
    } else if (at && at->kind != clash::BooleanNode::Kind::Leaf &&
               !(at->kind == clash::BooleanNode::Kind::Not && !at->children.empty())) {
        at->children.push_back(node);
    } else {
        // The root (or the selection) becomes the first child of a new AND.
        auto wrapper = std::make_shared<clash::BooleanNode>();
        wrapper->kind = clash::BooleanNode::Kind::And;
        wrapper->children = {root_, node};
        root_ = wrapper;
    }
    rebuild(raw);
}

void BooleanEditor::rebuild(clash::BooleanNode* current) {
    const QSignalBlocker blocker(tree_);
    tree_->clear();
    nodes_.clear();
    QTreeWidgetItem* current_item = nullptr;
    std::function<void(QTreeWidgetItem*, clash::BooleanNode*)> addItem = [&](QTreeWidgetItem* parent, clash::BooleanNode* node) {
        auto* item = parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem(tree_);
        item->setText(0, nodeText(*node));
        nodes_.insert(item, node);
        if (node == current) current_item = item;
        for (const auto& child : node->children) addItem(item, child.get());
    };
    if (root_) addItem(nullptr, root_.get());
    tree_->expandAll();
    if (current_item) tree_->setCurrentItem(current_item);
    showNode(current_item ? current : nullptr);
}

void BooleanEditor::showNode(clash::BooleanNode* node) {
    while (QLayoutItem* item = node_layout_->takeAt(0)) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }
    if (!node || node->kind != clash::BooleanNode::Kind::Leaf || !node->rule) return;
    auto* box = new QGroupBox(RuleDocument::labelOf(*node->rule), node_editor_);
    auto* layout = new QVBoxLayout(box);
    auto* form = new QFormLayout();
    auto* mode = new QComboBox(box);
    mode->addItems({"have_result", "have_no_result", "have_more", "have_more_or_equals", "have_less",
                    "have_less_or_equals", "equals"});
    mode->setCurrentText(node->mode);
    mode->setToolTip("<b>Mode</b><br>How the number of results of the rule becomes a boolean.");
    auto* value = new QLineEdit(node->value, box);
    value->setToolTip("<b>Value</b><br>The number the quantity modes compare with.");
    form->addRow("Mode", mode);
    form->addRow("Value", value);
    layout->addLayout(form);
    auto* params = new ParamForm(box);
    params->setRule(clash::RuleCatalogue::instance().find(node->rule->type), &node->rule->params);
    layout->addWidget(params);
    const auto refreshText = [this, node]() {
        for (auto i = nodes_.begin(); i != nodes_.end(); ++i) {
            if (i.value() == node) i.key()->setText(0, nodeText(*node));
        }
    };
    connect(mode, &QComboBox::currentTextChanged, this, [node, refreshText](const QString& text) {
        node->mode = text;
        refreshText();
    });
    connect(value, &QLineEdit::textEdited, this, [node, refreshText](const QString& text) {
        node->value = text.trimmed();
        refreshText();
    });
    if (with_selections_) {
        // A folder activation runs the rule on its own elements.
        const auto* spec = clash::RuleCatalogue::instance().find(node->rule->type);
        for (const QString& slot : spec ? spec->slot_names : QStringList{}) {
            clash::Select* select = slot == "source" ? &node->rule->source : slot == "target" ? &node->rule->target : &node->rule->context;
            if (select->kind != clash::Select::Kind::Facet) {
                *select = clash::Select{};
                select->kind = clash::Select::Kind::Facet;
            }
            auto* slot_box = new QGroupBox(slot + " elements", box);
            auto* slot_layout = new QVBoxLayout(slot_box);
            auto* facets = new FacetListEditor(slot_box);
            facets->setFacets(&select->applicability);
            slot_layout->addWidget(facets);
            layout->addWidget(slot_box);
        }
    } else {
        layout->addWidget(helpLabel("The rule runs on the source and the target of each result.", box));
    }
    node_layout_->addWidget(box);
}

// --- RuleEditDialog ---------------------------------------------------------------------------

RuleEditDialog::RuleEditDialog(std::shared_ptr<clash::RuleCheck> rule, RuleDocument* document, FacetContext context,
                               QWidget* parent)
    : QDialog(parent), rule_(std::move(rule)), copy_(clash::json::cloneRule(*rule_)), document_(document),
      context_(std::move(context)) {
    const clash::RuleSpec* spec = clash::RuleCatalogue::instance().find(copy_->type);
    const bool two = spec && spec->isTwoObject();
    setWindowTitle("Edit Rule — " + RuleDocument::labelOf(*copy_));
    resize(760, 720);
    auto* layout = new QVBoxLayout(this);
    auto* tabs = new QTabWidget(this);
    layout->addWidget(tabs);

    // Title, description, state and parameters (as the Rule Config panel).
    tabs->addTab(scrolled(buildRulePage(two)), "Rule");
    // Source / target / context: the elements the rule checks.
    if (spec && !spec->slot_names.isEmpty()) tabs->addTab(scrolled(buildSelectionPage(spec->slot_names, two)), "Selection");

    // Exception (two-object rules only: spec 09 "Exceptions: scope").
    auto* exception_page = new QWidget;
    auto* exception_layout = new QVBoxLayout(exception_page);
    if (!two) {
        exception_layout->addWidget(helpLabel("Exceptions apply to the pairs of a two-object rule only: chain a "
                                              "one-object rule with another rule instead.", exception_page));
    } else {
        exception_layout->addWidget(helpLabel("An exception discards the special cases: a pair it applies to is "
                                              "invalidated (after the run) or left out (before).", exception_page));
        auto* kind = new QComboBox(exception_page);
        kind->addItems({"No exception", "Relation", "Rule tree"});
        kind->setToolTip("<b>Exception</b><br>A relation between the two elements, or a boolean tree of rules run "
                         "on the pair.");
        auto* stack = new QStackedWidget(exception_page);
        stack->addWidget(new QWidget);
        auto* relation = new QComboBox;
        relation->addItems({"Same_IfcSystem", "Same_Building", "Same_Storey", "Same_Space"});
        relation->setToolTip("<b>Relation</b><br>The pair is excepted when both elements share it.");
        auto* relation_page = new QWidget;
        auto* relation_form = new QFormLayout(relation_page);
        relation_form->addRow("Relation", relation);
        stack->addWidget(relation_page);
        exception_tree_ = new BooleanEditor(false);
        stack->addWidget(exception_tree_);
        auto* timing_box = new QGroupBox("Timing", exception_page);
        auto* timing_layout = new QHBoxLayout(timing_box);
        auto* after = new QRadioButton("After the run (invalidate)", timing_box);
        auto* before = new QRadioButton("Before (leave out the pair)", timing_box);
        after->setToolTip("<b>After</b><br>The excepted results stay, with a false status.");
        before->setToolTip("<b>Before</b><br>The excepted pairs are left out of the results (early filter).");
        timing_layout->addWidget(after);
        timing_layout->addWidget(before);
        (copy_->exception.timing == QLatin1String("before") ? before : after)->setChecked(true);
        exception_layout->addWidget(kind);
        exception_layout->addWidget(stack, 1);
        exception_layout->addWidget(timing_box);
        const int initial = !copy_->exception.relation.isEmpty() ? 1 : copy_->exception.tree ? 2 : 0;
        if (!copy_->exception.relation.isEmpty()) relation->setCurrentText(copy_->exception.relation);
        exception_tree_->setRoot(cloneTree(copy_->exception.tree.get()));
        kind->setCurrentIndex(initial);
        stack->setCurrentIndex(initial);
        connect(kind, &QComboBox::currentIndexChanged, stack, &QStackedWidget::setCurrentIndex);
        connect(kind, &QComboBox::currentIndexChanged, this, [this, relation](int index) {
            copy_->exception.relation = index == 1 ? relation->currentText() : QString();
        });
        connect(relation, &QComboBox::currentTextChanged, this, [this, kind](const QString& text) {
            if (kind->currentIndex() == 1) copy_->exception.relation = text;
        });
        connect(before, &QRadioButton::toggled, this, [this](bool on) { copy_->exception.timing = on ? "before" : "after"; });
        // The tree is read back in apply(), the kind decides which form wins.
        connect(kind, &QComboBox::currentIndexChanged, this, [this](int index) {
            exception_tree_->setProperty("active", index == 2);
        });
        exception_tree_->setProperty("active", initial == 2);
    }
    tabs->addTab(scrolled(exception_page), "Exception");

    // Must (AbsoluteOrRelativeChecking).
    tabs->addTab(scrolled(buildMustPage(tabs)), "Must");

    // Grouping.
    auto* grouping_page = new QWidget;
    auto* grouping_layout = new QVBoxLayout(grouping_page);
    grouping_layout->addWidget(helpLabel("The results are grouped by the IFC class, by object, by a property or an "
                                         "attribute value, or by the elements of a grouping set (e.g. the spaces: "
                                         "one group per space, with the results in it). A Must rule checks the "
                                         "groups.", grouping_page));
    auto* grouping_kind = new QComboBox(grouping_page);
    grouping_kind->addItems({"No grouping", "ENTITY", "SOURCE", "TARGET", "Property", "Attribute", "Grouping set"});
    if (!two) {
        grouping_kind->setItemData(2, 0, Qt::UserRole - 1);
        grouping_kind->setItemData(3, 0, Qt::UserRole - 1);
    }
    auto* grouping_stack = new QStackedWidget(grouping_page);
    grouping_stack->addWidget(new QWidget); // none / ENTITY / SOURCE / TARGET
    auto* property_page = new QWidget;
    auto* property_form = new QFormLayout(property_page);
    auto* group_pset = new QLineEdit(property_page);
    auto* group_prop = new QLineEdit(property_page);
    property_form->addRow("Property set", group_pset);
    property_form->addRow("Property", group_prop);
    grouping_stack->addWidget(property_page);
    auto* attribute_page = new QWidget;
    auto* attribute_form = new QFormLayout(attribute_page);
    auto* group_attribute = new QLineEdit(attribute_page);
    attribute_form->addRow("Attribute", group_attribute);
    grouping_stack->addWidget(attribute_page);
    auto* set_editor = new FacetListEditor;
    grouping_stack->addWidget(set_editor);
    grouping_layout->addWidget(grouping_kind);
    grouping_layout->addWidget(grouping_stack, 1);
    clash::Grouping& g = copy_->grouping;
    int grouping_index = 0;
    if (g.kind == clash::Grouping::Kind::Name) grouping_index = g.name == "SOURCE" ? 2 : g.name == "TARGET" ? 3 : 1;
    if (g.kind == clash::Grouping::Kind::Facet && g.facet) {
        if (g.facet->type == it::FacetType::Property) {
            grouping_index = 4;
            group_pset->setText(paramText(g.facet->propertySet));
            group_prop->setText(paramText(g.facet->baseName));
        } else if (g.facet->type == it::FacetType::Attribute) {
            grouping_index = 5;
            group_attribute->setText(paramText(g.facet->name));
        }
    }
    if (g.kind == clash::Grouping::Kind::SelectFacet) grouping_index = 6;
    if (g.select.kind != clash::Select::Kind::Facet) {
        g.select = clash::Select{};
        g.select.kind = clash::Select::Kind::Facet;
        g.select.applicability.push_back(it::Facet::make(it::FacetType::Entity));
        g.select.applicability.back().name = it::Param::text("IFCSPACE");
    }
    set_editor->setFacets(&g.select.applicability);
    grouping_kind->setCurrentIndex(grouping_index);
    const auto pageOf = [](int index) { return index <= 3 ? 0 : index - 3; };
    grouping_stack->setCurrentIndex(pageOf(grouping_index));
    const auto storeGrouping = [this, grouping_kind, group_pset, group_prop, group_attribute]() {
        clash::Grouping& grouping = copy_->grouping;
        const int index = grouping_kind->currentIndex();
        grouping.facet.reset();
        if (index == 0) grouping.kind = clash::Grouping::Kind::None;
        else if (index <= 3) {
            grouping.kind = clash::Grouping::Kind::Name;
            grouping.name = grouping_kind->currentText();
        } else if (index == 4) {
            grouping.kind = clash::Grouping::Kind::Facet;
            it::Facet facet = it::Facet::make(it::FacetType::Property);
            facet.propertySet = it::Param::text(group_pset->text().trimmed().toStdString());
            facet.baseName = it::Param::text(group_prop->text().trimmed().toStdString());
            grouping.facet = facet;
        } else if (index == 5) {
            grouping.kind = clash::Grouping::Kind::Facet;
            it::Facet facet = it::Facet::make(it::FacetType::Attribute);
            facet.name = it::Param::text(group_attribute->text().trimmed().toStdString());
            grouping.facet = facet;
        } else {
            grouping.kind = clash::Grouping::Kind::SelectFacet;
        }
    };
    connect(grouping_kind, &QComboBox::currentIndexChanged, this, [grouping_stack, pageOf, storeGrouping](int index) {
        grouping_stack->setCurrentIndex(pageOf(index));
        storeGrouping();
    });
    connect(group_pset, &QLineEdit::textEdited, this, storeGrouping);
    connect(group_prop, &QLineEdit::textEdited, this, storeGrouping);
    connect(group_attribute, &QLineEdit::textEdited, this, storeGrouping);
    tabs->addTab(scrolled(grouping_page), "Grouping");

    tabs->addTab(scrolled(new LabelSetsEditor(&copy_->criticity, "Criticity", nullptr)), "Criticity");
    tabs->addTab(scrolled(new LabelSetsEditor(&copy_->actor, "Actor", nullptr)), "Actor");

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

QWidget* RuleEditDialog::buildRulePage(bool two_object) {
    const clash::RuleSpec* spec = clash::RuleCatalogue::instance().find(copy_->type);
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    auto* title = new QLabel(QString("<b>%1</b> — %2")
                                 .arg(spec ? spec->label : copy_->type,
                                      spec ? clash::familyBadge(spec->family) : QString()),
                             page);
    layout->addWidget(title);
    if (spec && !spec->doc.isEmpty()) layout->addWidget(helpLabel(spec->doc, page));

    auto* form = new QFormLayout();
    auto* name = new QLineEdit(copy_->viewer.label, page);
    name->setPlaceholderText(spec ? spec->label : copy_->type);
    name->setToolTip("<b>Title</b><br>The name of the rule in the Rule tree and the results "
                     "(empty: the name of the rule type).");
    connect(name, &QLineEdit::textEdited, this, [this](const QString& text) {
        copy_->viewer.label = text;
        setWindowTitle("Edit Rule — " + RuleDocument::labelOf(*copy_));
    });
    form->addRow("Title", name);
    auto* description = new QPlainTextEdit(copy_->viewer.description, page);
    description->setPlaceholderText("What the rule checks, and why");
    description->setToolTip("<b>Description</b><br>A free text kept with the rule in the rule file.");
    description->setMaximumHeight(90);
    connect(description, &QPlainTextEdit::textChanged, this,
            [this, description]() { copy_->viewer.description = description->toPlainText(); });
    form->addRow("Description", description);
    layout->addLayout(form);

    auto* params_box = new QGroupBox("Parameters", page);
    params_box->setToolTip(two_object ? "<b>Parameters</b><br>The settings of the check between each pair."
                                      : "<b>Parameters</b><br>The settings of the check of each element.");
    auto* params_layout = new QVBoxLayout(params_box);
    auto* params = new ParamForm(params_box);
    params->setRule(spec, &copy_->params);
    params_layout->addWidget(params);
    if (!spec) params_layout->addWidget(helpLabel("Unknown rule type: its parameters are kept as they are.", params_box));
    layout->addWidget(params_box);
    layout->addStretch(1);
    return page;
}

QWidget* RuleEditDialog::buildSelectionPage(const QStringList& slot_names, bool two_object) {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->addWidget(helpLabel("The elements the rule checks: an IDS selection (the elements matching all its "
                                "facets), or the elements of another rule of the tree (cascade).",
                                page));
    const auto help = [two_object](const QString& slot) {
        if (slot == QLatin1String("source")) {
            return two_object ? QString("The first element of each pair.") : QString("The elements checked one by one.");
        }
        if (slot == QLatin1String("target")) return QString("The second element of each pair.");
        return QString("The elements around (obstacles, blockers) the rule takes into account.");
    };
    auto* slot_tabs = new QTabWidget(page);
    layout->addWidget(slot_tabs, 1);
    for (const QString& name : slot_names) {
        auto state = std::make_unique<SlotState>();
        SlotState* slot = state.get();
        slot->name = name;

        auto* box = new QWidget(slot_tabs);
        box->setToolTip(QString("<b>%1</b><br>%2<br><i>Right click: copy / paste the elements of the slot.</i>")
                            .arg(name, help(name)));
        box->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(box, &QWidget::customContextMenuRequested, this,
                [this, slot, box](const QPoint& pos) { slotMenu(*slot, box->mapToGlobal(pos)); });
        auto* box_layout = new QVBoxLayout(box);
        box_layout->addWidget(helpLabel(help(name), box));
        auto* mode_row = new QFormLayout();
        auto* mode = new QComboBox(box);
        slot->mode_combo = mode;
        mode->addItems({"Empty", "IDS selection", "Elements of a rule (cascade)", "Nested rule"});
        mode->setItemData(0, "The slot selects nothing.", Qt::ToolTipRole);
        mode->setItemData(1, "The elements matching all the facets (the IDS editor).", Qt::ToolTipRole);
        mode->setItemData(2, "The elements another rule of the tree finds (or does not find).", Qt::ToolTipRole);
        mode->setItemData(3, "A rule kept inside this slot (not in the tree).", Qt::ToolTipRole);
        mode_row->addRow("Elements from", mode);
        box_layout->addLayout(mode_row);
        auto* stack = new QStackedWidget(box);
        stack->addWidget(helpLabel("No element: choose an IDS selection or a rule of the tree.", box));

        // The IDS selection editor of the filters / IDS panel, with its preview.
        slot->editor = new filters::SelectionEditor(context_.bridge, context_.ifc_path, box);
        connect(slot->editor, &filters::SelectionEditor::isolateRequested, this, &RuleEditDialog::isolateRequested);
        stack->addWidget(slot->editor);

        auto* cascade = new QWidget(box);
        auto* form = new QFormLayout(cascade);
        auto* rule_row = new QHBoxLayout();
        slot->rule = new QComboBox(cascade);
        slot->rule->setToolTip("<b>Rule</b><br>The rule of the tree whose elements feed the slot.");
        if (document_) {
            for (const auto& candidate : document_->treeRules()) {
                // Not the rule itself, nor a rule this rule already feeds (a loop).
                if (candidate->id == rule_->id || document_->wouldLoop(candidate->id, rule_->id)) continue;
                slot->rule->addItem(RuleDocument::labelOf(*candidate), candidate->id);
            }
        }
        auto* edit = new QPushButton("Edit…", cascade);
        edit->setToolTip("<b>Edit</b><br>Open the editing window of the chosen rule: its parameters, its own "
                         "source and target (which can come from another rule: cascade), its exceptions…");
        edit->setEnabled(slot->rule->count() > 0);
        connect(edit, &QPushButton::clicked, this, [this, slot]() {
            const QString id = slot->rule->currentData().toString();
            const auto source = document_ && !id.isEmpty() ? document_->treeRule(id) : nullptr;
            if (!source || !editNested(source)) return;
            slot->rule->setItemText(slot->rule->currentIndex(), RuleDocument::labelOf(*source));
            emit ruleEdited();
        });
        rule_row->addWidget(slot->rule, 1);
        rule_row->addWidget(edit);
        slot->element = new QComboBox(cascade);
        slot->element->setToolTip("<b>Elements passed</b><br>The sources or targets of the results of the rule "
                                  "(its passing elements), or the input elements it found nothing for (the complement).");
        const std::vector<std::tuple<QString, QString, bool>> passes{
            {"Sources of its results", "source", true},
            {"Targets of its results", "target", true},
            {"Sources without result", "source", false},
            {"Targets without result", "target", false}};
        for (const auto& [label, element, passing] : passes) {
            slot->element->addItem(label, QStringList{element, passing ? "1" : "0"});
        }
        form->addRow("Rule", rule_row);
        form->addRow("Elements", slot->element);
        if (slot->rule->count() == 0) form->addRow(helpLabel("No other rule in the tree can feed this slot.", cascade));
        stack->addWidget(cascade);

        auto* nested = new QWidget(box);
        auto* nested_layout = new QVBoxLayout(nested);
        nested_layout->addWidget(helpLabel("A rule nested in this slot (not in the tree): its elements feed the slot.",
                                           nested));
        auto* nested_edit = new QPushButton("Edit the nested rule…", nested);
        nested_edit->setToolTip("<b>Edit</b><br>Open the editing window of the nested rule "
                                "(written back with this rule, on OK).");
        connect(nested_edit, &QPushButton::clicked, this, [this, slot]() {
            if (!editNested(slot->original.rule)) return;
            slot->mode_combo->setItemText(3, "Nested rule: " + RuleDocument::labelOf(*slot->original.rule));
        });
        nested_layout->addWidget(nested_edit, 0, Qt::AlignLeft);
        nested_layout->addStretch(1);
        stack->addWidget(nested);

        connect(mode, &QComboBox::currentIndexChanged, this, [slot, stack](int index) {
            slot->mode = index;
            stack->setCurrentIndex(index);
        });
        box_layout->addWidget(stack, 1);
        slot_tabs->addTab(box, name.left(1).toUpper() + name.mid(1));
        loadSlot(*slot, name == "source" ? copy_->source : name == "target" ? copy_->target : copy_->context);
        slots_.push_back(std::move(state));
    }
    // Right click on a slot's tab: the same menu as on its page.
    slot_tabs->tabBar()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(slot_tabs->tabBar(), &QWidget::customContextMenuRequested, this, [this, slot_tabs](const QPoint& pos) {
        const int index = slot_tabs->tabBar()->tabAt(pos);
        if (index < 0 || index >= static_cast<int>(slots_.size())) return;
        slot_tabs->setCurrentIndex(index);
        slotMenu(*slots_[static_cast<size_t>(index)], slot_tabs->tabBar()->mapToGlobal(pos));
    });
    return page;
}

void RuleEditDialog::loadSlot(SlotState& slot, const clash::Select& select) {
    slot.original = select;
    const bool linked = select.kind == clash::Select::Kind::Rule && select.rule && document_ &&
                        document_->treeRule(select.rule->id) && select.rule->id != rule_->id;
    int mode = 0;
    bonsaiviewer::services::IdsBridge::SelectionFields fields;
    if (select.kind == clash::Select::Kind::Facet) {
        mode = 1;
        fields = idsFields(select.applicability);
    } else if (select.kind == clash::Select::Kind::Rule && select.rule) {
        mode = linked ? 2 : 3;
        // A nested rule is edited on a copy, written back on OK.
        if (!linked) slot.original.rule = clash::json::cloneRule(*select.rule);
    }
    slot.editor->setFields(fields.facets.empty() ? filters::SelectionEditor::defaultFields() : fields);
    if (mode == 2) slot.rule->setCurrentIndex(std::max(0, slot.rule->findData(select.rule->id)));
    if (select.kind == clash::Select::Kind::Rule) {
        slot.element->setCurrentIndex(std::max(0, slot.element->findData(
            QStringList{select.element_to_pass, select.element_state_to_pass ? "1" : "0"})));
    }
    // "Nested rule" is offered only when the slot holds one.
    auto* model = qobject_cast<QStandardItemModel*>(slot.mode_combo->model());
    if (auto* item = model ? model->item(3) : nullptr) item->setEnabled(mode == 3);
    slot.mode_combo->setItemText(3, mode == 3 ? "Nested rule: " + RuleDocument::labelOf(*slot.original.rule)
                                              : QString("Nested rule"));
    slot.mode = mode;
    if (slot.mode_combo->currentIndex() == mode) emit slot.mode_combo->currentIndexChanged(mode);
    else slot.mode_combo->setCurrentIndex(mode);
}

void RuleEditDialog::slotMenu(SlotState& slot, const QPoint& global_pos) {
    QMenu menu(this);
    menu.setToolTipsVisible(true);
    const clash::Select current = selectOf(slot);
    QAction* copy = menu.addAction("Copy the " + slot.name, this, [current]() { slot_clipboard::copy(current); });
    copy->setEnabled(!current.empty());
    copy->setToolTip("Copy the elements of this slot (IDS facets, or the rule they come from), "
                     "to paste them into another slot or rule.");
    QAction* paste = menu.addAction("Paste into the " + slot.name, this, [this, &slot]() {
        auto pasted = slot_clipboard::paste();
        if (!pasted) return;
        const QString error = document_ ? slot_clipboard::pasteError(*pasted, *rule_, *document_) : QString();
        if (!error.isEmpty()) {
            QMessageBox::information(this, "Paste", error);
            return;
        }
        loadSlot(slot, *pasted);
    });
    paste->setEnabled(slot_clipboard::canPaste());
    paste->setToolTip("Replace the elements of this slot by the copied ones (a copied slot, or IDS facets "
                      "copied from the filters, the tables or the IDS panel).");
    menu.exec(global_pos);
}

QWidget* RuleEditDialog::buildMustPage(QTabWidget* tabs) {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    auto* enabled = new QCheckBox("Turn this rule into a Must check", page);
    enabled->setToolTip("<b>Must check</b><br>The results of the rule are grouped, each group is measured and "
                        "checked: the rule then gives one verdict per group (passing or failing), not the pairs.");
    enabled->setChecked(copy_->must.has_value());
    layout->addWidget(enabled);
    auto* body = new QWidget(page);
    auto* body_layout = new QVBoxLayout(body);
    body_layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(body);
    const clash::AbsOrRelCheck initial = copy_->must.value_or(clash::AbsOrRelCheck{});

    // ① The groups checked: a reminder of the Grouping tab.
    auto* groups_box = new QGroupBox("① Groups checked", body);
    auto* groups_layout = new QVBoxLayout(groups_box);
    auto* groups_text = new QLabel(groups_box);
    groups_text->setWordWrap(true);
    auto* groups_warning = new QLabel(groups_box);
    groups_warning->setWordWrap(true);
    groups_warning->setStyleSheet("color: #B8860B;");
    auto* change = new QPushButton("Change the grouping…", groups_box);
    change->setToolTip("Open the Grouping tab: the Must check uses its groups.");
    connect(change, &QPushButton::clicked, this, [tabs]() {
        for (int t = 0; t < tabs->count(); ++t) {
            if (tabs->tabText(t) == QLatin1String("Grouping")) tabs->setCurrentIndex(t);
        }
    });
    groups_layout->addWidget(groups_text);
    groups_layout->addWidget(groups_warning);
    groups_layout->addWidget(change, 0, Qt::AlignLeft);
    const auto refreshGroups = [this, groups_text, groups_warning]() {
        const clash::Grouping& g = copy_->grouping;
        QString text;
        switch (g.kind) {
        case clash::Grouping::Kind::None: text = "One group per <b>source element</b> (no grouping set in the Grouping tab)."; break;
        case clash::Grouping::Kind::Name: text = QString("Grouping tab: by <b>%1</b>.").arg(g.name.toHtmlEscaped()); break;
        case clash::Grouping::Kind::Facet:
            text = QString("Grouping tab: by <b>%1</b>.").arg(g.facet ? facetSummary(*g.facet).toHtmlEscaped() : QString("?"));
            break;
        case clash::Grouping::Kind::SelectFacet: {
            QStringList facets;
            for (const auto& facet : g.select.applicability) facets << facetSummary(facet).toHtmlEscaped();
            text = QString("Grouping tab: one group per element of the <b>grouping set</b> (%1), empty groups included.")
                       .arg(facets.join(", "));
            break;
        }
        }
        groups_text->setText(text);
        const bool warn = g.kind != clash::Grouping::Kind::SelectFacet;
        groups_warning->setVisible(warn);
        groups_warning->setText("⚠ The elements with no result form no group and are not checked. To check them "
                                "too (e.g. a room with no socket), use a grouping set in the Grouping tab.");
    };
    refreshGroups();
    connect(tabs, &QTabWidget::currentChanged, this, [refreshGroups](int) { refreshGroups(); });
    body_layout->addWidget(groups_box);

    // ② What is measured in each group.
    auto* measure_box = new QGroupBox("② What is measured in each group", body);
    auto* measure_layout = new QVBoxLayout(measure_box);
    auto* by_number = new QRadioButton("The number of elements", measure_box);
    auto* by_quantity = new QRadioButton("The sum of a property of the elements", measure_box);
    by_number->setToolTip("SOURCE / TARGET are the numbers of source / target elements of the group.");
    by_quantity->setToolTip("SOURCE / TARGET are the sums of the property over the source / target elements of the "
                            "group (a missing or non numeric value counts 0).");
    auto* property_row = new QWidget(measure_box);
    auto* property_form = new QFormLayout(property_row);
    property_form->setContentsMargins(24, 0, 0, 0);
    auto* pset = new QComboBox(property_row);
    auto* prop = new QComboBox(property_row);
    pset->setEditable(true);
    prop->setEditable(true);
    pset->setToolTip("The property set (the list: those of the source and target elements in the active model).");
    prop->setToolTip("The numeric property summed.");
    pset->setCurrentText(initial.quantity_pset);
    prop->setCurrentText(initial.quantity_prop);
    property_form->addRow("Property set", pset);
    property_form->addRow("Property", prop);
    measure_layout->addWidget(by_number);
    measure_layout->addWidget(by_quantity);
    measure_layout->addWidget(property_row);
    (initial.check_type.contains("Quantity") ? by_quantity : by_number)->setChecked(true);
    property_row->setVisible(by_quantity->isChecked());
    body_layout->addWidget(measure_box);
    // The property sets of the slots' elements, from the active model.
    auto psets = std::make_shared<QMap<QString, QStringList>>();
    const auto fillProperties = [this, pset, prop, psets]() {
        const QString path = context_.ifc_path ? context_.ifc_path() : QString();
        if (path.isEmpty()) return;
        for (const clash::Select* select : {&copy_->source, &copy_->target}) {
            if (select->kind != clash::Select::Kind::Facet || select->applicability.empty()) continue;
            QPointer<QComboBox> guard_pset = pset, guard_prop = prop;
            context_.bridge.selectionProperties(
                path, idsFields(select->applicability),
                [guard_pset, guard_prop, psets](bool ok, const bonsaiviewer::services::IdsBridge::PropertyMap& map,
                                                 const auto&, const QString&) {
                    if (!ok || !guard_pset || !guard_prop) return;
                    for (auto it = map.begin(); it != map.end(); ++it) {
                        QStringList& props = (*psets)[it.key()];
                        for (auto p = it.value().begin(); p != it.value().end(); ++p) {
                            if (!props.contains(p.key())) props << p.key();
                        }
                    }
                    const QString current = guard_pset->currentText();
                    const QSignalBlocker block(guard_pset);
                    guard_pset->clear();
                    guard_pset->addItems(psets->keys());
                    guard_pset->setCurrentText(current);
                });
        }
    };
    connect(pset, &QComboBox::currentTextChanged, this, [prop, psets](const QString& name) {
        const QString current = prop->currentText();
        const QSignalBlocker block(prop);
        prop->clear();
        QStringList props = psets->value(name);
        props.sort();
        prop->addItems(props);
        prop->setCurrentText(current);
    });

    // ③ The condition to pass.
    auto* condition_box = new QGroupBox("③ Condition to pass", body);
    auto* condition_layout = new QVBoxLayout(condition_box);
    auto* simple = new QWidget(condition_box);
    auto* simple_row = new QHBoxLayout(simple);
    simple_row->setContentsMargins(0, 0, 0, 0);
    auto* left = new QComboBox(simple);
    left->addItem("TARGET", "TARGET");
    left->addItem("SOURCE", "SOURCE");
    left->setToolTip("The measure of the targets or of the sources of the group.");
    auto* op = new QComboBox(simple);
    for (const auto& [text, symbol] : std::vector<std::pair<QString, QString>>{
             {"≥", ">="}, {">", ">"}, {"=", "=="}, {"≤", "<="}, {"<", "<"}, {"≠", "!="}}) {
        op->addItem(text, symbol);
    }
    auto* value = new QDoubleSpinBox(simple);
    value->setRange(-1e9, 1e9);
    value->setDecimals(3);
    value->setValue(1);
    auto* times = new QComboBox(simple);
    times->addItem("", QString());
    times->addItem("× SOURCE", "SOURCE");
    times->addItem("× TARGET", "TARGET");
    times->setToolTip("Empty: compare with the value (absolute). × SOURCE / × TARGET: with the value times the "
                      "other measure (relative), e.g. SOURCE = 2 × TARGET.");
    simple_row->addWidget(left);
    simple_row->addWidget(op);
    simple_row->addWidget(value);
    simple_row->addWidget(times);
    simple_row->addStretch(1);
    auto* advanced = new QCheckBox("Advanced (free expression)", condition_box);
    advanced->setToolTip("A free expression with SOURCE and TARGET: + - * /, parentheses, = == != < <= > >=, "
                         "and / or / not. E.g. SOURCE >= 1 and TARGET <= 2*SOURCE.");
    auto* expression = new QLineEdit(condition_box);
    condition_layout->addWidget(simple);
    condition_layout->addWidget(advanced);
    condition_layout->addWidget(expression);
    body_layout->addWidget(condition_box);
    // The operation of the file, shown in the simple form when it fits.
    static const QRegularExpression pattern(
        R"(^\s*(SOURCE|TARGET)\s*(>=|<=|==|!=|=|>|<)\s*([-+]?[0-9]*\.?[0-9]+)\s*(?:\*\s*(SOURCE|TARGET))?\s*$)");
    const QString operation = initial.operation.isEmpty() ? QStringLiteral("TARGET>=1") : initial.operation;
    const auto match = pattern.match(operation);
    if (match.hasMatch()) {
        left->setCurrentIndex(left->findData(match.captured(1)));
        const QString symbol = match.captured(2) == QLatin1String("=") ? QStringLiteral("==") : match.captured(2);
        op->setCurrentIndex(std::max(0, op->findData(symbol)));
        value->setValue(match.captured(3).toDouble());
        times->setCurrentIndex(std::max(0, times->findData(match.captured(4))));
    } else {
        advanced->setChecked(true);
    }
    expression->setText(operation);

    // ④ The reading of the check, and the validation of the expression.
    auto* reading_box = new QGroupBox("④ Reading", body);
    auto* reading_layout = new QVBoxLayout(reading_box);
    auto* reading = new QLabel(reading_box);
    reading->setWordWrap(true);
    auto* validity = new QLabel(reading_box);
    validity->setWordWrap(true);
    reading_layout->addWidget(reading);
    reading_layout->addWidget(validity);
    body_layout->addWidget(reading_box);

    // Options.
    auto* count_excepted = new QCheckBox("Count the pairs an exception invalidated", body);
    count_excepted->setToolTip("Checked (as the Python): a pair excepted after the run still counts in its group. "
                               "Unchecked: the excepted pairs are left out of the groups.");
    count_excepted->setChecked(initial.count_excepted);
    body_layout->addWidget(count_excepted);
    auto* test_row = new QHBoxLayout();
    auto* test = new QPushButton("Test", body);
    test->setToolTip("Run the rule as configured in this window (not saved) and count the passing and failing "
                     "groups. The Result panel, the colours and the view do not change.");
    auto* test_result = new QLabel(body);
    test_result->setWordWrap(true);
    test->setVisible(static_cast<bool>(context_.test_rule));
    test_row->addWidget(test);
    test_row->addWidget(test_result, 1);
    body_layout->addLayout(test_row);
    layout->addStretch(1);

    // The operation of the current state, and everything that follows from it.
    const auto currentOperation = [=]() -> QString {
        if (advanced->isChecked()) return expression->text().trimmed();
        QString right = QString::number(value->value(), 'g', 10);
        const QString other = times->currentData().toString();
        if (!other.isEmpty()) right = (value->value() == 1.0 ? QString() : right + "*") + other;
        return left->currentData().toString() + op->currentData().toString() + right;
    };
    const auto store = [=]() {
        simple->setEnabled(!advanced->isChecked());
        expression->setReadOnly(!advanced->isChecked());
        property_row->setVisible(by_quantity->isChecked());
        body->setEnabled(enabled->isChecked());
        const QString operation = currentOperation();
        if (!advanced->isChecked()) {
            const QSignalBlocker block(expression);
            expression->setText(operation);
        }
        // The reading.
        const bool quantity = by_quantity->isChecked();
        const QString what = quantity ? QString("the sum of %1").arg(prop->currentText().isEmpty() ? "the property" : prop->currentText().toHtmlEscaped())
                                      : QString("the number");
        const auto of = [&](const QString& side) {
            const QString set = side == QLatin1String("SOURCE") ? "sources" : "targets";
            return quantity ? QString("%1 of its %2").arg(what, set) : QString("the number of its %1").arg(set);
        };
        QString sentence;
        if (!advanced->isChecked()) {
            static const QMap<QString, QString> words{{">=", "at least"}, {">", "more than"}, {"==", "exactly"},
                                                     {"<=", "at most"},  {"<", "less than"}, {"!=", "not"}};
            const QString other = times->currentData().toString();
            const QString amount = other.isEmpty() ? QString::number(value->value(), 'g', 10)
                                                   : (value->value() == 1.0 ? of(other)
                                                                            : QString("%1 × %2").arg(QString::number(value->value(), 'g', 10), of(other)));
            sentence = QString("A group passes when %1 is <b>%2 %3</b>.").arg(of(left->currentData().toString()),
                                                                                words.value(op->currentData().toString()), amount);
        } else {
            sentence = QString("A group passes when <b>%1</b> holds (SOURCE: %2, TARGET: %3).")
                           .arg(operation.toHtmlEscaped(), of("SOURCE"), of("TARGET"));
        }
        reading->setText(sentence);
        // The validation: the run's own parser.
        QString problem;
        if (!operation.contains("SOURCE") && !operation.contains("TARGET")) problem = "the expression uses neither SOURCE nor TARGET";
        try {
            clash::evaluateOperation(operation, 3, 7);
        } catch (const std::exception& e) {
            // The parser saw the numbers of the probe: the user's own text is shown.
            const QString what = QString::fromUtf8(e.what());
            problem = what.startsWith("division") ? QString("division by zero")
                                                  : QString("syntax error in \"%1\"").arg(operation);
        }
        if (problem.isEmpty() && !operation.contains(QRegularExpression("[<>=!]|\\band\\b|\\bor\\b|\\bnot\\b"))) {
            problem = "not a comparison: every group with a non-zero value passes";
        }
        if (quantity && (pset->currentText().trimmed().isEmpty() || prop->currentText().trimmed().isEmpty())) {
            problem = problem.isEmpty() ? QString("choose the property set and the property") : problem;
        }
        validity->setText(problem.isEmpty() ? QString("<span style='color:#3CB44B'>✔ valid</span>")
                                            : QString("<span style='color:#E6194B'>✖ %1</span>").arg(problem.toHtmlEscaped()));
        if (!enabled->isChecked()) {
            copy_->must.reset();
            return;
        }
        clash::AbsOrRelCheck check;
        const bool relative = operation.contains("SOURCE") && operation.contains("TARGET");
        check.check_type = QString("%1_%2").arg(relative ? "Relative" : "Absolute", quantity ? "Quantity" : "Number");
        if (quantity) {
            check.quantity_pset = pset->currentText().trimmed();
            check.quantity_prop = prop->currentText().trimmed();
        }
        check.operation = operation;
        check.count_excepted = count_excepted->isChecked();
        copy_->must = check;
    };
    store();
    fillProperties();
    connect(enabled, &QCheckBox::toggled, this, store);
    connect(by_number, &QRadioButton::toggled, this, store);
    connect(pset, &QComboBox::currentTextChanged, this, store);
    connect(prop, &QComboBox::currentTextChanged, this, store);
    connect(left, &QComboBox::currentIndexChanged, this, store);
    connect(op, &QComboBox::currentIndexChanged, this, store);
    connect(value, &QDoubleSpinBox::valueChanged, this, store);
    connect(times, &QComboBox::currentIndexChanged, this, store);
    connect(advanced, &QCheckBox::toggled, this, store);
    connect(expression, &QLineEdit::textEdited, this, store);
    connect(count_excepted, &QCheckBox::toggled, this, store);
    connect(test, &QPushButton::clicked, this, [this, test, test_result]() {
        if (!copy_->must) return;
        auto probe = clash::json::cloneRule(*copy_);
        applyTo(*probe);
        probe->state = QStringLiteral("Final");
        test->setEnabled(false);
        test_result->setText("Running…");
        QPointer<QPushButton> guard_test = test;
        QPointer<QLabel> guard_label = test_result;
        context_.test_rule(probe, [guard_test, guard_label](const QString& summary) {
            if (guard_test) guard_test->setEnabled(true);
            if (guard_label) guard_label->setText(summary);
        });
    });
    return page;
}

bool RuleEditDialog::editNested(const std::shared_ptr<clash::RuleCheck>& rule) {
    if (!rule) return false;
    RuleEditDialog dialog(rule, document_, context_, this);
    connect(&dialog, &RuleEditDialog::isolateRequested, this, &RuleEditDialog::isolateRequested);
    connect(&dialog, &RuleEditDialog::ruleEdited, this, &RuleEditDialog::ruleEdited);
    if (dialog.exec() != QDialog::Accepted) return false;
    dialog.apply();
    return true;
}

std::vector<bonsaiviewer::ports::ifctester::Facet> RuleEditDialog::facetsOf(const SlotState& slot,
                                                                            QString* error) const {
    if (!slot.editor) return {};
    const QString path = context_.ifc_path ? context_.ifc_path() : QString();
    ifcopenshell::file* file = path.isEmpty() ? nullptr : context_.bridge.file(path);
    QString message;
    auto facets = engineFacets(slot.editor->fields(), file, &message);
    if (error && !message.isEmpty()) *error = slot.name + ", " + message;
    return facets;
}

void RuleEditDialog::accept() {
    for (const auto& slot : slots_) {
        if (slot->mode != 1) continue;
        QString error;
        facetsOf(*slot, &error);
        if (!error.isEmpty()) {
            QMessageBox::warning(this, "Edit Rule", "The selection cannot be built:\n" + error);
            return;
        }
    }
    QDialog::accept();
}

clash::Select RuleEditDialog::selectOf(const SlotState& slot) const {
    clash::Select select;
    switch (slot.mode) {
    case 1: {
        auto facets = facetsOf(slot, nullptr);
        if (facets.empty()) return select;
        select.kind = clash::Select::Kind::Facet;
        select.applicability = std::move(facets);
        return select;
    }
    case 2: {
        const QString id = slot.rule ? slot.rule->currentData().toString() : QString();
        const auto source = document_ && !id.isEmpty() ? document_->treeRule(id) : nullptr;
        if (!source) return select;
        select.kind = clash::Select::Kind::Rule;
        select.rule = clash::json::cloneRule(*source);
        const QStringList pass = slot.element->currentData().toStringList();
        select.element_to_pass = pass.value(0, "source");
        select.element_state_to_pass = pass.value(1, "1") == QLatin1String("1");
        if (slot.original.kind == clash::Select::Kind::Rule) select.action_type = slot.original.action_type;
        return select;
    }
    case 3:
        return slot.original;
    default:
        return select;
    }
}

void RuleEditDialog::apply() {
    applyTo(*rule_);
}

void RuleEditDialog::applyTo(clash::RuleCheck& target) {
    for (const auto& slot : slots_) {
        const clash::Select select = selectOf(*slot);
        if (slot->name == QLatin1String("source")) target.source = select;
        else if (slot->name == QLatin1String("target")) target.target = select;
        else target.context = select;
    }
    if (exception_tree_) {
        if (exception_tree_->property("active").toBool()) {
            copy_->exception.relation.clear();
            copy_->exception.tree = exception_tree_->root();
        } else {
            copy_->exception.tree.reset();
        }
    }
    target.viewer = copy_->viewer;
    target.state = copy_->state;
    target.params = copy_->params;
    target.exception = copy_->exception;
    target.must = copy_->must;
    target.grouping = copy_->grouping;
    target.criticity = copy_->criticity;
    target.actor = copy_->actor;
}

// --- FolderEditDialog ----------------------------------------------------------------------------

FolderEditDialog::FolderEditDialog(std::shared_ptr<clash::RuleFolder> folder, QWidget* parent)
    : QDialog(parent), folder_(std::move(folder)) {
    copy_.activation_select = folder_->activation_select;
    copy_.activation_tree = cloneTree(folder_->activation_tree.get());
    setWindowTitle("Edit Folder — " + RuleDocument::labelOf(*folder_));
    resize(560, 560);
    auto* layout = new QVBoxLayout(this);
    active_ = folder_->active;
    auto* active = new QCheckBox("Active", this);
    active->setChecked(active_);
    active->setToolTip("<b>Active</b><br>Unchecked: the folder and its rules are left out of every run (they stay "
                       "in the tree, editable). Checked: the activation below decides.");
    connect(active, &QCheckBox::toggled, this, [this](bool on) { active_ = on; });
    layout->addWidget(active);
    layout->addWidget(helpLabel("The activation conditions the folder: its rules run only when the IDS facets "
                                "select at least one element, or when the rule tree holds (its rules run first).",
                                this));
    auto* kind = new QComboBox(this);
    kind->addItems({"Always active", "IDS facets select something", "Rule tree holds"});
    auto* stack = new QStackedWidget(this);
    stack->addWidget(new QWidget);
    auto* facets = new FacetListEditor;
    if (copy_.activation_select.kind != clash::Select::Kind::Facet) {
        copy_.activation_select = clash::Select{};
        copy_.activation_select.kind = clash::Select::Kind::Facet;
    }
    facets->setFacets(&copy_.activation_select.applicability);
    stack->addWidget(facets);
    tree_editor_ = new BooleanEditor(true);
    tree_editor_->setRoot(copy_.activation_tree);
    stack->addWidget(tree_editor_);
    mode_ = folder_->activation_tree ? 2 : (folder_->activation_select.empty() ? 0 : 1);
    kind->setCurrentIndex(mode_);
    stack->setCurrentIndex(mode_);
    connect(kind, &QComboBox::currentIndexChanged, this, [this, stack](int index) {
        mode_ = index;
        stack->setCurrentIndex(index);
    });
    layout->addWidget(kind);
    layout->addWidget(stack, 1);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

void FolderEditDialog::apply() {
    folder_->active = active_;
    folder_->activation_select = mode_ == 1 && !copy_.activation_select.applicability.empty() ? copy_.activation_select : clash::Select{};
    folder_->activation_tree = mode_ == 2 ? tree_editor_->root() : nullptr;
}

} // namespace bonsaiviewer::modules::rule
