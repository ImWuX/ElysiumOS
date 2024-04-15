#include "gdt.h"

#define STR(S) #S
#define XSTR(S) STR(S)

typedef struct {
    uint16_t limit;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t access;
    uint8_t flags;
    uint8_t base_high;
} __attribute__((packed)) gdt_entry_t;

typedef struct {
	gdt_entry_t entry;
	uint32_t base_ext;
	uint8_t rsv0;
	uint8_t zero_rsv1;
	uint16_t rsv2;
} __attribute__((packed)) gdt_system_entry_t;

typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) gdt_descriptor_t;

static gdt_entry_t g_gdt[] = {
    {},
    {
        .limit = 0,
        .base_low = 0,
        .base_mid = 0,
        .access = 0b10011010,
        .flags = 0b00100000,
        .base_high = 0
    },
    {
        .limit = 0,
        .base_low = 0,
        .base_mid = 0,
        .access = 0b10010010,
        .flags = 0b00000000,
        .base_high = 0
    },
    {
        .limit = 0,
        .base_low = 0,
        .base_mid = 0,
        .access = 0b11110010,
        .flags = 0b00000000,
        .base_high = 0
    },
    {
        .limit = 0,
        .base_low = 0,
        .base_mid = 0,
        .access = 0b11111010,
        .flags = 0b00100000,
        .base_high = 0
    },
    {}, {} // TSS
};

void x86_64_gdt_load() {
    gdt_descriptor_t gdtr;
    gdtr.limit = sizeof(g_gdt) - 1;
    gdtr.base = (uint64_t) &g_gdt;
    asm volatile(
        "lgdt %0\n"
        "push $" XSTR(X86_64_GDT_SELECTOR_CODE64_RING0) "\n"
        "lea 1f(%%rip), %%rax\n"
        "push %%rax\n"
        "lretq\n"
        "1:\n"
        "mov $" XSTR(X86_64_GDT_SELECTOR_DATA64_RING0) ", %%rax\n"
        "mov %%rax, %%ds\n"
        "mov %%rax, %%ss\n"
        "mov %%rax, %%es\n"
        : : "m" (gdtr) : "rax", "memory"
    );
}

void x86_64_gdt_load_tss(x86_64_tss_t *tss) {
    uint16_t tss_segment = sizeof(g_gdt) - 16;

    gdt_system_entry_t *entry = (gdt_system_entry_t *) ((uintptr_t) g_gdt + tss_segment);
    entry->entry.access = 0b10001001;
    entry->entry.flags = (1 << 4) | (((uint8_t) (sizeof(x86_64_tss_t) << 16)) & 0b00001111);
    entry->entry.limit = (uint16_t) sizeof(x86_64_tss_t);
    entry->entry.base_low = (uint16_t) (uint64_t) tss;
    entry->entry.base_mid = (uint8_t) ((uint64_t) tss >> 16);
    entry->entry.base_high = (uint8_t) ((uint64_t) tss >> 24);
    entry->base_ext = (uint32_t) ((uint64_t) tss >> 32);

    asm volatile("ltr %0" : : "m" (tss_segment));
}