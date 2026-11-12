# This file was generated with the assistance of an AI coding tool.
################################################################################
#                                                                              #
# This file is part of Bonsai.                                                 #
#                                                                              #
# Bonsai is free software: you can redistribute it and/or modify               #
# it under the terms of the GNU General Public License as published by         #
# the Free Software Foundation, either version 3.0 of the License, or          #
# (at your option) any later version.                                          #
#                                                                              #
# Bonsai is distributed in the hope that it will be useful,                    #
# but WITHOUT ANY WARRANTY; without even the implied warranty of               #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                 #
# GNU General Public License for more details.                                 #
#                                                                              #
# You should have received a copy of the GNU General Public License            #
# along with this program. If not, see <http://www.gnu.org/licenses/>.         #
#                                                                              #
################################################################################

message("Running CMakeLists.txt in /src/bonsaiviewer")

set(QT_VERSION 6 CACHE STRING "Qt version")
# Qt6::CorePrivate is exposed differently across Qt versions: Qt 6.8 ships the
# target inside Qt6Core (no CorePrivate config package), while Qt 6.10 provides
# it only as a separate CorePrivate package. OPTIONAL_COMPONENTS finds that
# package where it exists without failing where it does not; the Qt6::CorePrivate
# target ends up available either way for the link step below.
find_package(Qt${QT_VERSION} REQUIRED
             COMPONENTS Core Gui Widgets Svg
             OPTIONAL_COMPONENTS CorePrivate GuiPrivate
             PATHS ${QT_DIR})

