include build_scripts/config.mk

.PHONY: all floppy_image disk_image kernel bootloader clean always

all: floppy_image

include build_scripts/toolchain.mk

#
# Floppy image
#
floppy_image: $(BUILD_DIR)/main_floppy.img

$(BUILD_DIR)/main_floppy.img: bootloader kernel
	@./build_scripts/make_floppy_img.sh $@
	@echo "--> Created: " $@

#
# Disk image
#
disk_image: $(BUILD_DIR)/main_disk.raw

$(BUILD_DIR)/main_disk.raw: bootloader kernel
	@./build_scripts/make_disk_img.sh $@ $(MAKE_DISK_SIZE)
	@echo "--> Created: " $@

#
# Bootloader
#
bootloader: stage1 stage2

stage1: $(BUILD_DIR)/stage1.bin

$(BUILD_DIR)/stage1.bin: always
	@$(MAKE) -C src/bootloader/stage1 BUILD_DIR=$(abspath $(BUILD_DIR))

stage2: $(BUILD_DIR)/stage2.bin

$(BUILD_DIR)/stage2.bin: always
	@$(MAKE) -C src/bootloader/stage2 BUILD_DIR=$(abspath $(BUILD_DIR))

#
# Kernel
#
kernel: $(BUILD_DIR)/kernel.bin

$(BUILD_DIR)/kernel.bin: always
    # Create kernel binary code from assembly
	@$(MAKE) -C src/kernel BUILD_DIR=$(abspath $(BUILD_DIR))

#
# Always
#
always:
    # Always attempt to create build directory to ensure it exists
	@mkdir -p $(BUILD_DIR)

#
# Clean
#
clean:
    # Force remove all files in build directory to perform clean build
	@$(MAKE) -C src/bootloader/stage1 BUILD_DIR=$(abspath $(BUILD_DIR)) clean
	@$(MAKE) -C src/bootloader/stage2 BUILD_DIR=$(abspath $(BUILD_DIR)) clean
	@$(MAKE) -C src/kernel BUILD_DIR=$(abspath $(BUILD_DIR)) clean
	@rm -rf $(BUILD_DIR)/*
