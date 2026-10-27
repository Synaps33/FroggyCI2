# Installation

This guide covers the ready-made SD package and a build from source.

## 1. What you need

- Data Frog SF2000 or GB300 / GB300V2 with the **multicore** launcher.
- A patched multicore BIOS: `bios/bisrv.asd` for your console revision.
- FAT32-formatted SD card.
- Your own copy of Chicken Invaders 2.

DAT files expected by the loader:

| Edition | Required files | Recommended folder |
| --- | --- | --- |
| Full / remastered | `CI2rm.dat`, `CI2rm.dat.1250`, `CI2rm.dat.1252`, `CI2rm.dat.language.v2` | `ROMS/chicken/` |
| Christmas | `CI2rmXmas.dat`, `CI2rmXmas.dat.1250`, `CI2rmXmas.dat.1252`, `CI2rmXmas.dat.language.v2` | `ROMS/chicken_xmas/` |
| Legacy demo (optional) | `ChickenInvaders2.dat` or `ChickenInvaders2demo.dat` | same folder as the game |

`CIep2.dat.*` (`UVE WAD`) archives are not supported.

## 2. Install the SD package

1. Back up your SD card.
2. Download `FroggyCI2-sd-vX.Y.Z.zip` from the GitHub **Releases** page.
3. Extract the ZIP to the **root** of the SD card. Merge folders when asked.
4. Install your compatible patched multicore BIOS as `bios/bisrv.asd`.
5. Copy the official DAT files into `ROMS/chicken/` and
   `ROMS/chicken_xmas/`. Do not rename the files.
6. Safely eject the card, insert it into the console and power-cycle.
7. Start **Chicken Invaders 2** or **Chicken Invaders 2 Xmas**.

The package intentionally contains **no game DATs and no BIOS**. It includes the
core, launchers, covers, `res/` and `games.opt`.

### Expected SD layout

```
SD/
├── bios/
│   └── bisrv.asd
├── configs/
│   └── custom_cores/oci/games.opt
├── cores/
│   ├── oci.gb3k
│   ├── oci.sf2k
│   └── oci/
│       ├── core_87000000
│       ├── oci.gb3k
│       └── oci.sf2k
├── system/Deimos/cores/
│   ├── oci.gb3k
│   └── oci.sf2k
└── ROMS/
    ├── ChickenInvaders.gba
    ├── ChickenInvaders.rgb565
    ├── ChickenInvadersXmas.gba
    ├── ChickenInvadersXmas.rgb565
    ├── chicken/
    │   ├── ChickenInvaders.gba
    │   ├── ChickenInvaders.rgb565
    │   ├── CI2rm.dat
    │   ├── CI2rm.dat.1250
    │   ├── CI2rm.dat.1252
    │   ├── CI2rm.dat.language.v2
    │   ├── core_87000000
    │   ├── oci.gb3k
    │   ├── oci.sf2k
    │   └── res/
    └── chicken_xmas/
        ├── ChickenInvadersXmas.gba
        ├── CI2rmXmas.dat
        ├── CI2rmXmas.dat.1250
        ├── CI2rmXmas.dat.1252
        ├── CI2rmXmas.dat.language.v2
        ├── core_87000000
        ├── oci.gb3k
        ├── oci.sf2k
        └── res/
```

The `.gba` files are text launcher stubs:

```
oci;chicken;ChickenInvaders.gba
oci;chicken_xmas;ChickenInvadersXmas.gba
```

They must stay inside their game folders, and matching entries must exist in
`configs/custom_cores/oci/games.opt`.

## 3. Run from a DAT file

You can also launch `ROMS/chicken/CI2rm.dat` or
`ROMS/chicken_xmas/CI2rmXmas.dat` directly. The loader selects the matching
asset family from the filename. Do not launch locale files
(`*.1250`, `*.1252`, `*.language.v2`) directly.

## 4. First boot checklist

- Options shows the active archive: `dat: CI2rm.dat (full)`.
- Resolution can be switched between `320x240` and `640x480`.
- `continue.sav` stores checkpoints; delete it to start a new campaign.
- `video.cfg` stores the display mode.

## 5. Build the core yourself

Set the toolchain and multicore paths to match your machine:

```sh
export MIPS=/opt/frog-toolchain/mipsel-mti-elf_stable/bin/mipsel-mti-elf-
export MULTICORE=/path/to/sf2000_multicore

make PLATFORM=sf2000 MIPS="$MIPS" MULTICORE="$MULTICORE"
make -C "$MULTICORE" clean
make -C "$MULTICORE" CORE="$PWD" CONSOLE=oci FROGGY_TYPE=GB300V2
```

For a Data Frog SF2000, use `FROGGY_TYPE=SF2000`. The output is
`$MULTICORE/build/core_87000000`.

Copy the core into all locations listed in the expected layout, or use the
provided helper:

```sh
SD=/path/to/sdcard
CORE="$MULTICORE/build/core_87000000"
for d in "$SD/cores/oci" "$SD/cores" "$SD/system/Deimos/cores" \
         "$SD/ROMS/chicken" "$SD/ROMS/chicken_xmas"; do
  mkdir -p "$d"
  cp -f "$CORE" "$d/core_87000000"
  cp -f "$CORE" "$d/oci.gb3k"
  cp -f "$CORE" "$d/oci.sf2k"
done
sync
```

`make device` automates the SF2000/GB300 builds and calls `tools/package.py`.
`install_to_sd.sh` and `install_two_games.sh` install a staged build and remove
legacy demo DATs from the game folders.

## 6. Build the host runner (optional)

```sh
make
cp /path/to/ChickenInvaders2.dat .
make test
```

The host build has no SDL/SFML requirement; SDL and SFML are only used by the
optional desktop CMake frontend.

## 7. Troubleshooting

### Bluescreen / `sbrk oom`

The core reports `need_fullpath = true`, so the frontend must not load the whole
DAT into RAM. If you see this, you are probably running an older core or
launching a DAT with a frontend that buffers it. Rebuild/reinstall the core and
use the provided `.gba` launchers.

### `Cannot find file` / missing font

The full-version fonts live in `CI2rm.dat.1250` / `.1252` (or the Xmas
equivalents). Copy the locale DATs next to the main DAT. If they are absent, the
loader falls back to the demo DAT only when no full archive is present.

### Xmas shows the standard menu

The standard and Xmas DAT sets must be in separate folders. When launching a DAT
directly, the loader chooses the family from the filename. If both sets are in
one folder, use the `.gba` launchers or select the Xmas DAT by name.

### `CIep2` files are ignored

`CIep2.dat.*` uses the older `UVE WAD` container without readable filenames in
the TOC. It is not used by this engine.

### Card I/O errors

Always use `sync` and eject the card before removing it. A card removed during a
write may need `fsck.vfat` or a fresh format.

## 8. Controls

| Input | Action |
| --- | --- |
| D-pad | Move / navigate |
| A / B | Fire / confirm |
| X / Y / Start | Launch missile |
| Select / Escape | Skip intro or return |
