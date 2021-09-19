#include <stdint.h>
#include <avr/io.h>

#define WS2812_OUT        PORTD
#define WS2812_DIR        DDRD
#define WS2812_PIN        PD2

#define w_zeropulse    350
#define w_onepulse     900
#define w_totalperiod 1250
#define w_fixedlow       2
#define w_fixedhigh      4
#define w_fixedtotal     8

#define w_zerocycles      (((F_CPU / 1000) * w_zeropulse) / 1000000)
#define w_onecycles       (((F_CPU / 1000) * w_onepulse + 500000) / 1000000)
#define w_totalcycles     (((F_CPU / 1000) * w_totalperiod + 500000) / 1000000)

#define w1                (w_zerocycles - w_fixedlow)
#define w2                (w_onecycles - w_fixedhigh - w1)
#define w3                (w_totalcycles - w_fixedtotal - w1 - w2)

#if (w1 > 0)
#define w1_nops w1
#else
#define w1_nops  0
#endif

#define w_lowtime (((w1_nops + w_fixedlow) * 1000000) / (F_CPU / 1000))

#if (w_lowtime > 550)
#error "F_CPU to low"
#elif (w_lowtime > 450)
#warning "Critical timing"
#endif   

#if (w2 > 0)
#define w2_nops w2
#else
#define w2_nops  0
#endif

#if (w3 > 0)
#define w3_nops w3
#else
#define w3_nops  0
#endif

#define w_nop1  "nop      \n\t"
#define w_nop2  "rjmp .+0 \n\t"
#define w_nop4  w_nop2 w_nop2
#define w_nop8  w_nop4 w_nop4
#define w_nop16 w_nop8 w_nop8

static void ws2812(uint8_t *pixels, uint16_t count)
{
	uint8_t b, c, h, l, s;
	h = (1 << WS2812_PIN);
	WS2812_DIR |= h;
	l = ~h & WS2812_OUT;
	h |= WS2812_OUT;
	s = SREG;
	asm volatile ("cli");
	while(count--)
	{
		b = *pixels++;
		asm volatile
		(
			"       ldi   %0,8  \n\t"
			"loop%=:            \n\t"
			"       out   %2,%3 \n\t"
#if (w1_nops & 1)
w_nop1
#endif
#if (w1_nops & 2)
w_nop2
#endif
#if (w1_nops & 4)
w_nop4
#endif
#if (w1_nops & 8)
w_nop8
#endif
#if (w1_nops & 16)
w_nop16
#endif
			"       sbrs  %1,7  \n\t"
			"       out   %2,%4 \n\t"
			"       lsl   %1    \n\t"
#if (w2_nops & 1)
w_nop1
#endif
#if (w2_nops & 2)
w_nop2
#endif
#if (w2_nops & 4)
w_nop4
#endif
#if (w2_nops & 8)
w_nop8
#endif
#if (w2_nops & 16)
w_nop16 
#endif
			"       out   %2,%4 \n\t"
#if (w3_nops & 1)
w_nop1
#endif
#if (w3_nops & 2)
w_nop2
#endif
#if (w3_nops & 4)
w_nop4
#endif
#if (w3_nops & 8)
w_nop8
#endif
#if (w3_nops & 16)
w_nop16
#endif
			"       dec   %0    \n\t"
			"       brne  loop%=\n\t"
			:	"=&d" (c)
			:	"r" (b),
				"I" (_SFR_IO_ADDR(WS2812_OUT)),
				"r" (h),
				"r" (l)
		);
	}
  
	SREG = s;
}
