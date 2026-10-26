# This file was generated with the assistance of an AI coding tool.
"""The IDS worker of Bonsai Viewer: a JSON Lines process wrapping IfcTester.

Launch: ``python /path/to/ifctester_ipc.py`` — the script lives inside the
IfcOpenShell ``src/ifctester`` directory, so its parent is added to
``sys.path`` and the ``ifctester`` package imports directly.

Protocol (BonsaiViewer06 Filter Rule): one JSON message per line on stdin
(requests) and stdout (replies), UTF-8. A reply repeats the request ``id``.

Facets
------
A selection is a list of facets, close to the IDS structure::

    {"type": "entity", "name": VALUE, "includeSubtypes": true,
     "predefinedType": VALUE}
    {"type": "property", "propertySet": VALUE, "baseName": VALUE,
     "value": VALUE, "cardinality": "required" | "optional" | "prohibited"}

and every IDS field that accepts a restriction is a VALUE::

    null                                         no constraint
    {"kind": "simple", "value": "REI60"}
    {"kind": "enumeration", "values": ["REI60", "REI120"]}
    {"kind": "pattern", "pattern": "Pset_.*Common"}       (XSD regex)
    {"kind": "range", "min": "0.1", "minInclusive": true,
     "max": "0.3", "maxInclusive": false, "base": "double"}
    {"kind": "length", "min": "2", "max": "5"}   (characters; min == max
                                                 is an exact length)

The entity facet goes to the IDS applicability, the property facets to the
requirements; an element is selected when it passes all of them. IDS
matches the IFC class exactly (IfcWall does not match IfcWallStandardCase):
the entity ``name`` VALUE is a class (simple), several classes
(enumeration) or a pattern on the class name. Several classes, or
``includeSubtypes``, are written as an xs:enumeration of the classes (and
their subtypes); ``parse`` folds the enumeration back into the classes the
user picked. The 2.0 ``names`` list and the 1.x ``name`` string are still
accepted.

Requests
--------
    hello      -> the engine's identity and versions
    build      -> {name, facets} -> {ids_xml}: canonical IDS XML built by
                  IfcTester itself (Ids.to_string). The 1.1 payload
                  ({entity, properties[{comparison...}]}) is still accepted.
    parse      -> {ids_xml} -> {name, facets}
    select     -> {ifc, ids_xml, limit_ids?} -> {ids, count}: the EXPRESS
                  ids passing every facet of the first specification.
                  ``limit_ids`` restricts the candidates (Run on the box).
    preview    -> {ifc, facets, limit_ids?} -> {ids, count}: build + select
                  in one go, for the live preview of the creation window.
    classes    -> {ifc?} -> {classes, schema}: the selectable classes
                  (IfcObjectDefinition and subtypes) of the model's schema,
                  or of IFC2X3 + IFC4 + IFC4X3 without a model.
    classInfo  -> {name: VALUE, includeSubtypes, ifc?} -> {classes,
                  predefinedTypes, count?}: the classes the entity facet
                  covers, the PredefinedType enumeration of the class and,
                  with a model, how many elements it matches.
    properties -> {ifc, entity} -> {psets: {pset: {property: count}},
                  samples: {pset: {property: [up to 3 values]}}}: the
                  property sets (and quantity sets) found on the elements
                  of the entity facet, for the property picker.
    values     -> {ifc, entity, propertySet, baseName} -> {values:[{value,
                  count}], missing, base}: the distinct values of a property
                  on those elements; ``base`` is double / integer / boolean /
                  string, guessed from the values.
    shutdown   -> exit

Only the worker's own JSON ever reaches stdout; library output goes to
stderr.
"""

import json
import os
import re
import sys
import traceback

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import ifcopenshell  # noqa: E402
import ifcopenshell.ifcopenshell_wrapper as wrapper  # noqa: E402
import ifcopenshell.util.element  # noqa: E402
from elementpath.regex import translate_pattern  # noqa: E402
from ifctester import facet, ids  # noqa: E402

ENGINE_NAME = "ifctester"
ENGINE_VERSION = "2.1"

