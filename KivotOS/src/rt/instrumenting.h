//
// Created by neko on 6/23/25.
//

#ifndef INSTRUMENTING_H
#define INSTRUMENTING_H

#include <x86/idt.h>

void pause_logging_calls();
void resume_logging_calls();
void print_call_log();
void print_stack_and_heap_info(struct interrupt_frame* frame);

#endif //INSTRUMENTING_H
