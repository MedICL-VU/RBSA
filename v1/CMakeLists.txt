cmake_minimum_required(VERSION 3.1...3.23)
PROJECT("SyntheticAtrophy")

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${INCLUDE_DIR})


## ITK
set(ITK_DIR "/usr/pubsw/packages/itk/5.4.0_centos8/lib/cmake/ITK-5.4/")
find_package(ITK REQUIRED)
include(${ITK_USE_FILE})


## VTK
set(VTK_DIR "/usr/pubsw/packages/vtk/9.3.0/lib64/cmake/vtk-9.3/")
find_package(VTK REQUIRED)
include(${VTK_USE_FILE})


## Build/link executables
file(GLOB SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx)

foreach(SRC_FILE ${SRC_FILES})
  get_filename_component(EXE_FILE ${SRC_FILE} NAME_WE)
  add_executable(${EXE_FILE} ${SRC_FILE})
  target_link_libraries(${EXE_FILE} ${VTK_LIBRARIES} ${ITK_LIBRARIES})
endforeach(SRC_FILE ${SRC_FILES})
