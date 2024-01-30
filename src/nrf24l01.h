#ifndef _NRF24L01_H
#define _NRF24L01_H

//	States
#define POWERUP		1
#define POWERDOWN	2
#define RECEIVE		3
#define TRANSMIT	4
#define STANDBY1	5
#define STANDBY2	6

#ifndef NRF24_SPI
#warning "Setting default SPI instance to spi1"
#define NRF24_SPI spi1
#endif

typedef enum {          // en|2byte?
    NO_CRC      = 0x00, // 0b00
    CRC_1_BYTE  = 0x02, // 0b10
    CRC_2_BYTE  = 0x03  // 0b11
} nrf24_crc_t;

typedef enum {           // ~rx|~tx|~rt
    RX_INTERRUPT = 0x03, // 0b011
    TX_INTERRUPT = 0x05, // 0b101
    RT_INTERRUPT = 0x06, // 0b110
    NO_INTERRUPT = 0x07  // 0b111
} nrf24_int_t;

typedef enum {
    RETR_DELAY_250_US,
    RETR_DELAY_500_US,
    RETR_DELAY_750_US,
    RETR_DELAY_1000_US,
    RETR_DELAY_1250_US,
    RETR_DELAY_1500_US,
    RETR_DELAY_1750_US,
    RETR_DELAY_2000_US,
    RETR_DELAY_2250_US,
    RETR_DELAY_2500_US,
    RETR_DELAY_2750_US,
    RETR_DELAY_3000_US,
    RETR_DELAY_3250_US,
    RETR_DELAY_3500_US,
    RETR_DELAY_3750_US,
    RETR_DELAY_4000_US
} nrf24_retr_delay_t;

typedef enum {               // RF_DR_LOW:PLL_LOCK:RF_DR_HIGH 
    DATARATE_250Kbps = 0x04, // 0b100
    DATARATE_1Mbps = 0x00,   // 0b000
    DATARATE_2Mbps = 0x01    // 0b001
} nrf24_datarate_t;

typedef enum {
    PWR_0_DBM = 0x03,
    PWR_MINUS_6_DBM = 0x02,
    PWR_MINUS_12_DBM = 0x01,
    PWR_MINUS_18_DBM = 0x00
} nrf24_pwr_t;
typedef struct {
    nrf24_crc_t crc_scheme;
    nrf24_int_t int_trigger;
    bool rx_mode;
    bool auto_ack;
    uint8_t retr_count;
    nrf24_retr_delay_t retr_delay;
    nrf24_datarate_t datarate;
    nrf24_pwr_t power;
    bool cont_wave;
    bool dynamic_payload;
    uint8_t rx_address[5];
    uint8_t tx_address[5];
    uint8_t enable_pipe_bit_mask;
    uint8_t rx_address_p1[5];
    uint8_t rx_address_p2;
    uint8_t rx_address_p3;
    uint8_t rx_address_p4;
    uint8_t rx_address_p5;
} nrf24_config_t;

//	Forward declarations
uint8_t nrf24_read_register(uint8_t register_address, uint8_t* dst, unsigned int bytes);
uint8_t nrf24_write_read_register(uint8_t register_address, uint8_t* src, uint8_t* dst, unsigned int bytes);
uint8_t nrf24_write_register(uint8_t register_address, uint8_t* src, unsigned int bytes);

uint8_t rpd_status(void);

void nrf24_init(uint8_t sck, uint8_t mosi, uint8_t miso, uint8_t csn, uint8_t ce, uint8_t irq);
void nrf24_configure(nrf24_config_t* config);
void nrf24_switch_channel(uint8_t channel);
void nrf24_state(uint8_t state);

void nrf24_start_listening();
unsigned int nrf24_available(void);
uint8_t nrf24_read_message(uint8_t* rx_message);

void nrf24_send_message(uint8_t *tx_message, uint8_t length, bool auto_ack);
void nrf24_write_ack(uint8_t* ack_data, uint8_t length);

void nrf24_print_config();

#endif /*_NRF24L01_H*/
