bits 16

section _TEXT class=CODE

;
; U4D
;
; Operation:      Unsigned 4 byte divide
; Inputs:         DX;AX   Dividend
;                 CX;BX   Divisor
; Outputs:        DX;AX   Quotient
;                 CX;BX   Remainder
; Volatile:       none
;
global __U4D
__U4D:
    shl edx, 16         ; dx to upper half of edx
    mov dx, ax          ; edx - dividend
    mov eax, edx        ; eax - dividend
    xor edx, edx

    shl ecx, 16         ; cx to upper half of ecx
    mov cx, bx          ; ecx - divisor

    div ecx             ; eax - quot, edx - remainder
    mov ebx, edx
    mov ecx, edx
    shr ecx, 16

    mov edx, eax
    shr edx, 16

    ret


;
; void _cdecl x86_div64_32(uint64_t dividend, uint32_t divisor, uint64_t* quotient_out, uint32_t* remainder_out);
;
global _x86_div64_32
_x86_div64_32:

    ; Make new call frame
    push bp             ; Save old call frame
    mov bp, sp          ; Initialize new call frame

    push bx

    ; Divide upper 32 bits
    mov eax, [bp + 8]   ; eax <- upper 32 bits of dividend
    mov ecx, [bp + 12]  ; ecx <- divisor
    xor edx, edx
    div ecx

    ; Store upper 32 bits of quotient
    mov bx, [bp + 16]
    mov [bx + 4], eax

    ; Divide lower 32 bits
    mov eax, [bp + 4]
    div ecx

    ; Store results
    mov [bx], eax
    mov bx, [bp + 18]
    mov bx, [edx]

    pop bx

    ; Restore old call frame
    mov sp, bp
    pop bp
    ret

;
; int 10h ah=0Eh
; args: character, page
;
global _x86_Video_WriteCharTeletype
_x86_Video_WriteCharTeletype:
    push bp
    mov bp, sp

    push bx

    mov ah, 0Eh
    mov al, [bp + 4]
    mov bh, [bp + 6]

    int 10h

    pop bx

    ; Restore old call frame
    mov sp, bp
    pop bp
    ret
;
; bool _cdecl x86_Disk_Reset(uint8_t drive);
;
global _x86_Disk_Reset
_x86_Disk_Reset:

    ; Make new call frame
    push bp
    mov bp, sp

    mov ah, 0
    mov dl, [bp + 4]
    stc
    int 13h

    mov ax, 1
    sbb ax, 0

    ; Restore old call frame
    mov sp, bp
    pop bp
    ret
;
; bool _cdecl x86_Disk_Read(uint8_t drive,
;                           uint16_t cylinder,
;                           uint16_t head,
;                           uint16_t sector,
;                           uint8_t count,
;                           uint8_t far* data_out);
;
global _x86_Disk_Read
_x86_Disk_Read:

    ; Make new call frame
    push bp
    mov bp, sp

    ; Save modified registers
    push bx
    push es

    ; Set up arguments
    mov dl, [bp + 4]    ; dl - Drive number

    mov ch, [bp + 6]    ; ch - Cylinder (lower 8 bits)
    mov cl, [bp + 7]    ; cl - Cylinder to bits 6-7
    shl cl, 6

    mov dh, [bp + 10]   ; dh - Head

    mov al, [bp + 8]    ; cl - Sector to bits 0-5
    and al, 3Fh
    or cl, al

    mov al, [bp + 12]   ; al - Count

    mov bx, [bp + 16]   ; es:bx - Far pointer to data out
    mov es, bx
    mov bx, [bp + 14]

    ; Call interrupt 13h
    mov ah, 02h
    stc
    int 13h

    ; Set return value
    mov ax, 1
    sbb ax, 0

    ; Restore registers
    pop es
    pop bx

    ; Restore old call frame
    mov sp, bp
    pop bp
    ret

;
; bool _cdecl x86_Disk_GetDriveParams(uint8_t drive,
;                                     uint8_t* drive_type_out,
;                                     uint16_t* cylinders_out,
;                                     uint16_t* sectors_out,
;                                     uint16_t* heads_out);
;
global _x86_Disk_GetDriveParams
_x86_Disk_GetDriveParams:

    ; Make new call frame
    push bp             ; Save old call frame
    mov bp, sp          ; Initialize new call frame

    ; Save registers
    push es
    push bx
    push si
    push di

    mov dl, [bp + 4]
    mov ah, 08h
    mov di, 0
    mov es, di

    ; Call interrupt 13h
    stc
    int 13h

    ; Return value
    mov ax, 1
    sbb ax, 0

    ; Out parameters
    mov si, [bp + 6]    ; drive type from bl
    mov [si], bl

    mov bl, ch          ; cylinders - lower bits in ch
    mov bh, cl          ; cylinders - upper bits in cl (6-7)
    shr bh, 6
    mov si, [bp + 8]
    mov [si], bx

    xor ch, ch          ; sectors - lower 5 bits in cl
    and cl, 3Fh
    mov si, [bp + 10]
    mov [si], cx

    mov cl, dh          ; heads - dh
    mov si, [bp + 12]
    mov [si], cx

    ; Restore registers
    pop di
    pop si
    pop bx
    pop es

    ; Restore old call frame
    mov sp, bp
    pop bp
    ret