set(BONSAIVIEWER_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ElementRegistry.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ElementRegistry.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ViewerSettings.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ViewerSettings.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SessionState.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SessionState.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MainWindow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MainWindow.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Measurement.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Measurement.h
    ${CMAKE_CURRENT_SOURCE_DIR}/components/SvgIcon.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/components/SvgIcon.h
    ${CMAKE_CURRENT_SOURCE_DIR}/components/Style.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/components/Style.h
    ${CMAKE_CURRENT_SOURCE_DIR}/components/Dialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/components/GenericTopPanel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/components/GenericTopPanel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/components/SpecificTopPanel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/components/SpecificTopPanel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/components/Dialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/components/Tabs.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/components/Tabs.h
    ${CMAKE_CURRENT_SOURCE_DIR}/components/KeyValueTable.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/components/KeyValueTable.h
    ${CMAKE_CURRENT_SOURCE_DIR}/components/Buttons.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/components/FlowLayout.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/components/FlowLayout.h
    ${CMAKE_CURRENT_SOURCE_DIR}/components/Buttons.h
    ${CMAKE_CURRENT_SOURCE_DIR}/components/Section.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/components/Section.h
    ${CMAKE_CURRENT_SOURCE_DIR}/components/Panel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/components/Panel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/components/FileDialogPaths.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/components/FileDialogPaths.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/easy_load/View.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/easy_load/View.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/connectors/Discovery.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/connectors/Discovery.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/connectors/PickerDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/connectors/PickerDialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/connectors/Process.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/connectors/Process.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/connectors/Registry.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/connectors/Registry.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/models/AddModelDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/models/AddModelDialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/models/SettingsDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/models/SettingsDialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/models/SettingsView.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/models/SettingsView.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/models/Types.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/models/Commands.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/models/Commands.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/models/FederationItemModel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/models/FederationItemModel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/models/Panel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/models/Panel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/models/View.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/models/View.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/todo/Panel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/todo/Panel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/properties/Types.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/properties/Panel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/properties/Panel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/properties/View.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/properties/View.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/project/Commands.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/project/Commands.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/project/RecentProjects.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/project/RecentProjects.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/project/SaveProjectDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/project/SaveProjectDialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/settings/Dialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/settings/Dialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/menubar/MenuBar.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/menubar/MenuBar.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/searchbar/SearchBar.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/searchbar/SearchBar.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/filters/Panel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/filters/Panel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/filters/View.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/filters/View.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/filters/CreationDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/filters/CreationDialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/filters/LayerDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/filters/LayerDialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/filters/FacetPages.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/filters/FacetPages.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/filters/FacetClipboard.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/filters/FacetClipboard.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/filters/SelectionEditor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/filters/SelectionEditor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/filters/ValueEditor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/filters/ValueEditor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/property_selection/PropertyPicker.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/property_selection/PropertyPicker.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/property_selection/PropertyField.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/property_selection/PropertyField.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/spatial_hierarchy/Types.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/spatial_hierarchy/Panel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/spatial_hierarchy/Panel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/spatial_hierarchy/View.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/spatial_hierarchy/View.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/viewport/Panel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/viewport/Panel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/viewport/Commands.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/viewport/Commands.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/viewport/View.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/viewport/View.h
    ${CMAKE_CURRENT_SOURCE_DIR}/services/ActiveView.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/services/ActiveView.h
    ${CMAKE_CURRENT_SOURCE_DIR}/services/AppearanceService.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/services/AppearanceService.h
    ${CMAKE_CURRENT_SOURCE_DIR}/services/UndoStack.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/services/UndoStack.h
    ${CMAKE_CURRENT_SOURCE_DIR}/services/ShortcutStore.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/services/ShortcutStore.h
    ${CMAKE_CURRENT_SOURCE_DIR}/services/UnifiedTreeBuilder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/services/UnifiedTreeBuilder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/services/ActionPipeline.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/services/ActionPipeline.h
    ${CMAKE_CURRENT_SOURCE_DIR}/services/CameraService.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/services/CameraService.h
    ${CMAKE_CURRENT_SOURCE_DIR}/services/CategoryOverrideFilter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/services/CategoryOverrideFilter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/services/ContextMenuBuilder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/services/ContextMenuBuilder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/services/EntitySetStore.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/services/EntitySetStore.h
    ${CMAKE_CURRENT_SOURCE_DIR}/services/FilterRunner.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/services/FilterRunner.h
    ${CMAKE_CURRENT_SOURCE_DIR}/services/FilterStore.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/services/FilterStore.h
    ${CMAKE_CURRENT_SOURCE_DIR}/services/FilterExamples.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/services/FilterExamples.h
    ${CMAKE_CURRENT_SOURCE_DIR}/services/RelationValues.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/services/RelationValues.h
    ${CMAKE_CURRENT_SOURCE_DIR}/services/ActiveFilters.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/services/ActiveFilters.h
    ${CMAKE_CURRENT_SOURCE_DIR}/services/BcfViewpoints.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/services/BcfViewpoints.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/bcf/Panel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/bcf/Panel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/bcf/View.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/bcf/View.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/bcf/TopicDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/bcf/TopicDialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/bcf/ElementListDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/bcf/ElementListDialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/bcf/ImageEditor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/bcf/ImageEditor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ports/bcf/BcfXml.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ports/bcf/BcfXml.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ports/bcf/Zip.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ports/bcf/Zip.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/bcf/Author.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ports/bcf/Bcf.h
    ${CMAKE_CURRENT_SOURCE_DIR}/services/IdsBridge.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/services/IdsBridge.h
    ${CMAKE_CURRENT_SOURCE_DIR}/services/IdsEngine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/services/IdsEngine.h
    ${CMAKE_CURRENT_SOURCE_DIR}/services/IdsPropertyChecks.h
    # BonsaiViewer34: C++ port of IfcDiff and the Diff panel.
    ${CMAKE_CURRENT_SOURCE_DIR}/services/IfcDiff/IfcDiff.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/services/IfcDiff/IfcDiff.h
    ${CMAKE_CURRENT_SOURCE_DIR}/services/IfcDiff/DiffMarks.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/diff/Panel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/diff/Panel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/diff/ReportDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/diff/ReportDialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/diff/View.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/diff/View.h
    # BonsaiViewer09: the IfcClash_Plus rule model and orchestration (TI09).
    ${CMAKE_CURRENT_SOURCE_DIR}/services/clash/RuleModel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/services/clash/RuleCatalogue.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/services/clash/RuleCatalogue.h
    ${CMAKE_CURRENT_SOURCE_DIR}/services/clash/ObjectClashIndex.h
    ${CMAKE_CURRENT_SOURCE_DIR}/services/clash/RuleJson.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/services/clash/RuleJson.h
    ${CMAKE_CURRENT_SOURCE_DIR}/services/clash/Orchestrator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/services/clash/Orchestrator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/rule/RuleDocument.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/rule/RuleDocument.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/rule/Editors.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/rule/Editors.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/rule/ObbModifications.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/rule/ObbModifications.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/rule/ObbModificationsEditor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/rule/ObbModificationsEditor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/rule/RuleTree.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/rule/RuleTree.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/rule/RulePanels.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/rule/RulePanels.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/rule/RuleEditDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/rule/RuleEditDialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/rule/SlotClipboard.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/rule/SlotClipboard.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/rule/ResultKey.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/rule/ResultKey.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/rule/ResultStore.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/rule/ResultStore.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/rule/Results.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/rule/Results.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/rule/Measures.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/rule/ClashView.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/rule/ClashView.h
    # C++ ports of ifcopenshell.util and IfcTester (BonsaiViewer24 IDS).
    ${CMAKE_CURRENT_SOURCE_DIR}/ports/ifcopenshell_util/Attr.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ports/ifcopenshell_util/Attr.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ports/ifcopenshell_util/Classification.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ports/ifcopenshell_util/Classification.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ports/ifcopenshell_util/Element.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ports/ifcopenshell_util/Element.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ports/ifcopenshell_util/Unit.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ports/ifcopenshell_util/Unit.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ports/ifctester/PyValue.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ports/ifctester/PyValue.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ports/ifctester/Restriction.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ports/ifctester/Restriction.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ports/ifctester/Facets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ports/ifctester/Facets.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ports/ifctester/Ids.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ports/ifctester/Ids.h
    ${CMAKE_CURRENT_SOURCE_DIR}/services/IfcDataAccess.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/services/IfcDataAccess.h
    ${CMAKE_CURRENT_SOURCE_DIR}/services/PersistenceService.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/services/PersistenceService.h
    ${CMAKE_CURRENT_SOURCE_DIR}/services/PickingService.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/services/PickingService.h
    ${CMAKE_CURRENT_SOURCE_DIR}/services/PanelManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/services/PanelManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/services/PopupService.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/services/PopupService.h
    ${CMAKE_CURRENT_SOURCE_DIR}/services/QueryEngine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/services/QueryEngine.h
    ${CMAKE_CURRENT_SOURCE_DIR}/services/SectionPlaneManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/services/SectionPlaneManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/services/SelectionService.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/services/SelectionService.h
    ${CMAKE_CURRENT_SOURCE_DIR}/services/StepTextService.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/services/StepTextService.h
    ${CMAKE_CURRENT_SOURCE_DIR}/services/TreePanelFramework.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/services/TreePanelFramework.h
    ${CMAKE_CURRENT_SOURCE_DIR}/services/PropertyCatalog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/services/PropertyCatalog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/table/ColumnDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/table/ColumnDialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/table/CreationDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/table/CreationDialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/table/Panel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/table/Panel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/table/Table.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/table/TableModel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/table/TableModel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/table/TableHeader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/table/TableHeader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/table/TableProxy.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/table/TableProxy.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/table/TableStore.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/table/TableStore.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/table/TableExamples.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/table/TableExamples.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/table/TablesPanel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/table/TablesPanel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/table/TablesView.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/table/TablesView.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/table/View.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/table/View.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/class_tree/ClassTreeDataSource.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/class_tree/ClassTreeDataSource.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/unified_tree/Panel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/unified_tree/Panel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/unified_tree/View.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/unified_tree/View.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/ids/Editor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/ids/Editor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/ids/IdsDocument.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/ids/IdsDocument.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/ids/View.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/ids/View.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/text_digger/Panel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/text_digger/Panel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/text_digger/View.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/text_digger/View.h
    ${CMAKE_CURRENT_SOURCE_DIR}/bonsaiviewer_resources.qrc
)

