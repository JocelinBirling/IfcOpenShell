// This file was generated with the assistance of an AI coding tool.
// The trees read from the IFC relations (target bonsaiviewer_ifc_trees_tests),
// on a small IFC2X3 file written by the test.
#include "modules/tree_explorer/IfcTrees.h"

#include "../ifcparse/file.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include <iostream>

using namespace bonsaiviewer::modules::tree_explorer;
using bonsaiviewer::services::tree::TreeIndex;

namespace {

int failures = 0, checks = 0;

void check(bool ok, const QString& what) {
    ++checks;
    if (!ok) {
        ++failures;
        std::cerr << "FAILED: " << what.toStdString() << "\n";
    }
}

const char* kModel = R"(ISO-10303-21;
HEADER;
FILE_DESCRIPTION(('ViewDefinition [CoordinationView]'),'2;1');
FILE_NAME('trees.ifc','2026-11-12T00:00:00',(''),(''),'','','');
FILE_SCHEMA(('IFC2X3'));
ENDSEC;
DATA;
#1=IFCPROJECT('0YvctVUKr0kugbFTf53O9L',$,'P',$,$,$,$,$,$);
#2=IFCSITE('1YvctVUKr0kugbFTf53O9L',$,'Site',$,$,$,$,$,.ELEMENT.,$,$,$,$,$);
#3=IFCBUILDING('2YvctVUKr0kugbFTf53O9L',$,'B',$,$,$,$,$,.ELEMENT.,$,$,$);
#4=IFCBUILDINGSTOREY('3YvctVUKr0kugbFTf53O9L',$,'Level 1',$,$,$,$,$,.ELEMENT.,0.);
#5=IFCRELAGGREGATES('4YvctVUKr0kugbFTf53O9L',$,$,$,#1,(#2));
#6=IFCRELAGGREGATES('5YvctVUKr0kugbFTf53O9L',$,$,$,#2,(#3));
#7=IFCRELAGGREGATES('6YvctVUKr0kugbFTf53O9L',$,$,$,#3,(#4));
#10=IFCWALL('7YvctVUKr0kugbFTf53O9L',$,'Wall A',$,$,$,$,$);
#11=IFCWALL('8YvctVUKr0kugbFTf53O9L',$,'Wall B',$,$,$,$,$);
#12=IFCFLOWSEGMENT('9YvctVUKr0kugbFTf53O9L',$,'Pipe',$,$,$,$,$);
#13=IFCRELCONTAINEDINSPATIALSTRUCTURE('AYvctVUKr0kugbFTf53O9L',$,$,$,(#10,#11,#12),#4);
#20=IFCWALLTYPE('BYvctVUKr0kugbFTf53O9L',$,'WT 200',$,$,$,$,$,$,.STANDARD.);
#21=IFCRELDEFINESBYTYPE('CYvctVUKr0kugbFTf53O9L',$,$,$,(#10,#11),#20);
#30=IFCSYSTEM('DYvctVUKr0kugbFTf53O9L',$,'Heating',$,$);
#31=IFCSYSTEM('EYvctVUKr0kugbFTf53O9L',$,'Loop',$,$);
#32=IFCRELASSIGNSTOGROUP('FYvctVUKr0kugbFTf53O9L',$,$,$,(#31,#12),$,#30);
#33=IFCRELASSIGNSTOGROUP('GYvctVUKr0kugbFTf53O9L',$,$,$,(#12),$,#31);
#34=IFCZONE('HYvctVUKr0kugbFTf53O9L',$,'Zone',$,$);
#35=IFCGROUP('IYvctVUKr0kugbFTf53O9L',$,'Group',$,$);
#36=IFCRELASSIGNSTOGROUP('JYvctVUKr0kugbFTf53O9L',$,$,$,(#10),$,#35);
#40=IFCMATERIAL('Concrete');
#41=IFCMATERIAL('Insulation');
#42=IFCMATERIALLAYER(#40,0.2,$);
#43=IFCMATERIALLAYER(#41,0.1,$);
#44=IFCMATERIALLAYERSET((#42,#43),'Wall 300');
#45=IFCMATERIALLAYERSETUSAGE(#44,.AXIS2.,.POSITIVE.,0.);
#46=IFCRELASSOCIATESMATERIAL('KYvctVUKr0kugbFTf53O9L',$,$,$,(#11),#45);
#47=IFCRELASSOCIATESMATERIAL('LYvctVUKr0kugbFTf53O9L',$,$,$,(#20),#40);
#48=IFCRELASSOCIATESMATERIAL('MYvctVUKr0kugbFTf53O9L',$,$,$,(#12),#40);
ENDSEC;
END-ISO-10303-21;
)";

