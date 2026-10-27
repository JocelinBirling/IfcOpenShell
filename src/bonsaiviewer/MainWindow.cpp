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

#include "MainWindow.h"

#include "../ifcviewer/AppSettings.h"
#include "../ifcviewer/Federation.h"
#include "../ifcviewer/ViewportWindow.h"
#include "SessionState.h"
#include "components/Buttons.h"
#include "components/Dialog.h"
#include "components/GenericTopPanel.h"
#include "components/SpecificTopPanel.h"
#include "components/Panel.h"
#include "components/Style.h"
#include "components/Tabs.h"
#include "modules/models/Commands.h"
#include "modules/todo/Panel.h"
#include "modules/models/View.h"
#include "modules/models/Panel.h"
#include "modules/project/Commands.h"
#include "modules/project/RecentProjects.h"
#include "modules/properties/View.h"
#include "modules/properties/Panel.h"
#include "modules/settings/Dialog.h"
#include "modules/spatial_hierarchy/View.h"
#include "modules/spatial_hierarchy/Panel.h"
#include "modules/menubar/MenuBar.h"
#include "modules/searchbar/SearchBar.h"
#include "modules/filters/View.h"
#include "modules/filters/Panel.h"
#include "modules/table/Panel.h"
#include "modules/table/TableStore.h"
#include "modules/table/TablesPanel.h"
#include "modules/table/TablesView.h"
#include "modules/table/View.h"
#include "modules/text_digger/Panel.h"
#include "modules/text_digger/View.h"
#include "services/ActionPipeline.h"
#include "services/AppearanceService.h"
#include "services/ActiveView.h"
#include "services/UndoStack.h"
#include "services/ShortcutStore.h"

#include <functional>
#include "services/CameraService.h"
#include "services/CategoryOverrideFilter.h"
#include "services/ContextMenuBuilder.h"
#include "services/EntitySetStore.h"
#include "services/FilterRunner.h"
#include "services/FilterStore.h"
#include "services/TreePanelFramework.h"
#include "modules/class_tree/ClassTreeDataSource.h"
#include <QTreeWidget>
#include "services/IdsBridge.h"
#include "services/PropertyCatalog.h"
#include "services/IfcDataAccess.h"
#include "services/PanelManager.h"
#include "services/PickingService.h"
#include "services/PopupService.h"
#include "services/QueryEngine.h"
#include "services/SectionPlaneManager.h"
#include "services/SelectionService.h"
#include "modules/viewport/Commands.h"
#include "modules/viewport/Panel.h"
#include "modules/viewport/View.h"
#include "modules/easy_load/View.h"

#include <QAction>
#include <QDockWidget>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QFont>
#include <QIcon>
#include <QLabel>
#include <QMenu>
#include <QColorDialog>
#include <QPushButton>
#include <QMenuBar>
#include <QMessageBox>
#include <QMouseEvent>
#include <QShortcut>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QStatusBar>
#include <QProgressBar>
#include <QToolButton>
#include <QVBoxLayout>

namespace bonsaiviewer::shell {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    session_state_ = new bonsaiviewer::SessionState(this);
    auto on_mutated = [this]() {
        setWindowModified(true);
        updateWindowTitle();
    };
    auto on_clean_state = [this]() {
        setWindowModified(false);
        updateWindowTitle();
    };
    connect(session_state_, &bonsaiviewer::SessionState::federationChanged, this, on_mutated);
    connect(session_state_, &bonsaiviewer::SessionState::modelsChanged,     this, on_mutated);
    connect(session_state_, &bonsaiviewer::SessionState::projectReset,      this, on_clean_state);
    connect(session_state_, &bonsaiviewer::SessionState::projectOpened,     this, on_clean_state);
    connect(session_state_, &bonsaiviewer::SessionState::projectSaved,      this, on_clean_state);

    // Every successful open/save (local or cloud) feeds the recent list.
    auto remember_recent = [](const QString& path) {
        modules::project::RecentProjects::add(path);
    };
    connect(session_state_, &bonsaiviewer::SessionState::projectOpened, this, remember_recent);
    connect(session_state_, &bonsaiviewer::SessionState::projectSaved,  this, remember_recent);
    setupChrome();
    setupViewport();
    setupPanels();
    setupStatus();
    setupLoader();
    setupMenuBar();
    setupRibbon();
    resize(1720, 980);
}

namespace {

// BonsaiViewer31: one QShortcut per function, named after its id so that
// syncShortcutKeys finds it again. The key comes from ShortcutStore.
void bindShortcut(QWidget* window, const QString& id, std::function<void()> handler) {
    auto* shortcut = new QShortcut(
        bonsaiviewer::services::ShortcutStore::instance().sequence(id), window);
    shortcut->setObjectName(id);
    shortcut->setContext(Qt::WindowShortcut);
    QObject::connect(shortcut, &QShortcut::activated, window, std::move(handler));
}

// Re-reads the key of every bound function after a change in ShortcutStore.
void syncShortcutKeys(QWidget* window) {
    const auto& store = bonsaiviewer::services::ShortcutStore::instance();
    for (auto* shortcut : window->findChildren<QShortcut*>()) {
        if (store.definition(shortcut->objectName()) == nullptr) continue;
        shortcut->setKey(store.sequence(shortcut->objectName()));
    }
}

} // namespace

void MainWindow::setupChrome() {
    setObjectName("appWindow");
    setDockOptions(QMainWindow::AllowNestedDocks |
                   QMainWindow::AllowTabbedDocks |
                   QMainWindow::GroupedDragging);

    // BonsaiViewer31: the functions that are not ribbon buttons are bound
    // here; the ribbon buttons in populateGenericTopPanel, the search bar in
    // setupRibbon. Every key is read from ShortcutStore.
    // BonsaiViewer27: the stack exists only once the services are created.
    bindShortcut(this, "view.undo", [this]() {
        if (undo_stack_ != nullptr) undo_stack_->undo();
    });
    bindShortcut(this, "section.plane", [this]() {
        modules::viewport::commands::toggleSection(*session_state_, *viewport_widget_->viewport());
    });
    bindShortcut(this, "section.remove", [this]() {
        modules::viewport::commands::clearSection(*session_state_, *viewport_widget_->viewport());
    });
    bindShortcut(this, "section.hide", [this]() {
        auto* viewport = viewport_widget_->viewport();
        viewport->setSectionGizmosHidden(!viewport->sectionGizmosHidden());
    });
    bindShortcut(this, "section.suspend", [this]() {
        auto* viewport = viewport_widget_->viewport();
        viewport->setSectionsSuspended(!viewport->sectionsSuspended());
    });
    bindShortcut(this, "camera.fly", [this]() {
        modules::viewport::commands::fly(*session_state_, *viewport_widget_->viewport());
    });
    bindShortcut(this, "tool.measure_distance", [this]() {
        modules::viewport::commands::toggleDistance(*viewport_widget_->viewport());
    });
    bindShortcut(this, "tool.measure_area", [this]() {
        modules::viewport::commands::toggleArea(*viewport_widget_->viewport());
    });
    bindShortcut(this, "tool.measure_volume", [this]() {
        modules::viewport::commands::toggleVolume(*viewport_widget_->viewport());
    });
    bindShortcut(this, "view.isolate_selection", [this]() {
        active_view_->isolate(selection_service_->selectionIdsAsVector());
    });
    connect(&bonsaiviewer::services::ShortcutStore::instance(),
            &bonsaiviewer::services::ShortcutStore::changed, this,
            [this]() { syncShortcutKeys(this); });
    updateWindowTitle();
}

