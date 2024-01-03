#pragma once
#include <stdint.h>

#define LAPIC_IPI_ASSERT (1 << 14)

/**
 * @brief Initialize and enable the local apic for the current core.
 */
void lapic_initialize();

/**
 * @brief Get the local apic id of the current core.
 * @return local apic id
 */
uint32_t lapic_id();

/**
 * @brief Issue an end of interrupt.
 * @param interrupt_vector
 */
void lapic_eoi(uint8_t interrupt_vector);

/**
 * @brief Issue an IPI.
 * @param lapic_id
 * @param vec interrupt vector and flags
 */
void lapic_ipi(uint32_t lapic_id, uint32_t vec);