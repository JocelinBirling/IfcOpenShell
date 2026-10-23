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
 * Bonsai is distributed in the hope that it will be useful,                    *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of               *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                 *
 * GNU General Public License for more details.                                 *
 *                                                                              *
 * You should have received a copy of the GNU General Public License            *
 * along with this program. If not, see <http://www.gnu.org/licenses/>.         *
 *                                                                              *
 ********************************************************************************/

#ifndef IFCINTERFACE_SERVICES_ACTIONPIPELINE_H
#define IFCINTERFACE_SERVICES_ACTIONPIPELINE_H

#include <QColor>

#include <cstdint>
#include <map>
#include <vector>

namespace bonsaiviewer::services {
class ActiveView;
class AppearanceService;
class IfcDataAccess;
}

// B08 — ActionPipeline (see 00_Technical_Implementation/
// BonsaiViewerTI00). The shared executor of display actions applied to
// an element set: isolate / add to view / remove from view (through B02),
// colour / transparency (through B04), auto-colour by property (B05 read
// path + B04 apply). One implementation used by the search bar (TI03),
// filter rules (TI06), select by property (TI14) and the display
// functions (TI02).

namespace bonsaiviewer::services {

class ActionPipeline {
public:
    enum class Action {
        Isolate,
        AddToView,
        RemoveFromView,
    };

    ActionPipeline(ActiveView& active_view,
                   AppearanceService& appearance,
                   IfcDataAccess& data_access);

    // View mutations go through B02, which re-applies the category
    // filter (B03) after every mutation.
    void run(const std::vector<std::uint32_t>& ids, Action action);

    // Colour / transparency in the user paint layer (B04); filters that
    // need the FilterAction layer call AppearanceService directly.
    void colour(const std::vector<std::uint32_t>& ids, const QColor& colour);
    void transparency(const std::vector<std::uint32_t>& ids, int percent);

    // One generated colour per distinct value of pset.property; groups
    // without the property share a neutral grey.
    void autoColourByProperty(const std::vector<std::uint32_t>& ids,
                              const std::string& pset,
                              const std::string& property);

private:
    static QColor generatedColour(int index);

    ActiveView& active_view_;
    AppearanceService& appearance_;
    IfcDataAccess& data_access_;
};

} // namespace bonsaiviewer::services

#endif