add_executable(BonsaiViewer ${BONSAIVIEWER_FILES})

# BonsaiViewer09: the IfcClash_Plus C++ engine (clash rules, TI09).
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ports/ifcclash_engine ${CMAKE_CURRENT_BINARY_DIR}/ifcclash_engine)
# The clash rules query the IfcOpenShell trees, run-time plug-ins loaded
# from the geometry library directory: build them with the viewer.
foreach(tree_target geometry_tree_opencascade_trianglebvh geometry_tree_opencascade_brep)
    if(TARGET ${tree_target})
        add_dependencies(BonsaiViewer ${tree_target})
    endif()
endforeach()

set_target_properties(BonsaiViewer PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)

# macOS bundle runtime layout. BonsaiViewer's exe has
# `LC_LOAD_DYLIB @rpath/libwgpu_native.dylib` (CMake bakes that in from
# the dylib's own install_name). Without an LC_RPATH entry pointing at
# the bundle's Frameworks/ dir, dyld aborts at launch with
# "no LC_RPATH's found". Set the standard macOS app-bundle rpath so
# the load resolves; the wgpu CMakeLists installs the dylib straight
# into BonsaiViewer.app/Contents/Frameworks for us.
if(APPLE)
    set_target_properties(BonsaiViewer PROPERTIES
        INSTALL_RPATH "@executable_path/../Frameworks"
    )