QToolButton* MainWindow::makePanelToggle(const QString& text, QDockWidget* dock) {
    auto* button = components::buttons::makeButton(text, ":/icons/sidebar-expand.svg", this);
    button->setCheckable(true);
    button->setChecked(dock->isVisible());
    connect(button, &QToolButton::toggled, dock, [dock](bool checked) {
        dock->setVisible(checked);
        if (checked) dock->raise();
    });
    connect(dock, &QDockWidget::visibilityChanged, button, [button](bool visible) {
        const QSignalBlocker blocker(button);
        button->setChecked(visible);
    });
    return button;
}

void MainWindow::populateRecentMenu(QMenu* menu) {
    menu->clear();

    const QStringList recent = modules::project::RecentProjects::list();
    if (recent.isEmpty()) {
        QAction* empty = menu->addAction("No Recent Projects");
        empty->setEnabled(false);
        return;
    }

    for (const QString& path : recent) {
        QAction* action = menu->addAction(QFileInfo(path).fileName());
        action->setToolTip(path);
        connect(action, &QAction::triggered, this, [this, path]() {
            // A plain .ifc is a model: use the Add Model flow. Anything
            // else (project files) keeps the project-open path.
            const bool is_ifc = QFileInfo(path).suffix().compare(
                                    QStringLiteral("ifc"), Qt::CaseInsensitive) == 0;
            const bool opened = is_ifc
                ? modules::models::commands::addModelFromPath(*session_state_, path)
                : modules::project::commands::openProjectPath(
                      *session_state_, *this, *viewport_widget_->viewport(), path);
            // A recent entry that no longer loads is dropped so it stops
            // cluttering the menu (e.g. moved/deleted file).
            if (!opened && !QFileInfo::exists(path)) {
                modules::project::RecentProjects::remove(path);
            }
        });
    }

    menu->addSeparator();
    QAction* clear = menu->addAction("Clear Recent Projects");
    connect(clear, &QAction::triggered, this, []() {
        modules::project::RecentProjects::clear();
    });
}

