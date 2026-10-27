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

#include "UnifiedTreeBuilder.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace bonsaiviewer::services::unified {

namespace {

// Margin of the box prefilters (the precise tests use kPlanTolerance).
constexpr float kOverlapEpsilon = 0.05f;

bool overlapsXY(const Box& a, const Box& b) {
    for (int axis = 0; axis < 2; ++axis) {
        if (a.mn[axis] > b.mx[axis] + kOverlapEpsilon || b.mn[axis] > a.mx[axis] + kOverlapEpsilon) return false;
    }
    return true;
}

// Plan tolerance of the room tests: 5 cm is enough (BonsaiViewer13).
constexpr float kPlanTolerance = 0.05f;

struct Point2 {
    float x, y;
};

float cross(Point2 o, Point2 a, Point2 b) {
    return (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x);
}

float squaredDistanceToSegment(Point2 p, Point2 a, Point2 b) {
    const float dx = b.x - a.x, dy = b.y - a.y;
    const float length2 = dx * dx + dy * dy;
    float t = length2 > 0 ? ((p.x - a.x) * dx + (p.y - a.y) * dy) / length2 : 0.0f;
    t = std::max(0.0f, std::min(1.0f, t));
    const float ex = a.x + t * dx - p.x, ey = a.y + t * dy - p.y;
    return ex * ex + ey * ey;
}

Point2 corner(const float* triangle, int index) {
    return {triangle[2 * index], triangle[2 * index + 1]};
}

// `p` inside the triangle, or within `tolerance` of its edges.
bool pointNearTriangle(Point2 p, const float* triangle, float tolerance) {
    const Point2 a = corner(triangle, 0), b = corner(triangle, 1), c = corner(triangle, 2);
    const float d1 = cross(a, b, p), d2 = cross(b, c, p), d3 = cross(c, a, p);
    const bool negative = d1 < 0 || d2 < 0 || d3 < 0;
    const bool positive = d1 > 0 || d2 > 0 || d3 > 0;
    if (!(negative && positive)) return true;
    const float t2 = tolerance * tolerance;
    return squaredDistanceToSegment(p, a, b) <= t2 || squaredDistanceToSegment(p, b, c) <= t2
        || squaredDistanceToSegment(p, c, a) <= t2;
}

bool pointNearFootprint(Point2 p, const std::vector<float>& footprint, float tolerance) {
    for (std::size_t t = 0; t + 5 < footprint.size(); t += 6) {
        if (pointNearTriangle(p, &footprint[t], tolerance)) return true;
    }
    return false;
}

bool segmentsCross(Point2 a, Point2 b, Point2 c, Point2 d) {
    const float d1 = cross(c, d, a), d2 = cross(c, d, b), d3 = cross(a, b, c), d4 = cross(a, b, d);
    return ((d1 > 0) != (d2 > 0)) && ((d3 > 0) != (d4 > 0));
}

// Two triangles overlap, or come within `tolerance` of each other.
bool trianglesNear(const float* t1, const float* t2, float tolerance) {
    for (int i = 0; i < 3; ++i) {
        if (pointNearTriangle(corner(t1, i), t2, tolerance)) return true;
        if (pointNearTriangle(corner(t2, i), t1, tolerance)) return true;
    }
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (segmentsCross(corner(t1, i), corner(t1, (i + 1) % 3), corner(t2, j), corner(t2, (j + 1) % 3))) {
                return true;
            }
        }
    }
    return false;
}

