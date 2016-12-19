/*
 * led.h
 *
 * Created: 2014-05-09 20:56:18
 *  Author: Jakub Turowski
 */ 


#ifndef LED_H_
#define LED_H_

#include "bldc.h"
#include <avr/io.h>

#ifdef __ASSEMBLER__
	#define LED0_1 sbi _SFR_IO_ADDR(LED0_PORT), LED0_PIN
	#define LED0_0 cbi _SFR_IO_ADDR(LED0_PORT), LED0_PIN
	#define LED1_1 sbi _SFR_IO_ADDR(LED1_PORT), LED1_PIN
	#define LED1_0 cbi _SFR_IO_ADDR(LED1_PORT), LED1_PIN
#else

	#define LED0_1 LED0_PORT |= (1<<LED0_PIN)
	#define LED0_0 LED0_PORT &= ~(1<<LED0_PIN)
	#define LED1_1 LED1_PORT |= (1<<LED1_PIN)
	#define LED1_0 LED1_PORT &= ~(1<<LED1_PIN)
	#define LED1_x LED1_PORT ^= (1<<LED1_PIN)
	#define LED0_x LED0_PORT ^= (1<<LED0_PIN)
	#define LED0_INIT LED0_DDR |= 1<<LED0_PIN
	#define LED1_INIT LED1_DDR |= 1<<LED1_PIN

	inline void led_init()
	{
		LED0_INIT;
		LED1_INIT;
	}

#endif

#endif /* LED_H_ */