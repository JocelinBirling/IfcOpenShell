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

#include "Panel.h"
#include "Measurements.h"
#include "IfcMeasurements.h"

#include "../../../ifcviewer/ViewportWindow.h"
#include "../../ElementRegistry.h"
#include "../../services/ActiveView.h"
#include "../../services/SelectionService.h"
#include "../../SessionState.h"
#include "../../ViewerSettings.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QGroupBox>
#include <QFrame>

#include <QMap>
#include <QMenu>
#include <QStringList>

#include <algorithm>
#include <cmath>
#include <functional>
#include <vector>
#include <cstdint>

namespace bonsaiviewer::modules::measurement {

namespace {
// QTreeWidgetItem user roles.
constexpr int kRoleRecordId = Qt::UserRole;       // int, -1 for groups
constexpr int kRoleIsGroup  = Qt::UserRole + 1;    // bool
} // namespace

MeasurementPanel::MeasurementPanel(bonsaiviewer::services::SelectionService* selection,
                                   bonsaiviewer::SessionState* session,
                                   ViewportWindow* viewport,
                                   QWidget* parent)
    : components::Panel(QStringLiteral("Mesure"), nullptr, parent, false, true),
      selection_(selection), session_(session), viewport_(viewport) {

    auto* body = new QWidget(this);
    auto* layout = new QVBoxLayout(body);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    // --- Section: fonctions de mesure M1-M5 ---
    auto* functions_box = new QGroupBox(QStringLiteral("Fonctions de mesure"), body);
    auto* functions_layout = new QVBoxLayout(functions_box);
    functions_layout->setSpacing(4);

    m0_button_ = new QPushButton(QStringLiteral("M0 — Aire face supérieure"), functions_box);
    m0_button_->setToolTip(QStringLiteral("Aire (m²) de la plus grande face horizontale en haut de l'objet, détectée par la triangulation"));
    m1_button_ = new QPushButton(QStringLiteral("M1 — Compter"), functions_box);
    m1_button_->setToolTip(QStringLiteral("Compte les éléments sélectionnés (U / Nb)"));
    m2_button_ = new QPushButton(QStringLiteral("M2 — Longueur d'axe"), functions_box);
    m2_button_->setToolTip(QStringLiteral("Longueur suivant l'axe de l'élément (ml). Courbes en développé réel."));
    m3_button_ = new QPushButton(QStringLiteral("M3 — Périmètre"), functions_box);
    m3_button_->setToolTip(QStringLiteral("Longueur de base de l'objet (ml)"));
    m4_button_ = new QPushButton(QStringLiteral("M4 — Longueur d'arête"), functions_box);
    m4_button_->setToolTip(QStringLiteral("Longueur d'une arête extraite automatiquement (ml)"));
    m5_button_ = new QPushButton(QStringLiteral("M5 — Longueur main courante"), functions_box);
    m5_button_->setToolTip(QStringLiteral("Longueur développée de la surface haute coplanaire la plus longue (ml)"));
    m6_button_ = new QPushButton(QStringLiteral("M6 — Surface top"), functions_box);
    m6_button_->setToolTip(QStringLiteral("Aire (m²) de la face horizontale supérieure de l'élément"));
    m7_button_ = new QPushButton(QStringLiteral("M7 — Surface bottom"), functions_box);
    m7_button_->setToolTip(QStringLiteral("Aire (m²) de la face horizontale inférieure de l'élément"));
    m8_button_ = new QPushButton(QStringLiteral("M8 — Surface latérale"), functions_box);
    m8_button_->setToolTip(QStringLiteral("Aire (m²) des faces latérales (sans top ni bottom)"));
    m8_mode_combo_ = new QComboBox(functions_box);
    m8_mode_combo_->addItem(QStringLiteral("Toutes les faces latérales"), 0);
    m8_mode_combo_->addItem(QStringLiteral("Plus grande face coplanaire"), 1);
    m8_mode_combo_->addItem(QStringLiteral("Deux plus grandes faces"), 2);
    m8_mode_combo_->setToolTip(QStringLiteral("Mode de calcul de la surface latérale (M8)"));
    m11_button_ = new QPushButton(QStringLiteral("M11 — Surface toiture développée"), functions_box);
    m11_button_->setToolTip(QStringLiteral("Aire réelle en rampant (m²) des surfaces inclinées, pas la projection"));
    m12_button_ = new QPushButton(QStringLiteral("M12 — Surface coffrage"), functions_box);
    m12_button_->setToolTip(QStringLiteral("Aire (m²) de coffrage : joues + sous-face (abouts et top exclus par défaut)"));
    m12_abouts_check_ = new QCheckBox(QStringLiteral("Inclure abouts"), functions_box);
    m12_top_check_ = new QCheckBox(QStringLiteral("Inclure face supérieure"), functions_box);
    m13_button_ = new QPushButton(QStringLiteral("M13 — Surface section projetée"), functions_box);
    m13_button_->setToolTip(QStringLiteral("Aire (m²) = périmètre de section × longueur (conduits rectangulaires)"));
    m17_button_ = new QPushButton(QStringLiteral("M17 — Volume élément"), functions_box);
    m17_button_->setToolTip(QStringLiteral("Volume (m³) du BREP fermé, somme des volumes signés des tétraèdres"));
    m18_button_ = new QPushButton(QStringLiteral("M18 — Volume de couche"), functions_box);
    m18_button_->setToolTip(QStringLiteral("Volume (m³) = surface top × épaisseur de couche"));
    m18_epaisseur_spin_ = new QDoubleSpinBox(functions_box);
    m18_epaisseur_spin_->setRange(0.001, 100.0);
    m18_epaisseur_spin_->setSingleStep(0.01);
    m18_epaisseur_spin_->setValue(0.10);
    m18_epaisseur_spin_->setSuffix(QStringLiteral(" m"));
    m18_epaisseur_spin_->setToolTip(QStringLiteral("Épaisseur de la couche (m) pour M18"));
    m9_button_ = new QPushButton(QStringLiteral("M9 — Surface vitrage"), functions_box);
    m9_button_->setToolTip(QStringLiteral("Aire (m²) des surfaces transparentes détectées via le matériau IFC"));
    m16_button_ = new QPushButton(QStringLiteral("M16 — Surface ouverture"), functions_box);
    m16_button_->setToolTip(QStringLiteral("Aire (m²) des ouvertures liées (IfcOpeningElement via IfcRelVoidsElement)"));
    m20_button_ = new QPushButton(QStringLiteral("M20 — Volume entre 2 surfaces"), functions_box);
    m20_button_->setToolTip(QStringLiteral("Volume (m³) entre deux objets sélectionnés (paires consécutives)"));
    m27_button_ = new QPushButton(QStringLiteral("M27 — Pente"), functions_box);
    m27_button_->setToolTip(QStringLiteral("Pente (%) de la face dominante, ajustement de plan via la normale moyenne"));
    m21_button_ = new QPushButton(QStringLiteral("M21 — Local (surf/périm/hauteur/vol)"), functions_box);
    m21_button_->setToolTip(QStringLiteral("Métriques d'un IfcSpace : surface sol, périmètre, hauteur, volume"));
    m22_button_ = new QPushButton(QStringLiteral("M22 — Surface sous plafond"), functions_box);
    m22_button_->setToolTip(QStringLiteral("Surface au sol (m²) des locaux avec hauteur sous plafond >= seuil"));
    m22_seuil_spin_ = new QDoubleSpinBox(functions_box);
    m22_seuil_spin_->setRange(0.0, 20.0);
    m22_seuil_spin_->setSingleStep(0.10);
    m22_seuil_spin_->setValue(1.80);
    m22_seuil_spin_->setSuffix(QStringLiteral(" m"));
    m22_seuil_spin_->setToolTip(QStringLiteral("Hauteur minimale sous plafond (m) pour M22"));

    // Initialise the parameter widgets from the persisted Mesure settings.
    {
        auto& vs = bonsaiviewer::ViewerSettings::instance();
        m18_epaisseur_spin_->setValue(vs.mesureEpaisseurCoucheM());
        m22_seuil_spin_->setValue(vs.mesureHauteurSousPlafondM());
        const int m8 = vs.mesureM8Mode();
        if (m8 >= 0 && m8 < m8_mode_combo_->count()) m8_mode_combo_->setCurrentIndex(m8);
        m12_abouts_check_->setChecked(vs.mesureM12InclureAbouts());
        m12_top_check_->setChecked(vs.mesureM12InclureTop());
    }

    functions_layout->addWidget(m0_button_);
    functions_layout->addWidget(m1_button_);
    functions_layout->addWidget(m2_button_);
    functions_layout->addWidget(m3_button_);
    functions_layout->addWidget(m4_button_);
    functions_layout->addWidget(m5_button_);
    functions_layout->addWidget(m6_button_);
    functions_layout->addWidget(m7_button_);
    functions_layout->addWidget(m8_button_);
    functions_layout->addWidget(m8_mode_combo_);
    functions_layout->addWidget(m11_button_);
    functions_layout->addWidget(m12_button_);
    functions_layout->addWidget(m12_abouts_check_);
    functions_layout->addWidget(m12_top_check_);
    functions_layout->addWidget(m13_button_);
    functions_layout->addWidget(m17_button_);
    functions_layout->addWidget(m18_button_);
    functions_layout->addWidget(m18_epaisseur_spin_);
    functions_layout->addWidget(m9_button_);
    functions_layout->addWidget(m16_button_);
    functions_layout->addWidget(m20_button_);
    functions_layout->addWidget(m27_button_);
    functions_layout->addWidget(m21_button_);
    functions_layout->addWidget(m22_button_);
    functions_layout->addWidget(m22_seuil_spin_);

    layout->addWidget(functions_box);

    // --- Section: affichage ---
    auto* display_box = new QGroupBox(QStringLiteral("Affichage"), body);
    auto* display_layout = new QHBoxLayout(display_box);
    display_layout->setSpacing(4);

    geometry_toggle_ = new QToolButton(display_box);
    geometry_toggle_->setText(QStringLiteral("Géométries"));
    geometry_toggle_->setCheckable(true);
    geometry_toggle_->setChecked(true);
    geometry_toggle_->setToolTip(QStringLiteral("Afficher / masquer les géométries de mesure dans le viewer"));

    new_group_button_ = new QPushButton(QStringLiteral("Nouveau groupe"), display_box);
    new_group_button_->setToolTip(QStringLiteral("Créer un groupe dans l'arbre (sous la sélection courante)"));

    reset_button_ = new QPushButton(QStringLiteral("Reset"), display_box);
    reset_button_->setToolTip(QStringLiteral("Effacer toutes les mesures affichées"));

    settings_button_ = new QPushButton(QStringLiteral("Réglages"), display_box);
    settings_button_->setToolTip(QStringLiteral("Ouvrir les réglages Mesure dans les paramètres"));

    display_layout->addWidget(geometry_toggle_);
    display_layout->addWidget(new_group_button_);
    display_layout->addWidget(settings_button_);
    display_layout->addWidget(reset_button_);
    display_layout->addStretch();

    layout->addWidget(display_box);

    // --- Arbre des mesures ---
    measures_tree_ = new QTreeWidget(body);
    measures_tree_->setColumnCount(2);
    measures_tree_->setHeaderLabels({QStringLiteral("Mesure"), QStringLiteral("Valeur")});
    measures_tree_->setSelectionMode(QAbstractItemView::SingleSelection);
    measures_tree_->setDragDropMode(QAbstractItemView::InternalMove);
    measures_tree_->setExpandsOnDoubleClick(true);
    measures_tree_->setMinimumHeight(140);
    measures_tree_->header()->setStretchLastSection(false);
    measures_tree_->setContextMenuPolicy(Qt::CustomContextMenu);
    measures_tree_->installEventFilter(this);
    layout->addWidget(measures_tree_, 1);

    // --- Résultat ---
    auto* result_frame = new QFrame(body);
    result_frame->setFrameShape(QFrame::StyledPanel);
    auto* result_layout = new QVBoxLayout(result_frame);
    result_layout->setContentsMargins(6, 6, 6, 6);
    result_label_ = new QLabel(QStringLiteral("Aucune mesure"), result_frame);
    result_label_->setWordWrap(true);
    result_label_->setStyleSheet(QStringLiteral("font-weight: bold; color: #555;"));
    result_layout->addWidget(result_label_);
    layout->addWidget(result_frame);

    layout->addStretch();
    addBodyWidget(body);

    // --- Connexions ---
    connect(m0_button_, &QPushButton::clicked, this, &MeasurementPanel::runM0);
    connect(m1_button_, &QPushButton::clicked, this, &MeasurementPanel::runM1);
    connect(m2_button_, &QPushButton::clicked, this, &MeasurementPanel::runM2);
    connect(m3_button_, &QPushButton::clicked, this, &MeasurementPanel::runM3);
    connect(m4_button_, &QPushButton::clicked, this, &MeasurementPanel::runM4);
    connect(m5_button_, &QPushButton::clicked, this, &MeasurementPanel::runM5);
    connect(m6_button_, &QPushButton::clicked, this, &MeasurementPanel::runM6);
    connect(m7_button_, &QPushButton::clicked, this, &MeasurementPanel::runM7);
    connect(m8_button_, &QPushButton::clicked, this, &MeasurementPanel::runM8);
    connect(m11_button_, &QPushButton::clicked, this, &MeasurementPanel::runM11);
    connect(m12_button_, &QPushButton::clicked, this, &MeasurementPanel::runM12);
    connect(m13_button_, &QPushButton::clicked, this, &MeasurementPanel::runM13);
    connect(m17_button_, &QPushButton::clicked, this, &MeasurementPanel::runM17);
    connect(m18_button_, &QPushButton::clicked, this, &MeasurementPanel::runM18);
    connect(m9_button_, &QPushButton::clicked, this, &MeasurementPanel::runM9);
    connect(m16_button_, &QPushButton::clicked, this, &MeasurementPanel::runM16);
    connect(m20_button_, &QPushButton::clicked, this, &MeasurementPanel::runM20);
    connect(m27_button_, &QPushButton::clicked, this, &MeasurementPanel::runM27);
    connect(m21_button_, &QPushButton::clicked, this, &MeasurementPanel::runM21);
    connect(m22_button_, &QPushButton::clicked, this, &MeasurementPanel::runM22);
    connect(reset_button_, &QPushButton::clicked, this, &MeasurementPanel::resetMeasurements);
    connect(settings_button_, &QPushButton::clicked, this, &MeasurementPanel::openSettingsRequested);
    // Persist the Mesure parameters as soon as the user edits them.
    connect(m18_epaisseur_spin_, &QDoubleSpinBox::valueChanged, this, [](double v) {
        bonsaiviewer::ViewerSettings::instance().setMesureEpaisseurCoucheM(v);
    });
    connect(m22_seuil_spin_, &QDoubleSpinBox::valueChanged, this, [](double v) {
        bonsaiviewer::ViewerSettings::instance().setMesureHauteurSousPlafondM(v);
    });
    connect(m8_mode_combo_, &QComboBox::currentIndexChanged, this, [](int idx) {
        bonsaiviewer::ViewerSettings::instance().setMesureM8Mode(idx);
    });
    connect(m12_abouts_check_, &QCheckBox::toggled, this, [](bool v) {
        bonsaiviewer::ViewerSettings::instance().setMesureM12InclureAbouts(v);
    });
    connect(m12_top_check_, &QCheckBox::toggled, this, [](bool v) {
        bonsaiviewer::ViewerSettings::instance().setMesureM12InclureTop(v);
    });
    connect(new_group_button_, &QPushButton::clicked, this, &MeasurementPanel::onNewGroup);
    connect(geometry_toggle_, &QToolButton::toggled, this, &MeasurementPanel::setMeasurementGeometryVisible);
    connect(measures_tree_, &QTreeWidget::itemChanged, this, &MeasurementPanel::onItemChanged);
    connect(measures_tree_, &QTreeWidget::customContextMenuRequested,
            this, &MeasurementPanel::onTreeContextMenu);
    // Any structural change (add / remove / move a measure or a group) must
    // re-actualise the per-unit sums of every group.
    auto onStructureChanged = [this]() { refreshGroupSums(); refreshOverlays(); };
    connect(measures_tree_->model(), &QAbstractItemModel::rowsMoved, this, onStructureChanged);
    connect(measures_tree_->model(), &QAbstractItemModel::rowsInserted, this, onStructureChanged);
    connect(measures_tree_->model(), &QAbstractItemModel::rowsRemoved, this, onStructureChanged);
}

bool MeasurementPanel::eventFilter(QObject* obj, QEvent* ev) {
    if (obj == measures_tree_ && ev->type() == QEvent::Resize) {
        // Fixed 70/30 split between the "Mesure" and "Valeur" columns.
        const int w = measures_tree_->viewport()->width();
        if (w > 0) {
            measures_tree_->setColumnWidth(0, w * 7 / 10);
            measures_tree_->setColumnWidth(1, w * 3 / 10);
        }
    }
    return components::Panel::eventFilter(obj, ev);
}

void MeasurementPanel::setSelectionService(bonsaiviewer::services::SelectionService* selection) {
    selection_ = selection;
}

void MeasurementPanel::setActiveView(bonsaiviewer::services::ActiveView* active_view) {
    active_view_ = active_view;
}

void MeasurementPanel::runM0() {
    if (!selection_ || !viewport_) return;
    const auto ids = selection_->selectionIdsAsVector();
    if (ids.empty()) {
        updateResultLabel(QStringLiteral("M0 — Aucun élément sélectionné"));
        return;
    }
    double total = 0.0;
    int measured = 0;
    for (const std::uint32_t id : ids) {
        const TopFace face = measureTopFace(*viewport_, id);
        if (face.area <= 0.0) continue;
        // World AABB of the object, for zoom / isolate.
        Eigen::Vector3f emn, emx;
        if (!viewport_->computeObjectAabb(id, emn, emx)) continue;
        MeasuredObject obj;
        obj.id = id;
        for (int i = 0; i < 3; ++i) { obj.mn[i] = emn[i]; obj.mx[i] = emx[i]; }
        const int rec = addMeasureRecord(QStringLiteral("M0 — Aire face sup"),
                                         face.area, QStringLiteral("m²"), 3, {obj});
        records_[rec].highlight_tris = face.triangles;
        for (int i = 0; i < 3; ++i) records_[rec].label_pos[i] = face.centroid[i];
        total += face.area;
        ++measured;
    }
    refreshGroupSums();
    refreshOverlays();
    const QString text = QStringLiteral("M0 — Aire face supérieure : %1 m² (%2 objet(s))")
                             .arg(total, 0, 'f', 3).arg(measured);
    updateResultLabel(text);
    if (session_) session_->setStatusMessage(QStringLiteral("Mesure"), text);
    emit measurementPerformed(QStringLiteral("M0 Aire face sup"), total, QStringLiteral("m²"));
}

void MeasurementPanel::runM1() {
    if (!selection_ || !session_) return;
    const auto ids = selection_->selectionIdsAsVector();
    if (ids.empty()) {
        updateResultLabel(QStringLiteral("M1 — Aucun élément sélectionné"));
        return;
    }
    const double count = static_cast<double>(ids.size());
    const QString text = QStringLiteral("M1 — Compter : %1 élément(s)").arg(ids.size());
    updateResultLabel(text);
    addMeasureRecord(QStringLiteral("M1 — Compter"), count, QStringLiteral("U"), -1, {});
    if (session_) session_->setStatusMessage(QStringLiteral("Mesure"), text);
    emit measurementPerformed(QStringLiteral("M1 Compter"), count, QStringLiteral("U"));
}

void MeasurementPanel::runM2() {
    if (!selection_ || !viewport_) return;
    const auto ids = selection_->selectionIdsAsVector();
    if (ids.empty()) {
        updateResultLabel(QStringLiteral("M2 — Aucun élément sélectionné"));
        return;
    }
    std::vector<MeasuredObject> objects;
    const double total = measureLongueurAxe(*viewport_, ids, &objects);
    const QString text = QStringLiteral("M2 — Longueur d'axe : %1 ml").arg(total, 0, 'f', 3);
    updateResultLabel(text);
    addMeasureRecord(QStringLiteral("M2 — Longueur d'axe"), total, QStringLiteral("ml"), 0, objects);
    if (session_) session_->setStatusMessage(QStringLiteral("Mesure"), text);
    emit measurementPerformed(QStringLiteral("M2 Longueur d'axe"), total, QStringLiteral("ml"));
}

void MeasurementPanel::runM3() {
    if (!selection_ || !viewport_) return;
    const auto ids = selection_->selectionIdsAsVector();
    if (ids.empty()) {
        updateResultLabel(QStringLiteral("M3 — Aucun élément sélectionné"));
        return;
    }
    std::vector<MeasuredObject> objects;
    const double total = measurePerimetre(*viewport_, ids, &objects);
    const QString text = QStringLiteral("M3 — Périmètre : %1 ml").arg(total, 0, 'f', 3);
    updateResultLabel(text);
    addMeasureRecord(QStringLiteral("M3 — Périmètre"), total, QStringLiteral("ml"), 1, objects);
    if (session_) session_->setStatusMessage(QStringLiteral("Mesure"), text);
    emit measurementPerformed(QStringLiteral("M3 Périmètre"), total, QStringLiteral("ml"));
}

void MeasurementPanel::runM4() {
    if (!selection_ || !viewport_) return;
    const auto ids = selection_->selectionIdsAsVector();
    if (ids.empty()) {
        updateResultLabel(QStringLiteral("M4 — Aucun élément sélectionné"));
        return;
    }
    std::vector<MeasuredObject> objects;
    const double total = measureLongueurArete(*viewport_, ids, &objects);
    const QString text = QStringLiteral("M4 — Longueur d'arête : %1 ml").arg(total, 0, 'f', 3);
    updateResultLabel(text);
    addMeasureRecord(QStringLiteral("M4 — Longueur d'arête"), total, QStringLiteral("ml"), 0, objects);
    if (session_) session_->setStatusMessage(QStringLiteral("Mesure"), text);
    emit measurementPerformed(QStringLiteral("M4 Longueur d'arête"), total, QStringLiteral("ml"));
}

void MeasurementPanel::runM5() {
    if (!selection_ || !viewport_) return;
    const auto ids = selection_->selectionIdsAsVector();
    if (ids.empty()) {
        updateResultLabel(QStringLiteral("M5 — Aucun élément sélectionné"));
        return;
    }
    std::vector<MeasuredObject> objects;
    const double total = measureLongueurMainCourante(*viewport_, ids, &objects);
    const QString text = QStringLiteral("M5 — Longueur main courante : %1 ml").arg(total, 0, 'f', 3);
    updateResultLabel(text);
    addMeasureRecord(QStringLiteral("M5 — Longueur main courante"), total, QStringLiteral("ml"), 2, objects);
    if (session_) session_->setStatusMessage(QStringLiteral("Mesure"), text);
    emit measurementPerformed(QStringLiteral("M5 Main courante"), total, QStringLiteral("ml"));
}

void MeasurementPanel::runM6() {
    if (!selection_ || !viewport_) return;
    const auto ids = selection_->selectionIdsAsVector();
    if (ids.empty()) {
        updateResultLabel(QStringLiteral("M6 — Aucun élément sélectionné"));
        return;
    }
    std::vector<MeasuredObject> objects;
    std::vector<TopFace> faces;
    const double total = measureSurfaceTop(*viewport_, ids, &objects, &faces);
    const QString text = QStringLiteral("M6 — Surface top : %1 m²").arg(total, 0, 'f', 3);
    updateResultLabel(text);
    // One record per object so each top face gets its own highlight + label.
    for (std::size_t i = 0; i < faces.size(); ++i) {
        const int rec = addMeasureRecord(QStringLiteral("M6 — Surface top"),
                                         faces[i].area, QStringLiteral("m²"), 3, {objects[i]});
        records_[rec].highlight_tris = faces[i].triangles;
        for (int k = 0; k < 3; ++k) records_[rec].label_pos[k] = faces[i].centroid[k];
    }
    refreshGroupSums();
    refreshOverlays();
    if (session_) session_->setStatusMessage(QStringLiteral("Mesure"), text);
    emit measurementPerformed(QStringLiteral("M6 Surface top"), total, QStringLiteral("m²"));
}

void MeasurementPanel::runM7() {
    if (!selection_ || !viewport_) return;
    const auto ids = selection_->selectionIdsAsVector();
    if (ids.empty()) {
        updateResultLabel(QStringLiteral("M7 — Aucun élément sélectionné"));
        return;
    }
    std::vector<MeasuredObject> objects;
    std::vector<TopFace> faces;
    const double total = measureSurfaceBottom(*viewport_, ids, &objects, &faces);
    const QString text = QStringLiteral("M7 — Surface bottom : %1 m²").arg(total, 0, 'f', 3);
    updateResultLabel(text);
    for (std::size_t i = 0; i < faces.size(); ++i) {
        const int rec = addMeasureRecord(QStringLiteral("M7 — Surface bottom"),
                                         faces[i].area, QStringLiteral("m²"), 3, {objects[i]});
        records_[rec].highlight_tris = faces[i].triangles;
        for (int k = 0; k < 3; ++k) records_[rec].label_pos[k] = faces[i].centroid[k];
    }
    refreshGroupSums();
    refreshOverlays();
    if (session_) session_->setStatusMessage(QStringLiteral("Mesure"), text);
    emit measurementPerformed(QStringLiteral("M7 Surface bottom"), total, QStringLiteral("m²"));
}

void MeasurementPanel::runM8() {
    if (!selection_ || !viewport_) return;
    const auto ids = selection_->selectionIdsAsVector();
    if (ids.empty()) {
        updateResultLabel(QStringLiteral("M8 — Aucun élément sélectionné"));
        return;
    }
    const int mode = m8_mode_combo_ ? m8_mode_combo_->currentData().toInt() : 0;
    std::vector<MeasuredObject> objects;
    std::vector<ClassifiedFace> faces;
    const double total = measureSurfaceLateral(*viewport_, ids, mode, &objects, &faces);
    const QString mode_name = mode == 1 ? QStringLiteral("plus grande face")
                             : mode == 2 ? QStringLiteral("deux plus grandes faces")
                                         : QStringLiteral("toutes les faces");
    const QString text = QStringLiteral("M8 — Surface latérale (%1) : %2 m²")
                             .arg(mode_name).arg(total, 0, 'f', 3);
    updateResultLabel(text);
    for (std::size_t i = 0; i < faces.size(); ++i) {
        const int rec = addMeasureRecord(QStringLiteral("M8 — Surface latérale"),
                                         faces[i].area, QStringLiteral("m²"), 4, {objects[i]});
        records_[rec].highlight_tris = faces[i].triangles;
        for (int k = 0; k < 3; ++k) records_[rec].label_pos[k] = faces[i].centroid[k];
    }
    refreshGroupSums();
    refreshOverlays();
    if (session_) session_->setStatusMessage(QStringLiteral("Mesure"), text);
    emit measurementPerformed(QStringLiteral("M8 Surface latérale"), total, QStringLiteral("m²"));
}

void MeasurementPanel::runM11() {
    if (!selection_ || !viewport_) return;
    const auto ids = selection_->selectionIdsAsVector();
    if (ids.empty()) {
        updateResultLabel(QStringLiteral("M11 — Aucun élément sélectionné"));
        return;
    }
    std::vector<MeasuredObject> objects;
    std::vector<ClassifiedFace> faces;
    const double total = measureSurfaceToitureDeveloppee(*viewport_, ids, &objects, &faces);
    const QString text = QStringLiteral("M11 — Surface toiture développée : %1 m²").arg(total, 0, 'f', 3);
    updateResultLabel(text);
    for (std::size_t i = 0; i < faces.size(); ++i) {
        const int rec = addMeasureRecord(QStringLiteral("M11 — Toiture développée"),
                                         faces[i].area, QStringLiteral("m²"), 5, {objects[i]});
        records_[rec].highlight_tris = faces[i].triangles;
        for (int k = 0; k < 3; ++k) records_[rec].label_pos[k] = faces[i].centroid[k];
    }
    refreshGroupSums();
    refreshOverlays();
    if (session_) session_->setStatusMessage(QStringLiteral("Mesure"), text);
    emit measurementPerformed(QStringLiteral("M11 Toiture développée"), total, QStringLiteral("m²"));
}

void MeasurementPanel::runM17() {
    if (!selection_ || !viewport_) return;
    const auto ids = selection_->selectionIdsAsVector();
    if (ids.empty()) {
        updateResultLabel(QStringLiteral("M17 — Aucun élément sélectionné"));
        return;
    }
    std::vector<MeasuredObject> objects;
    const double total = measureVolumeElement(*viewport_, ids, &objects);
    const QString text = QStringLiteral("M17 — Volume élément : %1 m³").arg(total, 0, 'f', 3);
    updateResultLabel(text);
    addMeasureRecord(QStringLiteral("M17 — Volume élément"), total, QStringLiteral("m³"), 6, objects);
    if (session_) session_->setStatusMessage(QStringLiteral("Mesure"), text);
    emit measurementPerformed(QStringLiteral("M17 Volume élément"), total, QStringLiteral("m³"));
}

void MeasurementPanel::runM12() {
    if (!selection_ || !viewport_) return;
    const auto ids = selection_->selectionIdsAsVector();
    if (ids.empty()) {
        updateResultLabel(QStringLiteral("M12 — Aucun élément sélectionné"));
        return;
    }
    const bool include_abouts = m12_abouts_check_ && m12_abouts_check_->isChecked();
    const bool include_top = m12_top_check_ && m12_top_check_->isChecked();
    std::vector<MeasuredObject> objects;
    std::vector<ClassifiedFace> faces;
    const double total = measureSurfaceCoffrage(*viewport_, ids, include_abouts, include_top,
                                                &objects, &faces);
    const QString text = QStringLiteral("M12 — Surface coffrage : %1 m²").arg(total, 0, 'f', 3);
    updateResultLabel(text);
    for (std::size_t i = 0; i < faces.size(); ++i) {
        const int rec = addMeasureRecord(QStringLiteral("M12 — Surface coffrage"),
                                         faces[i].area, QStringLiteral("m²"), 4, {objects[i]});
        records_[rec].highlight_tris = faces[i].triangles;
        for (int k = 0; k < 3; ++k) records_[rec].label_pos[k] = faces[i].centroid[k];
    }
    refreshGroupSums();
    refreshOverlays();
    if (session_) session_->setStatusMessage(QStringLiteral("Mesure"), text);
    emit measurementPerformed(QStringLiteral("M12 Surface coffrage"), total, QStringLiteral("m²"));
}

void MeasurementPanel::runM13() {
    if (!selection_ || !viewport_) return;
    const auto ids = selection_->selectionIdsAsVector();
    if (ids.empty()) {
        updateResultLabel(QStringLiteral("M13 — Aucun élément sélectionné"));
        return;
    }
    std::vector<MeasuredObject> objects;
    const double total = measureSurfaceSectionProjetee(*viewport_, ids, &objects);
    const QString text = QStringLiteral("M13 — Surface section projetée : %1 m²").arg(total, 0, 'f', 3);
    updateResultLabel(text);
    addMeasureRecord(QStringLiteral("M13 — Surface section projetée"), total, QStringLiteral("m²"), 6, objects);
    if (session_) session_->setStatusMessage(QStringLiteral("Mesure"), text);
    emit measurementPerformed(QStringLiteral("M13 Section projetée"), total, QStringLiteral("m²"));
}

void MeasurementPanel::runM18() {
    if (!selection_ || !viewport_) return;
    const auto ids = selection_->selectionIdsAsVector();
    if (ids.empty()) {
        updateResultLabel(QStringLiteral("M18 — Aucun élément sélectionné"));
        return;
    }
    const double epaisseur = m18_epaisseur_spin_ ? m18_epaisseur_spin_->value() : 0.10;
    std::vector<MeasuredObject> objects;
    std::vector<TopFace> faces;
    const double total = measureVolumeCouche(*viewport_, ids, epaisseur, &objects, &faces);
    const QString text = QStringLiteral("M18 — Volume de couche (%1 m) : %2 m³")
                             .arg(epaisseur, 0, 'f', 3).arg(total, 0, 'f', 3);
    updateResultLabel(text);
    for (std::size_t i = 0; i < faces.size(); ++i) {
        const int rec = addMeasureRecord(QStringLiteral("M18 — Volume de couche"),
                                         faces[i].area * epaisseur, QStringLiteral("m³"), 3, {objects[i]});
        records_[rec].highlight_tris = faces[i].triangles;
        for (int k = 0; k < 3; ++k) records_[rec].label_pos[k] = faces[i].centroid[k];
    }
    refreshGroupSums();
    refreshOverlays();
    if (session_) session_->setStatusMessage(QStringLiteral("Mesure"), text);
    emit measurementPerformed(QStringLiteral("M18 Volume couche"), total, QStringLiteral("m³"));
}

void MeasurementPanel::runM9() {
    if (!selection_ || !viewport_ || !session_ || !session_->elementRegistry()) return;
    const auto ids = selection_->selectionIdsAsVector();
    if (ids.empty()) {
        updateResultLabel(QStringLiteral("M9 — Aucun élément sélectionné"));
        return;
    }
    double total = 0.0;
    int glazed = 0;
    for (const std::uint32_t id : ids) {
        const auto entity = session_->elementRegistry()->findEntity(id);
        if (!entity) continue;
        std::vector<MeasuredObject> objects;
        std::vector<ClassifiedFace> faces;
        const double area = measureSurfaceVitrage(*viewport_, *entity, id, &objects, &faces);
        if (area <= 0.0) continue;
        total += area;
        ++glazed;
        for (std::size_t i = 0; i < faces.size(); ++i) {
            const int rec = addMeasureRecord(QStringLiteral("M9 — Surface vitrage"),
                                             faces[i].area, QStringLiteral("m²"), 4, {objects[i]});
            records_[rec].highlight_tris = faces[i].triangles;
            for (int k = 0; k < 3; ++k) records_[rec].label_pos[k] = faces[i].centroid[k];
        }
    }
    refreshGroupSums();
    refreshOverlays();
    const QString text = QStringLiteral("M9 — Surface vitrage : %1 m² (%2 objet(s) transparent(s))")
                             .arg(total, 0, 'f', 3).arg(glazed);
    updateResultLabel(text);
    if (session_) session_->setStatusMessage(QStringLiteral("Mesure"), text);
    emit measurementPerformed(QStringLiteral("M9 Surface vitrage"), total, QStringLiteral("m²"));
}

void MeasurementPanel::runM16() {
    if (!selection_ || !viewport_ || !session_ || !session_->elementRegistry()) return;
    const auto ids = selection_->selectionIdsAsVector();
    if (ids.empty()) {
        updateResultLabel(QStringLiteral("M16 — Aucun élément sélectionné"));
        return;
    }
    double total_area = 0.0;
    int total_count = 0;
    std::vector<MeasuredObject> all_objects;
    for (const std::uint32_t id : ids) {
        const auto entity = session_->elementRegistry()->findEntity(id);
        if (!entity) continue;
        std::vector<MeasuredObject> objects;
        const OpeningResult r = measureSurfaceOuverture(*viewport_, *entity, id, &objects);
        total_area += r.area_m2;
        total_count += r.count;
        all_objects.insert(all_objects.end(), objects.begin(), objects.end());
    }
    const QString text = QStringLiteral("M16 — Surface ouverture : %1 m² (%2 ouverture(s))")
                             .arg(total_area, 0, 'f', 3).arg(total_count);
    updateResultLabel(text);
    addMeasureRecord(QStringLiteral("M16 — Surface ouverture"), total_area,
                     QStringLiteral("m²"), 6, all_objects);
    if (session_) session_->setStatusMessage(QStringLiteral("Mesure"), text);
    emit measurementPerformed(QStringLiteral("M16 Surface ouverture"), total_area, QStringLiteral("m²"));
}

void MeasurementPanel::runM20() {
    if (!selection_ || !viewport_) return;
    const auto ids = selection_->selectionIdsAsVector();
    if (ids.size() < 2) {
        updateResultLabel(QStringLiteral("M20 — Sélectionnez au moins 2 éléments"));
        return;
    }
    std::vector<MeasuredObject> objects;
    const double total = measureVolumeEntre(*viewport_, ids, &objects);
    const QString text = QStringLiteral("M20 — Volume entre 2 surfaces : %1 m³").arg(total, 0, 'f', 3);
    updateResultLabel(text);
    addMeasureRecord(QStringLiteral("M20 — Volume entre"), total, QStringLiteral("m³"), 6, objects);
    if (session_) session_->setStatusMessage(QStringLiteral("Mesure"), text);
    emit measurementPerformed(QStringLiteral("M20 Volume entre"), total, QStringLiteral("m³"));
}

void MeasurementPanel::runM27() {
    if (!selection_ || !viewport_) return;
    const auto ids = selection_->selectionIdsAsVector();
    if (ids.empty()) {
        updateResultLabel(QStringLiteral("M27 — Aucun élément sélectionné"));
        return;
    }
    std::vector<MeasuredObject> objects;
    const double slope = measurePente(*viewport_, ids, /*on_3d_length=*/false, &objects);
    const QString text = QStringLiteral("M27 — Pente moyenne : %1 %").arg(slope, 0, 'f', 2);
    updateResultLabel(text);
    addMeasureRecord(QStringLiteral("M27 — Pente"), slope, QStringLiteral("%"), 0, objects);
    if (session_) session_->setStatusMessage(QStringLiteral("Mesure"), text);
    emit measurementPerformed(QStringLiteral("M27 Pente"), slope, QStringLiteral("%"));
}

void MeasurementPanel::runM21() {
    if (!selection_ || !viewport_ || !session_ || !session_->elementRegistry()) return;
    const auto ids = selection_->selectionIdsAsVector();
    if (ids.empty()) {
        updateResultLabel(QStringLiteral("M21 — Aucun élément sélectionné"));
        return;
    }
    double surf = 0.0, perim = 0.0, haut = 0.0, vol = 0.0;
    int spaces = 0;
    for (const std::uint32_t id : ids) {
        const auto info = session_->elementRegistry()->findBasicElementInfo(id);
        if (!info || info->type != QStringLiteral("IfcSpace")) continue;
        const LocalMetrics m = measureLocalMetrics(*viewport_, id);
        if (m.surface_m2 <= 0.0 && m.hauteur_m <= 0.0) continue;
        ++spaces;
        surf += m.surface_m2;
        perim += m.perimetre_ml;
        haut += m.hauteur_m;
        vol += m.volume_m3;
        // One highlighted floor per space + 4 leaf records.
        Eigen::Vector3f emn, emx;
        if (viewport_->computeObjectAabb(id, emn, emx)) {
            MeasuredObject obj;
            obj.id = id;
            obj.mn[0] = emn.x(); obj.mn[1] = emn.y(); obj.mn[2] = emn.z();
            obj.mx[0] = emx.x(); obj.mx[1] = emx.y(); obj.mx[2] = emx.z();
            const int rec = addMeasureRecord(QStringLiteral("M21 — Local surface sol"),
                                             m.surface_m2, QStringLiteral("m²"), 3, {obj});
            records_[rec].highlight_tris = m.floor_tris;
            for (int k = 0; k < 3; ++k) records_[rec].label_pos[k] = m.label_pos[k];
        }
        addMeasureRecord(QStringLiteral("M21 — Local périmètre"), m.perimetre_ml, QStringLiteral("ml"), 1, {});
        addMeasureRecord(QStringLiteral("M21 — Local hauteur"), m.hauteur_m, QStringLiteral("m"), 0, {});
        addMeasureRecord(QStringLiteral("M21 — Local volume"), m.volume_m3, QStringLiteral("m³"), 6, {});
    }
    refreshGroupSums();
    refreshOverlays();
    const QString text = QStringLiteral("M21 — Local : %1 m², %2 ml, %3 m, %4 m³ (%5 local(aux))")
                             .arg(surf, 0, 'f', 3).arg(perim, 0, 'f', 3)
                             .arg(spaces > 0 ? haut / spaces : 0.0, 0, 'f', 3)
                             .arg(vol, 0, 'f', 3).arg(spaces);
    updateResultLabel(text);
    if (session_) session_->setStatusMessage(QStringLiteral("Mesure"), text);
    emit measurementPerformed(QStringLiteral("M21 Local"), surf, QStringLiteral("m²"));
}

void MeasurementPanel::runM22() {
    if (!selection_ || !viewport_ || !session_ || !session_->elementRegistry()) return;
    const auto ids = selection_->selectionIdsAsVector();
    if (ids.empty()) {
        updateResultLabel(QStringLiteral("M22 — Aucun élément sélectionné"));
        return;
    }
    const double seuil = m22_seuil_spin_ ? m22_seuil_spin_->value() : 1.80;
    double total = 0.0;
    int count = 0;
    for (const std::uint32_t id : ids) {
        const auto info = session_->elementRegistry()->findBasicElementInfo(id);
        if (!info || info->type != QStringLiteral("IfcSpace")) continue;
        std::vector<MeasuredObject> objects;
        std::vector<TopFace> faces;
        const double area = measureLocalSurfaceSousPlafond(*viewport_, id, seuil, &objects, &faces);
        if (area <= 0.0) continue;
        total += area;
        ++count;
        for (std::size_t i = 0; i < faces.size(); ++i) {
            const int rec = addMeasureRecord(QStringLiteral("M22 — Surface sous plafond"),
                                             faces[i].area, QStringLiteral("m²"), 3, {objects[i]});
            records_[rec].highlight_tris = faces[i].triangles;
            for (int k = 0; k < 3; ++k) records_[rec].label_pos[k] = faces[i].centroid[k];
        }
    }
    refreshGroupSums();
    refreshOverlays();
    const QString text = QStringLiteral("M22 — Surface sous plafond (>= %1 m) : %2 m² (%3 local(aux))")
                             .arg(seuil, 0, 'f', 2).arg(total, 0, 'f', 3).arg(count);
    updateResultLabel(text);
    if (session_) session_->setStatusMessage(QStringLiteral("Mesure"), text);
    emit measurementPerformed(QStringLiteral("M22 Sous plafond"), total, QStringLiteral("m²"));
}

int MeasurementPanel::addMeasureRecord(const QString& label, double value, const QString& unit,
                                       int mode, const std::vector<MeasuredObject>& objects) {
    // New measure goes into the selected group, or at the root.
    QTreeWidgetItem* parent = nullptr;
    QTreeWidgetItem* sel = measures_tree_->currentItem();
    if (sel) {
        const bool is_group = sel->data(0, kRoleIsGroup).toBool();
        parent = is_group ? sel : sel->parent();
    }

    auto* item = new QTreeWidgetItem();
    if (parent) {
        parent->addChild(item);
    } else {
        measures_tree_->addTopLevelItem(item);
    }
    item->setText(0, label);
    item->setText(1, QStringLiteral("%1 %2").arg(value, 0, 'f', 3).arg(unit));
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    item->setCheckState(0, Qt::Checked);
    const int id = static_cast<int>(records_.size());
    item->setData(0, kRoleRecordId, id);
    item->setData(0, kRoleIsGroup, false);

    MeasureRecord rec;
    rec.label = label;
    rec.value = value;
    rec.unit = unit;
    rec.mode = mode;
    rec.objects = objects;
    rec.item = item;
    records_.push_back(rec);

    refreshGroupSums();
    refreshOverlays();
    return id;
}

void MeasurementPanel::onNewGroup() {
    QTreeWidgetItem* parent = nullptr;
    QTreeWidgetItem* sel = measures_tree_->currentItem();
    if (sel) {
        const bool is_group = sel->data(0, kRoleIsGroup).toBool();
        parent = is_group ? sel : sel->parent();
    }
    auto* item = new QTreeWidgetItem();
    if (parent) {
        parent->addChild(item);
    } else {
        measures_tree_->addTopLevelItem(item);
    }
    item->setText(0, QStringLiteral("Nouveau groupe"));
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsEditable);
    item->setCheckState(0, Qt::Checked);
    item->setData(0, kRoleRecordId, -1);
    item->setData(0, kRoleIsGroup, true);
    measures_tree_->expandItem(item);
    measures_tree_->setCurrentItem(item);
    measures_tree_->editItem(item, 0);
    refreshGroupSums();
}

