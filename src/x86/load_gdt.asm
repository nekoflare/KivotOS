section .data
    extern gdtr
section .text
    global flush_gdt
flush_gdt:
    mov rax, rdi        ; rdi = code segment selector
    mov rcx, rsi        ; rsi = data segment selector

    lea rdx, [gdtr]
    lgdt [rdx]

    ; Reload segments
    mov ds, cx
    mov es, cx
    mov fs, cx
    mov gs, cx
    mov ss, cx

    push rax
    push .flush_done
    retfq
.flush_done:
    ret
