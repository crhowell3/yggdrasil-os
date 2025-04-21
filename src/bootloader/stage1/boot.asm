org 0x7C00
bits 16


%define ENDL 0x0D, 0x0A

; ========================
; FAT12 file system header
; ========================

; BIOS PARAMETER BLOCK
;
; 0x00:  Jump over disk format information
; 0x03:  Set OEM identifier
;        First 8 bytes is DOS version
;        Next 8 bytes provide the name of the version
;        Microsoft FAT spec recommends "MSWIN4.1" for OEM ID
; 0x0B:  Number of bytes per sector
; 0x0D:  Number of sectors per cluster
; 0x0E:  Number of reserved sectors, including the boot record sectors
; 0x10:  Number of FATs on storage media
; 0x11:  Number of root directory entries
; 0x13:  Total sectors in the logical volume
; 0x15:  Media descriptor type (0xF0 is 3.5" floppy disk)
; 0x16:  Number of sectors per FAT
; 0x18:  Number of sectors per track
; 0x1A:  Number of heads / sides on storage media
; 0x1C:  Number of hidden sectors
; 0x20:  The large sector count

jmp short start
nop

bdb_oem:                    db 'MSWIN4.1'           ; 8 bytes
bdb_bytes_per_sector:       dw 512
bdb_sectors_per_cluter:     db 1
bdb_reserved_sectors:       dw 1
bdb_fat_count:              db 2
bdb_dir_entries_count:      dw 0E0h
bdb_total_sectors:          dw 2880
bdb_media_descriptor_type:  db 0F0h                 ; 2880 * 512 = 1.44 MB
bdb_sectors_per_fat:        dw 9                    ; 0xF0 = 3.5" floppy disk
bdb_sectors_per_track:      dw 18                   ; 9 sectors/FAT
bdb_heads:                  dw 2
bdb_hidden_sectors:         dd 0
bdb_large_sector_count:     dd 0

; EXTENDED BOOT RECORD
;
; 0x024:  Drive number. This is 0x00 for a floppy disk
; 0x025:  Flags in Windows NT, but reserved otherwise
; 0x026:  Signature. Must be 0x28 or 0x29
; 0x027:  VolumeID serial number for tracking volumes between computers
; 0x02B:  Volume label string padded with whitespace
; 0x036:  System ID string for the FAT file system type
; 0x03E:  Boot code (448 bytes)
; 0x1FE:  Bootable partition signature (0xAA55)

ebr_drive_number:           db 0                    ; floppy drive ID
                            db 0                    ; reserved
ebr_signature:              db 29h
ebr_volume_id:              db 10h, 20h, 30h, 40h
ebr_volume_label:           db 'YGGDRASIL  '        ; 11 bytes with padding
ebr_system_id:              db 'FAT12   '           ; 8 bytes with padding

; =========
; Boot Code
; =========

start:
    ; Data segments
    mov ax, 0
    mov ds, ax
    mov es, ax

    ; Stack memory initialized at start of OS
    mov ss, ax
    mov sp, 0x7C00

    push es
    push word .after
    retf

.after:

    mov [ebr_drive_number], dl

    ; Print string to terminal
    mov si, msg_loading
    call puts

    ; Read drive parameters
    push es
    mov ah, 08h
    int 13h
    jc floppy_error
    pop es

    and cl, 0x3F
    xor ch, ch
    mov [bdb_sectors_per_track], cx

    inc dh
    mov [bdb_heads], dh

    ; Read FAT root dir
    mov ax, [bdb_sectors_per_fat]
    mov bl, [bdb_fat_count]
    xor bh, bh
    mul bx
    add ax, [bdb_reserved_sectors]
    push ax

    mov ax, [bdb_dir_entries_count]
    shl ax, 5
    xor dx, dx
    div word [bdb_bytes_per_sector]

    test dx, dx
    jz .root_dir_after
    inc ax

.root_dir_after:

    ; Read root dir
    mov cl, al
    pop ax
    mov dl, [ebr_drive_number]
    mov bx, buffer
    call disk_read

    ; Search for stage2.bin
    xor bx, bx
    mov di, buffer

.search_stage2:

    mov si, file_stage2_bin
    mov cx, 11
    push di
    repe cmpsb
    pop di
    je .found_stage2

    add di, 32
    inc bx
    cmp bx, [bdb_dir_entries_count]
    jl .search_stage2

    jmp stage2_not_found_error

.found_stage2:

    mov ax, [di + 26]
    mov [stage2_cluster], ax

    ; Load FAT from disk into memory
    mov ax, [bdb_reserved_sectors]
    mov bx, buffer
    mov cl, [bdb_sectors_per_fat]
    mov dl, [ebr_drive_number]
    call disk_read

    ; Read stage2 and process FAT chain
    mov bx, STAGE2_LOAD_SEGMENT
    mov es, bx
    mov bx, STAGE2_LOAD_OFFSET

.load_stage2_loop:

    ; Read next cluster
    mov ax, [stage2_cluster]
    add ax, 31
    mov cl, 1
    mov dl, [ebr_drive_number]
    call disk_read

    add bx, [bdb_bytes_per_sector]

    ; Compute location of next cluster
    mov ax, [stage2_cluster]
    mov cx, 3
    mul cx
    mov cx, 2
    div cx

    mov si, buffer
    add si, ax
    mov ax, [ds:si]

    or dx, dx
    jz .even

.odd:
    shr ax, 4
    jmp .next_cluster_after

.even:
    and ax, 0x0FFF

.next_cluster_after:
    cmp ax, 0x0FF8
    jae .read_finish

    mov [stage2_cluster], ax
    jmp .load_stage2_loop

.read_finish:

    ; Boot device in dl
    mov dl, [ebr_drive_number]

    mov ax, STAGE2_LOAD_SEGMENT
    mov ds, ax
    mov es, ax

    jmp STAGE2_LOAD_SEGMENT:STAGE2_LOAD_OFFSET

    jmp wait_key_and_reboot


    cli
    hlt

; ==============
; Error handling
; ==============

floppy_error:
    mov si, message_read_fail
    call puts
    jmp wait_key_and_reboot

stage2_not_found_error:
    mov si, msg_stage2_not_found
    call puts
    jmp wait_key_and_reboot

wait_key_and_reboot:
    mov ah, 0
    int 16h                                 ; Interrupt code for waiting for user keypress
    jmp 0FFFFh:0                            ; This jumps to the beginning of the BIOS to initiate reboot

.halt:
    cli
    hlt

;
; Prints a string to terminal
; Params:
;   - ds:si points to the string
;
puts:
    push si
    push ax
    push bx

.loop:
    lodsb
    or al, al
    jz .done

    mov ah, 0x0E
    mov bh, 0
    int 0x10

    jmp .loop

.done:
    pop bx
    pop ax
    pop si
    ret

; =============
; Disk routines
; =============

;
; Convert LBA address to CHS address
; Parameters:
;   - ax: LBA address
; Returns:
;   - cx [bits 0-5]: sector number
;   - cx [bits 6-15]: cylinder / track
;   - dh: head
;

lba_to_chs:

    push ax
    push dx

    xor dx, dx
    div word [bdb_sectors_per_track]

    inc dx
    mov cx, dx

    xor dx, dx
    div word [bdb_heads]

    mov dh, dl
    mov ch, al
    shl ah, 6
    or cl, ah

    pop ax
    mov dl, al
    pop ax
    ret

;
; Read sectors from a disk
; Parameters:
;   - ax: LBA address
;   - cl: Number of sectors to read
;   - dl: Drive number
;   - es:bx: Memory address where the data will be stored
;
disk_read:
    push ax
    push bx
    push cx
    push dx
    push di

    push cx                             ; temporarily save cl
    call lba_to_chs                     ; compute CHS position
    pop ax                              ; al is the number of sectors to read

    mov ah, 02h                         ; reading may not work first time
    mov di, 3                           ; number of times to retry

.retry:
    pusha                               ; save all registers
    stc                                 ; manually set carry flag
    int 13h                             ; carry flag cleared indicates success
    jnc .done

    ; Execute this if read fails
    popa
    call disk_reset

    dec di
    test di, di
    jnz .retry

.fail:
    ; Jump and halt because it is assumed that the disk cannot be read from
    ; since all attempts have been exhausted
    jmp floppy_error

.done:
    popa

    pop di
    pop dx
    pop cx
    pop bx
    pop ax
    ret

;
; Reset the disk controller
; Parameters:
;   - dl: Drive number
;
disk_reset:
    pusha
    mov ah, 0
    stc
    int 13h
    jc floppy_error
    popa
    ret

msg_loading:                db 'Loading...', ENDL, 0
message_read_fail:          db 'Read from disk failed!', ENDL, 0
msg_stage2_not_found:       db 'STAGE2.BIN file not found!', ENDL, 0
file_stage2_bin:            db 'STAGE2  BIN'
stage2_cluster:             dw 0

STAGE2_LOAD_SEGMENT         equ 0x2000
STAGE2_LOAD_OFFSET          equ 0


times 510-($-$$) db 0
dw 0AA55h

buffer: