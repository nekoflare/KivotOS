section .text
    global _start
    global stack_top
    global stack_bottom
    extern kernel_start

enable_sse:
    mov rax, cr0
    and ax, 0xFFFB		; clear coprocessor emulation CR0.EM
    or ax, 0x2			; set coprocessor monitoring  CR0.MP
    mov cr0, rax
    mov rax, cr4
    or ax, 3 << 9		; set CR4.OSFXSR and CR4.OSXMMEXCPT at the same time
    mov cr4, rax
    ret

    ret

_start:
    mov rsp, stack_bottom   ; load the stack bottom.
    and rsp, ~16            ; align to 16 bytes

    call enable_sse
    call kernel_start

    cli ; halt the evil kernel
    hlt

section .bss
    stack_top:
        resb 8 * 1024 * 1024 ; reserve 1MB of stack.
    stack_bottom: