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

// PORT — see Unit.h.

#include "Unit.h"

#include "../../../helpers/unit_convert.h"
#include "../../../ifcparse/file.h"
#include "../../../ifcparse/schema.h"

namespace bonsaiviewer::ports::ifcopenshell_util {

namespace {

void replaceAll(std::string& text, const std::string& from) {
    for (std::size_t at = text.find(from); at != std::string::npos; at = text.find(from)) text.erase(at, from.size());
}

// The measure class of the value at attribute 3 of an IfcPhysicalSimpleQuantity
// (entity.attribute_by_index(3).type_of_attribute().declared_type().name()).
std::string quantityMeasureClass(const Entity& prop) {
    const auto* entity = prop.declaration().as_entity();
    if (!entity || entity->attribute_count() < 4) return {};
    const auto* type = entity->attribute_by_index(3)->type_of_attribute();
    if (const auto* named = type->as_named_type()) return named->declared_type()->name();
    return {};
}

Entity firstOf(const std::vector<Entity>& values) {
    return values.empty() ? Entity() : values.front();
}

} // namespace

const std::map<std::string, std::string>& si_type_names() {
    static const std::map<std::string, std::string> names = {
        {"ABSORBEDDOSEUNIT", "GRAY"},
        {"AMOUNTOFSUBSTANCEUNIT", "MOLE"},
        {"AREAUNIT", "SQUARE_METRE"},
        {"DOSEEQUIVALENTUNIT", "SIEVERT"},
        {"ELECTRICCAPACITANCEUNIT", "FARAD"},
        {"ELECTRICCHARGEUNIT", "COULOMB"},
        {"ELECTRICCONDUCTANCEUNIT", "SIEMENS"},
        {"ELECTRICCURRENTUNIT", "AMPERE"},
        {"ELECTRICRESISTANCEUNIT", "OHM"},
        {"ELECTRICVOLTAGEUNIT", "VOLT"},
        {"ENERGYUNIT", "JOULE"},
        {"FORCEUNIT", "NEWTON"},
        {"FREQUENCYUNIT", "HERTZ"},
        {"ILLUMINANCEUNIT", "LUX"},
        {"INDUCTANCEUNIT", "HENRY"},
        {"LENGTHUNIT", "METRE"},
        {"LUMINOUSFLUXUNIT", "LUMEN"},
        {"LUMINOUSINTENSITYUNIT", "CANDELA"},
        {"MAGNETICFLUXDENSITYUNIT", "TESLA"},
        {"MAGNETICFLUXUNIT", "WEBER"},
        {"MASSUNIT", "GRAM"},
        {"PLANEANGLEUNIT", "RADIAN"},
        {"POWERUNIT", "WATT"},
        {"PRESSUREUNIT", "PASCAL"},
        {"RADIOACTIVITYUNIT", "BECQUEREL"},
        {"SOLIDANGLEUNIT", "STERADIAN"},
        {"THERMODYNAMICTEMPERATUREUNIT", "KELVIN"},
        {"TIMEUNIT", "SECOND"},
        {"VOLUMEUNIT", "CUBIC_METRE"},
        {"USERDEFINED", "METRE"},
    };
    return names;
}

// def get_measure_unit_type(measure_class)
std::string get_measure_unit_type(const std::string& measure_class) {
    if (measure_class == "IfcNumericMeasure") return "USERDEFINED";
    std::string text = measure_class;
    for (const char* part : {"Ifc", "Measure", "Non", "Positive", "Negative"}) replaceAll(text, part);
    for (char& c : text) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return text + "UNIT";
}

// def get_project_unit(ifc_file, unit_type)
Entity get_project_unit(const Entity& any_entity_of_the_file, const std::string& unit_type) {
    if (!any_entity_of_the_file) return {};
    ifcopenshell::file* file = any_entity_of_the_file.file();
    if (!file) return {};
    const auto projects = file->instances_by_type("IfcProject");
    if (projects.empty()) return {};
    const Entity unit_assignment = forwardEntity(projects.front(), "UnitsInContext");
    if (!unit_assignment) return {};
    for (const Entity& unit : forwardEntities(unit_assignment, "Units")) {
        if (forwardString(unit, "UnitType") == unit_type) return unit;
    }
    return {};
}

// def get_property_unit(prop, ifc_file)
Entity get_property_unit(const Entity& prop) {
    if (const Entity unit = forwardEntity(prop, "Unit")) return unit;
    std::string measure_class;
    if (isA(prop, "IfcPhysicalSimpleQuantity")) {
        measure_class = quantityMeasureClass(prop);
    } else if (isA(prop, "IfcPropertySingleValue")) {
        if (const Entity value = forwardEntity(prop, "NominalValue")) measure_class = className(value);
    } else if (isA(prop, "IfcPropertyEnumeratedValue")) {
        if (const Entity reference = forwardEntity(prop, "EnumerationReference")) {
            if (const Entity unit = forwardEntity(reference, "Unit")) return unit;
            if (const Entity value = firstOf(forwardEntities(reference, "EnumerationValues"))) {
                measure_class = className(value);
            }
        }
        if (const Entity value = firstOf(forwardEntities(prop, "EnumerationValues"))) measure_class = className(value);
    } else if (isA(prop, "IfcPropertyListValue")) {
        if (const Entity value = firstOf(forwardEntities(prop, "ListValues"))) measure_class = className(value);
    } else if (isA(prop, "IfcPropertyBoundedValue")) {
        Entity value = forwardEntity(prop, "UpperBoundValue");
        if (!value) value = forwardEntity(prop, "LowerBoundValue");
        if (!value) value = forwardEntity(prop, "SetPointValue");
        if (value) measure_class = className(value);
    }
    if (measure_class.empty()) return {};
    return get_project_unit(prop, get_measure_unit_type(measure_class));
}

// def get_property_table_unit(prop, ifc_file)
std::pair<Entity, Entity> get_property_table_unit(const Entity& prop) {
    Entity defining_unit = forwardEntity(prop, "DefiningUnit");
    if (!defining_unit) {
        if (const Entity value = firstOf(forwardEntities(prop, "DefiningValues"))) {
            defining_unit = get_project_unit(prop, get_measure_unit_type(className(value)));
        }
    }
    Entity defined_unit = forwardEntity(prop, "DefinedUnit");
    if (!defined_unit) {
        if (const Entity value = firstOf(forwardEntities(prop, "DefinedValues"))) {
            defined_unit = get_project_unit(prop, get_measure_unit_type(className(value)));
        }
    }
    return {defining_unit, defined_unit};
}

std::optional<std::string> unitName(const Entity& unit) {
    return forwardString(unit, "Name");
}

std::optional<std::string> unitPrefix(const Entity& unit) {
    return forwardString(unit, "Prefix");
}

std::optional<std::string> unitType(const Entity& unit) {
    return forwardString(unit, "UnitType");
}

// def convert(value, from_prefix, from_unit, to_prefix, to_unit) — delegates
// to the existing port in src/helpers (same tables, same rules).
double convert(double value, const std::optional<std::string>& from_prefix, const std::string& from_unit,
               const std::optional<std::string>& to_prefix, const std::string& to_unit) {
    return ::convert(value, from_prefix.value_or(std::string()), from_unit, to_prefix.value_or(std::string()), to_unit);
}

} // namespace bonsaiviewer::ports::ifcopenshell_util
