// standard libraries
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

// pico related libs
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/spi.h"

// nRF24L01+ include files
#include "nrf24l01.h"
#include "nrf24l01-mnemonics.h"

// PIN toggling
#define ce_low gpio_put(ce_pin, 0);
#define ce_high gpio_put(ce_pin, 1);
#define csn_low gpio_put(csn_pin, 0);
#define csn_high gpio_put(csn_pin, 1);

// Used to store SPI commands
static uint8_t ce_pin, csn_pin;

uint8_t nrf24_read_register(uint8_t register_address, uint8_t* dst, unsigned int bytes) {

	uint8_t status;
	
	csn_low;
	spi_write_read_blocking(NRF24_SPI, &register_address, &status, 1);
	if (bytes > 0) spi_read_blocking(NRF24_SPI, 0, dst, bytes);
	csn_high;
	
	return status;
}

uint8_t nrf24_write_read_register(uint8_t register_address, uint8_t* src, uint8_t* dst, unsigned int bytes) {

	uint8_t status;
	
	csn_low;
	spi_write_read_blocking(NRF24_SPI, &register_address, &status, 1);
	if (bytes > 0) spi_write_read_blocking(NRF24_SPI, src, dst, bytes);
	csn_high;
	
	return status;
}

uint8_t nrf24_write_register(uint8_t register_address, uint8_t* src, unsigned int bytes) {

	uint8_t status;
	
	csn_low;
	spi_write_read_blocking(NRF24_SPI, &register_address, &status, 1);
	if (bytes > 0)spi_write_blocking(NRF24_SPI, src, bytes);
	csn_high

	return status;
}

