/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "nrf24l01.h"
#include "nrf24l01-mnemonics.h"
#include "lfsr.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define HOP_FREQUENCY 250 // minimum is 250
#define HOP_PERIOD (1000000/HOP_FREQUENCY) // hop period in us

//	Used in IRQ ISR
volatile bool nrf24_interrupt_trigger = false;

// configure nrf24
nrf24_config_t nrf24_config = {
	.crc_scheme = CRC_2_BYTE,
	.int_trigger = RX_INTERRUPT,
	.rx_mode = true,
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

bool timer_callback(repeating_timer_t *t) {
    nrf24_switch_channel(lfsr_shift()%125);
    return true;
}

int main(void) {	
	
	//	Initialize UART/USB
	stdio_init_all();

	sleep_ms(5000);

    printf("Starting the RP2040 Rx\r\n");
	
	// Initialize nRF24L01+ and print configuration info
    nrf24_init(10, 11, 12, 9, 8, 13);
	nrf24_configure(&nrf24_config);
    gpio_set_irq_callback(gpio_callback);

	// setup LFSR for FHSS
	lfsr_seed(0x74);
	nrf24_switch_channel(0x74);

	// setup timer for FHSS
	repeating_timer_t timer;
    // cancelled = cancel_repeating_timer(&timer);
    // printf("cancelled... %d\n", cancelled);

    //power up the nrf24
	nrf24_state(POWERUP);

    nrf24_print_config();

    uint8_t data, length, fifo;
    char rx_message[32], tx_message[32];
    bool hop_started;

    //start listening
	nrf24_start_listening();
	
    while (1) {
		
        fifo = 0;
        nrf24_read_register(R_REGISTER | FIFO_STATUS, &fifo, 1);

        // If RX FIFO is full
        if ((fifo & (1 << RX_FULL)) > 0) {
            printf("Receive FIFO full, FIFO_STATUS %02x\r\n", fifo);
            
            nrf24_state(STANDBY1);

            // flush rx fifo
            nrf24_write_register(W_REGISTER | FLUSH_RX, 0, 0);

            nrf24_start_listening();
        }

		if (nrf24_interrupt_trigger) {
            length = nrf24_read_message(rx_message);
            rx_message[length] = 0;  
            printf("Received message: %s\r\n",rx_message);

            if (strstr(rx_message, "Transmitted packet 255")) {
                nrf24_print_config();
                nrf24_switch_channel(lfsr_shift()%125);
            }
            
			//	Message received, print it
			nrf24_interrupt_trigger = false;
		}
    }
}