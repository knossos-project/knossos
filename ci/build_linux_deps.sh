#!/bin/bash
set -euxo pipefail

time pacman -Sy archlinux-keyring --noconfirm
time pacman -Syu --noconfirm

sudo -u build bash <<'EOF'
cd

curl -fL https://aur.archlinux.org/cgit/aur.git/snapshot/quazip-qt5.tar.gz | bsdtar -xf -
curl -fL https://aur.archlinux.org/cgit/aur.git/snapshot/qtkeychain-qt5.tar.gz | bsdtar -xf -
curl -fL https://aur.archlinux.org/cgit/aur.git/snapshot/pythonqt-knossos-git.tar.gz | bsdtar -xf -

makepkg -s --noconfirm -D quazip-qt5/
makepkg -s --noconfirm -D qtkeychain-qt5/
makepkg -s --noconfirm -D pythonqt-knossos-git/

rm -v */*-debug-*.pkg.tar.zst
EOF

# Deploy
cp -v /home/build/*/*.pkg.tar.zst /root/knossos/
