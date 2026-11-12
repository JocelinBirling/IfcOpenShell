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

#include "RuleTree.h"

#include "Editors.h"
#include "SlotClipboard.h"

#include "../../services/PopupService.h"
#include "../../services/clash/RuleCatalogue.h"
#include "../../services/clash/RuleJson.h"

#include <QBrush>
#include <QColor>
#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFont>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMenu>
#include <QMimeData>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

#include <functional>
#include <set>

namespace bonsaiviewer::modules::rule {

namespace {

constexpr int kName = 0;
constexpr int kResults = 1;

const QStringList kFamilies{"OneObjectRule", "TwoObjectRule", "ComplexRule"};

QString familyTitle(const QString& family) {
    if (family == QLatin1String("OneObjectRule")) return QStringLiteral("One object");
    if (family == QLatin1String("TwoObjectRule")) return QStringLiteral("Two objects");
    return QStringLiteral("Complex");
}

bool containsFolder(const clash::RuleFolder& folder, const QString& id) {
    for (const clash::RuleNode& node : folder.contains) {
        if (const auto* f = std::get_if<std::shared_ptr<clash::RuleFolder>>(&node)) {
            if ((*f)->id == id || containsFolder(**f, id)) return true;
        }
    }
    return false;
}

} // namespace

clash::Select* TreeRef::select() const {
    if (!rule) return nullptr;
    if (slot == QLatin1String("source")) return &rule->source;
    if (slot == QLatin1String("target")) return &rule->target;
    if (slot == QLatin1String("context")) return &rule->context;
    return nullptr;
}

RuleTreeWidget::RuleTreeWidget(RuleDocument& document, QWidget* parent) : QTreeWidget(parent), document_(document) {
    setColumnCount(2);
    setHeaderLabels({"Rule", "Results"});
    header()->setSectionResizeMode(kName, QHeaderView::Stretch);
    header()->setSectionResizeMode(kResults, QHeaderView::ResizeToContents);
    header()->setStretchLastSection(false);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setDragEnabled(true);
    setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDefaultDropAction(Qt::MoveAction);
    setExpandsOnDoubleClick(false);

    // Rebuilt after the change, never inside the signal of an item.
    connect(&document_, &RuleDocument::changed, this, [this]() { QTimer::singleShot(0, this, &RuleTreeWidget::rebuild); });
    connect(this, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem* item, QTreeWidgetItem*) {
        emit refSelected(refOf(item));
    });
    connect(this, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem* item, int) {
        const auto ref = refOf(item);
        if (!ref) return;
        if (ref->kind == TreeRef::Kind::Rule) emit editRuleRequested(ref->rule);
        else if (ref->kind == TreeRef::Kind::Folder) emit editFolderRequested(ref->folder);
        else item->setExpanded(!item->isExpanded());
    });
}

std::optional<TreeRef> RuleTreeWidget::refOf(QTreeWidgetItem* item) const {
    if (item == nullptr || !refs_.contains(item)) return std::nullopt;
    return refs_.value(item);
}

std::optional<TreeRef> RuleTreeWidget::currentRef() const {
    return refOf(currentItem());
}

QString RuleTreeWidget::keyOf(QTreeWidgetItem* item) const {
    return keys_.value(item);
}

