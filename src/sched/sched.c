//
// Created by neko on 6/10/25.
//

#include "sched.h"

#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <elf/elf.h>
#include <mem/physical.h>
#include <mem/virtual.h>
#include <x86/apic.h>
#include <x86/log.h>
#include <rt/mutex.h>
#include <rt/nanoprintf.h>
#include <sys/stat.h>
#include <vfs/vfs.h>

#include "tls.h"

// empty kernel process that will be populated on first scheduling event
struct task kernel_process = {
    .tid = 0,
    .is_thread = false,
    .priority = 1000,
    .state = RUNNING,
    .pagemap = NULL,
    .vmem_freelist_head = NULL, // we use internal functions for vmem.
    .proc_used_physical_memory_head = NULL, // we use internal functions for pmem handling
    .frame = {}, // no interrupt frame yet.
    .used_time = 0,
    .start_time = 0,
    .parent_tid = -1, // no parent pid.
    .next = NULL,
    .proc_mutex = {0},
    .invocation = "/boot/kernel.elf"
};

struct task* processes_linked_list = &kernel_process;
struct task* current_process = &kernel_process;
int processes = 1;
int tid = 1;
static struct mutex sched_mutex = {0};

bool schedule_processes = false;

int allocate_tid() {
    static struct mutex allocate_tid_mutex = {0};
    int new_tid = tid ++;
    return new_tid;
}


void clock_handler(struct interrupt_frame* frame) {
    if (!schedule_processes) {
        return;
    }

    if (processes <= 1) {
        return;
    }

    schedule(frame);
}

struct task * get_kernel_process() {
    return &kernel_process;
}

uint64_t get_current_cr3() {
    uint64_t cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    return cr3;
}

uint64_t get_current_cr2() {
    uint64_t cr2;
    asm volatile("mov %%cr2, %0" : "=r"(cr2));
    return cr2;
}

uint64_t get_current_cr0() {
    uint64_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    return cr0;
}

uint64_t get_current_cr4() {
    uint64_t cr4;
    asm volatile("mov %%cr4, %0" : "=r"(cr4));
    return cr4;
}

struct task* create_process(int pid, int priority, void (*entry)(void)) {
    lock_mutex(&sched_mutex);
    struct task* proc = malloc(sizeof(struct task));
    if (!proc) {
        unlock_mutex(&sched_mutex);
        return NULL;
    }

    memset(proc, 0, sizeof(struct task));
    proc->tid = pid;
    proc->is_thread = false;
    proc->priority = priority;
    proc->state = READY;
    proc->parent_tid = 0;
    proc->proc_mutex.locked = 0;

    uint8_t* stack = malloc(32 * 1024); // stack is in kernel memory for now. but you know we use cr3 of the kernel anyway.
    if (!stack) {
        free(proc);
        unlock_mutex(&sched_mutex);
        return NULL;
    }

    uint64_t stack_top_addr = (uint64_t)(stack + 32 * 1024);
    stack_top_addr &= ~(0xFULL);
    proc->frame.rip = (uint64_t)entry;
    proc->frame.cs = 0x08;
    proc->frame.ds = 0x10;
    proc->frame.es = 0x10;
    proc->frame.fs = 0x10;
    proc->frame.gs = 0x10;
    proc->frame.ss = 0x10;
    proc->frame.rflags = 0x202;      // todo: maybe borrow rflags from our current process?
    proc->frame.orig_rsp = stack_top_addr;

    // insert into process list
    proc->next = processes_linked_list;
    processes_linked_list = proc;
    processes++;

    unlock_mutex(&sched_mutex);
    return proc;
}

struct task *create_kernel_thread(void (*entry)()) {
    lock_mutex(&sched_mutex);
    struct task* proc = malloc(sizeof(struct task));
    if (!proc) {
        unlock_mutex(&sched_mutex);
        return NULL;
    }