void MeasurementPanel::onTreeContextMenu(const QPoint& pos) {
    QTreeWidgetItem* item = measures_tree_->itemAt(pos);
    if (!item) return;
    const bool is_group = item->data(0, kRoleIsGroup).toBool();

    QMenu menu(this);
    if (is_group) {
        QAction* rename = menu.addAction(QStringLiteral("Renommer"));
        connect(rename, &QAction::triggered, this,
                [this, item]() { measures_tree_->editItem(item, 0); });
        menu.addSeparator();
    } else {
        const int id = item->data(0, kRoleRecordId).toInt();
        QAction* zoom = menu.addAction(QStringLiteral("Zoomer dessus"));
        connect(zoom, &QAction::triggered, this, [this, id]() { zoomOnRecord(id); });
        QAction* isolate = menu.addAction(QStringLiteral("Isoler"));
        connect(isolate, &QAction::triggered, this, [this, id]() { isolateRecord(id); });
        menu.addSeparator();
    }
    QAction* del = menu.addAction(is_group ? QStringLiteral("Supprimer le groupe")
                                           : QStringLiteral("Supprimer la mesure"));
    connect(del, &QAction::triggered, this, [this, item]() { deleteTreeItem(item); });
    menu.exec(measures_tree_->viewport()->mapToGlobal(pos));
}

void MeasurementPanel::deleteTreeItem(QTreeWidgetItem* item) {
    // Detach the whole sub-tree (group + its measures), then drop the item.
    if (QTreeWidgetItem* parent = item->parent()) {
        parent->removeChild(item);
    } else {
        const int idx = measures_tree_->indexOfTopLevelItem(item);
        if (idx >= 0) measures_tree_->takeTopLevelItem(idx);
    }
    delete item;
    refreshGroupSums();
    refreshOverlays();
}

void MeasurementPanel::zoomOnRecord(int id) {
    if (!viewport_ || id < 0 || id >= static_cast<int>(records_.size())) return;
    const auto& rec = records_[id];
    if (rec.objects.empty()) return;
    float mn[3] = { rec.objects.front().mn[0], rec.objects.front().mn[1], rec.objects.front().mn[2] };
    float mx[3] = { rec.objects.front().mx[0], rec.objects.front().mx[1], rec.objects.front().mx[2] };
    for (const auto& obj : rec.objects) {
        for (int i = 0; i < 3; ++i) {
            mn[i] = std::min(mn[i], obj.mn[i]);
            mx[i] = std::max(mx[i], obj.mx[i]);
        }
    }
    viewport_->frameAabb(mn, mx, 0.5f);
}

void MeasurementPanel::isolateRecord(int id) {
    if (!active_view_ || id < 0 || id >= static_cast<int>(records_.size())) return;
    std::vector<std::uint32_t> ids;
    for (const auto& obj : records_[id].objects) ids.push_back(obj.id);
    if (!ids.empty()) active_view_->isolate(ids);
}

