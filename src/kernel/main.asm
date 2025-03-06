org 0x7C00
bits 16


%define ENDL 0x0D, 0x0A

start:
    jmp main

;
; Prints a string to terminal
; Params:
;   - ds:si points to the string
;
puts:
    push si
    push ax

.loop:
    lodsb
    or al, al
    jz .done

    mov ah, 0x0E
    mov bh, 0
    int 0x10

    jmp .loop

.done:
    pop ax
    pop si
    ret

main:

    ; Data segments
    mov ax, 0
    mov ds, ax
    mov es, ax

    ; Stack memory initialized at start of OS
    mov ss, ax
    mov sp, 0x7C00    

    ; Print string to terminal
    mov si, msg_hello
    call puts

    hlt

.halt:
    jmp .halt


msg_hello: db 'Hello, world!', ENDL, 0

times 510-($-$$) db 0
dw 0AA55h