// Two footprints overlap or come within `tolerance` of each other.
bool footprintsNear(const std::vector<float>& a, const std::vector<float>& b, float tolerance) {
    for (std::size_t i = 0; i + 5 < a.size(); i += 6) {
        float ax0 = a[i], ax1 = a[i], ay0 = a[i + 1], ay1 = a[i + 1];
        for (int k = 1; k < 3; ++k) {
            ax0 = std::min(ax0, a[i + 2 * k]); ax1 = std::max(ax1, a[i + 2 * k]);
            ay0 = std::min(ay0, a[i + 2 * k + 1]); ay1 = std::max(ay1, a[i + 2 * k + 1]);
        }
        for (std::size_t j = 0; j + 5 < b.size(); j += 6) {
            const float bx0 = std::min({b[j], b[j + 2], b[j + 4]}), bx1 = std::max({b[j], b[j + 2], b[j + 4]});
            const float by0 = std::min({b[j + 1], b[j + 3], b[j + 5]}), by1 = std::max({b[j + 1], b[j + 3], b[j + 5]});
            if (bx0 > ax1 + tolerance || ax0 > bx1 + tolerance || by0 > ay1 + tolerance || ay0 > by1 + tolerance) continue;
            if (trianglesNear(&a[i], &b[j], tolerance)) return true;
        }
    }
    return false;
}

// Height shared by two boxes (negative when they are apart vertically).
float verticalOverlap(const Box& a, const Box& b) {
    return std::min(a.mx[2], b.mx[2]) - std::max(a.mn[2], b.mn[2]);
}

bool validBox(const Box& box) {
    for (int axis = 0; axis < 3; ++axis) {
        if (!std::isfinite(box.mn[axis]) || !std::isfinite(box.mx[axis]) || box.mn[axis] > box.mx[axis]) return false;
    }
    return true;
}

std::string metres(double value) {
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%.3f m", value);
    return buffer;
}

std::string quoted(const std::string& text) {
    return "'" + text + "'";
}

// Joins the distinct names, in first-seen order.
std::string joinNames(const std::vector<std::string>& names) {
    std::string joined;
    std::set<std::string> seen;
    for (const std::string& name : names) {
        if (!seen.insert(name).second) continue;
        if (!joined.empty()) joined += " / ";
        joined += name;
    }
    return joined;
}

// A unified level: a reference storey, or a storey that matched none.
struct Level {
    const StoreyIn* anchor = nullptr;
    Reason reason = Reason::None;  // LevelCreated, or None for a reference level
    std::vector<std::string> notes;  // one line per attached storey
    Reason first_attachment = Reason::None;
    int node = -1;
    int no_room_node = -1;
};

// Uniform grid over the rooms in plan, so each element only tests the rooms
// near it.
class RoomGrid {
public:
    RoomGrid(const std::vector<RoomIn>& rooms, const std::vector<int>& usable) {
        if (usable.empty()) return;
        // Cell size: the median room extent in plan, at least 1 m.
        std::vector<float> extents;
        for (int index : usable) {
            const Box& box = rooms[index].box;
            extents.push_back(std::max(box.mx[0] - box.mn[0], box.mx[1] - box.mn[1]));
        }
        std::nth_element(extents.begin(), extents.begin() + extents.size() / 2, extents.end());
        cell_ = std::max(1.0f, extents[extents.size() / 2]);
        for (int index : usable) {
            forCells(rooms[index].box, [&](long long key) { cells_[key].push_back(index); });
        }
    }

    // The rooms whose box may overlap `box` in plan, each once.
    std::vector<int> candidates(const Box& box) const {
        std::vector<int> found;
        if (cells_.empty()) return found;
        forCells(box, [&](long long key) {
            const auto it = cells_.find(key);
            if (it != cells_.end()) found.insert(found.end(), it->second.begin(), it->second.end());
        });
        std::sort(found.begin(), found.end());
        found.erase(std::unique(found.begin(), found.end()), found.end());
        return found;
    }

private:
    template <typename F>
    void forCells(const Box& box, F&& visit) const {
        const long long x0 = static_cast<long long>(std::floor((box.mn[0] - kOverlapEpsilon) / cell_));
        const long long x1 = static_cast<long long>(std::floor((box.mx[0] + kOverlapEpsilon) / cell_));
        const long long y0 = static_cast<long long>(std::floor((box.mn[1] - kOverlapEpsilon) / cell_));
        const long long y1 = static_cast<long long>(std::floor((box.mx[1] + kOverlapEpsilon) / cell_));
        // A very long element (a pipe run) still visits a bounded number of cells.
        if ((x1 - x0 + 1) * (y1 - y0 + 1) > 200000) {
            for (const auto& [key, rooms] : cells_) visit(key);
            return;
        }
        for (long long x = x0; x <= x1; ++x) {
            for (long long y = y0; y <= y1; ++y) visit((x << 32) ^ (y & 0xffffffffLL));
        }
    }