// The labels of the children of a node (-1: the roots), as shown.
QStringList labels(const TreeIndex& index, int node) {
    QStringList result;
    for (int child : node < 0 ? index.roots() : index.node(node).children) result << index.displayLabel(child);
    return result;
}

int child(const TreeIndex& index, int node, const QString& label) {
    for (int c : node < 0 ? index.roots() : index.node(node).children) {
        if (index.node(c).label == label) return c;
    }
    return -1;
}

QString joined(const QStringList& list) {
    return list.join(QStringLiteral(" | "));
}

} // namespace

int main() {
    QTemporaryDir dir;
    const QString path = QDir(dir.path()).filePath("trees.ifc");
    {
        QFile out(path);
        out.open(QIODevice::WriteOnly);
        out.write(kModel);
    }
    ifcopenshell::file file(path.toStdString());
    if (!file.good()) {
        std::cerr << "FAILED: the test model cannot be read\n";
        return 1;
    }
    ifc_trees::ModelInput input;
    input.model = 1;
    input.file = &file;
    input.object_by_ifc = {{10, 100}, {11, 101}, {12, 102}};

    {
        TreeIndex index;
        ifc_trees::buildSpatial(index, input, -1);
        index.finish();
        const int site = child(index, -1, "Site");
        const int storey = child(index, child(index, site, "B"), "Level 1");
        check(site >= 0 && storey >= 0, "Spatial: Site > Building > Storey");
        check(joined(labels(index, storey)) == "Pipe | Wall A | Wall B", "Spatial: the contained elements, sorted");
        check(index.displayLabel(site) == "Site (3)", "Spatial: the site counts the elements below");
        check(index.node(site).expanded && !index.node(storey).expanded, "Spatial: open down to the storeys");
        check(index.node(child(index, storey, "Wall A")).object_id == 100, "Spatial: elements carry their object id");
    }
    {
        TreeIndex index;
        ifc_trees::buildTypes(index, input, -1);
        index.finish();
        const int type = child(index, child(index, -1, "IfcWallType"), "WT 200");
        check(type >= 0, "Types: class > type");
        check(joined(labels(index, type)) == "Wall A | Wall B", "Types: the occurrences");
        check(index.node(type).ifc_id == 20, "Types: the type node is its entity (Show properties)");
    }
    {
        TreeIndex index;
        ifc_trees::buildGroups(index, input, -1, ifc_trees::GroupKind::Systems);
        index.finish();
        check(joined(labels(index, -1)) == "Heating (1)", "Systems: the sub-system is not a root, the zone is apart");
        const int heating = child(index, -1, "Heating");
        check(joined(labels(index, heating)) == QString::fromUtf8("Loop (1) | Pipe ×2"),
              "Systems: sub-system and member, the pipe listed twice");
    }
    {
        TreeIndex zones;
        ifc_trees::buildGroups(zones, input, -1, ifc_trees::GroupKind::Zones);
        zones.finish();
        check(joined(labels(zones, -1)) == "Zone (0)", "Zones: IfcZone only");
        TreeIndex groups;
        ifc_trees::buildGroups(groups, input, -1, ifc_trees::GroupKind::Groups);
        groups.finish();
        check(joined(labels(groups, -1)) == "Group (1)", "Groups: the other IfcGroup");
    }
    {
        TreeIndex index;
        ifc_trees::buildMaterials(index, input, -1);
        index.finish();
        check(joined(labels(index, -1)) == "Concrete (2) | Wall 300 (1)", "Materials: the material and the set");
        const int set = child(index, -1, "Wall 300");
        check(joined(labels(index, set)) == "Concrete (0.2) (1) | Insulation (0.1) (1)", "Materials: the layers");
        check(joined(labels(index, child(index, set, "Insulation (0.1)"))) == QString::fromUtf8("Wall B ×2"),
              "Materials: an element under each of its layers");
        check(joined(labels(index, child(index, -1, "Concrete"))) == "Pipe | Wall A",
              "Materials: the type's material for an occurrence without its own");
    }

    std::cout << checks - failures << "/" << checks << " checks passed\n";
    return failures == 0 ? 0 : 1;
}
