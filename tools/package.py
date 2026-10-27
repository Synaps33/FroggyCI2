#!/usr/bin/env python3
import os
import shutil
import zipfile
from PIL import Image

BASE_DIR = "/home/Sajnaps/open-chicken-invaders-sf2000"
DIST_DIR = os.path.join(BASE_DIR, "dist")

def make_rgb565(png_path, out_path, size=(160, 160)):
    im = Image.open(png_path).convert("RGB").resize(size)
    raw = bytearray()
    for y in range(size[1]):
        for x in range(size[0]):
            r, g, b = im.getpixel((x, y))
            val = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
            raw.append(val & 0xFF)
            raw.append((val >> 8) & 0xFF)
    with open(out_path, "wb") as f:
        f.write(raw)

def build_package(target_type): # 'sf2000' or 'gb300'
    pkg_dir = os.path.join(DIST_DIR, f"sd-{target_type}")
    shutil.rmtree(pkg_dir, ignore_errors=True)
    os.makedirs(pkg_dir, exist_ok=True)

    # Core files
    core_src = os.path.join(DIST_DIR, f"oci.{'sf2k' if target_type == 'sf2000' else 'gb3k'}")
    
    # The multicore loader always loads "<corename>.sf2k", even on GB300.
    # Therefore the console-specific core must be installed under BOTH names.
    # 1. cores/oci/core_87000000
    cores_dir = os.path.join(pkg_dir, "cores", "oci")
    os.makedirs(cores_dir, exist_ok=True)
    shutil.copyfile(core_src, os.path.join(cores_dir, "core_87000000"))
    for ext in ("sf2k", "gb3k"):
        shutil.copyfile(core_src, os.path.join(pkg_dir, "cores", f"oci.{ext}"))
        shutil.copyfile(core_src, os.path.join(cores_dir, f"oci.{ext}"))

    # 2. system/Deimos/cores/
    deimos_dir = os.path.join(pkg_dir, "system", "Deimos", "cores")
    os.makedirs(deimos_dir, exist_ok=True)
    for ext in ("sf2k", "gb3k"):
        shutil.copyfile(core_src, os.path.join(deimos_dir, f"oci.{ext}"))

    # 3. configs/custom_cores/oci/games.opt
    cfg_dir = os.path.join(pkg_dir, "configs", "custom_cores", "oci")
    os.makedirs(cfg_dir, exist_ok=True)
    with open(os.path.join(cfg_dir, "games.opt"), "w") as f:
        f.write("ChickenInvaders.gba|oci\nchicken|oci\noci.gb3k|oci\noci.sf2k|oci\n")

    # 4. ROMS/chicken
    roms_dir = os.path.join(pkg_dir, "ROMS")
    chicken_dir = os.path.join(roms_dir, "chicken")
    os.makedirs(chicken_dir, exist_ok=True)

    # Prefer the official full DAT. Never mix demo assets into a full install.
    full_dat_names = [
        "CI2rm.dat",
        "CI2rm.dat.1250",
        "CI2rm.dat.1252",
        "CI2rm.dat.language.v2",
        "CI2rmXmas.dat",
        "CI2rmXmas.dat.1250",
        "CI2rmXmas.dat.1252",
        "CI2rmXmas.dat.language.v2",
    ]
    legacy_dat_names = [
        "ChickenInvaders2demo.dat",
        "ChickenInvaders2.dat",
    ]
    copied_dat = False
    for name in full_dat_names:
        src = os.path.join(BASE_DIR, name)
        if os.path.isfile(src):
            shutil.copyfile(src, os.path.join(chicken_dir, name))
            copied_dat = True
    if not copied_dat:
        for name in legacy_dat_names:
            src = os.path.join(BASE_DIR, name)
            if os.path.isfile(src):
                shutil.copyfile(src, os.path.join(chicken_dir, "ChickenInvaders2.dat"))
                copied_dat = True
                break
    if not copied_dat:
        raise SystemExit("No Chicken Invaders DAT archive found in project directory")

    # Copy res/ directory
    shutil.copytree(os.path.join(BASE_DIR, "res"), os.path.join(chicken_dir, "res"), dirs_exist_ok=True)

    # Also place the core next to the ROM (some frontends look there).
    for ext in ("sf2k", "gb3k"):
        shutil.copyfile(core_src, os.path.join(chicken_dir, f"oci.{ext}"))
    shutil.copyfile(core_src, os.path.join(chicken_dir, "core_87000000"))

    # Short launcher gba files
    with open(os.path.join(roms_dir, "ChickenInvaders.gba"), "w") as f:
        f.write("oci;chicken;ChickenInvaders.gba")
    with open(os.path.join(chicken_dir, "ChickenInvaders.gba"), "w") as f:
        f.write("oci;chicken;ChickenInvaders.gba")

    # Cover image
    menu_png = os.path.join(BASE_DIR, "screenshots", "cont_01_menu.png")
    if not os.path.exists(menu_png):
        menu_png = os.path.join(BASE_DIR, "screenshots", "tour_01_menu.png")
    if os.path.exists(menu_png):
        make_rgb565(menu_png, os.path.join(roms_dir, "ChickenInvaders.rgb565"))
        make_rgb565(menu_png, os.path.join(chicken_dir, "ChickenInvaders.rgb565"))

    # Readme
    with open(os.path.join(pkg_dir, "README.txt"), "w") as f:
        f.write(f"Open Chicken Invaders port for {target_type.upper()}\n"
                "\n"
                "ZAPIS GRY:\n"
                "Gra zapisuje postep automatycznie (co ~2 s oraz przy zmianie poziomu,\n"
                "zdobyciu punktow/broni/zycia). Po ponownym uruchomieniu wybierz\n"
                "\"continue\" w menu glownym, aby wrocic do ostatniego poziomu.\n"
                "Plik zapisu: ROMS/chicken/continue.sav. Gra over kasuje zapis.\n"
                "\n"
                "Instrukcja instalacji:\n"
                "1. Skopiuj zawartość tego katalogu na kartę SD konsoli.\n"
                "2. Uruchom 'ChickenInvaders.gba' z menu ROMS.\n"
                "\n"
                "Sterowanie:\n"
                "D-Pad Lewo / Prawo: Ruch statkiem / menu\n"
                "D-Pad Góra / Dół: Nawigacja po menu\n"
                "A / B: Strzał / Potwierdzenie\n"
                "X / Y / Start: Wystrzelenie rakiety (Missile)\n"
                "Select / B: Pomiń intro / wyjście do menu\n"
                "\n"
                "WYMAGANIA:\n"
                "- patched multicore BIOS (bios/bisrv.asd) - instalator install_to_sd.sh\n"
                "  kopiuje go z dist/bisrv_gb300.asd lub dist/bisrv_sf2000.asd.\n"
                "- pelna wersja: skopiuj CI2rm.dat oraz CI2rm.dat.1250/.1252/.language.v2\n"
                "  do ROMS/chicken/ (loader wykrywa format automatycznie).\n"
                "\n"
                "DIAGNOSTYKA:\n"
                "- /mnt/sda1/oci_boot.log - etapy startu rdzenia\n"
                "- /mnt/sda1/system/logs/Multicore.log - log frontendu (xlog)\n"
                "- brak oci_boot.log = patched loader nie wystartowal (stock BIOS)\n")

    # Create zip archive
    zip_path = os.path.join(DIST_DIR, f"oci-{target_type}-sd.zip")
    with zipfile.ZipFile(zip_path, 'w', zipfile.ZIP_DEFLATED) as zf:
        for root, dirs, files in os.walk(pkg_dir):
            for file in files:
                full_path = os.path.join(root, file)
                rel_path = os.path.relpath(full_path, pkg_dir)
                zf.write(full_path, rel_path)
    print(f"Created {zip_path} ({os.path.getsize(zip_path)} bytes)")

def main():
    build_package("sf2000")
    build_package("gb300")
    print("Packaging complete!")

if __name__ == "__main__":
    main()
