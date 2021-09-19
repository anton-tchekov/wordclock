#include <stdint.h>
#include <avr/io.h>
#include "twi.c"

typedef struct
{
	uint8_t sec, min, hour, mday, mon, wday;
	uint16_t year;
} DateTime;

static void rtc_get_datetime(DateTime *dt);
static void rtc_set_datetime(DateTime *dt);
static void ds3231_get_temperature(int8_t *i, uint8_t *f);

#define DS3231_I2C_ADDR 0x68

static uint8_t _dec2bcd(uint8_t d)
{
	return (d / 10 * 16) + (d % 10);
}

static uint8_t _bcd2dec(uint8_t b)
{
	return (b / 16 * 10) + (b % 16);
}

static void rtc_get_datetime(DateTime *dt)
{
	uint8_t i;
	uint8_t rtc[9];
	twi_begin_transmission(DS3231_I2C_ADDR);
	twi_write(0x00);
	twi_end_transmission();
	twi_request_from(DS3231_I2C_ADDR, 7);
	for(i = 0; i < 7; i++) { rtc[i] = twi_read(); }
	twi_end_transmission();
	dt->sec = _bcd2dec(rtc[0]);
	dt->min = _bcd2dec(rtc[1]);
	dt->hour = _bcd2dec(rtc[2]);
	dt->mday = _bcd2dec(rtc[4]);
	dt->mon = _bcd2dec(rtc[5] & 0x1F);
	dt->year = ((rtc[5] & 0x80) >> 7) == 1 ? 2000 + _bcd2dec(rtc[6]) :
		1900 + _bcd2dec(rtc[6]);
	dt->wday = _bcd2dec(rtc[3]);
}

static void rtc_set_datetime(DateTime *dt)
{
	uint8_t century;
	twi_begin_transmission(DS3231_I2C_ADDR);
	twi_write(0x00);
	if(dt->year > 2000)
	{
		century = 0x80;
		dt->year -= 2000;
	}
	else
	{
		century = 0;
		dt->year -= 1900;
	}

	twi_write(_dec2bcd(dt->sec));
	twi_write(_dec2bcd(dt->min));
	twi_write(_dec2bcd(dt->hour));
	twi_write(_dec2bcd(dt->wday));
	twi_write(_dec2bcd(dt->mday));
	twi_write(_dec2bcd(dt->mon) + century);
	twi_write(_dec2bcd(dt->year));
	twi_end_transmission();
}

/* i = Integer part, f = fractional part
Conversion to float: ((((short)msb << 8) | (short)lsb) >> 6) / 4.0f */
static void ds3231_get_temperature(int8_t *i, uint8_t *f)
{
	uint8_t msb, lsb;
	*i = 0;
	*f = 0;
	twi_begin_transmission(DS3231_I2C_ADDR);
	twi_write(0x11);
	twi_end_transmission();
	twi_request_from(DS3231_I2C_ADDR, 2);
	if(twi_available())
	{
		msb = twi_read();
		lsb = twi_read();
		*i = msb;
		*f = (lsb >> 6) * 25;
	}
}

