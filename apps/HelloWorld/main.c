
#define SYSCALL0(n) ({ \
    long res; \
    asm volatile ("syscall" \
                  : "=a" (res) \
                  : "a" (n) \
                  : "rcx", "r11", "memory"); \
    res; \
})

#define SYSCALL1(n, a1) ({ \
    long res; \
    asm volatile ("syscall" \
                  : "=a" (res) \
                  : "a" (n), "D" ((long)(a1)) \
                  : "rcx", "r11", "memory"); \
    res; \
})

#define SYSCALL2(n, a1, a2) ({ \
    long res; \
    asm volatile ("syscall" \
                  : "=a" (res) \
                  : "a" (n), "D" ((long)(a1)), "S" ((long)(a2)) \
                  : "rcx", "r11", "memory"); \
    res; \
})

#define SYSCALL3(n, a1, a2, a3) ({ \
    long res; \
    asm volatile ("syscall" \
                  : "=a" (res) \
                  : "a" (n), "D" ((long)(a1)), "S" ((long)(a2)), "d" ((long)(a3)) \
                  : "rcx", "r11", "memory"); \
    res; \
})

#define SYSCALL4(n, a1, a2, a3, a4) ({ \
    long res; \
    register long r10 __asm__("r10") = (long)(a4); \
    asm volatile ("syscall" \
                  : "=a" (res) \
                  : "a" (n), "D" ((long)(a1)), "S" ((long)(a2)), "d" ((long)(a3)), "r" (r10) \
                  : "rcx", "r11", "memory"); \
    res; \
})

#define SYSCALL5(n, a1, a2, a3, a4, a5) ({ \
    long res; \
    register long r10 __asm__("r10") = (long)(a4); \
    register long r8  __asm__("r8")  = (long)(a5); \
    asm volatile ("syscall" \
                  : "=a" (res) \
                  : "a" (n), "D" ((long)(a1)), "S" ((long)(a2)), "d" ((long)(a3)), \
                    "r" (r10), "r" (r8) \
                  : "rcx", "r11", "memory"); \
    res; \
})

#define SYSCALL6(n, a1, a2, a3, a4, a5, a6) ({ \
    long res; \
    register long r10 __asm__("r10") = (long)(a4); \
    register long r8  __asm__("r8")  = (long)(a5); \
    register long r9  __asm__("r9")  = (long)(a6); \
    asm volatile ("syscall" \
                  : "=a" (res) \
                  : "a" (n), "D" ((long)(a1)), "S" ((long)(a2)), "d" ((long)(a3)), \
                    "r" (r10), "r" (r8), "r" (r9) \
                  : "rcx", "r11", "memory"); \
    res; \
})

#define SC_EXIT 0
#define SC_WRITE 1
#define STREAM_STDOUT 0

void _start() {
while (1) {}
    const char* msg = "Hello, World!";
    SYSCALL3(SC_WRITE, msg, sizeof(msg), STREAM_STDOUT);
    SYSCALL3(SC_WRITE, msg, sizeof(msg), STREAM_STDOUT);
    SYSCALL1(SC_EXIT, 0);    
}