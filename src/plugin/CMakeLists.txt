#################
#
# quickshell-bar - A simple desktop bar built with Quickshell that I'm developing mostly for myself :)
# Copyright (C) 2026 The caelestia-shell contributors (see caelestia-shell-contributors.txt), Andrej Wolfenhaut <andrejwolfenhaut@gmail.com>
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#
# Some code is reused from the caelestia-shell project (https://github.com/caelestia-dots/shell).
# In the following section of every source file, the relevant reused code and/or ideas is/are described.
# Otherwise, this comment ends here.
#
# - From plugin/CMakeLists.txt
#   - QML project setup
# - From plugin/cmake/qml-module.cmake
#   - qml_module function
#
#################

#
# Qt project setup
#

find_package(Qt6 REQUIRED COMPONENTS Core Quick Qml)

set(QT_QML_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/qml")
qt_standard_project_setup(REQUIRES 6.9)

#
# Helper function for QML modules
#

function(qml_module arg_TARGET)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "URI;INCLUDE_PREFIX" "SOURCES;QML_FILES;QML_SINGLETONS;RESOURCES;DEPENDENCIES;IMPORTS;OPTIONAL_IMPORTS;DEFAULT_IMPORTS;LIBRARIES")

    set_source_files_properties(${arg_QML_SINGLETONS} PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

    qt_add_qml_module(${arg_TARGET}
        URI ${arg_URI}
        SOURCES ${arg_SOURCES}
        QML_FILES ${arg_QML_FILES} ${arg_QML_SINGLETONS}
        RESOURCES ${arg_RESOURCES}
        DEPENDENCIES ${arg_DEPENDENCIES}
        IMPORTS ${arg_IMPORTS}
        OPTIONAL_IMPORTS ${arg_OPTIONAL_IMPORTS}
        DEFAULT_IMPORTS ${arg_DEFAULT_IMPORTS}
    )

    qt_query_qml_module(${arg_TARGET}
        URI module_uri
        PLUGIN_TARGET module_plugin_target
        TARGET_PATH module_target_path
        QMLDIR module_qmldir
        TYPEINFO module_typeinfo
    )

    message(STATUS "Created QML module: ${module_uri}")

    # Modules can't link together properly if the backing target and plugin target
    # are in the same dir (because diff modules can't access each other).
    # So install backing targets to QMLDIR/.../lib/ instead.
    string(REPLACE "/" ";" uri_parts "${module_target_path}")
    list(GET uri_parts 0 top_level)
    set(backing_lib_dir "${INSTALL_QMLDIR}/${top_level}/lib")
    set(module_dir "${INSTALL_QMLDIR}/${module_target_path}")

    install(TARGETS ${arg_TARGET}
        LIBRARY DESTINATION "${backing_lib_dir}"
        RUNTIME DESTINATION "${backing_lib_dir}"
    )
    install(TARGETS "${module_plugin_target}"
        LIBRARY DESTINATION "${module_dir}"
        RUNTIME DESTINATION "${module_dir}"
    )
    install(FILES "${module_qmldir}" DESTINATION "${module_dir}")
    install(FILES "${module_typeinfo}" DESTINATION "${module_dir}")

    target_link_libraries(${arg_TARGET} PRIVATE Qt::Core Qt::Qml ${arg_LIBRARIES})

    # Expose headers to other modules as <prefix>/xxx.hpp
    if(arg_INCLUDE_PREFIX)
        set(include_dir "${CMAKE_CURRENT_BINARY_DIR}/include")
        file(MAKE_DIRECTORY "${include_dir}")
        file(CREATE_LINK "${CMAKE_CURRENT_SOURCE_DIR}" "${include_dir}/${arg_INCLUDE_PREFIX}" SYMBOLIC)
        target_include_directories(${arg_TARGET} PUBLIC "${include_dir}")
    endif()

    # Add backing target dir to plugin rpath so it can find its backing target
    file(RELATIVE_PATH plugin_to_lib "/${module_target_path}" "/${top_level}/lib")
    set_property(TARGET ${module_plugin_target} APPEND PROPERTY INSTALL_RPATH "$ORIGIN/${plugin_to_lib}")
endfunction()

#
# Deleagte to core module
#

add_subdirectory(src/QuickshellBar)
