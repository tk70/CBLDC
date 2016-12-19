/*
 * n11e2.h
 *
 * Created: 2014-12-18 19:40:27
 *  Author: Jakub Turowski
 */ 


#ifndef N11E2_H_
#define N11E2_H_

#define F_CPU 16000000

#define INPUT_SIGNAL_TYPE 1

#define I2C_SLAVE_ADDRESS 4
#define I2C_PROTOCOL 2

#define RC_PWM_CHANNEL	0

// Power stage control pins
// phase R
#define RL_PIN			1			// Low MOSFET
#define RL_PORT			PORTC
#define RL_DDR			DDRC
#define RL_INVERTING	0			// Does the hardware invert signal?

#define RH_PIN			3			// High MOSFET
#define RH_PORT			PORTC
#define RH_DDR			DDRC
#define RH_INVERTING	1

#define R_COMP_CHANNEL	2			// phase comparator channel (port C)

// phase S
#define SL_PIN			5
#define SL_PORT			PORTD
#define SL_DDR			DDRD
#define SL_INVERTING	0

#define SH_PIN			0
#define SH_PORT			PORTD
#define SH_DDR			DDRD
#define SH_INVERTING	1

#define S_COMP_CHANNEL	7

// phase T
#define TL_PIN			4
#define TL_PORT			PORTD
#define TL_DDR			DDRD
#define TL_INVERTING	0

#define TH_PIN			1
#define TH_PORT			PORTD
#define TH_DDR			DDRD
#define TH_INVERTING	1

#define T_COMP_CHANNEL	6


// Leds
#define LED0_DDR	DDRB
#define LED0_PORT	PORTB
#define LED0_PIN	3

#define LED1_DDR	DDRB
#define LED1_PORT	PORTB
#define LED1_PIN	4

#endif /* N11E2_H_ */