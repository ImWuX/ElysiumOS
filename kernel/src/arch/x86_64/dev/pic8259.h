#pragma once
#include <stdint.h>

/**
 * @brief Remap the 8529 PIC irqs.
 */
void pic8259_remap();

/**
 * @brief Disable the 8259 PIC (mask all irqs).
 */
void pic8259_disable();

/**
 * @brief Issue an end of interrupt to the 8259 PIC.
 * @param interrupt_vector
 */
void pic8259_eoi(uint8_t interrupt_vector);