    float cell_ = 1.0f;
    std::unordered_map<long long, std::vector<int>> cells_;
};

class Builder {
public:
    explicit Builder(const Input& input) : in_(input), settings_(input.settings) {}

    Result run() {
        if (!matchLevels()) return std::move(result_);
        if (!checkRooms()) return std::move(result_);
        buildSkeleton();
        placeRooms();
        placeElements();
        finish();
        result_.ok = true;
        return std::move(result_);
    }

private:
    int addNode(Node node) {
        result_.nodes.push_back(std::move(node));
        return static_cast<int>(result_.nodes.size()) - 1;
    }

    void addChild(int parent, int child) {
        if (parent < 0) result_.roots.push_back(child);
        else result_.nodes[parent].children.push_back(child);
    }

    void report(const std::string& message) {
        result_.diagnostics.push_back(message);
    }

    static std::pair<std::uint32_t, int> key(std::uint32_t model, int ifc_id) {
        return {model, ifc_id};
    }

    // ---- Levels ------------------------------------------------------------

    // The nearest reference level to `elevation` among `candidates`; ties go
    // to the lower one.
    int nearestLevel(double elevation, const std::vector<int>& candidates) const {
        int best = -1;
        double best_distance = 0;
        for (int index : candidates) {
            const double level_elevation = *levels_[index].anchor->elevation;
            const double distance = std::fabs(level_elevation - elevation);
            if (best < 0 || distance < best_distance - 1e-9
                || (std::fabs(distance - best_distance) <= 1e-9
                    && level_elevation < *levels_[best].anchor->elevation)) {
                best = index;
                best_distance = distance;
            }
        }
        return best;
    }

    bool matchLevels() {
        std::map<std::uint32_t, std::vector<const StoreyIn*>> by_model;
        for (const StoreyIn& storey : in_.storeys) by_model[storey.model].push_back(&storey);
        const auto reference = by_model.find(settings_.levels_model);
        if (reference == by_model.end() || reference->second.empty()) {
            result_.error = "The levels reference model has no storey (IfcBuildingStorey): the levels cannot be computed.";
            return false;
        }

        // Reference levels.
        std::map<std::string, int> name_count;
        for (const StoreyIn* storey : reference->second) ++name_count[storey->name];
        std::vector<int> with_elevation;
        for (const StoreyIn* storey : reference->second) {
            Level level;
            level.anchor = storey;
            const int index = static_cast<int>(levels_.size());
            levels_.push_back(level);
            level_of_.emplace(key(storey->model, storey->ifc_id), index);
            if (name_count[storey->name] == 1) reference_by_name_.emplace(storey->name, index);
            if (storey->elevation) with_elevation.push_back(index);
            else report("Reference model " + storey->model_name + ": storey " + quoted(storey->name)
                        + " has no Elevation; it is never matched by elevation.");
        }
        for (const auto& [name, count] : name_count) {
            if (count > 1) report("Reference model " + reference->second.front()->model_name + ": "
                                  + std::to_string(count) + " storeys are named " + quoted(name)
                                  + "; this name is not matched automatically.");
        }
        double lowest = 0, highest = 0;
        for (std::size_t i = 0; i < with_elevation.size(); ++i) {
            const double elevation = *levels_[with_elevation[i]].anchor->elevation;
            lowest = i == 0 ? elevation : std::min(lowest, elevation);
            highest = i == 0 ? elevation : std::max(highest, elevation);
        }

        // Secondary storeys: matched to the reference only, never to one another.
        for (const auto& [model, storeys] : by_model) {
            if (model == settings_.levels_model) continue;
            std::map<std::string, int> own_count;
            for (const StoreyIn* storey : storeys) ++own_count[storey->name];
            for (const auto& [name, count] : own_count) {
                if (count > 1) report("Model " + storeys.front()->model_name + ": " + std::to_string(count)
                                      + " storeys are named " + quoted(name)
                                      + "; matched by elevation only.");
            }
            for (const StoreyIn* storey : storeys) {
                matchStorey(*storey, own_count[storey->name] == 1, with_elevation, lowest, highest);
            }
        }
        return true;
    }

