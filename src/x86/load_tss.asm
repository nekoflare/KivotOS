section .text
    global load_tss

load_tss:
    mov ax, 0x28
    ltr ax
    ret