void RuleTreeWidget::rebuild() {
    QString current_key;
    for (auto i = keys_.begin(); i != keys_.end(); ++i) {
        if (i.key()->childCount() == 0) continue;
        if (i.key()->isExpanded()) collapsed_.remove(i.value());
        else collapsed_.insert(i.value());
    }
    if (currentItem()) current_key = keyOf(currentItem());
    const QSignalBlocker blocker(this);
    clear();
    refs_.clear();
    keys_.clear();
    if (!document_.isOpen()) return;

    auto* root = new QTreeWidgetItem(this);
    const clash::RuleFile& file = document_.file();
    root->setText(kName, (file.viewer.label.isEmpty() ? QStringLiteral("Rule File") : file.viewer.label) +
                             (document_.isModified() ? QStringLiteral(" *") : QString()));
    root->setToolTip(kName, document_.path().isEmpty() ? QStringLiteral("Not saved") : document_.path());
    QFont bold = root->font(kName);
    bold.setBold(true);
    root->setFont(kName, bold);
    root->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled);
    refs_.insert(root, TreeRef{TreeRef::Kind::File});
    keys_.insert(root, QStringLiteral("file"));
    for (const clash::RuleNode& node : file.contains) addNode(root, node);

    QTreeWidgetItem* current = nullptr;
    for (auto i = keys_.begin(); i != keys_.end(); ++i) {
        i.key()->setExpanded(!collapsed_.contains(i.value()));
        if (i.value() == current_key) current = i.key();
    }
    if (current) setCurrentItem(current);
}

QTreeWidgetItem* RuleTreeWidget::addNode(QTreeWidgetItem* parent, const clash::RuleNode& node) {
    if (const auto* f = std::get_if<std::shared_ptr<clash::RuleFolder>>(&node)) {
        const auto& folder = *f;
        auto* item = new QTreeWidgetItem(parent);
        item->setText(kName, RuleDocument::labelOf(*folder));
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
        const bool activation = folder->activation_tree || !folder->activation_select.empty();
        item->setToolTip(kName, QString("<b>Folder</b>%1<br>Double click: edit it (active or not, activation rule).")
                                    .arg(activation ? "<br>Runs only when its activation rule holds." : ""));
        if (!folder->active) {
            // Deactivated in its Edit window: shown grey, left out of the runs.
            for (int column = 0; column < columnCount(); ++column) item->setForeground(column, QBrush(QColor(Qt::gray)));
            item->setToolTip(kName, "<b>Folder (deactivated)</b><br>Its rules do not run. Double click: edit it.");
        }
        if (result_texts_.contains(folder->id)) item->setText(kResults, result_texts_.value(folder->id));
        refs_.insert(item, TreeRef{TreeRef::Kind::Folder, folder});
        keys_.insert(item, "folder:" + folder->id);
        for (const clash::RuleNode& child : folder->contains) addNode(item, child);
        return item;
    }
    const auto& rule = std::get<std::shared_ptr<clash::RuleCheck>>(node);
    return addRuleItem(parent, rule, false, "rule:" + rule->id);
}

