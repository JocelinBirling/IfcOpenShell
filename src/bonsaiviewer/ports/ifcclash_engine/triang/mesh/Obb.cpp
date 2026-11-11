#include "Obb.h"

#include <TColgp_Array1OfPnt.hxx>
#include <gp_Pnt.hxx>

namespace ifcclash::triang {

Bnd_OBB optimal_obb(const std::vector<double>& xyz) {
    Bnd_OBB obb;
    const int n = static_cast<int>(xyz.size() / 3);
    if (n == 0) return obb;
    // BonsaiViewer: a 0-based array. The optimal ReBuild of OpenCASCADE 7.6
    // (the system OCCT of the viewer) raises Standard_OutOfRange on a
    // 1-based one; the box does not depend on the indexing.
    TColgp_Array1OfPnt points(0, n - 1);
    for (int i = 0; i < n; ++i) points.SetValue(i, gp_Pnt(xyz[3 * i], xyz[3 * i + 1], xyz[3 * i + 2]));
    obb.ReBuild(points, nullptr, true);
    return obb;
}

}  // namespace ifcclash::triang