    void attach(const StoreyIn& storey, int level, Reason reason, const std::string& how) {
        Level& target = levels_[level];
        if (target.first_attachment == Reason::None) target.first_attachment = reason;
        target.notes.push_back(quoted(storey.name) + " (" + storey.model_name + "): " + how);
        level_of_.emplace(key(storey.model, storey.ifc_id), level);
    }

    void matchStorey(const StoreyIn& storey, bool unique_name, const std::vector<int>& with_elevation,
                     double lowest, double highest) {
        const double tolerance = settings_.tolerance;
        // 1. Exact name.
        const auto by_name = unique_name ? reference_by_name_.find(storey.name) : reference_by_name_.end();
        if (by_name != reference_by_name_.end()) {
            const int level = by_name->second;
            const auto& reference_elevation = levels_[level].anchor->elevation;
            if (settings_.merge_same_name) {
                attach(storey, level, Reason::LevelByNameMerged, "same name, merged without elevation check");
                return;
            }
            if (storey.elevation && reference_elevation) {
                const double delta = std::fabs(*storey.elevation - *reference_elevation);
                if (delta <= tolerance) {
                    attach(storey, level, Reason::LevelByName, "same name, elevation difference " + metres(delta));
                    return;
                }
                report("Model " + storey.model_name + ": storey " + quoted(storey.name)
                       + " has the name of a reference level but its elevation differs by " + metres(delta)
                       + "; name match rejected.");
            } else {
                attach(storey, level, Reason::LevelByName, "same name, elevation not checked (missing)");
                report("Model " + storey.model_name + ": storey " + quoted(storey.name)
                       + " matched by name without elevation check (Elevation missing).");
                return;
            }
        }
        if (!storey.elevation) {
            report("Model " + storey.model_name + ": storey " + quoted(storey.name)
                   + " has no Elevation and no name match; it becomes a new level.");
            createLevel(storey, "no name match and no Elevation");
            return;
        }
        // 2. Elevation within the tolerance.
        std::vector<int> close;
        for (int index : with_elevation) {
            if (std::fabs(*levels_[index].anchor->elevation - *storey.elevation) <= tolerance) close.push_back(index);
        }
        if (!close.empty()) {
            const int level = nearestLevel(*storey.elevation, close);
            attach(storey, level, Reason::LevelByElevation,
                   "elevation difference " + metres(std::fabs(*levels_[level].anchor->elevation - *storey.elevation)));
            return;
        }
        // 3. Nearest level inside the reference range, else a new level.
        if (!with_elevation.empty() && *storey.elevation >= lowest && *storey.elevation <= highest) {
            const int level = nearestLevel(*storey.elevation, with_elevation);
            attach(storey, level, Reason::LevelNearest,
                   "nearest level, outside the tolerance (" +
                       metres(std::fabs(*levels_[level].anchor->elevation - *storey.elevation)) + ")");
            return;
        }
        createLevel(storey, "elevation " + metres(*storey.elevation) + " outside the reference levels");
    }

    void createLevel(const StoreyIn& storey, const std::string& why) {
        Level level;
        level.anchor = &storey;
        level.reason = Reason::LevelCreated;
        level.notes.push_back("New level from " + quoted(storey.name) + " (" + storey.model_name + "): " + why);
        const int index = static_cast<int>(levels_.size());
        levels_.push_back(level);
        level_of_.emplace(key(storey.model, storey.ifc_id), index);
    }

    bool checkRooms() {
        for (std::size_t i = 0; i < in_.rooms.size(); ++i) {
            if (in_.rooms[i].object_id != 0 && validBox(in_.rooms[i].box)) usable_rooms_.push_back(static_cast<int>(i));
        }
        if (usable_rooms_.empty()) {
            result_.error = "The rooms reference model has no usable room (IfcSpace with geometry): "
                            "the rooms cannot be computed.";
            return false;
        }
        return true;
    }

