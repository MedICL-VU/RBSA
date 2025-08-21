cmake_minimum_required(VERSION 3.12)
PROJECT(RBSA LANGUAGES CXX)

# ---- Build options -------------------------------------------------------------------------------

# C++ standards (override with -DCMAKE_CXX_STANDARD=17, etc.)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()


# ---- Dependencies --------------------------------------------------------------------------------
# Users can point CMake to their ITK/VTK installs via
#   -DITK_DIR=<ITK_install_path>/lib/cmake/ITK-<version>
#   -DITK_DIR=<VTK_install_path>/lib64/cmake/VTK-<version>
# or via -DCMAKE_PREFIX_PATH=/path/to/prefix

find_package(ITK REQUIRED)
include(${ITK_USE_FILE})

find_package(VTK REQUIRED)


# ---- Sources --------------------------------------------------------------------------------

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(SRCS
  ${SRC_DIR}/main.cxx
  ${SRC_DIR}/BinaryImageMorphology.cxx
  ${SRC_DIR}/BlurMask.cxx
  ${SRC_DIR}/HighResCrop.cxx
  ${SRC_DIR}/RBSA.cxx
  ${SRC_DIR}/MSDD.cxx
  ${SRC_DIR}/ParcellateSurface.cxx
  ${SRC_DIR}/utils.cxx
  ${SRC_DIR}/Warp.cxx
)

add_executable(RBSA ${SRCS})
target_include_directories(RBSA PRIVATE ${SRC_DIR})
target_link_libraries(RBSA PRIVATE ${VTK_LIBRARIES} ${ITK_LIBRARIES})

include(GNUInstallDirs)
install(TARGETS RBSA
  RUNTIME DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}
)
