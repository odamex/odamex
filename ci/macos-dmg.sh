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

for doc in 3RD-PARTY-LICENSES CHANGELOG LICENSE MAINTAINERS odamex-installed.txt README README.md; do
  if [ -e "${doc}" ]; then
    cp -R "${doc}" "${staging_dir}/Odamex/"
  fi
done

mkdir -p "${staging_dir}/.background"
cp "media/macinstaller_background.png" "${staging_dir}/.background/background.png"
cp "media/odamex.icns" "${staging_dir}/.background/odamex.icns"

size_mb=$(du -sm "${staging_dir}" | awk '{print $1 + 20}')
hdiutil create -size "${size_mb}m" -fs HFS+ -volname "${volume_name}" -format UDRW "${image_path}"
hdiutil attach -readwrite -noverify -noautoopen "${image_path}" -mountpoint "${mount_dir}"
attached=1

cp -R "${staging_dir}/Odamex" "${mount_dir}/"
mkdir -p "${mount_dir}/.background"
cp "${staging_dir}/.background/background.png" "${mount_dir}/.background/background.png"
cp "${staging_dir}/.background/odamex.icns" "${mount_dir}/.background/odamex.icns"

osascript "${root_dir}/ci/macos-dmg.applescript" "${volume_name}"

hdiutil detach "${mount_dir}"
attached=0
hdiutil convert "${image_path}" -format UDZO -o "${output_dmg}"
