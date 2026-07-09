#!/bin/sh
# Refresh the vendored libopenmpt. We bundle only the decode-relevant subtree
# (see CMakeLists.txt); the src/ tree is header-only for our build and the
# huge app dirs (mptrack, openmpt123, test, ...) are not needed.
#
# Pinned version: see OPENMPT_VERSION.txt (currently libopenmpt-0.8.7).
# No source patch is required as of 0.8.x.
set -e
TAG="${1:-libopenmpt-0.8.7}"

tmp="$(mktemp -d)"
git clone --depth 1 --branch "$TAG" https://github.com/OpenMPT/openmpt.git "$tmp/openmpt"

rm -rf openmpt
mkdir openmpt
for d in common soundlib sounddsp src libopenmpt; do
    cp -R "$tmp/openmpt/$d" openmpt/
done
rm -rf "$tmp"

echo "${TAG#libopenmpt-}" > OPENMPT_VERSION.txt
echo "Vendored $TAG into ./openmpt"