QTreeWidgetItem* RuleTreeWidget::addRuleItem(QTreeWidgetItem* parent, const std::shared_ptr<clash::RuleCheck>& rule,
                                             bool nested, const QString& key) {
    const clash::RuleSpec* spec = clash::RuleCatalogue::instance().find(rule->type);
    auto* item = new QTreeWidgetItem(parent);
    item->setText(kName, RuleDocument::labelOf(*rule));
    QString tip = QString("<b>%1</b> — %2").arg(spec ? spec->label : rule->type,
                                                 spec ? clash::familyBadge(spec->family) : QStringLiteral("unknown rule"));
    if (spec) tip += "<br>" + spec->doc.toHtmlEscaped();
    if (!nested) {
        const QStringList fed_by = document_.fedBy(*rule);
        const QStringList feeds = document_.feeds(*rule);
        if (!fed_by.isEmpty()) tip += "<br><b>Fed by:</b> " + fed_by.join(", ").toHtmlEscaped();
        if (!feeds.isEmpty()) tip += "<br><b>Feeds:</b> " + feeds.join(", ").toHtmlEscaped();
    }
    tip += "<br><i>Double click: edit the rule (exceptions, Must, criticity, actor, grouping).</i>";
    item->setToolTip(kName, tip);
    if (!spec) item->setForeground(kName, QBrush(QColor("#E6194B")));
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled |
                   (nested ? Qt::NoItemFlags : Qt::ItemIsDragEnabled));
    if (!nested) {
        if (result_texts_.contains(rule->id)) item->setText(kResults, result_texts_.value(rule->id));
    } else {
        QFont underlined = item->font(kName);
        underlined.setUnderline(true);
        item->setFont(kName, underlined);
    }
    refs_.insert(item, TreeRef{TreeRef::Kind::Rule, nullptr, rule, nested});
    keys_.insert(item, key);
    if (!spec) return item;

    for (const QString& slot : spec->slot_names) {
        auto* slot_item = new QTreeWidgetItem(item);
        const QString slot_key = key + "/" + slot;
        TreeRef slot_ref{TreeRef::Kind::Slot, nullptr, rule, false, slot};
        clash::Select* select = slot_ref.select();
        slot_item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled);
        refs_.insert(slot_item, slot_ref);
        keys_.insert(slot_item, slot_key);
        if (select->empty()) {
            slot_item->setText(kName, slot + " — drop IDS facets or a rule");
            slot_item->setForeground(kName, QBrush(QColor(Qt::gray)));
            continue;
        }
        if (select->kind == clash::Select::Kind::Facet) {
            slot_item->setText(kName, slot + " [facets]");
            for (int i = 0; i < static_cast<int>(select->applicability.size()); ++i) {
                auto* facet_item = new QTreeWidgetItem(slot_item);
                facet_item->setText(kName, facetSummary(select->applicability[static_cast<size_t>(i)]));
                facet_item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
                refs_.insert(facet_item, TreeRef{TreeRef::Kind::Facet, nullptr, rule, false, slot, i});
                keys_.insert(facet_item, slot_key + "/" + QString::number(i));
            }
            continue;
        }
        // A rule: a link to a rule of the tree, or a rule nested by value.
        const auto linked = select->rule ? document_.treeRule(select->rule->id) : nullptr;
        if (linked && linked != rule) {
            slot_item->setText(kName, slot + " [rule]");
            auto* link = new QTreeWidgetItem(slot_item);
            link->setText(kName, QString("↳ %1 (%2 elements, %3)")
                                     .arg(RuleDocument::labelOf(*linked), select->element_to_pass,
                                          select->element_state_to_pass ? "passing" : "complement"));
            link->setToolTip(kName, "<b>Cascade</b><br>The elements found by this rule of the tree feed the slot. "
                                    "Edit the rule where it is in the tree.");
            link->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            refs_.insert(link, TreeRef{TreeRef::Kind::Link, nullptr, rule, false, slot});
            keys_.insert(link, slot_key + "/link");
        } else if (select->rule) {
            slot_item->setText(kName, slot + " [rule]");
            addRuleItem(slot_item, select->rule, true, slot_key + "/rule");
        }
    }
    return item;
}

void RuleTreeWidget::setResultTexts(const QMap<QString, QString>& texts) {
    result_texts_ = texts;
    rebuild();
}

std::vector<clash::RuleNode> RuleTreeWidget::selectedNodes() const {
    std::set<QString> ids;
    bool file = false;
    for (QTreeWidgetItem* item : selectedItems()) {
        const auto ref = refOf(item);
        if (!ref) continue;
        if (ref->kind == TreeRef::Kind::File) file = true;
        else if (ref->kind == TreeRef::Kind::Folder) ids.insert(ref->folder->id);
        else if (ref->kind == TreeRef::Kind::Rule && !ref->nested) ids.insert(ref->rule->id);
    }
    if (file) return document_.file().contains;
    std::vector<clash::RuleNode> nodes;
    std::function<void(const std::vector<clash::RuleNode>&)> walk = [&](const std::vector<clash::RuleNode>& list) {
        for (const clash::RuleNode& node : list) {
            if (const auto* f = std::get_if<std::shared_ptr<clash::RuleFolder>>(&node)) {
                if (ids.count((*f)->id)) nodes.push_back(node);
                else walk((*f)->contains);
            } else if (ids.count(std::get<std::shared_ptr<clash::RuleCheck>>(node)->id)) {
                nodes.push_back(node);
            }
        }
    };
    walk(document_.file().contains);
    return nodes;
}

