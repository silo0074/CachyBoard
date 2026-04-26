#!/bin/bash

# Exit immediately if a command exits with a non-zero status
set -e

PROJECT_NAME="CachyBoard"
ROOT_DIR=$(pwd)
PACKAGING_DIR="${ROOT_DIR}/packaging"
OUT_DIR="${ROOT_DIR}/dist"

# Ensure output directory exists
mkdir -p "${OUT_DIR}"

echo "--- Starting Master Build for ${PROJECT_NAME} ---"

# 1. Update Translations (Native)
echo "[1/5] ---------- Updating Translations..."
# Create a temporary build dir for native tools if needed
# mkdir -p build_native && cd build_native
# cmake .. -DCMAKE_BUILD_TYPE=Release
# Run the translation target defined in your CMakeLists.txt
# cmake --build . --target update_translations || echo "Translations updated."
# cd "${ROOT_DIR}"

# 2. Build RPM (Fedora Container)
echo "[2/3] ---------- Building Fedora RPM via Podman..."
podman build -t cachyboard-rpm-builder -f "${PACKAGING_DIR}/Containerfile.fedora" .
podman run --rm \
    -v "${ROOT_DIR}:/project:Z" \
    -w /project \
    cachyboard-rpm-builder \
    bash -c "rm -rf build_rpm && mkdir build_rpm && cd build_rpm && \
             cmake .. -DCMAKE_BUILD_TYPE=Release && \
             make -j$(nproc) && cpack -G RPM && \
             mv *.rpm /project/dist/"

# 3. Build Ubuntu DEB (Container)
echo "[3/5] ---------- Building Ubuntu DEB via Podman..."
podman build -t cachyboard-deb-builder -f "${PACKAGING_DIR}/Containerfile.ubuntu" .
podman run --rm \
    -v "${ROOT_DIR}:/project:Z" \
    -w /project \
    cachyboard-deb-builder \
    bash -c "rm -rf build_ubuntu && mkdir build_ubuntu && cd build_ubuntu && \
             cmake .. -DCMAKE_BUILD_TYPE=Release && \
             make -j$(nproc) && cpack -G DEB && \
             mv *.deb /project/dist/"

# 4. Build Arch/CachyOS PKG (Container)
echo "[4/5] ---------- Building Arch Linux Package via Podman..."
# Copy the changelog into the arch directory so it's available to the PKGBUILD
cp "${ROOT_DIR}/CHANGELOG.spec" "${PACKAGING_DIR}/arch/"

echo "Updating PKGBUILD with current version from CMakeLists..."
CURRENT_VER=$(sed -n '/project(/,/)/p' "CMakeLists.txt" | grep -oP 'VERSION\s+\K[0-9.]+' | head -n1)
CURRENT_REL=$(grep -oP 'set\(PROJECT_RELEASE \K[0-9]+' CMakeLists.txt)

echo "CURRENT_VER:"$CURRENT_VER
echo "CURRENT_REL:"$CURRENT_REL

# Use sed to safely overwrite the lines in PKGBUILD
sed -i "s/^pkgver=.*/pkgver=${CURRENT_VER}/" packaging/arch/PKGBUILD
sed -i "s/^pkgrel=.*/pkgrel=${CURRENT_REL}/" packaging/arch/PKGBUILD

podman build -t cachyboard-arch-builder -f "${PACKAGING_DIR}/Containerfile.cachyos" .
# Use makepkg for Arch to ensure proper dependency tracking and optimizations
podman run --rm \
    --userns=keep-id \
    -v "${ROOT_DIR}:/project:Z" \
    -w /project/packaging/arch \
    cachyboard-arch-builder \
    bash -c "PACKAGER='Liviu Istrate <silo0074@github.com>' makepkg -f --noconfirm && mv *.pkg.tar.zst /project/dist/"

# 5. Generate Checksums
echo "[5/5] ---------- Generating SHA256 Checksums..."
cd "${OUT_DIR}"
# Remove the cmake call and use native tools
for file in *.rpm *.deb *.pkg.tar.zst; do
    if [ -f "$file" ]; then
        echo "Calculating SHA256 for: $file"
        sha256sum "$file" > "${file}.sha256"
        sha256sum "$file" >> "checksums.txt"
    fi
done

echo "--- All packages generated in ${OUT_DIR} ---"
ls -lh "${OUT_DIR}"