SCHEMAS = ("IFC2X3", "IFC4", "IFC4X3_ADD2")
CARDINALITIES = ("required", "optional", "prohibited")
BOUND_KEYS = ("minInclusive", "minExclusive", "maxInclusive", "maxExclusive")

# One open file, reopened when the file changes on disk: opening a large
# IFC is by far the most expensive part of every request.
_file_cache = {}


class UserError(Exception):
    """A request the user can fix (bad class, bad number...)."""


def open_ifc(path):
    if not path:
        return None
    if not os.path.isfile(path):
        raise UserError(f"the IFC file cannot be found: {path}")
    mtime = os.path.getmtime(path)
    cached = _file_cache.get(path)
    if cached is not None and cached[0] == mtime:
        return cached[1]
    ifc_file = ifcopenshell.open(path)
    _file_cache.clear()  # keep a single model in memory
    _file_cache[path] = (mtime, ifc_file)
    return ifc_file


def reply(req_id, **payload):
    out = {"id": req_id, "ok": True}
    out.update(payload)
    return out


def error(req_id, message):
    return {"id": req_id, "ok": False, "error": message}


# --------------------------------------------------------------------------
# Schema helpers
# --------------------------------------------------------------------------


def _schema_names(ifc_file=None):
    if ifc_file is not None:
        return [ifc_file.schema_identifier] if ifc_file.schema_identifier in SCHEMAS else [ifc_file.schema]
    return list(SCHEMAS)


def _declaration(schema_name, class_name):
    try:
        return wrapper.schema_by_name(schema_name).declaration_by_name(class_name)
    except Exception:  # noqa: BLE001 — the class does not exist in this schema
        return None


def _walk_subtypes(declaration):
    result = []
    stack = [declaration]
    while stack:
        current = stack.pop()
        result.append(current.name())
        if hasattr(current, "subtypes"):
            stack.extend(sorted(current.subtypes(), key=lambda d: d.name(), reverse=True))
    return result


def class_names(class_name, include_subtypes, ifc_file=None):
    """The class first (canonical case), then its subtypes."""
    names = []
    for schema_name in _schema_names(ifc_file):
        declaration = _declaration(schema_name, class_name)
        if declaration is None:
            continue
        found = _walk_subtypes(declaration) if include_subtypes else [declaration.name()]
        for name in found:
            if name not in names:
                names.append(name)
    if not names:
        raise UserError(f"unknown IFC class: {class_name}")
    return names


def predefined_types(class_name, ifc_file=None):
    values = []
    for schema_name in _schema_names(ifc_file):
        declaration = _declaration(schema_name, class_name)
        if declaration is None or not hasattr(declaration, "all_attributes"):
            continue
        for attribute in declaration.all_attributes():
            if attribute.name() != "PredefinedType":
                continue
            attribute_type = attribute.type_of_attribute()
            while hasattr(attribute_type, "declared_type"):
                attribute_type = attribute_type.declared_type()
            if hasattr(attribute_type, "enumeration_items"):
                for item in attribute_type.enumeration_items():
                    if item not in values:
                        values.append(item)
    return values


def selectable_classes(ifc_file=None):
    names = set()
    for schema_name in _schema_names(ifc_file):
        root = _declaration(schema_name, "IfcObjectDefinition")
        if root is not None:
            names.update(_walk_subtypes(root))
    return sorted(names)


# --------------------------------------------------------------------------
# VALUE <-> ifctester
# --------------------------------------------------------------------------


def _number(text, label):
    try:
        return float(str(text).strip().replace(",", "."))
    except ValueError:
        raise UserError(f"{label}: '{text}' is not a number") from None