std::shared_ptr<clash::RuleCheck> RuleTreeWidget::createRule(const QString& type) const {
    auto rule = std::make_shared<clash::RuleCheck>();
    rule->type = type;
    rule->id = clash::json::newId();
    if (const auto* spec = clash::RuleCatalogue::instance().find(type)) {
        for (const clash::ParamSpec& p : spec->params) rule->params[p.name] = p.default_value;
        rule->viewer.label = spec->label;
    }
    return rule;
}

void RuleTreeWidget::addRule(const QString& type, const QString& folder_id, int index) {
    if (!document_.isOpen()) document_.newFile();
    std::vector<clash::RuleNode>* nodes = document_.contents(folder_id);
    if (nodes == nullptr) return;
    auto rule = createRule(type);
    if (index < 0 || index > static_cast<int>(nodes->size())) nodes->push_back(rule);
    else nodes->insert(nodes->begin() + index, rule);
    document_.touch();
}

void RuleTreeWidget::addFolder(const QString& parent_folder_id) {
    if (!document_.isOpen()) document_.newFile();
    std::vector<clash::RuleNode>* nodes = document_.contents(parent_folder_id);
    if (nodes == nullptr) return;
    auto folder = std::make_shared<clash::RuleFolder>();
    folder->id = clash::json::newId();
    folder->viewer.label = QStringLiteral("Folder");
    nodes->push_back(folder);
    document_.touch();
}

QString RuleTreeWidget::targetFolderForCurrent() const {
    for (QTreeWidgetItem* item = currentItem(); item; item = item->parent()) {
        const auto ref = refOf(item);
        if (ref && ref->kind == TreeRef::Kind::Folder) return ref->folder->id;
    }
    return {};
}

// --- drag and drop ---------------------------------------------------------------------

QMimeData* RuleTreeWidget::mimeData(const QList<QTreeWidgetItem*>& items) const {
    auto* mime = new QMimeData;
    if (!items.isEmpty()) mime->setData(kTreeMime, keyOf(items.front()).toUtf8());
    return mime;
}

void RuleTreeWidget::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasFormat(kLibraryMime) || event->mimeData()->hasFormat(kTreeMime)) event->acceptProposedAction();
    else event->ignore();
}

void RuleTreeWidget::dragMoveEvent(QDragMoveEvent* event) {
    if (event->mimeData()->hasFormat(kLibraryMime) || event->mimeData()->hasFormat(kTreeMime)) event->acceptProposedAction();
    else event->ignore();
}

void RuleTreeWidget::dropEvent(QDropEvent* event) {
    QTreeWidgetItem* target = itemAt(event->position().toPoint());
    const QMimeData* mime = event->mimeData();
    if (mime->hasFormat(kLibraryMime)) {
        dropLibraryItem(QString::fromUtf8(mime->data(kLibraryMime)), target);
    } else if (mime->hasFormat(kTreeMime)) {
        const QString key = QString::fromUtf8(mime->data(kTreeMime));
        QTreeWidgetItem* dragged = nullptr;
        for (auto i = keys_.begin(); i != keys_.end(); ++i) {
            if (i.value() == key) dragged = i.key();
        }
        if (dragged && dragged != target) dropTreeItem(dragged, target);
    }
    event->setDropAction(Qt::CopyAction); // the model moves the nodes, not the view
    event->accept();
}

