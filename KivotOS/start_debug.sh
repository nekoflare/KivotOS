#!/bin/bash

qemu-system-x86_64 \
    -cpu qemu64,+fsgsbase,+syscall \
    -M smm=off \
    -smp 6 \
    -M q35 \
    -m 2G \
    -debugcon stdio \
    -cdrom image.iso \
    -drive if=pflash,unit=0,format=raw,file=../ovmf/ovmf-code-x86_64.fd,readonly=on \
    -drive if=pflash,unit=1,format=raw,file=../ovmf/ovmf-vars-x86_64.fd \
    -s -S &

sleep 1

gdb -x gdbinit kernel.elf