// TI05: the generic top panel hosts the search bar's neighbours — the
// display, box, zoom and colour groups — always visible whatever the
// ribbon tab.
void MainWindow::populateGenericTopPanel(components::GenericTopPanel* panel) {
    auto* row = panel->row();

    auto* hide_selected = components::buttons::makeButton("Hide", ":/icons/eye-closed.svg", this);
    connect(hide_selected, &QToolButton::clicked, this, [this]() {
        active_view_->remove(selection_service_->selectionIdsAsVector());
    });
    auto* show_all = components::buttons::makeButton("Show All", ":/icons/eye.svg", this);
    connect(show_all, &QToolButton::clicked, this, [this]() {
        active_view_->showAll();
    });
    auto* invert_selection = components::buttons::makeButton("Invert View", ":/icons/intersect.svg", this);
    connect(invert_selection, &QToolButton::clicked, this, [this]() {
        active_view_->invert();
    });
    // TI02 Display group: show the selection, hide everything, and
    // "Select Everything in view" (B01 from B02's visible set).
    auto* show_selected = components::buttons::makeButton("Show", ":/icons/eye-solid.svg", this);
    connect(show_selected, &QToolButton::clicked, this, [this]() {
        active_view_->add(selection_service_->selectionIdsAsVector());
    });
    auto* select_in_view = components::buttons::makeButton("Select Everything", ":/icons/cursor-pointer.svg", this);
    connect(select_in_view, &QToolButton::clicked, this, [this]() {
        selection_service_->setSelection(active_view_->visibleIds(),
                                         bonsaiviewer::services::SelectionSource::Service,
                                         bonsaiviewer::services::SelectionMode::Replace);
    });
    // Spec 02 Restore the View: one click resets the display state —
    // every colour / transparency override (B04), every hidden model,
    // and every hidden element (B02's showAll, which re-applies the
    // category filter).
    auto* restore_view = components::buttons::makeButton("Restore View", ":/icons/refresh-double.svg", this);
    connect(restore_view, &QToolButton::clicked, this, [this]() {
        appearance_service_->clearOverrides();
        for (std::uint32_t session_model_id : session_state_->sessionModelIds()) {
            viewport_widget_->viewport()->showModel(session_model_id);
        }
        active_view_->showAll();
        session_state_->setStatusMessage("Display", "View restored");
    });

    // TI02 quick category menu: « Show / Hide » toggles for the
    // categories a neophyte needs (B03 is the gate — a disabled
    // category can never come back through isolation or Show All).
    auto* categories = components::buttons::makeButton("Categories", ":/icons/filter.svg", this);
    categories->setPopupMode(QToolButton::InstantPopup);
    auto* categories_menu = new QMenu(categories);
    for (const char* category : {"IfcWall", "IfcSlab", "IfcRoof", "IfcCeiling",
                                 "IfcSpace", "IfcOpeningElement"}) {
        const QString ifc_class = QString::fromUtf8(category);
        auto* action = categories_menu->addAction(QString::fromUtf8(category));
        action->setCheckable(true);
        action->setChecked(category_filter_->isCategoryEnabled(ifc_class));
        connect(action, &QAction::toggled, this, [this, ifc_class](bool enabled) {
            category_filter_->setCategoryEnabled(ifc_class, enabled);
        });
        connect(category_filter_, &bonsaiviewer::services::CategoryOverrideFilter::categoryChanged,
                this, [action, ifc_class](const QString& changed, bool enabled) {
                    if (changed == ifc_class) action->setChecked(enabled);
                });
    }
    categories->setMenu(categories_menu);

    // Spec 04 Box (TI04): a temporary set of objects with session
    // lifetime, driven through B07. Isolate goes through B08, Hide
    // through B02.
    const auto report_box = [this]() {
        session_state_->setStatusMessage(
            "Box", QStringLiteral("Box: %1 objects")
                       .arg(entity_set_store_->current().size()));
    };
    auto* selection_to_box = components::buttons::makeButton("Selection", ":/icons/box-arrow-in.svg", this);
    connect(selection_to_box, &QToolButton::clicked, this, [this, report_box]() {
        entity_set_store_->replaceCurrent(selection_service_->selectionIdsAsVector());
        report_box();
    });
    auto* add_to_box = components::buttons::makeButton("Add", ":/icons/box-plus.svg", this);
    connect(add_to_box, &QToolButton::clicked, this, [this, report_box]() {
        entity_set_store_->addToCurrent(selection_service_->selectionIdsAsVector());
        report_box();
    });
    auto* remove_from_box = components::buttons::makeButton("Remove", ":/icons/box-minus.svg", this);
    connect(remove_from_box, &QToolButton::clicked, this, [this, report_box]() {
        entity_set_store_->removeFromCurrent(selection_service_->selectionIdsAsVector());
        report_box();
    });
    auto* isolate_box = components::buttons::makeButton("Isolate", ":/icons/box-eye.svg", this);
    connect(isolate_box, &QToolButton::clicked, this, [this]() {
        const auto& current = entity_set_store_->current();
        if (current.empty()) {
            session_state_->setStatusMessage("Box", "The box is empty");
            return;
        }
        action_pipeline_->run(current, bonsaiviewer::services::ActionPipeline::Action::Isolate);
    });
    auto* hide_box = components::buttons::makeButton("Hide", ":/icons/box-eye-off.svg", this);
    connect(hide_box, &QToolButton::clicked, this, [this]() {
        const auto& current = entity_set_store_->current();
        if (current.empty()) {
            session_state_->setStatusMessage("Box", "The box is empty");
            return;
        }
        action_pipeline_->run(current, bonsaiviewer::services::ActionPipeline::Action::RemoveFromView);
    });
    // Box Storage: save the current box to a slot, or isolate the view
    // on a saved slot without touching the current box.
    auto* box_storage = components::buttons::makeButton("Box Storage", ":/icons/floppy-disk.svg", this);
    box_storage->setPopupMode(QToolButton::InstantPopup);
    auto* box_storage_menu = new QMenu(box_storage);
    for (int slot = 1; slot <= bonsaiviewer::services::EntitySetStore::kSlotCount; ++slot) {
        box_storage_menu->addAction(
            QStringLiteral("Save to Slot %1").arg(slot), this, [this, slot, report_box]() {
                entity_set_store_->saveSlot(slot);
                report_box();
            });
    }
    box_storage_menu->addSeparator();
    for (int slot = 1; slot <= bonsaiviewer::services::EntitySetStore::kSlotCount; ++slot) {
        box_storage_menu->addAction(
            QStringLiteral("Isolate Slot %1").arg(slot), this, [this, slot]() {
                const auto& slot_ids = entity_set_store_->slot(slot);
                if (slot_ids.empty()) {
                    session_state_->setStatusMessage(
                        "Box", QStringLiteral("Slot %1 is empty").arg(slot));
                    return;
                }
                action_pipeline_->run(slot_ids,
                                      bonsaiviewer::services::ActionPipeline::Action::Isolate);
            });
    }
    box_storage->setMenu(box_storage_menu);


    // TI02 Colour group (B04): paint, restore the baked colour (opacity
    // kept), and the transparency presets.
    auto* paint = components::buttons::makeButton("Paint", ":/icons/paint-bucket.svg", this);
    connect(paint, &QToolButton::clicked, this, [this]() {
        const QColor colour = QColorDialog::getColor(Qt::white, this, QStringLiteral("Paint"));
        if (!colour.isValid()) return;
        appearance_service_->setColour(selection_service_->selectionIdsAsVector(), colour);
    });
    auto* remove_colour = components::buttons::makeButton("Remove Colour", ":/icons/paint-bucket-off.svg", this);
    connect(remove_colour, &QToolButton::clicked, this, [this]() {
        appearance_service_->removeColour(selection_service_->selectionIdsAsVector());
    });
    auto* transparency = components::buttons::makeButton("Transparency", ":/icons/cube-bandage.svg", this);
    connect(transparency, &QToolButton::clicked, this, [this]() {
        // The spec's preset dialog: 100/75/50/25 % opacity, 100 restores.
        components::Dialog dialog(this);
        auto* presets = new QWidget(&dialog);
        auto* preset_layout = new QHBoxLayout(presets);
        for (int percent : {100, 75, 50, 25}) {
            auto* preset_button = new QPushButton(QString::number(percent) + QStringLiteral("%"), &dialog);
            connect(preset_button, &QPushButton::clicked, &dialog, [this, percent, &dialog]() {
                appearance_service_->setOpacity(
                    selection_service_->selectionIdsAsVector(), percent);
                dialog.accept();
            });
            preset_layout->addWidget(preset_button);
        }
        dialog.addBodyWidget(presets);
        dialog.exec();
    });

    // TI05 ZOOM group (TI02's Fit View and Perpendicular View).
    auto* fit_view = components::buttons::makeButton("Fit View", ":/icons/cube-scan.svg", this);
    connect(fit_view, &QToolButton::clicked, this, [this]() {
        camera_service_->fitView();
    });
    auto* perpendicular_view = components::buttons::makeButton("Perpendicular", ":/icons/perspective-view.svg", this);
    perpendicular_view->setCheckable(true);
    connect(perpendicular_view, &QToolButton::toggled, this, [this](bool checked) {
        perpendicular_mode_ = checked;
        if (!viewport_widget_) return;
        if (checked) viewport_widget_->viewport()->setCursor(Qt::CrossCursor);
        else viewport_widget_->viewport()->unsetCursor();
        session_state_->setStatusMessage(
            "Orientation",
            checked ? "Perpendicular view armed - left click a surface"
                    : QStringLiteral("Orientation"));
    });

    // BonsaiViewer31: the buttons above, bound to their shortcuts. A shortcut
    // runs the button's own handler, so both paths stay identical.
    bindShortcut(this, "view.hide", [hide_selected]() { hide_selected->click(); });
    bindShortcut(this, "view.show", [show_selected]() { show_selected->click(); });
    bindShortcut(this, "view.show_all", [show_all]() { show_all->click(); });
    bindShortcut(this, "view.invert", [invert_selection]() { invert_selection->click(); });
    bindShortcut(this, "view.restore", [restore_view]() { restore_view->click(); });
    bindShortcut(this, "select.all", [select_in_view]() { select_in_view->click(); });
    bindShortcut(this, "color.paint", [paint]() { paint->click(); });
    bindShortcut(this, "color.remove", [remove_colour]() { remove_colour->click(); });
    bindShortcut(this, "color.transparency", [transparency]() { transparency->click(); });
    bindShortcut(this, "box.set", [selection_to_box]() { selection_to_box->click(); });
    bindShortcut(this, "box.add", [add_to_box]() { add_to_box->click(); });
    bindShortcut(this, "box.remove", [remove_from_box]() { remove_from_box->click(); });
    bindShortcut(this, "box.isolate", [isolate_box]() { isolate_box->click(); });
    bindShortcut(this, "box.hide", [hide_box]() { hide_box->click(); });
    bindShortcut(this, "view.fit", [fit_view]() { fit_view->click(); });
    bindShortcut(this, "view.perpendicular", [perpendicular_view]() { perpendicular_view->click(); });
    // Toggle Visibility (BonsaiViewer31): the selection is hidden when all of
    // it is shown, shown otherwise.
    bindShortcut(this, "view.toggle_visibility", [this]() {
        const auto ids = selection_service_->selectionIdsAsVector();
        if (ids.empty()) return;
        bool all_visible = true;
        for (const auto id : ids) {
            if (!active_view_->isVisible(id)) {
                all_visible = false;
                break;
            }
        }
        if (all_visible) active_view_->remove(ids);
        else active_view_->add(ids);
    });

    // Spec 02 section controls (user-placed in the generic band): arm
    // the tool, remove every plane, hide the gizmos (cuts stay),
    // suspend (gizmos and cuts both stop). The toggles are checkable so
    // their state is visible, per the spec.
    auto* section_mode = components::buttons::makeButton("Add", ":/icons/cube-cut-with-curve.svg", this);
    section_mode->setCheckable(true);
    connect(section_mode, &QToolButton::toggled, this, [this](bool checked) {
        // Guarded: the sectionToolChanged sync below also sets the
        // button, and the tool can be toggled from the K shortcut or
        // Esc — only push the user's click when it changes the state.
        if (checked != viewport_widget_->viewport()->sectionToolActive()) {
            modules::viewport::commands::toggleSection(*session_state_,
                                                       *viewport_widget_->viewport());
        }
    });
    // Spec 02: arming the section tool turns the cursor into a knife
    // (the section icon as a custom cursor — no dedicated knife asset);
    // the one-shot auto-deactivation restores it after the plane is
    // created.
    connect(viewport_widget_->viewport(), &ViewportWindow::sectionToolChanged,
            this, [this, section_mode](bool active) {
        section_mode->blockSignals(true);
        section_mode->setChecked(active);
        section_mode->blockSignals(false);
        if (active) {
            const QPixmap knife =
                QIcon(QStringLiteral(":/icons/cube-cut-with-curve.svg"))
                    .pixmap(QSize(24, 24));
            viewport_widget_->viewport()->setCursor(QCursor(knife, 8, 8));
        } else {
            viewport_widget_->viewport()->unsetCursor();
        }
    });
    // Remove / Hide / Suspend gathered under one "Modify" dropdown so the
    // SECTION group stays two buttons wide; the toggles stay checkable
    // in the menu so their state remains visible.
    auto* modify_section = components::buttons::makeButton("Modify", ":/icons/settings.svg", this);
    modify_section->setPopupMode(QToolButton::InstantPopup);
    auto* modify_menu = new QMenu(modify_section);
    modify_menu->addAction(QStringLiteral("Remove Section"), this, [this]() {
        section_manager_->clear();
    });
    auto* hide_action = modify_menu->addAction(QStringLiteral("Hide Section"));
    hide_action->setCheckable(true);
    connect(hide_action, &QAction::toggled, this, [this](bool checked) {
        section_manager_->setGizmosHidden(checked);
    });
    auto* suspend_action = modify_menu->addAction(QStringLiteral("Suspend Section"));
    suspend_action->setCheckable(true);
    connect(suspend_action, &QAction::toggled, this, [this](bool checked) {
        section_manager_->setSuspended(checked);
    });
    modify_section->setMenu(modify_menu);

    components::buttons::addButtonGroups(row, {
        components::buttons::makeButtonGroup("SELECTION", {hide_selected, show_selected, show_all, invert_selection, select_in_view, restore_view}, this),
        components::buttons::makeButtonGroup("CATEGORIES", {categories}, this),
        components::buttons::makeButtonGroup("SECTION", {section_mode, modify_section}, this),
    });
    // Box centred in the band: stretches on both sides push it to the
    // middle while Zoom and Colour justify to the right edge.
    row->addStretch(1);
    components::buttons::addButtonGroups(row, {
        components::buttons::makeButtonGroup("BOX", {selection_to_box, add_to_box, remove_from_box, isolate_box, hide_box, box_storage}, this),
    });
    row->addStretch(1);
    components::buttons::addButtonGroups(row, {
        components::buttons::makeButtonGroup("ZOOM", {fit_view, perpendicular_view}, this),
        components::buttons::makeButtonGroup("COLOUR", {paint, remove_colour, transparency}, this),
    });

    // Hover: the name in bold and a very short description. Set before the
    // labels are removed below.
    const auto describe = [](QToolButton* button, const QString& description) {
        button->setToolTip(QStringLiteral("<b>%1</b><br>%2").arg(button->text(), description));
    };
    describe(hide_selected, "Hide the selected objects.");
    describe(show_selected, "Show the selected objects.");
    describe(show_all, "Show all the hidden objects.");
    describe(invert_selection, "Swap the visible and hidden objects.");
    describe(select_in_view, "Select all the visible objects.");
    describe(restore_view, "Reset colours, hidden models and hidden objects.");
    describe(categories, "Show or hide IFC categories.");
    describe(selection_to_box, "Replace the box with the selection.");
    describe(add_to_box, "Add the selection to the box.");
    describe(remove_from_box, "Remove the selection from the box.");
    describe(isolate_box, "Show only the objects of the box.");
    describe(hide_box, "Hide the objects of the box.");
    describe(box_storage, "Save the box to a slot, or isolate a slot.");
    describe(paint, "Colour the selected objects.");
    describe(remove_colour, "Remove the colour of the selected objects.");
    describe(transparency, "Set the opacity of the selected objects.");
    describe(fit_view, "Frame the whole model.");
    describe(perpendicular_view, "Arm the view perpendicular to a surface.");
    describe(section_mode, "Add a section plane.");
    describe(modify_section, "Remove, hide or suspend the section.");

    // Icon only: the name and description are in the hover tooltip.
    for (int index = 0; index < row->count(); ++index) {
        if (QWidget* group = row->itemAt(index)->widget()) {
            for (QToolButton* button : group->findChildren<QToolButton*>()) {
                button->setToolButtonStyle(Qt::ToolButtonIconOnly);
                button->setText(QString());
            }
        }
    }
}