def to_ids_value(value, label):
    """VALUE json -> None | str | facet.Restriction."""
    if value is None:
        return None
    kind = value.get("kind") or "simple"
    if kind == "any":
        return None
    if kind == "simple":
        text = str(value.get("value") or "").strip()
        return text or None
    if kind == "enumeration":
        options = [str(v).strip() for v in value.get("values") or [] if str(v).strip()]
        if not options:
            raise UserError(f"{label}: the list of values is empty")
        return facet.Restriction(options={"enumeration": options})
    if kind == "pattern":
        pattern = str(value.get("pattern") or "")
        if not pattern:
            raise UserError(f"{label}: the pattern is empty")
        return facet.Restriction(options={"pattern": pattern})
    if kind == "range":
        base = value.get("base") or "double"
        options = {}
        low = str(value.get("min") or "").strip()
        high = str(value.get("max") or "").strip()
        if low:
            key = "minInclusive" if value.get("minInclusive", True) else "minExclusive"
            options[key] = _number(low, label)
        if high:
            key = "maxInclusive" if value.get("maxInclusive", True) else "maxExclusive"
            options[key] = _number(high, label)
        if not options:
            raise UserError(f"{label}: the range needs a minimum or a maximum")
        if low and high and _number(low, label) > _number(high, label):
            raise UserError(f"{label}: the minimum is greater than the maximum")
        if base == "integer":
            options = {k: int(v) if float(v).is_integer() else v for k, v in options.items()}
        return facet.Restriction(options=options, base=base)
    if kind == "length":
        low = str(value.get("min") or "").strip()
        high = str(value.get("max") or "").strip()
        try:
            low = int(low) if low else None
            high = int(high) if high else None
        except ValueError:
            raise UserError(f"{label}: a length is a whole number of characters") from None
        if low is None and high is None:
            raise UserError(f"{label}: the length needs a minimum or a maximum")
        if low is not None and high is not None and low > high:
            raise UserError(f"{label}: the minimum length is greater than the maximum")
        if low is not None and low == high:
            return facet.Restriction(options={"length": low})
        options = {}
        if low is not None:
            options["minLength"] = low
        if high is not None:
            options["maxLength"] = high
        return facet.Restriction(options=options)
    raise UserError(f"{label}: unknown value kind '{kind}'")


def _format_number(number):
    number = float(number)
    return str(int(number)) if number.is_integer() else repr(number)


def from_ids_value(value):
    """None | str | facet.Restriction -> VALUE json."""
    if value is None or value == "":
        return None
    if not isinstance(value, facet.Restriction):
        return {"kind": "simple", "value": str(value)}
    options = value.options or {}
    if "enumeration" in options:
        values = options["enumeration"]
        values = values if isinstance(values, list) else [values]
        return {"kind": "enumeration", "values": [str(v) for v in values]}
    if "pattern" in options:
        pattern = options["pattern"]
        pattern = pattern[0] if isinstance(pattern, list) else pattern
        return {"kind": "pattern", "pattern": str(pattern)}
    if any(key in options for key in BOUND_KEYS):
        result = {"kind": "range", "base": value.base or "double", "min": "", "max": "",
                  "minInclusive": True, "maxInclusive": True}
        for key in BOUND_KEYS:
            if key not in options:
                continue
            bound = options[key]
            bound = bound[0] if isinstance(bound, list) else bound
            side = "min" if key.startswith("min") else "max"
            result[side] = _format_number(bound)
            result[side + "Inclusive"] = key.endswith("Inclusive")
        return result
    if any(key in options for key in ("length", "minLength", "maxLength")):
        def first(key):
            bound = options.get(key)
            bound = bound[0] if isinstance(bound, list) else bound
            return "" if bound is None else str(int(float(bound)))
        if "length" in options:
            return {"kind": "length", "min": first("length"), "max": first("length")}
        return {"kind": "length", "min": first("minLength"), "max": first("maxLength")}
    # totalDigits / fractionDigits: IfcTester does not evaluate them.
    raise UserError(f"unsupported restriction: {options}")


# --------------------------------------------------------------------------
# facets <-> ifctester
# --------------------------------------------------------------------------