    // ---- Tree ----------------------------------------------------------------

    void buildSkeleton() {
        // Site and building nodes, keyed by name (merged when asked).
        std::vector<std::string> all_sites, all_buildings;
        for (const Level& level : levels_) {
            all_sites.push_back(siteName(*level.anchor));
            all_buildings.push_back(buildingName(*level.anchor));
        }
        const std::string merged_site = joinNames(all_sites);
        const std::string merged_building = joinNames(all_buildings);

        // Levels sorted by elevation (missing elevation last, then by name).
        std::vector<int> order(levels_.size());
        for (std::size_t i = 0; i < order.size(); ++i) order[i] = static_cast<int>(i);
        std::stable_sort(order.begin(), order.end(), [this](int a, int b) {
            const auto& ea = levels_[a].anchor->elevation;
            const auto& eb = levels_[b].anchor->elevation;
            if (ea && eb && *ea != *eb) return *ea < *eb;
            if (ea.has_value() != eb.has_value()) return ea.has_value();
            return levels_[a].anchor->name < levels_[b].anchor->name;
        });

        std::map<std::string, int> site_nodes;
        std::map<std::pair<std::string, std::string>, int> building_nodes;
        for (int index : order) {
            Level& level = levels_[index];
            const StoreyIn& anchor = *level.anchor;
            const std::string site = settings_.merge_sites ? merged_site : siteName(anchor);
            const std::string building = settings_.merge_buildings ? merged_building : buildingName(anchor);

            auto site_it = site_nodes.find(site);
            if (site_it == site_nodes.end()) {
                Node node;
                node.kind = Node::Kind::Site;
                node.label = site;
                if (!settings_.merge_sites && anchor.site_ifc != 0) {
                    node.model = anchor.model;
                    node.ifc_id = anchor.site_ifc;
                }
                const int id = addNode(node);
                addChild(-1, id);
                site_it = site_nodes.emplace(site, id).first;
            }
            auto building_it = building_nodes.find({site, building});
            if (building_it == building_nodes.end()) {
                Node node;
                node.kind = Node::Kind::Building;
                node.label = building;
                if (!settings_.merge_buildings && anchor.building_ifc != 0) {
                    node.model = anchor.model;
                    node.ifc_id = anchor.building_ifc;
                }
                const int id = addNode(node);
                addChild(site_it->second, id);
                building_it = building_nodes.emplace(std::make_pair(site, building), id).first;
            }

            Node node;
            node.kind = Node::Kind::Level;
            node.label = anchor.name;
            node.model = anchor.model;
            node.ifc_id = anchor.ifc_id;
            if (level.reason == Reason::LevelCreated) {
                node.reason = Reason::LevelCreated;
            } else if (level.first_attachment != Reason::None) {
                node.reason = level.first_attachment;
            }
            std::string text;
            for (const std::string& note : level.notes) text += (text.empty() ? "" : "\n") + note;
            node.reason_text = text;
            level.node = addNode(node);
            addChild(building_it->second, level.node);
        }
    }

    std::string siteName(const StoreyIn& storey) const {
        return storey.site_name.empty() ? std::string("(no site)") : storey.site_name;
    }

    std::string buildingName(const StoreyIn& storey) const {
        return storey.building_name.empty() ? std::string("(no building)") : storey.building_name;
    }

    int levelOfStorey(std::uint32_t model, int storey_ifc) const {
        if (storey_ifc == 0) return -1;
        const auto it = level_of_.find(key(model, storey_ifc));
        return it == level_of_.end() ? -1 : it->second;
    }

    int noLevelNode() {
        if (no_level_node_ < 0) {
            Node node;
            node.kind = Node::Kind::NoLevel;
            node.label = "(no level)";
            no_level_node_ = addNode(node);
            addChild(-1, no_level_node_);
        }
        return no_level_node_;
    }

    int noRoomNode(int level) {
        Level& target = levels_[level];
        if (target.no_room_node < 0) {
            Node node;
            node.kind = Node::Kind::NoRoom;
            node.label = "(no room)";
            target.no_room_node = addNode(node);
            addChild(target.node, target.no_room_node);
        }
        return target.no_room_node;
    }