    memset(proc, 0, sizeof(struct task));
    proc->tid = allocate_tid();
    proc->is_thread = true;
    proc->priority = 1000;
    proc->state = READY;
    proc->parent_tid = 0; // kernels TID = 0
    proc->proc_mutex.locked = 0; // initialize mutex

    uint8_t* stack = malloc(32 * 1024); // stack is in kernel memory for now. but you know we use cr3 of the kernel anyway.
    if (!stack) {
        free(proc);
        unlock_mutex(&sched_mutex);
        return NULL;
    }

    uint64_t stack_top_addr = (uint64_t)(stack + 32 * 1024);
    stack_top_addr &= ~(0xFULL);

    proc->frame.rip = (uint64_t)entry;
    proc->frame.cs = 0x08;
    proc->frame.ds = 0x10;
    proc->frame.es = 0x10;
    proc->frame.fs = 0x10;
    proc->frame.gs = 0x10;
    proc->frame.ss = 0x10;
    proc->frame.rflags = 0x202;
    proc->frame.orig_rsp = stack_top_addr;
    proc->frame.ss = 0x10;           // kernel stack segment

    // Insert into process list
    proc->next = processes_linked_list;
    processes_linked_list = proc;
    processes++;

    unlock_mutex(&sched_mutex);
    return proc;
}

void destroy_kernel_thread(struct task* task) {
    if (!task || task == &kernel_process) {
        return;
    }

    lock_mutex(&sched_mutex);

    // Remove from process list
    struct task *current = processes_linked_list;
    struct task *prev = NULL;

    while (current != NULL) {
        if (current == task) {
            if (prev) {
                prev->next = current->next;
            } else {
                processes_linked_list = current->next;
            }
            break;
        }
        prev = current;
        current = current->next;
    }


    free(task);
    processes--;

    unlock_mutex(&sched_mutex);
}

struct task * get_current_process() {
    return current_process;
}