void MeasurementPanel::onItemChanged(QTreeWidgetItem* /*item*/, int column) {
    if (column != 0) return;
    refreshGroupSums();
    refreshOverlays();
}

bool MeasurementPanel::isItemVisible(const QTreeWidgetItem* item) const {
    if (!item) return false;
    if (item->checkState(0) != Qt::Checked) return false;
    const QTreeWidgetItem* parent = item->parent();
    if (!parent) return true;  // top level / invisible root
    return isItemVisible(parent);
}

void MeasurementPanel::collectSums(QTreeWidgetItem* item, QMap<QString, double>& sums) const {
    for (int i = 0; i < item->childCount(); ++i) {
        QTreeWidgetItem* child = item->child(i);
        const int id = child->data(0, kRoleRecordId).toInt();
        if (id >= 0 && id < static_cast<int>(records_.size())) {
            sums[records_[id].unit] += records_[id].value;
        } else {
            collectSums(child, sums);
        }
    }
}

void MeasurementPanel::refreshGroupSums() {
    std::function<void(QTreeWidgetItem*)> walk = [&](QTreeWidgetItem* item) {
        for (int i = 0; i < item->childCount(); ++i) {
            QTreeWidgetItem* child = item->child(i);
            if (child->data(0, kRoleIsGroup).toBool()) {
                QMap<QString, double> sums;
                collectSums(child, sums);
                QStringList parts;
                for (auto it = sums.begin(); it != sums.end(); ++it) {
                    parts.append(QStringLiteral("%1 %2").arg(it.value(), 0, 'f', 3).arg(it.key()));
                }
                child->setText(1, parts.join(QStringLiteral(", ")));
                walk(child);
            }
        }
    };
    walk(measures_tree_->invisibleRootItem());
}