void RuleTreeWidget::dropLibraryItem(const QString& payload, QTreeWidgetItem* target) {
    const auto ref = refOf(target);
    if (payload.startsWith("rule:")) {
        const QString type = payload.mid(5);
        if (!ref || ref->kind == TreeRef::Kind::File) {
            addRule(type, {});
        } else if (ref->kind == TreeRef::Kind::Folder) {
            addRule(type, ref->folder->id);
        } else if (ref->kind == TreeRef::Kind::Rule && !ref->nested) {
            size_t index = 0;
            std::vector<clash::RuleNode>* container = document_.containerOf(ref->rule->id, &index);
            if (container == nullptr) return;
            container->insert(container->begin() + static_cast<std::ptrdiff_t>(index) + 1, createRule(type));
            document_.touch();
        } else if (ref->kind == TreeRef::Kind::Slot) {
            // A rule nested in the slot: its output feeds the slot (cascade).
            clash::Select* select = ref->select();
            *select = clash::Select{};
            select->kind = clash::Select::Kind::Rule;
            select->rule = createRule(type);
            document_.touch();
        }
        return;
    }
    if (payload.startsWith("facet:")) {
        if (!ref || (ref->kind != TreeRef::Kind::Slot && ref->kind != TreeRef::Kind::Facet)) {
            services::popup::information(this, "Drop an IDS facet on a slot (source, target or context) of a rule.");
            return;
        }
        const QString name = payload.mid(6);
        const std::string key = name == QLatin1String("PartOf") ? std::string("partOf") : name.toLower().toStdString();
        const auto type = it::facetTypeFromName(key);
        if (!type) return;
        clash::Select* select = ref->select();
        if (select->kind != clash::Select::Kind::Facet) {
            *select = clash::Select{};
            select->kind = clash::Select::Kind::Facet;
        }
        select->applicability.push_back(it::Facet::make(*type));
        document_.touch();
    }
}

void RuleTreeWidget::dropTreeItem(QTreeWidgetItem* dragged, QTreeWidgetItem* target) {
    const auto from = refOf(dragged);
    const auto to = refOf(target);
    if (!from) return;
    if (from->kind == TreeRef::Kind::Facet) {
        if (!to || (to->kind != TreeRef::Kind::Slot && to->kind != TreeRef::Kind::Facet)) return;
        clash::Select* source = from->select();
        clash::Select* destination = to->select();
        if (source == destination || from->facet < 0 || from->facet >= static_cast<int>(source->applicability.size())) return;
        const it::Facet facet = source->applicability[static_cast<size_t>(from->facet)];
        source->applicability.erase(source->applicability.begin() + from->facet);
        if (source->applicability.empty()) *source = clash::Select{};
        if (destination->kind != clash::Select::Kind::Facet) {
            *destination = clash::Select{};
            destination->kind = clash::Select::Kind::Facet;
        }
        destination->applicability.push_back(facet);
        document_.touch();
        return;
    }
    const bool is_folder = from->kind == TreeRef::Kind::Folder;
    const bool is_rule = from->kind == TreeRef::Kind::Rule && !from->nested;
    if (!is_folder && !is_rule) return;
    const QString id = is_folder ? from->folder->id : from->rule->id;

    if (to && to->kind == TreeRef::Kind::Slot && is_rule) {
        // Cascade: the dragged rule feeds the slot of the target rule.
        if (to->rule == from->rule || document_.wouldLoop(from->rule->id, to->rule->id)) {
            services::popup::information(this, "This cascade would loop: a rule cannot feed itself.");
            return;
        }
        clash::Select* select = to->select();
        *select = clash::Select{};
        select->kind = clash::Select::Kind::Rule;
        select->rule = clash::json::cloneRule(*from->rule);
        document_.touch();
        return;
    }

    std::vector<clash::RuleNode>* destination = nullptr;
    int index = -1;
    if (!to || to->kind == TreeRef::Kind::File) {
        destination = &document_.file().contains;
    } else if (to->kind == TreeRef::Kind::Folder) {
        if (is_folder && (to->folder == from->folder || containsFolder(*from->folder, to->folder->id))) return;
        destination = &to->folder->contains;
    } else if (to->kind == TreeRef::Kind::Rule && !to->nested) {
        if (to->rule == from->rule) return;
        destination = nullptr; // resolved after the take (the index may move)
    } else {
        return;
    }
    const auto node = document_.take(id);
    if (!node) return;
    if (destination == nullptr) {
        size_t position = 0;
        destination = document_.containerOf(to->rule->id, &position);
        if (destination == nullptr) destination = &document_.file().contains;
        index = static_cast<int>(position);
    }
    if (index < 0) destination->push_back(*node);
    else destination->insert(destination->begin() + index, *node);
    document_.touch();
}

