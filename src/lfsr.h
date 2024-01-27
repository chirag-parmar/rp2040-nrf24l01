#ifndef _LFSR_H_
#define _LFSR_H_

#include <stdint.h>

// x^8 + x^4 + x^3 + x^2 + 1
#define FEEDBACK_POLYNOMIAL 0b00011101

void lfsr_seed(uint8_t seed);
uint8_t lfsr_shift();

#endif