def legacy_facets(req):
    """The 1.1 payload: {entity, properties[{comparison, value...}]}."""
    entity = req.get("entity") or {}
    facets = [{
        "type": "entity",
        "names": [entity.get("name") or ""],
        "includeSubtypes": bool(entity.get("includeSubtypes", False)),
        "predefinedType": ({"kind": "simple", "value": entity["predefinedType"]}
                           if entity.get("predefinedType") else None),
    }]
    properties = req.get("properties")
    if properties is None:
        properties = [req["property"]] if req.get("property") else []
    bounds = {"greater": ("min", False), "greaterOrEqual": ("min", True),
              "less": ("max", False), "lessOrEqual": ("max", True)}
    for prop in properties:
        comparison = prop.get("comparison") or ("equals" if (prop.get("value") or "").strip() else "any")
        text = prop.get("value") or ""
        if comparison == "equals":
            value = {"kind": "simple", "value": text}
        elif comparison == "oneOf":
            value = {"kind": "enumeration", "values": [v.strip() for v in text.split(";") if v.strip()]}
        elif comparison == "pattern":
            value = {"kind": "pattern", "pattern": text}
        elif comparison in bounds:
            side, inclusive = bounds[comparison]
            value = {"kind": "range", "base": "double", side: text, side + "Inclusive": inclusive}
        else:
            value = None
        facets.append({
            "type": "property",
            "propertySet": {"kind": "simple", "value": prop.get("propertySet") or ""},
            "baseName": {"kind": "simple", "value": prop.get("baseName") or ""},
            "value": value,
            "cardinality": prop.get("cardinality") or "required",
        })
    return facets


def entity_value(spec_facet):
    """The class VALUE of an entity facet (2.1), from ``names`` (2.0) or a
    plain ``name`` string (1.x)."""
    name = spec_facet.get("name")
    if isinstance(name, dict):
        return name
    names = spec_facet.get("names")
    if names is None:
        names = [name or ""]
    names = [str(n).strip() for n in names if str(n).strip()]
    if len(names) == 1:
        return {"kind": "simple", "value": names[0]}
    return {"kind": "enumeration", "values": names}


def entity_names(spec_facet, ifc_file=None):
    """The classes an entity facet names (a pattern is matched against the
    selectable classes of the schema)."""
    value = entity_value(spec_facet)
    kind = value.get("kind") or "simple"
    if kind == "simple":
        names = [str(value.get("value") or "").strip()]
    elif kind == "enumeration":
        names = [str(v).strip() for v in value.get("values") or []]
    elif kind == "pattern":
        expression = re.compile(translate_pattern(_upper_pattern(value.get("pattern") or ""),
                                                  back_references=False, lazy_quantifiers=False,
                                                  anchors=False))
        # May be empty: not an error, the facet then selects nothing (the
        # editor shows a warning).
        return [c for c in selectable_classes(ifc_file) if expression.fullmatch(c.upper())]
    else:
        raise UserError(f"the IFC class cannot be a {kind}")
    names = [n for n in names if n]
    if not names:
        raise UserError("the entity facet needs an IFC class")
    return names


def _upper_pattern(pattern):
    """IDS compares the entity name in upper case: upper-case the letters of
    the pattern, but not the escapes (\\d, \\s...)."""
    out = []
    escaped = False
    for char in pattern:
        out.append(char if escaped else char.upper())
        escaped = char == "\\" and not escaped
    return "".join(out)


def covered_classes(names, include_subtypes, ifc_file=None):
    covered = []
    for class_name in names:
        for name in class_names(class_name, include_subtypes, ifc_file):
            if name not in covered:
                covered.append(name)
    return covered


def class_predefined_types(names, ifc_file=None):
    values = []
    for class_name in names:
        for value in predefined_types(class_name, ifc_file):
            if value not in values:
                values.append(value)
    return values


