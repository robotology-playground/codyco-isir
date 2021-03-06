# This file is part of ocra-icub.
# Copyright (C) 2016 Institut des Systèmes Intelligents et de Robotique (ISIR)
# author(s): Ryan Lober, Antoine Hoarau
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 2.8.11)
project(OcraIcub CXX)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
else()
    message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

set(VARS_PREFIX "OcraIcub")
# set(CMAKE_BUILD_TYPE Release)
# set(CMAKE_BUILD_TYPE Debug)

set(${VARS_PREFIX}_MAJOR_VERSION 1)
set(${VARS_PREFIX}_MINOR_VERSION 1)
set(${VARS_PREFIX}_PATCH_VERSION 1)
set(${VARS_PREFIX}_VERSION ${${VARS_PREFIX}_MAJOR_VERSION}.${${VARS_PREFIX}_MINOR_VERSION}.${${VARS_PREFIX}_PATCH_VERSION})

option(GENERATE_DOCUMENTATION "Generate the doxygen documentation." FALSE)

option(USING_ATOM_EDITOR "If using atom as an IDE build the necessary json files." FALSE)
if(USING_ATOM_EDITOR)
    # https://atom.io/packages/linter-clang
    # http://clang.llvm.org/docs/JSONCompilationDatabase.html
    message("-- Using Atom editor as a C++ IDE. Generating necessary files.")
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

find_package(YARP REQUIRED)
find_package(YCM REQUIRED)
find_package(iDynTree REQUIRED)
find_package(yarpWholeBodyInterface REQUIRED)
find_package(OcraRecipes REQUIRED)

list(APPEND CMAKE_MODULE_PATH
        ${YARP_MODULE_PATH}
        ${YCM_MODULE_PATH}
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules
     )

#add yarp definitions
add_definitions(${YARP_DEFINES}) #this contains also -D_REENTRANT

# new "data" installation - available with yarp2.4
include(YarpInstallationHelpers)
yarp_configure_external_installation(codyco)

# Adding RPATH support
option(OCRA_ICUB_ENABLE_RPATH "Enable RPATH for this library" FALSE)
# mark_as_advanced(OCRA_ICUB_ENABLE_RPATH)
include(AddInstallRPATHSupport)
add_install_rpath_support(BIN_DIRS "${CMAKE_INSTALL_PREFIX}/bin"
                          LIB_DIRS "${CMAKE_INSTALL_PREFIX}/lib"
                          DEPENDS OCRA_ICUB_ENABLE_RPATH
                          USE_LINK_PATH)

add_subdirectory(ocra-icub)
add_subdirectory(ocra-icub-server)
add_subdirectory(ocra-icub-clients)
add_subdirectory(ocra-icub-tools)


# add a target to generate API documentation with Doxygen
if(GENERATE_DOCUMENTATION)
    find_package(Doxygen REQUIRED)
    message("Generating API documentation with Doxygen")
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
    message("- Copied wbipluginsReferences.bib into: " ${CMAKE_CURRENT_BINARY_DIR}/wbipluginsReferences.bib)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/docs/wbipluginsReferences.bib
                   ${CMAKE_CURRENT_BINARY_DIR}/wbipluginsReferences.bib COPYONLY)

    add_custom_target(
        docs
        ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen" VERBATIM
    )
endif(GENERATE_DOCUMENTATION)


include(InstallBasicPackageFiles)
get_property(PACKAGE_${VARS_PREFIX}_INCLUDEDIR GLOBAL PROPERTY PACKAGE_${VARS_PREFIX}_INCLUDEDIR)
install_basic_package_files(OcraIcub VARS_PREFIX ${VARS_PREFIX}
                                        VERSION ${${VARS_PREFIX}_VERSION}
                                        COMPATIBILITY SameMajorVersion
                                        TARGETS_PROPERTY ${VARS_PREFIX}_TARGETS
                                        NO_CHECK_REQUIRED_COMPONENTS_MACRO
                                    )
include(AddUninstallTarget)


if(USING_ATOM_EDITOR)
    add_custom_target(  atom-files ALL
                        DEPENDS ocra-icub
                        COMMAND ${CMAKE_COMMAND} -E copy
                        ${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json
                        ${CMAKE_SOURCE_DIR}
                        COMMENT "Copying 'compile_commands.json' to '${CMAKE_SOURCE_DIR}'"
                     )
endif()
