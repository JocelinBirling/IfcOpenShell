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

#ifndef IFCINTERFACE_MODULES_DIFF_PANEL_H
#define IFCINTERFACE_MODULES_DIFF_PANEL_H

#include <QList>
#include <QPair>
#include <QString>
#include <QWidget>

#include <cstdint>
#include <map>
#include <set>
#include <string>

class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;

namespace bonsaiviewer::services { class PropertyCatalog; }
namespace bonsaiviewer::modules::property_selection { class PropertyField; }

namespace bonsaiviewer::modules::diff {

// The property change kinds of the selection buttons (BonsaiViewer34).
enum class SelectKind {
    PropertyAdded,
    PropertyRemoved,
    ValueAdded,
    ValueRemoved,
    ValueChanged,
    ValueIncreased,
    ValueDecreased,
};

// The four coloured cases of the viewport (BonsaiViewer34).
enum class DiffCase {
    Deleted,        // red, Before only
    Added,          // green, After only
    ModifiedBefore, // blue
    ModifiedAfter,  // orange
};

// The geometry change kinds of the selection buttons (BonsaiViewer34).
enum class GeometryKind {
    Any,      // any geometry change
    Moved,    // the placement location changed
    Rotated,  // the placement orientation changed
    Shape,    // the Body representation changed (items, dimensions, profile...)
    Openings, // openings or projections added / removed
};

// BonsaiViewer34 — the IfcDiff panel: Before / After models, relationships,
// options, Run, then the result actions (legend, transparency, Clear Diff,
// property filters, report). Holds no diff logic: DiffPanelView drives it.
class DiffPanel : public QWidget {
    Q_OBJECT

public:
    explicit DiffPanel(bonsaiviewer::services::PropertyCatalog& catalog, QWidget* parent = nullptr);

    // (session model id, display name) of every loaded model; the current
    // choices are kept when their model is still there.
    void setModels(const QList<QPair<std::uint32_t, QString>>& models);
    std::uint32_t beforeModel() const;
    std::uint32_t afterModel() const;
    std::set<std::string> relationships() const;
    bool isShallow() const;
    QString idsPath() const { return ids_path_; }
    QString propertySet() const;
    QString propertyName() const;

    void setRunning(bool running);
    void setSummary(const QString& text);
    // Enables the result actions (after a run, off once the result is cleared).
    void setResultAvailable(bool available);
    void setBeforeTransparent(bool before);

signals:
    void runRequested();
    void exportRequested();
    void clearRequested();
    void swapTransparencyRequested();
    void selectRequested(SelectKind kind, bool isolate);
    void geometrySelectRequested(GeometryKind kind, bool isolate);
    void caseSelectRequested(DiffCase kind, bool isolate);
    void reportRequested();
    void compareRequested();

private:
    void chooseIds();

    QComboBox* before_combo_ = nullptr;
    QComboBox* after_combo_ = nullptr;
    std::map<std::string, QCheckBox*> relationship_checks_;
    QCheckBox* shallow_check_ = nullptr;
    QLabel* ids_label_ = nullptr;
    QPushButton* ids_clear_button_ = nullptr;
    QString ids_path_;
    QPushButton* run_button_ = nullptr;
    QLabel* summary_label_ = nullptr;
    QWidget* result_box_ = nullptr;
    QPushButton* swap_button_ = nullptr;
    bonsaiviewer::modules::property_selection::PropertyField* property_field_ = nullptr;
};

} // namespace bonsaiviewer::modules::diff

#endif // IFCINTERFACE_MODULES_DIFF_PANEL_H
