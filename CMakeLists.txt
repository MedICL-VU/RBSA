cmake_minimum_required(VERSION 3.1...3.23)
PROJECT("RBSA")

set(CMAKE_BUILD_TYPE Debug)

# Source dirs
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(PUBSW_DIR "/usr/pubsw/packages")

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${SRC_DIR})


# ITK/VTK
set(ITK_DIR ${PUBSW_DIR}/itk/5.4.0_centos8/lib/cmake/ITK-5.4/)
find_package(ITK REQUIRED)
include(${ITK_USE_FILE})

set(VTK_DIR ${PUBSW_DIR}/vtk/9.3.0_rocky8/lib64/cmake/vtk-9.3/)
find_package(VTK REQUIRED)


# Build/link executables
set(SRCS
  ${SRC_DIR}/BinaryImageMorphology.cxx
  ${SRC_DIR}/Blur.cxx
  ${SRC_DIR}/RegistrationBasedSyntheticAtrophy.cxx
  ${SRC_DIR}/utils.cxx
)
add_executable(RegistrationBasedSyntheticAtrophy ${SRCS})
target_link_libraries(RegistrationBasedSyntheticAtrophy ${VTK_LIBRARIES} ${ITK_LIBRARIES})
