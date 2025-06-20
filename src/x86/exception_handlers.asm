section .text
    extern interrupt_handler

; Exceptions with error codes: 0x8, 0xa-0xe, 0x11, 0x15

%macro ISR_NOERRORCODE 1
    global interrupt_handler%1:
    interrupt_handler%1:
        push qword 0 ; dummy error code
        push qword %1 ; interrupt number
        jmp isr_common
%endmacro

%macro ISR_ERRORCODE 1
    global interrupt_handler%1:
    interrupt_handler%1:
        push qword %1 ; interrupt number
        jmp isr_common
%endmacro

ISR_NOERRORCODE 0
ISR_NOERRORCODE 1
ISR_NOERRORCODE 2
ISR_NOERRORCODE 3
ISR_NOERRORCODE 4
ISR_NOERRORCODE 5
ISR_NOERRORCODE 6
ISR_NOERRORCODE 7
ISR_ERRORCODE 8
ISR_NOERRORCODE 9
ISR_ERRORCODE 10
ISR_ERRORCODE 11
ISR_ERRORCODE 12
ISR_ERRORCODE 13
ISR_ERRORCODE 14
ISR_NOERRORCODE 15
ISR_NOERRORCODE 16
ISR_ERRORCODE 17
ISR_NOERRORCODE 18
ISR_NOERRORCODE 19
ISR_NOERRORCODE 20
ISR_ERRORCODE 21
ISR_NOERRORCODE 22
ISR_NOERRORCODE 23
ISR_NOERRORCODE 24
ISR_NOERRORCODE 25
ISR_NOERRORCODE 26
ISR_NOERRORCODE 27
ISR_NOERRORCODE 28
ISR_NOERRORCODE 29
ISR_NOERRORCODE 30
ISR_NOERRORCODE 31

; do the rest uwu
%assign i 32
%rep 224
ISR_NOERRORCODE i
%assign i i+1
%endrep

global isr_common
isr_common:
    swapgs
    push rax
    push rbx
    push rcx
    push rdx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    xor rax, rax

    mov ax, ds
    push rax
    mov ax, es
    push rax
    mov ax, fs
    push rax
    mov ax, gs
    push rax

    cld

    ; load to the rdi the address of saved registers.
    lea rdi, [rsp]
    call interrupt_handler

    xor rax, rax
    pop rax
    mov gs, ax
    pop rax
    mov fs, ax
    pop rax
    mov es, ax
    pop rax
    mov ds, ax

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rdx
    pop rcx
    pop rbx
    pop rax

    swapgs
    add rsp, 16
    iretq

section .rodata
    align 8
    global exception_handler_pointers
exception_handler_pointers:
    dq interrupt_handler0
    dq interrupt_handler1
    dq interrupt_handler2
    dq interrupt_handler3
    dq interrupt_handler4
    dq interrupt_handler5
    dq interrupt_handler6
    dq interrupt_handler7
    dq interrupt_handler8
    dq interrupt_handler9
    dq interrupt_handler10
    dq interrupt_handler11
    dq interrupt_handler12
    dq interrupt_handler13
    dq interrupt_handler14
    dq interrupt_handler15
    dq interrupt_handler16
    dq interrupt_handler17
    dq interrupt_handler18
    dq interrupt_handler19
    dq interrupt_handler20
    dq interrupt_handler21
    dq interrupt_handler22
    dq interrupt_handler23
    dq interrupt_handler24
    dq interrupt_handler25
    dq interrupt_handler26
    dq interrupt_handler27
    dq interrupt_handler28
    dq interrupt_handler29
    dq interrupt_handler30
    dq interrupt_handler31
    %assign i 32
    %rep 224
    dq interrupt_handler%+i
    %assign i i+1
    %endrep