void nrf24_init(uint8_t sck, uint8_t mosi, uint8_t miso, uint8_t csn, uint8_t ce, uint8_t irq) {

	gpio_init(ce);
    gpio_set_dir(ce, GPIO_OUT);
	gpio_put(ce, 0);

	gpio_init(csn);
    gpio_set_dir(csn, GPIO_OUT);
	gpio_put(csn, 1);

	gpio_set_irq_enabled(irq, GPIO_IRQ_EDGE_FALL, true);
	irq_set_enabled(IO_IRQ_BANK0, true);

	//save pins
	ce_pin = ce;
	csn_pin = csn;
	
	// Initialize SPI
	spi_init(NRF24_SPI, 16000 * 1000);
	spi_set_format(NRF24_SPI, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    gpio_set_function(sck, GPIO_FUNC_SPI);
    gpio_set_function(miso, GPIO_FUNC_SPI);
    gpio_set_function(mosi, GPIO_FUNC_SPI);

    // // Make the SPI pins available to picotool
    // bi_decl(bi_4pins_with_func(12, 11, 10, GPIO_FUNC_SPI));

	// Power on reset 100ms delay
	sleep_ms(100);

}

void nrf24_configure(nrf24_config_t* config) {
	// Start nRF24L01+ config
	uint8_t data =
	((config->int_trigger) << MASK_MAX_RT) |	// IRQ interrupt masking
	((config->crc_scheme) << CRC0) |			// CRC enable and scheme selection
	((config->rx_mode) << PRIM_RX);					// TX/RX select
	nrf24_write_register(W_REGISTER | CONFIG, &data, 1);
	
	// Auto-acknowledge on all pipes
	data =
	((config->auto_ack) << ENAA_P5) |
	((config->auto_ack) << ENAA_P4) |
	((config->auto_ack) << ENAA_P3) |
	((config->auto_ack) << ENAA_P2) |
	((config->auto_ack) << ENAA_P1) |
	((config->auto_ack) << ENAA_P0);
	data = data & config->enable_pipe_bit_mask;
	nrf24_write_register(W_REGISTER | EN_AA, &data, 1);
	
	// Set retries
	data = (config->retr_delay << 4) | (config->retr_count & 0x0F);
	nrf24_write_register(W_REGISTER | SETUP_RETR, &data, 1);
	
	// Disable RX addresses
	data = 0;
	nrf24_write_register(W_REGISTER | EN_RXADDR, &data, 1);
	
	// Setup
	data =
	((config->cont_wave) << CONT_WAVE) |					// Continuous carrier transmit
	((config->datarate) << RF_DR_HIGH) |	// Data rate
	((config->power) << RF_PWR);				// PA level
	nrf24_write_register(W_REGISTER | RF_SETUP, &data, 1);
	
	// Status - clear TX/RX Interrupt flags and MAX_RT Interrupt flags by writing 1 into them
	data =
	(1 << RX_DR) |								// RX FIFO
	(1 << TX_DS) |								// TX FIFO
	(1 << MAX_RT);								// MAX RT
	nrf24_write_register(W_REGISTER | STATUS, &data, 1);
	
	// Dynamic payload on all pipes
	data =
	((config->dynamic_payload) << DPL_P0) |
	((config->dynamic_payload) << DPL_P1) |
	((config->dynamic_payload) << DPL_P2) |
	((config->dynamic_payload) << DPL_P3) |
	((config->dynamic_payload) << DPL_P4) |
	((config->dynamic_payload) << DPL_P5);
	data = data & config->enable_pipe_bit_mask;
	nrf24_write_register(W_REGISTER | DYNPD, &data, 1);

	// Enable dynamic payload
	data =
	((config->dynamic_payload) << EN_DPL) |
	((config->auto_ack) << EN_ACK_PAY) |
	((config->auto_ack) << EN_DYN_ACK);
	nrf24_write_register(W_REGISTER | FEATURE, &data, 1);
	
	// Flush TX/RX
	// Clear RX FIFO which will reset interrupt
	nrf24_write_register(W_REGISTER | FLUSH_RX, 0, 0);
	nrf24_write_register(W_REGISTER | FLUSH_TX, 0, 0);
	
	// Open pipes
	nrf24_write_register(W_REGISTER | RX_ADDR_P0, config->rx_address, 5);
	nrf24_write_register(W_REGISTER | TX_ADDR, config->tx_address, 5);
	nrf24_write_register(W_REGISTER | EN_RXADDR, &(config->enable_pipe_bit_mask), 1);

	if (config->enable_pipe_bit_mask > 1) {
		nrf24_write_register(W_REGISTER | RX_ADDR_P1, config->rx_address_p1, 5);
		nrf24_write_register(W_REGISTER | RX_ADDR_P2, &(config->rx_address_p2), 1);
		nrf24_write_register(W_REGISTER | RX_ADDR_P3, &(config->rx_address_p3), 1);
		nrf24_write_register(W_REGISTER | RX_ADDR_P4, &(config->rx_address_p4), 1);
		nrf24_write_register(W_REGISTER | RX_ADDR_P5, &(config->rx_address_p5), 1);
	}
}

void nrf24_switch_channel(uint8_t channel) {
	nrf24_write_register(W_REGISTER | RF_CH, &channel, 1);
}

uint8_t rpd_status(void) {

	uint8_t rpd_status;
	nrf24_read_register(R_REGISTER | RPD, &rpd_status, 1);
	return rpd_status;
}

void nrf24_write_ack(uint8_t* ack_data, uint8_t length) {

	nrf24_write_register(W_REGISTER | FLUSH_TX, 0, 0);

	// ACK packet
	nrf24_write_register(W_ACK_PAYLOAD, ack_data, length);

	// write a null character
	// uint8_t data = 0;
	// spi_write_blocking(NRF24_SPI, &data, 1);
}

void nrf24_state(uint8_t state) {

	uint8_t config_register, data;
	nrf24_read_register(R_REGISTER | CONFIG, &config_register, 1);
	
	switch (state)
	{
		case POWERUP:
			// Check if already powered up
			if (!(config_register & (1 << PWR_UP)))
			{
				data = config_register | (1 << PWR_UP);
				nrf24_write_register(W_REGISTER | CONFIG, &data, 1);
				// 1.5ms from POWERDOWN to start up
				sleep_ms(2);
			}
			break;
		case POWERDOWN:
			data = config_register & ~(1 << PWR_UP);
			nrf24_write_register(W_REGISTER | CONFIG, &data, 1);
			break;
		case RECEIVE:
			data = config_register | (1 << PRIM_RX);
			nrf24_write_register(W_REGISTER | CONFIG, &data, 1);
			// Clear STATUS register
			data = (1 << RX_DR) | (1 << TX_DS) | (1 << MAX_RT);
			nrf24_write_register(W_REGISTER | STATUS, &data, 1);
			break;
		case TRANSMIT:
			data = config_register & ~(1 << PRIM_RX);
			nrf24_write_register(W_REGISTER | CONFIG, &data, 1);
			break;
		case STANDBY1:
			ce_low;
			break;
		case STANDBY2:
			data = config_register & ~(1 << PRIM_RX);
			nrf24_write_register(W_REGISTER | CONFIG, &data, 1);
			ce_high;
			sleep_us(150);
			break;
	}
}

void nrf24_start_listening() {

	nrf24_state(RECEIVE);				// Receive mode
	ce_high;
	sleep_us(150);						// Settling time
}

void nrf24_send_message(uint8_t *tx_message, uint8_t length, bool auto_ack) {	

	// clear TX interrupt
	uint8_t data = (1 << TX_DS);
	nrf24_write_register(W_REGISTER | STATUS, &data, 1);
	
	// Start SPI, load message into TX_PAYLOAD
	if (auto_ack) nrf24_write_register(W_TX_PAYLOAD, tx_message, length);
	else  nrf24_write_register(W_TX_PAYLOAD_NOACK, tx_message, length);

	// write a null character
	// data = 0;
	// spi_write_blocking(NRF24_SPI, &data, 1);
	
	// Send message by pulling CE high for more than 10us
	ce_high;
	sleep_us(15);
	ce_low;

}

unsigned int nrf24_available(void) {

	uint8_t config_register;
	nrf24_read_register(R_REGISTER | FIFO_STATUS, &config_register, 1);

	if (!(config_register & (1 << RX_EMPTY))) return 1;
	return 0;
}

uint8_t nrf24_read_message(uint8_t* rx_message) {

	uint8_t length;

	// Get length of incoming message
	nrf24_read_register(R_REGISTER | R_RX_PL_WID, &length, 1);
	
	// Read message
	if (length > 0) nrf24_read_register(R_RX_PAYLOAD, rx_message, length);
	
	// Clear RX interrupt
	uint8_t data = (1 << RX_DR);
	nrf24_write_register(W_REGISTER | STATUS, &data, 1);
	
	return length;
}

void nrf24_print_config(void)
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
	nrf24_read_register(R_REGISTER | FIFO_STATUS,&data,1);
	printf("FIFO		0x%x\r\n",data);
	printf("-------------------------------------------\r\n\r\n");
}