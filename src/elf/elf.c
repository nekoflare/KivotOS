//
// Created by neko on 6/8/25.
//

#include "elf.h"

#include <string.h>
#include <mem/gheap.h>
#include <mem/physical.h>
#include <mem/virtual.h>
#include <x86/log.h>

uint64_t load_elf(void* elf_file, void* pagemap) {
    Elf64_Ehdr* hdr = (Elf64_Ehdr*)elf_file;

    if (hdr->e_machine != EM_X86_64) {
        debug_print("Invalid machine type: %u\n", hdr->e_machine);
        return 0;
    }

    if (hdr->e_type != ET_EXEC) {
        debug_print("Not an executable ELF\n");
        return 0;
    }

    if (hdr->e_version != EV_CURRENT) {
        debug_print("Unsupported ELF version: %u\n", hdr->e_version);
        return 0;
    }

    uint64_t start_address = hdr->e_entry;
    Elf64_Off phoff = hdr->e_phoff;
    Elf64_Off shoff = hdr->e_shoff;
    Elf64_Half phentsize = hdr->e_phentsize;
    Elf64_Half phnum = hdr->e_phnum;
    Elf64_Half shentsize = hdr->e_shentsize;
    Elf64_Half shnum = hdr->e_shnum;

    //  print phdrs
    debug_print("Program Headers:\n");
    for (Elf64_Half i = 0; i < phnum; i++) {
        Elf64_Phdr* phdr = (Elf64_Phdr*)((uint8_t*)elf_file + phoff + i * phentsize);
        debug_print("PHDR[%u]: Type: 0x%x, VAddr: 0x%lx, PAddr: 0x%lx, FileSz: 0x%lx, MemSz: 0x%lx, Flags: 0x%x, Align: 0x%lx\n",
                    i, phdr->p_type, phdr->p_vaddr, phdr->p_paddr,
                    phdr->p_filesz, phdr->p_memsz, phdr->p_flags, phdr->p_align);
    }

    // print shdrs
    debug_print("Section Headers:\n");
    for (Elf64_Half i = 0; i < shnum; i++) {
        Elf64_Shdr* shdr = (Elf64_Shdr*)((uint8_t*)elf_file + shoff + i * shentsize);
        debug_print("SHDR[%u]: Type: 0x%x, Addr: 0x%lx, Offset: 0x%lx, Size: 0x%lx, Flags: 0x%lx, Align: 0x%lx\n",
                    i, shdr->sh_type, shdr->sh_addr, shdr->sh_offset,
                    shdr->sh_size, shdr->sh_flags, shdr->sh_addralign);
    }

    for (Elf64_Half i = 0; i < phnum; i++) {
        Elf64_Phdr* phdr = (Elf64_Phdr*)((uint8_t*)elf_file + phoff + i * phentsize);

        if (phdr->p_type == PT_LOAD) {
            Elf64_Addr vaddr = phdr->p_vaddr;
            Elf64_Off offset = phdr->p_offset;
            Elf64_Xword filesz = phdr->p_filesz;
            Elf64_Xword memsz = phdr->p_memsz;

            Elf64_Xword aligned_memsz = ALIGN_UP(memsz, 4096);

            for (uint64_t pg = 0; pg * 4096 < aligned_memsz; pg++) {
                uint64_t page = page_alloc();
                vmem_map(pagemap, vaddr + pg * 4096, page, 4096, FLAG_RW | FLAG_US);

                void* dst = (void*)(vaddr + pg * 4096);
                uint64_t page_offset = pg * 4096;
                uint64_t copy_size = 4096;

                if (page_offset + 4096 > filesz) {
                    if (page_offset >= filesz) {
                        // entirely beyond file contents — zero it
                        memset(dst, 0, 4096);
                    } else {
                        // partial overlap — copy part, zero the rest
                        copy_size = filesz - page_offset;
                        memcpy(dst, (uint8_t*)elf_file + offset + page_offset, copy_size);
                        memset((uint8_t*)dst + copy_size, 0, 4096 - copy_size);
                    }
                } else {
                    // fully within file data
                    memcpy(dst, (uint8_t*)elf_file + offset + page_offset, 4096);
                }
            }
        }
    }

    return start_address;
}
