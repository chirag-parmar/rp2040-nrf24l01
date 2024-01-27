#include <stdint.h>
#include <stdio.h>

#include "lfsr.h"

static uint8_t lfsr_state = 0;

void lfsr_seed(uint8_t seed) {
    lfsr_state = seed;
}

uint8_t lfsr_shift() {
    uint8_t shift_bit = lfsr_state & 0x80;
    lfsr_state = lfsr_state << 1;
    if (shift_bit > 0) lfsr_state = lfsr_state ^ FEEDBACK_POLYNOMIAL;

    return lfsr_state;
}

// quick test written to check if the constructed LFSR spans the entire extension field
// int main() {
//     uint8_t num, span_check[256];
//     lfsr_seed(33);

//     for (int i = 0 ; i < 256; i++) {
//         span_check[i] = 0;
//     }

//     for (int i = 1; i < 256; i++) {
//         num = lfsr_shift();
//         span_check[num] += 1;
//         // printf("num: %d\r\n", num);
//     }

//     for (int i = 1; i < 256; i++) {
//         if (span_check[i] == 0) {
//             printf("all numbers were not spanned: %d\r\n", i);
//         } else if (span_check[i] > 1) {
//             printf("generated more than once: %d\r\n", i);
//         }
//     }

//     return 0;
// }