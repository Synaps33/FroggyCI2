TARGET_NAME := oci

SRC_DIR     := src
LIBRETRO_DIR:= libretro
RUNNER_DIR  := runner
BUILD       ?= build_host

MULTICORE   ?= /home/Sajnaps/gb300/sf2000_multicore
MIPS_CROSS  ?= /home/Sajnaps/toolchains/mipsel-linux-muslsf-cross/bin/mipsel-linux-muslsf-
QEMU        ?= /home/Sajnaps/toolchains/qemu/usr/bin/qemu-mipsel

GAME_CPP := $(filter-out src/main.cpp, $(shell find $(SRC_DIR) -name "*.cpp"))
CORE_CPP := $(LIBRETRO_DIR)/oci_libretro.cpp
RUNNER_CPP := $(RUNNER_DIR)/test_runner.cpp

COMMON_DEFS := -D_GNU_SOURCE
INCLUDES := -I$(SRC_DIR) -I$(LIBRETRO_DIR) -I.

ifeq ($(PLATFORM),sf2000)
    MIPS ?= /opt/frog-toolchain/mipsel-mti-elf_stable/bin/mipsel-mti-elf-
    CC := $(MIPS)gcc
    CXX := $(MIPS)g++
    LD := $(MIPS)ld
    AR := $(MIPS)ar
    RANLIB := $(MIPS)ranlib
    BUILD := build_sf2000
    TARGET := $(TARGET_NAME)_libretro_sf2000.a
    MIPS_FLAGS := -EL -march=mips32 -mtune=mips32r2 -msoft-float -O2 -G0 \
                  -mno-abicalls -fno-pic -ffunction-sections -fdata-sections \
                  -fno-asynchronous-unwind-tables -fno-unwind-tables \
                  -fno-align-functions -fno-align-jumps -fno-align-loops \
                  -fno-align-labels -fomit-frame-pointer -DSF2000 -DGB300
    CXX_PRE := -include cstring -include cstdio -include cstdlib
    CXXFLAGS := $(MIPS_FLAGS) $(COMMON_DEFS) $(CXX_PRE) $(INCLUDES) \
                -I$(MULTICORE)/include \
                -std=c++17 -fno-exceptions -fno-rtti -fno-threadsafe-statics \
                -Wno-deprecated -Wno-narrowing -Wno-write-strings
    OBJS := $(patsubst %.cpp,$(BUILD)/%.o,$(GAME_CPP)) \
            $(patsubst $(LIBRETRO_DIR)/%.cpp,$(BUILD)/libretro/%.o,$(CORE_CPP))

else ifeq ($(PLATFORM),mips)
    CC := $(MIPS_CROSS)gcc
    CXX := $(MIPS_CROSS)g++
    AR := $(MIPS_CROSS)ar
    RANLIB := $(MIPS_CROSS)ranlib
    BUILD := build_mips
    TARGET := runner/test_runner_mips
    MIPS_FLAGS := -EL -march=mips32 -mtune=mips32r2 -msoft-float -O2 -static
    CXXFLAGS := $(MIPS_FLAGS) $(COMMON_DEFS) $(INCLUDES) \
                -std=c++17 -Wno-deprecated -Wno-narrowing -Wno-write-strings
    OBJS := $(patsubst %.cpp,$(BUILD)/%.o,$(GAME_CPP)) \
            $(patsubst $(LIBRETRO_DIR)/%.cpp,$(BUILD)/libretro/%.o,$(CORE_CPP)) \
            $(BUILD)/runner/test_runner.o

else
    CC := gcc
    CXX := g++
    AR ?= ar
    BUILD := build_host
    TARGET := runner/test_runner
    CXXFLAGS := -O2 -g $(COMMON_DEFS) $(INCLUDES) \
                -std=c++17 -Wno-deprecated -Wno-narrowing -Wno-write-strings
    OBJS := $(patsubst %.cpp,$(BUILD)/%.o,$(GAME_CPP)) \
            $(patsubst $(LIBRETRO_DIR)/%.cpp,$(BUILD)/libretro/%.o,$(CORE_CPP)) \
            $(BUILD)/runner/test_runner.o
endif

.PHONY: all clean test mips-test device

ifeq ($(PLATFORM),sf2000)
all: $(TARGET)

$(TARGET): $(OBJS)
	@echo "Creating $@ (partial link: keeps level registrars alive)"
	$(LD) -EL -m elf32ltsmip -r -o $(BUILD)/oci_combined.o $(OBJS)
	$(AR) rcs $@ $(BUILD)/oci_combined.o
	$(RANLIB) $@
else
all: $(TARGET)

$(TARGET): $(OBJS)
	@echo "Linking $@"
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) -lm
endif

$(BUILD)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD)/libretro/%.o: $(LIBRETRO_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD)/runner/%.o: $(RUNNER_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

test: all
	mkdir -p screenshots
	rm -f continue.sav
	./runner/test_runner . 1200 screenshots/tour tour
	./runner/test_runner . 800 screenshots/cont continue
	@python3 -c "import glob, subprocess; [subprocess.run(['ffmpeg', '-y', '-i', f, f[:-4]+'.png'], stderr=subprocess.DEVNULL) for f in glob.glob('screenshots/tour_*.ppm') + glob.glob('screenshots/cont_*.ppm')]"
	@ls -lh screenshots/tour_*.png screenshots/cont_*.png

mips-test:
	$(MAKE) PLATFORM=mips -j$$(nproc)
	mkdir -p screenshots
	$(QEMU) ./runner/test_runner_mips . 1200 screenshots/mips_tour
	@python3 -c "import glob, subprocess; [subprocess.run(['ffmpeg', '-y', '-i', f, f[:-4]+'.png'], stderr=subprocess.DEVNULL) for f in glob.glob('screenshots/mips_tour_*.ppm')]"
	@ls -lh screenshots/mips_tour_*.png

device:
	$(MAKE) clean
	$(MAKE) PLATFORM=sf2000 -j$$(nproc)
	mkdir -p dist
	$(MAKE) -C $(MULTICORE) clean
	$(MAKE) -C $(MULTICORE) CORE=$(CURDIR) CONSOLE=oci FROGGY_TYPE=SF2000
	cp -f $(MULTICORE)/build/core_87000000 dist/oci.sf2k
	cp -f $(MULTICORE)/build/bisrv.asd dist/bisrv_sf2000.asd
	$(MAKE) -C $(MULTICORE) clean
	$(MAKE) -C $(MULTICORE) CORE=$(CURDIR) CONSOLE=oci FROGGY_TYPE=GB300V2
	cp -f $(MULTICORE)/build/core_87000000 dist/oci.gb3k
	cp -f $(MULTICORE)/build/bisrv.asd dist/bisrv_gb300.asd
	python3 tools/package.py
	@ls -lh dist/

clean:
	rm -rf build_host build_sf2000 build_mips runner/test_runner runner/test_runner_mips \
	       $(TARGET_NAME)_libretro_sf2000.a
