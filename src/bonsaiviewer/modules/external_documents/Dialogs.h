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

#ifndef BONSAIVIEWER_MODULES_EXTERNAL_DOCUMENTS_DIALOGS_H
#define BONSAIVIEWER_MODULES_EXTERNAL_DOCUMENTS_DIALOGS_H

#include "JoinEngine.h"
#include "TableFile.h"
#include "Types.h"

#include <QDialog>

#include <cstdint>

class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QSlider;
class QSpinBox;
class QTableWidget;
class QTreeWidget;
class QCheckBox;

namespace bonsaiviewer {
class SessionState;
namespace services {
class IfcDataAccess;
class PropertyCatalog;
}
} // namespace bonsaiviewer

namespace bonsaiviewer::modules::property_selection { class PropertyField; }

namespace bonsaiviewer::modules::external_documents {

// BonsaiViewer35 — the join configuration window of an external table:
// model key (GlobalId / Name / Class / Property with the standard property
// field of BonsaiViewer26), table key column, sheet (.xlsx), Get Lucky
// auto-detection on the first 10 rows, and a preview of those rows with the
// chosen key column highlighted.
class JoinDialog : public QDialog {
    Q_OBJECT
public:
    JoinDialog(const ExternalDocument& document, const TableData& data,
               SessionState& session, services::IfcDataAccess& data_access,
               services::PropertyCatalog& catalog, QWidget* parent = nullptr);

    // The edited configuration; valid after Accepted.
    JoinConfig config() const { return config_; }

private:
    void rebuildPreview();
    void applyGuess(int index);

    ExternalDocument document_;
    TableData data_;
    SessionState& session_;
    services::IfcDataAccess& data_access_;
    services::PropertyCatalog& catalog_;
    JoinConfig config_;

    QComboBox* model_key_combo_ = nullptr;
    property_selection::PropertyField* property_field_ = nullptr;
    QComboBox* key_column_combo_ = nullptr;
    QComboBox* sheet_combo_ = nullptr;
    QComboBox* guess_combo_ = nullptr;
    QTableWidget* preview_ = nullptr;
    QList<JoinGuess> guesses_;
};

// BonsaiViewer36 — the Plan Configuration window (Modify Plan / Edit Plan):
// source with Change Source…, height Z, rotations, scale, translation
// (model or plan axes), page (PDF), opacity. Apply validates and closes,
// Cancel discards.
class PlanDialog : public QDialog {
    Q_OBJECT
public:
    PlanDialog(const ExternalDocument& document, QWidget* parent = nullptr);

    // The edited configuration; valid after Accepted.
    PlanConfig config() const { return config_; }
    QString sourcePath() const { return source_path_; }

private:
    PlanConfig config_;
    QString source_path_;

    QDoubleSpinBox* height_spin_ = nullptr;
    QDoubleSpinBox* rot_x_spin_ = nullptr;
    QDoubleSpinBox* rot_y_spin_ = nullptr;
    QDoubleSpinBox* rot_z_spin_ = nullptr;
    QDoubleSpinBox* scale_spin_ = nullptr;
    QDoubleSpinBox* trans_x_spin_ = nullptr;
    QDoubleSpinBox* trans_y_spin_ = nullptr;
    QDoubleSpinBox* trans_z_spin_ = nullptr;
    QCheckBox* trans_plan_check_ = nullptr;
    QSpinBox* page_spin_ = nullptr;
    QSlider* opacity_slider_ = nullptr;
    QLabel* opacity_label_ = nullptr;
};

// BonsaiViewer35 — the match report after a load, refresh or join change:
// unmatched rows, duplicate rows (the file's columns) and unmatched objects
// (Model, Class, Name, GlobalId, key value), with click-to-select on the
// object rows.
class ReportDialog : public QDialog {
    Q_OBJECT
public:
    ReportDialog(const QString& document_name, const TableData& data, const MatchReport& report,
                 QWidget* parent = nullptr);

signals:
    void objectSelected(std::uint32_t object_id);

private:
    QTreeWidget* objects_tree_ = nullptr;
};

} // namespace bonsaiviewer::modules::external_documents

#endif // BONSAIVIEWER_MODULES_EXTERNAL_DOCUMENTS_DIALOGS_H
