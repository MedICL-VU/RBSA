cmake_minimum_required(VERSION 3.1...3.23)
PROJECT("RBSA")

#set(CMAKE_CXX_STANDARD 11)
set(CMAKE_BUILD_TYPE Debug)

# Source dirs
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(PUBSW_DIR "/usr/pubsw/packages")

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# ITK/VTK
set(ITK_DIR ${PUBSW_DIR}/itk/4.13.2_centos8/lib/cmake/ITK-4.13)
find_package(ITK REQUIRED)
include(${ITK_USE_FILE})

set(VTK_DIR ${PUBSW_DIR}/vtk/9.3.0_rocky8/lib64/cmake/vtk-9.3/)
find_package(VTK REQUIRED)

# Build/link executables
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
