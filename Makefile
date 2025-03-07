# Using nasm assembler to create the machine code for the image
ASM=nasm

# Root directory of assembly source code
SRC_DIR=src
# Location for build output files
BUILD_DIR=build

.PHONY: all floppy_image kernel bootloader clean always

#
# Floppy image
#
floppy_image: $(BUILD_DIR)/main_floppy.img

$(BUILD_DIR)/main_floppy.img: bootloader kernel
    # Create empty 1.44 MB file
	dd if=/dev/zero of=$(BUILD_DIR)/main_floppy.img bs=512 count=2880
    # Create FAT12 file system with a default label that will be overwritten
	mkfs.fat -F 12 -n "YGOS" $(BUILD_DIR)/main_floppy.img
    # Put bootloader into first sector of the disk with no truncation
	dd if=$(BUILD_DIR)/bootloader.bin of=$(BUILD_DIR)/main_floppy.img conv=notrunc
    # Copy files to image without needing to mount
	mcopy -i $(BUILD_DIR)/main_floppy.img $(BUILD_DIR)/kernel.bin "::kernel.bin"

#
# Bootloader
#
bootloader: $(BUILD_DIR)/bootloader.bin

$(BUILD_DIR)/bootloader.bin: always
	# Create bootloader binary code from assembly
	$(ASM) $(SRC_DIR)/bootloader/boot.asm -f bin -o $(BUILD_DIR)/bootloader.bin

#
# Kernel
#
kernel: $(BUILD_DIR)/kernel.bin

$(BUILD_DIR)/kernel.bin: always
    # Create kernel binary code from assembly
	$(ASM) $(SRC_DIR)/kernel/main.asm -f bin -o $(BUILD_DIR)/kernel.bin

#
# Always
#
always:
    # Always attempt to create build directory to ensure it exists
	mkdir -p ${BUILD_DIR}

#
# Clean
#
clean:
    # Force remove all files in build directory to perform clean build
	rm -rf ${BUILD_DIR}/*