// --- context menus ------------------------------------------------------------------------

void RuleTreeWidget::addRuleMenu(QMenu* menu, const QString& folder_id) {
    QMenu* add = menu->addMenu("Add Rule");
    for (const QString& family : kFamilies) {
        QMenu* section = add->addMenu(familyTitle(family));
        for (const clash::RuleSpec& spec : clash::RuleCatalogue::instance().rules()) {
            if (spec.family != family) continue;
            QAction* action = section->addAction(spec.label, this, [this, type = spec.type, folder_id]() { addRule(type, folder_id); });
            action->setToolTip(spec.doc);
        }
        section->setToolTipsVisible(true);
    }
}

void RuleTreeWidget::removeRef(const TreeRef& ref) {
    switch (ref.kind) {
    case TreeRef::Kind::Folder:
        document_.take(ref.folder->id);
        break;
    case TreeRef::Kind::Rule:
        if (ref.nested) {
            // The nested rule leaves its slot: find the owner's slot holding it.
            for (const auto& owner_ref : refs_) {
                if (owner_ref.kind != TreeRef::Kind::Slot) continue;
                clash::Select* select = owner_ref.select();
                if (select && select->kind == clash::Select::Kind::Rule && select->rule == ref.rule) *select = clash::Select{};
            }
        } else {
            document_.take(ref.rule->id);
        }
        break;
    case TreeRef::Kind::Slot:
    case TreeRef::Kind::Link:
        if (clash::Select* select = ref.select()) *select = clash::Select{};
        break;
    case TreeRef::Kind::Facet:
        if (clash::Select* select = ref.select(); select && ref.facet >= 0 && ref.facet < static_cast<int>(select->applicability.size())) {
            select->applicability.erase(select->applicability.begin() + ref.facet);
            if (select->applicability.empty()) *select = clash::Select{};
        }
        break;
    case TreeRef::Kind::File:
        return;
    }
    document_.touch();
}

