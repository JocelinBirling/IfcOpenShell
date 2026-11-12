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

# BonsaiViewer09 (TI09): the IfcClash_Plus C++ engine, copied from the
# IfcClash_Plus repository (Apache-2.0, see LICENSE and README.md), built as
# a static library against the viewer's IfcParse / IfcGeom / OpenCASCADE.

file(GLOB_RECURSE IFCCLASH_ENGINE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/api/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/pipeline/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/provider/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/registry/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rules/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/triang/*.cpp
)

add_library(ifcclash_engine STATIC ${IFCCLASH_ENGINE_SOURCES})
set_target_properties(ifcclash_engine PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(ifcclash_engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party)
# IfcViewer brings IfcParse, IfcGeom, OpenCASCADE, Eigen, Boost and zstd,
# with the compile definitions the IfcParse classes were built with.
target_link_libraries(ifcclash_engine PUBLIC IfcViewer)
target_compile_options(ifcclash_engine PRIVATE -Wno-unused-parameter)

# GEOS (the polygon operations of a few rules) is optional: without it those
# rules raise a ConfigError when they run.
find_path(GEOS_INCLUDE_DIR geos_c.h)
find_library(GEOS_C_LIBRARY NAMES geos_c)
if(GEOS_INCLUDE_DIR AND GEOS_C_LIBRARY)
    message(STATUS "ifcclash_engine: GEOS found (${GEOS_C_LIBRARY})")
    target_include_directories(ifcclash_engine PRIVATE ${GEOS_INCLUDE_DIR})
    target_link_libraries(ifcclash_engine PRIVATE ${GEOS_C_LIBRARY})
    target_compile_definitions(ifcclash_engine PRIVATE IFCCLASH_WITH_GEOS)
else()
    message(WARNING "ifcclash_engine: GEOS not found (install libgeos-dev): "
                    "the clash rules that need polygon operations are disabled")
endif()

# BonsaiViewer: the unit tests of the copy (not built by default):
#   cmake --build <build> --target ifcclash_engine_tests && <build>/.../ifcclash_engine_tests
add_executable(ifcclash_engine_tests EXCLUDE_FROM_ALL
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_obb_modifications.cpp)
target_link_libraries(ifcclash_engine_tests PRIVATE ifcclash_engine)
add_test(NAME ifcclash_engine_tests COMMAND ifcclash_engine_tests)
