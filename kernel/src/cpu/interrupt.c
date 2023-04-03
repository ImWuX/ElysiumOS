#include "interrupt.h"

#define CS 0x8
#define FLAGS_NORMAL 0x8E
#define FLAGS_TRAP 0x8F
#define IDT_SIZE 256

extern uint64_t isr_stubs[256];
interrupt_irq_eoi_t g_interrupt_irq_eoi;

static interrupt_idt_entry_t g_idt[IDT_SIZE];
static interrupt_entry_t g_entries[IDT_SIZE];

static void set_idt_gate(uint8_t gate, uintptr_t handler, uint16_t segment, uint8_t flags) {
    g_idt[gate].low_offset = (uint16_t) handler;
    g_idt[gate].segment_selector = segment;
    g_idt[gate].ist = 0;
    g_idt[gate].flags = flags;
    g_idt[gate].middle_offset = (uint16_t) (handler >> 16);
    g_idt[gate].high_offset = (uint32_t) (handler >> 32);
    g_idt[gate].rsv0 = 0;
}

void interrupt_initialize() {
    for(int i = 0; i < IDT_SIZE; i++) {
        set_idt_gate(i, (uintptr_t) isr_stubs[i], CS, FLAGS_NORMAL);
        g_entries[i].free = true;
    }

    interrupt_idt_descriptor_t idtr;
    idtr.limit = sizeof(interrupt_idt_entry_t) * IDT_SIZE - 1;
    idtr.base = (uint64_t) &g_idt;
    asm volatile("lidt %0" : : "m" (idtr));
}

void interrupt_handler(interrupt_frame_t *frame) {
    if(!g_entries[frame->int_no].free) g_entries[frame->int_no].handler(frame);
}

void interrupt_set(uint8_t vector, interrupt_priority_t priority, interrupt_handler_t handler) {
    g_entries[vector].free = false;
    g_entries[vector].handler = handler;
    g_entries[vector].priority = priority;
}

int interrupt_request(interrupt_priority_t priority, interrupt_handler_t handler) {
    for(int i = priority << 4; i < IDT_SIZE; i++) {
        if(!g_entries[i].free) continue;
        interrupt_set(i, priority, handler);
        return i;
    }
    return -1;
}

void interrupt_irq_eoi(uint8_t vector) {
    g_interrupt_irq_eoi(vector);
}