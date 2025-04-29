include build_scripts/config.mk

.PHONY: all floppy_image kernel bootloader clean always

all: floppy_image

include build_scripts/toolchain.mk

#
# Floppy image
#
floppy_image: $(BUILD_DIR)/main_floppy.img

$(BUILD_DIR)/main_floppy.img: bootloader kernel
    # Create empty 1.44 MB file
	@dd if=/dev/zero of=$@ bs=512 count=2880 > /dev/null
    # Create FAT12 file system with a default label that will be overwritten
	@mkfs.fat -F 12 -n "PICO" $@ > /dev/null
    # Put bootloader into first sector of the disk with no truncation
	@dd if=$(BUILD_DIR)/stage1.bin of=$@ conv=notrunc > /dev/null
    # Copy files to image without needing to mount
	@mcopy -i $@ $(BUILD_DIR)/stage2.bin "::stage2.bin"
	@mcopy -i $@ $(BUILD_DIR)/kernel.bin "::kernel.bin"
	@mcopy -i $@ test.txt "::test.txt"
	@mmd -i $@ "::mydir"
	@mcopy -i $@ test.txt "::mydir/test.txt"

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
