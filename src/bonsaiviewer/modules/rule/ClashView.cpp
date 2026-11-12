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

#include "ClashView.h"

#include "ClashTopic.h"
#include "RuleEditDialog.h"
#include "RulePanels.h"
#include "RuleTree.h"

#include "../../SessionState.h"
#include "../bcf/Author.h"
#include "../bcf/View.h"
#include "../../components/Buttons.h"
#include "../../components/FlowLayout.h"
#include "../../services/ActionPipeline.h"
#include "../../services/ActiveView.h"
#include "../../services/AppearanceService.h"
#include "../../services/FilterRunner.h"
#include "../../services/IdsBridge.h"
#include "../../services/PopupService.h"
#include "../../services/SelectionService.h"
#include "../../services/UndoStack.h"
#include "../../services/clash/RuleCatalogue.h"
#include "../../services/clash/RuleJson.h"
#include "../viewport/Commands.h"
#include "../../../ifcviewer/Federation.h"
#include "../../../ifcviewer/OverlayRenderer.h"
#include "../../../ifcviewer/SceneLoader.h"
#include "../../../ifcviewer/ViewportWindow.h"

#include "api/Engine.h"

#include <QActionGroup>
#include <QColor>
#include <QDateTime>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QInputDialog>
#include <QLabel>
#include <QJsonDocument>
#include <QMenu>
#include <QMessageBox>
#include <QProgressDialog>
#include <QTimer>
#include <QSaveFile>
#include <QSettings>
#include <QToolButton>

#include <Eigen/Dense>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <set>
#include <tuple>

