#!/bin/bash

TARGET=$1

STAGE1_STAGE2_LOCATION_OFFSET=480

# Generate the image file
dd if=/dev/zero of=$TARGET bs=512 count=2880 >/dev/null

# Determine number of reserved sectors
STAGE2_SIZE=$(stat -c%s ${BUILD_DIR}/stage2.bin)
echo ${STAGE2_SIZE}
STAGE2_SECTORS=$(( ( ${STAGE2_SIZE} + 511 ) / 512))
echo ${STAGE2_SECTORS}
RESERVED_SECTORS=$(( 1 + ${STAGE2_SECTORS} ))
echo ${RESERVED_SECTORS}

# Create the file system
mkfs.fat -F 12 -R ${RESERVED_SECTORS} -n "PICO" $TARGET >/dev/null

# Install bootloader
dd if=${BUILD_DIR}/stage1.bin of=$TARGET conv=notrunc bs=1 count=3 2>&1 >/dev/null
dd if=${BUILD_DIR}/stage1.bin of=$TARGET conv=notrunc bs=1 seek=90 skip=90 2>&1 >/dev/null
dd if=${BUILD_DIR}/stage2.bin of=$TARGET conv=notrunc bs=512 seek=1 #>/dev/null

# Write the address of stage 2 to the bootloader
echo "01 00 00 00" | xxd -r -p | dd of=$TARGET conv=notrunc bs=1 seek=$STAGE1_STAGE2_LOCATION_OFFSET
printf "%x" ${STAGE2_SECTORS} | xxd -r -p | dd of=$TARGET conv=notrunc bs=1 seek=$(( $STAGE1_STAGE2_LOCATION_OFFSET + 4 ))

# Copy files
mcopy -i $TARGET ${BUILD_DIR}/kernel.bin "::kernel.bin"
mcopy -i $TARGET test.txt "::test.txt"
mmd -i $TARGET "::mydir"
mcopy -i $TARGET test.txt "::mydir/test.txt"