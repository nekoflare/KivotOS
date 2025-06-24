cmake_minimum_required(VERSION 3.10)

project(KivotOS VERSION 1.0 LANGUAGES C CXX ASM)
enable_language(ASM_NASM)

set(ENABLE_FUNCTION_INSTRUMENTING false)

# Compiler configuration
# set(CMAKE_C_COMPILER /usr/bin/x86_64-linux-gnu-gcc-11)
# set(CMAKE_CXX_COMPILER /usr/bin/x86_64-linux-gnu-g++-11)
set(CMAKE_ASM_NASM_COMPILER /usr/bin/nasm)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(UBSAN_FLAGS "-fsanitize=undefined -fsanitize=alignment -fsanitize=bool -fsanitize=bounds -fsanitize=builtin -fsanitize=enum -fsanitize=float-cast-overflow -fsanitize=float-divide-by-zero -fsanitize=integer-divide-by-zero -fsanitize=null -fsanitize=object-size -fsanitize=pointer-overflow -fsanitize=return -fsanitize=shift -fsanitize=signed-integer-overflow -fsanitize=unreachable -fsanitize=vla-bound -fsanitize=vptr")

# Common compiler flags
set(COMMON_FLAGS "${UBSAN_FLAGS} -fno-builtin -fno-stack-protector -fno-stack-check -fno-plt -fno-pic -ffreestanding -mcmodel=kernel -march=x86-64 -m64 -mno-red-zone -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0 -Wall -Wextra -Wformat -Wformat-security -Wall -Wextra -Wshadow -Wstrict-prototypes -fno-strict-aliasing -O2 -g")

if (ENABLE_FUNCTION_INSTRUMENTING)
    set(COMMON_FLAGS "${COMMON_FLAGS} -finstrument-functions")
endif ()

# C flags
set(CMAKE_C_FLAGS "${COMMON_FLAGS} -lgcc -static -msse -mmmx -msse2")

# C++ flags
set(CMAKE_CXX_FLAGS "${COMMON_FLAGS} -fpermissive -fno-rtti -fno-exceptions -fno-threadsafe-statics -std=c++20 -lgcc -static -msse -mmmx -msse2 -msse3 -mno-ssse3 -mno-avx -mno-avx2 -mno-avx512f")

# Linker flags
set(CMAKE_EXE_LINKER_FLAGS "-nostdlib -static -z text -T ../x86_64-linker.ld -Wl,--export-dynamic -z max-page-size=0x1000")

# NASM flags
set(CMAKE_ASM_NASM_FLAGS "-f elf64")

include(uACPI/uacpi.cmake)

include_directories(src)
include_directories(${UACPI_INCLUDES})

add_executable(KivotOS
        ${UACPI_SOURCES}
        src/init/init.asm
        src/init/early.c
        src/x86/gdt.c
        src/x86/gdt.h
        src/x86/load_gdt.asm
        src/x86/io.h
        src/rt/string.c
        src/x86/log.c
        src/x86/log.h
        src/x86/idt.c
        src/x86/idt.h
        src/x86/exception_handlers.asm
        src/x86/load_idt.asm
        src/x86/load_tss.asm
        src/rt/memory.c
        src/rt/nanoprintf.h
        src/mem/physical.c
        src/mem/physical.h
        src/mem/virtual.c
        src/mem/virtual.h
        src/vfs/vfs.c
        src/vfs/vfs.h
        src/mem/gheap.c
        src/mem/gheap.h
        src/mem/dlmalloc.c
        src/mem/dlmalloc.h
        src/rt/printf.c
        src/rt/errno.c
        src/elf/elf.c
        src/elf/elf.h
        src/x86/msr.h
        src/acpi/acpi.c
        src/acpi/acpi.h
        src/x86/apic.c
        src/x86/apic.h
        src/rt/mutex.c
        src/acpi/uacpi_glue.c
        src/acpi/uacpi_io.c
        src/sched/sched.c
        src/syscalls/syscalls.h
        src/syscalls/syscall_system.c
        src/syscalls/syscall_system.h
        src/syscalls/syscall_handler.asm
        src/sched/tls.c
        src/rt/instrumenting.c
        src/rt/instrumenting.h
        src/rt/kallsyms.c
        src/rt/kallsyms.h
        src/rt/mutex.h
        src/sched/sched.h
        src/x86/interrupts_helpers.h
        src/rt/ubsan.c
        src/syscalls/syscalls/write.c
)


file(GLOB APP_DIRS "../apps/*")

add_custom_target(CompileApps
        COMMENT "Compiling applications in apps directory"
)

foreach (APP_DIR ${APP_DIRS})
    if (IS_DIRECTORY ${APP_DIR})
        get_filename_component(APP_NAME ${APP_DIR} NAME)
        add_custom_command(
                TARGET CompileApps
                COMMAND sh compile.sh
                WORKING_DIRECTORY ${APP_DIR}
                COMMENT "Compiling ${APP_NAME}"
        )
    endif ()
endforeach ()

add_custom_target(PostBuild ALL
        COMMAND ${CMAKE_COMMAND} -E rename KivotOS kernel.elf
        COMMAND chmod +x ../iso.sh
        COMMAND ../iso.sh
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Post-build: Renaming KivotOS to kernel.elf and invoking iso.sh"
        DEPENDS KivotOS CompileApps
)

# Launch QEMU
add_custom_target(PerformAll ALL
    COMMAND qemu-system-x86_64
        -cpu qemu64,+fsgsbase,+syscall
        -M smm=off
        -smp 6
        -M q35
        -m 2G
        -enable-kvm
        -debugcon stdio
        -cdrom image.iso
        -drive if=pflash,unit=0,format=raw,file=../ovmf/ovmf-code-x86_64.fd,readonly=on
        -drive if=pflash,unit=1,format=raw,file=../ovmf/ovmf-vars-x86_64.fd
    DEPENDS PostBuild
    USES_TERMINAL
)