QWidget* MainWindow::buildInspectRibbonPage() {
    auto* page = new QFrame(this);
    page->setObjectName("ribbonPage");
    auto* row = new QHBoxLayout(page);
    row->setContentsMargins(2, 4, 2, 4);
    row->setSpacing(0);

    auto* distance = components::buttons::makeButton("Distance", ":/icons/select-edge3d.svg", this);
    connect(distance, &QToolButton::clicked, this, [this]() {
        modules::viewport::commands::toggleDistance(*viewport_widget_->viewport());
    });
    auto* area = components::buttons::makeButton("Area", ":/icons/select-face3d.svg", this);
    connect(area, &QToolButton::clicked, this, [this]() {
        modules::viewport::commands::toggleArea(*viewport_widget_->viewport());
    });
    auto* volume = components::buttons::makeButton("Volume", ":/icons/select-point3d.svg", this);
    connect(volume, &QToolButton::clicked, this, [this]() {
        modules::viewport::commands::toggleVolume(*viewport_widget_->viewport());
    });

    components::buttons::addButtonGroups(row, {
        components::buttons::makeButtonGroup("MEASURE", {distance, area, volume}, this),
    });
    row->addStretch(1);
    return page;
}

void MainWindow::setupRibbon() {
    auto* shell = new QFrame(this);

    auto* shell_layout = new QVBoxLayout(shell);
    shell_layout->setContentsMargins(0, 0, 0, 0);
    shell_layout->setSpacing(0);

    // TI21 phase 1: the workspace tabs (Home, Filter, Table) and the Inspect
    // tab share one row. Workspace tabs are indexes 0 to 2, Inspect is 3.
    ribbon_tabs_ = new components::TabBar(shell);
    ribbon_tabs_->addTab("Home");
    ribbon_tabs_->addTab("Filter");
    ribbon_tabs_->addTab("Table");
    ribbon_tabs_->addTab("Inspect");
    ribbon_tabs_->setCurrentIndex(0);

    auto* ribbon_band = new QFrame(shell);
    ribbon_band->setObjectName("ribbonBand");
    auto* band_layout = new QVBoxLayout(ribbon_band);
    band_layout->setContentsMargins(0, 0, 0, 0);
    band_layout->setSpacing(0);

    ribbon_pages_ = new QStackedWidget(ribbon_band);
    // Workspaces: no ribbon content, the band is hidden on these tabs.
    for (int i = 0; i < 3; ++i) {
        ribbon_pages_->addWidget(new QWidget(ribbon_pages_));
    }
    ribbon_pages_->addWidget(buildInspectRibbonPage());

    band_layout->addWidget(ribbon_pages_);
    // TI01: the menu bar sits above the tabs ribbon (spec 05). The shell
    // replaces the QMainWindow's own menu area, so the QMenuBar holding
    // the File / Display / Window / Help menus becomes its first row.
    shell_layout->addWidget(menuBar());
    shell_layout->addWidget(ribbon_tabs_);
    // TI05: the generic top panel below the tabs bar — search bar in the
    // left corner, then the display / box / zoom / colour groups. Always
    // visible whatever the ribbon tab.
    auto* generic_panel = new components::GenericTopPanel(shell);
    auto* search_bar = new modules::searchbar::SearchBar(query_engine_,
                                                         action_pipeline_, generic_panel);
    generic_panel->setSearchBar(search_bar);
    bindShortcut(this, "search.focus", [search_bar]() { search_bar->focusSearch(); });
    populateGenericTopPanel(generic_panel);
    generic_panel->makeCompact();
    // TI05: one specific top panel per tab, switched with the ribbon.
    // Empty until a feature registers its content.
    auto* specific_panel = new components::SpecificTopPanel(shell);
    specific_panel->addTab(QStringLiteral("Home"));
    specific_panel->addTab(QStringLiteral("Filter"));
    specific_panel->addTab(QStringLiteral("Table"));
    specific_panel->addTab(QStringLiteral("Inspect"));
    connect(ribbon_tabs_, &components::TabBar::currentChanged,
            specific_panel, &components::SpecificTopPanel::setCurrentIndex);
    // Order below the tabs: the tab ribbon, then the specific panel.
    shell_layout->addWidget(ribbon_band);
    shell_layout->addWidget(specific_panel);
    // Spec 05: the generic panel (Selection, Categories, Section, Box,
    // Zoom, Colour) sits at the top of the 3D view, always reachable.
    viewport_area_layout_->insertWidget(0, generic_panel);

    connect(ribbon_tabs_, &components::TabBar::currentChanged,
            ribbon_pages_, &QStackedWidget::setCurrentIndex);
    // The ribbon band is shown on Inspect only (index 3).
    connect(ribbon_tabs_, &components::TabBar::currentChanged, ribbon_band,
            [ribbon_band](int index) { ribbon_band->setVisible(index == 3); });
    ribbon_band->setVisible(false); // Home is the first tab, shown at start-up

    setMenuWidget(shell);
    setupWorkspaces();
}

