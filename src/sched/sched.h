//
// Created by neko on 6/10/25.
//

#ifndef SCHED_H
#define SCHED_H

#include <stdint.h>
#include <x86/idt.h>
#include <rt/mutex.h>

#define CODE_SEG_IDX 3
#define DATA_SEG_IDX 4

enum state {
    READY,
    RUNNING,
    BLOCKED,
    TERMINATED
};

struct vmem_free_area {
    struct vmem_free_area* next;
    uint64_t start;
    uint64_t end;
};

struct proc_used_physical_memory {
    struct proc_used_physical_memory* next;
    uint64_t start;
};

struct task {
    int tid;
    bool is_thread; // in this case it shares most things from the parent like page map.
    int priority;
    enum state state;
    void* pagemap;  // pagemap address in kernel address space
    struct vmem_free_area* vmem_freelist_head; // free virtual memory
    struct proc_used_physical_memory* proc_used_physical_memory_head; // used physical memory
    struct interrupt_frame frame;
    uint64_t used_time;
    uint64_t start_time;
    int parent_tid;

    struct task* next;
    struct mutex proc_mutex; // per-process mutex for multicore safety
};

extern struct task *current_process;

struct task *create_kernel_thread(void (*entry)());
struct task *get_current_process();

uint64_t allocate_virtual_memory_in_process(struct task* task, uint64_t aligned_length);
void free_virtual_memory_in_process(struct task* task, uint64_t start, uint64_t length);
void add_page_to_used_pages(struct task* task, uint64_t page_address);
void remove_used_page_from_process(struct task* task, uint64_t page_address);

struct task *create_user_process(const char *program, const char *args[],
                                 const char *envp[]);
void pause_scheduler();
void unpause_scheduler();
void schedule(struct interrupt_frame* frame);
struct task* get_kernel_process();
void sched_init();

#endif //SCHED_H
