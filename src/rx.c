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

//	Used in IRQ ISR
volatile bool nrf24_interrupt_trigger = false;

// configure nrf24
nrf24_config_t nrf24_config = {
	.crc_scheme = CRC_2_BYTE,
	.int_trigger = RX_INTERRUPT,
	.rx_mode = true,
	.auto_ack = true,
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

int main(void) {	
	
	//	Initialize UART/USB
	stdio_init_all();

	sleep_ms(5000);

    printf("Starting the RP2040 Rx\r\n");
	
	// Initialize nRF24L01+ and print configuration info
    nrf24_init(10, 11, 12, 9, 8, 13);
	nrf24_configure(&nrf24_config);
    gpio_set_irq_callback(gpio_callback);


	// setup the first channel
	lfsr_seed(0x74);
	nrf24_switch_channel(0x74);

    //power up the nrf24
	nrf24_state(POWERUP);

    nrf24_print_config();

    uint8_t data, length, fifo;
    char rx_message[32], tx_message[32];

    // pre load a ACK message into the fifo
    snprintf(tx_message, 32, "ACK: %d", rpd_status());
    nrf24_write_ack(tx_message, strlen(tx_message));

    //start listening
	nrf24_start_listening();
	
    while (1) {
		
        fifo = 0;
        nrf24_read_register(R_REGISTER | FIFO_STATUS, &fifo, 1);

        // If RX FIFO is full
        if ((fifo & (1 << RX_FULL)) > 0) {
            printf("Receive FIFO full, FIFO_STATUS %02x\r\n", fifo);
            
            // if tx fifo is empty and the rx fifo is full, it is probably because 
            // tx fifo didn't have any ack packets to send but it kept receiving new packets
            if ((fifo & (1 << TX_EMPTY)) > 0) {
                nrf24_state(STANDBY1);

                // flush rx fifo
                nrf24_write_register(W_REGISTER | FLUSH_RX, 0, 0);

                // pre load a ACK message into the tx fifo
                snprintf(tx_message, 32, "ACK: %d", rpd_status());
                nrf24_write_ack(tx_message, strlen(tx_message));

                nrf24_start_listening();
            }
        }

		if (nrf24_interrupt_trigger) {

            data = 0;
            nrf24_read_register(R_REGISTER | STATUS, &data, 1);

            // handle receive interrupt
            if ((data & (1 << RX_DR)) > 0) {
                // pre load a ACK message in fifo for the next receive
                snprintf(tx_message, 32, "ACK: %d", rpd_status());
                nrf24_write_ack(tx_message, strlen(tx_message));
                
                length = nrf24_read_message(rx_message);
                rx_message[length] = 0;  
                printf("Received message: %s\r\n",rx_message);

                // flush rx fifo
                nrf24_write_register(W_REGISTER | FLUSH_RX, 0, 0);

                // nrf24_switch_channel((lfsr_shift() & 0x7f) % 125);
            }
            
            // handle transmit interrupt
            // if ((data & (1 << TX_DS)) > 0) {

            // }
            
            // // handle maximum retransimissions interrupt
            // if ((data & (1 << MAX_RT)) > 0){

            // }
            
			//	Message received, print it
			nrf24_interrupt_trigger = false;
			
		}
    }
}