    void placeRooms() {
        room_nodes_.assign(in_.rooms.size(), -1);
        room_levels_.assign(in_.rooms.size(), -1);
        for (std::size_t i = 0; i < in_.rooms.size(); ++i) {
            const RoomIn& room = in_.rooms[i];
            Node node;
            node.kind = Node::Kind::Room;
            node.label = room.name.empty() ? std::string("(unnamed room)") : room.name;
            node.model = settings_.rooms_model;
            node.ifc_id = room.ifc_id;
            node.object_id = room.object_id;
            const int id = addNode(node);
            room_nodes_[i] = id;
            room_ifc_ids_.insert(room.ifc_id);
            if (room.object_id != 0) room_object_ids_.insert(room.object_id);
            const int level = levelOfStorey(settings_.rooms_model, room.storey_ifc);
            room_levels_[i] = level;
            if (level >= 0) {
                addChild(levels_[level].node, id);
            } else {
                addChild(noLevelNode(), id);
                report("Room " + quoted(node.label) + " has no storey: listed under (no level).");
            }
            if (room.object_id == 0 || !validBox(room.box)) {
                report("Room " + quoted(node.label) + " has no geometry: no element can be attached to it.");
            }
        }
    }

    // The first level at or above `elevation` (no tolerance).
    int levelAbove(double elevation) const {
        int best = -1;
        for (std::size_t i = 0; i < levels_.size(); ++i) {
            const auto& level_elevation = levels_[i].anchor->elevation;
            if (!level_elevation || *level_elevation < elevation) continue;
            if (best < 0 || *level_elevation < *levels_[best].anchor->elevation) best = static_cast<int>(i);
        }
        return best;
    }

    void addElement(int parent, const ElementIn& element, Reason reason, const std::string& why, int multiplicity) {
        Node node;
        node.kind = Node::Kind::Element;
        node.label = element.label;
        node.model = element.model;
        node.ifc_id = element.ifc_id;
        node.object_id = element.object_id;
        node.reason = reason;
        node.reason_text = why;
        node.multiplicity = multiplicity;
        addChild(parent, addNode(node));
    }

    // The rooms of an element (BonsaiViewer13, plan tolerance 5 cm). The room
    // footprint is its real shape in plan; without it, its box is used.
    //  - Slab: below the room (top face at its floor) and under it in plan.
    //  - Wall, column: shares height with the room and touches it in plan.
    //  - Other: the centre of its box is in the room.
    std::vector<int> matchRooms(const RoomGrid& grid, const ElementIn& element) {
        std::vector<int> rooms;
        const Point2 centre{0.5f * (element.box.mn[0] + element.box.mx[0]),
                            0.5f * (element.box.mn[1] + element.box.mx[1])};
        const float centre_z = 0.5f * (element.box.mn[2] + element.box.mx[2]);
        Box query = element.box;
        if (!element.is_slab && !element.is_wall) {
            query.mn[0] = query.mx[0] = centre.x;
            query.mn[1] = query.mx[1] = centre.y;
        }
        const bool element_shape = !element.footprint.empty();
        for (int index : grid.candidates(query)) {
            const RoomIn& room = in_.rooms[index];
            const bool room_shape = !room.footprint.empty();
            if (!room_shape || ((element.is_slab || element.is_wall) && !element_shape)) ++box_fallbacks_;
            bool match = false;
            if (element.is_slab) {
                match = overlapsXY(element.box, room.box)
                    && std::fabs(element.box.mx[2] - room.box.mn[2]) <= settings_.tolerance
                    && (!room_shape || !element_shape || footprintsNear(element.footprint, room.footprint, kPlanTolerance));
            } else if (element.is_wall) {
                match = overlapsXY(element.box, room.box) && verticalOverlap(element.box, room.box) > kPlanTolerance
                    && (!room_shape || !element_shape || footprintsNear(element.footprint, room.footprint, kPlanTolerance));
            } else {
                match = centre_z >= room.box.mn[2] - kPlanTolerance && centre_z <= room.box.mx[2] + kPlanTolerance
                    && (room_shape ? pointNearFootprint(centre, room.footprint, kPlanTolerance)
                                   : centre.x >= room.box.mn[0] - kPlanTolerance && centre.x <= room.box.mx[0] + kPlanTolerance
                                         && centre.y >= room.box.mn[1] - kPlanTolerance && centre.y <= room.box.mx[1] + kPlanTolerance);
            }
            if (match) rooms.push_back(index);
        }
        return rooms;
    }

