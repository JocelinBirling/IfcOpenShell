# This file was generated with the assistance of an AI coding tool.
"""The IDS worker of Bonsai Viewer: a JSON Lines process wrapping IfcTester.

Launch: ``python /path/to/ifctester_ipc.py`` — the script lives inside the
IfcOpenShell ``src/ifctester`` directory, so its parent is added to
``sys.path`` and the ``ifctester`` package imports directly.

Protocol (BonsaiViewer06 Filter Rule): one JSON message per line on stdin
(requests) and stdout (replies), UTF-8. A reply repeats the request ``id``.

Requests:
    hello    -> the engine's identity and versions
    select   -> {ifc, ids_xml, limit_ids?} — the EXPRESS ids of the
                entities that pass every facet of the first specification.
                ``limit_ids`` (EXPRESS ids) restricts the candidates, which
                is how "Run on the box" scopes the evaluation.
    build    -> {name, entity{...}, property{...}?} — a canonical IDS XML
                document built with IfcTester itself (Ids.to_string), so
                every envelope stores buildingSMART-standard XML.
    parse    -> {ids_xml} — the facet fields of the first specification,
                for reopening a filter in the creation window.
    shutdown -> exit

Only the worker's own JSON ever reaches stdout; library output goes to
stderr.
"""

import json
import os
import sys
import traceback

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import ifcopenshell  # noqa: E402
from ifctester import facet, ids  # noqa: E402

ENGINE_NAME = "ifctester"
ENGINE_VERSION = "1.0"


def reply(req_id, **payload):
    out = {"id": req_id, "ok": True}
    out.update(payload)
    return out


def error(req_id, message):
    return {"id": req_id, "ok": False, "error": message}


def op_hello(req_id, req):
    return reply(
        req_id,
        engine=ENGINE_NAME,
        engine_version=ENGINE_VERSION,
        ifcopenshell_version=ifcopenshell.version,
        ifctester_path=os.path.dirname(os.path.abspath(facet.__file__)),
    )


def op_select(req_id, req):
    ifc_path = req.get("ifc")
    ids_xml = req.get("ids_xml")
    if not ifc_path or not ids_xml:
        return error(req_id, "select needs 'ifc' and 'ids_xml'")

    limit_ids = req.get("limit_ids") or None
    limit = set(int(i) for i in limit_ids) if limit_ids else None

    ifc_file = ifcopenshell.open(ifc_path)
    doc = ids.from_string(ids_xml)
    if not doc.specifications:
        return error(req_id, "the IDS document has no specification")
    spec = doc.specifications[0]

    # Broadphase applicability, exactly like Specification.validate: each
    # applicability facet narrows the candidate list, the first one giving
    # the class-level candidates.
    elements = None
    for ifc_facet in spec.applicability:
        elements = ifc_facet.filter(ifc_file, elements)

    if elements is None:
        if limit is not None:
            elements = [ifc_file.by_id(i) for i in sorted(limit) if ifc_file.by_id(i)]
        else:
            # No applicability facet: every product is a candidate.
            elements = ifc_file.by_type("IfcProduct")
    elif limit is not None:
        elements = [e for e in elements if e.id() in limit]

    # A selection is stricter than an audit: the entity must pass every
    # facet (applicability re-test included) to be selected.
    selected = []
    for element in elements:
        passed = True
        for ifc_facet in spec.applicability:
            if isinstance(ifc_facet, facet.Entity):
                continue
            if not bool(ifc_facet(element)):
                passed = False
                break
        if not passed:
            continue
        for ifc_facet in spec.requirements:
            if not bool(ifc_facet(element)):
                passed = False
                break
        if passed:
            selected.append(element.id())

    return reply(req_id, ids=sorted(set(selected)), count=len(selected))


def op_build(req_id, req):
    entity = req.get("entity") or {}
    name = (req.get("name") or "Selection").strip() or "Selection"
    ifc_class = (entity.get("name") or "IFCWALL").strip().upper() or "IFCWALL"
    predefined_type = (entity.get("predefinedType") or "").strip() or None

    spec = ids.Specification(name=name)
    spec.applicability = [facet.Entity(name=ifc_class, predefinedType=predefined_type)]

    prop = req.get("property") or {}
    base_name = (prop.get("baseName") or "").strip()
    pset = (prop.get("propertySet") or "").strip()
    if base_name or pset:
        value = (prop.get("value") or "").strip() or None
        spec.requirements = [
            facet.Property(
                propertySet=pset or "Property_Set",
                baseName=base_name or "PropertyName",
                value=value,
                cardinality=prop.get("cardinality") or "required",
            )
        ]
    else:
        spec.requirements = []

    doc = ids.Ids(title=name)
    doc.specifications = [spec]
    return reply(req_id, ids_xml=doc.to_string())


def op_parse(req_id, req):
    ids_xml = req.get("ids_xml")
    if not ids_xml:
        return error(req_id, "parse needs 'ids_xml'")
    doc = ids.from_string(ids_xml)
    if not doc.specifications:
        return error(req_id, "the IDS document has no specification")
    spec = doc.specifications[0]

    entity = {"name": "", "predefinedType": ""}
    property_fields = None
    for ifc_facet in spec.applicability + spec.requirements:
        if isinstance(ifc_facet, facet.Entity):
            entity = {
                "name": ifc_facet.name or "",
                "predefinedType": ifc_facet.predefinedType or "",
            }
        elif isinstance(ifc_facet, facet.Property):
            value = ifc_facet.value
            if isinstance(value, facet.Restriction):
                # Simple cases our builder produces: one value or one
                # enumeration entry; anything richer loses the value.
                options = value.options or {}
                if "enumeration" in options:
                    values = options["enumeration"]
                    value = values[0] if values else ""
                else:
                    value = next(iter(options.values()), "")
            property_fields = {
                "propertySet": ifc_facet.propertySet or "",
                "baseName": ifc_facet.baseName or "",
                "value": value if isinstance(value, str) else (value or ""),
                "cardinality": ifc_facet.cardinality or "required",
            }

    return reply(req_id, name=spec.name or "", entity=entity, property=property_fields)


def main():
    handlers = {
        "hello": op_hello,
        "select": op_select,
        "build": op_build,
        "parse": op_parse,
        "shutdown": None,
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
        except Exception:  # noqa: BLE001 — the worker must survive bad input
            sys.stderr.write(traceback.format_exc())
            print(json.dumps(error(req_id, "the IDS engine failed; see stderr")), flush=True)


if __name__ == "__main__":
    main()
