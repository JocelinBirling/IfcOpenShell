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

#ifndef IFCINTERFACE_MODULES_MEASUREMENT_PANEL_H
#define IFCINTERFACE_MODULES_MEASUREMENT_PANEL_H

#include "../../components/Panel.h"
#include "Measurements.h"

#include "../../../ifcviewer/OverlayRenderer.h"

#include <QMap>
#include <QTreeWidget>

#include <vector>

class QLabel;
class QPoint;
class QToolButton;
class QPushButton;

namespace bonsaiviewer { class SessionState; }
namespace bonsaiviewer::services {
class ActiveView;
class SelectionService;
}
class ViewportWindow;

namespace bonsaiviewer::modules::measurement {

// BonsaiViewer50 — Mesure Plus panel (M1-M5). A dock panel with one button
// per measurement function, a Reset button, and a toggle to show/hide the
// measured geometry in the viewport. Each measurement lands in a tree:
// measures can be gathered into (nested) groups, every item has a checkbox
// to show/hide its overlay, and each group shows the sum of its measures
// per unit. The traits are drawn in the viewport as x-ray overlay lines.
class MeasurementPanel : public components::Panel {
    Q_OBJECT

public:
    MeasurementPanel(bonsaiviewer::services::SelectionService* selection,
                     bonsaiviewer::SessionState* session,
                     ViewportWindow* viewport,
                     QWidget* parent = nullptr);

    // The SelectionService is created after the panels; wire it in once ready.
    void setSelectionService(bonsaiviewer::services::SelectionService* selection);
    // Same for the ActiveView (isolate a measure's objects).
    void setActiveView(bonsaiviewer::services::ActiveView* active_view);

    // Clear all measurement overlays from the viewport and reset the result label.
    void resetMeasurements();

    // Show or hide all measurement geometry overlays.
    void setMeasurementGeometryVisible(bool visible);

protected:
    bool eventFilter(QObject* obj, QEvent* ev) override;

signals:
    void measurementPerformed(const QString& label, double value, const QString& unit);

private:
    void runM0();  // aire face supérieure (triangulation)
    void runM1();  // compter
    void runM2();  // longueur_axe
    void runM3();  // perimetre
    void runM4();  // longueur_arete
    void runM5();  // longueur_main_courante

    void updateResultLabel(const QString& text);
    void clearViewportOverlays();

    // One executed measurement: a leaf in the tree, with its overlay data.
    struct MeasureRecord {
        QString label;
        double value = 0.0;
        QString unit;
        int mode = 0;  // 0 = longest AABB edge (M2/M4), 1 = base rectangle (M3),
                       // 2 = longest horizontal top edge (M5), 3 = top-face
                       // highlight (M0), -1 = count only (M1)
        std::vector<MeasuredObject> objects;
        std::vector<float> highlight_tris;  // mode 3: 9 floats per triangle, world
        float label_pos[3] = {0, 0, 0};     // mode 3: label anchor
        QTreeWidgetItem* item = nullptr;
    };

    int addMeasureRecord(const QString& label, double value, const QString& unit,
                         int mode, const std::vector<MeasuredObject>& objects);
    void onNewGroup();
    void onTreeContextMenu(const QPoint& pos);
    void deleteTreeItem(QTreeWidgetItem* item);
    void zoomOnRecord(int id);
    void isolateRecord(int id);
    void onItemChanged(QTreeWidgetItem* item, int column);
    void refreshGroupSums();
    void refreshOverlays();
    bool isItemVisible(const QTreeWidgetItem* item) const;
    void collectSums(QTreeWidgetItem* item, QMap<QString, double>& sums) const;
    void drawRecord(OverlayRenderer::LineGroup& group,
                    std::vector<OverlayRenderer::Label>& labels,
                    std::vector<float>& highlights,
                    const MeasureRecord& rec);

    bonsaiviewer::services::SelectionService* selection_ = nullptr;
    bonsaiviewer::services::ActiveView* active_view_ = nullptr;
    bonsaiviewer::SessionState* session_ = nullptr;
    ViewportWindow* viewport_ = nullptr;

    QPushButton* m0_button_ = nullptr;
    QPushButton* m1_button_ = nullptr;
    QPushButton* m2_button_ = nullptr;
    QPushButton* m3_button_ = nullptr;
    QPushButton* m4_button_ = nullptr;
    QPushButton* m5_button_ = nullptr;
    QPushButton* reset_button_ = nullptr;
    QPushButton* new_group_button_ = nullptr;
    QToolButton* geometry_toggle_ = nullptr;
    QTreeWidget* measures_tree_ = nullptr;
    QLabel* result_label_ = nullptr;

    bool geometry_visible_ = true;
    std::vector<MeasureRecord> records_;
};

} // namespace bonsaiviewer::modules::measurement

#endif // IFCINTERFACE_MODULES_MEASUREMENT_PANEL_H