    // One level per element: when the rooms of `element` span several levels,
    // keep the rooms of one level only. Returns a note for the tooltip.
    std::string keepOneLevel(const ElementIn& element, std::vector<int>& rooms) const {
        std::map<int, float> height_by_level;  // level -> overlapped height, summed over its rooms
        for (int index : rooms) {
            const Box& room = in_.rooms[index].box;
            const float height = std::max(0.0f, std::min(element.box.mx[2], room.mx[2])
                                                    - std::max(element.box.mn[2], room.mn[2]));
            height_by_level[room_levels_[index]] += height;
        }
        if (height_by_level.size() < 2) return {};

        int kept = -1;
        std::string why;
        const int storey_level = levelOfStorey(element.model, element.storey_ifc);
        if (storey_level >= 0 && height_by_level.count(storey_level) != 0) {
            kept = storey_level;
            why = "level of its IFC storey";
        } else {
            float best = -1.0f;
            for (const auto& [level, height] : height_by_level) {
                const double elevation = level >= 0 && levels_[level].anchor->elevation
                    ? *levels_[level].anchor->elevation : 0.0;
                const double best_elevation = kept >= 0 && levels_[kept].anchor->elevation
                    ? *levels_[kept].anchor->elevation : 0.0;
                // Largest overlap; ties go to the lower level.
                if (height > best + 1e-4f || (std::fabs(height - best) <= 1e-4f && elevation < best_elevation)) {
                    best = height;
                    kept = level;
                }
            }
            why = "level where its box overlaps the rooms the most";
        }
        rooms.erase(std::remove_if(rooms.begin(), rooms.end(),
                                   [&](int index) { return room_levels_[index] != kept; }),
                    rooms.end());
        const std::string name = kept >= 0 ? levels_[kept].anchor->name : std::string("(no level)");
        return "\nOne level per element: kept " + quoted(name) + " (" + why + ") out of "
               + std::to_string(height_by_level.size()) + " levels";
    }

    void placeElements() {
        const RoomGrid grid(in_.rooms, usable_rooms_);
        for (const ElementIn& element : in_.elements) {
            // The rooms themselves are nodes, not elements.
            if (element.model == settings_.rooms_model && room_ifc_ids_.count(element.ifc_id) != 0) continue;
            if (element.object_id != 0 && room_object_ids_.count(element.object_id) != 0) continue;
            if (!validBox(element.box)) continue;

            std::vector<int> rooms = matchRooms(grid, element);

            std::string level_note;
            if (settings_.one_level_per_element) level_note = keepOneLevel(element, rooms);

            if (!rooms.empty()) {
                const int count = static_cast<int>(rooms.size());
                for (int index : rooms) {
                    const std::string& room_name = result_.nodes[room_nodes_[index]].label;
                    const std::string why = element.is_slab
                        ? "Slab below room " + quoted(room_name) + ": top face at its floor"
                        : element.is_wall
                            ? (count > 1 ? "Touches " + std::to_string(count) + " rooms, including " + quoted(room_name)
                                         : "Touches room " + quoted(room_name))
                            : "Its centre is in room " + quoted(room_name);
                    addElement(room_nodes_[index], element,
                               element.is_slab ? Reason::SlabBelowRoom : Reason::RoomOverlap, why + level_note, count);
                }
                continue;
            }

            if (element.is_slab) {
                const int level = levelAbove(element.box.mx[2]);
                if (level >= 0) {
                    addElement(levels_[level].node, element, Reason::SlabTopFace,
                               "Top face at " + metres(element.box.mx[2]) + ": first level above is "
                                   + quoted(levels_[level].anchor->name), 1);
                } else {
                    addElement(noLevelNode(), element, Reason::SlabTopFace,
                               "Top face at " + metres(element.box.mx[2]) + ": no level above", 1);
                }
                continue;
            }

            const int level = levelOfStorey(element.model, element.storey_ifc);
            if (level >= 0) {
                addElement(noRoomNode(level), element, Reason::NoRoomFromStorey,
                           "In no room: level of its IFC storey", 1);
            } else {
                addElement(noLevelNode(), element, Reason::NoRoomFromStorey,
                           "In no room, and no IFC storey", 1);
            }
        }
    }

