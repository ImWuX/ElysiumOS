#pragma once
#include <stdint.h>
#include <stddef.h>
#include <memory/vmm.h>

/**
 * @brief Initializes the heap.
 * @param address_space
 * @param start start vaddr for the heap
 * @param end end vaddr for the heap
 */
void heap_initialize(vmm_address_space_t *address_space, uintptr_t start, uintptr_t end);

/**
 * @brief Allocate a block of memory in the heap, without an alignment.
 * @param size
 * @return address
 */
void *heap_alloc(size_t size);

/**
 * @brief Allocate a block of memory in the heap, with an alignment.
 * @param size
 * @param alignment
 * @return address
 */
void *heap_alloc_align(size_t size, size_t alignment);

/**
 * @brief Free a block of memory in the heap.
 * @param address
 */
void heap_free(void* address);