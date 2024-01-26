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

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

void print_config(void)
{
	uint8_t data;
	printf("Startup successful\r\n\r\n nRF24L01+ configured as:\r\n");
	printf("-------------------------------------------\r\n");
	nrf24_read_register(R_REGISTER | CONFIG,&data,1);
	printf("CONFIG		0x%x\r\n",data);
	nrf24_read_register(R_REGISTER | EN_AA,&data,1);
	printf("EN_AA		0x%x\r\n",data);
	nrf24_read_register(R_REGISTER | EN_RXADDR,&data,1);
	printf("EN_RXADDR	0x%x\r\n",data);
	nrf24_read_register(R_REGISTER | SETUP_RETR,&data,1);
	printf("SETUP_RETR	0x%x\r\n",data);
	nrf24_read_register(R_REGISTER | RF_CH,&data,1);
	printf("RF_CH		0x%x\r\n",data);
	nrf24_read_register(R_REGISTER | RF_SETUP,&data,1);
	printf("RF_SETUP	0x%x\r\n",data);
	nrf24_read_register(R_REGISTER | STATUS,&data,1);
	printf("STATUS		0x%x\r\n",data);
	nrf24_read_register(R_REGISTER | FEATURE,&data,1);
	printf("FEATURE		0x%x\r\n",data);
	printf("-------------------------------------------\r\n\r\n");
}

#define HOP_FREQUENCY 250 // minimum is 250

//	Used in IRQ ISR
volatile bool message_sent = true;

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
	if(gpio == 13 && (events & GPIO_IRQ_EDGE_FALL) > 0) message_sent = true;
}

int main(void) {	
	
	//	Initialize UART/USB
	stdio_init_all();

	sleep_ms(5000);

    printf("Starting the RP2040 Tx");
	
	// Initialize nRF24L01+ and print configuration info
    nrf24_init(10, 11, 12, 9, 8, 13);
	nrf24_configure(&nrf24_config);
	nrf24_switch_channel(0x74);
	nrf24_state(POWERUP);

    gpio_set_irq_callback(gpio_callback);

    print_config();

    uint8_t data, iterator = 0;
    char tx_message[32], rx_message[32];
	snprintf(tx_message, 32, "Transmitted packet %d", iterator);	// Copy string into array

	nrf24_send_message(tx_message, strlen(tx_message), nrf24_config.auto_ack);;
	
    while (1) {
		
		nrf24_read_register(R_REGISTER | STATUS, &data, 1);
		// if ((data & (1 << MAX_RT))) {
		// 	printf("Maximum RT reached\r\n");
		// 	data = (1 << MAX_RT);
		// 	nrf24_write(STATUS, &data, 1);
		// 	nrf24_write(FLUSH_TX, 0, 0);

		// 	snprintf(tx_message, 32, "TX: %d", i);	// Copy string into array

		// 	//	Send message as response
		// 	nrf24_send_message(tx_message, nrf24_config.auto_ack);
		// }


		if (message_sent) {
            message_sent = false;

            printf("Message sent %d\r\n", iterator);

			snprintf(tx_message, 32, "Transmitted packet %d", iterator);	// Copy string into array

			sleep_ms(1000);

			//	Send message as response
			nrf24_send_message(tx_message, strlen(tx_message), nrf24_config.auto_ack);

			iterator++;
		}
    }
}