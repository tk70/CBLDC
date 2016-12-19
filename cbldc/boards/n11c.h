/*
 * n11c.h
 *
 * Created: 2014-12-18 19:40:27
 *  Author: Jakub Turowski
 */ 


#ifndef N11C_H_
#define N11C_H_

#define F_CPU 16000000UL

#define INPUT_SIGNAL_TYPE 1

#define I2C_SLAVE_ADDRESS 4
#define I2C_PROTOCOL 2

#define RC_PWM_PORT PORTD
#define RC_PWM_PIN PIND
#define RC_PWM_DDR DDRD
#define RC_PWM_P 2
#define RC_PWM_PIN_INTERRUPT 1


// Power stage control pins
// phase R
#define RL_PIN		PD4
#define RL_PORT 		PORTD
#define RL_DDR 		DDRD
#define RL_INVERTING 	0

#define RH_PIN		PD1
#define RH_PORT 		PORTD
#define RH_DDR 		DDRD
#define RH_INVERTING 	1

#define R_COMP_CHANNEL	 2

// phase S
#define SL_PIN 		PD3
#define SL_PORT 		PORTD
#define SL_DDR 		DDRD
#define SL_INVERTING 	0

#define SH_PIN 		PB1
#define SH_PORT 		PORTB
#define SH_DDR 		DDRB
#define SH_INVERTING 	1

#define S_COMP_CHANNEL	 1

// phase T
#define TL_PIN		PB2
#define TL_PORT 		PORTB
#define TL_DDR 		DDRB
#define TL_INVERTING 	0

#define TH_PIN		PB3
#define TH_PORT 		PORTB
#define TH_DDR 		DDRB
#define TH_INVERTING 	1

#define T_COMP_CHANNEL	 0



// Leds
#define LED0_DDR		DDRB
#define LED0_PORT		PORTB
#define LED0_PIN		PB0

#define LED1_DDR		DDRB
#define LED1_PORT		PORTB
#define LED1_PIN		PB4

#endif /* N11C_H_ */