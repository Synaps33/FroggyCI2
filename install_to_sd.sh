#!/usr/bin/env bash
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
DIST="$PROJECT_DIR/dist"
SD_ROOT="${1:-}"

if [ -z "$SD_ROOT" ]; then
    for p in /media/Sajnaps/GB300 /media/Sajnaps/SF2000 /media/* /mnt/*; do
        if [ -d "$p/system/Deimos/cores" ] || [ -d "$p/cores" ]; then
            SD_ROOT="$p"
            break
        fi
    done
fi

if [ -z "$SD_ROOT" ] || [ ! -d "$SD_ROOT" ]; then
    echo "No SD card detected. Usage: $0 <card_path>"
    exit 1
fi

IS_GB300=0
if [ "${FORCE_GB300:-0}" = "1" ] || [[ "$SD_ROOT" =~ GB300 ]] || \
   [ -f "$SD_ROOT/cores/sonicmania.gb3k" ] || \
   [ -f "$SD_ROOT/system/Deimos/cores/sonicmania.gb3k" ]; then
    IS_GB300=1
fi

if [ "$IS_GB300" -eq 1 ]; then
    CORE="$DIST/oci.gb3k"
    BIOS="$DIST/bisrv_gb300.asd"
    PKG="$DIST/sd-gb300"
    echo "Detected GB300 / GB300V2"
else
    CORE="$DIST/oci.sf2k"
    BIOS="$DIST/bisrv_sf2000.asd"
    PKG="$DIST/sd-sf2000"
    echo "Detected Data Frog SF2000"
fi

if [ ! -f "$CORE" ]; then
    echo "Missing $CORE - run make device first"
    exit 1
fi
if ! compgen -G "$PKG/ROMS/chicken/*.dat*" >/dev/null; then
    echo "No DAT archive in $PKG/ROMS/chicken - run make device first"
    exit 1
fi

echo "Installing Open Chicken Invaders into: $SD_ROOT"
cp -a "$PKG"/. "$SD_ROOT"/

# Multicore zawsze laduje "<core>.sf2k" (takze na GB300), wiec wybrany
# build konsoli musi byc zainstalowany pod OBOMA nazwami.
mkdir -p "$SD_ROOT/system/Deimos/cores" "$SD_ROOT/cores/oci" "$SD_ROOT/ROMS/chicken"
for ext in sf2k gb3k; do
    cp -f "$CORE" "$SD_ROOT/system/Deimos/cores/oci.$ext"
    cp -f "$CORE" "$SD_ROOT/cores/oci.$ext"
    cp -f "$CORE" "$SD_ROOT/ROMS/chicken/oci.$ext"
done
cp -f "$CORE" "$SD_ROOT/cores/oci/core_87000000"
cp -f "$CORE" "$SD_ROOT/ROMS/chicken/core_87000000"

# Patched multicore BIOS jest wymagany do uruchamiania stubow .gba.
# Domyslnie instalujemy tylko gdy brak; INSTALL_BIOS=1 wymusza aktualizacje.
if [ -f "$BIOS" ]; then
    if [ "${INSTALL_BIOS:-0}" = "1" ]; then
        mkdir -p "$SD_ROOT/bios"
        [ -f "$SD_ROOT/bios/bisrv.asd" ] && cp -f "$SD_ROOT/bios/bisrv.asd" "$SD_ROOT/bios/bisrv.asd.bak" || true
        cp -f "$BIOS" "$SD_ROOT/bios/bisrv.asd"
        echo "Updated bios/bisrv.asd (patched multicore)"
    elif [ ! -f "$SD_ROOT/bios/bisrv.asd" ]; then
        mkdir -p "$SD_ROOT/bios"
        cp -f "$BIOS" "$SD_ROOT/bios/bisrv.asd"
        echo "Copied missing bios/bisrv.asd (patched multicore)"
    fi
fi

# Logi startu rdzenia: /mnt/sda1/oci_boot.log + Multicore.log
mkdir -p "$SD_ROOT/bios"
: > "$SD_ROOT/bios/enable_xlog.txt"

sync
echo "=========================================================="
echo " Open Chicken Invaders installed."
echo " Launch from the menu: ROMS/ChickenInvaders.gba"
echo " Boot log:       /mnt/sda1/oci_boot.log"
echo " Frontend log:   /mnt/sda1/system/logs/Multicore.log"
echo "=========================================================="
