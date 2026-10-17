// This file was generated with the assistance of an AI coding tool.
/********************************************************************************
 *                                                                              *
 * This file is part of Bonsai.                                                 *
 *                                                                              *
 * Bonsai is free software: you can redistribute it and/or modify               *
 * it under the terms of the GNU General Public License as published by         *
 * the Free Software Foundation, either version 3.0 of the License, or          *
 * (at your option) any later version.                                           *
 * Bonsai is distributed in the hope that it will be useful,                    *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the               *
 * GNU General Public License for more details.                                 *
 *                                                                              *
 * You should have received a copy of the GNU General Public License           *
 * along with this program. If not, see <http://www.gnu.org/licenses/>.         *
 ********************************************************************************/

#include "ResultInfoPanel.h"

#include <QFormLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace bonsaiviewer::modules::rule {

namespace {

QString statusText(ResultStatus status) {
    switch (status) {
    case ResultStatus::Valid: return "Valide";
    case ResultStatus::NonDetermined: return "Non Déterminé";
    case ResultStatus::Invalid: return "Invalide";
    }
    return {};
}

} // namespace

ResultInfoPanel::ResultInfoPanel(QWidget* parent) : QWidget(parent) {
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(4, 4, 4, 4);
    body_ = new QWidget(this);
    outer->addWidget(body_);
    outer->addStretch(1);
    clear();
}

void ResultInfoPanel::clear() {
    delete body_;
    body_ = new QWidget(this);
    layout()->addWidget(body_);
    auto* label = new QLabel("No result selected", body_);
    label->setEnabled(false);
}

void ResultInfoPanel::showResultByIndex(const std::vector<ResultRow>* rows, int index) {
    if (!rows || index < 0 || index >= static_cast<int>(rows->size())) {
        clear();
        return;
    }
    showResult(&(*rows)[index]);
}

void ResultInfoPanel::showResult(const ResultRow* row) {
    if (!row) {
        clear();
        return;
    }

    delete body_;
    body_ = new QWidget(this);
    layout()->addWidget(body_);

    auto* form = new QFormLayout(body_);
    form->setLabelAlignment(Qt::AlignRight);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    const auto add_row = [this, form](const QString& label, const QString& value) {
        auto* value_label = new QLabel(value, body_);
        value_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
        value_label->setWordWrap(true);
        form->addRow(label, value_label);
    };

    add_row("Rule", row->rule_name);
    add_row("Source", row->source_name);
    add_row("Target", row->target_name);
    if (row->source_model_name == row->target_model_name) {
        add_row("Model", row->source_model_name);
    } else {
        add_row("Source model", row->source_model_name);
        add_row("Target model", row->target_model_name);
    }
    if (!row->measure.isEmpty()) add_row("Measure", row->measure);
    if (!row->criticity.isEmpty()) add_row("Criticity", row->criticity);
    if (!row->actor.isEmpty()) add_row("Actor", row->actor);
    add_row("Status", statusText(row->status));
}

} // namespace bonsaiviewer::modules::rule
