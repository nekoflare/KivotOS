gcc -nostdlib -ffreestanding -mno-sse -mno-avx -m64 -mno-red-zone -fno-stack-check -fno-stack-protector main.c -c -o main.o
ld main.o ../../mlibc/build/sysdeps/kivotos/*.o ../../mlibc/build/libc.a -nostdlib -o main
    