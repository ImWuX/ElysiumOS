#pragma once
#include <stdint.h>
#include <stddef.h>
#include <arch/types.h>
#include <memory/pmm.h>
#include <lib/slock.h>
#include <lib/list.h>

typedef enum {
    VMM_PROT_WRITE = (1 << 0),
    VMM_PROT_EXEC = (1 << 1),
    VMM_PROT_USER = (1 << 2)
} vmm_prot_t;

typedef struct {
    struct vmm_address_space *address_space;
    uintptr_t base;
    size_t length;
    int protection;
    struct vmm_segment_ops *ops;
    void *data;
    list_t list;
} vmm_segment_t;

typedef struct vmm_segment_ops {
    int (* map)(vmm_segment_t *segment, uintptr_t base, size_t length);
    int (* unmap)(vmm_segment_t *segment, uintptr_t base, size_t length);
    bool (* fault)(vmm_segment_t *segment, uintptr_t address);
    void (* free)(vmm_segment_t *segment);
} vmm_segment_ops_t;

typedef struct vmm_address_space {
    slock_t lock;
    list_t segments;
} vmm_address_space_t;

extern vmm_address_space_t *g_kernel_address_space;

int vmm_map(vmm_segment_t *segment);

int vmm_unmap(vmm_address_space_t *as, uintptr_t vaddr, size_t length);

int vmm_map_anon(vmm_address_space_t *as, uintptr_t vaddr, size_t length, int prot, bool wired);

int vmm_map_direct(vmm_address_space_t *as, uintptr_t vaddr, size_t length, int prot, uintptr_t paddr);

bool vmm_fault(vmm_address_space_t *as, uintptr_t address);