def entity_facet(spec_facet, ifc_file=None):
    value = entity_value(spec_facet)
    names = entity_names(spec_facet, ifc_file)
    # A pattern stays a pattern, unless the subtypes widen it to the
    # classes it matches (when it matches some).
    if value.get("kind") == "pattern" and not (spec_facet.get("includeSubtypes") and names):
        name = facet.Restriction(options={"pattern": _upper_pattern(value["pattern"])})
    else:
        upper = [n.upper() for n in covered_classes(names, bool(spec_facet.get("includeSubtypes")), ifc_file)]
        name = upper[0] if len(upper) == 1 else facet.Restriction(options={"enumeration": upper})
    predefined = to_ids_value(spec_facet.get("predefinedType"), "PredefinedType")
    if isinstance(predefined, str) and predefined.upper() in class_predefined_types(names):
        predefined = predefined.upper()
    return facet.Entity(name=name, predefinedType=predefined)


def fold_classes(values):
    """An entity enumeration -> (classes, includeSubtypes).

    The roots are the classes whose supertype is not listed. When the
    enumeration is exactly the roots and all their subtypes, it was written
    with includeSubtypes; otherwise every class was picked one by one.
    """
    canonical = []
    for value in values:
        name = _canonical_class(str(value))
        if name not in canonical:
            canonical.append(name)
    listed = set(canonical)
    roots = []
    for name in canonical:
        if not any(_is_strict_subtype(name, other) for other in listed if other != name):
            roots.append(name)
    try:
        if set(covered_classes(roots, True)) == listed and len(listed) > len(roots):
            return roots, True
    except UserError:
        pass
    return canonical, False


def _is_strict_subtype(name, other):
    for schema_name in SCHEMAS:
        declaration = _declaration(schema_name, name)
        while declaration is not None and hasattr(declaration, "supertype"):
            declaration = declaration.supertype()
            if declaration is not None and declaration.name() == other:
                return True
    return False


def build_specification(name, facets):
    spec = ids.Specification(name=name)
    applicability = []
    requirements = []
    entities = [f for f in facets if f.get("type") == "entity"]
    if len(entities) != 1:
        raise UserError("a selection needs exactly one entity facet")
    for index, spec_facet in enumerate(facets, start=1):
        kind = spec_facet.get("type")
        if kind == "entity":
            applicability.append(entity_facet(spec_facet))
        elif kind == "property":
            label = f"property facet {index}"
            pset = to_ids_value(spec_facet.get("propertySet"), label + " (property set)")
            base_name = to_ids_value(spec_facet.get("baseName"), label + " (property)")
            if pset is None or base_name is None:
                raise UserError(f"{label}: both the property set and the property name are needed")
            cardinality = spec_facet.get("cardinality") or "required"
            if cardinality not in CARDINALITIES:
                raise UserError(f"{label}: unknown cardinality '{cardinality}'")
            value = to_ids_value(spec_facet.get("value"), label)
            requirements.append(facet.Property(propertySet=pset, baseName=base_name, value=value,
                                               cardinality=cardinality))
        else:
            raise UserError(f"facet {index}: unsupported facet type '{kind}'")
    spec.applicability = applicability
    spec.requirements = requirements
    return spec


def parse_facets(spec):
    facets = []
    for ifc_facet in spec.applicability + spec.requirements:
        if isinstance(ifc_facet, facet.Entity):
            name = ifc_facet.name
            include_subtypes = False
            if isinstance(name, facet.Restriction) and "enumeration" in (name.options or {}):
                values = name.options["enumeration"]
                values = values if isinstance(values, list) else [values]
                names, include_subtypes = fold_classes(values)
                value = ({"kind": "simple", "value": names[0]} if len(names) == 1
                         else {"kind": "enumeration", "values": names})
            elif isinstance(name, facet.Restriction):
                value = from_ids_value(name)
            else:
                value = {"kind": "simple", "value": _canonical_class(name or "")}
            facets.append({
                "type": "entity",
                "name": value,
                "includeSubtypes": include_subtypes,
                "predefinedType": from_ids_value(ifc_facet.predefinedType),
            })
        elif isinstance(ifc_facet, facet.Property):
            facets.append({
                "type": "property",
                "propertySet": from_ids_value(ifc_facet.propertySet),
                "baseName": from_ids_value(ifc_facet.baseName),
                "value": from_ids_value(ifc_facet.value),
                "cardinality": ifc_facet.cardinality or "required",
            })
        else:
            raise UserError(f"unsupported facet in this IDS: {type(ifc_facet).__name__}")
    return facets