void RuleTreeWidget::contextMenuEvent(QContextMenuEvent* event) {
    QTreeWidgetItem* item = itemAt(event->pos());
    const auto ref = refOf(item);
    QMenu menu(this);
    menu.setToolTipsVisible(true);
    if (!ref) {
        if (!document_.isOpen()) return;
        menu.addAction("Add Folder", this, [this]() { addFolder({}); });
        addRuleMenu(&menu, {});
        menu.exec(event->globalPos());
        return;
    }
    switch (ref->kind) {
    case TreeRef::Kind::File:
        menu.addAction("Run the Rule File", this, [this]() { emit runRequested(document_.file().contains); });
        menu.addSeparator();
        menu.addAction("Add Folder", this, [this]() { addFolder({}); });
        addRuleMenu(&menu, {});
        if (clipboard_) {
            menu.addAction("Paste Rule", this, [this]() {
                auto copy = clash::json::cloneRule(*clipboard_);
                copy->id = clash::json::newId();
                document_.file().contains.push_back(copy);
                document_.touch();
            });
        }
        menu.addSeparator();
        menu.addAction("Delete the Rule File", this, [this]() {
            if (!services::popup::confirmation(this, "Delete the rule file, its folders and rules? (the file on disk "
                                                     "is kept)")) {
                return;
            }
            document_.newFile();
        });
        break;
    case TreeRef::Kind::Folder: {
        const auto folder = ref->folder;
        menu.addAction("Run Folder", this, [this, folder]() { emit runRequested({folder}); });
        menu.addSeparator();
        menu.addAction("Edit Folder", this, [this, folder]() { emit editFolderRequested(folder); });
        menu.addAction("Add Folder", this, [this, folder]() { addFolder(folder->id); });
        addRuleMenu(&menu, folder->id);
        if (clipboard_) {
            menu.addAction("Paste Rule", this, [this, folder]() {
                auto copy = clash::json::cloneRule(*clipboard_);
                copy->id = clash::json::newId();
                folder->contains.push_back(copy);
                document_.touch();
            });
        }
        menu.addSeparator();
        menu.addAction("Delete Folder", this, [this, r = *ref]() {
            if (services::popup::confirmation(this, "Delete the folder and all its rules?")) removeRef(r);
        });
        break;
    }
    case TreeRef::Kind::Rule: {
        const auto rule = ref->rule;
        if (!ref->nested) {
            menu.addAction("Run Rule", this, [this, rule]() { emit runRequested({rule}); });
            menu.addSeparator();
        }
        menu.addAction("Edit Rule", this, [this, rule]() { emit editRuleRequested(rule); });
        QAction* display = menu.addAction("Display Input", this, [this, rule]() { emit displayInputRequested(rule); });
        display->setToolTip("Run the rule and draw, over the 3D view, the zones (dashed magenta) and the boxes "
                            "(yellow) it checked. The results, colours and visibility do not change.");
        menu.addAction("Show the source elements", this, [this, rule]() {
            emit selectionRequested(rule->source, RuleDocument::labelOf(*rule) + ": source");
        });
        if (const auto* spec = clash::RuleCatalogue::instance().find(rule->type); spec && spec->isTwoObject()) {
            menu.addAction("Show the target elements", this, [this, rule]() {
                emit selectionRequested(rule->target, RuleDocument::labelOf(*rule) + ": target");
            });
        }
        menu.addAction("Copy Rule", this, [this, rule]() { clipboard_ = clash::json::cloneRule(*rule); });
        menu.addSeparator();
        menu.addAction(ref->nested ? "Remove from the slot" : "Delete Rule", this, [this, r = *ref]() { removeRef(r); });
        break;
    }
    case TreeRef::Kind::Slot:
    case TreeRef::Kind::Facet:
    case TreeRef::Kind::Link: {
        clash::Select* select = ref->select();
        if (select && !select->empty()) {
            menu.addAction("Show the selection", this, [this, r = *ref]() {
                emit selectionRequested(*r.select(), RuleDocument::labelOf(*r.rule) + ": " + r.slot);
            });
        }
        if (ref->kind == TreeRef::Kind::Link && select && select->rule) {
            QMenu* pass = menu.addMenu("Elements passed");
            const bool two = [&]() {
                const auto* spec = clash::RuleCatalogue::instance().find(select->rule->type);
                return spec && spec->isTwoObject();
            }();
            for (const auto& [element, state, label] : std::vector<std::tuple<QString, bool, QString>>{
                     {"source", true, "Sources of the results"},
                     {"target", true, "Targets of the results"},
                     {"source", false, "Sources without result"},
                     {"target", false, "Targets without result"}}) {
                if (element == QLatin1String("target") && !two) continue;
                QAction* action = pass->addAction(label, this, [this, select, element = element, state = state]() {
                    select->element_to_pass = element;
                    select->element_state_to_pass = state;
                    document_.touch();
                });
                action->setCheckable(true);
                action->setChecked(select->element_to_pass == element && select->element_state_to_pass == state);
            }
        }
        // Copy / paste the elements of the slot (to another slot, rule or rule file).
        menu.addSeparator();
        QAction* copy = menu.addAction("Copy the slot", this, [select]() { slot_clipboard::copy(*select); });
        copy->setEnabled(select && !select->empty());
        copy->setToolTip("Copy the elements of this slot (IDS facets, or the rule they come from), "
                         "to paste them into another slot.");
        QAction* paste = menu.addAction("Paste into the slot", this, [this, r = *ref]() {
            auto pasted = slot_clipboard::paste();
            clash::Select* target = r.select();
            if (!pasted || !target) return;
            const QString error = slot_clipboard::pasteError(*pasted, *r.rule, document_);
            if (!error.isEmpty()) {
                services::popup::information(this, error);
                return;
            }
            *target = std::move(*pasted);
            document_.touch();
        });
        paste->setEnabled(select && slot_clipboard::canPaste());
        paste->setToolTip("Replace the elements of this slot by the copied ones (a copied slot, or IDS facets "
                          "copied from the filters, the tables or the IDS panel).");
        if (ref->kind != TreeRef::Kind::Slot || (select && !select->empty())) {
            menu.addAction(ref->kind == TreeRef::Kind::Facet ? "Remove Facet" : "Clear the slot", this, [this, r = *ref]() { removeRef(r); });
        }
        break;
    }
    }
    if (!menu.isEmpty()) menu.exec(event->globalPos());
}

