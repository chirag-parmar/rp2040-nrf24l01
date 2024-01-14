/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "nrf24l01.h"
#include "nrf24l01-mnemonics.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

void print_config(void)
{
	uint8_t config, data;
	printf("Startup successful\r\n\r\n nRF24L01+ configured as:\r\n");
	printf("-------------------------------------------\r\n");
	nrf24_read_register(R_REGISTER | CONFIG,&config,1);
	printf("CONFIG		0x%x\r\n",config);
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
volatile bool message_sent = false;
volatile bool timer_expired = false;
volatile bool status = false;

// configure nrf24
nrf24_config_t nrf24_config = {
	.crc_scheme = CRC_2_BYTE,
	.int_trigger = TX_INTERRUPT,
	.rx_mode = false,
	.auto_ack = true,
	.retr_count = 1,
	.retr_delay = RETR_DELAY_4000_US,
	.datarate = DATARATE_250Kbps,
	.power = PWR_MINUS_18_DBM,
	.cont_wave = false,
	.dynamic_payload = true,
	.rx_address = {0xe7, 0xe7, 0xe7, 0xe7, 0xe7},
	.tx_address = {0xe7, 0xe7, 0xe7, 0xe7, 0xe7},
	.enable_pipe_bit_mask = 0x01
};

int main(void) {	
	
	//	Initialize UART/USB
	stdio_init_all();
	
	//	Initialize nRF24L01+ and print configuration info
    nrf24_init(9, 8);
	nrf24_configure(&nrf24_config);
	nrf24_switch_channel(0x74);
	nrf24_state(POWERUP);

	// // timer1 start
	// timer2_start(1000);

	// int i = 0;

	// snprintf(tx_message, 32, "TX: %d", i);	// Copy string into array

	// //	Send message as response
	// nrf24_send_message(tx_message, nrf24_config.auto_ack);
	
    while (1) {

        sleep_ms(1000);
        // print nrf24 config
	    print_config();
		
		// // nrf24_read_register(R_REGISTER | STATUS, &data, 1);
		// // if ((data & (1 << MAX_RT))) {
		// // 	printf("Maximum RT reached\r\n");
		// // 	data = (1 << MAX_RT);
		// // 	nrf24_write(STATUS, &data, 1);
		// // 	nrf24_write(FLUSH_TX, 0, 0);

		// // 	snprintf(tx_message, 32, "TX: %d", i);	// Copy string into array

		// // 	//	Send message as response
		// // 	nrf24_send_message(tx_message, nrf24_config.auto_ack);
		// // }

		// if (message_sent && i < 100) {
		// 	printf("Ack Packet Received %s\r\n", nrf24_read_message(false));

		// 	message_sent = false;

		// 	snprintf(tx_message, 32, "TX: %d", i);	// Copy string into array

		// 	//	Send message as response
		// 	nrf24_send_message(tx_message, nrf24_config.auto_ack);

		// 	i++;
		// }
    }
}

// //	Used in IRQ ISR
// volatile bool message_received = false;
// volatile bool timer_expired = false;
// volatile bool status = false;

// // configure nrf24
// nrf24_config_t nrf24_config = {
// 	.crc_scheme = CRC_2_BYTE,
// 	.int_trigger = RX_INTERRUPT,
// 	.rx_mode = true,
// 	.auto_ack = true,
// 	.retr_count = 1,
// 	.retr_delay = RETR_DELAY_4000_US,
// 	.datarate = DATARATE_250Kbps,
// 	.power = PWR_0_DBM,
// 	.cont_wave = false,
// 	.dynamic_payload = true,
// 	.rx_address = {0xe7, 0xe7, 0xe7, 0xe7, 0xe7},
// 	.tx_address = {0xe7, 0xe7, 0xe7, 0xe7, 0xe7},
// 	.enable_pipe_bit_mask = 0x01
// };

// ISR(TIMER2_COMPA_vect) {
// 	timer_expired = true;
// }

// //	Interrupt on IRQ pin
// ISR(INT0_vect) {
// 	message_received = true;
// }

// int main(void)
// {	
// 	//	Initialize UART
// 	uart_init();
	
// 	//	Initialize nRF24L01+ and print configuration info
//     nrf24_init(10, 9);
// 	nrf24_configure(&nrf24_config);
// 	nrf24_switch_channel(0x74);
// 	nrf24_state(POWERUP);

// 	// print nrf24 config
// 	print_config();

// 	// timer1 start
// 	timer2_start(1000);

// 	nrf24_start_listening(nrf24_config.auto_ack);
	
//     while (1) {
		

// 		if (message_received) {
// 			//	Message received, print it
// 			message_received = false;
// 				// Write ACK message
// 			nrf24_write_ack();
// 			printf("Received message: %s\r\n",nrf24_read_message());
// 		}
//     }
// }