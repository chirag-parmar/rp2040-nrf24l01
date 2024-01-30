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

#define MAX_LOST_TRANSMISSIONS 100
#define INTER_PACKET_DELAY 1
#define HOP_FREQUENCY 250 // minimum is 250

//	Used in IRQ ISR
volatile bool nrf24_interrupt_trigger = false;

// configure nrf24
nrf24_config_t nrf24_config = {
	.crc_scheme = CRC_2_BYTE,
	// would traditionally be `|` but since this particular bit is activelow/inverted it is a `&`
	.int_trigger = TX_INTERRUPT & RT_INTERRUPT, 
	.rx_mode = false,
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

clock_t clock() {
    return (clock_t) time_us_64();
}

void gpio_callback(uint gpio, uint32_t events) {
	if(gpio == 13 && (events & GPIO_IRQ_EDGE_FALL) > 0) nrf24_interrupt_trigger = true;
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

	// setup the first channel
	lfsr_seed(0x74);
	nrf24_switch_channel(0x74);

	// power up the nrf24
	nrf24_state(POWERUP);

    nrf24_print_config();

    uint8_t data, length, iterator = 0, lost_transmissions = 0;
    char tx_message[32], rx_message[32];
	uint64_t start_time, ping;

	// send the first packet
	snprintf(tx_message, 32, "Transmitted packet %d", iterator);	// Copy string into array
	nrf24_send_message(tx_message, strlen(tx_message), nrf24_config.auto_ack);
	
    while (1) {
		
		if (nrf24_interrupt_trigger) {

            data = 0;
            nrf24_read_register(R_REGISTER | STATUS, &data, 1);

            // // handle receive interrupt
            // if ((data & (1 << RX_DR)) > 0) {

            // }
            
            // handle transmit interrupt
            if ((data & (1 << TX_DS)) > 0) {

				// read the received ACK packet
				length = nrf24_read_message(rx_message);
				rx_message[length] = 0;

        		ping = time_us_64() - start_time;
				printf("Message %d Sent: %s, Ping: %" PRIu64 "\r\n", iterator, rx_message, ping);

				// increment the iterator 
				iterator++;
				//reset the lost_transmissions counter on succesful transmissions
				lost_transmissions = 0;

				sleep_ms(INTER_PACKET_DELAY);
				nrf24_switch_channel(iterator % 2 == 0 ? 0x74 : 0x44);

				// if (iterator > 254) sleep_ms(5000);

				// send a new packet
				start_time = time_us_64();
				snprintf(tx_message, 32, "Transmitted packet %d", iterator);	// Copy string into array
				nrf24_send_message(tx_message, strlen(tx_message), nrf24_config.auto_ack);
            }
            
            // handle maximum retransimissions interrupt
            if ((data & (1 << MAX_RT)) > 0){
				// reset the interrupt
				uint8_t reset_int = (1 << MAX_RT);
				nrf24_write_register(W_REGISTER | STATUS, &reset_int, 1);

				if (lost_transmissions <= MAX_LOST_TRANSMISSIONS) {
					printf("Maximum retransmissions reached\r\n");
					printf("Message %d Lost\r\n", iterator);
					lost_transmissions++;
					iterator++;
					nrf24_write_register(W_REGISTER | FLUSH_TX, 0, 0);
				} else { // this else case indicates low channel quality and can be used to adopt a strategy for better comms
					printf("Lost more than 100 packets consecutively\r\n");
					sleep_ms(2000); // TODO: remove this in production 
					lost_transmissions = 0;
					nrf24_write_register(W_REGISTER | FLUSH_TX, 0, 0);
				}

				snprintf(tx_message, 32, "Transmitted packet %d", iterator);
				nrf24_send_message(tx_message, strlen(tx_message), nrf24_config.auto_ack);
            }
            
			//	Message received, print it
			nrf24_interrupt_trigger = false;
			
		}
    }
}