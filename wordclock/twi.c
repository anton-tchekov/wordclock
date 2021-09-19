#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <compat/twi.h>

#define TWI_FREQ          100000L
#define TWI_BUFFER_LENGTH     32
#define TWI_READY              0
#define TWI_MRX                1
#define TWI_MTX                2
#define TWI_SRX                3
#define TWI_STX                4

static void twi_init(void);
static void twi_begin_transmission(uint8_t address);
static uint8_t twi_end_transmission(void);
static uint8_t twi_request_from(uint8_t address, uint8_t quantity);
static void twi_write(uint8_t data);
static uint8_t twi_read(void);
static uint8_t twi_available(void);

/* Private Variables */
static volatile uint8_t twi_state;
static uint8_t twi_slarw;
static volatile uint8_t twi_error;

static uint8_t twi_masterBuffer[TWI_BUFFER_LENGTH];
static volatile uint8_t twi_masterBufferIndex;
static uint8_t twi_masterBufferLength;

static uint8_t rxBuffer[TWI_BUFFER_LENGTH];
static uint8_t rxBufferIndex = 0;
static uint8_t rxBufferLength = 0;

static uint8_t txAddress = 0;
static uint8_t txBuffer[TWI_BUFFER_LENGTH];
static uint8_t txBufferIndex = 0;
static uint8_t txBufferLength = 0;

/* Private Functions */
static void twi_stop(void);
static void twi_release_bus(void);
static uint8_t twi_read_from
	(uint8_t address, uint8_t *data, uint8_t length);
static uint8_t twi_write_to
	(uint8_t address, uint8_t *data, uint8_t length, uint8_t wait);
static void twi_reply_ack(void);
static void twi_reply_nack(void);

/* Public */
static void twi_init(void)
{
	rxBufferIndex = 0;
	rxBufferLength = 0;
	txBufferIndex = 0;
	txBufferLength = 0;

	/* Initialize state */
	twi_state = TWI_READY;

	/* Internal pullups on TWI pins */
	PORTC |= (1 << 4);
	PORTC |= (1 << 5);

	/* Initialize TWI prescaler and bitrate */
	TWSR &= ~TWPS0;
	TWSR &= ~TWPS1;
	TWBR = ((F_CPU / TWI_FREQ) - 16) / 2;

	/* Enable TWI module and interrupt */
	TWCR = (1 << TWEN) | (1 << TWIE) | (1 << TWEA);
}

static void twi_begin_transmission(uint8_t address)
{
	txAddress = address;
	txBufferIndex = 0;
	txBufferLength = 0;
}

static uint8_t twi_end_transmission(void)
{
	int8_t ret = twi_write_to(txAddress, txBuffer, txBufferLength, 1);
	txBufferIndex = 0;
	txBufferLength = 0;
	return ret;
}

static uint8_t twi_request_from(uint8_t address, uint8_t quantity)
{
	uint8_t read;
	if(quantity > TWI_BUFFER_LENGTH)
	{
		quantity = TWI_BUFFER_LENGTH;
	}

	read = twi_read_from(address, rxBuffer, quantity);
	rxBufferIndex = 0;
	rxBufferLength = read;
	return read;
}

static void twi_write(uint8_t data)
{
	if(txBufferLength >= TWI_BUFFER_LENGTH) { return; }
	txBuffer[txBufferIndex++] = data;
	txBufferLength = txBufferIndex;
}

static uint8_t twi_read(void)
{
	return (rxBufferIndex < rxBufferLength)
		? rxBuffer[rxBufferIndex++] : '\0';
}

static uint8_t twi_available(void)
{
	return rxBufferLength - rxBufferIndex;
}

/* Private */
static void twi_stop(void)
{
	TWCR = (1 << TWEN) | (1 << TWIE) | (1 << TWEA) | (1 << TWINT) |
		(1 << TWSTO);

	while(TWCR & _BV(TWSTO)) ;
	twi_state = TWI_READY;
}

static void twi_release_bus(void)
{
	TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA) | _BV(TWINT);
	twi_state = TWI_READY;
}

