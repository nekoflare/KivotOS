#include "vfs.h"

#include <x86/log.h>

volatile struct limine_module_request module_request = {
    .id = LIMINE_MODULE_REQUEST,
    .revision = LIMINE_API_REVISION,
    .response = NULL,
    .internal_module_count = 0,
    .internal_modules = NULL
};

void vfs_init() {
    if (module_request.response == NULL) {
        debug_print("No modules found.\n");
        return;
    }

    uint64_t module_count        = module_request.response->module_count;
    struct limine_file** modules = module_request.response->modules;
    for (uint64_t i = 0; i < module_count; i++) {
        struct limine_file* module = modules[i];

        debug_print("%s\n", module->path);
    }
}

int open (const char *file, int oflag, ...) {
    if (module_request.response == NULL) {
        debug_print("No modules found.\n");
        return;
    }

    uint64_t module_count        = module_request.response->module_count;
    struct limine_file** modules = module_request.response->modules;
    for (uint64_t i = 0; i < module_count; i++) {
        struct limine_file* module = modules[i];

        if (strcmp(module->path, file) == 0) {
            return (int)i;
        }
    }

    return -1;
}

int read(int __fd, void *__buf, size_t __nbytes) {
    struct limine_file* file = (struct limine_file*)(module_request.response->modules[__fd]);

    memcpy(__buf, file->address, __nbytes);
}

int fstat(int __fd, struct stat *__buf) {
    struct limine_file* file = (struct limine_file*)(module_request.response->modules[__fd]);
    __buf->st_size = file->size;
}