def _canonical_class(name):
    for schema_name in SCHEMAS:
        declaration = _declaration(schema_name, name)
        if declaration is not None:
            return declaration.name()
    return name


# --------------------------------------------------------------------------
# selection
# --------------------------------------------------------------------------


def select_ids(ifc_file, spec, limit=None):
    # Broadphase applicability, exactly like Specification.validate: each
    # applicability facet narrows the candidate list.
    elements = None
    for ifc_facet in spec.applicability:
        elements = ifc_facet.filter(ifc_file, elements)
    if elements is None:
        if limit is not None:
            elements = [e for e in (ifc_file.by_id(i) for i in sorted(limit)) if e is not None]
        else:
            elements = ifc_file.by_type("IfcProduct")
    elif limit is not None:
        elements = [e for e in elements if e.id() in limit]
    # The element must pass every facet; applicability facets are re-tested
    # since some broadphase filters only over-approximate.
    selected = []
    for element in elements:
        if all(bool(f(element)) for f in spec.applicability) and all(
            bool(f(element)) for f in spec.requirements
        ):
            selected.append(element.id())
    return sorted(set(selected))


def _limit(req):
    limit_ids = req.get("limit_ids")
    return set(int(i) for i in limit_ids) if limit_ids is not None else None


def entity_elements(ifc_file, entity):
    spec = ids.Specification(name="entity")
    spec.applicability = [entity_facet(dict(entity, type="entity"), ifc_file)]
    spec.requirements = []
    return [ifc_file.by_id(i) for i in select_ids(ifc_file, spec)]


# --------------------------------------------------------------------------
# ops
# --------------------------------------------------------------------------


def op_hello(req_id, req):
    return reply(req_id, engine=ENGINE_NAME, engine_version=ENGINE_VERSION,
                 ifcopenshell_version=ifcopenshell.version,
                 ifctester_path=os.path.dirname(os.path.abspath(facet.__file__)))


def op_build(req_id, req):
    name = (req.get("name") or "Selection").strip() or "Selection"
    facets = req.get("facets")
    if facets is None:
        facets = legacy_facets(req)
    doc = ids.Ids(title=name)
    doc.specifications = [build_specification(name, facets)]
    return reply(req_id, ids_xml=doc.to_string())


def op_parse(req_id, req):
    ids_xml = req.get("ids_xml")
    if not ids_xml:
        return error(req_id, "parse needs 'ids_xml'")
    doc = ids.from_string(ids_xml)
    if not doc.specifications:
        return error(req_id, "the IDS document has no specification")
    spec = doc.specifications[0]
    return reply(req_id, name=spec.name or "", facets=parse_facets(spec))


def op_select(req_id, req):
    ifc_path = req.get("ifc")
    ids_xml = req.get("ids_xml")
    if not ifc_path or not ids_xml:
        return error(req_id, "select needs 'ifc' and 'ids_xml'")
    ifc_file = open_ifc(ifc_path)
    doc = ids.from_string(ids_xml)
    if not doc.specifications:
        return error(req_id, "the IDS document has no specification")
    selected = select_ids(ifc_file, doc.specifications[0], _limit(req))
    return reply(req_id, ids=selected, count=len(selected))


def op_preview(req_id, req):
    ifc_file = open_ifc(req.get("ifc"))
    if ifc_file is None:
        return error(req_id, "no model to preview on")
    spec = build_specification("preview", req.get("facets") or [])
    selected = select_ids(ifc_file, spec, _limit(req))
    return reply(req_id, ids=selected, count=len(selected))


def op_classes(req_id, req):
    ifc_file = open_ifc(req.get("ifc")) if req.get("ifc") else None
    schema = ifc_file.schema_identifier if ifc_file is not None else "+".join(SCHEMAS)
    return reply(req_id, classes=selectable_classes(ifc_file), schema=schema)


