    global syscall_handler
    extern syscall_dispatch
    extern kernel_rsp
    extern user_rsp

section .text
syscall_handler:
        swapgs

        mov qword [user_rsp], rsp
        mov rsp, qword [kernel_rsp]

        push qword 0 ;; padding
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

        lea rdi, [rsp]

        ; call me baby
        call syscall_dispatch

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
        add rsp, 8

        mov rsp, qword [user_rsp]
        swapgs
        o64 sysret