// TI21 phase 1: three workspaces, each one a PanelManager tab with its own
// dock layout. The default layout of a workspace is applied on its first
// visit; afterwards PanelManager restores what the user left.
void MainWindow::setupWorkspaces() {
    for (const QString& name : QStringList{QStringLiteral("Home"),
                                           QStringLiteral("Filter"),
                                           QStringLiteral("Table")}) {
        workspace_ids_.append(panel_manager_->addTab(name));
    }
    // Home is the start-up workspace: the shell already placed its panels,
    // so its defaults are applied in place.
    panel_manager_->switchTab(workspace_ids_.at(0));
    applyWorkspaceDefaults(0);
    workspace_defaults_applied_.append(workspace_ids_.at(0));
    connect(ribbon_tabs_, &components::TabBar::currentChanged,
            this, &MainWindow::onWorkspaceChanged);
}

void MainWindow::onWorkspaceChanged(int index) {
    if (index < 0 || index >= workspace_ids_.size()) return;
    const QString tab_id = workspace_ids_.at(index);
    panel_manager_->switchTab(tab_id);
    if (!workspace_defaults_applied_.contains(tab_id)) {
        applyWorkspaceDefaults(index);
        workspace_defaults_applied_.append(tab_id);
    }
}

// Default panels of each workspace (BonsaiViewer21, phase 1).
// Home: Models, Spatial Hierarchy, Ifc Class Tree, Properties, 3D view.
// Filter: Home plus Filters, under the Spatial Hierarchy.
// Table: Home plus Tables, under the Spatial Hierarchy, and the Table
// with the Ifc Text Digger at the bottom.
void MainWindow::applyWorkspaceDefaults(int index) {
    QStringList visible{QStringLiteral("models"), QStringLiteral("spatial"),
                        QStringLiteral("classtree"), QStringLiteral("properties")};
    if (index == 1) {
        // Filter: no Ifc Class Tree in this workspace.
        visible.removeAll(QStringLiteral("classtree"));
        addDockWidget(Qt::LeftDockWidgetArea, filters_panel_);
        splitDockWidget(spatial_panel_, filters_panel_, Qt::Vertical);
        visible << QStringLiteral("filters");
    } else if (index == 2) {
        addDockWidget(Qt::LeftDockWidgetArea, tables_panel_);
        splitDockWidget(spatial_panel_, tables_panel_, Qt::Vertical);
        visible << QStringLiteral("tables") << QStringLiteral("sheets")
                << QStringLiteral("text_digger");
    }
    for (const QString& id : panel_manager_->registeredPanels()) {
        panel_manager_->setPanelVisible(id, visible.contains(id));
    }
}

void MainWindow::setupViewport() {
    // The central area holds the generic panel (added by setupRibbon) above
    // the 3D view. Not an overlay: the viewport is a native window.
    auto* area = new QWidget(this);
    viewport_area_layout_ = new QVBoxLayout(area);
    viewport_area_layout_->setContentsMargins(0, 0, 0, 0);
    viewport_area_layout_->setSpacing(0);
    viewport_widget_ = new modules::viewport::ViewportPanel(area);
    viewport_area_layout_->addWidget(viewport_widget_, 1);
    setCentralWidget(area);
}

void MainWindow::setupPanels() {
    models_panel_ = new modules::models::ModelsPanel(
        session_state_, viewport_widget_->viewport(), this);
    spatial_panel_ = new modules::spatial_hierarchy::SpatialHierarchyPanel(this);
    properties_panel_ = new modules::properties::PropertiesPanel(this);

    models_view_ = new modules::models::ModelsPanelView(
        models_panel_, session_state_, viewport_widget_->viewport(), this);
    spatial_view_ = new modules::spatial_hierarchy::SpatialHierarchyPanelView(
        spatial_panel_, session_state_, viewport_widget_->viewport(), this);

    layers_panel_ = new components::Panel("Layers", new modules::todo::TodoPanel("Layers", this), this);
    stored_views_panel_ = new components::Panel(
        "Stored Views", new modules::todo::TodoPanel("Stored Views", this), this);
    search_panel_ = new components::Panel(
        "Search and Query", new modules::todo::TodoPanel("Search and Query", this), this);
    filters_panel_ = new modules::filters::FiltersPanel(this);
    table_panel_ = new modules::table::TablePanel(this);
    text_digger_panel_ = new modules::text_digger::TextDiggerPanel(this);
    tables_panel_ = new modules::table::TablesPanel(this);
    audit_panel_ = new components::Panel("Audit", new modules::todo::TodoPanel("Audit", this), this);
    clash_panel_ = new components::Panel("Clash", new modules::todo::TodoPanel("Clash", this), this);
    issues_panel_ = new components::Panel("Issues", new modules::todo::TodoPanel("Issues", this), this);
    class_tree_widget_ = new QTreeWidget(this);
    class_tree_panel_ = new components::Panel("Ifc Class Tree", class_tree_widget_, this);

    addDockWidget(Qt::LeftDockWidgetArea, models_panel_);
    addDockWidget(Qt::LeftDockWidgetArea, spatial_panel_);
    splitDockWidget(models_panel_, spatial_panel_, Qt::Vertical);
    // TI12: the Ifc Class Tree is tabbed with the Spatial Hierarchy, shown at start-up.
    addDockWidget(Qt::LeftDockWidgetArea, class_tree_panel_);
    tabifyDockWidget(spatial_panel_, class_tree_panel_);
    spatial_panel_->raise();

    addDockWidget(Qt::RightDockWidgetArea, properties_panel_);
    addDockWidget(Qt::RightDockWidgetArea, layers_panel_);
    addDockWidget(Qt::RightDockWidgetArea, stored_views_panel_);
    addDockWidget(Qt::RightDockWidgetArea, search_panel_);
    addDockWidget(Qt::RightDockWidgetArea, audit_panel_);
    addDockWidget(Qt::RightDockWidgetArea, clash_panel_);
    addDockWidget(Qt::RightDockWidgetArea, issues_panel_);
    addDockWidget(Qt::BottomDockWidgetArea, table_panel_);
    // BonsaiViewer16: the Ifc Text Digger, in the bottom tabs with the Table.
    addDockWidget(Qt::BottomDockWidgetArea, text_digger_panel_);
    tabifyDockWidget(table_panel_, text_digger_panel_);
    // Tables: on the left, tabbed with Models, and shown at start-up.
    addDockWidget(Qt::LeftDockWidgetArea, tables_panel_);
    tabifyDockWidget(models_panel_, tables_panel_);

    tabifyDockWidget(properties_panel_, layers_panel_);
    tabifyDockWidget(layers_panel_, stored_views_panel_);
    tabifyDockWidget(stored_views_panel_, search_panel_);
    tabifyDockWidget(search_panel_, filters_panel_);
    tabifyDockWidget(filters_panel_, audit_panel_);
    tabifyDockWidget(audit_panel_, clash_panel_);
    tabifyDockWidget(clash_panel_, issues_panel_);
    properties_panel_->raise();

    layers_panel_->hide();
    stored_views_panel_->hide();
    search_panel_->hide();
    filters_panel_->hide();
    table_panel_->hide();
    text_digger_panel_->hide();
    tables_panel_->raise();
    audit_panel_->hide();
    clash_panel_->hide();
    issues_panel_->hide();

    resizeDocks({models_panel_, properties_panel_}, {290, 330}, Qt::Horizontal);
    resizeDocks({models_panel_, spatial_panel_}, {280, 240}, Qt::Vertical);
}

void MainWindow::setupStatus() {
    status_mode_label_ = new QLabel("Ready", this);
    status_selection_label_ = new QLabel("No selection", this);
    status_perf_label_ = new QLabel(this);
    status_memory_label_ = new QLabel(this);
    status_memory_label_->setVisible(false);
    status_memory_label_->setToolTip(
        "The geometry in view needs more GPU memory than is available, so the "
        "viewer keeps the largest on-screen parts resident and streams the rest "
        "as you move. Right-click a model in the Models panel and choose "
        "\"Unload Model\" to free its GPU memory for the others.");
    status_progress_bar_ = new QProgressBar(this);
    status_perf_label_->setVisible(AppSettings::instance().showStats());
    status_progress_bar_->setMaximumWidth(200);
    status_progress_bar_->setVisible(false);

    statusBar()->setSizeGripEnabled(false);
    statusBar()->addWidget(status_mode_label_);
    statusBar()->addWidget(status_selection_label_, 1);
    statusBar()->addPermanentWidget(status_memory_label_);
    statusBar()->addPermanentWidget(status_perf_label_);
    statusBar()->addPermanentWidget(status_progress_bar_);

    connect(&AppSettings::instance(), &AppSettings::showStatsChanged, this, [this](bool show) {
        status_perf_label_->setVisible(show);
        if (!show) status_perf_label_->clear();
    });

    // Nav mouse preset. The WGPU_NAV_PRESET env var is a dev override applied at
    // ViewportWindow construction; otherwise apply the persisted Settings choice
    // here, and re-apply live whenever the user changes it in the dialog.
    if (!std::getenv("WGPU_NAV_PRESET")) {
        if (auto* vp = viewport_widget_->viewport())
            vp->applyNavPreset(AppSettings::navPresetName(AppSettings::instance().navPreset()));
    }
    connect(&AppSettings::instance(), &AppSettings::navPresetChanged, this,
            [this](AppSettings::NavPreset preset) {
                if (auto* vp = viewport_widget_->viewport())
                    vp->applyNavPreset(AppSettings::navPresetName(preset));
            });

    // Backface culling: apply the persisted choice and re-apply live on change.
    if (auto* vp = viewport_widget_->viewport())
        vp->setBackfaceCulling(AppSettings::instance().backfaceCulling());
    connect(&AppSettings::instance(), &AppSettings::backfaceCullingChanged, this,
            [this](bool enabled) {
                if (auto* vp = viewport_widget_->viewport())
                    vp->setBackfaceCulling(enabled);
            });

    connect(session_state_, &bonsaiviewer::SessionState::statusMessageChanged,
            this, [this](const QString& mode, const QString& detail) {
        status_mode_label_->setText(mode);
        status_selection_label_->setText(detail);
    });
    connect(session_state_, &bonsaiviewer::SessionState::progressBegan, this, [this](const QString&) {
        // Start in indeterminate mode (spinning bar). The first concrete
        // setProgress(...) call below switches it to a determinate 0-100 bar.
        status_progress_bar_->setRange(0, 0);
        status_progress_bar_->setVisible(true);
    });
    connect(session_state_, &bonsaiviewer::SessionState::progressChanged, this, [this](int percent) {
        if (status_progress_bar_->maximum() == 0) status_progress_bar_->setRange(0, 100);
        status_progress_bar_->setValue(percent);
    });
    connect(session_state_, &bonsaiviewer::SessionState::progressEnded, this, [this]() {
        status_progress_bar_->setVisible(false);
    });
    session_state_->setStatusMessage("Ready", "No selection");
}

