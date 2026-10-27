#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
CI2_SRC="${CI2_SRC:-/home/Sajnaps/Pobrane/Chicken Invaders 2}"
XMAS_SRC="$CI2_SRC/DLC_xmas"
CORE="${CORE:-/home/Sajnaps/gb300/sf2000_multicore/build/core_87000000}"
RES="$ROOT/res"
COVERS="$ROOT/dist/covers"

if [ ! -f "$CORE" ]; then
    echo "Missing core: $CORE (run make PLATFORM=sf2000 and the multicore build first)"
    exit 1
fi
if [ ! -f "$CI2_SRC/CI2rm.dat" ]; then
    echo "Missing official file: $CI2_SRC/CI2rm.dat"
    exit 1
fi
if [ ! -f "$XMAS_SRC/CI2rmXmas.dat" ]; then
    echo "Missing official file: $XMAS_SRC/CI2rmXmas.dat"
    exit 1
fi

SD=""
for candidate in /media/GB300 /media/Sajnaps/GB300 /run/media/GB300 /run/media/"$USER"/GB300; do
    if mountpoint -q "$candidate" 2>/dev/null && [ -d "$candidate/ROMS" ]; then
        SD="$candidate"
        break
    fi
done
if [ -z "$SD" ]; then
    echo "SD card is not mounted (check /media/GB300 or /media/$USER/GB300)."
    echo "Insert the card and run again: $0"
    exit 2
fi
echo "SD card: $SD"

install_core() {
    local dest="$1"
    mkdir -p "$dest"
    cp -f "$CORE" "$dest/core_87000000"
    cp -f "$CORE" "$dest/oci.sf2k"
    cp -f "$CORE" "$dest/oci.gb3k"
}

echo "=== Core ==="
for dest in "$SD/cores/oci" "$SD/cores" "$SD/system/Deimos/cores"; do
    install_core "$dest"
done

setup_game() {
    local dir="$1"
    local stub="$2"
    local cover="$3"
    mkdir -p "$dir"
    rm -f "$dir/ChickenInvaders2.dat" "$dir/ChickenInvaders2demo.dat"
    install_core "$dir"
    rm -rf "$dir/res"
    cp -a "$RES" "$dir/res"
    printf "oci;%s;%s" "$(basename "$dir")" "$stub" > "$dir/$stub"
    if [ -f "$COVERS/$cover" ]; then
        cp -f "$COVERS/$cover" "$dir/${stub%.gba}.rgb565"
    fi
}

echo "=== Chicken Invaders 2 (official CI2rm) ==="
GAME1="$SD/ROMS/chicken"
setup_game "$GAME1" "ChickenInvaders.gba" "ci2.rgb565"
cp -f "$CI2_SRC/CI2rm.dat" "$GAME1/"
cp -f "$CI2_SRC/CI2rm.dat.1250" "$GAME1/"
cp -f "$CI2_SRC/CI2rm.dat.1252" "$GAME1/"
cp -f "$CI2_SRC/CI2rm.dat.language.v2" "$GAME1/"
printf "oci;chicken;ChickenInvaders.gba" > "$SD/ROMS/ChickenInvaders.gba"
cp -f "$COVERS/ci2.rgb565" "$SD/ROMS/ChickenInvaders.rgb565"

echo "=== Chicken Invaders 2 Xmas (official CI2rmXmas) ==="
GAME2="$SD/ROMS/chicken_xmas"
setup_game "$GAME2" "ChickenInvadersXmas.gba" "ci2xmas.rgb565"
cp -f "$XMAS_SRC/CI2rmXmas.dat" "$GAME2/"
cp -f "$XMAS_SRC/CI2rmXmas.dat.1250" "$GAME2/"
cp -f "$XMAS_SRC/CI2rmXmas.dat.1252" "$GAME2/"
cp -f "$XMAS_SRC/CI2rmXmas.dat.language.v2" "$GAME2/"
printf "oci;chicken_xmas;ChickenInvadersXmas.gba" > "$SD/ROMS/ChickenInvadersXmas.gba"
cp -f "$COVERS/ci2xmas.rgb565" "$SD/ROMS/ChickenInvadersXmas.rgb565"

echo "=== games.opt ==="
cat > "$SD/configs/custom_cores/oci/games.opt" <<'OPTS'
ChickenInvaders.gba|oci
ChickenInvadersXmas.gba|oci
chicken|oci
chicken_xmas|oci
oci.gb3k|oci
oci.sf2k|oci
OPTS

sync
echo "DONE: only official CI2rm.dat / CI2rmXmas.dat files (no demo)."
