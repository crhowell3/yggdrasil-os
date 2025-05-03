[bits 32]

global i686_IDT_Load
i686_IDT_Load:

    ; Make new call frame
    push ebp
    mov ebp, esp

    ; Load IDT
    mov eax, [ebp + 8]
    lidt [eax]

    ; Restore old call frame
    mov esp, ebp
    pop ebp
    ret