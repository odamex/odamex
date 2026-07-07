#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -lt 2 ]; then
  echo "Usage: $0 <output-dmg> <volume-name>" >&2
  exit 1
fi

output_dmg="$1"
volume_name="$2"

root_dir="$(pwd)"
work_dir="$(mktemp -d)"
staging_dir="${work_dir}/staging"

cleanup() {
  rm -rf "${work_dir}"
}
trap cleanup EXIT

odamex_dir="Odamex"
mkdir -p "${staging_dir}/${odamex_dir}"

if [ -d "build/client/odamex.app" ]; then
  cp -R "build/client/odamex.app" "${staging_dir}/${odamex_dir}/Odamex.app"
fi
if [ -d "build/odalaunch/odalaunch.app" ]; then
  cp -R "build/odalaunch/odalaunch.app" "${staging_dir}/${odamex_dir}/odalaunch.app"
fi
if [ -f "build/server/odasrv" ]; then
  cp "build/server/odasrv" "${staging_dir}/${odamex_dir}/odasrv"
fi
if [ -f "${staging_dir}/${odamex_dir}/Odamex.app/Contents/MacOS/odamex.wad" ]; then
  ln -s "Odamex.app/Contents/MacOS/odamex.wad" "${staging_dir}/${odamex_dir}/odamex.wad"
fi

for doc in 3RD-PARTY-LICENSES CHANGELOG LICENSE MAINTAINERS odamex-installed.txt README README.md; do
  if [ -e "${doc}" ]; then
    cp -R "${doc}" "${staging_dir}/${odamex_dir}/"
  fi
done
if [ -d "config-samples" ]; then
  cp -R "config-samples" "${staging_dir}/${odamex_dir}/"
fi
if [ -e "version.txt" ]; then
  cp -R "version.txt" "${staging_dir}/${odamex_dir}/"
fi

licenses_dir="${staging_dir}/${odamex_dir}/licenses"
mkdir -p "${licenses_dir}"
copy_license() {
  local src="$1"
  local dest_name="$2"
  if [ -e "${src}" ]; then
    cp -R "${src}" "${licenses_dir}/${dest_name}"
  fi
}
copy_license "libraries/curl/COPYING" "COPYING.curl.txt"
copy_license "libraries/libpng/LICENSE" "LICENSE.libpng.txt"
copy_license "libraries/miniupnp/LICENSE" "LICENSE.miniupnp.txt"
copy_license "libraries/portmidi/license.txt" "license.portmidi.txt"
copy_license "libraries/fltk/COPYING" "COPYING.fltk.txt"
copy_license "libraries/minilzo/COPYING" "COPYING.minilzo.txt"
copy_license "libraries/jsoncpp/LICENSE" "LICENSE.jsoncpp.txt"
copy_license "libraries/fmt/LICENSE" "LICENSE.fmt.txt"
copy_license "libraries/protobuf/LICENSE" "LICENSE.protobuf.txt"
if [ -d "deps/universal/licenses" ]; then
  for dep_license in deps/universal/licenses/*; do
    if [ -f "${dep_license}" ]; then
      cp "${dep_license}" "${licenses_dir}/"
    fi
  done
fi

setfile_cmd=""
if command -v SetFile >/dev/null 2>&1; then
  setfile_cmd="SetFile"
elif command -v xcrun >/dev/null 2>&1; then
  setfile_cmd="xcrun SetFile"
fi
set_custom_icon() {
  local target="$1"
  local icon="$2"
  if [ ! -e "${target}" ] || [ ! -f "${icon}" ]; then
    return 0
  fi
  if command -v fileicon >/dev/null 2>&1; then
    fileicon set "${target}" "${icon}" || true
  fi
  if [ -n "${setfile_cmd}" ]; then
    ${setfile_cmd} -a C "${target}" || true
  fi
}

folder_icon="media/odamex.icns"
if [ -f "build/client/odamex.app/Contents/Resources/odamex.icns" ]; then
  folder_icon="build/client/odamex.app/Contents/Resources/odamex.icns"
fi
set_custom_icon "${staging_dir}/${odamex_dir}" "${folder_icon}"
set_custom_icon "${staging_dir}/${odamex_dir}/odasrv" "media/odasrv.icns"

if ! command -v create-dmg >/dev/null 2>&1; then
  echo "create-dmg is required but not installed." >&2
  exit 1
fi

create-dmg \
  --volname "${volume_name}" \
  --window-size 500 378 \
  --icon-size 104 \
  --background "${root_dir}/media/macinstaller_background.png" \
  --icon "${odamex_dir}" 111 179 \
  --app-drop-link 384 179 \
  "${output_dmg}" \
  "${staging_dir}"
