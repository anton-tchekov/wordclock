#include "ws2812.c"

#define WIDTH               10
#define HEIGHT              10
#define PIXELS                (WIDTH * HEIGHT)
#define BYTES                 (3 * PIXELS)

static uint8_t _pixels[BYTES];

static void pixel(uint8_t x, uint8_t y, uint8_t r, uint8_t g, uint8_t b)
{
	if(x < WIDTH && y < HEIGHT)
	{
		uint16_t i;
		x = (WIDTH - 1) - x;
		y = (HEIGHT - 1) - y;
		i = 3 * ((y % 2)
			? ((WIDTH * y) + ((WIDTH - 1) - x)) : (WIDTH * y + x));

		_pixels[i] = g;
		_pixels[++i] = r;
		_pixels[++i] = b;
	}
}

static void clear(void)
{
	memset(_pixels, 0, BYTES);
}

static void update(void)
{
	ws2812(_pixels, BYTES);
}
