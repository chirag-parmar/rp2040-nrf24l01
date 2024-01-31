/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "nrf24l01.h"
#include "nrf24l01-mnemonics.h"
#include "lfsr.h"

#include <stdbool.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <time.h>


#define HOP_FREQUENCY 50 // minimum is 250
#define HOP_PERIOD (1000000/HOP_FREQUENCY)

//	Used in IRQ ISR
volatile bool nrf24_interrupt_trigger = false;
volatile bool hop_flag = false;

// configure nrf24
nrf24_config_t nrf24_config = {
	.crc_scheme = CRC_2_BYTE,
	.int_trigger = TX_INTERRUPT, 
	.rx_mode = false,
	.auto_ack = false,
	.retr_count = 1,
	.retr_delay = RETR_DELAY_4000_US,
	.datarate = DATARATE_250Kbps,
	.power = PWR_0_DBM,
	.cont_wave = false,
	.dynamic_payload = true,
	.rx_address = {0xe7, 0xe7, 0xe7, 0xe7, 0xe7},
	.tx_address = {0xe7, 0xe7, 0xe7, 0xe7, 0xe7},
	.enable_pipe_bit_mask = 0x01
};

void gpio_callback(uint gpio, uint32_t events) {
	if(gpio == 13 && (events & GPIO_IRQ_EDGE_FALL) > 0) nrf24_interrupt_trigger = true;
}

bool timer_callback(struct repeating_timer *t) {
	hop_flag = true;
	return true;
}

int main(void) {	
	
	//	Initialize UART/USB
	stdio_init_all();

	sleep_ms(5000);

    printf("Starting the RP2040 Tx\r\n");
	
	// Initialize nRF24L01+ and print configuration info
    nrf24_init(10, 11, 12, 9, 8, 13);
	nrf24_configure(&nrf24_config);
	gpio_set_irq_callback(gpio_callback);

	// setup LFSR for FHSS
	lfsr_seed(0x74);
	nrf24_switch_channel(0x74);

	// setup timer for FHSS
	struct repeating_timer timer;
    // cancelled = cancel_repeating_timer(&timer);
    // printf("cancelled... %d\n", cancelled);

	// power up the nrf24
	nrf24_state(POWERUP);

    nrf24_print_config();

    uint8_t data, length, iterator = 0;
    char tx_message[32], rx_message[32];
	bool hop_started = false;
	// uint64_t start_time, ping;

	// send the first packet
	snprintf(tx_message, 32, "Transmitted packet %d", iterator);	// Copy string into array
	nrf24_send_message(tx_message, strlen(tx_message), nrf24_config.auto_ack);
	
    while (1) {
		
		if (nrf24_interrupt_trigger) {
			if (!hop_started) {
				add_repeating_timer_us((-1)*HOP_PERIOD, timer_callback, NULL, &timer);
                hop_started = true;
			}

			if (hop_flag) {
				nrf24_switch_channel(lfsr_shift()%125);
				hop_flag = false;
			}

			printf("Message %d sent\r\n", iterator);

			// increment the iterator 
			iterator++;

			snprintf(tx_message, 32, "Transmitted packet %d", iterator);	// Copy string into array
			nrf24_send_message(tx_message, strlen(tx_message), nrf24_config.auto_ack);
            
			// set flag to false
			nrf24_interrupt_trigger = false;
		}
    }
}