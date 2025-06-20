//
// Created by neko on 6/8/25.
//

#ifndef ELF_H
#define ELF_H

#include <elf.h>

uint64_t load_elf(void* elf_file, void* pagemap);

#endif //ELF_H