struct task *create_user_process(const char *program, const char *args[],
                                 const char *envp[], const char *invocation_string, int parent_tid, int priority) {
    pause_scheduler();

    extern volatile bool test;
    // calculate the space needed for the argv and envp, calculate argc too

    int argc = 0;

    if (args != NULL) {
        while (args[argc] != NULL) {
            argc++;
        }
    }

    // calculate the space needed on the user memory space for args.
    int argv_space = 0;
    for (int i = 0; argc > i; i++) {
        argv_space += strlen(args[i]) + 1; // +1 for the null terminator
    }

    struct task* task = malloc(sizeof(struct task));

    memset(task, 0, sizeof(struct task));
    task->tid = allocate_tid();
    task->is_thread = false; // i aint no thread cuh
    task->priority = priority;
    task->state = READY;
    task->parent_tid = parent_tid;
    task->proc_mutex.locked = 0; // initialize mutex

    task->invocation = strdup(invocation_string);

    struct vmem_free_area* vmem_area = malloc(sizeof(struct vmem_free_area));

    memset(vmem_area, 0, sizeof(struct vmem_free_area));

    vmem_area->start = 0x0;
    vmem_area->end = 0x800000000000;
    vmem_area->next = NULL;

    task->vmem_freelist_head = vmem_area;

    debug_print("our cr3: %016lX\n", get_current_cr3());

    // Set up process frame

    task->frame.rip = 0; // Will be set by ELF loader
    task->frame.cs = (CODE_SEG_IDX * 0x8) | 3;
    task->frame.ds = (DATA_SEG_IDX * 0x8) | 3;
    task->frame.es = (DATA_SEG_IDX * 0x8) | 3;
    task->frame.fs = (DATA_SEG_IDX * 0x8) | 3;
    task->frame.gs = (DATA_SEG_IDX * 0x8) | 3;
    task->frame.ss = (DATA_SEG_IDX * 0x8) | 3;
    task->frame.rflags = 0x202; // rsvd + IF

    // paging setup
    uint64_t page_table_addr = page_alloc();
    uint64_t page_table_addr_virtual = page_table_addr + get_hhdm_slide();

    uint64_t page_table_flags = 0; // not used uwu

    memset((void*)(page_table_addr_virtual), 0, 4096);

    // copy higher 2048 bytes of the kernel page map to this page map.
    memcpy((void*)(page_table_addr + get_hhdm_slide() + 2048), (void*)((uintptr_t)get_kernel_pagemap() + 2048), 2048);

    add_page_to_used_pages(task, page_table_addr);

    uint64_t stack_size = 2 * 4096;

    // but first allocate the vmem within the task for the stack..
    uint64_t stack_vmem_start = allocate_virtual_memory_in_process(task, stack_size);
    uint64_t stack_vmem_end = stack_vmem_start + stack_size;

    task->pagemap = (void*) page_table_addr;

    // now allocate pages and map them in.
    for (int page = 0; stack_size / 4096 > page; page++) {
        uint64_t stack_page = page_alloc();
        // add to the used pages
        add_page_to_used_pages(task, stack_page);

        // map it in using the task's page map.
        vmem_map((void*)(page_table_addr), stack_vmem_start + page * 4096, stack_page, 4096, FLAG_RW | FLAG_US | FLAG_NX); // read write, user, no execute.
    }

    task->frame.orig_rsp = stack_vmem_end - 16;

    // load elf...

    int elf_fd = open(program, 0);
    if (elf_fd < 0) {
        debug_print("Error opening file: %s\n", program);
        return NULL; // leaking memory.. and also not unpausing scheduler..
    }

    // get the file size nao.
    struct stat stat_buf;
    fstat(elf_fd, &stat_buf);

    size_t file_size = stat_buf.st_size;

    // create elf_file buffer.
    char* elf_file = malloc(file_size);

    // read the file into the buffer.
    read(elf_fd, elf_file, file_size);

    task->frame.rip = load_elf(elf_file, (void*)(page_table_addr));

    debug_print("RIP = %016lX\n", task->frame.rip);

    // Add to process list
    task->next = processes_linked_list;
    processes_linked_list = task;
    processes++;

    unpause_scheduler();
    return task;
}

void pause_scheduler() {
    schedule_processes = false;
}

void unpause_scheduler() {
    schedule_processes = true;
}

uint64_t allocate_virtual_memory_in_process(struct task *task,
                                            uint64_t aligned_length) {
    struct vmem_free_area *current = task->vmem_freelist_head;
    struct vmem_free_area *prev = NULL;

    while (current != NULL) {
        if (current->end - current->start >= aligned_length) {
            uint64_t allocated_address = current->start;
            current->start += aligned_length;

            if (current->start >= current->end) {
                if (prev) {
                    prev->next = current->next;
                } else {
                    task->vmem_freelist_head = current->next;
                }
                free(current);
            }

            return allocated_address;
        }
        prev = current;
        current = current->next;
    }
    return 0;
}

void free_virtual_memory_in_process(struct task *task, uint64_t start,
                                    uint64_t length) {
    struct vmem_free_area *new_area = malloc(sizeof(struct vmem_free_area));
    new_area->start = start;
    new_area->end = start + length;
    new_area->next = NULL;

    if (task->vmem_freelist_head == NULL) {
        task->vmem_freelist_head = new_area;
        return;
    }

    struct vmem_free_area *current = task->vmem_freelist_head;
    struct vmem_free_area *prev = NULL;

    while (current != NULL && current->start < start) {
        prev = current;
        current = current->next;
    }

    if (prev) {
        prev->next = new_area;
    } else {
        task->vmem_freelist_head = new_area;
    }
    new_area->next = current;
}

void add_page_to_used_pages(struct task *task, uint64_t page_address) {
    struct proc_used_physical_memory *new_page = malloc(
        sizeof(struct proc_used_physical_memory));
    new_page->start = page_address;
    new_page->next = task->proc_used_physical_memory_head;
    task->proc_used_physical_memory_head = new_page;
}

