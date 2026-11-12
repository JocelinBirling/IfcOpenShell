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


#include "SlotClipboard.h"
#include "RuleDocument.h"

#include "../../ports/ifctester/Ids.h"
#include "../../services/clash/RuleJson.h"

#include <QApplication>
#include <QClipboard>
#include <QJsonDocument>
#include <QMimeData>

namespace bonsaiviewer::modules::rule::slot_clipboard {

namespace {
const QString kMime = QStringLiteral("application/x-bonsai-clash-slot");
}

void copy(const clash::Select& select) {
    auto* data = new QMimeData;
    const QJsonValue json = clash::json::toJson(select);
    data->setData(kMime, QJsonDocument(json.toObject()).toJson(QJsonDocument::Compact));
    if (select.kind == clash::Select::Kind::Facet && !select.applicability.empty()) {
        data->setText(QString::fromUtf8(bonsaiviewer::ports::ifctester::facetsToXml(
            select.applicability)));
    } else {
        data->setText(QString::fromUtf8(QJsonDocument(json.toObject()).toJson(QJsonDocument::Indented)));
    }
    QApplication::clipboard()->setMimeData(data);
}

std::optional<clash::Select> paste() {
    const QMimeData* data = QApplication::clipboard()->mimeData();
    if (!data) return std::nullopt;
    try {
        if (data->hasFormat(kMime)) {
            const QJsonDocument document = QJsonDocument::fromJson(data->data(kMime));
            if (document.isObject()) return clash::json::selectFromJson(document.object());
        }
        const QString text = data->text().trimmed();
        if (text.startsWith('<')) {
            auto facets = bonsaiviewer::ports::ifctester::facetsFromXml(text.toUtf8());
            if (facets && !facets->empty()) {
                clash::Select select;
                select.kind = clash::Select::Kind::Facet;
                select.applicability = std::move(*facets);
                return select;
            }
        }
    } catch (const std::exception&) {
    }
    return std::nullopt;
}

bool canPaste() {
    const QMimeData* data = QApplication::clipboard()->mimeData();
    if (!data) return false;
    if (data->hasFormat(kMime)) return true;
    const QString text = data->text().trimmed();
    return text.startsWith('<') && (text.contains(QLatin1String("<facets")) || text.contains(QLatin1String("<ids")) ||
                                    text.contains(QLatin1String("<entity")) || text.contains(QLatin1String("<property")));
}

QString pasteError(const clash::Select& select, const clash::RuleCheck& into, const RuleDocument& document) {
    if (select.kind != clash::Select::Kind::Rule || !select.rule) return {};
    const QString id = select.rule->id;
    if (!document.treeRule(id)) return {}; // a nested rule: a copy of its own
    if (id == into.id) return QString("A rule cannot take its elements from itself (%1).").arg(RuleDocument::labelOf(into));
    if (document.wouldLoop(id, into.id)) {
        return QString("%1 already takes its elements from %2: the cascade would loop.")
            .arg(RuleDocument::labelOf(*select.rule), RuleDocument::labelOf(into));
    }
    return {};
}

} // namespace bonsaiviewer::modules::rule::slot_clipboard