    // ---- Finish --------------------------------------------------------------

    void sortChildren(int node) {
        std::vector<int>& children = node < 0 ? result_.roots : result_.nodes[node].children;
        // Levels keep their elevation order; the rest is sorted by kind then label.
        if (node < 0 || result_.nodes[node].kind != Node::Kind::Building) {
            std::stable_sort(children.begin(), children.end(), [this](int a, int b) {
                const Node& na = result_.nodes[a];
                const Node& nb = result_.nodes[b];
                if (na.kind != nb.kind) return static_cast<int>(na.kind) < static_cast<int>(nb.kind);
                return na.label < nb.label;
            });
        }
        for (int child : children) sortChildren(child);
    }

    // Distinct elements below each node: an element listed under several rooms
    // counts once in its level and above.
    void countBelow(int node, std::unordered_set<std::uint32_t>& out) {
        Node& current = result_.nodes[node];
        std::unordered_set<std::uint32_t> mine;
        if (current.kind == Node::Kind::Element && current.object_id != 0) mine.insert(current.object_id);
        for (int child : current.children) countBelow(child, mine);
        current.count = static_cast<int>(mine.size());
        out.insert(mine.begin(), mine.end());
    }

    void finish() {
        if (box_fallbacks_ > 0) {
            report(std::to_string(box_fallbacks_) + " room tests used bounding boxes: the shape of the room or "
                   "of the element was not loaded (geometry not resident).");
        }
        sortChildren(-1);
        std::unordered_set<std::uint32_t> all;
        for (int root : result_.roots) countBelow(root, all);
        if (!result_.diagnostics.empty()) {
            Node node;
            node.kind = Node::Kind::Diagnostics;
            node.label = "Diagnostics";
            const int id = addNode(node);
            for (const std::string& message : result_.diagnostics) {
                Node line;
                line.kind = Node::Kind::Message;
                line.label = message;
                result_.nodes[id].children.push_back(addNode(line));
            }
            result_.nodes[id].count = static_cast<int>(result_.diagnostics.size());
            result_.roots.push_back(id);
        }
    }

    const Input& in_;
    const Settings& settings_;
    Result result_;
    std::vector<Level> levels_;
    std::map<std::pair<std::uint32_t, int>, int> level_of_;  // (model, storey) -> level
    std::map<std::string, int> reference_by_name_;           // unique reference names
    std::vector<int> usable_rooms_;
    std::vector<int> room_nodes_;
    std::vector<int> room_levels_;  // level of each room, -1 when none
    long long box_fallbacks_ = 0;   // room tests made on boxes (shape missing)
    std::unordered_set<int> room_ifc_ids_;
    std::unordered_set<std::uint32_t> room_object_ids_;
    int no_level_node_ = -1;
};

void collectBelow(const Result& result, int node, std::unordered_set<std::uint32_t>& seen,
                  std::vector<std::uint32_t>& out) {
    const Node& current = result.nodes[node];
    if (current.object_id != 0 && seen.insert(current.object_id).second) out.push_back(current.object_id);
    for (int child : current.children) collectBelow(result, child, seen, out);
}

} // namespace

Result build(const Input& input) {
    return Builder(input).run();
}

std::vector<std::uint32_t> objectIdsBelow(const Result& result, int node) {
    std::vector<std::uint32_t> ids;
    if (node < 0 || node >= static_cast<int>(result.nodes.size())) return ids;
    std::unordered_set<std::uint32_t> seen;
    collectBelow(result, node, seen, ids);
    return ids;
}

} // namespace bonsaiviewer::services::unified