endif()

target_link_libraries(BonsaiViewer PRIVATE
    ifcclash_engine
    # IfcViewer is the unified scene + render lib (SceneLoader, Federation,
    # GeometryStreamer, IfcViewAssembler, ViewportWindow, OverlayRenderer,
    # BufferPool, …) since the wgpu/ifcviewer merge.
    IfcViewer
    plugin
    Qt${QT_VERSION}::Core
    Qt${QT_VERSION}::CorePrivate
    # BonsaiViewer10 (BCF): QZipReader / QZipWriter live in QtGui's private
    # module before Qt 6.5, in QtCore's after.
    $<TARGET_NAME_IF_EXISTS:Qt${QT_VERSION}::GuiPrivate>
    Qt${QT_VERSION}::Gui
    Qt${QT_VERSION}::Svg
    Qt${QT_VERSION}::Widgets
)
# Match the plugin library's boost::dll path type (plugin.h includes boost/dll).
target_compile_definitions(BonsaiViewer PRIVATE BOOST_DLL_USE_STD_FS)

# TI01 About popup: the version of the repo's VERSION file, so releases
# (the publish workflow reads the same file) and the menu agree.
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/../../VERSION" BONSAIVIEWER_VERSION_FILE)
string(STRIP "${BONSAIVIEWER_VERSION_FILE}" BONSAIVIEWER_VERSION_FILE)
target_compile_definitions(BonsaiViewer PRIVATE BONSAIVIEWER_VERSION="${BONSAIVIEWER_VERSION_FILE}")

# BUNDLE DESTINATION must be "." (install-prefix root) on macOS — Qt's
# deploy generator emits `macdeployqt BonsaiViewer.app` (no path
# prefix), which resolves only when the .app sits at the install root.
install(TARGETS BonsaiViewer
    EXPORT  ${IFCOPENSHELL_EXPORT_TARGETS}
    RUNTIME DESTINATION bin
    BUNDLE  DESTINATION .
)
# BonsaiViewer09: the unit tests of the OBB_Custom steps editor model (Qt Core
# only; not built by default): cmake --build <build> --target bonsaiviewer_obb_tests
add_executable(bonsaiviewer_obb_tests EXCLUDE_FROM_ALL
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/rule/ObbModifications.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_obb_rows.cpp)
target_include_directories(bonsaiviewer_obb_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(bonsaiviewer_obb_tests PRIVATE ifcclash_engine Qt${QT_VERSION}::Core)
add_test(NAME bonsaiviewer_obb_tests COMMAND bonsaiviewer_obb_tests)
# The key of the clash results (Qt Core only; not built by default).
add_executable(bonsaiviewer_result_key_tests EXCLUDE_FROM_ALL
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/rule/ResultKey.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_result_key.cpp)
target_include_directories(bonsaiviewer_result_key_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(bonsaiviewer_result_key_tests PRIVATE Qt${QT_VERSION}::Core)
add_test(NAME bonsaiviewer_result_key_tests COMMAND bonsaiviewer_result_key_tests)
# The clash section of a federation (Qt Core; Widgets for the headers only).
add_executable(bonsaiviewer_result_store_tests EXCLUDE_FROM_ALL
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/rule/ResultKey.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/rule/ResultStore.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_result_store.cpp)
target_include_directories(bonsaiviewer_result_store_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(bonsaiviewer_result_store_tests PRIVATE ifcclash_engine Qt${QT_VERSION}::Core Qt${QT_VERSION}::Widgets)
add_test(NAME bonsaiviewer_result_store_tests COMMAND bonsaiviewer_result_store_tests)

ifcopenshell_deploy_qt_runtime(BonsaiViewer)
ifcopenshell_stage_app_bundle_dylibs(BonsaiViewer)
