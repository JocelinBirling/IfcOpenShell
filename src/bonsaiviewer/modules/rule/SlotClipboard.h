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


#ifndef IFCINTERFACE_MODULES_RULE_SLOTCLIPBOARD_H
#define IFCINTERFACE_MODULES_RULE_SLOTCLIPBOARD_H

#include "../../services/clash/RuleModel.h"

#include <optional>

namespace bonsaiviewer::modules::rule {
class RuleDocument;
}

namespace bonsaiviewer::modules::rule::slot_clipboard {

namespace clash = bonsaiviewer::services::clash;

// BonsaiViewer09 (TI09) — copy / paste of the elements of a slot (source,
// target, context) between rules, from the Rule tree and the Edit window.
// The clipboard holds the slot in the rule file's JSON form; a slot of IDS
// facets also as IDS XML, so its facets paste into the filters, the tables
// and the IDS panel, and their facets paste into a slot.
void copy(const clash::Select& select);
// The slot of the clipboard (a copied slot, or IDS facets); none otherwise.
std::optional<clash::Select> paste();
bool canPaste();
// Why `select` cannot go into a slot of `into` (a cascade from the rule
// itself, or a loop); empty when it can.
QString pasteError(const clash::Select& select, const clash::RuleCheck& into, const RuleDocument& document);

} // namespace bonsaiviewer::modules::rule::slot_clipboard

#endif // IFCINTERFACE_MODULES_RULE_SLOTCLIPBOARD_H
