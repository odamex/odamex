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
mount_dir="${work_dir}/mount"
image_path="${work_dir}/temp.dmg"
attached=0

cleanup() {
  if [ "${attached}" -eq 1 ]; then
    hdiutil detach "${mount_dir}" >/dev/null 2>&1 || true
  fi
  rm -rf "${work_dir}"
}
trap cleanup EXIT

mkdir -p "${staging_dir}/Odamex"

if [ -d "build/client/odamex.app" ]; then
  cp -R "build/client/odamex.app" "${staging_dir}/Odamex/Odamex.app"
fi
if [ -d "build/odalaunch/odalaunch.app" ]; then
  cp -R "build/odalaunch/odalaunch.app" "${staging_dir}/Odamex/odalaunch.app"
fi
if [ -f "build/server/odasrv" ]; then
  cp "build/server/odasrv" "${staging_dir}/Odamex/odasrv"
fi
if [ -f "${staging_dir}/Odamex/Odamex.app/Contents/MacOS/odamex.wad" ]; then
  ln -s "Odamex.app/Contents/MacOS/odamex.wad" "${staging_dir}/Odamex/odamex.wad"
fi

for doc in 3RD-PARTY-LICENSES CHANGELOG LICENSE MAINTAINERS odamex-installed.txt README README.md; do
  if [ -e "${doc}" ]; then
    cp -R "${doc}" "${staging_dir}/Odamex/"
  fi
done
if [ -d "config-samples" ]; then
  cp -R "config-samples" "${staging_dir}/Odamex/"
fi
if [ -e "version.txt" ]; then
  cp -R "version.txt" "${staging_dir}/Odamex/"
fi

licenses_dir="${staging_dir}/Odamex/licenses"
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

mkdir -p "${staging_dir}/.background"
cp "media/macinstaller_background.png" "${staging_dir}/.background/background.png"
cp "media/odamex.icns" "${staging_dir}/.background/odamex.icns"
cp "media/odasrv.icns" "${staging_dir}/.background/odasrv.icns"

size_mb=$(du -sm "${staging_dir}" | awk '{print $1 + 20}')
hdiutil create -size "${size_mb}m" -srcfolder "${staging_dir}" -fs HFS+ -volname "${volume_name}" -format UDRW "${image_path}"
hdiutil attach -readwrite -noverify -noautoopen "${image_path}" -mountpoint "${mount_dir}"
attached=1

if command -v SetFile >/dev/null 2>&1; then
  SetFile -a C "${mount_dir}/Odamex" || true
fi

osascript "${root_dir}/ci/macos-dmg.applescript" "${mount_dir}"

hdiutil detach "${mount_dir}"
attached=0
hdiutil convert "${image_path}" -format UDZO -o "${output_dmg}"