void remove_used_page_from_process(struct task *task, uint64_t page_address) {
    struct proc_used_physical_memory *current = task->proc_used_physical_memory_head;
    struct proc_used_physical_memory *prev = NULL;

    while (current != NULL) {
        if (current->start == page_address) {
            if (prev) {
                prev->next = current->next;
            } else {
                task->proc_used_physical_memory_head = current->next;
            }
            free(current);
            return;
        }
        prev = current;
        current = current->next;
    }
}

void schedule(struct interrupt_frame *frame) {
    lock_mutex(&sched_mutex);

    // lock current process before modifying its frame/state
    lock_mutex(&current_process->proc_mutex);
    current_process->frame = *frame;

    // save current CR3 value to current process if it's not kernel process
    if (current_process != &kernel_process) {
        uint64_t cr3;
        asm volatile("mov %%cr3, %0" : "=r"(cr3));
        current_process->pagemap = (void *) cr3;
    }


    // Find next ready process (round-robin)
    struct task* next = current_process->next;
    while (1) {
        if (next == NULL) {
            next = processes_linked_list;
        }
        if (next->state == READY) {
            break;
        }
        if (next == current_process) {
            break;
        }
        next = next->next;
    }

    // lock next process before switching
    lock_mutex(&next->proc_mutex);

    // update process states
    current_process->state = READY;
    next->state = RUNNING;

    // unlock current process after state update
    unlock_mutex(&current_process->proc_mutex);

    // update current process
    current_process = next;

    // load next process's pagemap if it's not kernel process
    if (next != &kernel_process && next->pagemap != NULL) {
        asm volatile("mov %0, %%cr3" : : "r"((uint64_t) next->pagemap) :
            "memory");
    }

    // restore next process's context
    memcpy(frame, &next->frame, sizeof(struct interrupt_frame));

    unlock_mutex(&next->proc_mutex);
    unlock_mutex(&sched_mutex);
}

void sched_init() {
    unlock_mutex(&sched_mutex);

    kernel_process.state = BLOCKED;

    debug_print("Scheduling init done.\nRegistering timer handler.\n");
    set_lapic_secondary_timer_handler(clock_handler); // bam, sched me up babee
}

static void print_header() {
    debug_print("%-*s|%-*s|%-*s|%-*s|%-*s|%-*s\n",
                COL_WIDTH_TID, "TID",
                COL_WIDTH_TYPE, "Type",
                COL_WIDTH_PRIORITY, "Priority",
                COL_WIDTH_STATE, "State",
                COL_WIDTH_PARENT, "Parent",
                COL_WIDTH_INVOCATION, "Command");

    for (int i = 0; i < TOTAL_WIDTH; i++) {
        debug_print("-");
    }
    debug_print("\n");
}

static void print_process(struct task *proc) {
    const char *type = proc->is_thread ? "Thread" : "Process";
    const char *state;
    switch (proc->state) {
        case READY: state = "Ready";
            break;
        case RUNNING: state = "Running";
            break;
        case BLOCKED: state = "Blocked";
            break;
        default: state = "Unknown";
            break;
    }

    debug_print("%-*d|%-*s|%-*d|%-*s|%-*d|%-*s\n",
                COL_WIDTH_TID, proc->tid,
                COL_WIDTH_TYPE, type,
                COL_WIDTH_PRIORITY, proc->priority,
                COL_WIDTH_STATE, state,
                COL_WIDTH_PARENT, proc->parent_tid,
                COL_WIDTH_INVOCATION, proc->invocation ? proc->invocation : "<missing invocation command>");
}

void print_processes() {
    disable_printing_time();
    print_header();

    struct task *current = processes_linked_list;
    while (current != NULL) {
        print_process(current);
        current = current->next;
    }

    enable_printing_time();
}
