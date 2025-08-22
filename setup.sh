#!/usr/bin/env bash

# Usage:
# ./setup.sh \
#    [--build-type Release|Debug] \
#    [--itk <ITK_install_path>/lib/cmake/ITK-<version>] \
#    [--vtk /path/to/vtk-<ver>]
#
# Notes:
# - If you already set CMAKE_PREFIX_PATH to include ITK/VTK, you can omit --itk/--vtk.
# - You can also pass arbitrary -D flags after a double dash, e.g.:
#     ./setup.sh --build-type Debug -- -DCMAKE_CXX_STANDARD=17

# Parse cmake args
BUILD_TYPE="Release"
ITK_DIR_ARG=""
VTK_DIR_ARG=""
PREFIX_ARG=""

PASS_THROUGH_CMAKE_ARGS=()

while [[ $# -gt 0 ]]; do
  case "$1" in
    --build-type)
      BUILD_TYPE="$2"; shift 2;;
    --itk)
      ITK_DIR_ARG="-DITK_DIR=$2"; shift 2;;
    --vtk)
      VTK_DIR_ARG="-DVTK_DIR=$2"; shift 2;;
    --prefix)
      # Useful if both ITK and VTK live under the same prefix
      PREFIX_ARG="-DCMAKE_PREFIX_PATH=$2"; shift 2;;
    --) shift; PASS_THROUGH_CMAKE_ARGS+=("$@"); break;;
    *)
      echo "Unknown option: $1"; exit 1;;
  esac
done


# Build
BUILD_DIR="build"
mkdir -p "${BUILD_DIR}/bin"

echo "Configuring in $BUILD_DIR (type: $BUILD_TYPE)"
cmake -S . -B "$BUILD_DIR" \
  -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
  ${PREFIX_ARG} \
  ${ITK_DIR_ARG} \
  ${VTK_DIR_ARG} \
  "${PASS_THROUGH_CMAKE_ARGS[@]}"

echo "Building..."
cmake --build "$BUILD_DIR" --config "$BUILD_TYPE" -j
cd $BUILD_DIR && make && cd ../