// --- RulePanel ------------------------------------------------------------------------------

RulePanel::RulePanel(RuleDocument& document, QWidget* parent) : QWidget(parent), document_(document) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);
    const auto button = [this](const QString& text, const QString& tip) {
        auto* b = new QPushButton(text, this);
        b->setToolTip(QString("<b>%1</b><br>%2").arg(text, tip));
        return b;
    };
    auto* save = button("Save", "Save the rule file as JSON (the format of the IfcClash_Plus command line); the location is asked each time.");
    auto* load = button("Load", "Open a rule file (JSON).");
    auto* create_file = button("New", "A new, empty rule file (replaces the open one).");
    auto* create_rule = button("Create Rule", "Add a rule to the selected folder (or to the file).");
    auto* create_folder = button("Create Folder", "Add a folder to the selected folder (or to the file).");
    auto* run = button("Run", "Run the selected rules and folders, or the whole file when nothing is selected. "
                              "A deactivated folder (grey) does not run.");
    auto* rule_menu = new QMenu(create_rule);
    rule_menu->setToolTipsVisible(true);
    create_rule->setMenu(rule_menu);
    connect(rule_menu, &QMenu::aboutToShow, this, [this, rule_menu]() {
        rule_menu->clear();
        for (const QString& family : kFamilies) {
            QMenu* section = rule_menu->addMenu(familyTitle(family));
            section->setToolTipsVisible(true);
            for (const clash::RuleSpec& spec : clash::RuleCatalogue::instance().rules()) {
                if (spec.family != family) continue;
                QAction* action = section->addAction(spec.label, this, [this, type = spec.type]() {
                    tree_->addRule(type, tree_->targetFolderForCurrent());
                });
                action->setToolTip(spec.doc);
            }
        }
    });
    auto* row1 = new QHBoxLayout();
    row1->addWidget(save);
    row1->addWidget(load);
    row1->addWidget(create_file);
    auto* row2 = new QHBoxLayout();
    row2->addWidget(create_rule);
    row2->addWidget(create_folder);
    row2->addWidget(run);
    layout->addLayout(row1);
    layout->addLayout(row2);
    tree_ = new RuleTreeWidget(document_, this);
    layout->addWidget(tree_, 1);

    connect(save, &QPushButton::clicked, this, &RulePanel::saveRequested);
    connect(load, &QPushButton::clicked, this, &RulePanel::loadRequested);
    connect(create_file, &QPushButton::clicked, this, [this]() {
        if (document_.isOpen() && document_.isModified() &&
            !services::popup::confirmation(this, "The open rule file is not saved. Replace it with a new one?")) {
            return;
        }
        document_.newFile();
    });
    connect(create_folder, &QPushButton::clicked, this, [this]() { tree_->addFolder(tree_->targetFolderForCurrent()); });
    connect(run, &QPushButton::clicked, this, [this]() {
        std::vector<clash::RuleNode> nodes = tree_->selectedNodes();
        if (nodes.empty()) nodes = document_.file().contains;
        emit tree_->runRequested(nodes);
    });
}

} // namespace bonsaiviewer::modules::rule