void MainWindow::setupLoader() {
    session_state_->createLoader(viewport_widget_->viewport());

    // TI00 phase 1 services. B01/B02/B03 drive the viewport and the
    // element registry; B05's index is (re)built as each model's geometry
    // becomes ready, and the category filter re-applies there too so a
    // freshly loaded model cannot resurrect a disabled category.
    selection_service_ = new bonsaiviewer::services::SelectionService(
        *session_state_, *viewport_widget_->viewport(), this);
    category_filter_ = new bonsaiviewer::services::CategoryOverrideFilter(
        *session_state_, *viewport_widget_->viewport(), this);
    active_view_ = new bonsaiviewer::services::ActiveView(
        *session_state_, *viewport_widget_->viewport(), *category_filter_);
    property_index_ = new bonsaiviewer::services::PropertyIndex(
        *session_state_->elementRegistry(), this);
    connect(session_state_, &bonsaiviewer::SessionState::modelGeometryReady, this,
            [this](uint32_t session_model_id) {
                property_index_->indexModel(session_model_id);
                category_filter_->applyToViewport();
            });
    connect(session_state_, &bonsaiviewer::SessionState::federationChanged, this,
            [this]() { category_filter_->applyToViewport(); });

    // TI02 display-function services: B05's read path, B04 colour /
    // opacity, B10 fit view / perpendicular framing, B09 picking and B11
    // section toggles. All the display mutations below route through them.
    data_access_ = new bonsaiviewer::services::IfcDataAccess(
        *session_state_->elementRegistry());
    appearance_service_ = new bonsaiviewer::services::AppearanceService(
        *session_state_, *viewport_widget_->viewport(), this);
    // BonsaiViewer02 / 29: the Models panel's Colour, Isolate and Select Model
    // actions need these services; they exist only from this point on.
    if (models_view_ != nullptr) {
        models_view_->setAppearanceService(appearance_service_);
        models_view_->setModelServices(active_view_, selection_service_);
    }
    camera_service_ = new bonsaiviewer::services::CameraService(
        *session_state_, *viewport_widget_->viewport(), this);
    picking_service_ = new bonsaiviewer::services::PickingService(
        *viewport_widget_->viewport(), this);
    section_manager_ = new bonsaiviewer::services::SectionPlaneManager(
        *viewport_widget_->viewport(), this);
    // TI03: the search path — B07 (the -box scope), B06 (prefix grammar
    // evaluated against B05) and B08 (isolate / -add / -rm).
    entity_set_store_ = new bonsaiviewer::services::EntitySetStore(*session_state_, this);
    query_engine_ = new bonsaiviewer::services::QueryEngine(
        *session_state_, *property_index_, *data_access_);
    query_engine_->setActiveView(active_view_);
    query_engine_->setEntitySetStore(entity_set_store_);
    action_pipeline_ = new bonsaiviewer::services::ActionPipeline(
        *active_view_, *appearance_service_, *data_access_);
    // TI06 filter rules: the IfcTester IDS bridge, the persisted filter
    // tree (loads filters.json through B14) and the layer-semantics runner.
    ids_bridge_ = new bonsaiviewer::services::IdsBridge(this);
    filter_store_ = new bonsaiviewer::services::FilterStore(this);
    filter_runner_ = new bonsaiviewer::services::FilterRunner(
        *session_state_, *ids_bridge_, *action_pipeline_, *entity_set_store_, this);
    // BonsaiViewer27: CTRL+Z. The display services record their state before
    // each user action; the filter runner suspends recording while it runs.
    undo_stack_ = new bonsaiviewer::services::UndoStack(*active_view_, *appearance_service_);
    active_view_->setUndoStack(undo_stack_);
    appearance_service_->setUndoStack(undo_stack_);
    filter_runner_->setUndoStack(undo_stack_);
    connect(session_state_, &bonsaiviewer::SessionState::projectReset, this,
            [this]() { undo_stack_->clear(); });
    filters_view_ = new modules::filters::FiltersPanelView(
        filters_panel_, *session_state_, *filter_store_, *filter_runner_, *ids_bridge_, this);
    filters_view_->setActionPipeline(action_pipeline_);
    // TI07 table: the property catalog of all the loaded models (BonsaiViewer26)
    // and the table panel, evaluated through the same IDS bridge.
    property_catalog_ = new bonsaiviewer::services::PropertyCatalog(*session_state_, *data_access_, this);
    // TI07 phase 2: the saved tables (one JSON document) and their Tables panel.
    table_store_ = new modules::table::TableStore(this);
    table_view_ = new modules::table::TablePanelView(
        table_panel_, *session_state_, *ids_bridge_, *data_access_, *property_catalog_,
        *selection_service_, *action_pipeline_, *table_store_, *appearance_service_, *entity_set_store_, this);
    tables_view_ = new modules::table::TablesView(tables_panel_, *table_store_, this);
    // TI14: Select By Property, the right-click functions of the Properties panel.
    // Built here, once the services it uses exist; "Colour by property" shows its table here.
    properties_view_ = new modules::properties::PropertiesPanelView(
        properties_panel_, session_state_, *data_access_, *action_pipeline_, *selection_service_,
        *entity_set_store_, this);
    properties_view_->setTableView(table_view_);
    // BonsaiViewer16: the Ifc Text Digger reads the selection and the IFC text files.
    text_digger_view_ = new modules::text_digger::TextDiggerView(
        text_digger_panel_, *session_state_, *selection_service_, this);
    // TI12: the Ifc Class Tree (B12 tree over the open models' IFC classes).
    auto* class_tree_data = new modules::class_tree::ClassTreeDataSource(
        *session_state_, *selection_service_, *action_pipeline_, this);
    auto* class_tree = new bonsaiviewer::services::tree::TreePanelWidget(class_tree_widget_, class_tree_data, this);
    class_tree->refresh();
    // Several nodes at once (Ctrl / Shift); the viewport selection follows the tree selection.
    class_tree_widget_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    connect(class_tree_widget_, &QTreeWidget::itemSelectionChanged, class_tree_data, [this, class_tree_data]() {
        QStringList keys;
        for (QTreeWidgetItem* item : class_tree_widget_->selectedItems()) {
            keys << item->data(0, Qt::UserRole).toString();
        }
        class_tree_data->onSelectionChanged(keys);
    });
    connect(tables_view_, &modules::table::TablesView::openRequested,
            table_view_, &modules::table::TablePanelView::openTable);
    connect(tables_view_, &modules::table::TablesView::newTableRequested,
            table_view_, &modules::table::TablePanelView::newTable);
    // Perpendicular View (right click) and the section gizmo right-click
    // delete menu are viewport-level interactions owned by the shell.
    viewport_widget_->viewport()->installEventFilter(this);

    viewport_view_ = new modules::viewport::ViewportView(
        session_state_, viewport_widget_->viewport(), this);

    // BonsaiViewer22: IFC files and folders dropped on the Models panel.
    easy_load_view_ = new modules::easy_load::EasyLoadView(
        *session_state_, *viewport_widget_->viewport(), this, this);
    models_panel_->setFilesDropHandler([this](const QList<QUrl>& urls) {
        easy_load_view_->handleDrop(urls);
    });

    // Load errors surface through SessionState as a session-level signal; the
    // status text + progress are already cleared there, we only show the modal.
    connect(session_state_, &bonsaiviewer::SessionState::loadError, this,
            [this](const QString& message) {
        QMessageBox::warning(this, "Bonsai Viewer", message);
    });

    connect(viewport_widget_->viewport(), &ViewportWindow::frameStatsUpdated, this,
            [this](const ViewportWindow::FrameStats& stats) {
        const double mb = 1.0 / (1024.0 * 1024.0);

        // Missing chunks are normal for a moment after every camera move
        // while streaming catches up; only a shortfall that persists means
        // the view does not fit, and only that is worth telling the user.
        constexpr qint64 kShortfallNoticeMs = 3000;
        if (stats.chunks_wanted_missing == 0) {
            memory_shortfall_since_.invalidate();
            status_memory_label_->setVisible(false);
        } else {
            if (!memory_shortfall_since_.isValid()) memory_shortfall_since_.start();
            if (memory_shortfall_since_.elapsed() >= kShortfallNoticeMs) {
                status_memory_label_->setText(
                    QString("GPU memory full: %1 of %2 visible chunks (%3 MB) not loaded")
                        .arg(stats.chunks_wanted_missing)
                        .arg(stats.chunks_wanted)
                        .arg(double(stats.wanted_missing_bytes) * mb, 0, 'f', 0));
                status_memory_label_->setVisible(true);
            }
        }

        if (!status_perf_label_->isVisible()) return;
        QString text =
            QString("%1 fps | %2 ms | %3/%4 obj | %5/%6 tri | %7 draws | VRAM %8/%9 MB")
                .arg(stats.fps, 0, 'f', 1)
                .arg(stats.frame_time_ms, 0, 'f', 1)
                .arg(stats.visible_objects)
                .arg(stats.total_objects)
                .arg(stats.visible_triangles)
                .arg(stats.total_triangles)
                .arg(stats.gl_draw_calls)
                .arg(double(stats.vram_used_bytes) * mb, 0, 'f', 0)
                .arg(double(stats.vram_capacity_bytes) * mb, 0, 'f', 0);
        // The budget is where the pool may grow to; the pool can also sit
        // a sub-buffer above it (a release would undershoot). Show it
        // only when it tells the user something capacity does not.
        if (stats.vram_budget_bytes > 0
            && stats.vram_budget_bytes != stats.vram_capacity_bytes) {
            text += QString(" (budget %1)")
                .arg(double(stats.vram_budget_bytes) * mb, 0, 'f', 0);
        }
        // Device total is only known when a driver backend answered.
        if (stats.device_vram_total_bytes > 0) {
            text += QString(" | Device %1/%2 MB")
                .arg(double(stats.device_vram_used_bytes) * mb, 0, 'f', 0)
                .arg(double(stats.device_vram_total_bytes) * mb, 0, 'f', 0);
        }
        if (stats.chunks_wanted_missing > 0) {
            text += QString(" | %1/%2 chunks waiting")
                .arg(stats.chunks_wanted_missing)
                .arg(stats.chunks_wanted);
        }
        status_perf_label_->setText(text);
    });
    connect(viewport_widget_->viewport(), &ViewportWindow::objectPicked,
            this, [this](uint32_t object_id) {
        selection_service_->onViewportPicked(object_id);
    });
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event) {
    const bool is_viewport = watched == viewport_widget_->viewport();
    if (is_viewport && event->type() == QEvent::MouseButtonPress) {
        auto* mouse = static_cast<QMouseEvent*>(event);
        if (mouse->button() == Qt::LeftButton && perpendicular_mode_) {
            // TI02 Perpendicular View: the first left click retrieves
            // the surface normal, frames the view against it, and the
            // tool deactivates — cursor restored.
            const float dpr = viewport_widget_->viewport()->dpr();
            const int x_px = int(mouse->position().x() * dpr);
            const int y_px = int(mouse->position().y() * dpr);
            std::uint32_t object_id = 0;
            Eigen::Vector3f world_pos, world_normal;
            if (viewport_widget_->viewport()->pickSurfaceAt(
                    x_px, y_px, object_id, world_pos, world_normal)) {
                camera_service_->framePerpendicular(world_normal);
            }
            perpendicular_mode_ = false;
            viewport_widget_->viewport()->unsetCursor();
            return true;
        }
        if (mouse->button() == Qt::RightButton) {
            // TI02: right-click a plane gizmo → "Delete plane" (B15). Works
            // with or without the section tool armed — the tool one-shots
            // off after a plane was created; only a hit on a gizmo claims
            // the click, orbit elsewhere is untouched.
            const int hit = viewport_widget_->viewport()->hitTestSectionGizmo(
                int(mouse->position().x()), int(mouse->position().y()));
            if (hit >= 0) {
                bonsaiviewer::services::ContextMenuBuilder menu;
                menu.addItem({QStringLiteral("delete-plane"),
                              QStringLiteral("Delete plane"),
                              {},
                              {},
                              [this, hit]() {
                                  section_manager_->removePlaneAt(hit);
                              }});
                menu.exec(mouse->globalPosition().toPoint(), this);
                return true;
            }
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::setupMenuBar() {
    // TI01: the docks are already created by setupPanels; B13 adopts them
    // so the Window menu toggles the same widgets the Panels ribbon
    // toggles. The menu bar itself needs category_filter_ from
    // setupLoader, hence this runs after it.
    panel_manager_ = new bonsaiviewer::services::PanelManager(this, this);
    panel_manager_->adoptPanel(QStringLiteral("models"),
                               QStringLiteral("Models"), models_panel_);
    panel_manager_->adoptPanel(QStringLiteral("spatial"),
                               QStringLiteral("Spatial"), spatial_panel_);
    panel_manager_->adoptPanel(QStringLiteral("classtree"),
                               QStringLiteral("Ifc Class Tree"), class_tree_panel_);
    panel_manager_->adoptPanel(QStringLiteral("layers"),
                               QStringLiteral("Layers"), layers_panel_);
    panel_manager_->adoptPanel(QStringLiteral("properties"),
                               QStringLiteral("Properties"), properties_panel_);
    panel_manager_->adoptPanel(QStringLiteral("views"),
                               QStringLiteral("Views"), stored_views_panel_);
    panel_manager_->adoptPanel(QStringLiteral("search"),
                               QStringLiteral("Search"), search_panel_);
    panel_manager_->adoptPanel(QStringLiteral("filters"),
                               QStringLiteral("Filters"), filters_panel_);
    panel_manager_->adoptPanel(QStringLiteral("sheets"),
                               QStringLiteral("Table"), table_panel_);
    panel_manager_->adoptPanel(QStringLiteral("text_digger"),
                               QStringLiteral("Ifc Text Digger"), text_digger_panel_);
    panel_manager_->adoptPanel(QStringLiteral("tables"),
                               QStringLiteral("Tables"), tables_panel_);
    panel_manager_->adoptPanel(QStringLiteral("audit"),
                               QStringLiteral("Audit"), audit_panel_);
    panel_manager_->adoptPanel(QStringLiteral("clash"),
                               QStringLiteral("Clash"), clash_panel_);
    panel_manager_->adoptPanel(QStringLiteral("issues"),
                               QStringLiteral("Issues"), issues_panel_);

    menu_bar_ = new modules::menubar::MenuBar(
        this, session_state_, viewport_widget_->viewport(),
        category_filter_, panel_manager_, this);
    menu_bar_->setOpenRecentPopulator(
        [this](QMenu* menu) { populateRecentMenu(menu); });
}

void MainWindow::updateWindowTitle() {
    auto* federation = session_state_->federation();
    const QString project_path = federation->filePath();
    if (project_path.isEmpty() && federation->models().empty()) {
        setWindowTitle("Bonsai Viewer");
    } else if (project_path.isEmpty()) {
        setWindowTitle("untitled[*] - Bonsai Viewer");
    } else {
        setWindowTitle(QFileInfo(project_path).fileName() + "[*] - Bonsai Viewer");
    }
}

} // namespace bonsaiviewer::shell