namespace bonsaiviewer::modules::rule {

namespace {

constexpr auto kLastDirectoryKey = "clash/lastDirectory";
using Layer = bonsaiviewer::services::AppearanceService::Layer;

const QColor kSourceColour("#E6194B");
const QColor kTargetColour("#F08A24");
const QColor kPassColour("#3CB44B");

// The engine works in the IFC world coordinates of the model: the overlay
// goes through the model's federation matrix (TI09).
struct Transform {
    Eigen::Matrix4d m = Eigen::Matrix4d::Identity();
    void push(std::vector<float>& out, const ifcclash::Point3& p) const {
        const Eigen::Vector4d v = m * Eigen::Vector4d(p[0], p[1], p[2], 1.0);
        out.push_back(static_cast<float>(v.x()));
        out.push_back(static_cast<float>(v.y()));
        out.push_back(static_cast<float>(v.z()));
    }
    void segment(std::vector<float>& out, const ifcclash::Point3& a, const ifcclash::Point3& b) const {
        push(out, a);
        push(out, b);
    }
};

void obbEdges(const Transform& t, const ifcclash::ObbData& obb, std::vector<float>& out) {
    std::array<ifcclash::Point3, 8> corners;
    for (int i = 0; i < 8; ++i) {
        const double signs[3] = {(i & 1) ? 1.0 : -1.0, (i & 2) ? 1.0 : -1.0, (i & 4) ? 1.0 : -1.0};
        for (int c = 0; c < 3; ++c) {
            double v = obb.center[static_cast<size_t>(c)];
            for (int a = 0; a < 3; ++a) v += signs[a] * obb.half_sizes[static_cast<size_t>(a)] * obb.axes[static_cast<size_t>(a)][static_cast<size_t>(c)];
            corners[static_cast<size_t>(i)][static_cast<size_t>(c)] = v;
        }
    }
    static constexpr int kEdges[12][2] = {{0, 1}, {1, 3}, {3, 2}, {2, 0}, {4, 5}, {5, 7}, {7, 6}, {6, 4}, {0, 4}, {1, 5}, {3, 7}, {2, 6}};
    for (const auto& e : kEdges) t.segment(out, corners[static_cast<size_t>(e[0])], corners[static_cast<size_t>(e[1])]);
}

OverlayRenderer::LineGroup lineGroup(const QColor& colour, float width, float dash = 0.0f) {
    OverlayRenderer::LineGroup group;
    group.color[0] = static_cast<float>(colour.redF());
    group.color[1] = static_cast<float>(colour.greenF());
    group.color[2] = static_cast<float>(colour.blueF());
    group.color[3] = 0.95f;
    group.line_width = width;
    group.dash_period_px = dash;
    return group;
}

bool isIfcPath(const QString& path) {
    const QString suffix = QFileInfo(path).suffix().toLower();
    return (suffix == QLatin1String("ifc") || suffix == QLatin1String("ifcxml")) && QFileInfo::exists(path);
}

} // namespace

ClashView::ClashView(Services services, QWidget* host, QObject* parent) : QObject(parent), s_(services), host_(host) {
    rule_panel_ = new RulePanel(document_, host_);
    library_ = new RuleLibraryPanel(host_);
    config_ = new RuleConfigPanel(document_, host_);
    results_ = new ResultPanel(host_);
    info_ = new ResultInfoPanel(host_);
    geometry_ribbon_ = buildGeometryRuleRibbon();
    filter_ribbon_ = buildClashFilterRibbon();
    document_.newFile();

    RuleTreeWidget* tree = rule_panel_->tree();
    connect(tree, &RuleTreeWidget::refSelected, config_, &RuleConfigPanel::showRef);
    connect(tree, &RuleTreeWidget::refSelected, this, &ClashView::showResultsOf);
    connect(library_, &RuleLibraryPanel::ruleTypeSelected, config_, &RuleConfigPanel::showLibraryRule);
    config_->setFacetContext(&s_.ids, [this]() { return services::FilterRunner::activeIfcPath(s_.session); });
    connect(config_, &RuleConfigPanel::isolateRequested, this, &ClashView::isolateExpressIds);
    connect(tree, &RuleTreeWidget::displayInputRequested, this, &ClashView::displayInput);
    connect(tree, &RuleTreeWidget::runRequested, this, &ClashView::run);
    connect(tree, &RuleTreeWidget::editRuleRequested, this, &ClashView::editRule);
    connect(tree, &RuleTreeWidget::editFolderRequested, this, &ClashView::editFolder);
    connect(config_, &RuleConfigPanel::editRuleRequested, this, &ClashView::editRule);
    connect(config_, &RuleConfigPanel::editFolderRequested, this, &ClashView::editFolder);
    connect(tree, &RuleTreeWidget::selectionRequested, this, &ClashView::showSelection);
    connect(rule_panel_, &RulePanel::saveRequested, this, [this]() { saveRules(); });
    connect(rule_panel_, &RulePanel::loadRequested, this, &ClashView::loadRules);

    connect(results_, &ResultPanel::currentResultChanged, this, [this](int row) {
        info_->showRow(row >= 0 ? &results_->rows()[static_cast<size_t>(row)] : nullptr);
        drawOverlay(row);
    });
    connect(results_, &ResultPanel::rowsSelected, this, [this](const std::vector<int>& rows) {
        std::vector<std::uint32_t> ids;
        for (int row : rows) {
            const auto objects = results_->rows()[static_cast<size_t>(row)].objects();
            ids.insert(ids.end(), objects.begin(), objects.end());
        }
        s_.selection.setSelection(ids, services::SelectionSource::Table, services::SelectionMode::Replace);
    });
    connect(results_, &ResultPanel::isolateRequested, this, &ClashView::isolateRows);
    // Result Info: select the source or the target (the Properties panel
    // shows it), and the review of the result.
    connect(info_, &ResultInfoPanel::selectRequested, this, [this](const std::vector<std::uint32_t>& objects) {
        if (!objects.empty()) s_.selection.setSelection(objects, services::SelectionSource::Table, services::SelectionMode::Replace);
    });
    connect(info_, &ResultInfoPanel::reviewRequested, this, [this](ReviewStatus status) {
        const auto rows = results_->selectedRows();
        if (!rows.empty()) results_->setReviewOf(rows, status);
    });
    // The Clash section of the Properties panel: kept up to date, its actions.
    connect(results_, &ResultPanel::reviewChanged, this, &ClashView::publishIndex);
    connect(&s_.index, &clash::ObjectClashIndex::showResultRequested, this, [this](int row) {
        if (row < 0 || row >= static_cast<int>(results_->rows().size())) return;
        const QString node = results_->rows()[static_cast<size_t>(row)].node_id;
        results_->setScope(std::set<QString>{node}, "Results of " + results_->rows()[static_cast<size_t>(row)].rule_label);
        results_->selectRow(row);
        emit showResultsRequested();
    });
    connect(&s_.index, &clash::ObjectClashIndex::isolateRequested, this, [this](int row) { isolateRows({row}); });
    connect(&s_.index, &clash::ObjectClashIndex::reviewRequested, this, [this](int row, const QString& review) {
        results_->setReviewOf({row}, review == QLatin1String("Valid")     ? ReviewStatus::Valid
                                     : review == QLatin1String("Invalid") ? ReviewStatus::Invalid
                                                                           : ReviewStatus::NonDetermined);
    });
    connect(results_, &ResultPanel::reviewChanged, this, [this]() {
        const auto rows = results_->selectedRows();
        if (rows.size() == 1) info_->showRow(&results_->rows()[static_cast<size_t>(rows.front())]);
    });
    connect(results_, &ResultPanel::showRequested, this, [this](const std::vector<int>& rows) {
        std::vector<std::uint32_t> ids;
        for (int row : rows) {
            const auto objects = results_->rows()[static_cast<size_t>(row)].objects();
            ids.insert(ids.end(), objects.begin(), objects.end());
        }
        // Everything shown again, the objects of the results selected.
        s_.view.showAll();
        s_.selection.setSelection(ids, services::SelectionSource::Table, services::SelectionMode::Replace);
    });

    // The results point at the elements of the models by model id and
    // GlobalId: a change of the loaded models resolves them again (a result
    // of a model no longer loaded stays, without objects); a new project
    // drops them.
    connect(&s_.session, &SessionState::modelsChanged, this, [this]() {
        engine_key_.clear();
        if (busy_) return;
        engine_.reset();
        clearOverlay();
        resolveShownRows();
    });
    connect(&s_.session, &SessionState::projectReset, this, [this]() {
        engine_key_.clear();
        if (busy_) return;
        engine_.reset();
        clearClash();
    });
    // The federation keeps the rules, the last run of each rule and the
    // results with their review, group and topic.
    connect(&s_.session, &SessionState::projectAboutToSave, this, &ClashView::saveToFederation);
    connect(&s_.session, &SessionState::projectOpened, this, [this](const QString&) { restoreFromFederation(); });
    // The rules are saved with the project: the rule file is no longer "modified".
    connect(&s_.session, &SessionState::projectSaved, this, [this](const QString&) {
        if (document_.isOpen() && document_.isModified()) document_.markSaved();
    });
    connect(&s_.session, &SessionState::modelGeometryReady, this, [this](std::uint32_t) { tryRestore(); });
    if (auto* loader = s_.session.loader()) connect(loader, &SceneLoader::allLoadsFinished, this, &ClashView::tryRestore);
    connect(results_, &ResultPanel::reviewChanged, this, &ClashView::markProjectDirty);
    connect(results_, &ResultPanel::createTopicsRequested, this, &ClashView::createTopics);
    connect(results_, &ResultPanel::addToTopicRequested, this, &ClashView::addToTopic);
    connect(results_, &ResultPanel::openTopicRequested, this, [this](const QString& guid) {
        if (bcf_ && bcf_->topic(guid)) bcf_->openTopic(guid);
        else s_.session.setStatusMessage("Clash", "The topic is not in the loaded BCF file: load it in the BCF panel");
    });
    connect(results_, &ResultPanel::unlinkTopicRequested, this,
            [this](const std::vector<int>& rows) { results_->setTopicOf(rows, QString()); });
    info_->setTopicTitle([this](const QString& guid) { return results_->topicTitle(guid); });
    connect(&document_, &RuleDocument::changed, this, [this]() {
        if (document_.isModified()) markProjectDirty();
    });
}

ClashView::~ClashView() {
    if (cancel_) cancel_->cancel();
    if (worker_.joinable()) worker_.join();
}

// --- runs ------------------------------------------------------------------------------------

bool ClashView::prepareModels(QString* error) {
    auto* loader = s_.session.loader();
    run_models_.clear();
    run_paths_.clear();
    QStringList key;
    QStringList skipped;
    for (std::uint32_t id : s_.session.sessionModelIds()) {
        const QString path = loader ? loader->filePath(id) : QString();
        if (!isIfcPath(path)) {
            skipped << (loader ? loader->displayName(id) : QString::number(id));
            continue;
        }
        run_models_.push_back(id);
        run_paths_ << path;
        key << path + "@" + QString::number(QFileInfo(path).lastModified().toMSecsSinceEpoch());
    }
    if (run_models_.empty()) {
        *error = skipped.isEmpty() ? QStringLiteral("Load an IFC model before running the rules.")
                                   : QStringLiteral("The rules run on IFC files: the loaded models have no IFC file on disk.");
        return false;
    }
    if (!skipped.isEmpty()) {
        run_messages_.emplace_back(QString(), "Left out (no IFC file on disk): " + skipped.join(", "));
    }
    if (key != engine_key_) {
        engine_.reset(); // the worker opens a new session
        engine_key_ = key;
    }
    elements_.clear();
    if (auto* registry = s_.session.elementRegistry()) {
        for (std::uint32_t id : run_models_) {
            QHash<int, BasicElementInfo> by_id;
            for (const BasicElementInfo& info : registry->basicElementInfoForModel(id)) by_id.insert(info.ifc_id, info);
            elements_.insert(id, by_id);
        }
    }
    return true;
}

bool ClashView::startWork(const QString& label, std::function<void(ifcclash::Session&, size_t)> work) {
    if (busy_) {
        s_.session.setStatusMessage("Clash", "A run is in progress");
        return false;
    }
    if (worker_.joinable()) worker_.join();
    busy_ = true;
    cancel_ = std::make_unique<ifcclash::CancelToken>();
    if (run_button_) run_button_->setEnabled(false);
    if (stop_button_) stop_button_->setEnabled(true);
    s_.session.beginProgress(label);
    s_.session.setStatusMessage("Clash", label + "…");
    std::vector<std::filesystem::path> paths;
    for (const QString& path : run_paths_) paths.emplace_back(path.toStdString());
    auto engine = engine_;
    worker_ = std::thread([this, paths, engine, work]() mutable {
        QString error;
        try {
            if (!engine) {
                ifcclash::SessionOptions options;
                options.triangle_source = "auto";
                engine = std::make_shared<ifcclash::Session>(paths, options);
            }
            work(*engine, engine->file_count());
        } catch (const ifcclash::Cancelled&) {
        } catch (const std::exception& e) {
            error = QString::fromUtf8(e.what());
        } catch (...) {
            error = QStringLiteral("unknown error");
        }
        const bool cancelled = cancel_ && cancel_->cancelled();
        QMetaObject::invokeMethod(
            this,
            [this, engine, cancelled, error]() {
                engine_ = engine;
                onRunFinished(cancelled, error);
            },
            Qt::QueuedConnection);
    });
    return true;
}

void ClashView::run(const std::vector<clash::RuleNode>& nodes) {
    if (busy_) {
        s_.session.setStatusMessage("Clash", "A run is in progress");
        return;
    }
    if (nodes.empty() || clash::countRules(nodes) == 0) {
        services::popup::information(host_, "There is no rule to run: add rules from the Rule Library.");
        return;
    }
    run_messages_.clear();
    QString error;
    if (!prepareModels(&error)) {
        services::popup::information(host_, error);
        return;
    }
    // The run works on a copy: the tree stays editable meanwhile.
    document_.refreshLinks();
    std::vector<clash::RuleNode> copy;
    for (const clash::RuleNode& node : nodes) {
        if (const auto* f = std::get_if<std::shared_ptr<clash::RuleFolder>>(&node)) copy.push_back(clash::json::nodeFromJson(clash::json::toJson(**f)));
        else copy.push_back(clash::json::cloneRule(*std::get<std::shared_ptr<clash::RuleCheck>>(node)));
    }
    pending_rows_.clear();
    pending_nodes_.clear();
    pending_inputs_.clear();
    pending_saved_nodes_.clear();
    pending_run_ = store::SavedRun{};
    pending_run_.id = clash::json::newId();
    pending_run_.started = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    for (size_t i = 0; i < run_models_.size(); ++i) {
        const QFileInfo info(run_paths_.value(static_cast<int>(i)));
        store::SavedModel model;
        model.model_id = s_.session.modelIdForSessionModelId(run_models_[i]);
        model.name = s_.session.loader() ? s_.session.loader()->displayName(run_models_[i]) : info.fileName();
        model.path = info.absoluteFilePath();
        model.mtime = info.lastModified().toMSecsSinceEpoch();
        model.size = info.size();
        pending_run_.models.push_back(model);
    }
    startWork("Clash rules", [this, copy](ifcclash::Session& session, size_t files) {
        clash::Orchestrator::Callbacks callbacks;
        callbacks.finished = [this](const clash::RuleOutcome& outcome) {
            QMetaObject::invokeMethod(this, [this, outcome]() { onOutcome(outcome); }, Qt::QueuedConnection);
        };
        callbacks.progress = [this](int done, int total) {
            QMetaObject::invokeMethod(this, [this, done, total]() { s_.session.setProgress(total > 0 ? done * 100 / total : 100); },
                                      Qt::QueuedConnection);
        };
        callbacks.started = [this](const QString&, const QString& label) {
            QMetaObject::invokeMethod(this, [this, label]() { s_.session.setStatusMessage("Clash", "Running " + label + "…"); },
                                      Qt::QueuedConnection);
        };
        clash::Orchestrator orchestrator(session, files, callbacks, *cancel_);
        orchestrator.run(copy);
    });
    setProperty("work", "run");
}

void ClashView::publishIndex() {
    using Entry = clash::ObjectClashIndex::Entry;
    std::unordered_map<std::uint32_t, std::vector<Entry>> entries;
    // The rules in the order of the tree (then the others: nested, removed).
    std::vector<QString> order;
    for (const auto& rule : document_.treeRules()) {
        if (rule_inputs_.count(rule->id)) order.push_back(rule->id);
    }
    for (const auto& [node, inputs] : rule_inputs_) {
        if (std::find(order.begin(), order.end(), node) == order.end()) order.push_back(node);
    }
    std::map<QString, std::vector<int>> rows_of;
    for (int i = 0; i < static_cast<int>(results_->rows().size()); ++i) rows_of[results_->rows()[static_cast<size_t>(i)].node_id].push_back(i);
    const auto reviewText = [](ReviewStatus status) {
        return status == ReviewStatus::Valid ? QStringLiteral("Valid") : status == ReviewStatus::Invalid ? QStringLiteral("Invalid") : QStringLiteral("Non Determined");
    };
    int rules = 0;
    for (const QString& node : order) {
        const RuleInputs& inputs = rule_inputs_.at(node);
        // A Must rule (its results are groups): left out, its working is to be redesigned.
        const auto& node_rows = rows_of[node];
        if (std::any_of(node_rows.begin(), node_rows.end(), [this](int i) { return results_->rows()[static_cast<size_t>(i)].complex; })) {
            continue;
        }
        ++rules;
        std::set<std::uint32_t> in_result;
        for (int i : rows_of[node]) {
            const ResultRow& row = results_->rows()[static_cast<size_t>(i)];
            const auto add = [&](std::uint32_t object, Entry::Role role, const QString& other) {
                Entry e;
                e.node_id = node;
                e.rule_label = inputs.label;
                e.rule_type = inputs.type;
                e.role = role;
                e.other = other;
                e.measure = row.measure;
                e.review = reviewText(row.review);
                e.group = row.manual_group;
                e.excepted = !row.status;
                e.row = i;
                entries[object].push_back(std::move(e));
                in_result.insert(object);
            };
            for (std::uint32_t object : row.source_objects) add(object, Entry::Role::Source, row.target_name);
            for (std::uint32_t object : row.target_objects) add(object, Entry::Role::Target, row.source_name);
        }
        // Checked without result, or not checked (no geometry).
        std::set<std::uint32_t> not_checked(inputs.not_checked.begin(), inputs.not_checked.end());
        std::set<std::uint32_t> seen;
        for (const auto* list : {&inputs.sources, &inputs.targets}) {
            for (std::uint32_t object : *list) {
                if (in_result.count(object) || !seen.insert(object).second) continue;
                Entry e;
                e.node_id = node;
                e.rule_label = inputs.label;
                e.rule_type = inputs.type;
                e.role = not_checked.count(object) ? Entry::Role::NotChecked : Entry::Role::Tested;
                entries[object].push_back(std::move(e));
            }
        }
    }
    s_.index.publish(std::move(entries), rules);
}

void ClashView::showResultsOf(const std::optional<TreeRef>& ref) {
    if (!ref) return;
    std::set<QString> ids;
    QString label;
    if (ref->kind == TreeRef::Kind::File) {
        if (results_->rows().empty()) return;
        results_->setScope(std::nullopt, QString()); // the whole file: every result
        return;
    }
    if (ref->kind == TreeRef::Kind::Rule && !ref->nested) {
        if (!results_->hasRun(ref->rule->id)) return; // never run: the panel stays as it is
        ids.insert(ref->rule->id);
        label = "Results of " + RuleDocument::labelOf(*ref->rule);
    } else if (ref->kind == TreeRef::Kind::Folder) {
        std::set<QString> folders{ref->folder->id};
        std::function<void(const std::vector<clash::RuleNode>&)> walk = [&](const std::vector<clash::RuleNode>& nodes) {
            for (const auto& node : nodes) {
                if (const auto* f = std::get_if<std::shared_ptr<clash::RuleFolder>>(&node)) {
                    folders.insert((*f)->id);
                    walk((*f)->contains);
                } else if (results_->hasRun(std::get<std::shared_ptr<clash::RuleCheck>>(node)->id)) {
                    ids.insert(std::get<std::shared_ptr<clash::RuleCheck>>(node)->id);
                }
            }
        };
        walk(ref->folder->contains);
        if (ids.empty()) return;
        ids.insert(folders.begin(), folders.end());
        label = "Results of the folder " + RuleDocument::labelOf(*ref->folder);
    } else {
        return;
    }
    results_->setScope(ids, label);
    info_->showRow(nullptr);
}

void ClashView::stop() {
    if (busy_ && cancel_) {
        cancel_->cancel();
        s_.session.setStatusMessage("Clash", "Stopping after the current rule…");
    }
}

void ClashView::onOutcome(const clash::RuleOutcome& outcome) {
    if (!outcome.rule_type.isEmpty()) {
        pending_nodes_.insert(outcome.node_id);
        RuleInputs& inputs = pending_inputs_[outcome.node_id];
        inputs.label = outcome.label;
        inputs.type = outcome.rule_type;
        inputs.sources = objectsOf(outcome.sources);
        inputs.targets = objectsOf(outcome.targets);
        inputs.not_checked = objectsOf(outcome.not_checked);
        store::SavedNode& saved = pending_saved_nodes_[outcome.node_id];
        saved.node_id = outcome.node_id;
        saved.run_id = pending_run_.id;
        saved.label = outcome.label;
        saved.type = outcome.rule_type;
        for (const auto& rule : document_.treeRules()) {
            if (rule->id == outcome.node_id) saved.rule = clash::json::toJson(*rule);
        }
        saved.sources = elementsOf(outcome.sources);
        saved.targets = elementsOf(outcome.targets);
        saved.not_checked = elementsOf(outcome.not_checked);
    }
    if (!outcome.skipped.isEmpty()) {
        result_texts_[outcome.node_id] = "skipped";
        const QString label = outcome.label.isEmpty() ? QStringLiteral("Folder") : outcome.label;
        run_messages_.emplace_back(outcome.node_id, QString("%1: skipped, %2").arg(label, outcome.skipped));
        return;
    }
    if (!outcome.error.isEmpty()) {
        result_texts_[outcome.node_id] = outcome.error == QLatin1String("cancelled") ? "stopped" : "error";
        if (outcome.error != QLatin1String("cancelled")) {
            run_messages_.emplace_back(outcome.node_id, QString("%1: %2").arg(outcome.label, outcome.error));
        }
        return;
    }
    std::vector<QString> group_of(outcome.results.size());
    for (const clash::ResultGroup& group : outcome.groups) {
        for (int index : group.results) {
            if (index >= 0 && index < static_cast<int>(group_of.size()) && group_of[static_cast<size_t>(index)].isEmpty()) {
                group_of[static_cast<size_t>(index)] = group.label;
            }
        }
    }
    int excepted = 0;
    for (size_t i = 0; i < outcome.results.size(); ++i) {
        if (!outcome.results[i].status) ++excepted;
        pending_rows_.push_back(rowOf(outcome, outcome.results[i], group_of[i]));
    }
    QString text = QString("%1 result%2").arg(outcome.results.size()).arg(outcome.results.size() == 1 ? "" : "s");
    if (excepted > 0) text += QString(" (%1 %2)").arg(excepted).arg(outcome.results.front().complex ? "failing" : "excepted");
    if (!outcome.results.empty() && outcome.results.front().complex) {
        text = QString("%1 group%2, %3 failing").arg(outcome.results.size()).arg(outcome.results.size() == 1 ? "" : "s").arg(excepted);
    }
    result_texts_[outcome.node_id] = text;
    for (const QString& message : outcome.messages) {
        run_messages_.emplace_back(outcome.node_id, QString("%1: %2").arg(outcome.label, message));
    }
}

void ClashView::onRunFinished(bool cancelled, const QString& error) {
    if (worker_.joinable()) worker_.join();
    busy_ = false;
    if (run_button_) run_button_->setEnabled(true);
    if (stop_button_) stop_button_->setEnabled(false);
    s_.session.endProgress();
    const QString work = property("work").toString();
    setProperty("work", QVariant());
    if (!error.isEmpty()) {
        services::popup::information(host_, "The clash engine failed: " + error);
        s_.session.setStatusMessage("Clash", "Failed");
        return;
    }
    if (work != QLatin1String("run")) return;
    clearOverlay();
    // The topics of the results of the rules run: those left without result are resolved.
    QSet<QString> topics_before;
    for (const ResultRow& row : results_->rows()) {
        if (pending_nodes_.count(row.node_id) && !row.topic_guid.isEmpty()) topics_before.insert(row.topic_guid);
    }
    // The results of the other rules stay (a click on a rule shows its own).
    results_->mergeResults(std::move(pending_rows_), pending_nodes_);
    for (const ResultRow& row : results_->rows()) topics_before.remove(row.topic_guid);
    resolved_topics_.unite(topics_before);
    pending_rows_.clear();
    for (const QString& node : pending_nodes_) rule_inputs_.erase(node);
    for (auto& [node, inputs] : pending_inputs_) rule_inputs_[node] = std::move(inputs);
    pending_inputs_.clear();
    // The last run of each rule, for the federation.
    for (const QString& node : pending_nodes_) saved_nodes_.erase(node);
    for (auto& [node, saved] : pending_saved_nodes_) {
        saved.text = result_texts_.value(node);
        saved_nodes_[node] = std::move(saved);
    }
    pending_saved_nodes_.clear();
    pending_run_.seconds = QDateTime::fromString(pending_run_.started, Qt::ISODate).msecsTo(QDateTime::currentDateTimeUtc()) / 1000.0;
    saved_runs_.push_back(pending_run_);
    saved_runs_.erase(std::remove_if(saved_runs_.begin(), saved_runs_.end(),
                                     [this](const store::SavedRun& run) {
                                         return std::none_of(saved_nodes_.begin(), saved_nodes_.end(),
                                                             [&run](const auto& n) { return n.second.run_id == run.id; });
                                     }),
                      saved_runs_.end());
    markProjectDirty();
    // The warnings by rule: a rule of the tree shows its own; the folders
    // left out of this run are in its scope.
    results_->mergeMessages(run_messages_, pending_nodes_);
    std::set<QString> last_run = pending_nodes_;
    for (const auto& message : run_messages_) last_run.insert(message.first);
    results_->setScope(last_run, QString("Results of the last run (%1 rule%2) — click a rule of the tree for its own")
                                           .arg(pending_nodes_.size()).arg(pending_nodes_.size() == 1 ? "" : "s"));
    info_->showRow(nullptr);
    rule_panel_->tree()->setResultTexts(result_texts_);
    applyColours();
    publishIndex();
    offerResolvedTopics();
    const size_t count = results_->rows().size();
    QString summary = QString("%1 result(s)%2").arg(count).arg(cancelled ? " (stopped)" : "");
    // The elements the engine could not test, the folders left out, the
    // failed rules: above the results, not in a modal box (set above).
    if (!run_messages_.empty()) summary += QString(", %1 warning(s) in the Result panel").arg(run_messages_.size());
    s_.session.setStatusMessage("Clash", summary);
    // The Result panel shows them, even with no result.
    if (count > 0 || !run_messages_.empty()) emit showResultsRequested();
}

std::vector<std::uint32_t> ClashView::objectsOf(const clash::Elements& keys) const {
    std::vector<std::uint32_t> ids;
    for (const auto& key : keys) {
        if (key.file_index >= run_models_.size()) continue;
        const auto& elements = elements_.value(run_models_[key.file_index]);
        const auto found = elements.find(key.entity_id);
        if (found != elements.end() && found->object_id != 0) ids.push_back(found->object_id);
    }
    return ids;
}

ResultRow ClashView::rowOf(const clash::RuleOutcome& outcome, const clash::ClashResult& result, const QString& group) const {
    ResultRow row;
    row.node_id = outcome.node_id;
    row.rule_label = outcome.label;
    row.rule_type = outcome.rule_type;
    row.status = result.status;
    row.complex = result.complex;
    row.criticity = result.criticity;
    row.actor = result.actor;
    row.group = group;
    row.record = result.record;
    if (result.record) row.measure = measureText(*result.record);
    auto* loader = s_.session.loader();
    const auto describe = [&](const clash::ElementKey& key, QString& name, QString& guid, QString& cls, QString& model,
                              std::uint32_t& session_model, int& ifc_id) {
        if (key.file_index >= run_models_.size()) return;
        session_model = run_models_[key.file_index];
        ifc_id = key.entity_id;
        guid = QString::fromStdString(key.global_id);
        model = loader ? loader->displayName(session_model) : QString();
        const auto elements = elements_.value(session_model);
        const auto found = elements.find(key.entity_id);
        if (found != elements.end()) {
            name = found->name;
            cls = found->type;
        }
        if (name.isEmpty()) name = QString("#%1 %2").arg(key.entity_id).arg(guid);
    };
    if (result.complex) {
        row.sources = elementsOf(result.sources);
        row.targets = elementsOf(result.targets);
    } else {
        row.sources.push_back(elementOf(result.source));
        if (result.target) row.targets.push_back(elementOf(*result.target));
    }
    row.key = resultKey(outcome.node_id, result.complex, group, row.sources, row.targets);
    if (result.complex) {
        row.source_objects = objectsOf(result.sources);
        row.target_objects = objectsOf(result.targets);
        row.source_name = QString("%1 — %2 source(s)").arg(group.isEmpty() ? QStringLiteral("Group") : group).arg(result.sources.size());
        row.target_name = QString("%1 target(s)").arg(result.targets.size());
        if (!result.sources.empty()) {
            QString n, g, c;
            describe(result.sources.front(), n, g, c, row.source_model_name, row.source_model, row.source_ifc_id);
        }
        row.measure = result.status ? "passes" : "fails";
        return row;
    }
    describe(result.source, row.source_name, row.source_guid, row.source_class, row.source_model_name, row.source_model,
             row.source_ifc_id);
    row.source_objects = objectsOf({result.source});
    if (result.target) {
        describe(*result.target, row.target_name, row.target_guid, row.target_class, row.target_model_name, row.target_model,
                 row.target_ifc_id);
        row.target_objects = objectsOf({*result.target});
    }
    return row;
}

ResultElement ClashView::elementOf(const clash::ElementKey& key) const {
    ResultElement out;
    out.global_id = QString::fromStdString(key.global_id);
    out.step_id = key.entity_id;
    if (key.file_index < run_models_.size()) out.model_id = s_.session.modelIdForSessionModelId(run_models_[key.file_index]);
    return out;
}

std::vector<ResultElement> ClashView::elementsOf(const clash::Elements& keys) const {
    std::vector<ResultElement> out;
    out.reserve(keys.size());
    for (const auto& key : keys) out.push_back(elementOf(key));
    return out;
}

// --- the federation ---------------------------------------------------------------------------

void ClashView::markProjectDirty() {
    if (auto* federation = s_.session.federation()) federation->markDirty();
}

void ClashView::saveToFederation() {
    Federation* federation = s_.session.federation();
    if (!federation) return;
    // A restore still waiting for its models: written back as it was read.
    if (pending_restore_) {
        federation->setExtension(store::kExtensionKey, store::toJson(*pending_restore_));
        return;
    }
    const bool has_rules = document_.isOpen() && !document_.file().contains.empty();
    if (!has_rules && results_->rows().empty() && saved_nodes_.empty()) {
        federation->setExtension(store::kExtensionKey, QJsonValue());
        return;
    }
    store::SavedClash clash;
    if (document_.isOpen()) {
        document_.refreshLinks();
        clash.rule_file = clash::json::toJson(document_.file());
        clash.rule_file_path = document_.path();
    }
    clash.runs = saved_runs_;
    for (const auto& [node, saved] : saved_nodes_) clash.nodes.push_back(saved);
    clash.results = results_->rows();
    clash.messages = results_->messages();
    if (bcf_ && !bcf_->filePath().isEmpty()) {
        const QString fed = federation->filePath();
        clash.bcf_file = fed.isEmpty() ? bcf_->filePath() : QFileInfo(fed).dir().relativeFilePath(bcf_->filePath());
    }
    federation->setExtension(store::kExtensionKey, store::toJson(clash));
}

void ClashView::clearClash() {
    pending_restore_.reset();
    clearOverlay();
    if (!results_->rows().empty()) clearColours();
    results_->setResults({});
    results_->clearMessages();
    info_->showRow(nullptr);
    result_texts_.clear();
    rule_panel_->tree()->setResultTexts({});
    rule_inputs_.clear();
    saved_nodes_.clear();
    saved_runs_.clear();
    publishIndex();
}

void ClashView::restoreFromFederation() {
    if (busy_) return;
    clearClash();
    Federation* federation = s_.session.federation();
    const QJsonValue section = federation ? federation->extension(store::kExtensionKey) : QJsonValue();
    if (!section.isObject()) return;
    QStringList warnings;
    store::SavedClash clash = store::fromJson(section.toObject(), &warnings);
    if (!clash.rule_file.isEmpty()) {
        try {
            document_.setFile(clash::json::fileFromJson(clash.rule_file), clash.rule_file_path);
        } catch (const std::exception& e) {
            warnings << QString("The rules saved in the project cannot be read: %1").arg(e.what());
        }
    }
    for (const QString& warning : warnings) clash.messages.emplace_back(QString(), warning);
    pending_restore_ = std::move(clash);
    tryRestore();
}

std::vector<std::uint32_t> ClashView::objectsOfElements(const std::vector<ResultElement>& elements) const {
    std::vector<std::uint32_t> ids;
    auto* registry = s_.session.elementRegistry();
    if (!registry) return ids;
    // One lookup table per model, built on demand.
    std::map<QString, std::pair<QHash<QString, std::uint32_t>, QHash<int, std::uint32_t>>> tables;
    for (const ResultElement& element : elements) {
        const std::uint32_t model = s_.session.sessionModelIdForModelId(element.model_id);
        if (model == 0) continue;
        auto found = tables.find(element.model_id);
        if (found == tables.end()) {
            std::pair<QHash<QString, std::uint32_t>, QHash<int, std::uint32_t>> table;
            for (const BasicElementInfo& info : registry->basicElementInfoForModel(model)) {
                if (info.object_id == 0) continue;
                if (!info.guid.isEmpty()) table.first.insert(info.guid, info.object_id);
                table.second.insert(info.ifc_id, info.object_id);
            }
            found = tables.emplace(element.model_id, std::move(table)).first;
        }
        std::uint32_t id = element.global_id.isEmpty() ? 0 : found->second.first.value(element.global_id, 0);
        if (id == 0) id = found->second.second.value(element.step_id, 0);
        if (id != 0) ids.push_back(id);
    }
    return ids;
}

int ClashView::resolveRows(std::vector<ResultRow>& rows) const {
    int unresolved = 0;
    for (ResultRow& row : rows) {
        row.source_objects = objectsOfElements(row.sources);
        row.target_objects = objectsOfElements(row.targets);
        row.source_model = row.sources.empty() ? 0 : s_.session.sessionModelIdForModelId(row.sources.front().model_id);
        row.target_model = row.targets.empty() ? 0 : s_.session.sessionModelIdForModelId(row.targets.front().model_id);
        if (row.source_objects.size() < row.sources.size() || row.target_objects.size() < row.targets.size()) ++unresolved;
    }
    return unresolved;
}

void ClashView::resolveShownRows() {
    if (results_->rows().empty()) return;
    // Every row through one pass: the lookup tables are built once per model.
    std::vector<ResultRow> rows = results_->rows();
    resolveRows(rows);
    size_t i = 0;
    results_->updateRows([&rows, &i](ResultRow& row) {
        row.source_objects = std::move(rows[i].source_objects);
        row.target_objects = std::move(rows[i].target_objects);
        row.source_model = rows[i].source_model;
        row.target_model = rows[i].target_model;
        ++i;
    });
    for (auto& [node, inputs] : rule_inputs_) {
        const auto saved = saved_nodes_.find(node);
        if (saved == saved_nodes_.end()) continue;
        inputs.sources = objectsOfElements(saved->second.sources);
        inputs.targets = objectsOfElements(saved->second.targets);
        inputs.not_checked = objectsOfElements(saved->second.not_checked);
    }
    applyColours();
    publishIndex();
}

void ClashView::tryRestore() {
    if (!pending_restore_ || busy_) return;
    auto* loader = s_.session.loader();
    if (loader && loader->isLoading()) return; // once every model is loaded
    store::SavedClash clash = std::move(*pending_restore_);
    pending_restore_.reset();

    // The models of the runs: changed since the run, or not loaded.
    std::set<QString> reported;
    for (const store::SavedRun& run : clash.runs) {
        for (const store::SavedModel& model : run.models) {
            if (!reported.insert(model.model_id).second) continue;
            const std::uint32_t session_model = s_.session.sessionModelIdForModelId(model.model_id);
            if (session_model == 0) {
                clash.messages.emplace_back(QString(), QString("%1 is not loaded: its results are not shown in the 3D view.").arg(model.name));
                continue;
            }
            const QFileInfo info(loader ? loader->filePath(session_model) : model.path);
            if (info.exists() && (info.lastModified().toMSecsSinceEpoch() != model.mtime || info.size() != model.size)) {
                clash.messages.emplace_back(QString(), QString("%1 changed since the run of %2: its results may be out of date, "
                                                               "run the rules again.")
                                                           .arg(model.name, run.started));
            }
        }
    }
    const int unresolved = resolveRows(clash.results);
    if (unresolved > 0) {
        clash.messages.emplace_back(QString(), QString("%1 result(s) point at elements not found in the loaded models.").arg(unresolved));
    }

    std::set<QString> nodes;
    for (const store::SavedNode& node : clash.nodes) {
        nodes.insert(node.node_id);
        RuleInputs& inputs = rule_inputs_[node.node_id];
        inputs.label = node.label;
        inputs.type = node.type;
        inputs.sources = objectsOfElements(node.sources);
        inputs.targets = objectsOfElements(node.targets);
        inputs.not_checked = objectsOfElements(node.not_checked);
        if (!node.text.isEmpty()) result_texts_[node.node_id] = node.text;
        saved_nodes_[node.node_id] = node;
    }
    for (const ResultRow& row : clash.results) nodes.insert(row.node_id);
    saved_runs_ = clash.runs;
    results_->setResults({});
    results_->mergeResults(std::move(clash.results), nodes);
    results_->clearMessages();
    results_->mergeMessages(clash.messages, nodes);
    rule_panel_->tree()->setResultTexts(result_texts_);
    applyColours();
    publishIndex();
    if (!results_->rows().empty()) {
        s_.session.setStatusMessage("Clash", QString("%1 result(s) restored from the project").arg(results_->rows().size()));
    }
}

// --- BCF topics -------------------------------------------------------------------------------

void ClashView::setBcf(bcf::BcfView* view) {
    bcf_ = view;
    if (bcf_) connect(bcf_, &bcf::BcfView::topicsChanged, this, &ClashView::refreshTopics);
    refreshTopics();
}

void ClashView::refreshTopics() {
    QHash<QString, ResultPanel::TopicInfo> topics;
    QSet<QString> deleted;
    if (bcf_ && bcf_->file()) {
        for (const ports::bcf::Topic& t : bcf_->file()->topics) {
            const bool is_deleted = bcf_->isDeleted(t.guid);
            topics.insert(t.guid, {t.title, t.topic_status, is_deleted});
            if (is_deleted) deleted.insert(t.guid);
        }
    }
    // A topic deleted, then saved out of the file: its results are free.
    std::vector<int> released;
    for (int i = 0; i < static_cast<int>(results_->rows().size()); ++i) {
        const QString& guid = results_->rows()[static_cast<size_t>(i)].topic_guid;
        if (deleted_topics_.contains(guid) && !topics.contains(guid)) released.push_back(i);
    }
    deleted_topics_ = deleted;
    results_->setTopics(std::move(topics));
    if (!released.empty()) results_->setTopicOf(released, QString());
    offerResolvedTopics();
}

void ClashView::addToTopic(const std::vector<int>& indices, const QString& chosen) {
    if (!bcf_ || !bcf_->file()) {
        services::popup::information(host_, "Load a BCF file in the BCF panel first (or create a topic).");
        return;
    }
    QString guid = chosen;
    if (guid.isEmpty()) {
        QStringList titles;
        QStringList guids;
        for (const ports::bcf::Topic& t : bcf_->file()->topics) {
            if (bcf_->isDeleted(t.guid)) continue;
            titles << QString("%1  [%2]").arg(t.title, t.topic_status);
            guids << t.guid;
        }
        if (titles.isEmpty()) {
            services::popup::information(host_, "The BCF file has no topic.");
            return;
        }
        bool ok = false;
        const QString picked = QInputDialog::getItem(host_, "Add to Topic", "Topic", titles, 0, false, &ok);
        if (!ok) return;
        guid = guids.value(titles.indexOf(picked));
    }
    ports::bcf::Topic* target = bcf_->topic(guid);
    if (!target) return;
    std::vector<const ResultRow*> rows;
    std::vector<int> kept;
    int moved = 0;
    for (int index : indices) {
        if (index < 0 || index >= static_cast<int>(results_->rows().size())) continue;
        const ResultRow& row = results_->rows()[static_cast<size_t>(index)];
        if (row.topic_guid == guid) continue;
        if (results_->isLinked(row)) ++moved;
        rows.push_back(&row);
        kept.push_back(index);
    }
    if (rows.empty()) return;
    if (moved > 0 && !services::popup::confirmation(host_, QString("%1 of the selected results already belong to another "
                                                                    "topic. Move them to \"%2\"?").arg(moved).arg(target->title))) {
        return;
    }
    const QString author = bcf_->ensureAuthor();
    if (author.isEmpty()) return;
    target->description += QString("\n\nAdded on %1:\n%2")
                               .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm"), topic::description(rows));
    for (const QString& label : topic::labels(rows)) {
        if (!target->labels.contains(label)) target->labels << label;
    }
    target->modified_date = ports::bcf::now();
    target->modified_author = author;
    bcf_->topicChanged(guid);
    results_->setTopicOf(kept, guid);
    s_.session.setStatusMessage("Clash", QString("%1 result(s) added to the topic %2").arg(kept.size()).arg(target->title));
}

void ClashView::offerResolvedTopics() {
    if (resolved_topics_.isEmpty() || !bcf_ || !bcf_->file()) return;
    QStringList titles;
    std::vector<ports::bcf::Topic*> topics;
    for (const QString& guid : std::as_const(resolved_topics_)) {
        ports::bcf::Topic* t = bcf_->topic(guid);
        if (!t || bcf_->isDeleted(guid)) continue;
        // Still linked to a result (a later run found it again): not resolved.
        const bool used = std::any_of(results_->rows().begin(), results_->rows().end(),
                                      [&guid](const ResultRow& row) { return row.topic_guid == guid; });
        if (used || t->topic_status.compare("Closed", Qt::CaseInsensitive) == 0) continue;
        titles << "• " + t->title;
        topics.push_back(t);
    }
    // The topics found in the loaded file are offered once.
    for (auto it = resolved_topics_.begin(); it != resolved_topics_.end();) {
        it = bcf_->topic(*it) ? resolved_topics_.erase(it) : std::next(it);
    }
    if (topics.empty()) return;
    const auto answer = QMessageBox::question(
        host_, "Topics without result",
        QString("The last run found none of the results of these topics any more:\n\n%1\n\n"
                "Close them, with a comment \"Resolved\"?").arg(titles.join('\n')),
        QMessageBox::Yes | QMessageBox::No);
    if (answer != QMessageBox::Yes) return;
    const QString author = bcf_->ensureAuthor();
    if (author.isEmpty()) return;
    const QStringList statuses = bcf_->file()->extensions.topic_statuses;
    QString closed = QStringLiteral("Closed");
    for (const QString& status : statuses) {
        if (status.compare("Closed", Qt::CaseInsensitive) == 0) closed = status;
    }
    const QString now = ports::bcf::now();
    for (ports::bcf::Topic* t : topics) {
        t->topic_status = closed;
        t->modified_date = now;
        t->modified_author = author;
        t->comments.push_back({ports::bcf::newGuid(), now, author,
                               "Resolved: the clash rules no longer find the results of this topic.", QString(), QString(),
                               QString()});
        bcf_->topicChanged(t->guid);
    }
}

std::vector<std::pair<Eigen::Vector3d, Eigen::Vector3d>> ClashView::linesOf(const ResultRow& row) const {
    std::vector<std::pair<Eigen::Vector3d, Eigen::Vector3d>> lines;
    if (!row.record) return lines;
    Eigen::Matrix4d m = Eigen::Matrix4d::Identity();
    s_.viewport.modelToWorld(row.source_model, m);
    const auto render = [&m](const ifcclash::Point3& p) {
        return Eigen::Vector3d((m * Eigen::Vector4d(p[0], p[1], p[2], 1.0)).head<3>());
    };
    for (const auto& pair : row.record->points) lines.emplace_back(render(pair.source), render(pair.target));
    return lines;
}

void ClashView::createTopics(const std::vector<int>& indices, bool per_result) {
    if (busy_ || topic_job_) {
        s_.session.setStatusMessage("Clash", "Wait until the current work finishes");
        return;
    }
    if (!bcf_) {
        services::popup::information(host_, "The BCF panel is not available.");
        return;
    }
    std::vector<const ResultRow*> rows;
    for (int index : indices) {
        if (index >= 0 && index < static_cast<int>(results_->rows().size())) rows.push_back(&results_->rows()[static_cast<size_t>(index)]);
    }
    if (rows.empty()) return;
    // A result belongs to one topic at most: it moves to the new one.
    const int linked = static_cast<int>(std::count_if(rows.begin(), rows.end(), [this](const ResultRow* row) {
        return !row->topic_guid.isEmpty() && bcf_->topic(row->topic_guid) && !bcf_->isDeleted(row->topic_guid);
    }));
    if (linked > 0 && !services::popup::confirmation(host_, QString("%1 of the selected results already belong to a topic. "
                                                                     "Move them to the new topic(s)?").arg(linked))) {
        return;
    }
    const QString author = bcf_->ensureAuthor();
    if (author.isEmpty()) return;
    const ports::bcf::Extensions extensions = bcf_->file() ? bcf_->file()->extensions : ports::bcf::Extensions{};
    const QString name = !bcf_->filePath().isEmpty() ? QFileInfo(bcf_->filePath()).fileName()
                         : bcf_->file()            ? QStringLiteral("the BCF file (not saved yet)")
                                                   : QStringLiteral("a new BCF file");
    ClashTopicDialog dialog(rows, per_result, extensions, bcf::criticityPriorities(), !context_.empty(), name, host_);
    if (dialog.exec() != QDialog::Accepted) return;
    const ClashTopicOptions options = dialog.options();
    if (options.per_result && options.snapshot && rows.size() > 100 &&
        !services::popup::confirmation(host_, QString("%1 topics, each with its image: it takes a while. Continue?").arg(rows.size()))) {
        return;
    }

    auto job = std::make_unique<TopicJob>();
    job->options = options;
    job->start = bcf_->viewpoints().capture();
    std::vector<std::vector<const ResultRow*>> groups;
    if (options.per_result) {
        for (const ResultRow* row : rows) groups.push_back({row});
    } else {
        groups.push_back(rows);
    }
    std::vector<topic::ModelFile> models;
    for (std::uint32_t id : s_.session.sessionModelIds()) {
        models.push_back({s_.session.modelIdForSessionModelId(id), s_.session.loader() ? s_.session.loader()->filePath(id) : QString(), QString()});
    }
    const topic::PriorityTable priorities = bcf::criticityPriorities();
    const QString now = ports::bcf::now();
    const auto focusOf = [this, &options](const std::vector<const ResultRow*>& of) {
        services::BcfViewpoints::Focus focus;
        for (const ResultRow* row : of) {
            focus.sources.insert(focus.sources.end(), row->source_objects.begin(), row->source_objects.end());
            focus.targets.insert(focus.targets.end(), row->target_objects.begin(), row->target_objects.end());
            if (options.lines) {
                const auto lines = linesOf(*row);
                focus.lines.insert(focus.lines.end(), lines.begin(), lines.end());
            }
        }
        if (options.context) focus.context = context_;
        return focus;
    };
    for (const auto& group : groups) {
        ports::bcf::Topic t;
        t.guid = ports::bcf::newGuid();
        t.title = topic::title(options.title, group);
        t.topic_type = options.type;
        t.topic_status = options.status;
        t.priority = options.priority.isEmpty() ? topic::priority(group, priorities) : options.priority;
        if (options.per_result) {
            t.labels = topic::labels(group);
            for (const QString& label : options.labels) {
                if (!t.labels.contains(label)) t.labels << label;
            }
        } else {
            t.labels = options.labels.isEmpty() ? topic::labels(group) : options.labels;
        }
        t.assigned_to = options.assigned_to;
        t.stage = options.stage;
        if (!options.due_date.isEmpty()) t.due_date = options.due_date + "T00:00:00Z";
        t.creation_date = now;
        t.creation_author = author;
        t.description = options.description.isEmpty() ? topic::description(group) : options.description;
        t.header_files = topic::headerFiles(group, models);
        const size_t index = job->topics.size();
        // The main viewpoint: every result of the topic.
        ports::bcf::Viewpoint main;
        main.guid = ports::bcf::newGuid();
        main.index = 0;
        t.viewpoints.push_back(main);
        job->shots.push_back({index, main.guid, focusOf(group)});
        if (!options.comment.isEmpty()) {
            t.comments.push_back({ports::bcf::newGuid(), now, author, options.comment, main.guid, QString(), QString()});
        }
        // Several results: a comment per result, with its own viewpoint.
        if (group.size() > 1) {
            int number = 1;
            for (const ResultRow* row : group) {
                ports::bcf::Viewpoint own;
                own.guid = ports::bcf::newGuid();
                own.index = number;
                t.viewpoints.push_back(own);
                job->shots.push_back({index, own.guid, focusOf({row})});
                const QString text = QString("Result %1: %2 / %3%4")
                                         .arg(number)
                                         .arg(row->source_name, row->target_name.isEmpty() ? QStringLiteral("—") : row->target_name,
                                              row->measure.isEmpty() ? QString() : " — " + row->measure);
                t.comments.push_back({ports::bcf::newGuid(), now, author, text, own.guid, QString(), QString()});
                ++number;
            }
        }
        std::vector<QString> keys;
        for (const ResultRow* row : group) keys.push_back(row->key);
        job->keys.push_back(std::move(keys));
        job->topics.push_back(std::move(t));
    }
    auto* progress = new QProgressDialog("Taking the viewpoints of the topics…", "Cancel", 0,
                                         static_cast<int>(job->shots.size()), host_);
    progress->setWindowModality(Qt::WindowModal);
    progress->setMinimumDuration(500);
    progress->setAttribute(Qt::WA_DeleteOnClose);
    job->progress = progress;
    topic_job_ = std::move(job);
    nextTopicShot();
}

void ClashView::nextTopicShot() {
    if (!topic_job_) return;
    TopicJob& job = *topic_job_;
    if (job.progress && job.progress->wasCanceled()) {
        finishTopics(false);
        return;
    }
    if (job.next >= job.shots.size()) {
        finishTopics(true);
        return;
    }
    if (job.progress) job.progress->setValue(static_cast<int>(job.next));
    const TopicJob::Shot& shot = job.shots[job.next];
    ports::bcf::VisualizationInfo info = bcf_ ? bcf_->viewpoints().focus(shot.focus) : ports::bcf::VisualizationInfo{};
    ports::bcf::Viewpoint* viewpoint = nullptr;
    for (auto& entry : job.topics[shot.topic].viewpoints) {
        if (entry.guid == shot.viewpoint) viewpoint = &entry;
    }
    info.guid = shot.viewpoint;
    viewpoint->info = std::move(info);
    if (!job.options.snapshot || !bcf_) {
        ++job.next;
        QTimer::singleShot(0, this, &ClashView::nextTopicShot);
        return;
    }
    const size_t topic_index = shot.topic;
    const QString guid = shot.viewpoint;
    bcf_->viewpoints().captureSnapshot([this, topic_index, guid](const QByteArray& png) {
        if (!topic_job_) return;
        for (auto& entry : topic_job_->topics[topic_index].viewpoints) {
            if (entry.guid == guid) entry.snapshot = png;
        }
        ++topic_job_->next;
        nextTopicShot();
    });
}

void ClashView::finishTopics(bool create) {
    std::unique_ptr<TopicJob> job = std::move(topic_job_);
    if (!job) return;
    if (job->progress) job->progress->close();
    if (bcf_) {
        services::BcfViewpoints::Parts parts;
        parts.colours = false;
        parts.clipping = false;
        bcf_->viewpoints().apply(job->start, parts);
    }
    if (!create || !bcf_) {
        s_.session.setStatusMessage("Clash", "No topic created");
        return;
    }
    std::map<QString, QString> topic_of; // result key -> topic guid
    QString last;
    for (size_t i = 0; i < job->topics.size(); ++i) {
        last = bcf_->addTopic(std::move(job->topics[i]));
        for (const QString& key : job->keys[i]) topic_of[key] = last;
    }
    std::vector<int> linked;
    results_->updateRows([&topic_of](ResultRow& row) {
        const auto found = topic_of.find(row.key);
        if (found != topic_of.end()) row.topic_guid = found->second;
    });
    for (int i = 0; i < static_cast<int>(results_->rows().size()); ++i) {
        if (topic_of.count(results_->rows()[static_cast<size_t>(i)].key)) linked.push_back(i);
    }
    if (job->options.mark_valid) results_->setReviewOf(linked, ReviewStatus::Valid);
    markProjectDirty();
    s_.session.setStatusMessage("Clash", QString("%1 BCF topic(s) created for %2 result(s)").arg(job->topics.size()).arg(linked.size()));
    if (job->topics.size() == 1) bcf_->openTopic(last);
}

void ClashView::showSelection(const clash::Select& select, const QString& title) {
    if (select.empty()) {
        s_.session.setStatusMessage("Clash", title + ": the slot is empty");
        return;
    }
    run_messages_.clear();
    QString error;
    if (!prepareModels(&error)) {
        services::popup::information(host_, error);
        return;
    }
    document_.refreshLinks();
    const clash::Select copy = clash::json::selectFromJson(clash::json::toJson(select));
    startWork("Selection", [this, copy, title](ifcclash::Session& session, size_t files) {
        clash::Orchestrator orchestrator(session, files, {}, *cancel_);
        const clash::Elements keys = orchestrator.select(copy);
        QMetaObject::invokeMethod(this, [this, keys, title]() {
            const auto ids = objectsOf(keys);
            if (!ids.empty()) {
                s_.pipeline.run(ids, services::ActionPipeline::Action::Isolate);
                s_.selection.setSelection(ids, services::SelectionSource::Service, services::SelectionMode::Replace);
            }
            s_.session.setStatusMessage("Clash", QString("%1: %2 element(s), %3 shown").arg(title).arg(keys.size()).arg(ids.size()));
        }, Qt::QueuedConnection);
    });
    setProperty("work", "selection");
}

// --- display -------------------------------------------------------------------------------------

void ClashView::applyColours() {
    if (colour_red_) return; // "Color Red" holds the layer until it is turned off
    clearColours();
    std::vector<std::uint32_t> sources, targets, passing;
    // The results the panel shows (the last run, or the rule clicked in the tree).
    for (int index : results_->shownRows()) {
        const ResultRow& row = results_->rows()[static_cast<size_t>(index)];
        if (row.complex) {
            auto& set = row.status ? passing : sources;
            set.insert(set.end(), row.source_objects.begin(), row.source_objects.end());
            set.insert(set.end(), row.target_objects.begin(), row.target_objects.end());
            continue;
        }
        if (!row.status) continue; // excepted
        sources.insert(sources.end(), row.source_objects.begin(), row.source_objects.end());
        targets.insert(targets.end(), row.target_objects.begin(), row.target_objects.end());
    }
    if (sources.empty() && targets.empty() && passing.empty()) return;
    const bool was_suspended = s_.undo.isSuspended();
    s_.undo.record();
    s_.undo.setSuspended(true);
    s_.appearance.setLayerColour(passing, kPassColour, Layer::ClashRun);
    s_.appearance.setLayerColour(targets, kTargetColour, Layer::ClashRun);
    s_.appearance.setLayerColour(sources, kSourceColour, Layer::ClashRun); // a source wins over a target
    s_.undo.setSuspended(was_suspended);
}

void ClashView::clearColours() {
    s_.appearance.clearLayer(Layer::ClashRun);
}

void ClashView::clearOverlay() {
    // Only our own lines: the slot is shared with the measure tools.
    if (!overlay_drawn_) return;
    s_.viewport.setOverlayLines({});
    overlay_drawn_ = false;
}

void ClashView::drawOverlay(int index) {
    if (index < 0 || index >= static_cast<int>(results_->rows().size()) || !overlay_.any()) {
        clearOverlay();
        return;
    }
    const ResultRow& row = results_->rows()[static_cast<size_t>(index)];
    if (!row.record) {
        clearOverlay();
        return;
    }
    Transform t;
    Eigen::Matrix4d m;
    if (s_.viewport.modelToWorld(row.source_model, m)) t.m = m;
    const ifcclash::ResultRecord& r = *row.record;
    auto points = lineGroup(QColor("#00B4D8"), 2.5f);
    for (const auto& p : r.points) t.segment(points.world_xyz, p.source, p.target);
    auto zones = lineGroup(QColor("#CC33FF"), 2.0f, 12.0f);
    auto boxes = lineGroup(QColor("#FFD91A"), 1.5f);
    for (const auto* list : {&r.source_obbs, &r.target_obbs}) {
        for (const auto& obb : *list) obbEdges(t, obb, obb.label && *obb.label == "zone" ? zones.world_xyz : boxes.world_xyz);
    }
    auto axes = lineGroup(QColor("#3CB44B"), 2.0f);
    for (const auto& axis : r.axes) {
        ifcclash::Point3 a, b;
        for (size_t c = 0; c < 3; ++c) {
            a[c] = axis.center[c] - 0.5 * axis.length * axis.direction[c];
            b[c] = axis.center[c] + 0.5 * axis.length * axis.direction[c];
        }
        t.segment(axes.world_xyz, a, b);
    }
    auto faces = lineGroup(QColor("#F08A24"), 1.2f);
    for (const auto& pair : r.faces) {
        for (const ifcclash::FaceData* face : {&pair.source, &pair.target}) {
            for (const auto& tri : face->triangles) {
                t.segment(faces.world_xyz, tri[0], tri[1]);
                t.segment(faces.world_xyz, tri[1], tri[2]);
                t.segment(faces.world_xyz, tri[2], tri[0]);
            }
        }
    }
    auto hits = lineGroup(QColor("#3CB44B"), 1.0f);
    auto blocked = lineGroup(QColor("#E6194B"), 1.0f);
    if (r.raycast) {
        for (const auto& [a, b] : r.raycast->hit_segments) t.segment(hits.world_xyz, a, b);
        for (const auto& [a, b] : r.raycast->blocked_segments) t.segment(blocked.world_xyz, a, b);
    }
    std::vector<OverlayRenderer::LineGroup> groups;
    for (const auto& [group, on] : std::vector<std::pair<OverlayRenderer::LineGroup*, bool>>{
             {&zones, overlay_.zones}, {&boxes, overlay_.obbs}, {&faces, overlay_.faces}, {&axes, overlay_.axes},
             {&hits, overlay_.rays}, {&blocked, overlay_.rays}, {&points, overlay_.points}}) {
        if (on && !group->world_xyz.empty()) groups.push_back(std::move(*group));
    }
    if (groups.empty()) {
        clearOverlay();
        return;
    }
    s_.viewport.setOverlayLines(groups);
    overlay_drawn_ = true;
}

void ClashView::displayInput(std::shared_ptr<clash::RuleCheck> rule) {
    if (!rule) return;
    QString error;
    if (!prepareModels(&error)) {
        services::popup::information(host_, error);
        return;
    }
    document_.refreshLinks();
    auto copy = clash::json::cloneRule(*rule);
    copy->state = QStringLiteral("Final"); // its own results, even for a rule that feeds others
    const QString label = RuleDocument::labelOf(*rule);
    const bool started = startWork("Display input", [this, copy, label](ifcclash::Session& session, size_t files) {
        auto records = std::make_shared<std::vector<std::shared_ptr<const ifcclash::ResultRecord>>>();
        clash::Orchestrator::Callbacks callbacks;
        callbacks.finished = [records](const clash::RuleOutcome& outcome) {
            for (const auto& result : outcome.results) {
                if (result.record) records->push_back(result.record);
            }
        };
        clash::Orchestrator orchestrator(session, files, callbacks, *cancel_);
        orchestrator.run({copy});
        QMetaObject::invokeMethod(this, [this, records, label]() {
            drawDisplay(*records);
            s_.session.setStatusMessage("Clash", QString("Display input of %1: %2 result(s) drawn").arg(label).arg(records->size()));
        }, Qt::QueuedConnection);
    });
    if (started) setProperty("work", "display");
}

void ClashView::testRule(std::shared_ptr<clash::RuleCheck> rule, std::function<void(const QString&)> answer) {
    QString error;
    if (busy_) {
        answer("A run is in progress: try again when it ends.");
        return;
    }
    if (!prepareModels(&error)) {
        answer(error);
        return;
    }
    document_.refreshLinks();
    auto summary = std::make_shared<QString>();
    const bool started = startWork("Must test", [this, rule, answer, summary](ifcclash::Session& session, size_t files) {
        clash::Orchestrator::Callbacks callbacks;
        callbacks.finished = [summary](const clash::RuleOutcome& outcome) {
            if (!outcome.error.isEmpty()) {
                *summary = "Failed: " + outcome.error;
                return;
            }
            int pass = 0, fail = 0;
            for (const auto& result : outcome.results) (result.status ? pass : fail)++;
            *summary = QString("%1 group%2: <span style='color:#3CB44B'>%3 pass</span>, "
                               "<span style='color:#E6194B'>%4 fail</span>")
                           .arg(pass + fail).arg(pass + fail == 1 ? "" : "s").arg(pass).arg(fail);
        };
        try {
            clash::Orchestrator orchestrator(session, files, callbacks, *cancel_);
            orchestrator.run({rule});
        } catch (const std::exception& e) {
            *summary = "Failed: " + QString::fromUtf8(e.what()); // said here, not in a modal box
        }
        QMetaObject::invokeMethod(this, [answer, summary]() { answer(summary->isEmpty() ? QString("No group") : *summary); },
                                  Qt::QueuedConnection);
    });
    if (started) setProperty("work", "test");
    else answer("The engine is busy.");
}

void ClashView::drawDisplay(const std::vector<std::shared_ptr<const ifcclash::ResultRecord>>& records) {
    auto zones = lineGroup(QColor("#CC33FF"), 2.0f, 12.0f);
    auto boxes = lineGroup(QColor("#FFD91A"), 1.5f);
    // One box once, even when several results share it.
    std::set<std::tuple<long long, long long, long long, long long>> seen;
    for (const auto& record : records) {
        Transform t;
        Eigen::Matrix4d m;
        const std::uint32_t file = record->source.file_index;
        if (file < run_models_.size() && s_.viewport.modelToWorld(run_models_[file], m)) t.m = m;
        for (const auto* list : {&record->source_obbs, &record->target_obbs}) {
            for (const auto& obb : *list) {
                const auto key = std::make_tuple(std::llround(obb.center[0] * 1e4), std::llround(obb.center[1] * 1e4),
                                                 std::llround(obb.center[2] * 1e4),
                                                 std::llround((obb.half_sizes[0] + obb.half_sizes[1] + obb.half_sizes[2]) * 1e4));
                if (!seen.insert(key).second) continue;
                obbEdges(t, obb, obb.label && *obb.label == "zone" ? zones.world_xyz : boxes.world_xyz);
            }
        }
    }
    std::vector<OverlayRenderer::LineGroup> groups;
    for (auto* group : {&zones, &boxes}) {
        if (!group->world_xyz.empty()) groups.push_back(std::move(*group));
    }
    if (groups.empty()) {
        clearOverlay();
        s_.session.setStatusMessage("Clash", "Display input: the rule gives no zone or box");
        return;
    }
    s_.viewport.setOverlayLines(groups);
    overlay_drawn_ = true;
}

void ClashView::setColourRed(bool on) {
    colour_red_ = on;
    const bool was_suspended = s_.undo.isSuspended();
    s_.undo.record();
    s_.undo.setSuspended(true);
    if (on) {
        std::vector<std::uint32_t> all;
        if (auto* registry = s_.session.elementRegistry()) {
            for (std::uint32_t model : s_.session.sessionModelIds()) {
                for (const auto& info : registry->basicElementInfoForModel(model)) all.push_back(info.object_id);
            }
        }
        s_.appearance.clearLayer(Layer::ClashRun);
        s_.appearance.setLayerColour(all, kSourceColour, Layer::ClashRun);
    }
    s_.undo.setSuspended(was_suspended);
    if (!on) {
        // Back to the colours of the results (none without results).
        clearColours();
        if (!results_->rows().empty()) applyColours();
    }
}

void ClashView::isolateRows(const std::vector<int>& rows) {
    std::vector<std::uint32_t> ids;
    for (int row : rows) {
        if (row < 0 || row >= static_cast<int>(results_->rows().size())) continue;
        const auto objects = results_->rows()[static_cast<size_t>(row)].objects();
        ids.insert(ids.end(), objects.begin(), objects.end());
    }
    if (ids.empty()) {
        s_.session.setStatusMessage("Clash", "The result has no element with geometry in the viewer");
        return;
    }
    std::vector<std::uint32_t> shown = ids;
    if (always_context_) shown.insert(shown.end(), context_.begin(), context_.end());
    s_.pipeline.run(shown, services::ActionPipeline::Action::Isolate);
    s_.selection.setSelection(ids, services::SelectionSource::Table, services::SelectionMode::Replace);
    modules::viewport::commands::viewSelected(s_.viewport);
    if (rows.size() == 1) drawOverlay(rows.front());
}

// --- files -----------------------------------------------------------------------------------------

bool ClashView::confirmDiscard() {
    if (!document_.isOpen() || !document_.isModified() || document_.file().contains.empty()) return true;
    const auto answer = QMessageBox::question(host_, "Rule file", "The rule file is modified. Save it?",
                                              QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (answer == QMessageBox::Cancel) return false;
    if (answer == QMessageBox::Save) {
        saveRules();
        return !document_.isModified();
    }
    return true;
}

void ClashView::saveRules() {
    if (!document_.isOpen()) return;
    // Always a "Save As": the location is asked each time (the current file proposed).
    QSettings settings;
    const QString proposed = document_.path().isEmpty() ? settings.value(kLastDirectoryKey).toString() : document_.path();
    QString path = QFileDialog::getSaveFileName(host_, "Save the rule file", proposed,
                                                "Rule files (*.ifcrule.json *.json);;All files (*)");
    if (path.isEmpty()) return;
    if (QFileInfo(path).suffix().isEmpty()) path += ".ifcrule.json";
    settings.setValue(kLastDirectoryKey, QFileInfo(path).absolutePath());
    document_.refreshLinks();
    clash::RuleFile& file = document_.file();
    file.list_ifc_path.clear(); // not saved: the rules apply to any set of models
    file.path_to_save = path;
    QSaveFile out(path);
    if (!out.open(QIODevice::WriteOnly) || out.write(QJsonDocument(clash::json::toJson(file)).toJson(QJsonDocument::Indented)) < 0 ||
        !out.commit()) {
        services::popup::information(host_, QString("Cannot write %1.").arg(path));
        return;
    }
    document_.setPath(path);
    document_.markSaved();
    s_.session.setStatusMessage("Clash", "Rule file saved: " + QFileInfo(path).fileName());
}

void ClashView::loadRules() {
    if (!confirmDiscard()) return;
    QSettings settings;
    const QString path = QFileDialog::getOpenFileName(host_, "Open a rule file", settings.value(kLastDirectoryKey).toString(),
                                                      "Rule files (*.ifcrule.json *.json);;All files (*)");
    if (path.isEmpty()) return;
    settings.setValue(kLastDirectoryKey, QFileInfo(path).absolutePath());
    QFile in(path);
    if (!in.open(QIODevice::ReadOnly)) {
        services::popup::information(host_, QString("Cannot read %1.").arg(path));
        return;
    }
    try {
        document_.setFile(clash::json::fileFromJson(clash::json::parseDocument(in.readAll())), path);
    } catch (const std::exception& e) {
        services::popup::information(host_, QString("%1 is not a rule file: %2").arg(QFileInfo(path).fileName(), e.what()));
        return;
    }
    QStringList unknown;
    for (const auto& rule : document_.treeRules()) {
        if (!clash::RuleCatalogue::instance().find(rule->type)) unknown << rule->type;
    }
    unknown.removeDuplicates();
    if (!unknown.isEmpty()) {
        services::popup::information(host_, "These rules have no triangle version in the viewer and will fail: " + unknown.join(", "));
    }
    s_.session.setStatusMessage("Clash", "Rule file loaded: " + QFileInfo(path).fileName());
}

void ClashView::saveResults() {
    if (results_->rows().empty()) {
        services::popup::information(host_, "No result to save: run the rules first.");
        return;
    }
    QSettings settings;
    QString path = QFileDialog::getSaveFileName(host_, "Save the results", settings.value(kLastDirectoryKey).toString(),
                                                "JSON files (*.json)");
    if (path.isEmpty()) return;
    if (QFileInfo(path).suffix().isEmpty()) path += ".json";
    const auto review = [](ReviewStatus status) {
        return status == ReviewStatus::Valid ? "Valid" : status == ReviewStatus::Invalid ? "Invalid" : "Non Determined";
    };
    const auto element = [](const QString& name, const QString& guid, const QString& cls, const QString& model, int id) {
        return QJsonObject{{"name", name}, {"global_id", guid}, {"ifc_type", cls}, {"model", model}, {"step_id", id}};
    };
    QJsonArray results;
    for (const ResultRow& row : results_->rows()) {
        QJsonObject json{{"rule", row.rule_label}, {"rule_type", row.rule_type}, {"node_id", row.node_id},
                         {"status", row.status}, {"review", review(row.review)}, {"measure", row.measure},
                         {"criticity", QJsonArray::fromStringList(row.criticity)}, {"actor", QJsonArray::fromStringList(row.actor)},
                         {"source", element(row.source_name, row.source_guid, row.source_class, row.source_model_name, row.source_ifc_id)}};
        if (!row.target_guid.isEmpty()) {
            json["target"] = element(row.target_name, row.target_guid, row.target_class, row.target_model_name, row.target_ifc_id);
        }
        if (!row.group.isEmpty()) json["group"] = row.group;
        if (!row.manual_group.isEmpty()) json["manual_group"] = row.manual_group;
        if (row.record) json["info"] = QJsonDocument::fromJson(QByteArray::fromStdString(ifcclash::info_to_json(*row.record).dump())).object();
        results.append(json);
    }
    document_.refreshLinks();
    const QJsonObject json{{"rule_file", clash::json::toJson(document_.file())}, {"results", results}};
    QSaveFile out(path);
    if (!out.open(QIODevice::WriteOnly) || out.write(QJsonDocument(json).toJson(QJsonDocument::Indented)) < 0 || !out.commit()) {
        services::popup::information(host_, QString("Cannot write %1.").arg(path));
        return;
    }
    s_.session.setStatusMessage("Clash", QString("%1 result(s) saved").arg(results.size()));
}

void ClashView::editRule(std::shared_ptr<clash::RuleCheck> rule) {
    if (!rule) return;
    RuleEditDialog dialog(
        rule, &document_,
        RuleEditDialog::FacetContext{s_.ids, [this]() { return services::FilterRunner::activeIfcPath(s_.session); },
                                     [this](auto rule, auto answer) { testRule(rule, answer); }},
        host_);
    connect(&dialog, &RuleEditDialog::isolateRequested, this, &ClashView::isolateExpressIds);
    // A rule edited from a cascade is written at once (its own window's OK).
    connect(&dialog, &RuleEditDialog::ruleEdited, &document_, &RuleDocument::touch);
    if (dialog.exec() != QDialog::Accepted) return;
    dialog.apply();
    document_.touch();
}

void ClashView::isolateExpressIds(const std::vector<int>& express_ids) {
    std::uint32_t model = 0;
    services::FilterRunner::activeIfcPath(s_.session, &model);
    auto* registry = s_.session.elementRegistry();
    if (model == 0 || registry == nullptr) return;
    QHash<int, std::uint32_t> object_of;
    for (const auto& info : registry->basicElementInfoForModel(model)) object_of.insert(info.ifc_id, info.object_id);
    std::vector<std::uint32_t> ids;
    for (int id : express_ids) {
        const auto found = object_of.find(id);
        if (found != object_of.end()) ids.push_back(found.value());
    }
    s_.pipeline.run(ids, services::ActionPipeline::Action::Isolate);
}

void ClashView::editFolder(std::shared_ptr<clash::RuleFolder> folder) {
    if (!folder) return;
    FolderEditDialog dialog(folder, host_);
    if (dialog.exec() != QDialog::Accepted) return;
    dialog.apply();
    document_.touch();
}

// --- ribbons -----------------------------------------------------------------------------------------

QWidget* ClashView::buildGeometryRuleRibbon() {
    auto* page = new QFrame(host_);
    auto* row = new QHBoxLayout(page);
    row->setContentsMargins(2, 4, 2, 4);
    row->setSpacing(0);
    const auto describe = [](QToolButton* button, const QString& text) {
        button->setToolTip(QString("<b>%1</b><br>%2").arg(button->text(), text));
    };
    run_button_ = components::buttons::makeButton("Run Rule", ":/icons/intersect.svg", page);
    describe(run_button_, "Run the selected rules and folders of the Rule tree, or the whole rule file.");
    connect(run_button_, &QToolButton::clicked, this, [this]() {
        auto nodes = rule_panel_->tree()->selectedNodes();
        if (nodes.empty()) nodes = document_.file().contains;
        run(nodes);
    });
    stop_button_ = components::buttons::makeButton("Stop", ":/icons/xmark-circle.svg", page);
    stop_button_->setEnabled(false);
    describe(stop_button_, "Stop the run after the current rule.");
    connect(stop_button_, &QToolButton::clicked, this, &ClashView::stop);
    auto* show = components::buttons::makeButton("Show Results", ":/icons/check.svg", page);
    describe(show, "Show the Manage Rule Result workspace with the results of the last run.");
    connect(show, &QToolButton::clicked, this, &ClashView::showResultsRequested);
    auto* save = components::buttons::makeButton("Save Rules", ":/icons/floppy-disk.svg", page);
    describe(save, "Save the rule file (JSON, the format of the IfcClash_Plus command line); the location is "
                   "asked each time.");
    connect(save, &QToolButton::clicked, this, [this]() { saveRules(); });
    auto* load = components::buttons::makeButton("Load Rules", ":/icons/folder.svg", page);
    describe(load, "Open a rule file (JSON).");
    connect(load, &QToolButton::clicked, this, &ClashView::loadRules);
    auto* colour = components::buttons::makeButton("Color Results", ":/icons/paint-bucket.svg", page);
    colour->setCheckable(true);
    colour->setChecked(true);
    describe(colour, "Colour the elements of the results: sources red, targets orange, Must groups green or red.");
    connect(colour, &QToolButton::toggled, this, [this](bool on) {
        if (on) applyColours();
        else clearColours();
    });
    auto* red = components::buttons::makeButton("Color Red", ":/icons/paint-bucket.svg", page);
    red->setCheckable(true);
    describe(red, "Colour every object of the loaded models red; off: back to the colours of the results "
                  "(the other colours were never changed).");
    connect(red, &QToolButton::toggled, this, &ClashView::setColourRed);
    components::buttons::addButtonGroups(row, {
        components::buttons::makeButtonGroup("RUN", {run_button_, stop_button_, show}, page),
        components::buttons::makeButtonGroup("RULE FILE", {save, load}, page),
        components::buttons::makeButtonGroup("COLOUR", {colour, red}, page),
    });
    row->addStretch(1);
    return page;
}

QWidget* ClashView::buildClashFilterRibbon() {
    // A dock panel (Rule Result Tools): on the top, one row; on a side or
    // floating, the groups — and the buttons of a group too wide — wrap.
    auto* page = new components::FlowFrame(host_);
    auto* row = new components::FlowLayout(page);
    row->setContentsMargins(2, 4, 2, 4);
    // The group of the ribbon (buttons::makeButtonGroup), its buttons in a flow.
    const auto flowGroup = [page](const QString& title, const QList<QToolButton*>& buttons) {
        auto* group = new QFrame(page);
        group->setObjectName("ribbonGroup");
        auto* group_layout = new QVBoxLayout(group);
        group_layout->setContentsMargins(8, 6, 8, 4);
        group_layout->setSpacing(2);
        auto* button_row = new components::FlowLayout(nullptr, 4);
        for (auto* button : buttons) button_row->addWidget(button);
        auto* label = new QLabel(title, group);
        label->setObjectName("ribbonGroupLabel");
        label->setProperty("textRole", "secondary");
        label->setAlignment(Qt::AlignCenter);
        group_layout->addLayout(button_row);
        group_layout->addWidget(label);
        return static_cast<QWidget*>(group);
    };
    const auto describe = [](QToolButton* button, const QString& text) {
        button->setToolTip(QString("<b>%1</b><br>%2").arg(button->text(), text));
    };
    auto* save_context = components::buttons::makeButton("Save Context", ":/icons/floppy-disk.svg", page);
    describe(save_context, "Keep the elements shown now as the context of the results.");
    connect(save_context, &QToolButton::clicked, this, [this]() {
        context_ = s_.view.visibleIds();
        s_.session.setStatusMessage("Clash", QString("Context saved: %1 object(s)").arg(context_.size()));
    });
    auto* show_context = components::buttons::makeButton("Show Context", ":/icons/eye.svg", page);
    describe(show_context, "Show the saved context again, beside what is shown.");
    connect(show_context, &QToolButton::clicked, this, [this]() {
        if (context_.empty()) {
            s_.session.setStatusMessage("Clash", "No saved context");
            return;
        }
        s_.view.add(context_);
    });
    auto* always = components::buttons::makeButton("Always Display Context", ":/icons/cube-dots.svg", page);
    always->setCheckable(true);
    describe(always, "When a result is isolated, the saved context stays shown around it.");
    connect(always, &QToolButton::toggled, this, [this](bool on) { always_context_ = on; });
    auto* colour = components::buttons::makeButton("Color Result", ":/icons/paint-bucket.svg", page);
    describe(colour, "Colour the elements of the results again (sources red, targets orange).");
    connect(colour, &QToolButton::clicked, this, &ClashView::applyColours);

    auto* everything = components::buttons::makeButton("Display Everything", ":/icons/eye-solid.svg", page);
    describe(everything, "Show every object of the models again; the colours do not change.");
    connect(everything, &QToolButton::clicked, this, [this]() { s_.view.showAll(); });

    auto* reset = components::buttons::makeButton("Show All Results", ":/icons/refresh-double.svg", page);
    describe(reset, "Clear the filters (the hidden categories too) and the grouping of the Result panel.");
    connect(reset, &QToolButton::clicked, results_, &ResultPanel::clearFilters);
    auto* undetermined = components::buttons::makeButton("Only Non Determined", ":/icons/filter.svg", page);
    undetermined->setCheckable(true);
    describe(undetermined, "Show only the results not reviewed yet.");
    connect(undetermined, &QToolButton::toggled, results_, &ResultPanel::setFilterOnlyNonDetermined);
    auto* excepted = components::buttons::makeButton("Hide Excepted", ":/icons/eye-closed.svg", page);
    excepted->setCheckable(true);
    describe(excepted, "Hide the results an exception invalidated.");
    connect(excepted, &QToolButton::toggled, results_, &ResultPanel::setHideExcepted);
    auto* with_topic = components::buttons::makeButton("Hide With Topic", ":/icons/eye-closed.svg", page);
    with_topic->setCheckable(true);
    describe(with_topic, "Hide the results already in a BCF topic (a closed topic counts, a deleted one does not).");
    connect(with_topic, &QToolButton::toggled, results_, &ResultPanel::setHideLinked);
    auto* categories = components::buttons::makeButton("Categories", ":/icons/filter.svg", page);
    describe(categories, "Hide the results by category (rule, IFC classes, models, criticity, actor, review, manual "
                         "group): uncheck the values to hide, to work on a small set.");
    connect(categories, &QToolButton::clicked, this, [this]() {
        if (results_->rows().empty()) {
            s_.session.setStatusMessage("Clash", "No result: run the rules first");
            return;
        }
        CategoryFilterDialog dialog(*results_, host_);
        if (dialog.exec() != QDialog::Accepted) return;
        results_->setHiddenValues(dialog.hidden());
        s_.session.setStatusMessage("Clash", QString("%1 of %2 result(s) shown")
                                                 .arg(results_->shownRows().size())
                                                 .arg(results_->rows().size()));
    });
    auto* group = components::buttons::makeButton("Group by", ":/icons/cube.svg", page);
    group->setPopupMode(QToolButton::InstantPopup);
    describe(group, "Group the results by source, target, source model, rule, criticity, the rule's own grouping, "
                    "or the groups you made (right click on results: Add to group).");
    auto* menu = new QMenu(group);
    auto* modes = new QActionGroup(menu);
    for (const auto& [label, mode] : std::vector<std::pair<QString, ResultPanel::GroupMode>>{
             {"No grouping", ResultPanel::GroupMode::Flat},
             {"Source", ResultPanel::GroupMode::BySource},
             {"Target", ResultPanel::GroupMode::ByTarget},
             {"Source model", ResultPanel::GroupMode::BySourceModel},
             {"Rule", ResultPanel::GroupMode::ByRule},
             {"Criticity", ResultPanel::GroupMode::ByCriticity},
             {"Rule grouping", ResultPanel::GroupMode::ByRuleGroup},
             {"Manual group", ResultPanel::GroupMode::ByManualGroup},
             {"BCF topic", ResultPanel::GroupMode::ByTopic}}) {
        QAction* action = menu->addAction(label, this, [this, mode = mode]() { results_->setGroupMode(mode); });
        action->setCheckable(true);
        action->setChecked(mode == ResultPanel::GroupMode::Flat);
        modes->addAction(action);
    }
    group->setMenu(menu);
    // The geometry of the current result (one row selected), each kind on / off.
    QList<QToolButton*> geometry;
    for (const auto& [text, icon, flag, tip] : std::vector<std::tuple<QString, QString, bool OverlayKinds::*, QString>>{
             {"Points", ":/icons/select-point3d.svg", &OverlayKinds::points,
              "The closest points (or the contact points) between the two elements, in cyan."},
             {"Zones", ":/icons/cube-scan.svg", &OverlayKinds::zones,
              "The zones the rule checked (above, below, around the element…), dashed magenta boxes."},
             {"OBB", ":/icons/square3d-from-center.svg", &OverlayKinds::obbs,
              "The oriented bounding boxes of the elements the rule used, in yellow."},
             {"Axes", ":/icons/select-edge3d.svg", &OverlayKinds::axes,
              "The axes of the elements (main direction, length), in green."},
             {"Faces", ":/icons/select-face3d.svg", &OverlayKinds::faces,
              "The faces in contact or facing each other, in orange."},
             {"Rays", ":/icons/perspective-view.svg", &OverlayKinds::rays,
              "The rays of a visibility / free space rule: free in green, blocked in red."}}) {
        auto* button = components::buttons::makeButton(text, icon, page);
        button->setCheckable(true);
        describe(button, tip + "<br><i>Drawn for the selected result, over the 3D view; the colours and the "
                               "visibility of the results do not change.</i>");
        connect(button, &QToolButton::toggled, this, [this, flag = flag](bool on) {
            overlay_.*flag = on;
            const auto rows = results_->selectedRows();
            drawOverlay(rows.size() == 1 ? rows.front() : -1);
        });
        geometry.push_back(button);
    }
    auto* save = components::buttons::makeButton("Save Results", ":/icons/download-square.svg", page);
    describe(save, "Save the results of the last run (and the rules) to a JSON file, with the reviews.");
    connect(save, &QToolButton::clicked, this, &ClashView::saveResults);
    const QList<QWidget*> groups{
        flowGroup("CONTEXT VIEW", {save_context, show_context, always, colour, everything}),
        flowGroup("REGROUP RESULT", {reset, undetermined, excepted, with_topic, categories, group}),
        flowGroup("RESULT GEOMETRY", geometry),
        flowGroup("EXPORT", {save}),
    };
    for (int i = 0; i < groups.size(); ++i) {
        groups[i]->setProperty("separator", i + 1 < groups.size()); // the divider between groups (stylesheet)
        row->addWidget(groups[i]);
    }
    return page;
}

} // namespace bonsaiviewer::modules::rule
