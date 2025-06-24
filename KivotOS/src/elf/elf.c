//
// Created by neko on 6/8/25.
//

#include "elf.h"

#include <string.h>
#include <mem/gheap.h>
#include <mem/physical.h>
#include <mem/virtual.h>
#include <x86/log.h>
#include <mem/virtual.h>

#define PAGE_SIZE 4096

uint64_t load_elf(void* elf_file, void* pagemap) {
    void *old_pagemap = get_current_pagemap();
    load_pagemap(pagemap);

    Elf64_Ehdr *hdr = (Elf64_Ehdr *) elf_file;

    if (hdr->e_machine != EM_X86_64) {
        debug_print("Invalid machine type: %u\n", hdr->e_machine);
        load_pagemap(old_pagemap);
        return 0;
    }

    if (hdr->e_type != ET_EXEC) {
        debug_print("Not an executable ELF\n");
        load_pagemap(old_pagemap);
        return 0;
    }

    if (hdr->e_version != EV_CURRENT) {
        debug_print("Unsupported ELF version: %u\n", hdr->e_version);
        load_pagemap(old_pagemap);
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
            Elf64_Addr aligned_vaddr = ALIGN_DOWN(phdr->p_vaddr, PAGE_SIZE);
            Elf64_Off aligned_offset = ALIGN_DOWN(phdr->p_offset, PAGE_SIZE);
            uint64_t vaddr_offset = phdr->p_vaddr - aligned_vaddr;
            Elf64_Xword total_memsz = ALIGN_UP(vaddr_offset + phdr->p_memsz, PAGE_SIZE);
            Elf64_Xword total_filesz = vaddr_offset + phdr->p_filesz;

            for (uint64_t pg = 0; pg * PAGE_SIZE < total_memsz; pg++) {
                uint64_t page = allocate_physical_page();
                vmem_map(pagemap, aligned_vaddr + pg * PAGE_SIZE, page, PAGE_SIZE, FLAG_RW | FLAG_US);

                void* dst = (void*)(aligned_vaddr + pg * PAGE_SIZE);
                uint64_t page_offset = pg * PAGE_SIZE;

                if (page_offset + PAGE_SIZE > total_filesz) {
                    if (page_offset >= total_filesz) {
                        // entirely beyond file contents — zero it
                        memset(dst, 0, PAGE_SIZE);
                    } else {
                        // partial overlap — copy part, zero the rest
                        uint64_t copy_size = total_filesz - page_offset;
                        memcpy(dst, (uint8_t*)elf_file + aligned_offset + page_offset, copy_size);
                        memset((uint8_t*)dst + copy_size, 0, PAGE_SIZE - copy_size);
                    }
                } else {
                    // fully within file data
                    memcpy(dst, (uint8_t*)elf_file + aligned_offset + page_offset, PAGE_SIZE);
                }
            }
        }
    }

    load_pagemap(old_pagemap);
    return start_address;
}
