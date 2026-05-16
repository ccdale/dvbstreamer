#!/usr/bin/env bash
set -euo pipefail

# Build a disposable copy of dvbstreamer under /tmp.
# Usage:
#   misc/build-in-tmp.sh [source_dir] [build_root]
# Defaults:
#   source_dir = current working directory
#   build_root = /tmp/dvbstreamer-build

SOURCE_DIR="${1:-$PWD}"
BUILD_ROOT="${2:-/tmp/dvbstreamer-build}"
BUILD_DIR="${BUILD_ROOT}/dvbstreamer"

if [[ ! -f "${SOURCE_DIR}/configure" && ! -f "${SOURCE_DIR}/configure.in" ]]; then
  echo "error: ${SOURCE_DIR} does not look like a dvbstreamer source tree" >&2
  exit 1
fi

rm -rf "${BUILD_ROOT}"
mkdir -p "${BUILD_ROOT}"
cp -a "${SOURCE_DIR}" "${BUILD_DIR}"
rm -rf "${BUILD_DIR}/.git"

cd "${BUILD_DIR}"
if [[ -x ./configure ]]; then
  ./configure
else
  echo "configure script missing, trying autogen.sh first"
  ./autogen.sh
fi

make -j"$(nproc)"

echo "Build completed in ${BUILD_DIR}"