def op_class_info(req_id, req):
    ifc_file = open_ifc(req.get("ifc")) if req.get("ifc") else None
    names = entity_names(req, ifc_file)
    include = bool(req.get("includeSubtypes"))
    payload = {"classes": covered_classes(names, include, ifc_file),
               "predefinedTypes": class_predefined_types(names, ifc_file)}
    if not names:
        payload["warning"] = "the class pattern matches no IFC class: the layer will select nothing"
    if ifc_file is not None:
        payload["count"] = len(entity_elements(ifc_file, {"name": entity_value(req), "includeSubtypes": include}))
    return reply(req_id, **payload)


def op_properties(req_id, req):
    ifc_file = open_ifc(req.get("ifc"))
    if ifc_file is None:
        return error(req_id, "no model to read the properties from")
    psets = {}
    samples = {}
    for element in entity_elements(ifc_file, req.get("entity") or {}):
        for pset_name, properties in ifcopenshell.util.element.get_psets(element).items():
            target = psets.setdefault(pset_name, {})
            target_samples = samples.setdefault(pset_name, {})
            for property_name, value in properties.items():
                if property_name == "id":
                    continue
                target[property_name] = target.get(property_name, 0) + 1
                shown = target_samples.setdefault(property_name, [])
                if value not in (None, "") and len(shown) < 3:
                    text = _value_text(value)
                    if text not in shown:
                        shown.append(text)
    return reply(req_id, psets=psets, samples=samples)


def _value_text(value):
    if isinstance(value, bool):
        return "true" if value else "false"
    if isinstance(value, float):
        return _format_number(value)
    return str(value)


def op_values(req_id, req):
    ifc_file = open_ifc(req.get("ifc"))
    if ifc_file is None:
        return error(req_id, "no model to read the values from")
    pset_name = req.get("propertySet") or ""
    property_name = req.get("baseName") or ""
    counts = {}
    kinds = set()
    missing = 0
    for element in entity_elements(ifc_file, req.get("entity") or {}):
        properties = ifcopenshell.util.element.get_psets(element).get(pset_name)
        if properties is None or property_name not in properties or properties[property_name] in (None, ""):
            missing += 1
            continue
        value = properties[property_name]
        kinds.add("boolean" if isinstance(value, bool) else
                  "integer" if isinstance(value, int) else
                  "double" if isinstance(value, float) else "string")
        text = _value_text(value)
        counts[text] = counts.get(text, 0) + 1
    if kinds <= {"integer"} and kinds:
        base = "integer"
    elif kinds and kinds <= {"integer", "double"}:
        base = "double"
    elif kinds == {"boolean"}:
        base = "boolean"
    else:
        base = "string"
    values = sorted(({"value": v, "count": c} for v, c in counts.items()),
                    key=lambda item: (-item["count"], item["value"]))
    return reply(req_id, values=values, missing=missing, base=base)


def main():
    handlers = {
        "hello": op_hello,
        "build": op_build,
        "parse": op_parse,
        "select": op_select,
        "preview": op_preview,
        "classes": op_classes,
        "classInfo": op_class_info,
        "properties": op_properties,
        "values": op_values,
    }
    for line in sys.stdin:
        line = line.strip()
        if not line:
            continue
        try:
            request = json.loads(line)
        except json.JSONDecodeError as e:
            print(json.dumps(error(None, f"invalid JSON: {e}")), flush=True)
            continue
        req_id = request.get("id")
        op = request.get("op")
        if op == "shutdown":
            print(json.dumps(reply(req_id)), flush=True)
            return
        handler = handlers.get(op)
        if handler is None:
            print(json.dumps(error(req_id, f"unknown op: {op}")), flush=True)
            continue
        try:
            print(json.dumps(handler(req_id, request)), flush=True)
        except UserError as e:
            print(json.dumps(error(req_id, str(e))), flush=True)
        except Exception as e:  # noqa: BLE001 — the worker must survive bad input
            sys.stderr.write(traceback.format_exc())
            print(json.dumps(error(req_id, f"the IDS engine failed: {e}")), flush=True)


if __name__ == "__main__":
    main()
