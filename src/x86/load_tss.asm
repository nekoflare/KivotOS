section .text
    global load_tss

load_tss:
    mov ax, 0x30
    ltr ax
    ret