void MeasurementPanel::drawRecord(OverlayRenderer::LineGroup& group,
                                  std::vector<OverlayRenderer::Label>& labels,
                                  std::vector<float>& highlights,
                                  const MeasureRecord& rec) {
    if (rec.mode < 0) return;  // M1: counting only, no trait
    if (rec.mode >= 3 && rec.mode <= 5) {
        // M0/M6/M7: horizontal face highlight, M8: lateral faces, M11:
        // inclined roof faces. Triangles are pushed 2 cm out along their
        // normal so the highlight does not z-fight / clip into the geometry.
        constexpr float kHighlightOffset = 0.02f;
        for (std::size_t t = 0; t + 8 < rec.highlight_tris.size(); t += 9) {
            const float* a = &rec.highlight_tris[t];
            const float* b = &rec.highlight_tris[t + 3];
            const float* c = &rec.highlight_tris[t + 6];
            const float ux = b[0] - a[0], uy = b[1] - a[1], uz = b[2] - a[2];
            const float vx = c[0] - a[0], vy = c[1] - a[1], vz = c[2] - a[2];
            const float nx = uy * vz - uz * vy;
            const float ny = uz * vx - ux * vz;
            const float nz = ux * vy - uy * vx;
            const float len = std::sqrt(nx * nx + ny * ny + nz * nz);
            if (len < 1e-9f) continue;
            const float ox = nx / len * kHighlightOffset;
            const float oy = ny / len * kHighlightOffset;
            const float oz = nz / len * kHighlightOffset;
            for (int vtx = 0; vtx < 3; ++vtx) {
                const float* p = &rec.highlight_tris[t + 3 * vtx];
                highlights.push_back(p[0] + ox);
                highlights.push_back(p[1] + oy);
                highlights.push_back(p[2] + oz);
            }
        }
        if (!rec.highlight_tris.empty()) {
            OverlayRenderer::Label lb;
            lb.world_pos[0] = rec.label_pos[0];
            lb.world_pos[1] = rec.label_pos[1];
            lb.world_pos[2] = rec.label_pos[2];
            lb.text = QStringLiteral("%1 %2").arg(rec.value, 0, 'f', 3).arg(rec.unit);
            labels.push_back(lb);
        }
        return;
    }
    if (rec.mode == 6) {
        // M17 volume: draw the AABB box edges as the measured trait.
        for (const auto& obj : rec.objects) {
            const float* mn = obj.mn;
            const float* mx = obj.mx;
            auto addSegment = [&](float ax, float ay, float az, float bx, float by, float bz) {
                const float seg[6] = {ax, ay, az, bx, by, bz};
                group.world_xyz.insert(group.world_xyz.end(), seg, seg + 6);
            };
            const float x0 = mn[0], y0 = mn[1], z0 = mn[2];
            const float x1 = mx[0], y1 = mx[1], z1 = mx[2];
            // 12 edges of the box.
            addSegment(x0,y0,z0, x1,y0,z0); addSegment(x1,y0,z0, x1,y1,z0);
            addSegment(x1,y1,z0, x0,y1,z0); addSegment(x0,y1,z0, x0,y0,z0);
            addSegment(x0,y0,z1, x1,y0,z1); addSegment(x1,y0,z1, x1,y1,z1);
            addSegment(x1,y1,z1, x0,y1,z1); addSegment(x0,y1,z1, x0,y0,z1);
            addSegment(x0,y0,z0, x0,y0,z1); addSegment(x1,y0,z0, x1,y0,z1);
            addSegment(x1,y1,z0, x1,y1,z1); addSegment(x0,y1,z0, x0,y1,z1);
            OverlayRenderer::Label lb;
            lb.world_pos[0] = (x0 + x1) / 2.0f;
            lb.world_pos[1] = (y0 + y1) / 2.0f;
            lb.world_pos[2] = (z0 + z1) / 2.0f;
            lb.text = QStringLiteral("%1 %2").arg(rec.value, 0, 'f', 3).arg(rec.unit);
            labels.push_back(lb);
        }
        return;
    }
    for (const auto& obj : rec.objects) {
        const float* mn = obj.mn;
        const float* mx = obj.mx;
        const double dx = static_cast<double>(mx[0]) - mn[0];
        const double dy = static_cast<double>(mx[1]) - mn[1];
        const double dz = static_cast<double>(mx[2]) - mn[2];
        const float cx = (mn[0] + mx[0]) / 2.0f;
        const float cy = (mn[1] + mx[1]) / 2.0f;
        const float cz = (mn[2] + mx[2]) / 2.0f;

        auto addSegment = [&](float ax, float ay, float az, float bx, float by, float bz) {
            const float seg[6] = {ax, ay, az, bx, by, bz};
            group.world_xyz.insert(group.world_xyz.end(), seg, seg + 6);
        };
        auto addLabel = [&](float x, float y, float z, double value) {
            OverlayRenderer::Label lb;
            lb.world_pos[0] = x; lb.world_pos[1] = y; lb.world_pos[2] = z;
            lb.text = QStringLiteral("%1 %2").arg(value, 0, 'f', 3).arg(rec.unit);
            labels.push_back(lb);
        };

        if (rec.mode == 1) {
            // Base rectangle at z = mn[2].
            const float z = mn[2];
            addSegment(mn[0], mn[1], z, mx[0], mn[1], z);
            addSegment(mx[0], mn[1], z, mx[0], mx[1], z);
            addSegment(mx[0], mx[1], z, mn[0], mx[1], z);
            addSegment(mn[0], mx[1], z, mn[0], mn[1], z);
            addLabel(cx, cy, z, 2.0 * (dx + dy));
        } else if (rec.mode == 2) {
            // Longest horizontal edge on the top face (z = mx[2]);
            // horizontal = XY plane (Z is up).
            const float z = mx[2];
            if (dx >= dy) {
                addSegment(mn[0], cy, z, mx[0], cy, z);
                addLabel(cx, cy, z, dx);
            } else {
                addSegment(cx, mn[1], z, cx, mx[1], z);
                addLabel(cx, cy, z, dy);
            }
        } else {
            // Longest AABB edge through the box center.
            if (dx >= dy && dx >= dz) {
                addSegment(mn[0], cy, cz, mx[0], cy, cz);
                addLabel(cx, cy, cz, dx);
            } else if (dy >= dz) {
                addSegment(cx, mn[1], cz, cx, mx[1], cz);
                addLabel(cx, cy, cz, dy);
            } else {
                addSegment(cx, cy, mn[2], cx, cy, mx[2]);
                addLabel(cx, cy, cz, dz);
            }
        }
    }
}

