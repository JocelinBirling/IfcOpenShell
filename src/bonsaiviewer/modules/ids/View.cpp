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

#include "View.h"

#include "Editor.h"

#include "../../ElementRegistry.h"
#include "../../SessionState.h"
#include "../../components/Panel.h"
#include "../../ports/ifcopenshell_util/Attr.h"
#include "../../ports/ifctester/Ids.h"
#include "../../services/ActionPipeline.h"
#include "../../services/AppearanceService.h"
#include "../../services/FilterRunner.h"
#include "../../services/IdsBridge.h"
#include "../../services/IdsPropertyChecks.h"
#include "../../../ifcviewer/SceneLoader.h"
#include "../../../ifcparse/file.h"
#include "../../../ifcparse/schema.h"

#include <QFileInfo>
#include <QHash>
#include <QTimer>

#include <map>

namespace bonsaiviewer::modules::ids {

namespace it = bonsaiviewer::ports::ifctester;
namespace util = bonsaiviewer::ports::ifcopenshell_util;

namespace {

QString elementLabel(const QString& model, const express::base& element) {
    const auto name = util::forwardString(element, "Name");
    QString label = QString("%1 — #%2 %3").arg(model).arg(element.id()).arg(QString::fromStdString(util::className(element)));
    if (name && !name->empty()) label += QString(" '%1'").arg(QString::fromStdString(*name));
    return label;
}

} // namespace

IdsPanelView::IdsPanelView(components::Panel* panel,
                           SessionState& session,
                           services::IdsBridge& bridge,
                           services::ActionPipeline& pipeline,
                           services::AppearanceService& appearance,
                           QObject* parent)
    : QObject(parent),
      session_(session),
      bridge_(bridge),
      pipeline_(pipeline),
      appearance_(appearance)
{
    editor_ = new IdsEditor(bridge_, [this]() { return services::FilterRunner::activeIfcPath(session_); }, panel);
    panel->addBodyWidget(editor_);
    connect(editor_, &IdsEditor::runRequested, this, [this]() {
        editor_->setRunning(true);
        QTimer::singleShot(0, this, &IdsPanelView::run); // let "Running…" show first
    });
    connect(editor_, &IdsEditor::isolateRequested, this, &IdsPanelView::isolate);
    connect(editor_, &IdsEditor::colourRequested, this, &IdsPanelView::colour);
    connect(editor_, &IdsEditor::colorPropertiesToggled, this, &IdsPanelView::publishPropertyChecks);
    connect(editor_, &IdsEditor::filterPropertiesToggled, this, &IdsPanelView::publishPropertyChecks);
    connect(editor_, &IdsEditor::clearColoursRequested, this, [this]() {
        appearance_.clearLayer(services::AppearanceService::Layer::IdsResult);
    });
}

void IdsPanelView::run() {
    const IdsDocument document = editor_->document();
    if (document.specifications.empty()) {
        editor_->setRunError("The IDS has no specification.");
        return;
    }
    it::Ids ids;
    try {
        ids = document.toIds();
    } catch (const std::exception& e) {
        editor_->setRunError(QString("The IDS cannot be run: %1.").arg(e.what()));
        return;
    }

    std::vector<SpecificationResult> results(ids.specifications.size());
    for (size_t s = 0; s < ids.specifications.size(); ++s) {
        const it::Specification& spec = ids.specifications[s];
        results[s].name = QString::fromStdString(spec.name);
        for (const it::Facet& requirement : spec.requirements) {
            RequirementResult row;
            row.text = QString::fromStdString(requirement.toString(it::Clause::Requirement, spec.prohibited(), &requirement));
            row.facet = requirement;
            results[s].requirements.push_back(row);
        }
    }

    auto* loader = session_.loader();
    int models = 0;
    for (uint32_t session_model_id : session_.sessionModelIds()) {
        if (loader == nullptr || session_model_id == 0) continue;
        const QString path = loader->filePath(session_model_id);
        auto* file = bridge_.file(path);
        if (file == nullptr) continue;
        ++models;
        const QString model = QFileInfo(path).fileName();
        it::Ids checked = ids;
        checked.validate(*file, true); // only the specifications of the model's IFC version
        for (size_t s = 0; s < checked.specifications.size(); ++s) {
            const it::Specification& spec = checked.specifications[s];
            SpecificationResult& result = results[s];
            if (!spec.is_ifc_version.value_or(false)) {
                ++result.models_other_schema;
                continue;
            }
            ++result.models_checked;
            for (const auto& element : spec.applicable_entities) {
                const ElementRef ref{session_model_id, static_cast<int>(element.id())};
                result.applicable.push_back(ref);
                if (spec.failed_entities.count(element)) result.failed.push_back(ref);
                else result.passed.push_back(ref);
            }
            for (size_t r = 0; r < spec.requirement_outcomes.size(); ++r) {
                const it::FacetOutcome& outcome = spec.requirement_outcomes[r];
                RequirementResult& row = result.requirements[r];
                row.passed += static_cast<int>(outcome.passed_entities.size());
                for (const it::FacetFailure& failure : outcome.failures) {
                    row.failures.push_back({{session_model_id, static_cast<int>(failure.element.id())},
                                            elementLabel(model, failure.element),
                                            QString::fromStdString(failure.reason)});
                }
            }
        }
    }
    if (models == 0) {
        editor_->setRunError("No model is loaded: open a model to run the IDS on.");
        return;
    }

    // The status on the whole federation (Specification.validate, summed).
    for (size_t s = 0; s < results.size(); ++s) {
        const it::Specification& spec = ids.specifications[s];
        SpecificationResult& result = results[s];
        for (RequirementResult& row : result.requirements) row.status = row.failures.empty();
        result.status = result.failed.empty();
        if (result.models_checked == 0) {
            result.status = false;
            result.note = "no loaded model of the IFC versions of the specification";
        } else if (spec.minOccurs != 0 && result.applicable.empty()) {
            result.status = false;
            result.note = "required, but no element is applicable";
            for (RequirementResult& row : result.requirements) row.status = false;
        } else if (spec.prohibited() && !result.applicable.empty()) {
            result.status = false;
            result.note = "prohibited, but elements are applicable";
        }
    }
    results_ = results;
    editor_->setResults(results);
    publishPropertyChecks();
}

void IdsPanelView::publishPropertyChecks() {
    if (property_checks_ == nullptr) return;
    if ((!editor_->colorProperties() && !editor_->filterProperties()) || results_.empty()) {
        property_checks_->clear();
        return;
    }
    services::IdsPropertyChecks::Checks checks;
    for (const SpecificationResult& result : results_) {
        for (const RequirementResult& requirement : result.requirements) {
            if (!requirement.facet) continue;
            const auto type = requirement.facet->type;
            if (type != it::FacetType::Property && type != it::FacetType::Attribute) continue;
            const QString text = result.name + ": " + requirement.text;
            std::map<std::pair<uint32_t, int>, QString> failed;
            for (const FailureRow& failure : requirement.failures) {
                failed[{failure.element.session_model_id, failure.element.ifc_id}] = failure.reason;
            }
            for (const ElementRef& element : result.applicable) {
                const std::pair<uint32_t, int> key{element.session_model_id, element.ifc_id};
                const auto failure = failed.find(key);
                services::IdsPropertyChecks::Check check;
                check.facet = *requirement.facet;
                check.passed = failure == failed.end();
                check.text = check.passed ? text : text + "\n" + failure->second;
                checks[key].push_back(std::move(check));
            }
        }
    }
    property_checks_->setChecks(std::move(checks), editor_->colorProperties(), editor_->filterProperties());
}

std::vector<uint32_t> IdsPanelView::objectIds(const std::vector<ElementRef>& elements) const {
    auto* registry = session_.elementRegistry();
    if (registry == nullptr) return {};
    std::map<uint32_t, QHash<int, uint32_t>> object_ids_by_model;
    std::vector<uint32_t> object_ids;
    for (const ElementRef& element : elements) {
        auto found_model = object_ids_by_model.find(element.session_model_id);
        if (found_model == object_ids_by_model.end()) {
            QHash<int, uint32_t> by_ifc_id;
            for (const auto& info : registry->basicElementInfoForModel(element.session_model_id)) {
                by_ifc_id.insert(info.ifc_id, info.object_id);
            }
            found_model = object_ids_by_model.emplace(element.session_model_id, std::move(by_ifc_id)).first;
        }
        const auto object = found_model->second.find(element.ifc_id);
        if (object != found_model->second.end()) object_ids.push_back(object.value());
    }
    return object_ids;
}

void IdsPanelView::isolate(const std::vector<ElementRef>& elements) {
    const std::vector<uint32_t> object_ids = objectIds(elements);
    pipeline_.run(object_ids, services::ActionPipeline::Action::Isolate);
    const int without_geometry = static_cast<int>(elements.size() - object_ids.size());
    session_.setStatusMessage("IDS", QString("%1 element(s) isolated%2")
                                         .arg(object_ids.size())
                                         .arg(without_geometry > 0
                                                  ? QString(" (%1 without geometry)").arg(without_geometry)
                                                  : QString()));
}

void IdsPanelView::colour(const std::vector<ElementRef>& passed, const std::vector<ElementRef>& failed) {
    using Layer = services::AppearanceService::Layer;
    appearance_.clearLayer(Layer::IdsResult);
    const std::vector<uint32_t> green = objectIds(passed);
    const std::vector<uint32_t> red = objectIds(failed);
    appearance_.setLayerColour(green, QColor(46, 160, 67), Layer::IdsResult);
    appearance_.setLayerColour(red, QColor(220, 50, 47), Layer::IdsResult); // after green: red wins
    session_.setStatusMessage("IDS", QString("%1 element(s) in green, %2 in red").arg(green.size()).arg(red.size()));
}

} // namespace bonsaiviewer::modules::ids
