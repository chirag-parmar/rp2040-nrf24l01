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
volatile bool message_received = false;

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
	if(gpio == 13 && (events & GPIO_IRQ_EDGE_FALL) > 0) message_received = true;
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

    uint8_t data, length;
    char rx_message[32];

    // pre load a ACK message into the fifo
    nrf24_write_ack("ACK", strlen("ACK"));

    //start listening
	nrf24_start_listening();
	
    while (1) {
		
		nrf24_read_register(R_REGISTER | STATUS, &data, 1);
		if (message_received) {
			//	Message received, print it
			message_received = false;
			
			// pre load a ACK message in fifo for the nest receive
			nrf24_write_ack("ACK", strlen("ACK"));
			
            length = nrf24_read_message(rx_message);
            rx_message[length] = 0;  
            printf("Received message: %s\r\n",rx_message);
		}
    }
}