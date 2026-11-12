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

#ifndef IFCINTERFACE_MODULES_RULE_EDITORS_H
#define IFCINTERFACE_MODULES_RULE_EDITORS_H

#include "../../services/clash/RuleCatalogue.h"
#include "../../services/clash/RuleModel.h"
#include "../../services/IdsBridge.h"

#include <QJsonObject>
#include <QWidget>

#include <vector>

class QFormLayout;
class QListWidget;
class QVBoxLayout;

namespace bonsaiviewer::modules::rule {

namespace clash = bonsaiviewer::services::clash;
namespace it = bonsaiviewer::ports::ifctester;

// BonsaiViewer09 (TI09) — the parameters of a rule, one editor per entry
// of the engine catalogue (float, int, bool, enum, string, string_list,
// direction, object). Edits go to the QJsonObject at once (the keys of the
// rule file).
class ParamForm : public QWidget {
    Q_OBJECT

public:
    explicit ParamForm(QWidget* parent = nullptr);
    // `params` must outlive the form (or until the next setRule).
    void setRule(const clash::RuleSpec* spec, QJsonObject* params);

signals:
    void edited();

private:
    QFormLayout* form_ = nullptr;
};

// One IDS facet. A parameter is written as plain text (a simple value),
// or "enum: a | b", "pattern: regex", "bounds: min .. max",
// "length: min .. max"; empty is no value.
class FacetEditor : public QWidget {
    Q_OBJECT

public:
    explicit FacetEditor(QWidget* parent = nullptr);
    void setFacet(it::Facet* facet); // null: empty editor

signals:
    void edited();

private:
    QFormLayout* form_ = nullptr;
};

// A list of facets (the applicability of a SelectFacet), with Add / Remove
// and the editor of the current one. Used by the slots, the criticity and
// actor sets, the grouping set and the folder activation.
class FacetListEditor : public QWidget {
    Q_OBJECT

public:
    explicit FacetListEditor(QWidget* parent = nullptr);
    void setFacets(std::vector<it::Facet>* facets);

signals:
    void edited();

private:
    void refreshList(int current);

    std::vector<it::Facet>* facets_ = nullptr;
    QListWidget* list_ = nullptr;
    FacetEditor* editor_ = nullptr;
};

// "Entity IFCWALL", "Property Pset_WallCommon.FireRating = REI60"...
QString facetSummary(const it::Facet& facet);
QString paramText(const it::Param& param);
it::Param paramFromText(const QString& text);
// The facet types of the IDS Facet section, by their IDS name.
const QStringList& facetTypeNames();

// The IDS selection editor (filters / IDS panel) <-> the engine's facets.
// A facet the editor cannot show is left out.
bonsaiviewer::services::IdsBridge::SelectionFields idsFields(const std::vector<it::Facet>& facets);
// Empty with `error` set when a facet cannot be built (an unknown class...).
// `file`: the active model, for the class names (null: the schemas).
std::vector<it::Facet> engineFacets(const bonsaiviewer::services::IdsBridge::SelectionFields& fields,
                                    ifcopenshell::file* file, QString* error);

} // namespace bonsaiviewer::modules::rule

#endif // IFCINTERFACE_MODULES_RULE_EDITORS_H