void MeasurementPanel::refreshOverlays() {
    if (!viewport_) return;

    OverlayRenderer::LineGroup group;
    group.color[0] = 1.0f;  group.color[1] = 0.85f; group.color[2] = 0.1f; group.color[3] = 1.0f;
    group.stroke_color[0] = 0.0f; group.stroke_color[1] = 0.0f;
    group.stroke_color[2] = 0.0f; group.stroke_color[3] = 1.0f;
    group.line_width = 3.0f;
    group.stroke_extra = 1.0f;
    group.xray = true;  // measure traits stay visible through transparent objects

    std::vector<OverlayRenderer::Label> labels;
    std::vector<float> highlights;
    QMap<QString, double> visible_sums;

    if (geometry_visible_) {
        std::function<void(QTreeWidgetItem*)> walk = [&](QTreeWidgetItem* parent) {
            for (int i = 0; i < parent->childCount(); ++i) {
                QTreeWidgetItem* child = parent->child(i);
                const int id = child->data(0, kRoleRecordId).toInt();
                if (id >= 0 && id < static_cast<int>(records_.size())) {
                    if (isItemVisible(child)) {
                        drawRecord(group, labels, highlights, records_[id]);
                        visible_sums[records_[id].unit] += records_[id].value;
                    }
                } else {
                    walk(child);
                }
            }
        };
        walk(measures_tree_->invisibleRootItem());
    }

    if (group.world_xyz.empty()) {
        viewport_->setOverlayLines({});
    } else {
        viewport_->setOverlayLines({group});
    }
    viewport_->setOverlayLabels(labels);
    viewport_->setHighlightTriangles(highlights, 1.0f, 0.85f, 0.1f, 0.55f);

    QStringList hud_parts;
    for (auto it = visible_sums.begin(); it != visible_sums.end(); ++it) {
        hud_parts.append(QStringLiteral("%1 %2").arg(it.value(), 0, 'f', 3).arg(it.key()));
    }
    viewport_->setHudText(hud_parts.isEmpty() ? std::string{}
                                              : hud_parts.join(QStringLiteral(", ")).toStdString());
}

void MeasurementPanel::resetMeasurements() {
    records_.clear();
    measures_tree_->clear();
    clearViewportOverlays();
    updateResultLabel(QStringLiteral("Mesures effacées"));
    if (session_) session_->setStatusMessage(QStringLiteral("Mesure"), QStringLiteral("Mesures effacées"));
}

void MeasurementPanel::setMeasurementGeometryVisible(bool visible) {
    geometry_visible_ = visible;
    refreshOverlays();
}

void MeasurementPanel::updateResultLabel(const QString& text) {
    if (result_label_) result_label_->setText(text);
}

void MeasurementPanel::clearViewportOverlays() {
    // Clear all measurement overlays from the viewport
    if (viewport_) {
        viewport_->setOverlayLines({});
        viewport_->setOverlayPoints({}, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        viewport_->setOverlayLabels({});
        viewport_->setHighlightTriangles({}, 0, 0, 0, 0);
        viewport_->setHudText({});
    }
}

} // namespace bonsaiviewer::modules::measurement
