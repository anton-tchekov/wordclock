#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "ds3231.c"
#include "board.c"

typedef struct WORD
{
	uint8_t StartX, StartY, Length;
} Word;

static const Word
	fuenf   = {  1,  0,  4 }, /* fuenf */
	zehn    = {  6,  0,  4 }, /* zehn */
	viertel = {  2,  2,  7 }, /* viertel */
	zwanzig = {  1,  1,  7 }, /* zwanzig */
	nach    = {  1,  3,  4 }, /* nach */
	vor     = {  6,  3,  3 }, /* vor */
	halb    = {  0,  4,  4 }, /* halb */
	hours[12] =
	{
		{  6,  4,  4 }, /* eins */
		{  4,  4,  4 }, /* zwei */
		{  6,  6,  4 }, /* drei */
		{  1,  8,  4 }, /* vier */
		{  5,  5,  4 }, /* fuenf */
		{  5,  9,  5 }, /* sechs */
		{  0,  6,  6 }, /* sieben */
		{  5,  8,  4 }, /* acht */
		{  5,  7,  4 }, /* neun */
		{  1,  9,  4 }, /* zehn */
		{  2,  7,  3 }, /* elf */
		{  1,  5,  5 }, /* zwoelf */
	};

static void word_show(const Word *word)
{
	uint8_t i;
	for(i = 0; i < word->Length; ++i)
	{
		pixel(word->StartY, word->StartX + i, 0, 0, 255);
	}
}

static void time_show(uint8_t hour, uint8_t minute)
{
	const Word *this, *next;

	uint8_t m, h;

	/* Twelve hour time */
	h = hour % 12;

	/* 5-minute time */
	m = (minute + 2) / 5;

	if(m >= 12)
	{
		m = 0;
		++h;
	}

	if(h >= 12)
	{
		h = 0;
	}

	this = &hours[h];
	if(++h >= 12)
	{
		h = 0;
	}

	next = &hours[h];

	switch(m)
	{
		case 0: /* 00 */
			word_show(this);
			break;

		case 1: /* 05 */
			word_show(&fuenf);
			word_show(&nach);
			word_show(this);
			break;

		case 2: /* 10 */
			word_show(&zehn);
			word_show(&nach);
			word_show(this);
			break;

		case 3: /* 15 */
			word_show(&viertel);
			word_show(&nach);
			word_show(this);
			break;

		case 4: /* 20 */
			word_show(&zwanzig);
			word_show(&nach);
			word_show(this);
			break;

		case 5: /* 25 */
			word_show(&fuenf);
			word_show(&vor);
			word_show(&halb);
			word_show(next);
			break;

		case 6: /* 30 */
			word_show(&halb);
			word_show(next);
			break;

		case 7: /* 35 */
			word_show(&fuenf);
			word_show(&nach);
			word_show(&halb);
			word_show(next);
			break;

		case 8: /* 40 */
			word_show(&zwanzig);
			word_show(&vor);
			word_show(next);
			break;

		case 9: /* 45 */
			word_show(&viertel);
			word_show(&vor);
			word_show(next);
			break;

		case 10: /* 50 */
			word_show(&zehn);
			word_show(&vor);
			word_show(next);
			break;

		case 11: /* 55 */
			word_show(&fuenf);
			word_show(&vor);
			word_show(next);
			break;
	}
}

int main(void)
{
	uint8_t old_min = 0;

	DateTime dt;
	twi_init();
	dt.year = 2019;
	dt.mon = 5;
	dt.mday = 15;
	dt.hour = 2;
	dt.min = 54;
	dt.sec = 0;
	dt.wday = 3;
	rtc_set_datetime(&dt);

	rtc_get_datetime(&dt);
	clear();
	time_show(dt.hour, dt.min);
	update();

	for(;;)
	{
		rtc_get_datetime(&dt);
		if(dt.min != old_min)
		{
			old_min = dt.min;

			clear();
			time_show(dt.hour, dt.sec);
			update();
		}
	}

	return 0;
}