static uint8_t twi_read_from
	(uint8_t address, uint8_t *data, uint8_t length)
{
	uint8_t i;

	if(TWI_BUFFER_LENGTH < length)
	{
		return 0;
	}

	while(TWI_READY != twi_state) ;
	twi_state = TWI_MRX;
	twi_error = 0xFF;
	twi_masterBufferIndex = 0;
	twi_masterBufferLength = length - 1;
	twi_slarw = TW_READ;
	twi_slarw |= address << 1;
	TWCR = (1 << TWEN) | (1 << TWIE) | (1 << TWEA) | (1 << TWINT) |
		(1 << TWSTA);

	while(TWI_MRX == twi_state) ;

	if(twi_masterBufferIndex < length)
	{
		length = twi_masterBufferIndex;
	}

	for(i = 0; i < length; ++i)
	{
		data[i] = twi_masterBuffer[i];
	}

	return length;
}

static uint8_t twi_write_to
	(uint8_t address, uint8_t *data, uint8_t length, uint8_t wait)
{
	uint8_t i;

	if(TWI_BUFFER_LENGTH < length)
	{
		return 1;
	}

	while(TWI_READY != twi_state) ;
	twi_state = TWI_MTX;
	twi_error = 0xFF;
	twi_masterBufferIndex = 0;
	twi_masterBufferLength = length;
	for(i = 0; i < length; ++i)
	{
		twi_masterBuffer[i] = data[i];
	}

	twi_slarw = TW_WRITE;
	twi_slarw |= address << 1;
	TWCR = (1 << TWEN) | (1 << TWIE) | (1 << TWEA) | (1 << TWINT) |
		(1 << TWSTA);

	while(wait && (TWI_MTX == twi_state)) ;

	if(twi_error == 0xFF)
	{
		/* Success */
		return 0;
	}
	else if(twi_error == TW_MT_SLA_NACK)
	{
		/* Error: Address sent, NACK received */
		return 2;
	}
	else if(twi_error == TW_MT_DATA_NACK)
	{
		/* Error: Data sent, NACK received */
		return 3;
	}
	else
	{
		/* Other Error */
		return 4;
	}
}

static void twi_reply_ack(void)
{
	TWCR = (1 << TWEN) | (1 << TWIE) | (1 << TWINT) | (1 << TWEA);
}

static void twi_reply_nack(void)
{
	TWCR = (1 << TWEN) | (1 << TWIE) | (1 << TWINT);
}

SIGNAL(TWI_vect)
{
	switch(TW_STATUS)
	{
		/* All master */
		case TW_START:
			/* Sent start condition */

		case TW_REP_START:
			/* Sent repeated start condition */
			TWDR = twi_slarw;
			twi_reply_ack();
			break;

		/* Master transmitter */
		case TW_MT_SLA_ACK:
			/* Slave receiver ACKed address */

		case TW_MT_DATA_ACK:
			/* Slave receiver ACKed data */
			if(twi_masterBufferIndex < twi_masterBufferLength)
			{
				TWDR = twi_masterBuffer[twi_masterBufferIndex++];
				twi_reply_ack();
			}
			else
			{
				twi_stop();
			}
			break;

		case TW_MT_SLA_NACK:
			/* Address sent, NACK received */
			twi_error = TW_MT_SLA_NACK;
			twi_stop();
			break;

		case TW_MT_DATA_NACK:
			/* Data sent, NACK received */
			twi_error = TW_MT_DATA_NACK;
			twi_stop();
			break;

		case TW_MT_ARB_LOST:
			/* Bus arbitration lost */
			twi_error = TW_MT_ARB_LOST;
			twi_release_bus();
			break;

		/* Master receiver */
		case TW_MR_DATA_ACK:
			/* Data received, ACK sent */
			twi_masterBuffer[twi_masterBufferIndex++] = TWDR;

		case TW_MR_SLA_ACK:
			/* Address sent, ACK received */
			if(twi_masterBufferIndex < twi_masterBufferLength)
			{
				twi_reply_ack();
			}
			else
			{
				twi_reply_nack();
			}
			break;

		case TW_MR_DATA_NACK:
			/* Data received, NACK sent */
			twi_masterBuffer[twi_masterBufferIndex++] = TWDR;

		case TW_MR_SLA_NACK:
			/* Address sent, NACK received */
			twi_stop();
			break;

		/* All */
		case TW_NO_INFO:
			/* No state information */
			break;

		case TW_BUS_ERROR:
			/* Bus error, illegal stop/start */
			twi_error = TW_BUS_ERROR;
			twi_stop();
			break;
	}
}

